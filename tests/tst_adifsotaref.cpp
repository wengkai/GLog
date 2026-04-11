#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifSOTARef : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_NormalizationAndSet();
};

void TestAdifSOTARef::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("Standard G/LD-003") << "G/LD-003"
                                       << "G/LD-003";
    QTest::newRow("Standard W2/WE-003") << "W2/WE-003"
                                        << "W2/WE-003";
    QTest::newRow("Standard HB9/BE-001") << "HB9/BE-001"
                                         << "HB9/BE-001";
    QTest::newRow("Lowercase input") << "ct/es-001"
                                     << "CT/ES-001";
    QTest::newRow("Longer number") << "W6/CT-100"
                                   << "W6/CT-100";
}

void TestAdifSOTARef::test_ValidInputs() {
    QFETCH(QString, input);
    QFETCH(QString, expected);

    std::string raw = input.toStdString();

    QVERIFY2(AdifSOTARef::check(raw), "Check should accept valid SOTA reference patterns");

    auto obj = AdifSOTARef::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), expected);
}

void TestAdifSOTARef::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Empty string") << "";
    QTest::newRow("Missing Slash") << "GLD-003";
    QTest::newRow("Missing Hyphen") << "G/LD003";
    QTest::newRow("Reversed Slash Hyphen") << "G-LD/003";
    QTest::newRow("Only Association") << "HB9/";
    QTest::newRow("Only Number") << "/LD-003";
    QTest::newRow("Non-ASCII") << "G/LD-00中";
}

void TestAdifSOTARef::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifSOTARef::check(raw), "Check should reject malformed SOTA references");

    auto obj = AdifSOTARef::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifSOTARef::test_NormalizationAndSet() {
    auto obj = AdifSOTARef::create("G/LD-003");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("hb9/be-001"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("HB9/BE-001"));

    QVERIFY(!obj->set("LONDON-123"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("HB9/BE-001"));
}

QTEST_MAIN(TestAdifSOTARef)
#include "tst_adifsotaref.moc"