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
    void test_Cmp_data();
    void test_Cmp();
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

using CompareRes = AdifNumber::CompareRes;

void TestAdifNumber::test_Cmp_data() {
    QTest::addColumn<CompareRes>("result");
    QTest::addColumn<std::string>("left");
    QTest::addColumn<std::string>("right");

    QTest::newRow("Zero") << CompareRes::equal_to << std::string("0") << std::string("00");
    QTest::newRow("Positive1") << CompareRes::less << std::string("007.65") << std::string("10.65");
    QTest::newRow("Positive2") << CompareRes::less << std::string("29.600") << std::string("21000");

    QTest::newRow("equal_with_leading_zeros")
        << CompareRes::equal_to << std::string("000123") << std::string("123");
    QTest::newRow("equal_with_trailing_zeros")
        << CompareRes::equal_to << std::string("45.67000") << std::string("45.67");
    QTest::newRow("equal_negative_with_zeros")
        << CompareRes::equal_to << std::string("-000.100") << std::string("-0.1");
    QTest::newRow("equal_zero_variants")
        << CompareRes::equal_to << std::string("-0") << std::string("0.00");

    QTest::newRow("greater_positive")
        << CompareRes::greater << std::string("105") << std::string("99.9");
    QTest::newRow("greater_decimal")
        << CompareRes::greater << std::string("0.003") << std::string("0.00299");
    QTest::newRow("greater_negative")
        << CompareRes::greater << std::string("-3.5") << std::string("-7.5");
    QTest::newRow("greater_large") << CompareRes::greater << std::string("100000000000000000000")
                                   << std::string("99999999999999999999");
    QTest::newRow("greater_different_length")
        << CompareRes::greater << std::string("123.001") << std::string("123.0009");

    QTest::newRow("less_positive")
        << CompareRes::less << std::string("3.14") << std::string("3.1401");
    QTest::newRow("less_mixed_sign")
        << CompareRes::less << std::string("-5.2") << std::string("3.1");
    QTest::newRow("less_both_negative")
        << CompareRes::less << std::string("-100.5") << std::string("-50.5");
    QTest::newRow("less_fraction")
        << CompareRes::less << std::string("0.0001") << std::string("0.001");

    QTest::newRow("edge_max_integer") << CompareRes::greater << std::string("99999999999999999999")
                                      << std::string("0.0000000001");
    QTest::newRow("edge_negative_zero")
        << CompareRes::equal_to << std::string("-0.0") << std::string("0.000");
    QTest::newRow("edge_one_vs_point_nines")
        << CompareRes::greater << std::string("1") << std::string("0.99999999999999999999");
    QTest::newRow("edge_very_long_fraction")
        << CompareRes::less << std::string("1.00000000000000000001")
        << std::string("1.00000000000000000002");

    QTest::newRow("implicit_zero_start")
        << CompareRes::equal_to << std::string(".5") << std::string("0.5");
    QTest::newRow("implicit_zero_end")
        << CompareRes::equal_to << std::string("5.") << std::string("5.0");
}

void TestAdifNumber::test_Cmp() {
    auto flip = [](CompareRes res) {
        if (res == CompareRes::greater) {
            return CompareRes::less;
        }
        if (res == CompareRes::less) {
            return CompareRes::greater;
        }
        return res; // equal stays equal
    };

    QFETCH(CompareRes, result);
    QFETCH(std::string, left);
    QFETCH(std::string, right);
    auto left_obj = AdifNumber::create(left);
    auto right_obj = AdifNumber::create(right);
    QVERIFY(left_obj.has_value() && right_obj.has_value());
    QCOMPARE(result, left_obj->compare(*right_obj));
    QCOMPARE(flip(result), right_obj->compare(*left_obj));
}

QTEST_MAIN(TestAdifNumber)
#include "tst_adifnumber.moc"