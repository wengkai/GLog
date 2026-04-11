#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifBoolean : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifBoolean::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");

    QTest::newRow("Upper True") << "Y"
                                << "Y";
    QTest::newRow("Lower True") << "y"
                                << "Y";
    QTest::newRow("Upper False") << "N"
                                 << "N";
    QTest::newRow("Lower False") << "n"
                                 << "N";
}

void TestAdifBoolean::test_ValidInputs() {
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);

    std::string raw = input.toStdString();

    QVERIFY2(AdifBoolean::check(raw), "Check should accept valid Boolean character");

    auto obj = AdifBoolean::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), expectedOutput);
}

void TestAdifBoolean::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Invalid Char") << "A";
    QTest::newRow("Numeric") << "1";
    QTest::newRow("Longer String") << "YES";
    QTest::newRow("Empty") << "";
    QTest::newRow("Space") << " ";
}

void TestAdifBoolean::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifBoolean::check(raw), "Check should reject invalid Boolean input");

    auto obj = AdifBoolean::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifBoolean::test_SetLogic() {
    auto obj = AdifBoolean::create("Y");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("n"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("N"));

    QVERIFY(!obj->set("X"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("N"));
}

QTEST_MAIN(TestAdifBoolean)
#include "tst_adifboolean.moc"