#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifLocation : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_NormalizationAndSet();
};

void TestAdifLocation::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("North Standard") << "N040 26.700"
                                    << "N040 26.700";
    QTest::newRow("South Min") << "S000 00.000"
                               << "S000 00.000";
    QTest::newRow("East Boundary") << "E180 00.000"
                                   << "E180 00.000";
    QTest::newRow("West Max Min") << "W122 15.123"
                                  << "W122 15.123";
    QTest::newRow("Lowercase Direction") << "e015 30.500"
                                         << "E015 30.500";
}

void TestAdifLocation::test_ValidInputs() {
    QFETCH(QString, input);
    QFETCH(QString, expected);

    std::string raw = input.toStdString();

    QVERIFY2(AdifLocation::check(raw), "Check should accept valid 11-char location string");

    auto obj = AdifLocation::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), expected);
}

void TestAdifLocation::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Too short") << "N040 26.70";
    QTest::newRow("Too long") << "N040 26.7000";
    QTest::newRow("Missing Space") << "N04026.700";
    QTest::newRow("Wrong Separator") << "N040-26.700";

    QTest::newRow("Invalid Direction") << "X040 26.700";

    QTest::newRow("DDD Overflow") << "N181 00.000";
    QTest::newRow("DDD Non-digit") << "NA40 26.700";

    QTest::newRow("MM Overflow") << "E015 60.000";
    QTest::newRow("MM Incomplete") << "E015 5.500 ";
    QTest::newRow("Dot Position") << "E015 305.00";
}

void TestAdifLocation::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifLocation::check(raw), "Check should reject invalid location formats or values");

    auto obj = AdifLocation::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifLocation::test_NormalizationAndSet() {
    auto obj = AdifLocation::create("N000 00.000");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("w120 45.999"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("W120 45.999"));

    QVERIFY(!obj->set("W120 5.000")); // 05.000

    QCOMPARE(QString::fromStdString(obj->get()), QString("W120 45.999"));
}

QTEST_MAIN(TestAdifLocation)
#include "tst_adiflocation.moc"