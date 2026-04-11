#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifIntlString : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_MultibyteSequence_data();
    void test_MultibyteSequence();

    void test_SetLogic();
};

void TestAdifIntlString::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Pure ASCII String") << "Hello World 73";
    QTest::newRow("Chinese String") << "致以 73 和 88";
    QTest::newRow("Mixed Multi-language") << "Operator: 张三 (ZS1AAA) / Москва";
    QTest::newRow("Empty String") << "";
    QTest::newRow("String with Spaces") << "  Leading and trailing spaces  ";
    QTest::newRow("Symbols") << "~!@#$%^&*()_+{}|:\"<>?";
}

void TestAdifIntlString::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifIntlString::check(raw), "Check should accept valid UTF-8 sequences without CR/LF");

    auto obj = AdifIntlString::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifIntlString::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Contains LF") << "Line1\nLine2";
    QTest::newRow("Contains CR") << "Line1\rLine2";
    QTest::newRow("Contains CRLF") << "Line1\r\nLine2";
    QTest::newRow("Mixed valid and LF") << "Valid Text\n";
}

void TestAdifIntlString::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifIntlString::check(raw), "Check should reject strings containing CR or LF");
    auto obj = AdifIntlString::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifIntlString::test_MultibyteSequence_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Single Emoji 📡") << "📡";
    QTest::newRow("Multiple Emojis 📡📡📡") << "📡📡📡";
    QTest::newRow("Emoji mixed with ASCII") << "Antenna 📡 Status: OK";
    QTest::newRow("4-byte character (U+1F600)") << "😀";
    QTest::newRow("Complex Unicode (Zero Width Joiner)") << "👨‍👩‍👧‍👦";
}

void TestAdifIntlString::test_MultibyteSequence() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifIntlString::check(raw),
             "Check must correctly handle multibyte UTF-8 sequences like Emojis");

    auto obj = AdifIntlString::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifIntlString::test_SetLogic() {
    auto obj = AdifIntlString::create("Original Text");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("New Data: 📡"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("New Data: 📡"));

    QVERIFY(!obj->set("Invalid\nData"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("New Data: 📡"));
}

QTEST_MAIN(TestAdifIntlString)
#include "tst_adifintlstring.moc"