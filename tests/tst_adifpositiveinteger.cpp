#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifPositiveInteger : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifPositiveInteger::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Single digit positive") << "1";
    QTest::newRow("Large positive") << "999999";
    QTest::newRow("Leading zeros positive") << "007";
    QTest::newRow("Leading zeros large") << "000123";
}

void TestAdifPositiveInteger::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifPositiveInteger::check(raw), "Check should accept positive integers > 0");

    auto obj = AdifPositiveInteger::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifPositiveInteger::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Zero") << "0";
    QTest::newRow("Multiple Zeros") << "000";
    QTest::newRow("Negative") << "-1";
    QTest::newRow("Negative with leading zeros") << "-001";
    QTest::newRow("Empty string") << "";
    QTest::newRow("Decimal point") << "1.5";
    QTest::newRow("Alpha characters") << "12A";
    QTest::newRow("Plus sign") << "+5"; // ADIF spec says "unsigned sequence"
    QTest::newRow("Spaces") << " 123";
}

void TestAdifPositiveInteger::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifPositiveInteger::check(raw),
             "Check should reject zero, negatives, and non-integers");

    auto obj = AdifPositiveInteger::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifPositiveInteger::test_SetLogic() {
    auto obj = AdifPositiveInteger::create("42");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("08"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("08"));

    QVERIFY(!obj->set("0"));

    QVERIFY(!obj->set("-10"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("08"));
}

QTEST_MAIN(TestAdifPositiveInteger)
#include "tst_adifpositiveinteger.moc"