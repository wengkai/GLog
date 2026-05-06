#include <QtTest>
#include <optional>
#include <string>
#include "AdifDataTypes.h"

class TestAdifIntlMultilineString : public QObject {
    Q_OBJECT

  private slots:
    void test_StrictCheck_data();
    void test_StrictCheck();
    void test_FakeUnicode_data();
    void test_FakeUnicode();

    void test_AutoCorrection_data();
    void test_AutoCorrection();

    void test_UnicodeIntegrity();
};

void TestAdifIntlMultilineString::test_StrictCheck_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("expected");

    QTest::newRow("Unicode with CRLF") << "Hello 📡\r\nLine 2 中文" << true;
    QTest::newRow("Pure Unicode No Breaks") << "📡🌏" << true;
    QTest::newRow("Isolated LF with Unicode") << "📡\n🌏" << false;
    QTest::newRow("Isolated CR with Unicode") << "📡\r🌏" << false;
    QTest::newRow("Empty") << "" << true;
}

void TestAdifIntlMultilineString::test_StrictCheck() {
    QFETCH(QString, input);
    QFETCH(bool, expected);

    std::string raw = input.toStdString();
    QCOMPARE(AdifIntlMultilineString::check(raw), expected);
}

void TestAdifIntlMultilineString::test_FakeUnicode_data() {
    QTest::addColumn<QByteArray>("raw");
    QTest::newRow("lone FF1") << QByteArray("\xff📡🌏");
    QTest::newRow("lone FF2") << QByteArray("📡\xff🌏");
    QTest::newRow("lone FF3") << QByteArray("📡🌏\xff");
    QTest::newRow("cut off") << QByteArray("\xf0\x9f\x93");
    QTest::newRow("overlong A") << QByteArray("\xc1\x81");
}

void TestAdifIntlMultilineString::test_FakeUnicode() {
    QFETCH(QByteArray, raw);
    QVERIFY(!AdifIntlMultilineString::check(raw.data()));

    QVERIFY(!AdifIntlMultilineString::create(raw.data()).has_value());
    auto obj2 = AdifIntlMultilineString::create("init");
    QVERIFY(!obj2->set(raw.data()));
    QCOMPARE(QString::fromStdString(obj2->get()), "init");
}

void TestAdifIntlMultilineString::test_AutoCorrection_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedStandardized");

    QTest::newRow("Fix LF in Unicode") << "📡\n中文"
                                       << "📡\r\n中文";
    QTest::newRow("Fix CR in Unicode") << "📡\r中文"
                                       << "📡\r\n中文";
    QTest::newRow("Fix Mixed breaks") << "Line1\nLine2\r\nLine3\r"
                                      << "Line1\r\nLine2\r\nLine3\r\n";
    QTest::newRow("Already Standard") << "📡\r\n🌏"
                                      << "📡\r\n🌏";
    QTest::newRow("Unexpected LF CR") << "📡\n\r🌏"
                                      << "📡\r\n🌏";
    QTest::newRow("Unexpected LF CR Plus") << "📡\n\r\n🌏"
                                           << "📡\r\n\r\n🌏";
    QTest::newRow("Long UTF-8 sequence") << "🌉\n\n🌃"
                                         << "🌉\r\n\r\n🌃";
}

void TestAdifIntlMultilineString::test_AutoCorrection() {
    QFETCH(QString, input);
    QFETCH(QString, expectedStandardized);

    std::string raw = input.toStdString();

    auto obj = AdifIntlMultilineString::create(raw);
    QVERIFY(obj.has_value());
    QCOMPARE(QString::fromStdString(obj->get()), expectedStandardized);

    auto obj2 = AdifIntlMultilineString::create("init");
    QVERIFY(obj2->set(raw));
    QCOMPARE(QString::fromStdString(obj2->get()), expectedStandardized);
}

void TestAdifIntlMultilineString::test_UnicodeIntegrity() {
    QString complexInput = "📡\n📡\r📡";
    QString expectedOutput = "📡\r\n📡\r\n📡";

    auto obj = AdifIntlMultilineString::create(complexInput.toStdString());
    QVERIFY(obj.has_value());

    QString result = QString::fromStdString(obj->get());
    QCOMPARE(result, expectedOutput);
    QVERIFY(result.contains("📡"));

    QVERIFY(AdifIntlMultilineString::check(obj->get()));
}

QTEST_MAIN(TestAdifIntlMultilineString)
#include "tst_adifintlmultilinestring.moc"