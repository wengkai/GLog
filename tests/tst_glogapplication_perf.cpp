// tst_glogapplication_perf.cpp
#include <QDebug>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include "GLogApplication.h"
#include "adif_stress_generator.hpp"
#include "adifdb.h"

namespace {

AdifModel *adifModel(GLogApplication *app) { return app->findChild<AdifModel *>(); }

int columnOf(const AdifModel *m, const char *field) {
    for (int c = 0; c < m->columnCount(); ++c) {
        if (m->headerData(c, Qt::Horizontal).toString() == QLatin1String(field)) {
            return c;
        }
    }
    return -1;
}

QString cellAt(const AdifModel *m, int row, const char *field) {
    const int c = columnOf(m, field);
    if (c < 0) {
        return {};
    }
    return m->data(m->index(row, c), Qt::DisplayRole).toString();
}

bool hasColumn(const AdifModel *m, const char *field) { return columnOf(m, field) >= 0; }

} // namespace

class GLogApplicationTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();

    // 正确性测试
    void testValidSmallFile();
    void testLargeField();
    void testNestedTag();
    void testUnknownTag();
    void testEmptyTag();
    void testOutOfOrderTags();
    void testEncodingAndSpecialChars();
    void testInvalidDateTime();
    void testInvalidMode();
    void testInvalidFrequency();
    void testTruncatedInTag();
    void testLengthMismatch();
    void testMissingTerminator();

    // 性能压测
    void testVolume_data();
    void testVolume();

    // 极限压测（需环境变量启用）
    void testExtremeVolume();

  protected:
    GLogApplication *m_app = nullptr;

    // 同步等待 openFile 完成，返回错误列表（若抛出异常则将其 what() 放入列表）
    std::vector<std::string> waitForOpenFile(const QString &filename);

    /** After openFile completes, returns the live model (child of the main window). */
    AdifModel *model() const { return adifModel(m_app); }
};

void GLogApplicationTest::initTestCase() {
    m_app = new GLogApplication();
    QVERIFY(m_app != nullptr);
}

void GLogApplicationTest::cleanupTestCase() {
    delete m_app;
    m_app = nullptr;
}

std::vector<std::string> GLogApplicationTest::waitForOpenFile(const QString &filename) {
    QFuture<std::vector<std::string>> future = m_app->openFile(filename);

    QFutureWatcher<std::vector<std::string>> watcher;
    QEventLoop loop;
    QObject::connect(&watcher, &QFutureWatcher<std::vector<std::string>>::finished, &loop,
                     &QEventLoop::quit);

    watcher.setFuture(future);
    loop.exec();

    std::vector<std::string> errors;
    try {
        errors = future.result();
    } catch (const std::exception &e) {
        errors.push_back(std::string("Exception: ") + e.what());
    } catch (...) {
        errors.emplace_back("Unknown exception");
    }
    return errors;
}

// ----------------------------------------------------------------------------
// 正确性测试（每个测试内部自行创建临时文件）
// ----------------------------------------------------------------------------

void GLogApplicationTest::testValidSmallFile() {
    std::string content = "ADIF 3.1.4\n"
                          "<EOH>\n"
                          "<CALL:4>W1AW\n"
                          "<FREQ:4>14.1"
                          "<BAND:3>20M\n"
                          "<MODE:3>SSB\n"
                          "<EOR>\n";

    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QCOMPARE(errors.size(), size_t(0));
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 1);
    QCOMPARE(cellAt(m, 0, "call"), QStringLiteral("W1AW"));
    QCOMPARE(cellAt(m, 0, "band"), QStringLiteral("20M"));
    QCOMPARE(cellAt(m, 0, "mode"), QStringLiteral("SSB"));
}

void GLogApplicationTest::testLargeField() {
    std::string content = adif_stress::Generator::makeLargeField(1024);
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY2(errors.empty(), "Large-field ADIF should parse without driver errors");
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 1);
    const QString call = cellAt(m, 0, "call");
    QCOMPARE(call.size(), 1024);
    QVERIFY(std::all_of(call.begin(), call.end(),
                        [](const QChar &ch) { return ch == QLatin1Char('X'); }));
}

void GLogApplicationTest::testNestedTag() {
    std::string content = adif_stress::Generator::makeNestedTag();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY2(errors.empty(), "Nested literal angle brackets in field data should not fail parse");
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 1);
    QVERIFY2(hasColumn(m, "notes"), "NOTES field should appear as a column");
    QCOMPARE(cellAt(m, 0, "notes"), QStringLiteral("Contains <TAG>\r\n"));
}

void GLogApplicationTest::testUnknownTag() {
    std::string content = adif_stress::Generator::makeUnknownTag();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY2(errors.empty(), "Unknown ADIF field should still parse");
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 1);
    QCOMPARE(cellAt(m, 0, "call"), QStringLiteral("W1AW"));
    QVERIFY2(hasColumn(m, "my_new_field"), "Unknown tag should become a dynamic column");
    QCOMPARE(cellAt(m, 0, "my_new_field"), QStringLiteral("HELLO"));
}

void GLogApplicationTest::testEmptyTag() {
    std::string content = adif_stress::Generator::makeEmptyTag();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY2(errors.empty(), "Zero-length field and trailing EOM should parse");
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 1);
    QCOMPARE(cellAt(m, 0, "call"), QStringLiteral("W1AW"));
    QVERIFY2(hasColumn(m, "notes"), "NOTES tag should exist even with length 0");
    QVERIFY(cellAt(m, 0, "notes").isEmpty());
}

void GLogApplicationTest::testOutOfOrderTags() {
    std::string content = adif_stress::Generator::makeOutOfOrderTags();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY2(errors.empty(), "Out-of-order tags should still produce one valid row");
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 1);
    QCOMPARE(cellAt(m, 0, "call"), QStringLiteral("W1AW"));
    QCOMPARE(cellAt(m, 0, "band"), QStringLiteral("20M"));
    QCOMPARE(cellAt(m, 0, "mode"), QStringLiteral("SSB"));
    QCOMPARE(cellAt(m, 0, "qso_date"), QStringLiteral("20240315"));
}

void GLogApplicationTest::testEncodingAndSpecialChars() {
    std::string content = adif_stress::Generator::makeEncodingTest() +
                          adif_stress::Generator::makeSpecialChars(false);
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY2(errors.empty(), "No utf-8 errors");
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 2);
    QCOMPARE(cellAt(m, 0, "name_intl"), QStringLiteral("W1AW/Ø"));
    QVERIFY(cellAt(m, 0, "address_intl").contains(QChar(0x00B0)));
    QCOMPARE(cellAt(m, 0, "qslmsg_intl"), QStringLiteral("Café con leche"));
    QVERIFY2(hasColumn(m, "notes"), "Second record should carry NOTES");
    QVERIFY(cellAt(m, 1, "notes").contains(QLatin1String("AT&T")));
    QVERIFY(cellAt(m, 1, "notes").contains(QLatin1Char('<')));
}

void GLogApplicationTest::testInvalidDateTime() {
    std::string content = adif_stress::Generator::makeInvalidDateTime();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 1);
    QCOMPARE(cellAt(m, 0, "call"), QStringLiteral("W1AW"));
    QVERIFY2(cellAt(m, 0, "qso_date") != QStringLiteral("20261332"),
             "Invalid QSO_DATE must not be stored as typed value");
    QVERIFY2(cellAt(m, 0, "time_on") != QStringLiteral("2560"),
             "Invalid TIME_ON must not be stored as typed value");
    QVERIFY2(!cellAt(m, 0, "time_off").contains(QLatin1Char(':')),
             "TIME_OFF with ':' is invalid and must not appear as HHMM");
}

void GLogApplicationTest::testInvalidMode() {
    std::string content = adif_stress::Generator::makeInvalidMode();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 1);
    QCOMPARE(cellAt(m, 0, "call"), QStringLiteral("W1AW"));
    QVERIFY2(cellAt(m, 0, "mode") != QStringLiteral("NO_SUCH_MODE"),
             "Invalid MODE must not be applied to the model");
    QVERIFY2(cellAt(m, 0, "submode") != QStringLiteral("FT999"),
             "Invalid SUBMODE must not be applied to the model");
}

void GLogApplicationTest::testInvalidFrequency() {
    std::string content = adif_stress::Generator::makeInvalidFrequency();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), 2);
    QCOMPARE(cellAt(m, 0, "call"), QStringLiteral("W1AW"));
    QVERIFY2(cellAt(m, 0, "freq") != QStringLiteral("-7.0"), "Negative FREQ must not be stored");
    QVERIFY2(cellAt(m, 1, "freq") != QStringLiteral("99999999999999999999.99"),
             "Out-of-range FREQ must not be stored as given");
}

void GLogApplicationTest::testTruncatedInTag() {
    std::string content = adif_stress::Generator::makeTruncatedInTag();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY(!errors.empty()); // 应产生解析错误
}

void GLogApplicationTest::testLengthMismatch() {
    std::string content = adif_stress::Generator::makeLengthLongerThanData() + "\n" +
                          adif_stress::Generator::makeLengthShorterThanData();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY(!errors.empty());
}

void GLogApplicationTest::testMissingTerminator() {
    std::string content = adif_stress::Generator::makeMissingTerminator();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY(!errors.empty());
}

// ----------------------------------------------------------------------------
// 性能压测
// ----------------------------------------------------------------------------

void GLogApplicationTest::testVolume_data() {
    QTest::addColumn<int>("recordCount");
    QTest::addColumn<QString>("description");

    // Default row stays small: makeVolume() is all in-memory; under OpenCppCoverage (especially
    // Debug) 1k+ QSOs can time out or OOM and abort the process, so the HTML report has no module
    // for this exe. Use ADIF_VOLUME_STRESS=1 for 1k/10k/50k/100k runs.
    QTest::newRow("200 QSOs") << 200 << "200 records";
    if (qEnvironmentVariableIsSet("ADIF_VOLUME_STRESS")) {
        QTest::newRow("1k QSOs") << 1000 << "1,000 records";
        QTest::newRow("10k QSOs") << 10000 << "10,000 records";
        QTest::newRow("50k QSOs") << 50000 << "50,000 records";
        QTest::newRow("100k QSOs") << 100000 << "100,000 records";
    }
}

void GLogApplicationTest::testVolume() {
    QFETCH(int, recordCount);
    QFETCH(QString, description);

    // 生成测试数据（内存版本，适用于 100k 以内）
    std::string content = adif_stress::Generator::makeVolume(static_cast<size_t>(recordCount));

    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    qDebug() << "Testing" << description << "file:" << tempFile.fileName();

    auto start = std::chrono::steady_clock::now();
    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    qDebug() << "Parsed" << recordCount << "QSOs in" << duration << "ms, errors:" << errors.size();

    QVERIFY2(errors.empty(), "Volume stress ADIF should parse without driver errors");
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), recordCount);

    // 100k 参考预算 10s：超时仅告警（插桩/低配机器上仍应通过用例）
    if (recordCount == 100000) {
        constexpr qint64 kBudgetMs = 10000;
        if (duration >= kBudgetMs) {
            qWarning() << "testVolume:" << recordCount << "QSOs took" << duration
                       << "ms (soft budget" << kBudgetMs << "ms)";
        }
    }
}

void GLogApplicationTest::testExtremeVolume() {
    if (!qEnvironmentVariableIsSet("ADIF_EXTREME_TEST")) {
        QSKIP("Skipping extreme volume test (set ADIF_EXTREME_TEST=1 to enable)");
    }

    const int hugeCount = 1000000; // 1M QSOs

    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.close();
    QString path = tempFile.fileName();

    // 使用流式生成器写入 1M 条记录，避免内存爆炸
    adif_stress::Generator::writeVolumeFile(path.toStdString(), hugeCount);
    qDebug() << "Extreme file generated:" << path;

    auto start = std::chrono::steady_clock::now();
    std::vector<std::string> errors = waitForOpenFile(path);
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    qDebug() << "Parsed 1,000,000 QSOs in" << duration << "ms, errors:" << errors.size();

    QVERIFY2(errors.empty(), "Extreme volume ADIF should parse without driver errors");
    AdifModel *m = model();
    QVERIFY(m != nullptr);
    QCOMPARE(m->rowCount(), hugeCount);

    constexpr qint64 kBudgetMs = 120000; // 参考 2 分钟
    if (duration >= kBudgetMs) {
        qWarning() << "testExtremeVolume: 1M QSOs took" << duration << "ms (soft budget"
                   << kBudgetMs << "ms)";
    }
}

QTEST_MAIN(GLogApplicationTest)
#include "tst_glogapplication_perf.moc"