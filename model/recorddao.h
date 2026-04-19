#ifndef RECORDDAO_H
#define RECORDDAO_H

#include <QSqlDatabase>
#include <QString>
#include <cstdint>
#include <string>
#include <vector>

class SqliteDbExecutor;
class GRecord;

class GRecordDao {
  public:
    // 初始化数据库表结构（若不存在则创建）
    static bool initSchema(SqliteDbExecutor &exec);

    // 文件级操作
    static int64_t createFile(SqliteDbExecutor &exec, const std::string &filename);
    static void deleteFile(SqliteDbExecutor &exec, int64_t fileId);
    static std::vector<GRecord> loadFile(SqliteDbExecutor &exec, int64_t fileId);
    static void updateFileSyncTime(SqliteDbExecutor &exec, int64_t fileId);

    // 记录级操作（upsert 包含字段级差异更新）
    static void upsertRecord(SqliteDbExecutor &exec, int64_t fileId, GRecord &record);
    static void deleteRecord(SqliteDbExecutor &exec, int64_t recordDbId);

    // 字段级更新
    static void updateField(SqliteDbExecutor &exec, int64_t recordDbId, const std::string &key,
                            const std::string &value);
    static void deleteField(SqliteDbExecutor &exec, int64_t recordDbId, const std::string &key);

    // 同步内存顺序到数据库
    static void syncOrder(SqliteDbExecutor &exec, const std::vector<GRecord> &records);
};

#endif // RECORDDAO_H
