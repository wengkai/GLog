#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifInteger : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifInteger::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Single digit") << "5";
    QTest::newRow("Multi digits") << "1234567890";
    QTest::newRow("Negative integer") << "-42";
    QTest::newRow("Zero") << "0";
    QTest::newRow("Negative zero") << "-0";
    QTest::newRow("Leading zeros") << "007";
    QTest::newRow("Negative leading zeros") << "-00123";
}

void TestAdifInteger::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifInteger::check(raw), "Check should accept valid decimal integers");

    auto obj = AdifInteger::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifInteger::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Empty string") << "";
    QTest::newRow("Only minus sign") << "-";
    QTest::newRow("Plus sign (Not allowed by spec)") << "+123";
    QTest::newRow("Embedded minus") << "1-2";
    QTest::newRow("Decimal points") << "123.45";
    QTest::newRow("Alphabets") << "123a";
    QTest::newRow("Spaces") << " 123";
    QTest::newRow("Special characters") << "12!3";
}

void TestAdifInteger::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifInteger::check(raw), "Check should reject invalid integer formats");

    auto obj = AdifInteger::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifInteger::test_SetLogic() {
    auto obj = AdifInteger::create("100");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("-500"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("-500"));

    QVERIFY(obj->set("007"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("007"));

    QVERIFY(!obj->set("12.5"));
    QVERIFY(!obj->set("abc"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("007"));
}

QTEST_MAIN(TestAdifInteger)
#include "tst_adifinteger.moc"