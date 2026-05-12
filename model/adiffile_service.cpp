#include "adiffile_service.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QPointer>
#include <QSaveFile>
#include <QTextStream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "Concurrent.h"
#include "adif_serialization.h"
#include "adifdb.h"
#include "adifparse_service.h"
#include "recordrepository.h"

namespace {

auto operator<<(QTextStream &stream, const std::string &str) -> QTextStream & {
    stream << QString::fromStdString(str);
    return stream;
}

auto operator<<(QTextStream &stream, std::string_view sv) -> QTextStream & {
    stream << QString::fromUtf8(sv);
    return stream;
}

auto persistableAbsolutePath(const QString &filename) -> QString {
    const QFileInfo fi(filename);
    const QString c = fi.canonicalFilePath();
    if (!c.isEmpty()) {
        return c;
    }
    return fi.absoluteFilePath();
}

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

template <typename T> auto syncWaitResult(QFuture<T> future) -> T {
    waitForFuture(future);
    future.waitForFinished();
    return future.result();
}

void syncWaitVoid(QFuture<void> future) {
    waitForFuture(future);
    future.waitForFinished();
}

void runApplyFullLoadOnGui(AdifModel *model, AdifParseService::ParseRes &&res) {
    auto fut = GLogConcurrent::makeFuture(
        [model, r = std::move(res)]() mutable { model->applyFullLoad(std::move(r)); },
        GLogConcurrent::MainThreadExecutor{});
    waitForFuture(fut);
    fut.waitForFinished();
}

void runApplyInsertAtOnGui(AdifModel *model, int row, AdifParseService::ParseRes &&res) {
    auto fut = GLogConcurrent::makeFuture(
        [model, row, r = std::move(res)]() mutable { model->applyInsertAt(row, std::move(r)); },
        GLogConcurrent::MainThreadExecutor{});
    waitForFuture(fut);
    fut.waitForFinished();
}

} // namespace

AdifFileService::AdifFileService(AdifModel *model, QObject *parent)
    : QObject(parent), m_model(model) {}

auto AdifFileService::openFile(const QString &filename, bool remove) -> std::vector<std::string> {
    QFileInfo file(filename);
    std::ifstream in(file.filesystemAbsoluteFilePath());
    if (!in) {
        throw std::runtime_error("Can not open file");
    }
    std::vector<std::string> errors;
    auto res = AdifParseService::parse(in, errors, AdifParseService::ParseMode::Batch);
    in.close();
    if (!res.parse_res) {
        throw std::runtime_error(errors.empty() ? "Unknown parse error" : errors.back());
    }

    if (auto repo = m_model->getDbBackup()) {
        const std::string canon = persistableAbsolutePath(filename).toStdString();
        if (auto existing = syncWaitResult(repo->findFileIdByFilenameAsync(canon))) {
            syncWaitVoid(repo->deleteFileAsync(*existing));
        }
        m_activeFileId = syncWaitResult(repo->createFileAsync(canon));
    } else {
        m_activeFileId = -1;
    }

    runApplyFullLoadOnGui(m_model, std::move(res));

    if (auto repo = m_model->getDbBackup(); repo && m_activeFileId > 0) {
        const std::vector<GRecord> batch = syncWaitResult(GLogConcurrent::makeFuture(
            [this]() { return m_model->cloneRecordsForPersistenceSlice(0, -1); },
            GLogConcurrent::MainThreadExecutor{}));
        for (const auto &rec : batch) {
            syncWaitVoid(repo->upsertRecordAsync(m_activeFileId, rec));
        }
    }

    if (remove) {
        QFile::remove(QFileInfo(filename).absoluteFilePath());
    }
    return errors;
}

auto AdifFileService::openFileAsync(const QString &filename, bool remove)
    -> QFuture<std::vector<std::string>> {
    return GLogConcurrent::makeFuture(
        [this, filename = filename, remove](QPromise<std::vector<std::string>> &promise) {
            promise.addResult(openFile(filename, remove));
        },
        *(m_model->getFIFOBackendThreadExecutor()));
}

auto AdifFileService::insertFile(int row, const QString &filename, bool remove)
    -> std::vector<std::string> {
    QFileInfo file(filename);
    std::ifstream in(file.filesystemAbsoluteFilePath());
    if (!in) {
        throw std::runtime_error("Can not open file");
    }
    std::vector<std::string> errors;
    auto res = AdifParseService::parse(in, errors, AdifParseService::ParseMode::Batch);
    in.close();
    if (!res.parse_res) {
        throw std::runtime_error(errors.empty() ? "Unknown parse error" : errors.back());
    }

    if (auto repo = m_model->getDbBackup()) {
        const std::string canon = persistableAbsolutePath(filename).toStdString();
        if (m_activeFileId <= 0) {
            if (auto existing = syncWaitResult(repo->findFileIdByFilenameAsync(canon))) {
                m_activeFileId = *existing;
            } else {
                m_activeFileId = syncWaitResult(repo->createFileAsync(canon));
            }
        }
    }

    runApplyInsertAtOnGui(m_model, row, std::move(res));

    if (remove) {
        QFile::remove(QFileInfo(filename).absoluteFilePath());
    }
    return errors;
}

auto AdifFileService::insertFileAsync(int where, const QString &filename, bool remove)
    -> QFuture<std::vector<std::string>> {
    return GLogConcurrent::makeFuture(
        [this, filename = filename, where, remove](QPromise<std::vector<std::string>> &promise) {
            promise.addResult(insertFile(where, filename, remove));
        },
        *(m_model->getFIFOBackendThreadExecutor()));
}

auto AdifFileService::insertStringData(int row, const std::string &data)
    -> std::vector<std::string> {
    std::stringstream in(data);
    std::vector<std::string> errors;
    auto res = AdifParseService::parse(in, errors, AdifParseService::ParseMode::Batch);
    if (!res.parse_res) {
        throw std::runtime_error(errors.empty() ? "Unknown parse error" : errors.back());
    }
    runApplyInsertAtOnGui(m_model, row, std::move(res));
    return errors;
}

auto AdifFileService::insertStringDataAsync(int row, std::string data)
    -> QFuture<std::vector<std::string>> {
    return GLogConcurrent::makeFuture(
        [this, row, data = std::move(data)]() mutable { return insertStringData(row, data); },
        *(m_model->getFIFOBackendThreadExecutor()));
}

void AdifFileService::saveAs(const QString &filename) {
    if (m_model->rowCount() == 0) {
        return;
    }
    QPointer<AdifFileService> self(this);
    AdifModel *model = m_model;
    model->snapshotAsync().then(
        QCoreApplication::instance(), [self, filename, model](AdifModelSnapshot snap) mutable {
            if (!self) {
                return;
            }
            std::vector<GRecord> p_records = std::move(snap.records);
            std::vector<std::string> p_rheaders = std::move(snap.columnHeaders);
            GLogConcurrent::makeFuture(
                [filename, p_records = std::move(p_records),
                 p_rheaders = std::move(p_rheaders)]() mutable {
                    QSaveFile outFile(filename);
                    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        throw std::runtime_error("Cannot open file for writing");
                    }
                    QTextStream stream(&outFile);
                    bool handled = false;
                    if (filename.endsWith(".csv", Qt::CaseInsensitive)) {
                        AdifSerialization::toCsv(stream, p_records, p_rheaders);
                        handled = true;
                    } else if (filename.endsWith(".adi", Qt::CaseInsensitive) ||
                               filename.endsWith(".adif", Qt::CaseInsensitive)) {
                        AdifSerialization::toAdif(stream, p_records);
                        handled = true;
                    }
                    if (!handled) {
                        throw std::runtime_error("Unsupported file extension");
                    }
                    if (!outFile.commit()) {
                        throw std::runtime_error("Failed to commit changes to file");
                    }
                },
                *(model->getFIFOBackendThreadExecutor()))
                .then(QCoreApplication::instance(),
                      [self]() {
                          if (self) {
                              emit self->saveDone();
                          }
                      })
                .onFailed(QCoreApplication::instance(), [self](const std::exception &e) {
                    if (self) {
                        emit self->saveFailed(QString::fromUtf8(e.what()));
                    }
                });
        });
}
