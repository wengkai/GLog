#ifndef RECORD_REPOSITORY_H
#define RECORD_REPOSITORY_H

#include <QFuture>
#include <QPromise>
#include <QThread>
#include <QThreadPool>
#include <exception>
#include <functional>
#include <type_traits>

#include "SqliteDbExecutor.h"
#include "record.h"

class GRecordRepository {
    // 持有之前审查过的执行器
    std::shared_ptr<SqliteDbExecutor> m_dbExec;

  public:
    explicit GRecordRepository(const QString &dbPath)
        : GRecordRepository(std::make_shared<SqliteDbExecutor>(dbPath)) {}
    explicit GRecordRepository(std::shared_ptr<SqliteDbExecutor> exec)
        : m_dbExec(std::move(exec)) {}

    // --- 异步 API ---

    // 初始化数据库表结构（若不存在则创建）
    QFuture<bool> initSchemaAsync();

    // 文件级操作
    QFuture<int64_t> createFileAsync(const std::string &filename);
    QFuture<void> deleteFileAsync(int64_t fileId);
    QFuture<std::vector<GRecord>> loadFileAsync(int64_t fileId);
    QFuture<void> updateFileSyncTimeAsync(int64_t fileId);

    // 记录级操作（upsert 包含字段级差异更新）

    // 内部先同步复制副本，再异步刷盘
    // 即使是值传递，修改m_dbInternalId的结果也能传回上层
    QFuture<void> upsertRecordAsync(int64_t fileId, const GRecord &record);
    QFuture<void> deleteRecordAsync(int64_t recordDbId);

    // 字段级更新
    QFuture<void> updateFieldAsync(int64_t recordDbId, const std::string &key,
                                   const std::string &value);
    QFuture<void> deleteFieldAsync(int64_t recordDbId, const std::string &key);

    // 内部先同步复制副本，再异步刷盘
    QFuture<void> syncOrderAsync(const std::vector<GRecord> &records);
};

#endif // RECORD_REPOSITORY_H