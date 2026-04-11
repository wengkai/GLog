#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifMultilineString : public QObject {
    Q_OBJECT

  private slots:
    void test_StrictCheck_data();
    void test_StrictCheck();

    void test_AutoCorrection_data();
    void test_AutoCorrection();

    void test_InvalidCharacters();
};

void TestAdifMultilineString::test_StrictCheck_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("expected");

    QTest::newRow("Pure ASCII") << "Hello ADIF" << true;
    QTest::newRow("Standard CRLF") << "Line1\r\nLine2" << true;
    QTest::newRow("Multiple CRLF") << "\r\n\r\n" << true;
    QTest::newRow("Isolated LF") << "Line1\nLine2" << false;
    QTest::newRow("Isolated CR") << "Line1\rLine2" << false;
    QTest::newRow("Mixed bad breaks") << "Standard\r\nBad\n" << false;
}

void TestAdifMultilineString::test_StrictCheck() {
    QFETCH(QString, input);
    QFETCH(bool, expected);

    std::string raw = input.toStdString();
    QCOMPARE(AdifMultilineString::check(raw), expected);
}

void TestAdifMultilineString::test_AutoCorrection_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedStandardized");

    QTest::newRow("Fix LF to CRLF") << "Line1\nLine2"
                                    << "Line1\r\nLine2";
    QTest::newRow("Fix CR to CRLF") << "Line1\rLine2"
                                    << "Line1\r\nLine2";
    QTest::newRow("Fix Mixed to CRLF") << "A\nB\rC\r\nD"
                                       << "A\r\nB\r\nC\r\nD";
    QTest::newRow("Keep Standard") << "Already\r\nStandard"
                                   << "Already\r\nStandard";
    QTest::newRow("Empty remains empty") << ""
                                         << "";
}

void TestAdifMultilineString::test_AutoCorrection() {
    QFETCH(QString, input);
    QFETCH(QString, expectedStandardized);

    std::string raw = input.toStdString();

    auto obj = AdifMultilineString::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), expectedStandardized);

    QVERIFY(AdifMultilineString::check(obj->get()));

    auto obj2 = AdifMultilineString::create("initial");
    QVERIFY(obj2->set(raw));
    QCOMPARE(QString::fromStdString(obj2->get()), expectedStandardized);
}

void TestAdifMultilineString::test_InvalidCharacters() {
    std::string withTab = "Text\twith tab";
    std::string withUnicode = "Text with 📡";

    QVERIFY2(!AdifMultilineString::check(withTab), "Should reject Tab");
    QVERIFY2(!AdifMultilineString::check(withUnicode), "Should reject Unicode");

    auto objTab = AdifMultilineString::create(withTab);
    QVERIFY(!objTab.has_value());
}

QTEST_MAIN(TestAdifMultilineString)
#include "tst_adifmultilinestring.moc"