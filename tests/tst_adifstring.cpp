#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifString : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifString::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Simple text") << "Hello World";
    QTest::newRow("Radio Gear") << "IC-7300 & Wire Antenna";
    QTest::newRow("Numbers and Symbols") << "123!@#$%^&*()_+";
    QTest::newRow("Min length (Empty)") << "";
    QTest::newRow("Single character") << "A";
    QTest::newRow("Max ASCII boundary") << "Bound~"; // '~' is 126
}

void TestAdifString::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifString::check(raw), "Check should accept sequences of printable ASCII (32-126)");

    auto obj = AdifString::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifString::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Contains Newline") << "Line 1\nLine 2";
    QTest::newRow("Contains Tab") << "Field1\tField2";
    QTest::newRow("Contains CR") << "Carriage\rReturn";
    QTest::newRow("Contains Unicode") << "73, 朋友"; // '朋友' is non-ASCII
    QTest::newRow("Contains Emoji") << "Good DX 📡";
    QTest::newRow("Control Char (127)") << QString("Del") + QChar(127);
}

void TestAdifString::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifString::check(raw), "Check should reject strings containing non-printable ASCII");

    auto obj = AdifString::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifString::test_SetLogic() {
    auto obj = AdifString::create("Initial Comment");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("New Comment 123"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("New Comment 123"));

    QVERIFY(!obj->set("Broken\nString"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("New Comment 123"));

    QVERIFY(obj->set(""));
    QVERIFY(obj->get().empty());
}

QTEST_MAIN(TestAdifString)
#include "tst_adifstring.moc"