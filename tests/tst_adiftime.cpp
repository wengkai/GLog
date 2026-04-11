#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifTime : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifTime::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("HHMM format min") << "0000";
    QTest::newRow("HHMM format max") << "2359";
    QTest::newRow("HHMMSS format min") << "000000";
    QTest::newRow("HHMMSS format max") << "235959";
    QTest::newRow("Midday with seconds") << "123045";
    QTest::newRow("Late night") << "2300";
}

void TestAdifTime::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifTime::check(raw), "Check should accept valid HHMM or HHMMSS UTC time");

    auto obj = AdifTime::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifTime::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Too short") << "123";
    QTest::newRow("Odd length") << "12345";
    QTest::newRow("Too long") << "1234567";
    QTest::newRow("Invalid hour") << "2400";
    QTest::newRow("Invalid minute") << "1260";
    QTest::newRow("Invalid second") << "123060";
    QTest::newRow("Contains non-digits") << "12:30";
    QTest::newRow("Empty string") << "";
    QTest::newRow("Space in between") << "12 30";
}

void TestAdifTime::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifTime::check(raw), "Check should reject invalid time formats or values");

    auto obj = AdifTime::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifTime::test_SetLogic() {
    auto obj = AdifTime::create("1200");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("120059"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("120059"));

    QVERIFY(!obj->set("250000"));

    QVERIFY(!obj->set("12:00"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("120059"));
}

QTEST_MAIN(TestAdifTime)
#include "tst_adiftime.moc"