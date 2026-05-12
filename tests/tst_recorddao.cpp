#include <QtTest>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QVariant>
#include <stdexcept>
#include <string>

#include "Concurrent.h"
#include "SqliteDbExecutor.h"
#include "record.h"
#include "recorddao.h"

class GRecordDaoTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_initSchema_idempotent();
    void test_upsert_insert_then_update();
    void test_updateField_invalidId_throws();
    void test_deleteField_negativeId_noOp();
    void test_deleteRecord_cascade_and_noOp();
    void test_loadFile_order_matches_insert();
    void test_deleteFile_removesRowFromFilesTable();
    void test_findFileIdByFilename_exists_and_missing();
    void test_updateFileSyncTime_updatesLastSync();
    void test_updateField_validId_persistsValue();
    void test_deleteField_validIdAndKey_removesOnlyThatField();
    void test_syncOrder_skipsRecordsWithInvalidDbId();

  private:
    template <typename T> void waitForFuture(const QFuture<T> &future) {
        if (future.isFinished()) {
            return;
        }
        QEventLoop loop;
        QFutureWatcher<T> watcher;
        watcher.setFuture(future);
        QObject::connect(&watcher, &QFutureWatcher<T>::finished, &loop, &QEventLoop::quit);
        loop.exec();
    }

    template <typename Func> void runOnDb(Func &&fn) {
        auto f = GLogConcurrent::makeFuture(std::forward<Func>(fn), *m_exec);
        waitForFuture(f);
        try {
            f.waitForFinished();
        } catch (const std::exception &e) {
            QFAIL(e.what());
        }
    }

    std::unique_ptr<SqliteDbExecutor> m_exec;
    QString m_dbPath;
    QTemporaryFile m_tempFile;
};

void GRecordDaoTest::initTestCase() {
    QVERIFY(m_tempFile.open());
    m_dbPath = m_tempFile.fileName();
    m_tempFile.close();
    m_exec = std::make_unique<SqliteDbExecutor>(m_dbPath);
}

void GRecordDaoTest::cleanupTestCase() { m_exec.reset(); }

void GRecordDaoTest::test_initSchema_idempotent() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        GRecordDao::initSchema(*m_exec);
    });
}

void GRecordDaoTest::test_upsert_insert_then_update() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t fileId = GRecordDao::createFile(*m_exec, "t1.adif");
        if (fileId <= 0) {
            throw std::runtime_error("createFile failed");
        }

        GRecord r;
        if (!r.addOrSetPair("call", "DAO1") || !r.addOrSetPair("notes", "first")) {
            throw std::runtime_error("addOrSetPair failed");
        }
        GRecordDao::upsertRecord(*m_exec, fileId, r);
        if (r.getDbId() <= 0) {
            throw std::runtime_error("expected db id");
        }

        GRecord r2;
        if (!r2.addOrSetPair("call", "DAO1")) {
            throw std::runtime_error("r2 call");
        }
        r2.setDbId(r.getDbId());
        GRecordDao::upsertRecord(*m_exec, fileId, r2);

        auto loaded = GRecordDao::loadFile(*m_exec, fileId);
        if (loaded.size() != 1) {
            throw std::runtime_error("load size");
        }
        if (loaded[0].find("notes") != loaded[0].end()) {
            throw std::runtime_error("notes should be cleared");
        }
        if (!GRecord::equal_to(loaded[0], r2, std::vector<std::string>{"call"})) {
            throw std::runtime_error("call mismatch");
        }
    });
}

void GRecordDaoTest::test_updateField_invalidId_throws() {
    auto f = GLogConcurrent::makeFuture(
        [this]() {
            GRecordDao::initSchema(*m_exec);
            GRecordDao::updateField(*m_exec, GRecord::INVALID_DB_ID, "call", "X");
        },
        *m_exec);
    waitForFuture(f);
    bool caught = false;
    try {
        f.waitForFinished();
    } catch (const std::invalid_argument &) {
        caught = true;
    }
    QVERIFY(caught);
}

void GRecordDaoTest::test_deleteField_negativeId_noOp() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t fileId = GRecordDao::createFile(*m_exec, "t2.adif");
        GRecord r;
        if (!r.addOrSetPair("call", "DAO2")) {
            throw std::runtime_error("pair");
        }
        GRecordDao::upsertRecord(*m_exec, fileId, r);
        GRecordDao::deleteField(*m_exec, GRecord::INVALID_DB_ID, "call");
        auto loaded = GRecordDao::loadFile(*m_exec, fileId);
        if (loaded.size() != 1 || loaded[0].find("call") == loaded[0].end()) {
            throw std::runtime_error("deleteField noop broke row");
        }
    });
}

void GRecordDaoTest::test_deleteRecord_cascade_and_noOp() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t fileId = GRecordDao::createFile(*m_exec, "t3.adif");
        GRecord r;
        if (!r.addOrSetPair("call", "DAO3")) {
            throw std::runtime_error("pair");
        }
        GRecordDao::upsertRecord(*m_exec, fileId, r);
        const int64_t rid = r.getDbId();
        GRecordDao::deleteRecord(*m_exec, rid);
        if (!GRecordDao::loadFile(*m_exec, fileId).empty()) {
            throw std::runtime_error("expected empty");
        }
        GRecordDao::deleteRecord(*m_exec, GRecord::INVALID_DB_ID);
    });
}

void GRecordDaoTest::test_loadFile_order_matches_insert() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t fileId = GRecordDao::createFile(*m_exec, "order.adif");
        for (int i = 0; i < 3; ++i) {
            GRecord r;
            if (!r.addOrSetPair("call", std::string("Z") + char('0' + i))) {
                throw std::runtime_error("insert");
            }
            GRecordDao::upsertRecord(*m_exec, fileId, r);
        }
        auto loaded = GRecordDao::loadFile(*m_exec, fileId);
        if (loaded.size() != 3) {
            throw std::runtime_error("size");
        }
        if (static_cast<std::string>(loaded[0].at("call")) != "Z0" ||
            static_cast<std::string>(loaded[1].at("call")) != "Z1" ||
            static_cast<std::string>(loaded[2].at("call")) != "Z2") {
            throw std::runtime_error("order");
        }
    });
}

void GRecordDaoTest::test_findFileIdByFilename_exists_and_missing() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t id = GRecordDao::createFile(*m_exec, "/tmp/glog_persist_test.adi");
        const auto found = GRecordDao::findFileIdByFilename(*m_exec, "/tmp/glog_persist_test.adi");
        QVERIFY(found.has_value());
        QCOMPARE(*found, id);
        const auto missing = GRecordDao::findFileIdByFilename(*m_exec, "/no/such/file.adi");
        QVERIFY(!missing.has_value());
    });
}

void GRecordDaoTest::test_deleteFile_removesRowFromFilesTable() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t fileId = GRecordDao::createFile(*m_exec, "delete_me.adif");
        if (fileId <= 0) {
            throw std::runtime_error("createFile failed");
        }

        QSqlQuery before(m_exec->database());
        if (!before.prepare("SELECT COUNT(*) FROM files WHERE id = :id")) {
            throw std::runtime_error("prepare count before");
        }
        before.bindValue(":id", static_cast<qlonglong>(fileId));
        if (!before.exec() || !before.next() || before.value(0).toInt() != 1) {
            throw std::runtime_error("file row missing before delete");
        }

        GRecordDao::deleteFile(*m_exec, fileId);

        QSqlQuery after(m_exec->database());
        if (!after.prepare("SELECT COUNT(*) FROM files WHERE id = :id")) {
            throw std::runtime_error("prepare count after");
        }
        after.bindValue(":id", static_cast<qlonglong>(fileId));
        if (!after.exec() || !after.next() || after.value(0).toInt() != 0) {
            throw std::runtime_error("file row still present after delete");
        }

        GRecordDao::deleteFile(*m_exec, fileId);
    });
}

void GRecordDaoTest::test_updateFileSyncTime_updatesLastSync() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t fileId = GRecordDao::createFile(*m_exec, "sync_time.adif");
        if (fileId <= 0) {
            throw std::runtime_error("createFile failed");
        }

        QSqlQuery readBefore(m_exec->database());
        if (!readBefore.prepare("SELECT last_sync FROM files WHERE id = :id")) {
            throw std::runtime_error("prepare select before");
        }
        readBefore.bindValue(":id", static_cast<qlonglong>(fileId));
        if (!readBefore.exec() || !readBefore.next()) {
            throw std::runtime_error("select before failed");
        }
        const qlonglong before = readBefore.value(0).toLongLong();

        QSqlQuery rewind(m_exec->database());
        if (!rewind.prepare("UPDATE files SET last_sync = :ls WHERE id = :id")) {
            throw std::runtime_error("prepare rewind");
        }
        rewind.bindValue(":ls", static_cast<qlonglong>(before - 100));
        rewind.bindValue(":id", static_cast<qlonglong>(fileId));
        if (!rewind.exec()) {
            throw std::runtime_error("rewind failed");
        }

        GRecordDao::updateFileSyncTime(*m_exec, fileId);

        QSqlQuery readAfter(m_exec->database());
        if (!readAfter.prepare("SELECT last_sync FROM files WHERE id = :id")) {
            throw std::runtime_error("prepare select after");
        }
        readAfter.bindValue(":id", static_cast<qlonglong>(fileId));
        if (!readAfter.exec() || !readAfter.next()) {
            throw std::runtime_error("select after failed");
        }
        const qlonglong after = readAfter.value(0).toLongLong();
        if (after <= before - 100) {
            throw std::runtime_error("last_sync did not advance");
        }
    });
}

void GRecordDaoTest::test_updateField_validId_persistsValue() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t fileId = GRecordDao::createFile(*m_exec, "update_field.adif");
        GRecord r;
        if (!r.addOrSetPair("call", "UPF1") || !r.addOrSetPair("notes", "initial")) {
            throw std::runtime_error("seed pair");
        }
        GRecordDao::upsertRecord(*m_exec, fileId, r);
        if (r.getDbId() <= 0) {
            throw std::runtime_error("expected db id");
        }

        GRecordDao::updateField(*m_exec, r.getDbId(), "notes", "updated");

        auto loaded = GRecordDao::loadFile(*m_exec, fileId);
        if (loaded.size() != 1) {
            throw std::runtime_error("expected single row");
        }
        auto it = loaded[0].find("notes");
        if (it == loaded[0].end()) {
            throw std::runtime_error("notes missing after update");
        }
        if (static_cast<std::string>(it->second) != "updated") {
            throw std::runtime_error("notes not updated");
        }

        GRecordDao::updateField(*m_exec, r.getDbId(), "qslmsg", "added");
        auto loaded2 = GRecordDao::loadFile(*m_exec, fileId);
        if (loaded2.size() != 1) {
            throw std::runtime_error("size after insert field");
        }
        auto qit = loaded2[0].find("qslmsg");
        if (qit == loaded2[0].end() || static_cast<std::string>(qit->second) != "added") {
            throw std::runtime_error("qslmsg insert via updateField failed");
        }
    });
}

void GRecordDaoTest::test_deleteField_validIdAndKey_removesOnlyThatField() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t fileId = GRecordDao::createFile(*m_exec, "delete_field.adif");
        GRecord r;
        if (!r.addOrSetPair("call", "DLF1") || !r.addOrSetPair("notes", "keepme")) {
            throw std::runtime_error("seed pair");
        }
        GRecordDao::upsertRecord(*m_exec, fileId, r);
        if (r.getDbId() <= 0) {
            throw std::runtime_error("expected db id");
        }

        GRecordDao::deleteField(*m_exec, r.getDbId(), "notes");

        auto loaded = GRecordDao::loadFile(*m_exec, fileId);
        if (loaded.size() != 1) {
            throw std::runtime_error("size after deleteField");
        }
        if (loaded[0].find("notes") != loaded[0].end()) {
            throw std::runtime_error("notes should be deleted");
        }
        if (loaded[0].find("call") == loaded[0].end()) {
            throw std::runtime_error("call should remain");
        }

        GRecordDao::deleteField(*m_exec, r.getDbId(), "no_such_field");
        auto loaded2 = GRecordDao::loadFile(*m_exec, fileId);
        if (loaded2.size() != 1) {
            throw std::runtime_error("size after no-op deleteField");
        }
    });
}

void GRecordDaoTest::test_syncOrder_skipsRecordsWithInvalidDbId() {
    runOnDb([this]() {
        GRecordDao::initSchema(*m_exec);
        const int64_t fileId = GRecordDao::createFile(*m_exec, "sync_order.adif");

        std::vector<GRecord> persisted;
        for (int i = 0; i < 3; ++i) {
            GRecord r;
            if (!r.addOrSetPair("call", std::string("SO") + char('0' + i))) {
                throw std::runtime_error("seed pair");
            }
            GRecordDao::upsertRecord(*m_exec, fileId, r);
            persisted.push_back(std::move(r));
        }

        std::vector<GRecord> reordered;
        reordered.push_back(persisted[2]);
        reordered.push_back(persisted[0]);
        GRecord skipMe;
        if (!skipMe.addOrSetPair("call", "SKIP_ME")) {
            throw std::runtime_error("skip seed");
        }
        if (skipMe.getDbId() != GRecord::INVALID_DB_ID) {
            throw std::runtime_error("expected skipMe to be unpersisted");
        }
        reordered.push_back(std::move(skipMe));
        reordered.push_back(persisted[1]);

        GRecordDao::syncOrder(*m_exec, reordered);

        QSqlQuery q(m_exec->database());
        if (!q.prepare("SELECT id, internal_order FROM records WHERE file_id = :fid "
                       "ORDER BY internal_order ASC")) {
            throw std::runtime_error("prepare select order");
        }
        q.bindValue(":fid", static_cast<qlonglong>(fileId));
        if (!q.exec()) {
            throw std::runtime_error("select order failed");
        }
        std::vector<int64_t> seenIds;
        std::vector<qlonglong> seenOrders;
        while (q.next()) {
            seenIds.push_back(q.value(0).toLongLong());
            seenOrders.push_back(q.value(1).toLongLong());
        }
        if (seenIds.size() != 3) {
            throw std::runtime_error("unexpected number of persisted rows");
        }

        // reordered[0] -> idx 0 -> persisted[2]
        // reordered[1] -> idx 1 -> persisted[0]
        // reordered[2] (skip dbId==-1) -> not applied
        // reordered[3] -> idx 3 -> persisted[1]
        if (seenOrders[0] != 0 || seenIds[0] != persisted[2].getDbId()) {
            throw std::runtime_error("order 0 mismatch");
        }
        if (seenOrders[1] != 1 || seenIds[1] != persisted[0].getDbId()) {
            throw std::runtime_error("order 1 mismatch");
        }
        if (seenOrders[2] != 3 || seenIds[2] != persisted[1].getDbId()) {
            throw std::runtime_error("order 3 mismatch (skip slot 2)");
        }

        GRecordDao::syncOrder(*m_exec, std::vector<GRecord>{});
    });
}

QTEST_MAIN(GRecordDaoTest)
#include "tst_recorddao.moc"
