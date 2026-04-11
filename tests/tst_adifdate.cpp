#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifDate : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifDate::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Min Year") << "19300101";
    QTest::newRow("Modern Date") << "20231027";
    QTest::newRow("Leap Year Feb 29") << "20240229";
    QTest::newRow("Max Day of Oct") << "20231031";
    QTest::newRow("Future Year") << "20991231";
}

void TestAdifDate::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifDate::check(raw), "Check should accept valid YYYYMMDD dates since 1930");

    auto obj = AdifDate::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifDate::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Too short") << "2023101";
    QTest::newRow("Too long") << "202310271";
    QTest::newRow("Year too early") << "19291231";
    QTest::newRow("Month 0") << "20230001";
    QTest::newRow("Month 13") << "20231301";
    QTest::newRow("Day 0") << "20231000";
    QTest::newRow("Day 32") << "20231032";
    QTest::newRow("Small month day overflow") << "20230431";
    QTest::newRow("Non-leap year Feb 29") << "20230229";
    QTest::newRow("Contains Non-digits") << "2023-10-2";
    QTest::newRow("Empty") << "";
}

void TestAdifDate::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifDate::check(raw), "Check should reject invalid dates or formats");

    auto obj = AdifDate::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifDate::test_SetLogic() {
    auto obj = AdifDate::create("20200101");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("20000229"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("20000229"));

    QVERIFY(!obj->set("19000229"));

    QVERIFY(!obj->set("TODAY"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("20000229"));
}

QTEST_MAIN(TestAdifDate)
#include "tst_adifdate.moc"