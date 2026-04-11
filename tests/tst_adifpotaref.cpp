#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifPOTARef : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_NormalizationAndSet();
};

void TestAdifPOTARef::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("Basic K-5033") << "K-5033"
                                  << "K-5033";

    QTest::newRow("5-digit number") << "K-10000"
                                    << "K-10000";

    QTest::newRow("Short prefix") << "G-1234"
                                  << "G-1234";

    QTest::newRow("4-char prefix") << "3DA0-1111"
                                   << "3DA0-1111";

    QTest::newRow("With Extension") << "VE-5082@CA-AB"
                                    << "VE-5082@CA-AB";
    QTest::newRow("With Extension US") << "K-4562@US-CA"
                                       << "K-4562@US-CA";

    QTest::newRow("Lowercase input") << "vk-0556"
                                     << "VK-0556";
}

void TestAdifPOTARef::test_ValidInputs() {
    QFETCH(QString, input);
    QFETCH(QString, expected);

    std::string raw = input.toStdString();

    QVERIFY2(AdifPOTARef::check(raw), "Check should accept valid POTA references");

    auto obj = AdifPOTARef::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), expected);
}

void TestAdifPOTARef::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Too short") << "K-123";            // 5 chars
    QTest::newRow("Too long") << "K-12345@ABCDEFGHI"; // > 17 chars

    QTest::newRow("Missing hyphen") << "K1234";
    QTest::newRow("Empty suffix after @") << "K-1234@";

    QTest::newRow("Prefix too long") << "ABCDE-1234";

    QTest::newRow("Number too short") << "K-123";
    QTest::newRow("Number too long") << "K-123456";

    QTest::newRow("Ext too short") << "K-1234@A";
    QTest::newRow("Ext too long") << "K-1234@ABCDEFG";
}

void TestAdifPOTARef::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifPOTARef::check(raw), "Check should reject malformed POTA references");

    auto obj = AdifPOTARef::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifPOTARef::test_NormalizationAndSet() {
    auto obj = AdifPOTARef::create("K-5033");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("k-4562@us-ca"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("K-4562@US-CA"));

    QVERIFY2(!obj->set("VE-12345@US-NYZZ"), "Should reject extension > 6 chars");

    // xxxx(4) - nnnnn(5) @ yyyyyy(6) -> 4+1+5+1+6 = 17
    QString maxValid = "WWFF-12345@AB-CDE";
    QVERIFY2(obj->set(maxValid.toStdString()), "Should accept exactly 17 chars");
    QCOMPARE(maxValid.length(), 17);

    QString invalid18 = "WWFF-12345@ABC-DEF";
    QVERIFY2(!obj->set(invalid18.toStdString()), "Should reject 18 chars");
}

QTEST_MAIN(TestAdifPOTARef)
#include "tst_adifpotaref.moc"