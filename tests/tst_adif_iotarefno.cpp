#include <QtTest>

#include <string>

#include "AdifIOTARefNo.h"
#include "constants/continent_map.h"

class TestAdifIOTARefNo : public QObject {
    Q_OBJECT

  private slots:
    void test_check_valid_invalid_and_take_paths();
};

void TestAdifIOTARefNo::test_check_valid_invalid_and_take_paths() {
    QVERIFY(!ADIF::CONTINENT_MAP.empty());

    // Valid with leading zeros; also validates continent lookup.
    QVERIFY(AdifIOTARefNo::check("EU-001"));
    QVERIFY(AdifIOTARefNo::check("eu-099")); // mixed case continent accepted

    // Range invalid: 000 is outside [1..999].
    QVERIFY(!AdifIOTARefNo::check("EU-000"));

    // Format invalid: missing hyphen / hyphen at wrong position.
    QVERIFY(!AdifIOTARefNo::check("EU001"));
    QVERIFY(!AdifIOTARefNo::check("E-U-001"));

    // Format invalid: numeric part must be exactly 3 chars.
    QVERIFY(!AdifIOTARefNo::check("EU-01"));
    QVERIFY(!AdifIOTARefNo::check("EU-0010"));

    // Invalid continent code.
    QVERIFY(!AdifIOTARefNo::check("XX-001"));

    // Numeric part must be all digits.
    QVERIFY(!AdifIOTARefNo::check("EU-0A1"));

    // take success + normalization to upper-case
    auto objOpt = AdifIOTARefNo::create("eu-001");
    QVERIFY(objOpt.has_value());
    AdifIOTARefNo obj = *objOpt;
    QCOMPARE(QString::fromStdString(obj.get()), QStringLiteral("EU-001"));

    const auto resOk = obj.take(std::string("na-123"));
    QVERIFY(resOk.flag);
    QVERIFY(!resOk.failed_store.has_value());
    QCOMPARE(QString::fromStdString(obj.get()), QStringLiteral("NA-123"));

    // take failure should not modify existing raw value.
    const std::string prev = obj.get();
    const auto resBad = obj.take(std::string("EU-000"));
    QVERIFY(!resBad.flag);
    QVERIFY(resBad.failed_store.has_value());
    QCOMPARE(resBad.failed_store.value(), std::string("EU-000"));
    QCOMPARE(QString::fromStdString(obj.get()), QString::fromStdString(prev));
}

QTEST_MAIN(TestAdifIOTARefNo)
#include "tst_adif_iotarefno.moc"
