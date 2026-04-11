#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifGridSquare : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_NormalizationAndSet();
};

void TestAdifGridSquare::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("normalized");

    // 2-character
    QTest::newRow("2-char Min") << "AA"
                                << "AA";
    QTest::newRow("2-char Max") << "RR"
                                << "RR";

    // 4-character
    QTest::newRow("4-char Standard") << "FN01"
                                     << "FN01";
    QTest::newRow("4-char Mixed Case") << "fn01"
                                       << "FN01";

    // 6-character
    QTest::newRow("6-char Standard") << "PM95un"
                                     << "PM95UN"; // A-X range
    QTest::newRow("6-char Boundary X") << "RR99XX"
                                       << "RR99XX";

    // 8-character
    QTest::newRow("8-char Standard") << "FN01MH42"
                                     << "FN01MH42";
}

void TestAdifGridSquare::test_ValidInputs() {
    QFETCH(QString, input);
    QFETCH(QString, normalized);

    std::string raw = input.toStdString();

    QVERIFY2(AdifGridSquare::check(raw), "Check should accept valid 2/4/6/8-char locators");

    auto obj = AdifGridSquare::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), normalized);
}

void TestAdifGridSquare::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Too short") << "A";
    QTest::newRow("Odd length") << "FN0";
    QTest::newRow("Too long") << "FN01MH42BQ";

    QTest::newRow("Pair1 Out of Range") << "S1";
    QTest::newRow("Pair1 Digit error") << "11";

    QTest::newRow("Pair2 Alpha error") << "FNAA";

    QTest::newRow("Pair3 Out of Range") << "FN01YY"; // Y exceeds X

    QTest::newRow("Pair4 Alpha error") << "FN01MHAA";

    QTest::newRow("Empty") << "";
    QTest::newRow("Special Chars") << "FN01-MH";
}

void TestAdifGridSquare::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifGridSquare::check(raw), "Check should reject invalid locator formats or ranges");
    auto obj = AdifGridSquare::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifGridSquare::test_NormalizationAndSet() {
    auto obj = AdifGridSquare::create("PM95");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("pm95un"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("PM95UN"));

    QVERIFY(!obj->set("FN01MH42BQ"));

    QVERIFY(!obj->set("SS00"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("PM95UN"));
}

QTEST_MAIN(TestAdifGridSquare)
#include "tst_adifgridsquare.moc"