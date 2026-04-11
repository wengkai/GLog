#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifCharacter : public QObject {
    Q_OBJECT

  private slots:
    void test_ValidInputs_data();
    void test_ValidInputs();

    void test_InvalidInputs_data();
    void test_InvalidInputs();

    void test_SetLogic();
};

void TestAdifCharacter::test_ValidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Space (32)") << " ";
    QTest::newRow("Tilde (126)") << "~";
    QTest::newRow("Uppercase A") << "A";
    QTest::newRow("Digit 0") << "0";
    QTest::newRow("Symbol #") << "#";
    QTest::newRow("Symbol @") << "@";
}

void TestAdifCharacter::test_ValidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(AdifCharacter::check(raw), "Check should accept ASCII characters from 32 to 126");

    auto obj = AdifCharacter::create(raw);
    QVERIFY(obj.has_value());

    QCOMPARE(QString::fromStdString(obj->get()), input);
}

void TestAdifCharacter::test_InvalidInputs_data() {
    QTest::addColumn<QString>("input");

    QTest::newRow("Empty") << "";
    QTest::newRow("Multi-character") << "AB";
    QTest::newRow("Tab (9)") << "\t";
    QTest::newRow("Newline (10)") << "\n";
    QTest::newRow("Delete (127)") << QString(QChar(127));
    QTest::newRow("Extended ASCII (128)") << QString(QChar(128));
    QTest::newRow("UTF-8 Chinese character") << "好";
}

void TestAdifCharacter::test_InvalidInputs() {
    QFETCH(QString, input);
    std::string raw = input.toStdString();

    QVERIFY2(!AdifCharacter::check(raw),
             "Check should reject control chars, multi-chars, or non-ASCII");

    auto obj = AdifCharacter::create(raw);
    QVERIFY(!obj.has_value());
}

void TestAdifCharacter::test_SetLogic() {
    auto obj = AdifCharacter::create("X");
    QVERIFY(obj.has_value());

    QVERIFY(obj->set(" "));
    QCOMPARE(QString::fromStdString(obj->get()), QString(" "));

    QVERIFY(!obj->set("\n"));

    QVERIFY(!obj->set("YZ"));

    QCOMPARE(QString::fromStdString(obj->get()), QString(" "));
}

QTEST_MAIN(TestAdifCharacter)
#include "tst_adifcharacter.moc"