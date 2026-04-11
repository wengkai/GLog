#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifPOTARefList : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_Normalization();
};

void TestAdifPOTARefList::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("normalized");

    QTest::newRow("Single item") << "K-5033"
                                 << "K-5033";
    QTest::newRow("Multiple basic") << "K-5033,VE-5082,VK-0556"
                                    << "K-5033,VE-5082,VK-0556";
    QTest::newRow("Mixed with extensions") << "K-4562@US-CA,K-5033"
                                           << "K-4562@US-CA,K-5033";
    QTest::newRow("With spaces and lowercase") << " k-5033 , ve-5082@ca-ab "
                                               << "K-5033,VE-5082@CA-AB";
    QTest::newRow("5-digit numbers") << "K-10000,K-10001"
                                     << "K-10000,K-10001";
}

void TestAdifPOTARefList::test_ValidInputs() {
    QFETCH(QString, input);
    QFETCH(QString, normalized);

    std::string raw = input.toStdString();

    QVERIFY2(AdifPOTARefList::check(raw),
             "Check should accept valid comma-separated POTA references");

    auto obj = AdifPOTARefList::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), normalized);
}

void TestAdifPOTARefList::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Empty string") << "";
    QTest::newRow("Contains invalid item") << "K-5033,INVALID-REF";
    QTest::newRow("Wrong separator") << "K-5033;VE-5082";
    QTest::newRow("Empty item in list") << "K-5033,,VE-5082";
    QTest::newRow("Invalid extension length") << "K-5033@A";
    QTest::newRow("Trailing comma") << "K-5033,";
    QTest::newRow("Leading comma") << ",K-5033";
}

void TestAdifPOTARefList::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifPOTARefList::check(raw),
             "Check should reject lists with any invalid POTA reference");

    auto obj = AdifPOTARefList::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifPOTARefList::test_Normalization() {
    auto obj = AdifPOTARefList::create("K-5033");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("  vk-0556 , k-4562@us-ca  "));
    QCOMPARE(QString::fromStdString(obj->get()), QString("VK-0556,K-4562@US-CA"));

    QVERIFY(!obj->set("K-5033,K-123"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("VK-0556,K-4562@US-CA"));
}

QTEST_MAIN(TestAdifPOTARefList)
#include "tst_adifpotareflist.moc"