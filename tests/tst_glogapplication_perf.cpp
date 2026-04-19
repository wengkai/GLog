// tst_glogapplication_perf.cpp
#include <QDebug>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include <chrono>

#include "GLogApplication.h"
#include "adif_stress_generator.hpp"

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

  private:
    GLogApplication *m_app = nullptr;

    // 同步等待 openFile 完成，返回错误列表（若抛出异常则将其 what() 放入列表）
    std::vector<std::string> waitForOpenFile(const QString &filename);
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
        errors.push_back("Unknown exception");
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
    qDebug() << "LargeField errors:" << errors.size();
    QVERIFY(true); // 仅要求不崩溃
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
    QVERIFY(true);
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
    QVERIFY(true);
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
    QVERIFY(true);
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
    QVERIFY(true);
}

void GLogApplicationTest::testEncodingAndSpecialChars() {
    std::string content =
        adif_stress::Generator::makeEncodingTest() + adif_stress::Generator::makeSpecialChars();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QVERIFY2(false, "Failed to create temp file");
        return;
    }
    tempFile.write(content.data(), static_cast<qint64>(content.size()));
    tempFile.close();

    std::vector<std::string> errors = waitForOpenFile(tempFile.fileName());
    QVERIFY(true);
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
    qDebug() << "InvalidDateTime errors:" << errors.size();
    QVERIFY(true);
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
    QVERIFY(true);
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
    QVERIFY(true);
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

    QTest::newRow("1k QSOs") << 1000 << "1,000 records";
    QTest::newRow("10k QSOs") << 10000 << "10,000 records";
    QTest::newRow("50k QSOs") << 50000 << "50,000 records";
    QTest::newRow("100k QSOs") << 100000 << "100,000 records";
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

    // 100k 记录应在 10 秒内完成（可根据实际硬件调整阈值）
    if (recordCount == 100000) {
        QVERIFY(duration < 10000);
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
    QVERIFY(duration < 120000); // 2 分钟内完成
}

QTEST_MAIN(GLogApplicationTest)
#include "tst_glogapplication_perf.moc"