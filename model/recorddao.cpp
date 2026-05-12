#include "recorddao.h"
#include "SqliteDbExecutor.h"
#include "record.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
inline auto qstr(const std::string &s) -> QString { return QString::fromStdString(s); }

inline auto currentUnixTimestamp() -> qlonglong {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

[[noreturn]] inline void throwQueryFailed(const QSqlQuery &q, std::string_view context) {
    const std::string err = q.lastError().text().toStdString();
    qWarning() << "GRecordDao:" << context << ":" << q.lastError().text();
    std::string msg;
    msg.reserve(context.size() + 2 + err.size());
    msg.append(context);
    msg.append(": ");
    msg.append(err);
    throw std::runtime_error(msg);
}

inline void throwUnlessCommitted(SqlTransaction &trans, std::string_view context) {
    if (!trans.commit()) {
        qWarning() << "GRecordDao: commit failed:" << context;
        std::string msg = "commit failed: ";
        msg.append(context);
        throw std::runtime_error(msg);
    }
}
} // namespace

// ==================== GRecordDao ====================
void GRecordDao::initSchema(SqliteDbExecutor &exec) {
    auto &db = exec.database();
    QSqlQuery query(db);

    if (!query.exec("CREATE TABLE IF NOT EXISTS files ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "filename TEXT NOT NULL UNIQUE,"
                    "last_sync INTEGER NOT NULL)")) {
        throwQueryFailed(query, "initSchema: create files table");
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS records ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "file_id INTEGER NOT NULL,"
                    "internal_order INTEGER NOT NULL,"
                    "FOREIGN KEY(file_id) REFERENCES files(id) ON DELETE CASCADE)")) {
        throwQueryFailed(query, "initSchema: create records table");
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_records_file_order ON records(file_id, "
                    "internal_order)")) {
        throwQueryFailed(query, "initSchema: create idx_records_file_order");
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS record_fields ("
                    "record_id INTEGER NOT NULL,"
                    "field_key TEXT NOT NULL,"
                    "field_value TEXT,"
                    "PRIMARY KEY(record_id, field_key),"
                    "FOREIGN KEY(record_id) REFERENCES records(id) ON DELETE CASCADE)")) {
        throwQueryFailed(query, "initSchema: create record_fields table");
    }

    if (!query.exec(
            "CREATE INDEX IF NOT EXISTS idx_record_fields_key ON record_fields(field_key)")) {
        throwQueryFailed(query, "initSchema: create idx_record_fields_key");
    }
}

auto GRecordDao::createFile(SqliteDbExecutor &exec, const std::string &filename) -> int64_t {
    auto &db = exec.database();
    auto trans = exec.createTransaction();
    QSqlQuery query(db);
    if (!query.prepare("INSERT INTO files (filename, last_sync) VALUES (:filename, :last_sync)")) {
        throwQueryFailed(query, "createFile: prepare");
    }
    query.bindValue(":filename", qstr(filename));
    query.bindValue(":last_sync", currentUnixTimestamp());

    if (!query.exec()) {
        throwQueryFailed(query, "createFile: insert");
    }

    int64_t fileId = query.lastInsertId().toLongLong();
    throwUnlessCommitted(trans, "createFile");
    return fileId;
}

auto GRecordDao::findFileIdByFilename(SqliteDbExecutor &exec, const std::string &canonicalPath)
    -> std::optional<int64_t> {
    auto &db = exec.database();
    QSqlQuery query(db);
    if (!query.prepare("SELECT id FROM files WHERE filename = :filename LIMIT 1")) {
        throwQueryFailed(query, "findFileIdByFilename: prepare");
    }
    query.bindValue(":filename", qstr(canonicalPath));
    if (!query.exec()) {
        throwQueryFailed(query, "findFileIdByFilename: exec");
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return query.value(0).toLongLong();
}

void GRecordDao::deleteFile(SqliteDbExecutor &exec, int64_t fileId) {
    auto &db = exec.database();
    auto trans = exec.createTransaction();
    QSqlQuery query(db);
    if (!query.prepare("DELETE FROM files WHERE id = :id")) {
        throwQueryFailed(query, "deleteFile: prepare");
    }
    query.bindValue(":id", static_cast<qlonglong>(fileId));

    if (!query.exec()) {
        throwQueryFailed(query, "deleteFile: exec");
    }

    throwUnlessCommitted(trans, "deleteFile");
}

void GRecordDao::updateFileSyncTime(SqliteDbExecutor &exec, int64_t fileId) {
    auto &db = exec.database();
    QSqlQuery query(db);
    if (!query.prepare("UPDATE files SET last_sync = :last_sync WHERE id = :id")) {
        throwQueryFailed(query, "updateFileSyncTime: prepare");
    }
    query.bindValue(":last_sync", currentUnixTimestamp());
    query.bindValue(":id", static_cast<qlonglong>(fileId));

    if (!query.exec()) {
        throwQueryFailed(query, "updateFileSyncTime: exec");
    }
}

auto GRecordDao::loadFile(SqliteDbExecutor &exec, int64_t fileId) -> std::vector<GRecord> {
    auto &db = exec.database();
    std::vector<GRecord> result;

    QSqlQuery query(db);
    if (!query.prepare(R"(
        SELECT 
            r.id AS record_id,
            rf.field_key,
            rf.field_value
        FROM records r
        LEFT JOIN record_fields rf ON r.id = rf.record_id
        WHERE r.file_id = :fileId
        ORDER BY r.internal_order, r.id
    )")) {
        throwQueryFailed(query, "loadFile: prepare");
    }
    query.bindValue(":fileId", static_cast<qlonglong>(fileId));

    if (!query.exec()) {
        throwQueryFailed(query, "loadFile: exec");
    }

    int64_t currentRecordId = -1;
    GRecord currentRecord;

    while (query.next()) {
        int64_t rid = query.value(0).toLongLong();
        QString key = query.value(1).toString();
        QString value = query.value(2).toString();

        if (rid != currentRecordId) {
            if (currentRecordId != -1) {
                result.push_back(std::move(currentRecord));
                currentRecord = GRecord();
            }

            currentRecordId = rid;
            currentRecord.setDbId(rid);
        }

        if (!key.isNull()) {
            currentRecord.addOrSetPair(key.toStdString(), value.toStdString());
        }
    }

    if (currentRecordId != -1) {
        result.push_back(std::move(currentRecord));
    }

    return result;
}

void GRecordDao::upsertRecord(SqliteDbExecutor &exec, int64_t fileId, GRecord &record) {
    auto &db = exec.database();
    auto trans = exec.createTransaction();
    bool isNew = (record.getDbId() == -1);
    int64_t recordId = record.getDbId();
    bool opt = false;

    if (isNew) {
        QSqlQuery insertRecord(db);
        opt = insertRecord.prepare("INSERT INTO records (file_id, internal_order) "
                                   "VALUES (:fileId, (SELECT IFNULL(MAX(internal_order), -1) + 1 "
                                   "FROM records WHERE file_id = :fileId))");
        if (!opt) {
            trans.setRollbackOnly();
            throwQueryFailed(insertRecord, "upsertRecord: prepare insert record");
        }
        insertRecord.bindValue(":fileId", static_cast<qlonglong>(fileId));

        if (!insertRecord.exec()) {
            trans.setRollbackOnly();
            throwQueryFailed(insertRecord, "upsertRecord: insert record");
        }
        recordId = insertRecord.lastInsertId().toLongLong();
    } else {
        QSqlQuery deleteFields(db);
        opt = deleteFields.prepare("DELETE FROM record_fields WHERE record_id = :recordId");
        if (!opt) {
            trans.setRollbackOnly();
            throwQueryFailed(deleteFields, "upsertRecord: prepare delete fields");
        }
        deleteFields.bindValue(":recordId", static_cast<qlonglong>(recordId));
        if (!deleteFields.exec()) {
            trans.setRollbackOnly();
            throwQueryFailed(deleteFields, "upsertRecord: delete fields");
        }
    }

    QSqlQuery insertField(db);
    opt = insertField.prepare("INSERT INTO record_fields (record_id, field_key, field_value) "
                              "VALUES (:recordId, :key, :value)");
    if (!opt) {
        trans.setRollbackOnly();
        throwQueryFailed(insertField, "upsertRecord: prepare insert field");
    }

    for (const auto &[key, valueWrap] : record) {
        insertField.bindValue(":recordId", static_cast<qlonglong>(recordId));
        insertField.bindValue(":key", qstr(key));
        std::string valueStr = static_cast<std::string>(valueWrap);
        insertField.bindValue(":value", qstr(valueStr));

        if (!insertField.exec()) {
            trans.setRollbackOnly();
            throwQueryFailed(insertField, "upsertRecord: insert field");
        }
    }

    throwUnlessCommitted(trans, "upsertRecord");
    if (isNew) {
        record.setDbId(recordId);
    }
}

void GRecordDao::deleteRecord(SqliteDbExecutor &exec, int64_t recordDbId) {
    auto &db = exec.database();
    if (recordDbId == -1) {
        return;
    }

    auto trans = exec.createTransaction();
    QSqlQuery query(db);
    if (!query.prepare("DELETE FROM records WHERE id = :id")) {
        trans.setRollbackOnly();
        throwQueryFailed(query, "deleteRecord: prepare");
    }
    query.bindValue(":id", static_cast<qlonglong>(recordDbId));

    if (!query.exec()) {
        trans.setRollbackOnly();
        throwQueryFailed(query, "deleteRecord: exec");
    }

    throwUnlessCommitted(trans, "deleteRecord");
}

void GRecordDao::updateField(SqliteDbExecutor &exec, int64_t recordDbId, const std::string &key,
                             const std::string &value) {
    auto &db = exec.database();
    if (recordDbId == -1) {
        throw std::invalid_argument("GRecordDao::updateField: invalid recordDbId (-1)");
    }

    auto trans = exec.createTransaction();
    QSqlQuery query(db);
    if (!query.prepare("INSERT OR REPLACE INTO record_fields (record_id, field_key, field_value) "
                       "VALUES (:recordId, :key, :value)")) {
        trans.setRollbackOnly();
        throwQueryFailed(query, "updateField: prepare");
    }
    query.bindValue(":recordId", static_cast<qlonglong>(recordDbId));
    query.bindValue(":key", qstr(key));
    query.bindValue(":value", qstr(value));

    if (!query.exec()) {
        trans.setRollbackOnly();
        throwQueryFailed(query, "updateField: exec");
    }

    throwUnlessCommitted(trans, "updateField");
}

void GRecordDao::deleteField(SqliteDbExecutor &exec, int64_t recordDbId, const std::string &key) {
    auto &db = exec.database();
    if (recordDbId == -1) {
        return;
    }

    auto trans = exec.createTransaction();
    QSqlQuery query(db);
    if (!query.prepare(
            "DELETE FROM record_fields WHERE record_id = :recordId AND field_key = :key")) {
        trans.setRollbackOnly();
        throwQueryFailed(query, "deleteField: prepare");
    }
    query.bindValue(":recordId", static_cast<qlonglong>(recordDbId));
    query.bindValue(":key", qstr(key));

    if (!query.exec()) {
        trans.setRollbackOnly();
        throwQueryFailed(query, "deleteField: exec");
    }

    throwUnlessCommitted(trans, "deleteField");
}

void GRecordDao::syncOrder(SqliteDbExecutor &exec, const std::vector<GRecord> &records) {
    auto &db = exec.database();
    if (records.empty()) {
        return;
    }

    auto trans = exec.createTransaction();
    QSqlQuery query(db);
    if (!query.prepare("UPDATE records SET internal_order = :idx WHERE id = :id")) {
        trans.setRollbackOnly();
        throwQueryFailed(query, "syncOrder: prepare");
    }

    for (size_t i = 0; i < records.size(); ++i) {
        int64_t dbId = records[i].getDbId();
        if (dbId == -1) {
            continue;
        }

        query.bindValue(":idx", static_cast<qlonglong>(i));
        query.bindValue(":id", static_cast<qlonglong>(dbId));

        if (!query.exec()) {
            trans.setRollbackOnly();
            throwQueryFailed(query, "syncOrder: update internal_order");
        }
    }

    throwUnlessCommitted(trans, "syncOrder");
}
