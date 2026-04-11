#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifGridSquareExt : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_NormalizationAndSet();
};

void TestAdifGridSquareExt::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("normalized");

    // 2-character (Characters 9 & 10)
    QTest::newRow("9-10 Min") << "AA"
                              << "AA";
    QTest::newRow("9-10 Max") << "XX"
                              << "XX";
    QTest::newRow("9-10 Mixed Case") << "ax"
                                     << "AX";

    // 4-character (Characters 9, 10, 11 & 12)
    QTest::newRow("9-12 Standard") << "BQ42"
                                   << "BQ42";
    QTest::newRow("9-12 Min") << "AA00"
                              << "AA00";
    QTest::newRow("9-12 Max") << "XX99"
                              << "XX99";
    QTest::newRow("9-12 Mixed Case") << "bq42"
                                     << "BQ42";
}

void TestAdifGridSquareExt::test_ValidInputs() {
    QFETCH(QString, input);
    QFETCH(QString, normalized);

    std::string raw = input.toStdString();

    QVERIFY2(AdifGridSquareExt::check(raw), "Check should accept valid 2 or 4 char extension");

    auto obj = AdifGridSquareExt::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), normalized);
}

void TestAdifGridSquareExt::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Single char") << "A";
    QTest::newRow("Odd length") << "BQ4";
    QTest::newRow("Too long") << "BQ42AA";

    QTest::newRow("9-10 Out of Range") << "YZ"; // Y is beyond X
    QTest::newRow("9-10 Digit Error") << "11";  // Should be letters

    QTest::newRow("11-12 Alpha Error") << "BQAA"; // Should be digits

    QTest::newRow("Empty") << "";
    QTest::newRow("Special Chars") << "B#";
}

void TestAdifGridSquareExt::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifGridSquareExt::check(raw),
             "Check should reject invalid extension formats or ranges");

    auto obj = AdifGridSquareExt::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifGridSquareExt::test_NormalizationAndSet() {
    auto obj = AdifGridSquareExt::create("AX");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("bq42"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("BQ42"));

    QVERIFY(!obj->set("Y1"));

    QVERIFY(!obj->set("1122"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("BQ42"));
}

QTEST_MAIN(TestAdifGridSquareExt)
#include "tst_adifgridsquareext.moc"