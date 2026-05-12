#include <QtTest>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QString>
#include <QTemporaryFile>
#include <stdexcept>
#include <string>
#include <vector>

#include "Concurrent.h"
#include "record.h"
#include "recordrepository.h"

namespace {

const std::vector<std::string> kEssentialCompareFields = {"mode", "submode", "freq", "band"};

std::vector<std::string> fieldsWithCall() {
    auto f = kEssentialCompareFields;
    f.push_back("call");
    return f;
}

std::vector<std::string> fieldsWithCallAndNotes() {
    auto f = fieldsWithCall();
    f.push_back("notes");
    return f;
}

} // namespace

class GRecordRepositoryTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_scenarioA_twoFilesIsolation();
    void test_scenarioB_sameFileTwoVectorsInsertOrder();
    void test_scenarioC_syncOrderReversesLoadOrder();
    void test_scenarioC_syncOrder_skipsNegativeDbId();
    void test_scenarioD_sharedDbIdAfterUpsert();
    void test_upsertReplacesAllFields();
    void test_deleteRecord_and_noOps();
    void test_updateField_invalidId_throws();
    void test_deleteField_negativeId_noOp();
    void test_createFile_duplicateName_throws();
    void test_findFileIdByFilenameAsync();
    void test_deleteFile_cascadesRecords();

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
        try {
            future.waitForFinished();
        } catch (const std::exception &e) {
            QFAIL((QStringLiteral("syncWait: ") + e.what()).toUtf8().constData());
        }
    }

    template <typename T> static T syncWaitResult(QFuture<T> future) {
        waitForFuture(future);
        try {
            future.waitForFinished();
        } catch (const std::exception &e) {
            // Avoid QFAIL inside template: MSVC (C2561) may not treat QFAIL as terminating.
            throw std::runtime_error(std::string("syncWaitResult: ") + e.what());
        }
        return future.result();
    }

    void initRepoAndSchema();

    std::unique_ptr<GRecordRepository> m_repo;
    QString m_dbPath;
    QTemporaryFile m_tempFile;
};

void GRecordRepositoryTest::initTestCase() {
    QVERIFY(m_tempFile.open());
    m_dbPath = m_tempFile.fileName();
    m_tempFile.close();
    m_repo = std::make_unique<GRecordRepository>(m_dbPath);
    initRepoAndSchema();
}

void GRecordRepositoryTest::initRepoAndSchema() {
    auto schemaFut = m_repo->initSchemaAsync();
    syncWait(schemaFut);
}

void GRecordRepositoryTest::cleanupTestCase() { m_repo.reset(); }

void GRecordRepositoryTest::test_scenarioA_twoFilesIsolation() {
    const int64_t fileA = syncWaitResult(m_repo->createFileAsync("a.adif"));
    const int64_t fileB = syncWaitResult(m_repo->createFileAsync("b.adif"));
    QVERIFY(fileA > 0);
    QVERIFY(fileB > 0);
    QVERIFY(fileA != fileB);

    std::vector<GRecord> va;
    va.emplace_back();
    va.back().addOrSetPair("call", "W1AAA");
    va.emplace_back();
    va.back().addOrSetPair("call", "W2BBB");

    std::vector<GRecord> vb;
    vb.emplace_back();
    vb.back().addOrSetPair("call", "W3CCC");
    vb.back().addOrSetPair("gridsquare", "FN31");

    for (auto &r : va) {
        auto f = m_repo->upsertRecordAsync(fileA, r);
        syncWait(f);
    }
    for (auto &r : vb) {
        auto f = m_repo->upsertRecordAsync(fileB, r);
        syncWait(f);
    }

    auto loadAFut = m_repo->loadFileAsync(fileA);
    const std::vector<GRecord> loadedA = syncWaitResult(loadAFut);
    auto loadBFut = m_repo->loadFileAsync(fileB);
    const std::vector<GRecord> loadedB = syncWaitResult(loadBFut);

    QCOMPARE(static_cast<int>(loadedA.size()), 2);
    QCOMPARE(static_cast<int>(loadedB.size()), 1);

    const auto fields = fieldsWithCall();
    QVERIFY(GRecord::equal_to(loadedA[0], va[0], fields));
    QVERIFY(GRecord::equal_to(loadedA[1], va[1], fields));

    QVERIFY(GRecord::equal_to(loadedB[0], vb[0], fields));
    const std::vector<std::string> withGrid = {"call", "gridsquare"};
    QVERIFY(GRecord::equal_to(loadedB[0], vb[0], withGrid));
}

void GRecordRepositoryTest::test_scenarioB_sameFileTwoVectorsInsertOrder() {
    const int64_t fileId = syncWaitResult(m_repo->createFileAsync("merge.adif"));

    std::vector<GRecord> v1;
    for (int i = 0; i < 3; ++i) {
        v1.emplace_back();
        v1.back().addOrSetPair("call", QStringLiteral("V1_%1").arg(i).toStdString());
    }
    std::vector<GRecord> v2;
    for (int i = 0; i < 2; ++i) {
        v2.emplace_back();
        v2.back().addOrSetPair("call", QStringLiteral("V2_%1").arg(i).toStdString());
    }

    for (auto &r : v1) {
        auto f = m_repo->upsertRecordAsync(fileId, r);
        syncWait(f);
    }
    for (auto &r : v2) {
        auto f = m_repo->upsertRecordAsync(fileId, r);
        syncWait(f);
    }

    auto loadFut = m_repo->loadFileAsync(fileId);
    const std::vector<GRecord> loaded = syncWaitResult(loadFut);
    QCOMPARE(static_cast<int>(loaded.size()), 5);

    const auto fields = fieldsWithCall();
    for (int i = 0; i < 3; ++i) {
        QVERIFY(
            GRecord::equal_to(loaded[static_cast<size_t>(i)], v1[static_cast<size_t>(i)], fields));
    }
    for (int i = 0; i < 2; ++i) {
        QVERIFY(GRecord::equal_to(loaded[static_cast<size_t>(3 + i)], v2[static_cast<size_t>(i)],
                                  fields));
    }
}

void GRecordRepositoryTest::test_scenarioC_syncOrderReversesLoadOrder() {
    const int64_t fileId = syncWaitResult(m_repo->createFileAsync("order.adif"));

    std::vector<GRecord> inserted;
    for (const char *call : {"AA1", "BB2", "CC3"}) {
        inserted.emplace_back();
        inserted.back().addOrSetPair("call", call);
        auto f = m_repo->upsertRecordAsync(fileId, inserted.back());
        syncWait(f);
        QVERIFY(inserted.back().getDbId() > 0);
    }

    std::vector<GRecord> orderVec = inserted;
    std::reverse(orderVec.begin(), orderVec.end());

    auto syncFut = m_repo->syncOrderAsync(orderVec);
    syncWait(syncFut);

    auto loadFut = m_repo->loadFileAsync(fileId);
    const std::vector<GRecord> loaded = syncWaitResult(loadFut);
    QCOMPARE(static_cast<int>(loaded.size()), 3);

    const auto fields = fieldsWithCall();
    QVERIFY(GRecord::equal_to(loaded[0], orderVec[0], fields));
    QVERIFY(GRecord::equal_to(loaded[1], orderVec[1], fields));
    QVERIFY(GRecord::equal_to(loaded[2], orderVec[2], fields));
}

void GRecordRepositoryTest::test_scenarioC_syncOrder_skipsNegativeDbId() {
    const int64_t fileId = syncWaitResult(m_repo->createFileAsync("skip.adif"));

    GRecord a;
    a.addOrSetPair("call", "SKIP_A");
    auto f1 = m_repo->upsertRecordAsync(fileId, a);
    syncWait(f1);
    QVERIFY(a.getDbId() > 0);

    GRecord c;
    c.addOrSetPair("call", "SKIP_C");
    auto f2 = m_repo->upsertRecordAsync(fileId, c);
    syncWait(f2);
    QVERIFY(c.getDbId() > 0);

    GRecord ghost; // still INVALID_DB_ID
    QVERIFY(ghost.getDbId() == GRecord::INVALID_DB_ID);

    std::vector<GRecord> orderVec = {a, ghost, c};
    auto syncFut = m_repo->syncOrderAsync(orderVec);
    syncWait(syncFut);

    auto loadFut = m_repo->loadFileAsync(fileId);
    const std::vector<GRecord> loaded = syncWaitResult(loadFut);
    QCOMPARE(static_cast<int>(loaded.size()), 2);

    const auto fields = fieldsWithCall();
    // internal_order 0 and 2 in DB; ORDER BY internal_order, id => A then C
    QVERIFY(GRecord::equal_to(loaded[0], a, fields));
    QVERIFY(GRecord::equal_to(loaded[1], c, fields));
}

void GRecordRepositoryTest::test_scenarioD_sharedDbIdAfterUpsert() {
    const int64_t fileId = syncWaitResult(m_repo->createFileAsync("shared.adif"));

    GRecord orig;
    orig.addOrSetPair("call", "SHARED1");
    GRecord copy = orig;
    QCOMPARE(orig.getDbId(), copy.getDbId());

    auto f = m_repo->upsertRecordAsync(fileId, copy);
    syncWait(f);

    QCOMPARE(orig.getDbId(), copy.getDbId());
    QVERIFY(orig.getDbId() > 0);
}

void GRecordRepositoryTest::test_upsertReplacesAllFields() {
    const int64_t fileId = syncWaitResult(m_repo->createFileAsync("replace.adif"));

    GRecord r;
    r.addOrSetPair("call", "REP1");
    r.addOrSetPair("notes", "first notes");
    auto f1 = m_repo->upsertRecordAsync(fileId, r);
    syncWait(f1);
    QVERIFY(r.getDbId() > 0);

    GRecord r2;
    r2.addOrSetPair("call", "REP1");
    r2.setDbId(r.getDbId());
    // same db row, no notes field - DAO deletes all fields then inserts only current map keys
    auto f2 = m_repo->upsertRecordAsync(fileId, r2);
    syncWait(f2);

    auto loadFut = m_repo->loadFileAsync(fileId);
    const std::vector<GRecord> loaded = syncWaitResult(loadFut);
    QCOMPARE(static_cast<int>(loaded.size()), 1);

    const auto withNotes = fieldsWithCallAndNotes();
    QVERIFY(!GRecord::equal_to(loaded[0], r, withNotes)); // r still has old notes in memory
    QVERIFY(loaded[0].find("notes") == loaded[0].end());
    const auto fields = fieldsWithCall();
    QVERIFY(GRecord::equal_to(loaded[0], r2, fields));
}

void GRecordRepositoryTest::test_deleteRecord_and_noOps() {
    const int64_t fileId = syncWaitResult(m_repo->createFileAsync("del.adif"));

    GRecord r;
    r.addOrSetPair("call", "DEL1");
    auto up = m_repo->upsertRecordAsync(fileId, r);
    syncWait(up);
    const int64_t rid = r.getDbId();
    QVERIFY(rid > 0);

    auto delOk = m_repo->deleteRecordAsync(rid);
    syncWait(delOk);

    auto load1 = m_repo->loadFileAsync(fileId);
    QCOMPARE(static_cast<int>(syncWaitResult(load1).size()), 0);

    auto delNoop = m_repo->deleteRecordAsync(GRecord::INVALID_DB_ID);
    syncWait(delNoop);
}

void GRecordRepositoryTest::test_updateField_invalidId_throws() {
    const int64_t fileId = syncWaitResult(m_repo->createFileAsync("inv.adif"));
    (void)fileId;

    auto fut = m_repo->updateFieldAsync(GRecord::INVALID_DB_ID, "call", "X");
    waitForFuture(fut);
    bool caught = false;
    try {
        fut.waitForFinished();
    } catch (const std::invalid_argument &) {
        caught = true;
    } catch (const std::exception &e) {
        QFAIL((QStringLiteral("expected invalid_argument, got: ") + e.what()).toUtf8().constData());
    }
    QVERIFY(caught);
}

void GRecordRepositoryTest::test_deleteField_negativeId_noOp() {
    const int64_t fileId = syncWaitResult(m_repo->createFileAsync("dnf.adif"));

    GRecord r;
    r.addOrSetPair("call", "DNF1");
    auto up = m_repo->upsertRecordAsync(fileId, r);
    syncWait(up);

    auto loadBefore = m_repo->loadFileAsync(fileId);
    const std::vector<GRecord> before = syncWaitResult(loadBefore);

    auto delF = m_repo->deleteFieldAsync(GRecord::INVALID_DB_ID, "call");
    syncWait(delF);

    auto loadAfter = m_repo->loadFileAsync(fileId);
    const std::vector<GRecord> after = syncWaitResult(loadAfter);
    QCOMPARE(static_cast<int>(after.size()), static_cast<int>(before.size()));
    const auto fields = fieldsWithCall();
    QVERIFY(GRecord::equal_to(after[0], before[0], fields));
}

void GRecordRepositoryTest::test_findFileIdByFilenameAsync() {
    initRepoAndSchema();
    const int64_t id = syncWaitResult(m_repo->createFileAsync("repo_find_by_name.adif"));
    const auto found = syncWaitResult(m_repo->findFileIdByFilenameAsync("repo_find_by_name.adif"));
    QVERIFY(found.has_value());
    QCOMPARE(*found, id);
    const auto missing = syncWaitResult(m_repo->findFileIdByFilenameAsync("missing_path.adif"));
    QVERIFY(!missing.has_value());
}

void GRecordRepositoryTest::test_createFile_duplicateName_throws() {
    const std::string name = "dup.adif";
    QVERIFY(syncWaitResult(m_repo->createFileAsync(name)) > 0);

    auto fut = m_repo->createFileAsync(name);
    waitForFuture(fut);
    bool caught = false;
    try {
        fut.waitForFinished();
        (void)fut.result();
    } catch (const std::runtime_error &) {
        caught = true;
    } catch (const std::exception &e) {
        QFAIL((QStringLiteral("expected runtime_error, got: ") + e.what()).toUtf8().constData());
    }
    QVERIFY(caught);
}

void GRecordRepositoryTest::test_deleteFile_cascadesRecords() {
    const int64_t fileId = syncWaitResult(m_repo->createFileAsync("cascade.adif"));

    GRecord r;
    r.addOrSetPair("call", "CAS1");
    auto up = m_repo->upsertRecordAsync(fileId, r);
    syncWait(up);

    auto delFile = m_repo->deleteFileAsync(fileId);
    syncWait(delFile);

    auto loadFut = m_repo->loadFileAsync(fileId);
    const std::vector<GRecord> loaded = syncWaitResult(loadFut);
    QCOMPARE(static_cast<int>(loaded.size()), 0);
}

QTEST_MAIN(GRecordRepositoryTest)
#include "tst_recordrepository.moc"
