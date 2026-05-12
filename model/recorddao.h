#ifndef RECORDDAO_H
#define RECORDDAO_H

#include <QSqlDatabase>
#include <QString>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SqliteDbExecutor;
class GRecord;

class GRecordDao {
  public:
    // 初始化数据库表结构（若不存在则创建）。失败抛出 std::runtime_error。
    static void initSchema(SqliteDbExecutor &exec);

    // 文件级操作（失败抛出 std::runtime_error）
    static int64_t createFile(SqliteDbExecutor &exec, const std::string &filename);
    static std::optional<int64_t> findFileIdByFilename(SqliteDbExecutor &exec,
                                                       const std::string &canonicalPath);
    static void deleteFile(SqliteDbExecutor &exec, int64_t fileId);
    static std::vector<GRecord> loadFile(SqliteDbExecutor &exec, int64_t fileId);
    static void updateFileSyncTime(SqliteDbExecutor &exec, int64_t fileId);

    // 记录级操作（upsert 包含字段级差异更新；失败抛出 std::runtime_error）
    static void upsertRecord(SqliteDbExecutor &exec, int64_t fileId, GRecord &record);
    // recordDbId == -1 时为 no-op（未持久化行）
    static void deleteRecord(SqliteDbExecutor &exec, int64_t recordDbId);

    // 字段级更新（失败抛出 std::runtime_error；recordDbId == -1 抛出 std::invalid_argument）
    static void updateField(SqliteDbExecutor &exec, int64_t recordDbId, const std::string &key,
                            const std::string &value);
    // recordDbId == -1 时为 no-op
    static void deleteField(SqliteDbExecutor &exec, int64_t recordDbId, const std::string &key);

    // 同步内存顺序到数据库（失败抛出 std::runtime_error）
    static void syncOrder(SqliteDbExecutor &exec, const std::vector<GRecord> &records);
};

#endif // RECORDDAO_H
