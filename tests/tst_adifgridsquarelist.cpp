#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifGridSquareList : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_Normalization();
};

void TestAdifGridSquareList::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("normalized");

    QTest::newRow("Single item") << "FN01"
                                 << "FN01";
    QTest::newRow("Multiple items") << "FN01,PM95,IO83"
                                    << "FN01,PM95,IO83";
    QTest::newRow("Mixed lengths") << "RR,FN01,PM95UN"
                                   << "RR,FN01,PM95UN";
    QTest::newRow("With spaces") << "FN01, PM95 , IO83"
                                 << "FN01,PM95,IO83";
    QTest::newRow("Mixed case") << "fn01,pm95un"
                                << "FN01,PM95UN";
}

void TestAdifGridSquareList::test_ValidInputs() {
    QFETCH(QString, input);
    QFETCH(QString, normalized);

    std::string raw = input.toStdString();

    QVERIFY2(AdifGridSquareList::check(raw),
             "Check should accept valid comma-separated GridSquares");

    auto obj = AdifGridSquareList::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), normalized);
}

void TestAdifGridSquareList::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Contains invalid item") << "FN01,INVALID,PM95";
    QTest::newRow("Wrong separator") << "FN01;PM95";
    QTest::newRow("Odd length item") << "FN01,PM9";
    QTest::newRow("Out of range item") << "FN01,SS00"; // S > R
    QTest::newRow("Empty item in list") << "FN01,,PM95";
    QTest::newRow("Trailing comma") << "FN01,PM95,";
}

void TestAdifGridSquareList::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifGridSquareList::check(raw),
             "Check should reject lists with any invalid GridSquare");

    auto obj = AdifGridSquareList::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifGridSquareList::test_Normalization() {
    auto obj = AdifGridSquareList::create("FN01");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set(" io83uf , rr99 "));
    QCOMPARE(QString::fromStdString(obj->get()), QString("IO83UF,RR99"));

    QVERIFY(!obj->set("FN01,FN01MH42BQ"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("IO83UF,RR99"));
}

QTEST_MAIN(TestAdifGridSquareList)
#include "tst_adifgridsquarelist.moc"