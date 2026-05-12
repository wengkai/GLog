#include <QtTest>

#include <cctype>
#include <string>
#include <vector>

#include "AdifCreditList.h"
#include "constants/adif_constants.h"

namespace {

std::string toUpperAscii(std::string s) {
    for (char &c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string makeMixedCase(std::string s) {
    // Flip the first alphabetic character to lower-case so we exercise the normalize-to-uppercase
    // path.
    for (char &c : s) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalpha(uc)) {
            c = static_cast<char>(std::tolower(uc));
            break;
        }
    }
    return s;
}

} // namespace

class TestAdifCreditList : public QObject {
    Q_OBJECT

  private slots:
    void test_check_create_take_pureCredit_and_creditWithMedia();
    void test_check_invalid_variants();
};

void TestAdifCreditList::test_check_create_take_pureCredit_and_creditWithMedia() {
    QVERIFY(!ADIF::CREDIT_MAP.empty());
    QVERIFY(!ADIF::QSL_MEDIUM_MAP.empty());

    const std::string creditKey = ADIF::CREDIT_MAP.begin()->first;
    const std::string expectedUpperCredit = toUpperAscii(creditKey);
    const std::string mixedCredit = makeMixedCase(creditKey);
    QCOMPARE(toUpperAscii(mixedCredit), expectedUpperCredit);

    QVERIFY(AdifCreditList::check(mixedCredit));
    auto objOpt = AdifCreditList::create(mixedCredit);
    QVERIFY(objOpt.has_value());
    AdifCreditList obj = *objOpt;
    QCOMPARE(QString::fromStdString(obj.get()), QString::fromStdString(expectedUpperCredit));

    // Credit:Medium1&Medium2...
    std::vector<std::string> mediums;
    mediums.reserve(ADIF::QSL_MEDIUM_MAP.size());
    for (const auto &kv : ADIF::QSL_MEDIUM_MAP) {
        mediums.push_back(kv.first);
    }
    QVERIFY(mediums.size() >= 2);

    const std::string medium1 = mediums[0];
    const std::string medium2 = mediums[1];
    const std::string medium3 = (mediums.size() >= 3) ? mediums[2] : mediums[1];

    const std::string inputMixed =
        makeMixedCase(creditKey) + ":" + makeMixedCase(medium1) + "&" + makeMixedCase(medium2);
    QVERIFY(AdifCreditList::check(inputMixed));

    auto objOpt2 = AdifCreditList::create(inputMixed);
    QVERIFY(objOpt2.has_value());
    AdifCreditList obj2 = *objOpt2;
    QCOMPARE(QString::fromStdString(obj2.get()), QString::fromStdString(toUpperAscii(inputMixed)));

    // take success
    const std::string validNew = creditKey + ":" + medium1 + "&" + medium3;
    const std::string expectedUpperNew = toUpperAscii(validNew);
    const auto res = obj2.take(std::string(validNew));
    QVERIFY2(res.flag, "Expected take(valid) to succeed");
    QVERIFY(!res.failed_store.has_value());
    QCOMPARE(QString::fromStdString(obj2.get()), QString::fromStdString(expectedUpperNew));

    // take failure
    const std::string invalidNew = "INVALID_CREDIT_LIST";
    const std::string prevRaw = obj2.get();
    const auto res2 = obj2.take(std::string(invalidNew));
    QVERIFY(!res2.flag);
    QVERIFY(res2.failed_store.has_value());
    QCOMPARE(res2.failed_store.value(), invalidNew);
    QCOMPARE(QString::fromStdString(obj2.get()), QString::fromStdString(prevRaw));
}

void TestAdifCreditList::test_check_invalid_variants() {
    const std::string creditKey = ADIF::CREDIT_MAP.begin()->first;
    const std::string medium1 = ADIF::QSL_MEDIUM_MAP.begin()->first;

    // No colon; and not a pure credit.
    QVERIFY(!AdifCreditList::check("NOT_A_REAL_CREDIT"));

    // Colon present but empty medium list.
    QVERIFY(!AdifCreditList::check(creditKey + ":"));

    // Unknown credit part.
    QVERIFY(!AdifCreditList::check(std::string("NOT_A_CREDIT:") + medium1));

    // Unknown medium token.
    QVERIFY(!AdifCreditList::check(creditKey + ":NOT_A_MEDIUM"));

    // '&' trailing empty token.
    QVERIFY(!AdifCreditList::check(creditKey + ":" + medium1 + "&"));

    // Empty token between '&&' (i.e. consecutive separators).
    auto it = ADIF::QSL_MEDIUM_MAP.begin();
    ++it;
    QVERIFY(it != ADIF::QSL_MEDIUM_MAP.end());
    const std::string medium2 = it->first;
    QVERIFY(!AdifCreditList::check(creditKey + ":" + medium1 + "&&" + medium2));
}

QTEST_MAIN(TestAdifCreditList)
#include "tst_adif_creditlist.moc"
