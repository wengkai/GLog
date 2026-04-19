#include "recordrepository.h"
#include "Concurrent.h"
#include "recorddao.h"

// --- 异步 API 实现 ---

QFuture<bool> GRecordRepository::initSchemaAsync() {
    // 捕获 m_dbExec 共享指针，确保 executor 存活
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture([exec]() -> bool { return GRecordDao::initSchema(*exec); },
                                      *exec);
}

QFuture<int64_t> GRecordRepository::createFileAsync(const std::string &filename) {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, filename]() -> int64_t { return GRecordDao::createFile(*exec, filename); }, *exec);
}

QFuture<void> GRecordRepository::deleteFileAsync(int64_t fileId) {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture([exec, fileId]() { GRecordDao::deleteFile(*exec, fileId); },
                                      *exec);
}

QFuture<std::vector<GRecord>> GRecordRepository::loadFileAsync(int64_t fileId) {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, fileId]() -> std::vector<GRecord> { return GRecordDao::loadFile(*exec, fileId); },
        *exec);
}

QFuture<void> GRecordRepository::updateFileSyncTimeAsync(int64_t fileId) {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, fileId]() { GRecordDao::updateFileSyncTime(*exec, fileId); }, *exec);
}

QFuture<void> GRecordRepository::upsertRecordAsync(int64_t fileId, const GRecord &record) {
    auto exec = m_dbExec;
    // 先同步复制副本（深拷贝，但共享 m_dbInternalId 原子指针）
    GRecord snapshot = record.cloneForPersistence();
    return GLogConcurrent::makeFuture(
        [exec, fileId, snapshot = std::move(snapshot)]() mutable {
            // snapshot 内的 m_dbInternalId 与原始 record 共享，修改会传回上层
            GRecordDao::upsertRecord(*exec, fileId, snapshot);
        },
        *exec);
}

QFuture<void> GRecordRepository::deleteRecordAsync(int64_t recordDbId) {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, recordDbId]() { GRecordDao::deleteRecord(*exec, recordDbId); }, *exec);
}

QFuture<void> GRecordRepository::updateFieldAsync(int64_t recordDbId, const std::string &key,
                                                  const std::string &value) {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, recordDbId, key, value]() {
            GRecordDao::updateField(*exec, recordDbId, key, value);
        },
        *exec);
}

QFuture<void> GRecordRepository::deleteFieldAsync(int64_t recordDbId, const std::string &key) {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, recordDbId, key]() { GRecordDao::deleteField(*exec, recordDbId, key); }, *exec);
}

QFuture<void> GRecordRepository::syncOrderAsync(const std::vector<GRecord> &records) {
    auto exec = m_dbExec;
    // 构建轻量副本（共享 m_dbInternalId，不深拷贝字段数据）
    std::vector<GRecord> snapshots;
    snapshots.reserve(records.size());
    for (const auto &rec : records) {
        snapshots.push_back(rec.cloneForSyncOrder());
    }
    return GLogConcurrent::makeFuture(
        [exec, snapshots = std::move(snapshots)]() { GRecordDao::syncOrder(*exec, snapshots); },
        *exec);
}