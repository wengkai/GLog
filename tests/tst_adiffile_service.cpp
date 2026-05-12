#include <QtTest>
#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFutureWatcher>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTimer>
#include <QWidget>
#include "adifdb.h"
#include "adiffile_service.h"
#include "record.h"

class AdifFileServiceTest : public QObject {
    Q_OBJECT

  private slots:
    void test_openFileAsync_loadsRows();
    void test_openFileAsync_missingFile_throws();
    void test_openFileAsync_remove_deletesSourceFile();
    void test_insertFileAsync_success_and_parseError();
    void test_insertStringDataAsync_success_and_parseError();
    void test_saveAs_roundTrip_adi();
    void test_saveAs_unsupportedExtension_triggersFailurePath();

  private:
    template <typename T> static void waitForFuture(const QFuture<T> &future) {
        if (future.isFinished()) {
            return;
        }
        QEventLoop loop;
        QFutureWatcher<T> watcher;
        watcher.setFuture(future);
        QObject::connect(&watcher, &QFutureWatcher<T>::finished, &loop, &QEventLoop::quit);
        loop.exec();
    }

    static void drainUi() {
        for (int i = 0; i < 50; ++i) {
            QCoreApplication::processEvents();
        }
    }

    AdifModel m_model;
};

void AdifFileServiceTest::test_openFileAsync_loadsRows() {
    const std::string content = "ADIF 3.1.4\n"
                                "<EOH>\n"
                                "<CALL:4>W1AW\n"
                                "<BAND:3>20M\n"
                                "<MODE:3>SSB\n"
                                "<EOR>\n";

    QTemporaryFile f;
    QVERIFY(f.open());
    f.write(content.data(), static_cast<qint64>(content.size()));
    f.flush();
    const QString path = f.fileName();
    f.close();

    AdifFileService svc(&m_model);
    auto fut = svc.openFileAsync(path, false);
    waitForFuture(fut);
    std::vector<std::string> errors;
    try {
        errors = fut.result();
    } catch (const std::exception &e) {
        QFAIL(e.what());
    }
    QCOMPARE(errors.size(), size_t(0));
    drainUi();
    QCOMPARE(m_model.rowCount(), 1);
}

void AdifFileServiceTest::test_openFileAsync_missingFile_throws() {
    AdifFileService svc(&m_model);
    auto fut = svc.openFileAsync(QStringLiteral("/no/such/glog_adif_file_12345.adi"), false);
    waitForFuture(fut);
    bool threw = false;
    try {
        (void)fut.result();
    } catch (const std::exception &) {
        threw = true;
    }
    QVERIFY(threw);
}

void AdifFileServiceTest::test_saveAs_roundTrip_adi() {
    AdifModel model;
    std::vector<GRecord> one;
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W2YY"));
    one.push_back(std::move(r));
    model.insertRecords(0, std::move(one));
    drainUi();

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString outPath = dir.path() + QStringLiteral("/out.adi");

    AdifFileService svc(&model);
    bool done = false;
    QObject::connect(&svc, &AdifFileService::saveDone, [&done]() { done = true; });
    svc.saveAs(outPath);

    QElapsedTimer t;
    t.start();
    while (!done && t.elapsed() < 8000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    QVERIFY2(done, "saveDone not received");

    QVERIFY(QFile::exists(outPath));
    QVERIFY(QFile(outPath).size() > 10);

    AdifModel model2;
    AdifFileService svc2(&model2);
    auto fut = svc2.openFileAsync(outPath, false);
    waitForFuture(fut);
    try {
        (void)fut.result();
    } catch (const std::exception &e) {
        QFAIL(e.what());
    }
    drainUi();
    QCOMPARE(model2.rowCount(), 1);
}

void AdifFileServiceTest::test_openFileAsync_remove_deletesSourceFile() {
    const std::string content = "ADIF 3.1.4\n"
                                "<EOH>\n"
                                "<CALL:4>W9XX\n"
                                "<BAND:3>20M\n"
                                "<MODE:3>SSB\n"
                                "<EOR>\n";

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/remove_me.adi");
    {
        QFile out(path);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.write(content.data(), static_cast<qint64>(content.size()));
        out.close();
    }
    QVERIFY(QFile::exists(path));

    AdifModel localModel;
    AdifFileService svc(&localModel);
    auto fut = svc.openFileAsync(path, /*remove=*/true);
    waitForFuture(fut);
    try {
        (void)fut.result();
    } catch (const std::exception &e) {
        QFAIL(e.what());
    }
    drainUi();
    QCOMPARE(localModel.rowCount(), 1);
    QVERIFY(!QFile::exists(path));
}

void AdifFileServiceTest::test_insertFileAsync_success_and_parseError() {
    AdifModel localModel;
    {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", "EXIST"));
        localModel.insertRecords(0, std::vector<GRecord>{std::move(r)});
        drainUi();
    }
    QCOMPARE(localModel.rowCount(), 1);

    const std::string content = "<CALL:5>VKINS\n<EOR>\n";
    QTemporaryFile good;
    QVERIFY(good.open());
    good.write(content.data(), static_cast<qint64>(content.size()));
    good.flush();
    const QString goodPath = good.fileName();
    good.close();

    AdifFileService svc(&localModel);
    auto fut = svc.insertFileAsync(0, goodPath, false);
    waitForFuture(fut);
    try {
        (void)fut.result();
    } catch (const std::exception &e) {
        QFAIL(e.what());
    }
    drainUi();
    QCOMPARE(localModel.rowCount(), 2);

    QTemporaryFile bad;
    QVERIFY(bad.open());
    const std::string garbage = "this is not adif content";
    bad.write(garbage.data(), static_cast<qint64>(garbage.size()));
    bad.flush();
    const QString badPath = bad.fileName();
    bad.close();

    auto badFut = svc.insertFileAsync(localModel.rowCount(), badPath, false);
    waitForFuture(badFut);
    bool threw = false;
    try {
        (void)badFut.result();
    } catch (const std::exception &) {
        threw = true;
    }
    QVERIFY(threw);
    drainUi();
    QCOMPARE(localModel.rowCount(), 2);

    auto missingFut = svc.insertFileAsync(0, QStringLiteral("/no/such/insert_missing.adi"), false);
    waitForFuture(missingFut);
    bool missingThrew = false;
    try {
        (void)missingFut.result();
    } catch (const std::exception &) {
        missingThrew = true;
    }
    QVERIFY(missingThrew);
}

void AdifFileServiceTest::test_insertStringDataAsync_success_and_parseError() {
    AdifModel localModel;
    AdifFileService svc(&localModel);

    auto okFut = svc.insertStringDataAsync(0, std::string("<CALL:5>SD0XX\n<EOR>\n"));
    waitForFuture(okFut);
    try {
        (void)okFut.result();
    } catch (const std::exception &e) {
        QFAIL(e.what());
    }
    drainUi();
    QCOMPARE(localModel.rowCount(), 1);

    auto badFut =
        svc.insertStringDataAsync(localModel.rowCount(), std::string("just text no adif"));
    waitForFuture(badFut);
    bool threw = false;
    try {
        (void)badFut.result();
    } catch (const std::exception &) {
        threw = true;
    }
    QVERIFY(threw);
    drainUi();
    QCOMPARE(localModel.rowCount(), 1);
}

void AdifFileServiceTest::test_saveAs_unsupportedExtension_triggersFailurePath() {
    AdifModel model;
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "K9XX"));
    model.insertRecords(0, std::vector<GRecord>{std::move(r)});
    drainUi();

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString outPath = dir.path() + QStringLiteral("/out.unsupported_ext");

    AdifFileService svc(&model);
    bool saveDoneEmitted = false;
    QObject::connect(&svc, &AdifFileService::saveDone,
                     [&saveDoneEmitted]() { saveDoneEmitted = true; });

    QSignalSpy saveFailedSpy(&svc, &AdifFileService::saveFailed);
    svc.saveAs(outPath);

    QVERIFY2(saveFailedSpy.wait(8000), "expected saveFailed signal from unsupported extension");
    QVERIFY(!saveDoneEmitted);
    QVERIFY(!QFile::exists(outPath));
    QCOMPARE(saveFailedSpy.count(), 1);
    const QString err = saveFailedSpy.at(0).at(0).toString();
    QVERIFY2(err.contains(QStringLiteral("Unsupported"), Qt::CaseInsensitive),
             qPrintable(QStringLiteral("unexpected saveFailed message: ") + err));
}

QTEST_MAIN(AdifFileServiceTest)
#include "tst_adiffile_service.moc"
