#include "recordrepository.h"
#include "Concurrent.h"
#include "recorddao.h"

auto GRecordRepository::initSchemaAsync() -> QFuture<void> {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture([exec]() { GRecordDao::initSchema(*exec); }, *exec);
}

auto GRecordRepository::createFileAsync(std::string filename) -> QFuture<int64_t> {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, filename = std::move(filename)]() -> int64_t {
            return GRecordDao::createFile(*exec, filename);
        },
        *exec);
}

auto GRecordRepository::findFileIdByFilenameAsync(std::string canonicalPath)
    -> QFuture<std::optional<int64_t>> {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, path = std::move(canonicalPath)]() -> std::optional<int64_t> {
            return GRecordDao::findFileIdByFilename(*exec, path);
        },
        *exec);
}

auto GRecordRepository::deleteFileAsync(int64_t fileId) -> QFuture<void> {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture([exec, fileId]() { GRecordDao::deleteFile(*exec, fileId); },
                                      *exec);
}

auto GRecordRepository::loadFileAsync(int64_t fileId) -> QFuture<std::vector<GRecord>> {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, fileId]() -> std::vector<GRecord> { return GRecordDao::loadFile(*exec, fileId); },
        *exec);
}

auto GRecordRepository::updateFileSyncTimeAsync(int64_t fileId) -> QFuture<void> {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, fileId]() { GRecordDao::updateFileSyncTime(*exec, fileId); }, *exec);
}

auto GRecordRepository::upsertRecordAsync(int64_t fileId, const GRecord &record) -> QFuture<void> {
    auto exec = m_dbExec;
    GRecord snapshot = record.cloneForPersistence();
    return GLogConcurrent::makeFuture(
        [exec, fileId, snapshot = std::move(snapshot)]() mutable {
            GRecordDao::upsertRecord(*exec, fileId, snapshot);
        },
        *exec);
}

auto GRecordRepository::deleteRecordAsync(int64_t recordDbId) -> QFuture<void> {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, recordDbId]() { GRecordDao::deleteRecord(*exec, recordDbId); }, *exec);
}

auto GRecordRepository::updateFieldAsync(int64_t recordDbId, std::string key, std::string value)
    -> QFuture<void> {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, recordDbId, key = std::move(key), value = std::move(value)]() {
            GRecordDao::updateField(*exec, recordDbId, key, value);
        },
        *exec);
}

auto GRecordRepository::deleteFieldAsync(int64_t recordDbId, std::string key) -> QFuture<void> {
    auto exec = m_dbExec;
    return GLogConcurrent::makeFuture(
        [exec, recordDbId, key = std::move(key)]() {
            GRecordDao::deleteField(*exec, recordDbId, key);
        },
        *exec);
}

auto GRecordRepository::syncOrderAsync(const std::vector<GRecord> &records) -> QFuture<void> {
    auto exec = m_dbExec;
    std::vector<GRecord> snapshots;
    snapshots.reserve(records.size());
    for (const auto &rec : records) {
        snapshots.push_back(rec.cloneForSyncOrder());
    }
    return GLogConcurrent::makeFuture(
        [exec, snapshots = std::move(snapshots)]() { GRecordDao::syncOrder(*exec, snapshots); },
        *exec);
}