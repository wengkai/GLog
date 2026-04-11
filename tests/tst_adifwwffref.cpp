#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifWWFFRef : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_NormalizationAndSet();
};

void TestAdifWWFFRef::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("Basic KFF-4655") << "KFF-4655"
                                    << "KFF-4655";
    QTest::newRow("4-char prefix 3DAFF-0002") << "3DAFF-0002"
                                              << "3DAFF-0002";
    QTest::newRow("2-char prefix URFF-0001") << "URFF-0001"
                                             << "URFF-0001";
    QTest::newRow("Lowercase input") << "kff-4655"
                                     << "KFF-4655";
    QTest::newRow("Max length 11") << "ABCDFF-9999"
                                   << "ABCDFF-9999";
    QTest::newRow("Min length 8") << "AFF-0001"
                                  << "AFF-0001";
}

void TestAdifWWFFRef::test_ValidInputs() {
    QFETCH(QString, input);
    QFETCH(QString, expected);

    std::string raw = input.toStdString();

    QVERIFY2(AdifWWFFRef::check(raw), "Check should accept valid WWFF reference patterns");

    auto obj = AdifWWFFRef::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), expected);
}

void TestAdifWWFFRef::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Too short") << "FF-0001";
    QTest::newRow("Too long") << "ABCDEFF-0001";

    QTest::newRow("Missing FF") << "K-4655";
    QTest::newRow("Missing Dash") << "KFF4655";
    QTest::newRow("Wrong Constant") << "KGG-4655";

    QTest::newRow("Digits too short") << "KFF-123";
    QTest::newRow("Digits too long") << "KFF-12345";
    QTest::newRow("Non-digits in number") << "KFF-ABCD";

    QTest::newRow("Empty string") << "";
}

void TestAdifWWFFRef::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifWWFFRef::check(raw), "Check should reject malformed WWFF references");

    auto obj = AdifWWFFRef::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifWWFFRef::test_NormalizationAndSet() {
    auto obj = AdifWWFFRef::create("KFF-4655");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("urff-0123"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("URFF-0123"));

    QVERIFY(!obj->set("URFF_0123"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("URFF-0123"));
}

QTEST_MAIN(TestAdifWWFFRef)
#include "tst_adifwwffref.moc"