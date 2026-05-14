// tst_glogapplication_perf.cpp
#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QEvent>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QLineEdit>
#include <QObject>
#include <QSignalBlocker>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QThread>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "AwardPluginManager.h"
#include "DropAbleTableView.h"
#include "GLogApplication.h"
#include "LoadedAwardPlugin.h"
#include "MapWidget.h"
#include "adif_stress_generator.hpp"
#include "adifdb.h"
#include "ctydb.h"

namespace {

AdifModel *adifModel(GLogApplication *app) { return app->findChild<AdifModel *>(); }

/** Measures elapsed time from arm() until the first QEvent::Paint on the watched viewport. */
class ViewportPaintProbe final : public QObject {
  public:
    explicit ViewportPaintProbe(QObject *parent = nullptr) : QObject(parent) {}

    void arm() {
        sawFirstPaint = false;
        elapsedMsToFirstPaint = -1;
        m_timer.start();
    }

    bool sawFirstPaint = false;
    qint64 elapsedMsToFirstPaint = -1;

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        Q_UNUSED(watched);
        if (event->type() == QEvent::Paint && !sawFirstPaint) {
            sawFirstPaint = true;
            elapsedMsToFirstPaint = m_timer.elapsed();
        }
        return false;
    }

  private:
    QElapsedTimer m_timer;
};

static const char kPerfSlowPluginName[] = "PerfSlowFakePlugin";

static const char *perf_slow_pluginName() { return kPerfSlowPluginName; }

static bool perf_stub_bool_true() { return true; }

static int g_perfSlowSleepUs = 400;

static bool perf_stub_evaluate_slow(const IGRecord *, IGRecordGetValueByField) {
    QThread::usleep(static_cast<unsigned long>(g_perfSlowSleepUs));
    return true;
}

static void perf_stub_get_empty(char *result_buf, uint64_t *result_len, uint64_t max_result_len) {
    if (result_len != nullptr) {
        if (result_buf == nullptr) {
            *result_len = 0;
            return;
        }
        *result_len = 0;
    }
    if (result_buf != nullptr && max_result_len > 0) {
        result_buf[0] = '\0';
    }
}

static void perf_stub_get_result_ok(char *result_buf, uint64_t *result_len,
                                    uint64_t max_result_len) {
    static const char msg[] = "perf_ok";
    const uint64_t n = static_cast<uint64_t>(strlen(msg));
    if (result_len != nullptr && result_buf == nullptr) {
        *result_len = n;
        return;
    }
    if (result_buf != nullptr && result_len != nullptr && max_result_len > 0) {
        const uint64_t copy = (n < max_result_len) ? n : max_result_len - 1;
        memcpy(result_buf, msg, static_cast<size_t>(copy));
        result_buf[copy] = '\0';
        *result_len = copy;
    }
}

static LoadedAwardPlugin makeSlowFakeAwardPlugin(int sleepUs) {
    g_perfSlowSleepUs = sleepUs;
    LoadedAwardPlugin p;
    p.filename = QStringLiteral("/tmp/perf_fake_award_plugin");
    p.pluginName = perf_slow_pluginName;
    p.install = perf_stub_bool_true;
    p.uninstall = perf_stub_bool_true;
    p.beforeEvaluate = perf_stub_bool_true;
    p.afterEvaluate = perf_stub_bool_true;
    p.evaluate = perf_stub_evaluate_slow;
    p.getResult = perf_stub_get_result_ok;
    p.getLastError = perf_stub_get_empty;
    p.status->store(LoadedAwardPlugin::Status::Disabled);
    return p;
}

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

/** Wait for GLogApplication's async initCtyDB() to finish; do not call loadLocalDB in parallel. */
bool waitForCtyReady(int timeoutMs = 60000, bool logProgress = false) {
    QElapsedTimer t;
    t.start();
    qint64 lastLog = -5000;
    while (!CtyDB::instance()->ready() && t.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        if (logProgress) {
            const qint64 e = t.elapsed();
            if (e - lastLog >= 5000) {
                std::cerr << "[waitForCtyReady] elapsed_ms=" << e
                          << " ready=" << CtyDB::instance()->ready() << std::endl
                          << std::flush;
                lastLog = e;
            }
        }
    }
    if (logProgress) {
        std::cerr << "[waitForCtyReady] exit elapsed_ms=" << t.elapsed()
                  << " ready=" << CtyDB::instance()->ready() << std::endl
                  << std::flush;
    }
    return CtyDB::instance()->ready();
}

/** Deterministic ADIF: every row has a valid CALL (avoids sparse remap edge cases). */
std::string makeAdifRepeatingW1aw(int count) {
    std::ostringstream oss;
    oss << "ADIF 3.1.4\n<EOH>\n";
    for (int i = 0; i < count; ++i) {
        oss << "<CALL:4>W1AW\n"
            << "<BAND:3>20M\n"
            << "<MODE:3>SSB\n"
            << "<QSO_DATE:8>20240101\n"
            << "<TIME_ON:4>1200\n"
            << "<EOR>\n";
    }
    return oss.str();
}

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

    // UI / 搜索 / 奖项 / 地图 延迟与耗时（可观测指标）
    void testViewportFirstPaintLatency();
    void testSearchLatency_data();
    void testSearchLatency();
    void testAwardEvaluation_withFakeSlowPlugin();
    void testMapDataVisualizeLatency();

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

void GLogApplicationTest::testViewportFirstPaintLatency() {
    constexpr int kRows = 120;
    std::string content = adif_stress::Generator::makeVolume(static_cast<size_t>(kRows));
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    QVERIFY(waitForOpenFile(tempFile.fileName()).empty());

    auto *tv = m_app->findChild<DropAbleTableView *>();
    QVERIFY(tv != nullptr);
    QWidget *vp = tv->viewport();
    QVERIFY(vp != nullptr);

    ViewportPaintProbe probe;
    vp->installEventFilter(&probe);

    m_app->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_app));

    probe.arm();
    vp->update();

    QElapsedTimer wait;
    wait.start();
    while (!probe.sawFirstPaint && wait.elapsed() < 10000) {
        QCoreApplication::processEvents();
    }
    QVERIFY2(probe.sawFirstPaint, "Expected a paint event on the table viewport");
    qDebug() << "viewport first paint after update (ms):" << probe.elapsedMsToFirstPaint;
    QVERIFY(probe.elapsedMsToFirstPaint >= 0);
    QVERIFY(probe.elapsedMsToFirstPaint < 30000);

    vp->removeEventFilter(&probe);
}

void GLogApplicationTest::testSearchLatency_data() {
    QTest::addColumn<int>("rows");
    int rows = 800;
    if (qEnvironmentVariableIsSet("ADIF_SEARCH_STRESS")) {
        rows = 5000;
    }
    QTest::newRow("search_rows") << rows;
}

void GLogApplicationTest::testSearchLatency() {
    QFETCH(int, rows);

    std::string content = adif_stress::Generator::makeVolume(static_cast<size_t>(rows));
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    QVERIFY(waitForOpenFile(tempFile.fileName()).empty());

    auto *tv = m_app->findChild<DropAbleTableView *>();
    QVERIFY(tv != nullptr);
    tv->show();
    QVERIFY(QTest::qWaitForWindowExposed(tv));

    QElapsedTimer t;
    t.start();
    constexpr int kIterations = 25;
    for (int i = 0; i < kIterations; ++i) {
        tv->findNext(QStringLiteral("call"), QStringLiteral("JA"), false);
    }
    const qint64 ms = t.elapsed();
    qDebug() << "search:" << kIterations << "x findNext on" << rows << "rows took" << ms << "ms";

    if (rows >= 5000 && ms > 30000) {
        qWarning() << "testSearchLatency: soft budget exceeded (" << ms << "ms)";
    }
    QVERIFY(ms < 120000);
}

void GLogApplicationTest::testAwardEvaluation_withFakeSlowPlugin() {
    if (!waitForCtyReady()) {
        QSKIP("CTY database not ready (async initCtyDB did not finish in time)");
    }

    constexpr int kRows = 40;
    constexpr int kSleepUs = 400;
    std::string content = makeAdifRepeatingW1aw(kRows);
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();
    QVERIFY(waitForOpenFile(tempFile.fileName()).empty());

    auto *mgr = m_app->findChild<AwardPluginManager *>();
    QVERIFY(mgr != nullptr);
    mgr->show();
    QCoreApplication::processEvents();

    LoadedAwardPlugin plugin = makeSlowFakeAwardPlugin(kSleepUs);
    QVERIFY(plugin.valid());
    mgr->loadAwardPluginAfterConfirmed(std::move(plugin));
    QCoreApplication::processEvents();

    const QPersistentModelIndex pix(mgr->index(0));
    QVERIFY(pix.isValid());
    mgr->installAwardPluginAfterConfirmed(pix);
    QTRY_VERIFY2(mgr->index(0).data(Qt::DisplayRole).toString().length() > 0, "plugin row settled");

    QAction *award = m_app->findChild<QAction *>(QStringLiteral("actionAward"));
    QVERIFY(award != nullptr);

    QSignalSpy perfSpy(m_app, &GLogApplication::perfAwardReportFinished);
    award->trigger();
    QTRY_VERIFY(perfSpy.count() >= 1);

    const qint64 us = perfSpy.at(0).at(0).toLongLong();
    qDebug() << "perfAwardReportFinished us:" << us;
    const qint64 expectedMinUs = static_cast<qint64>(kRows) * kSleepUs * 8 / 10;
    QVERIFY2(us >= expectedMinUs,
             "AwardEntityCountReport::format should reflect fake plugin evaluate() cost");
}

void GLogApplicationTest::testMapDataVisualizeLatency() {
    qDebug() << "[testMapDataVisualizeLatency] M01 enter thread" << QThread::currentThreadId();
    std::cerr << "[testMapDataVisualizeLatency] M01 enter thread" << std::endl << std::flush;

    qDebug() << "[testMapDataVisualizeLatency] M02 before waitForCtyReady";
    std::cerr << "[testMapDataVisualizeLatency] M02 before waitForCtyReady" << std::endl
              << std::flush;
    if (!waitForCtyReady(60000, true)) {
        qDebug() << "[testMapDataVisualizeLatency] M03 waitForCtyReady -> QSKIP";
        std::cerr << "[testMapDataVisualizeLatency] M03 waitForCtyReady -> QSKIP" << std::endl
                  << std::flush;
        QSKIP("CTY database not ready (async initCtyDB did not finish in time)");
    }
    qDebug() << "[testMapDataVisualizeLatency] M04 after waitForCtyReady ok";
    std::cerr << "[testMapDataVisualizeLatency] M04 after waitForCtyReady ok" << std::endl
              << std::flush;

    constexpr int kRows = 200;
    std::string content = makeAdifRepeatingW1aw(kRows);
    qDebug() << "[testMapDataVisualizeLatency] M05 ADIF content bytes" << content.size();
    std::cerr << "[testMapDataVisualizeLatency] M05 ADIF content bytes " << content.size()
              << std::endl
              << std::flush;

    QTemporaryFile tempFile;
    qDebug() << "[testMapDataVisualizeLatency] M06 before tempFile.open()";
    std::cerr << "[testMapDataVisualizeLatency] M06 before tempFile.open()" << std::endl
              << std::flush;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    qDebug() << "[testMapDataVisualizeLatency] M07 after tempFile.open() path"
             << tempFile.fileName();
    std::cerr << "[testMapDataVisualizeLatency] M07 after tempFile.open" << std::endl << std::flush;

    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    qDebug() << "[testMapDataVisualizeLatency] M08 after tempFile.write";
    std::cerr << "[testMapDataVisualizeLatency] M08 after tempFile.write" << std::endl
              << std::flush;

    tempFile.close();
    qDebug() << "[testMapDataVisualizeLatency] M09 after tempFile.close";
    std::cerr << "[testMapDataVisualizeLatency] M09 after tempFile.close" << std::endl
              << std::flush;

    qDebug() << "[testMapDataVisualizeLatency] M10 before waitForOpenFile";
    std::cerr << "[testMapDataVisualizeLatency] M10 before waitForOpenFile" << std::endl
              << std::flush;
    QVERIFY(waitForOpenFile(tempFile.fileName()).empty());
    qDebug() << "[testMapDataVisualizeLatency] M11 after waitForOpenFile ok rows"
             << model()->rowCount();
    std::cerr << "[testMapDataVisualizeLatency] M11 after waitForOpenFile ok" << std::endl
              << std::flush;

    auto *map = m_app->findChild<MapWidget *>();
    qDebug() << "[testMapDataVisualizeLatency] M12 after findChild<MapWidget> ptr" << (void *)map;
    std::cerr << "[testMapDataVisualizeLatency] M12 findChild MapWidget" << std::endl << std::flush;
    QVERIFY(map != nullptr);

    qDebug() << "[testMapDataVisualizeLatency] M13 before map->show()";
    std::cerr << "[testMapDataVisualizeLatency] M13 before map->show()" << std::endl << std::flush;
    map->show();
    qDebug() << "[testMapDataVisualizeLatency] M14 after map->show()";
    std::cerr << "[testMapDataVisualizeLatency] M14 after map->show()" << std::endl << std::flush;

    qDebug() << "[testMapDataVisualizeLatency] M15 before qWaitForWindowExposed";
    std::cerr << "[testMapDataVisualizeLatency] M15 before qWaitForWindowExposed" << std::endl
              << std::flush;
    QVERIFY(QTest::qWaitForWindowExposed(map));
    qDebug() << "[testMapDataVisualizeLatency] M16 after qWaitForWindowExposed";
    std::cerr << "[testMapDataVisualizeLatency] M16 after qWaitForWindowExposed" << std::endl
              << std::flush;

    QElapsedTimer markerWait;
    markerWait.start();
    qDebug() << "[testMapDataVisualizeLatency] M17 entering marker wait loop isMapMarkersReady="
             << map->isMapMarkersReady();
    std::cerr << "[testMapDataVisualizeLatency] M17 entering marker wait loop" << std::endl
              << std::flush;
    qint64 lastLoopLogMs = -5000;
    while (!map->isMapMarkersReady() && markerWait.elapsed() < 120000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        const qint64 elapsed = markerWait.elapsed();
        if (elapsed - lastLoopLogMs >= 5000) {
            qDebug() << "[testMapDataVisualizeLatency] M17-loop elapsed_ms=" << elapsed
                     << "isMapMarkersReady=" << map->isMapMarkersReady();
            std::cerr << "[testMapDataVisualizeLatency] M17-loop elapsed_ms=" << elapsed
                      << " isMapMarkersReady=" << map->isMapMarkersReady() << std::endl
                      << std::flush;
            lastLoopLogMs = elapsed;
        }
    }
    qDebug() << "[testMapDataVisualizeLatency] M18 exited marker wait loop ready="
             << map->isMapMarkersReady() << "elapsed_ms=" << markerWait.elapsed();
    std::cerr << "[testMapDataVisualizeLatency] M18 exited marker wait loop elapsed_ms="
              << markerWait.elapsed() << std::endl
              << std::flush;

    if (!map->isMapMarkersReady()) {
        qDebug() << "[testMapDataVisualizeLatency] M19 QFAIL markers never ready";
        std::cerr << "[testMapDataVisualizeLatency] M19 QFAIL markers never ready" << std::endl
                  << std::flush;
        QFAIL("CTY markers should finish initializing");
    }
    qDebug() << "[testMapDataVisualizeLatency] M20 markers ready confirmed";
    std::cerr << "[testMapDataVisualizeLatency] M20 markers ready confirmed" << std::endl
              << std::flush;

    auto *center = map->findChild<QLineEdit *>(QStringLiteral("centerCallEdit"));
    qDebug() << "[testMapDataVisualizeLatency] M21 after findChild<centerCallEdit> ptr"
             << (void *)center;
    std::cerr << "[testMapDataVisualizeLatency] M21 findChild centerCallEdit" << std::endl
              << std::flush;
    QVERIFY(center != nullptr);

    qDebug() << "[testMapDataVisualizeLatency] M22 before QSignalBlocker + setText W1AW";
    std::cerr << "[testMapDataVisualizeLatency] M22 before setText" << std::endl << std::flush;
    {
        const QSignalBlocker blocker(center);
        center->setText(QStringLiteral("W1AW"));
    }
    qDebug() << "[testMapDataVisualizeLatency] M23 after setText W1AW";
    std::cerr << "[testMapDataVisualizeLatency] M23 after setText W1AW" << std::endl << std::flush;

    QSignalSpy doneSpy(map, &MapWidget::mapDataVisualizeFinished);
    qDebug() << "[testMapDataVisualizeLatency] M24 after QSignalSpy doneSpy constructed";
    std::cerr << "[testMapDataVisualizeLatency] M24 after QSignalSpy" << std::endl << std::flush;

    QElapsedTimer wall;
    wall.start();
    qDebug() << "[testMapDataVisualizeLatency] M25 before map->dataVisualize()";
    std::cerr << "[testMapDataVisualizeLatency] M25 before dataVisualize" << std::endl
              << std::flush;
    map->dataVisualize();
    qDebug() << "[testMapDataVisualizeLatency] M26 after map->dataVisualize() returned wall_ms="
             << wall.elapsed() << "doneSpy.count=" << doneSpy.count();
    std::cerr << "[testMapDataVisualizeLatency] M26 after dataVisualize wall_ms=" << wall.elapsed()
              << std::endl
              << std::flush;

    qDebug() << "[testMapDataVisualizeLatency] M27 before QTRY_VERIFY mapDataVisualizeFinished";
    std::cerr << "[testMapDataVisualizeLatency] M27 before QTRY_VERIFY" << std::endl << std::flush;
    QTRY_VERIFY2(doneSpy.count() >= 1, "mapDataVisualize should complete");
    qDebug() << "[testMapDataVisualizeLatency] M28 after QTRY_VERIFY doneSpy.count="
             << doneSpy.count() << "wall_ms=" << wall.elapsed();
    std::cerr << "[testMapDataVisualizeLatency] M28 after QTRY_VERIFY" << std::endl << std::flush;

    const qint64 ms = wall.elapsed();
    qDebug() << "[testMapDataVisualizeLatency] M29 map dataVisualize wall clock ms:" << ms;
    std::cerr << "[testMapDataVisualizeLatency] M29 wall clock ms=" << ms << std::endl
              << std::flush;

    constexpr qint64 kSoftBudgetMs = 120000;
    if (ms >= kSoftBudgetMs) {
        qWarning() << "testMapDataVisualizeLatency: took" << ms << "ms (soft budget"
                   << kSoftBudgetMs << "ms)";
    }
    qDebug() << "[testMapDataVisualizeLatency] M30 before final QVERIFY";
    std::cerr << "[testMapDataVisualizeLatency] M30 before final QVERIFY" << std::endl
              << std::flush;
    QVERIFY(ms < kSoftBudgetMs);
    qDebug() << "[testMapDataVisualizeLatency] M31 exit ok";
    std::cerr << "[testMapDataVisualizeLatency] M31 exit ok" << std::endl << std::flush;
}

QTEST_MAIN(GLogApplicationTest)
#include "tst_glogapplication_perf.moc"