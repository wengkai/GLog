#include <QtTest>
#include <optional>
#include "AdifDataTypes.h"

class TestAdifDataTypes : public QObject {
    Q_OBJECT

  private slots:

    void test_AdifGeneral_Creation();

    void test_AdifGeneral_DataFlow();
};

void TestAdifDataTypes::test_AdifGeneral_Creation() {
    std::string rawData = "FT8";

    QVERIFY(AdifGeneral::check(rawData) == true);

    auto obj = AdifGeneral::create(rawData);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), QString("FT8"));
}

void TestAdifDataTypes::test_AdifGeneral_DataFlow() {
    auto obj = AdifGeneral::create("InitialValue");

    bool success = obj->set("NewValue");
    QVERIFY(success == true);
    QCOMPARE(QString::fromStdString(obj->get()), QString("NewValue"));

    QVERIFY(obj->set("") == true);
    QVERIFY(obj->get().empty());
}

QTEST_MAIN(TestAdifDataTypes)
#include "tst_adifdatatypes.moc"