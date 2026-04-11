#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifDigit : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifDigit::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Zero") << "0";
    QTest::newRow("Five") << "5";
    QTest::newRow("Nine") << "9";
}

void TestAdifDigit::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifDigit::check(raw), "Check should accept single digit character");

    auto obj = AdifDigit::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifDigit::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Alphabet") << "A";
    QTest::newRow("Multi-digit") << "10";
    QTest::newRow("Symbol") << "/";
    QTest::newRow("Colon (58)") << ":";
    QTest::newRow("Slash (47)") << "/";
    QTest::newRow("Empty") << "";
    QTest::newRow("Space") << " ";
}

void TestAdifDigit::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifDigit::check(raw), "Check should reject non-digit or multi-character input");

    auto obj = AdifDigit::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifDigit::test_SetLogic() {
    auto obj = AdifDigit::create("1");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("7"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("7"));

    QVERIFY(!obj->set("12"));
    QVERIFY(!obj->set("x"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("7"));
}

QTEST_MAIN(TestAdifDigit)
#include "tst_adifdigit.moc"