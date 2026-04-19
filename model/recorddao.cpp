#include "recorddao.h"
#include "SqliteDbExecutor.h"
#include "record.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <atomic>
#include <chrono>
#include <string>

namespace {
// 将 std::string 转为 QString
inline QString qstr(const std::string &s) { return QString::fromStdString(s); }

// 获取当前 Unix 时间戳（秒）
inline qlonglong currentUnixTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}
} // namespace

// ==================== GRecordDao 静态方法实现 ====================
bool GRecordDao::initSchema(SqliteDbExecutor &exec) {
    auto &db = exec.database();
    QSqlQuery query(db);

    // files 表
    if (!query.exec("CREATE TABLE IF NOT EXISTS files ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "filename TEXT NOT NULL UNIQUE,"
                    "last_sync INTEGER NOT NULL)")) {
        qWarning() << "GRecordDao: Failed to create files table:" << query.lastError().text();
        return false;
    }

    // records 表
    if (!query.exec("CREATE TABLE IF NOT EXISTS records ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "file_id INTEGER NOT NULL,"
                    "internal_order INTEGER NOT NULL,"
                    "FOREIGN KEY(file_id) REFERENCES files(id) ON DELETE CASCADE)")) {
        qWarning() << "GRecordDao: Failed to create records table:" << query.lastError().text();
        return false;
    }

    // 索引：加速按文件 ID 和顺序查询
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_records_file_order ON records(file_id, "
                    "internal_order)")) {
        qWarning() << "GRecordDao: Failed to create index on records:" << query.lastError().text();
        return false;
    }

    // record_fields 表（EAV）
    if (!query.exec("CREATE TABLE IF NOT EXISTS record_fields ("
                    "record_id INTEGER NOT NULL,"
                    "field_key TEXT NOT NULL,"
                    "field_value TEXT,"
                    "PRIMARY KEY(record_id, field_key),"
                    "FOREIGN KEY(record_id) REFERENCES records(id) ON DELETE CASCADE)")) {
        qWarning() << "GRecordDao: Failed to create record_fields table:"
                   << query.lastError().text();
        return false;
    }

    // 索引：加速按字段键查询
    if (!query.exec(
            "CREATE INDEX IF NOT EXISTS idx_record_fields_key ON record_fields(field_key)")) {
        qWarning() << "GRecordDao: Failed to create index on record_fields:"
                   << query.lastError().text();
        return false;
    }

    return true;
}

int64_t GRecordDao::createFile(SqliteDbExecutor &exec, const std::string &filename) {
    auto &db = exec.database();
    auto trans = exec.createTransaction();
    QSqlQuery query(db);
    query.prepare("INSERT INTO files (filename, last_sync) VALUES (:filename, :last_sync)");
    query.bindValue(":filename", qstr(filename));
    query.bindValue(":last_sync", currentUnixTimestamp());

    if (!query.exec()) {
        qWarning() << "GRecordDao: Failed to insert file:" << query.lastError().text();
        return -1;
    }

    int64_t fileId = query.lastInsertId().toLongLong();
    trans.commit();
    return fileId;
}

void GRecordDao::deleteFile(SqliteDbExecutor &exec, int64_t fileId) {
    auto &db = exec.database();
    auto trans = exec.createTransaction();
    QSqlQuery query(db);
    query.prepare("DELETE FROM files WHERE id = :id");
    query.bindValue(":id", static_cast<qlonglong>(fileId));

    if (!query.exec()) {
        qWarning() << "GRecordDao: Failed to delete file:" << query.lastError().text();
    } else {
        trans.commit();
    }
    // ON DELETE CASCADE 会自动删除关联的 records 和 record_fields
}

void GRecordDao::updateFileSyncTime(SqliteDbExecutor &exec, int64_t fileId) {
    auto &db = exec.database();
    QSqlQuery query(db);
    query.prepare("UPDATE files SET last_sync = :last_sync WHERE id = :id");
    query.bindValue(":last_sync", currentUnixTimestamp());
    query.bindValue(":id", static_cast<qlonglong>(fileId));

    if (!query.exec()) {
        qWarning() << "GRecordDao: Failed to update file sync time:" << query.lastError().text();
    }
}

// v2: 无事务单 SELECT 语句版
std::vector<GRecord> GRecordDao::loadFile(SqliteDbExecutor &exec, int64_t fileId) {
    auto &db = exec.database();
    std::vector<GRecord> result;

    // 1. 一次性查询：JOIN 两表，并按 internal_order 和 record_id 排序
    //    排序至关重要：只有有序了，才能在遍历时判断"是否进入了下一个记录"
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT 
            r.id AS record_id,
            rf.field_key,
            rf.field_value
        FROM records r
        LEFT JOIN record_fields rf ON r.id = rf.record_id
        WHERE r.file_id = :fileId
        ORDER BY r.internal_order, r.id
    )");
    query.bindValue(":fileId", static_cast<qlonglong>(fileId));

    if (!query.exec() || !query.isActive()) {
        qWarning() << "GRecordDao: Failed to execute JOIN query:" << query.lastError().text();
        return result;
    }

    // 2. 内存中分组合并（状态机模式）
    int64_t currentRecordId = -1;
    GRecord currentRecord; // 当前正在构建的记录

    // 预分配内存以减少 vector 扩容开销 (可选优化)
    // result.reserve(estimatedCount);

    while (query.next()) {
        int64_t rid = query.value(0).toLongLong();
        QString key = query.value(1).toString();
        QString value = query.value(2).toString();

        // 检测是否切换到了下一条 Record
        if (rid != currentRecordId) {
            // 如果不是第一条记录，将上一条记录存入结果集
            if (currentRecordId != -1) {
                result.push_back(std::move(currentRecord));
                // currentRecord 被移动后变为空壳，需要显式重新初始化
                // 被移动的对象需要满足状态与GRecord()初始化时相同(重建保留字段)
                currentRecord = GRecord();
            }

            // 开始新记录
            currentRecordId = rid;
            currentRecord.setDbId(rid);
            // 清理可能残留的数据（移动后自动清理，但为了安全可以显式清理，GRecord需有clear方法）
            // 假设 GRecord 有 clear() 或直接使用新对象，这里用 std::move 后重建最简单
        }

        // 填充字段（跳过 key 为 NULL 的情况，虽然 LEFT JOIN 通常不会产生 NULL key）
        if (!key.isNull()) {
            currentRecord.addOrSetPair(key.toStdString(), value.toStdString());
        }
    }

    // 3. 处理最后一条记录
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
        // 自动计算 internal_order 为当前文件内最大序号 +1
        opt = insertRecord.prepare("INSERT INTO records (file_id, internal_order) "
                                   "VALUES (:fileId, (SELECT IFNULL(MAX(internal_order), -1) + 1 "
                                   "FROM records WHERE file_id = :fileId))");
        if (!opt) {
            qWarning() << "GRecordDao: Failed to prepare insert"
                       << ":" << insertRecord.lastError().text();
            trans.setRollbackOnly();
            return;
        }
        insertRecord.bindValue(":fileId", static_cast<qlonglong>(fileId));

        if (!insertRecord.exec()) {
            qWarning() << "GRecordDao: Failed to insert record:" << insertRecord.lastError().text();
            trans.setRollbackOnly();
            return;
        }
        recordId = insertRecord.lastInsertId().toLongLong();
        record.setDbId(recordId);
    } else {
        // 已有记录：先删除其所有旧字段（简单但有效的全量替换）
        QSqlQuery deleteFields(db);
        opt = deleteFields.prepare("DELETE FROM record_fields WHERE record_id = :recordId");
        if (!opt) {
            qWarning() << "GRecordDao: Failed to prepare delete"
                       << ":" << deleteFields.lastError().text();
            trans.setRollbackOnly();
            return;
        }
        deleteFields.bindValue(":recordId", static_cast<qlonglong>(recordId));
        if (!deleteFields.exec()) {
            qWarning() << "GRecordDao: Failed to clear fields for record" << recordId << ":"
                       << deleteFields.lastError().text();
            trans.setRollbackOnly();
            return;
        }
    }

    // 插入当前所有字段
    QSqlQuery insertField(db);
    opt = insertField.prepare("INSERT INTO record_fields (record_id, field_key, field_value) "
                              "VALUES (:recordId, :key, :value)");
    if (!opt) {
        qWarning() << "GRecordDao: Failed to prepare insert"
                   << ":" << insertField.lastError().text();
        trans.setRollbackOnly();
        return;
    }

    for (const auto &[key, valueWrap] : record) {
        insertField.bindValue(":recordId", static_cast<qlonglong>(recordId));
        insertField.bindValue(":key", qstr(key));
        std::string valueStr = static_cast<std::string>(valueWrap);
        insertField.bindValue(":value", qstr(valueStr));

        if (!insertField.exec()) {
            qWarning() << "GRecordDao: Failed to insert field" << qstr(key) << ":"
                       << insertField.lastError().text();
            trans.setRollbackOnly();
            return;
        }
    }

    trans.commit();
}

void GRecordDao::deleteRecord(SqliteDbExecutor &exec, int64_t recordDbId) {
    auto &db = exec.database();
    if (recordDbId == -1) return;

    auto trans = exec.createTransaction();
    bool opt = false;
    QSqlQuery query(db);
    opt = query.prepare("DELETE FROM records WHERE id = :id");
    if (!opt) {
        qWarning() << "GRecordDao: Failed to prepare delete"
                   << ":" << query.lastError().text();
        trans.setRollbackOnly();
        return;
    }
    query.bindValue(":id", static_cast<qlonglong>(recordDbId));

    if (!query.exec()) {
        qWarning() << "GRecordDao: Failed to delete record:" << query.lastError().text();
        trans.setRollbackOnly();
        return;
    }

    trans.commit();
    // ON DELETE CASCADE 自动清理 record_fields
}

void GRecordDao::updateField(SqliteDbExecutor &exec, int64_t recordDbId, const std::string &key,
                             const std::string &value) {
    auto &db = exec.database();
    if (recordDbId == -1) {
        qWarning() << "GRecordDao: updateField called with invalid recordDbId (-1)";
        return;
    }

    auto trans = exec.createTransaction();
    bool opt = false;
    QSqlQuery query(db);
    // INSERT OR REPLACE 实现 upsert
    opt = query.prepare("INSERT OR REPLACE INTO record_fields (record_id, field_key, field_value) "
                        "VALUES (:recordId, :key, :value)");
    if (!opt) {
        qWarning() << "GRecordDao: Failed to prepare update"
                   << ":" << query.lastError().text();
        trans.setRollbackOnly();
        return;
    }
    query.bindValue(":recordId", static_cast<qlonglong>(recordDbId));
    query.bindValue(":key", qstr(key));
    query.bindValue(":value", qstr(value));

    if (!query.exec()) {
        qWarning() << "GRecordDao: Failed to update field" << qstr(key) << ":"
                   << query.lastError().text();
        trans.setRollbackOnly();
        return;
    }

    trans.commit();
}

void GRecordDao::deleteField(SqliteDbExecutor &exec, int64_t recordDbId, const std::string &key) {
    auto &db = exec.database();
    if (recordDbId == -1) return;

    auto trans = exec.createTransaction();
    bool opt = false;
    QSqlQuery query(db);
    opt =
        query.prepare("DELETE FROM record_fields WHERE record_id = :recordId AND field_key = :key");
    if (!opt) {
        qWarning() << "GRecordDao: Failed to prepare delete"
                   << ":" << query.lastError().text();
        trans.setRollbackOnly();
        return;
    }
    query.bindValue(":recordId", static_cast<qlonglong>(recordDbId));
    query.bindValue(":key", qstr(key));

    if (!query.exec()) {
        qWarning() << "GRecordDao: Failed to delete field" << qstr(key) << ":"
                   << query.lastError().text();
        trans.setRollbackOnly();
        return;
    }

    trans.commit();
}

void GRecordDao::syncOrder(SqliteDbExecutor &exec, const std::vector<GRecord> &records) {
    auto &db = exec.database();
    if (records.empty()) return;

    auto trans = exec.createTransaction();
    bool opt = false;
    QSqlQuery query(db);
    opt = query.prepare("UPDATE records SET internal_order = :idx WHERE id = :id");
    if (!opt) {
        qWarning() << "GRecordDao: Failed to prepare SET internal_order"
                   << ":" << query.lastError().text();
        trans.setRollbackOnly();
        return;
    }

    for (size_t i = 0; i < records.size(); ++i) {
        int64_t dbId = records[i].getDbId();
        if (dbId == -1) continue; // 未持久化的记录跳过

        query.bindValue(":idx", static_cast<qlonglong>(i));
        query.bindValue(":id", static_cast<qlonglong>(dbId));

        if (!query.exec()) {
            qWarning() << "GRecordDao: Failed to update order for record" << dbId << ":"
                       << query.lastError().text();
            trans.setRollbackOnly();
            return;
        }
    }

    trans.commit();
}
