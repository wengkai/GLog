#include <QtTest>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QMimeData>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QThread>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "GLogApplication.h"
#include "adifdb.h"
#include "adiffile_service.h"
#include "adifparse_service.h"
#include "record.h"
#include "recordrepository.h"

class AdifModelTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void test_defaults_rowColumn_headerData_data_flags();
    void test_addHeader_uniqueness();
    void test_getDefaultSortModel();
    void test_insertRows_returnsFalse();
    void test_applyFullLoad_and_applyInsertAt();
    void test_setData_editRole();
    void test_removeRows_moveRows_sort();
    void test_findNext_and_findAll();
    void test_findNextS_signal_regex_and_plain();
    void test_selectAll_deselectAll_signals();
    void test_findDuplicates_deleteRows_mergeRows_async();
    void test_copyRows_pasteRows_withFileService();
    void test_mapCallSignInView();
    void test_setDbBackup_pointer();
    void test_openFile_persists_rows_roundtrip();
    void test_dbBackup_insertSetDataDelete_roundtrip();
    void test_removeSelectedColumn();

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

    static void syncWait(QFuture<void> future) {
        waitForFuture(future);
        future.waitForFinished();
    }

    static void drainModelUi() {
        for (int i = 0; i < 80; ++i) {
            QCoreApplication::processEvents();
        }
    }

    static int columnOf(AdifModel &m, const std::string &key) {
        auto f = m.snapshotAsync();
        waitForFuture(f);
        const auto &h = f.result().columnHeaders;
        for (size_t i = 0; i < h.size(); ++i) {
            if (h[i] == key) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    static std::string minimalAdifTwoRecords() {
        return std::string("ADIF 3.1.4\n"
                           "<EOH>\n"
                           "<CALL:4>W1AW\n"
                           "<BAND:3>20M\n"
                           "<MODE:3>SSB\n"
                           "<EOR>\n"
                           "<CALL:4>W2BB\n"
                           "<BAND:3>40M\n"
                           "<MODE:2>CW\n"
                           "<EOR>\n");
    }
};

void AdifModelTest::initTestCase() { (void)GLogApplication::getCtyDBInstance()->loadLocalDB(); }

void AdifModelTest::test_defaults_rowColumn_headerData_data_flags() {
    AdifModel m;
    QCOMPARE(m.rowCount(), 0);
    QVERIFY(m.columnCount() > 0);
    QVERIFY(!m.headerData(0, Qt::Horizontal).toString().isEmpty());
    QVERIFY(m.addHeader("notes"));
    drainModelUi();

    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W9ZZ"));
    m.insertRecords(0, std::vector<GRecord>{std::move(r)});
    drainModelUi();

    QCOMPARE(m.rowCount(), 1);
    const int cCall = columnOf(m, "call");
    QVERIFY(cCall >= 0);
    const QModelIndex idx = m.index(0, cCall);
    QVERIFY(!m.data(idx).toString().isEmpty());
    QVERIFY(m.flags(idx).testFlag(Qt::ItemIsEditable));
}

void AdifModelTest::test_addHeader_uniqueness() {
    AdifModel m;
    QVERIFY(!m.addHeader("call"));
    QVERIFY(m.addHeader("my_note"));
    drainModelUi();
    QVERIFY(!m.addHeader("my_note"));
}

void AdifModelTest::test_getDefaultSortModel() {
    auto v = AdifModel::getDefaultSortModel("qso_date");
    QVERIFY(std::find(v.begin(), v.end(), std::string("qso_date")) != v.end());
    QVERIFY(std::find(v.begin(), v.end(), std::string("time_on")) != v.end());
}

void AdifModelTest::test_insertRows_returnsFalse() {
    AdifModel m;
    QVERIFY(!m.insertRows(0, 1, {}));
}

void AdifModelTest::test_applyFullLoad_and_applyInsertAt() {
    AdifModel m;
    std::vector<std::string> err;
    std::istringstream in(minimalAdifTwoRecords());
    auto res = AdifParseService::parse(in, err, AdifParseService::ParseMode::Batch);
    QVERIFY(res.parse_res);
    m.applyFullLoad(std::move(res));
    drainModelUi();
    QCOMPARE(m.rowCount(), 2);

    std::string extra = "<CALL:4>W3CC\n<BAND:3>20M\n<MODE:3>SSB\n<EOR>\n";
    std::istringstream in2(extra);
    std::vector<std::string> err2;
    auto res2 = AdifParseService::parse(in2, err2, AdifParseService::ParseMode::Batch);
    QVERIFY(res2.parse_res);
    m.applyInsertAt(1, std::move(res2));
    drainModelUi();
    QCOMPARE(m.rowCount(), 3);
}

void AdifModelTest::test_setData_editRole() {
    AdifModel m;
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W4AA"));
    m.insertRecords(0, std::vector<GRecord>{std::move(r)});
    drainModelUi();
    const int cCall = columnOf(m, "call");
    const QModelIndex ix = m.index(0, cCall);
    QVERIFY(m.setData(ix, QStringLiteral("W5BB")));
    QCOMPARE(m.data(ix).toString(), QStringLiteral("W5BB"));

    const int cDate = columnOf(m, "qso_date");
    if (cDate >= 0) {
        QVERIFY(!m.setData(m.index(0, cDate), QStringLiteral("20200101")));
    }
}

void AdifModelTest::test_removeRows_moveRows_sort() {
    AdifModel m;
    for (int i = 0; i < 3; ++i) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", std::string("S") + char('0' + i)));
        m.insertRecords(m.rowCount(), std::vector<GRecord>{std::move(r)});
        drainModelUi();
    }
    QCOMPARE(m.rowCount(), 3);
    QVERIFY(m.removeRows(2, 1, {}));
    drainModelUi();
    QCOMPARE(m.rowCount(), 2);

    GRecord r2;
    QVERIFY(r2.addOrSetPair("call", "S9"));
    m.insertRecords(m.rowCount(), std::vector<GRecord>{std::move(r2)});
    drainModelUi();
    QCOMPARE(m.rowCount(), 3);
    QVERIFY(m.moveRows({}, 0, 1, {}, 3));
    drainModelUi();

    const int cCall = columnOf(m, "call");
    m.sort(cCall, Qt::AscendingOrder);
    drainModelUi();
    QVERIFY(m.rowCount() >= 1);
    m.sortSelectedColumn(cCall, Qt::DescendingOrder);
    drainModelUi();
}

void AdifModelTest::test_findNext_and_findAll() {
    AdifModel m;
    for (const char *c : {"ALPHA", "BETA"}) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", c));
        m.insertRecords(m.rowCount(), std::vector<GRecord>{std::move(r)});
        drainModelUi();
    }
    const int cCall = columnOf(m, "call");
    AdifModel::Condition cond = [](const std::string &v) {
        return v.find("ET") != std::string::npos;
    };
    const QModelIndex n = m.findNext({}, cond, "call");
    QVERIFY(n.isValid());
    QCOMPARE(n.row(), 1);

    const QList<int> all = m.findAll(cond, "call");
    QCOMPARE(all.size(), 1);
    QCOMPARE(all[0], 1);

    const QList<int> allAny = m.findAll(cond, "");
    QVERIFY(allAny.size() >= 1);
}

void AdifModelTest::test_findNextS_signal_regex_and_plain() {
    AdifModel m;
    for (const char *c : {"ALPHA", "BETA", "GAMMA"}) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", c));
        m.insertRecords(m.rowCount(), std::vector<GRecord>{std::move(r)});
        drainModelUi();
    }

    QSignalSpy plainSpy(&m, &AdifModel::foundNext);
    m.findNextS(QModelIndex{}, QStringLiteral("call"), QStringLiteral("ET"), false);
    QCOMPARE(plainSpy.size(), 1);
    auto plainIndex = qvariant_cast<QModelIndex>(plainSpy.at(0).at(0));
    QVERIFY(plainIndex.isValid());
    QCOMPARE(plainIndex.row(), 1);

    QSignalSpy regexSpy(&m, &AdifModel::foundNext);
    m.findNextS(QModelIndex{}, QStringLiteral("call"), QStringLiteral("^GAMMA$"), true);
    QCOMPARE(regexSpy.size(), 1);
    auto regexIndex = qvariant_cast<QModelIndex>(regexSpy.at(0).at(0));
    QVERIFY(regexIndex.isValid());
    QCOMPARE(regexIndex.row(), 2);

    QSignalSpy anyKeySpy(&m, &AdifModel::foundNext);
    m.findNextS(QModelIndex{}, QStringLiteral(""), QStringLiteral("BETA"), false);
    QCOMPARE(anyKeySpy.size(), 1);
    auto anyKeyIndex = qvariant_cast<QModelIndex>(anyKeySpy.at(0).at(0));
    QVERIFY(anyKeyIndex.isValid());
    QCOMPARE(anyKeyIndex.row(), 1);

    QSignalSpy missSpy(&m, &AdifModel::foundNext);
    m.findNextS(QModelIndex{}, QStringLiteral("call"), QStringLiteral("__no_match__"), false);
    QCOMPARE(missSpy.size(), 1);
    auto missIndex = qvariant_cast<QModelIndex>(missSpy.at(0).at(0));
    QVERIFY(!missIndex.isValid());
}

void AdifModelTest::test_selectAll_deselectAll_signals() {
    AdifModel m;
    for (const char *c : {"ABLE", "BAKER", "ACE"}) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", c));
        m.insertRecords(m.rowCount(), std::vector<GRecord>{std::move(r)});
        drainModelUi();
    }

    QSignalSpy selPlain(&m, &AdifModel::selectRows);
    m.selectAll(QStringLiteral("call"), QStringLiteral("A"), false);
    QCOMPARE(selPlain.size(), 1);
    const auto rowsPlain = qvariant_cast<QList<int>>(selPlain.at(0).at(0));
    QCOMPARE(rowsPlain.size(), 3);

    QSignalSpy selRegex(&m, &AdifModel::selectRows);
    m.selectAll(QStringLiteral("call"), QStringLiteral("^A"), true);
    QCOMPARE(selRegex.size(), 1);
    const auto rowsRegex = qvariant_cast<QList<int>>(selRegex.at(0).at(0));
    QCOMPARE(rowsRegex.size(), 2);
    QVERIFY(rowsRegex.contains(0));
    QVERIFY(rowsRegex.contains(2));

    QSignalSpy selEmptyKey(&m, &AdifModel::selectRows);
    m.selectAll(QStringLiteral(""), QStringLiteral("BAK"), false);
    QCOMPARE(selEmptyKey.size(), 1);
    const auto rowsEmptyKey = qvariant_cast<QList<int>>(selEmptyKey.at(0).at(0));
    QCOMPARE(rowsEmptyKey.size(), 1);
    QCOMPARE(rowsEmptyKey.first(), 1);

    QSignalSpy desPlain(&m, &AdifModel::deselectRows);
    m.deselectAll(QStringLiteral("call"), QStringLiteral("A"), false);
    QCOMPARE(desPlain.size(), 1);
    const auto desRows = qvariant_cast<QList<int>>(desPlain.at(0).at(0));
    QCOMPARE(desRows.size(), 3);

    QSignalSpy desRegex(&m, &AdifModel::deselectRows);
    m.deselectAll(QStringLiteral("call"), QStringLiteral("^B"), true);
    QCOMPARE(desRegex.size(), 1);
    const auto desRowsRegex = qvariant_cast<QList<int>>(desRegex.at(0).at(0));
    QCOMPARE(desRowsRegex.size(), 1);
    QCOMPARE(desRowsRegex.first(), 1);

    QSignalSpy desEmptyKey(&m, &AdifModel::deselectRows);
    m.deselectAll(QStringLiteral(""), QStringLiteral("CE"), false);
    QCOMPARE(desEmptyKey.size(), 1);
    const auto desRowsEmpty = qvariant_cast<QList<int>>(desEmptyKey.at(0).at(0));
    QCOMPARE(desRowsEmpty.size(), 1);
    QCOMPARE(desRowsEmpty.first(), 2);
}

void AdifModelTest::test_findDuplicates_deleteRows_mergeRows_async() {
    AdifModel m;
    for (int i = 0; i < 2; ++i) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", "MERGE"));
        if (i == 1) {
            QVERIFY(r.addOrSetPair("notes", "only-on-second"));
        }
        m.insertRecords(m.rowCount(), std::vector<GRecord>{std::move(r)});
        drainModelUi();
    }
    auto dups = m.findDuplicates(std::vector<std::string>{"call"});
    QCOMPARE(dups.size(), size_t(1));

    const QModelIndex major = m.index(0, 0);
    QModelIndexList allRows;
    allRows << m.index(0, 0) << m.index(1, 0);
    QVERIFY(m.mergeRows(major, allRows));
    drainModelUi();

    QCOMPARE(m.deleteRowsPersistent({QPersistentModelIndex(m.index(0, 0))}), size_t(1));
    drainModelUi();

    GRecord r;
    QVERIFY(r.addOrSetPair("call", "X1"));
    m.insertRecords(0, std::vector<GRecord>{std::move(r)});
    GRecord r2;
    QVERIFY(r2.addOrSetPair("call", "X2"));
    m.insertRecords(1, std::vector<GRecord>{std::move(r2)});
    drainModelUi();
    QModelIndexList two;
    two << m.index(0, 0) << m.index(1, 0);
    auto fut = m.deleteRowsAsync(std::move(two));
    waitForFuture(fut);
    try {
        fut.waitForFinished();
    } catch (const std::exception &e) {
        QFAIL(e.what());
    }
    QVERIFY(fut.result() >= 1u);
    drainModelUi();
}

void AdifModelTest::test_copyRows_pasteRows_withFileService() {
    AdifModel m;
    AdifFileService svc(&m);
    m.setFileService(&svc);

    std::vector<std::string> err;
    std::istringstream in(minimalAdifTwoRecords());
    auto res = AdifParseService::parse(in, err, AdifParseService::ParseMode::Batch);
    m.applyFullLoad(std::move(res));
    drainModelUi();
    QCOMPARE(m.rowCount(), 2);

    QSignalSpy clip(&m, &AdifModel::setCilpboard);
    const int cCall = columnOf(m, "call");
    QModelIndexList ix;
    ix << m.index(0, 0) << m.index(0, cCall);
    m.copyRows(ix);
    QCOMPARE(clip.size(), 1);

    auto *mime = clip.at(0).at(0).value<QMimeData *>();
    QVERIFY(mime != nullptr);
    m.pasteRows(mime);
    m.getFIFOBackendThreadExecutor()
        ->waitForDoneAndProcessEvents(); // fix: wait until insert data are applied
    drainModelUi();                      // actually update here
    QVERIFY(m.rowCount() >= 3);          // now the paste is applied
}

void AdifModelTest::test_mapCallSignInView() {
    AdifModel m;
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W1AW"));
    m.insertRecords(0, std::vector<GRecord>{std::move(r)});
    drainModelUi();

    QSignalSpy spyEnd(&m, &AdifModel::mapCallSignInViewEnd);
    m.mapCallSignInView(true);
    drainModelUi();
    QVERIFY(spyEnd.size() >= 1);
}

void AdifModelTest::test_setDbBackup_pointer() {
    QTemporaryFile tf;
    QVERIFY(tf.open());
    const QString dbPath = tf.fileName();
    tf.close();
    auto repo = std::make_shared<GRecordRepository>(dbPath);
    syncWait(repo->initSchemaAsync());

    AdifModel m;
    m.setDbBackup(repo);
    QCOMPARE(m.getDbBackup().get(), repo.get());
}

void AdifModelTest::test_openFile_persists_rows_roundtrip() {
    QTemporaryFile adif;
    const std::string adifText = minimalAdifTwoRecords();
    QVERIFY(adif.open());
    adif.write(adifText.data(), static_cast<qint64>(adifText.size()));
    adif.close();

    QTemporaryFile db;
    QVERIFY(db.open());
    const QString dbPath = db.fileName();
    db.close();

    AdifModel model;
    AdifFileService fs(&model);
    model.setFileService(&fs);
    auto repo = std::make_shared<GRecordRepository>(dbPath);
    syncWait(repo->initSchemaAsync());
    model.setDbBackup(repo);

    auto fut = fs.openFileAsync(adif.fileName());
    waitForFuture(fut);
    fut.waitForFinished();
    drainModelUi();

    QCOMPARE(model.rowCount(), 2);
    const int64_t fid = fs.activePersistedFileId();
    QVERIFY(fid > 0);

    auto loadFut = repo->loadFileAsync(fid);
    waitForFuture(loadFut);
    const std::vector<GRecord> loaded = loadFut.result();
    QCOMPARE(static_cast<int>(loaded.size()), model.rowCount());
}

void AdifModelTest::test_dbBackup_insertSetDataDelete_roundtrip() {
    QTemporaryFile adif;
    const std::string adifText = minimalAdifTwoRecords();
    QVERIFY(adif.open());
    adif.write(adifText.data(), static_cast<qint64>(adifText.size()));
    adif.close();

    QTemporaryFile db;
    QVERIFY(db.open());
    const QString dbPath = db.fileName();
    db.close();

    AdifModel model;
    AdifFileService fs(&model);
    model.setFileService(&fs);
    auto repo = std::make_shared<GRecordRepository>(dbPath);
    syncWait(repo->initSchemaAsync());
    model.setDbBackup(repo);

    auto openFut = fs.openFileAsync(adif.fileName());
    waitForFuture(openFut);
    openFut.waitForFinished();
    drainModelUi();

    QCOMPARE(model.rowCount(), 2);
    const int64_t fid = fs.activePersistedFileId();
    QVERIFY(fid > 0);

    // Sanity check: openFile persists the parsed rows so the DB already mirrors the model.
    {
        auto loadFut = repo->loadFileAsync(fid);
        waitForFuture(loadFut);
        QCOMPARE(static_cast<int>(loadFut.result().size()), 2);
    }

    // (a) insertRecords -> upsertRecordAsync per inserted row.
    GRecord newRec;
    QVERIFY(newRec.addOrSetPair("call", "K9NEW"));
    QVERIFY(newRec.addOrSetPair("notes", "inserted"));
    model.insertRecords(model.rowCount(), std::vector<GRecord>{std::move(newRec)});
    drainModelUi();
    QCOMPARE(model.rowCount(), 3);

    {
        auto loadFut = repo->loadFileAsync(fid);
        waitForFuture(loadFut);
        const std::vector<GRecord> loaded = loadFut.result();
        QCOMPARE(static_cast<int>(loaded.size()), 3);
        bool foundNew = false;
        for (const auto &rec : loaded) {
            auto it = rec.find("call");
            if (it != rec.end() && it->second && it->second->get() == "K9NEW") {
                foundNew = true;
                break;
            }
        }
        QVERIFY(foundNew);
    }

    // (b) setData -> updateFieldAsync for an existing dbId.
    const int cCall = columnOf(model, "call");
    QVERIFY(cCall >= 0);
    const QModelIndex editIdx = model.index(0, cCall);
    QVERIFY(model.setData(editIdx, QStringLiteral("W1EDIT")));
    drainModelUi();

    {
        auto loadFut = repo->loadFileAsync(fid);
        waitForFuture(loadFut);
        const std::vector<GRecord> loaded = loadFut.result();
        QCOMPARE(static_cast<int>(loaded.size()), 3);
        bool foundEdited = false;
        for (const auto &rec : loaded) {
            auto it = rec.find("call");
            if (it != rec.end() && it->second && it->second->get() == "W1EDIT") {
                foundEdited = true;
                break;
            }
        }
        QVERIFY(foundEdited);
    }

    // (c) deleteRows -> deleteRecordAsync for each persisted dbId.
    const int rowsBeforeDelete = model.rowCount();
    QModelIndexList toDelete;
    toDelete << model.index(0, 0);
    QCOMPARE(model.deleteRows(toDelete), size_t(1));
    drainModelUi();

    {
        auto loadFut = repo->loadFileAsync(fid);
        waitForFuture(loadFut);
        const std::vector<GRecord> loaded = loadFut.result();
        QCOMPARE(static_cast<int>(loaded.size()), rowsBeforeDelete - 1);
        for (const auto &rec : loaded) {
            auto it = rec.find("call");
            if (it != rec.end() && it->second) {
                QVERIFY(it->second->get() != "W1EDIT");
            }
        }
    }
}

void AdifModelTest::test_removeSelectedColumn() {
    AdifModel m;
    QVERIFY(m.addHeader("notes"));
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W8QQ"));
    QVERIFY(r.addOrSetPair("notes", "hi"));
    m.insertRecords(0, std::vector<GRecord>{std::move(r)});
    drainModelUi();
    const int notesCol = columnOf(m, "notes");
    QVERIFY(notesCol >= 0);
    m.removeSelectedColumn(notesCol);
    drainModelUi();
    QCOMPARE(columnOf(m, "notes"), -1);
}

QTEST_MAIN(AdifModelTest)
#include "tst_adifmodel.moc"