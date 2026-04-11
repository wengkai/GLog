#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifIntlCharacter : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifIntlCharacter::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("ASCII char") << "A";
    QTest::newRow("Space") << " ";
    QTest::newRow("Chinese Char") << "中"; // 3-byte in UTF-8
    QTest::newRow("Emoji") << "📡";        // 4-byte in UTF-8
    QTest::newRow("Cyrillic") << "Д";      // 2-byte in UTF-8
    QTest::newRow("Greek") << "Ω";         // 2-byte in UTF-8
}

void TestAdifIntlCharacter::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifIntlCharacter::check(raw),
             "Check should accept single Unicode characters except CR/LF");

    auto obj = AdifIntlCharacter::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifIntlCharacter::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Empty") << "";
    QTest::newRow("Carriage Return (CR)") << "\r";
    QTest::newRow("Line Feed (LF)") << "\n";
    QTest::newRow("Multi-character ASCII") << "AB";
    QTest::newRow("Multi-character Unicode") << "你好";
    QTest::newRow("Mixed Multi-char") << "A中";
}

void TestAdifIntlCharacter::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifIntlCharacter::check(raw),
             "Check should reject CR, LF, empty, or multiple characters");

    auto obj = AdifIntlCharacter::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifIntlCharacter::test_SetLogic() {
    auto obj = AdifIntlCharacter::create("X");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set("🌏"));
    QCOMPARE(QString::fromStdString(obj->get()), QString("🌏"));

    QVERIFY(!obj->set("\n"));

    QVERIFY(!obj->set("73"));

    QCOMPARE(QString::fromStdString(obj->get()), QString("🌏"));
}

QTEST_MAIN(TestAdifIntlCharacter)
#include "tst_adifintlcharacter.moc"