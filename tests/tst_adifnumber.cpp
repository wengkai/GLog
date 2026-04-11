#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifNumber : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifNumber::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Simple Integer") << "123";
    QTest::newRow("Simple Decimal") << "123.45";
    QTest::newRow("Negative Decimal") << "-123.45";
    QTest::newRow("Negative Integer") << "-123";
    QTest::newRow("Zero") << "0";
    QTest::newRow("Leading zeros decimal") << "00123.450";
    QTest::newRow("Starting with dot") << ".5";
    QTest::newRow("Ending with dot") << "123.";
    QTest::newRow("Single zero dot") << "0.0";
}

void TestAdifNumber::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifNumber::check(raw), "Check should accept valid decimal numbers");

    auto obj = AdifNumber::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifNumber::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Empty") << "";
    QTest::newRow("Only Minus") << "-";
    QTest::newRow("Only Dot") << ".";
    QTest::newRow("Multiple Dots") << "12.34.56";
    QTest::newRow("Plus Sign") << "+123.45";
    QTest::newRow("Embedded Minus") << "12-3.45";
    QTest::newRow("Alpha characters") << "12a.3";
    QTest::newRow("Spaces") << " 12.3";
    QTest::newRow("Comma as separator") << "12,34";
}

void TestAdifNumber::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifNumber::check(raw), "Check should reject invalid number formats");

    auto obj = AdifNumber::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifNumber::test_SetLogic() {
    auto obj = AdifNumber::create("10.0");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("-0.005"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("-0.005"));

    QVERIFY(!obj->set("10.0px"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("-0.005"));
}

QTEST_MAIN(TestAdifNumber)
#include "tst_adifnumber.moc"