#include "adifdb.h"
#include <QApplication>
#include <QByteArrayView>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <type_traits>
#include "CaseInsensitiveLess.h"
#include "Concurrent.h"
#include "adiffile_service.h"

#ifdef _WIN32
#include <io.h>
#define IS_PIPED (!_isatty(_fileno(stdin)))
#else
#include <unistd.h>
#define IS_PIPED (!isatty(fileno(stdin)))
#endif

namespace {

void attachBackupFailureLogger(QFuture<void> fut) {
    fut.onFailed(QCoreApplication::instance(), [](const std::exception &e) {
        qWarning() << "GRecordRepository backup task failed:" << e.what();
    });
}

} // namespace

void AdifModel::newViewWithRows(const QModelIndexList &indexes) const {
    auto *mineData = mimeData(indexes);
    GLogConcurrent::makeFuture([text = mineData->data("text/plain")]() {
        auto *process = new QProcess();
        QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         process, &QObject::deleteLater);
        process->setProcessChannelMode(QProcess::ForwardedChannels);
        process->start(QCoreApplication::applicationFilePath(), {});
        if (!process->waitForStarted()) {
            qDebug() << "Failed to create new process";
            process->deleteLater();
            return;
        }
        process->write(text);
        if (!process->waitForBytesWritten()) {
            qDebug() << "Write timeout or error:" << process->errorString();
        }
        process->closeWriteChannel();
    });
    // process->setProcessChannelMode(QProcess::ForwardedChannels);

    // auto tempFile = new QTemporaryFile(QDir::tempPath() + "/XXXXXX.txt");
    // tempFile->setAutoRemove(false);

    // if (tempFile->open()) {
    //     tempFile->write(text);
    //     QString tempFilePath = tempFile->fileName();
    //     tempFile->close();

    //     QString program = QCoreApplication::applicationFilePath();
    //     QStringList arguments;
    //     arguments << "-i" << tempFilePath << "--remove";

    //     if (QProcess::startDetached(program, arguments)) {
    //         qDebug() << "Handoff successful. Path:" << tempFilePath;
    //     } else {
    //         qWarning() << "Failed to start detached process.";
    //         QFile::remove(tempFilePath);
    //     }
    // }
    // tempFile->deleteLater();
}

void AdifModel::pasteRows(const QMimeData *mimeData) {
    dropMimeData(mimeData, Qt::DropAction::CopyAction, -1, -1, QModelIndex());
}

void AdifModel::copyRows(const QModelIndexList &indexes) {
    auto *mimeData = new QMimeData();
    mimeData->setData("text/plain", this->mimeData(indexes)->data("text/plain"));
    emit setCilpboard(mimeData);
}

void AdifModel::findNextS(QModelIndex current, const QString &key, const QString &value,
                          bool isReg) {
    auto sv = value.toStdString();
    auto sk = key.toStdString();
    if (!isReg) {
        auto index = this->findNext(
            current, [=](const std::string &v) { return v.find(sv) != std::string::npos; }, sk);
        emit foundNext(index);
        return;
    }
    QRegularExpression re(value);
    auto match = [=](const std::string &v) {
        return re.match(QString::fromStdString(v)).hasMatch();
    };
    auto index = this->findNext(current, match, sk);
    emit foundNext(index);
}

void AdifModel::selectAll(const QString &key, const QString &value, bool isReg) {
    auto sv = value.toStdString();
    auto sk = key.toStdString();
    if (!isReg) {
        auto rows = this->findAll(
            [=](const std::string &v) { return v.find(sv) != std::string::npos; }, sk);
        emit selectRows(rows);
        return;
    }
    QRegularExpression re(value);
    auto match = [=](const std::string &v) {
        return re.match(QString::fromStdString(v)).hasMatch();
    };
    auto rows = this->findAll(match, sk);
    emit selectRows(rows);
}

void AdifModel::deselectAll(const QString &key, const QString &value, bool isReg) {
    auto sv = value.toStdString();
    auto sk = key.toStdString();
    if (!isReg) {
        auto rows = this->findAll(
            [=](const std::string &v) { return v.find(sv) != std::string::npos; }, sk);
        emit deselectRows(rows);
        return;
    }
    QRegularExpression re(value);
    auto match = [=](const std::string &v) {
        return re.match(QString::fromStdString(v)).hasMatch();
    };
    auto rows = this->findAll(match, sk);
    emit deselectRows(rows);
}

auto AdifModel::addHeader(std::string header) -> bool { return _addHeader(std::move(header)); }

auto AdifModel::_addHeaderCheck(const std::string &header) const
    -> std::pair<bool, std::vector<std::string>::const_iterator> {
    if (sheaders.count(header) != 0) {
        return {false, rheaders.end()};
    }
    auto iter = std::lower_bound(rheaders_begin(), rheaders.end(), header);
    if (iter != rheaders.end() && *iter == header) {
        return {false, rheaders.end()};
    }
    return {true, iter};
}

auto AdifModel::_addHeader(std::string header) -> bool {
    bool inserted = false;
    GLogConcurrent::runOnMainThreadSync([this, header = std::move(header), &inserted]() mutable {
        auto [check, iter] = _addHeaderCheck(header);
        if (!check) {
            return;
        }
        auto column = static_cast<int>(iter - rheaders_begin());
        beginInsertColumns(QModelIndex(), column, column);
        rheaders.insert(iter, std::move(header));
        endInsertColumns();
        inserted = true;
    });
    return inserted;
}

auto AdifModel::_addHeaderNoSignal(std::string header) -> bool {
    auto [check, iter] = _addHeaderCheck(header);
    if (!check) {
        return false;
    }
    rheaders.insert(iter, std::move(header));
    return true;
}

auto AdifModel::_records2StdString(const std::set<int> &rows) const -> std::string {
    std::stringstream stream{};
    for (const auto &row : rows) {
        if (0 <= row && row < m_records.size()) {
            stream << m_records[row] << "\n";
        }
    }
    return stream.str();
}

auto AdifModel::getDefaultSortModel(const std::string &field) -> std::vector<std::string> {
    std::vector<std::string> ret{field};
    if (field == "qso_date") {
        ret.emplace_back("time_on");
        return ret;
    }
    if (field == "time_on") {
        ret.emplace_back("qso_date");
        return ret;
    }
    ret.emplace_back("qso_date");
    ret.emplace_back("time_on");
    return ret;
}

void AdifModel::mapCallSignInView(bool keepOrigin) {
    GLogConcurrent::runOnMainThreadSync([this, keepOrigin]() {
        if (m_records.empty()) {
            return;
        }
        std::string testField = "z_glog_resolved_1";
        _addHeader(testField);
        auto *ctydb = CtyDB::instance();
        emit mapCallSignInViewBegin();
        std::shared_lock<decltype(ctydb->mutex)> lock0(ctydb->mutex);

        int failCount = 0;
        int confictCount = 0;
        auto writeByKeepOriginFlag = [&](Record &dest, const std::string &key,
                                         const std::string &value) {
            if (auto iter = dest.find(key); iter != dest.end() && iter->second->get() != value) {
                ++confictCount;
                if (keepOrigin) {
                    return;
                }
            }
            dest.addOrSetPair(key, value);
        };

        QString buf;
        for (auto &record : m_records) {
            auto call = record.at("call")->get();
            buf = QString::fromUtf8(call.data(), qsizetype(call.size()));
            CtyDB::normalizeCallSign(buf);
            auto result = ctydb->lookUpCallSign(QStringView(buf));
            if (result.first->valid) {
                record.addOrSetPair(testField, (result.first->name + result.second).toStdString());
                // CQZ, ITUZ, CONT, COUNTRY
                writeByKeepOriginFlag(record, "cqz",
                                      QString::number(result.first->cq).toStdString());
                writeByKeepOriginFlag(record, "ituz",
                                      QString::number(result.first->itu).toStdString());
                writeByKeepOriginFlag(record, "cont", result.first->continent.toStdString());
                writeByKeepOriginFlag(record, "country", result.first->name.toStdString());
            } else {
                record.addOrSetPair(testField, result.second.toStdString());
                ++failCount;
            }
        }

        emit layoutChanged();
        emit mapCallSignInViewEnd(failCount, confictCount);
    });
}

void AdifModel::_clear() {
    m_records.clear();
    while (rheaders.size() > GRecord::RESOLVE_HEADERS_COUNT) {
        rheaders.pop_back();
    }
}

AdifModel::AdifModel(QObject *parent) : QAbstractTableModel(parent) {
    for (const auto *i : GRecord::RESOLVE_HEADERS) {
        rheaders.emplace_back(i);
        sheaders.insert(i);
    }
}

auto AdifModel::rowCount(const QModelIndex &parent) const -> int { return int(m_records.size()); }

auto AdifModel::columnCount(const QModelIndex &parent) const -> int { return int(rheaders.size()); }

auto AdifModel::data(const QModelIndex &index, int role) const -> QVariant {
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }

    if (!index.isValid()) {
        return {};
    }

    if (index.column() < rheaders.size() && index.row() < m_records.size()) {
        const auto &key = rheaders.at(index.column());
        const auto &row = m_records.at(index.row());
        auto iter = row.find(key);
        if (iter != row.end() && iter->second) {
            return QString::fromUtf8(iter->second->asView());
        }
    }
    return {};
}

auto AdifModel::headerData(int section, Qt::Orientation orientation, int role) const -> QVariant {
    if (role != Qt::DisplayRole) {
        return {};
    }
    if (orientation == Qt::Horizontal) {
        QVariant ret{};
        if (section < rheaders.size()) {
            return QString::fromStdString(rheaders.at(section));
        }
    }
    return section + 1;
}

auto AdifModel::setData(const QModelIndex &index, const QVariant &value, int role) -> bool {
    if (role == Qt::EditRole) {
        auto v = value.toString().toStdString();
        if (index.row() < m_records.size() && index.column() < rheaders.size()) {
            auto &field = rheaders[index.column()];
            if (field == "qso_date" || field == "time_on") {
                return false;
            }
            auto res = m_records[index.row()].addOrSetPair(field, v);
            dataChanged(index, index);
            tryDbBackup([&](const std::shared_ptr<GRecordRepository> &repo) {
                const int64_t fid = persistFileIdForBackup();
                auto &rec = m_records[index.row()];
                const int64_t dbId = rec.getDbId();
                if (dbId > 0) {
                    attachBackupFailureLogger(repo->updateFieldAsync(dbId, field, v));
                } else if (fid > 0) {
                    attachBackupFailureLogger(repo->upsertRecordAsync(fid, rec));
                }
            });
            return res;
        }
    }

    return false;
}

auto AdifModel::flags(const QModelIndex &index) const -> Qt::ItemFlags {
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled;
}

auto AdifModel::supportedDropActions() const -> Qt::DropActions {
    return QAbstractTableModel::supportedDropActions() | Qt::DropAction::MoveAction;
}

auto AdifModel::mimeTypes() const -> QStringList {
    auto ret = QAbstractTableModel::mimeTypes();
    ret.append("text/plain");
    ret.append("text/uri-list");
    return ret;
}

auto AdifModel::mimeData(const QModelIndexList &indexes) const -> QMimeData * {
    QMimeData *mimeData = QAbstractTableModel::mimeData(indexes);
    std::set<int> rows;
    for (const auto &index : indexes) {
        if (index.isValid()) {
            rows.insert(index.row());
        }
    }
    mimeData->setText(QString::fromStdString(_records2StdString(rows)));
    return mimeData;
}

auto AdifModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                             const QModelIndex &parent) -> bool {
    if (action == Qt::IgnoreAction) {
        return true;
    }

    int beginRow = (row != -1) ? row : rowCount(parent);

    bool handled = false;

    if (m_file_service) {
        if (data->hasText()) {
            std::string s = data->text().toStdString();
            m_file_service->insertStringDataAsync(beginRow, std::move(s))
                .then(this,
                      [](const std::vector<std::string> &errors) {

                      })
                .onFailed(this, [](const std::exception &e) {

                });
            handled = true;
        }

        if (data->hasUrls()) {
            auto urls = data->urls();
            for (auto url = urls.rbegin(); url != urls.rend(); ++url) {
                QString file = url->toLocalFile();
                if (!file.isEmpty()) {
                    m_file_service->insertFileAsync(beginRow, file)
                        .then(this,
                              [](const std::vector<std::string> &errors) {

                              })
                        .onFailed(this, [](const std::exception &e) {

                        });
                    handled = true;
                }
            }
        }
    }

    return handled;
}

auto AdifModel::insertRows(int row, int count, const QModelIndex &parent) -> bool {
    // std::unique_lock<decltype(mutex)> lock(mutex);
    // if (count > 0 && row >= 0 && row <= m_records.size()) {
    //     beginInsertRows(parent, row, row + count - 1);
    //     while (count--) {
    //         m_records.insert(m_records.begin() + row, decltype(m_records)::value_type{});
    //     }
    //     endInsertRows();
    // }
    return false;
}

auto AdifModel::removeRows(int row, int count, const QModelIndex &parent) -> bool {
    if (row >= 0 && count > 0 && row + count <= m_records.size()) {
        beginRemoveRows(parent, row, row + count - 1);
        m_records.erase(m_records.begin() + row, m_records.begin() + row + count);
        endRemoveRows();
        return true;
    }
    return false;
}

auto AdifModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                         const QModelIndex &destinationParent, int destinationChild) -> bool {
    if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent,
                       destinationChild)) {
        endMoveRows();
        return false;
    }
    if (sourceParent == destinationParent && destinationChild > sourceRow &&
        destinationChild < sourceRow + count) {
        endMoveRows();
        return false;
    }
    if (destinationChild > sourceRow) {
        std::rotate(m_records.begin() + sourceRow, m_records.begin() + sourceRow + count,
                    m_records.begin() + destinationChild);
    }
    if (destinationChild < sourceRow) {
        std::rotate(m_records.begin() + destinationChild, m_records.begin() + sourceRow,
                    m_records.begin() + sourceRow + count);
    }
    endMoveRows();
    tryDbBackup([&](const std::shared_ptr<GRecordRepository> &repo) {
        const int64_t fid = persistFileIdForBackup();
        if (fid <= 0) {
            return;
        }
        std::vector<GRecord> orderVec;
        orderVec.reserve(m_records.size());
        for (const auto &rec : m_records) {
            if (rec.getDbId() > 0) {
                try {
                    orderVec.push_back(rec.cloneForSyncOrder());
                } catch (const std::exception &e) {
                    qWarning() << "AdifModel::moveRows syncOrder clone skipped:" << e.what();
                    GRecord placeholder;
                    orderVec.push_back(std::move(placeholder));
                }
            } else {
                GRecord placeholder;
                orderVec.push_back(std::move(placeholder));
            }
        }
        attachBackupFailureLogger(repo->syncOrderAsync(orderVec));
    });
    return true;
}

void AdifModel::sort(int column, Qt::SortOrder order) {
    GLogConcurrent::runOnMainThreadSync([this, column, order]() {
        if (column < 0 || column >= static_cast<int>(rheaders.size())) {
            return;
        }
        std::vector<std::string> fields = getDefaultSortModel(rheaders[column]);
        auto less = [fields](const Record &a, const Record &b) -> bool {
            return Record::less(a, b, fields);
        };
        beginResetModel();
        if (order == Qt::AscendingOrder) {
            std::sort(m_records.begin(), m_records.end(),
                      [&](const Record &a, const Record &b) -> bool { return less(a, b); });
        } else {
            std::sort(m_records.begin(), m_records.end(),
                      [&](const Record &a, const Record &b) -> bool { return less(b, a); });
        }
        endResetModel();
        tryDbBackup([&](const std::shared_ptr<GRecordRepository> &repo) {
            const int64_t fid = persistFileIdForBackup();
            if (fid <= 0) {
                return;
            }
            std::vector<GRecord> orderVec;
            orderVec.reserve(m_records.size());
            for (const auto &rec : m_records) {
                if (rec.getDbId() > 0) {
                    try {
                        orderVec.push_back(rec.cloneForSyncOrder());
                    } catch (const std::exception &e) {
                        qWarning() << "AdifModel::sort syncOrder clone skipped:" << e.what();
                        GRecord placeholder;
                        orderVec.push_back(std::move(placeholder));
                    }
                } else {
                    GRecord placeholder;
                    orderVec.push_back(std::move(placeholder));
                }
            }
            attachBackupFailureLogger(repo->syncOrderAsync(orderVec));
        });
    });
}

AdifModel::~AdifModel() = default;

auto AdifModel::persistFileIdForBackup() const -> int64_t {
    return m_file_service ? m_file_service->activePersistedFileId() : -1;
}

auto AdifModel::cloneRecordsForPersistenceSlice(int startRow, int rowCount) const
    -> std::vector<GRecord> {
    std::vector<GRecord> out;
    if (startRow < 0 || startRow > static_cast<int>(m_records.size())) {
        return out;
    }
    const int end = rowCount < 0
                        ? static_cast<int>(m_records.size())
                        : std::min(static_cast<int>(m_records.size()), startRow + rowCount);
    const int span = end - startRow;
    out.reserve(span > 0 ? static_cast<size_t>(span) : 0);
    for (int i = startRow; i < end; ++i) {
        try {
            out.push_back(m_records[static_cast<size_t>(i)].cloneForPersistence());
        } catch (const std::exception &e) {
            qWarning() << "AdifModel: persistence clone skipped at row" << i << ":" << e.what();
        }
    }
    return out;
}

void AdifModel::clear() {
    GLogConcurrent::runOnMainThreadSync([this]() {
        beginResetModel();
        _clear();
        endResetModel();
    });
}

auto AdifModel::records2StdString(const std::set<int> &rows) const -> std::string {
    return _records2StdString(rows);
}

auto AdifModel::deleteRowsPersistent(const std::vector<QPersistentModelIndex> &indexes) -> size_t {
    QModelIndexList rows;
    for (const auto &index : indexes) {
        if (index.isValid()) {
            rows.append(index);
        }
    }
    return deleteRows(std::move(rows));
}

auto AdifModel::mergeRows(const QModelIndex &major, const QModelIndexList &indexes) -> bool {
    if (!major.isValid() || indexes.empty()) {
        return false;
    }
    bool ok = false;
    GLogConcurrent::runOnMainThreadSync([this, major, indexes, &ok]() {
        std::vector<Record> merged_records;
        merged_records.reserve(indexes.size());
        for (const auto &index : indexes) {
            if (!index.isValid()) {
                continue;
            }
            merged_records.push_back(m_records[index.row()]);
        }
        auto major_row = major.row();
        if (!m_records[major_row].merge(merged_records)) {
            return;
        }
        emit dataChanged(createIndex(major_row, 0), createIndex(major_row, columnCount()));
        ok = true;
        tryDbBackup([&](const std::shared_ptr<GRecordRepository> &repo) {
            const int64_t fid = persistFileIdForBackup();
            if (fid <= 0) {
                return;
            }
            const auto &majorRec = m_records[major_row];
            attachBackupFailureLogger(repo->upsertRecordAsync(fid, majorRec));
        });
    });
    return ok;
}

auto AdifModel::mergeRowsAsync(QModelIndex major, QModelIndexList indexes) -> QFuture<bool> {
    return GLogConcurrent::makeFuture(
        [this, major, indexes = std::move(indexes)]() { return mergeRows(major, indexes); },
        GLogConcurrent::MainThreadExecutor{});
}

auto AdifModel::deleteRowsAsync(QModelIndexList indexes) -> QFuture<size_t> {
    return GLogConcurrent::makeFuture(
        [this, indexes = std::move(indexes)]() mutable { return deleteRows(std::move(indexes)); },
        GLogConcurrent::MainThreadExecutor{});
}

void AdifModel::applyFullLoad(AdifParseService::ParseRes &&res) {
    if (!res.parse_res) {
        return;
    }
    _resetRecords(std::move(res.parse_records), std::move(res.parse_headers));
}

void AdifModel::applyInsertAt(int row, AdifParseService::ParseRes &&res) {
    if (!res.parse_res) {
        return;
    }
    _insertRecords(row, std::move(res.parse_records), std::move(res.parse_headers));
}

void AdifModel::_resetRecords(std::vector<Record> new_record,
                              std::unordered_set<std::string> new_headers) {
    GLogConcurrent::runOnMainThreadSync(
        [this, new_record = std::move(new_record), new_headers = std::move(new_headers)]() mutable {
            beginResetModel();
            _clear();
            m_records = std::move(new_record);
            while (!new_headers.empty()) {
                auto node = new_headers.extract(new_headers.begin());
                _addHeaderNoSignal(std::move(node.value()));
            }
            endResetModel();
        });
}

void AdifModel::_insertRecords(int where, std::vector<Record> new_record,
                               std::unordered_set<std::string> new_headers) {
    GLogConcurrent::runOnMainThreadSync([this, where, new_record = std::move(new_record),
                                         new_headers = std::move(new_headers)]() mutable {
        const int insertCount = static_cast<int>(new_record.size());
        while (!new_headers.empty()) {
            auto node = new_headers.extract(new_headers.begin());
            std::string header = std::move(node.value());
            auto [check, iter] = _addHeaderCheck(header);
            if (!check) {
                continue;
            }
            auto column = static_cast<int>(iter - rheaders_begin());
            beginInsertColumns(QModelIndex(), column, column);
            rheaders.insert(iter, std::move(header));
            endInsertColumns();
        }

        if (!new_record.empty()) {
            int row = (where < 0 || where > static_cast<int>(m_records.size()))
                          ? static_cast<int>(m_records.size())
                          : where;
            int row_end = row + static_cast<int>(new_record.size()) - 1;
            beginInsertRows(QModelIndex(), row, row_end);
            m_records.insert(m_records.begin() + row, std::make_move_iterator(new_record.begin()),
                             std::make_move_iterator(new_record.end()));
            endInsertRows();
            tryDbBackup([&](const std::shared_ptr<GRecordRepository> &repo) {
                const int64_t fid = persistFileIdForBackup();
                if (fid <= 0 || insertCount == 0) {
                    return;
                }
                for (int i = 0; i < insertCount; ++i) {
                    const auto &rec = m_records[static_cast<size_t>(row + i)];
                    attachBackupFailureLogger(repo->upsertRecordAsync(fid, rec));
                }
            });
        }
    });
}

void AdifModel::insertRecords(int where, std::vector<Record> new_record) {
    std::unordered_set<std::string> new_headers;
    for (const auto &rec : new_record) {
        for (const auto &[key, value] : rec) {
            new_headers.insert(key);
        }
    }
    _insertRecords(where, std::move(new_record), std::move(new_headers));
}

void AdifModel::insertRecords(int where, std::vector<Record> new_record,
                              std::unordered_set<std::string> new_headers) {
    _insertRecords(where, std::move(new_record), std::move(new_headers));
}

auto AdifModel::findDuplicates(const std::vector<std::string> &fields) const
    -> std::vector<std::vector<QPersistentModelIndex>> {
    if (m_records.empty() || fields.empty()) {
        return {};
    }

    // 1. Grouping Phase
    auto m_less = [&](const Record &a, const Record &b) { return Record::less(a, b, fields); };
    // Key: Record, Value: List of row integers (saves memory over QModelIndex)
    std::map<Record, std::vector<int>, decltype(m_less)> groups(m_less);
    for (int i = 0; i < static_cast<int>(m_records.size()); ++i) {
        groups[m_records[i]].push_back(i);
    }

    // 2. Collection Phase
    std::vector<std::vector<QPersistentModelIndex>> duplicates;
    for (const auto &[_r, rowList] : groups) {
        if (rowList.size() > 1) {
            std::vector<QPersistentModelIndex> p_indices;
            p_indices.reserve(rowList.size());

            for (int row : rowList) {
                // Create index on the fly and convert to persistent
                p_indices.emplace_back(createIndex(row, 0));
            }
            duplicates.push_back(std::move(p_indices));
        }
    }

    return duplicates;
}

void AdifModel::sortSelectedColumn(int column, Qt::SortOrder order) { sort(column, order); }

void AdifModel::removeSelectedColumn(int column) {
    GLogConcurrent::runOnMainThreadSync([this, column]() {
        if (column < static_cast<int>(sheaders.size()) ||
            column >= static_cast<int>(rheaders.size())) {
            return;
        }
        beginRemoveColumns(QModelIndex(), column, column);
        auto field = rheaders.at(column);
        for (auto &record : m_records) {
            record.tryRemoveField(field);
        }
        rheaders.erase(rheaders.begin() + column);
        endRemoveColumns();
        tryDbBackup([&](const std::shared_ptr<GRecordRepository> &repo) {
            const std::string removedKey = field;
            for (const auto &record : m_records) {
                const int64_t dbId = record.getDbId();
                if (dbId > 0) {
                    attachBackupFailureLogger(repo->deleteFieldAsync(dbId, removedKey));
                }
            }
        });
    });
}

auto AdifModel::deleteRows(QModelIndexList indexes) -> size_t {
    size_t removed = 0;
    GLogConcurrent::runOnMainThreadSync([this, indexes = std::move(indexes), &removed]() mutable {
        indexes.erase(std::remove_if(indexes.begin(), indexes.end(),
                                     [](const QModelIndex &idx) { return !idx.isValid(); }),
                      indexes.end());
        if (indexes.empty()) {
            return;
        }
        std::sort(indexes.begin(), indexes.end(),
                  [](const QModelIndex &a, const QModelIndex &b) { return a.row() < b.row(); });
        indexes.erase(std::unique(indexes.begin(), indexes.end(),
                                  [](const QModelIndex &a, const QModelIndex &b) {
                                      return a.row() == b.row();
                                  }),
                      indexes.end());
        std::vector<int64_t> dbIdsToRemove;
        dbIdsToRemove.reserve(indexes.size());
        for (const auto &idx : indexes) {
            const int64_t id = m_records.at(idx.row()).getDbId();
            if (id > 0) {
                dbIdsToRemove.push_back(id);
            }
        }
        std::sort(dbIdsToRemove.begin(), dbIdsToRemove.end());
        dbIdsToRemove.erase(std::unique(dbIdsToRemove.begin(), dbIdsToRemove.end()),
                            dbIdsToRemove.end());
        auto idx = indexes.size() - 1;
        int deletebegin = indexes.at(idx).row();
        int deleteend = deletebegin + 1;
        while (true) {
            if (idx > 0 && indexes.at(idx - 1).row() >= 0 &&
                indexes.at(idx - 1).row() == deletebegin - 1) {
                --deletebegin;
                --idx;
                continue;
            }
            beginRemoveRows(QModelIndex(), deletebegin, deleteend - 1);
            m_records.erase(m_records.begin() + deletebegin, m_records.begin() + deleteend);
            endRemoveRows();
            if (idx == 0) {
                break;
            }
            --idx;
            deletebegin = indexes.at(idx).row();
            deleteend = deletebegin + 1;
        }
        auto iter = rheaders_begin();
        while (iter != rheaders.end()) {
            bool valid = false;
            for (auto &record : m_records) {
                if (record.find(*iter) != record.end()) {
                    valid = true;
                    break;
                }
            }
            if (valid) {
                ++iter;
                continue;
            }
            auto cloumn = static_cast<int>(iter - rheaders_begin());
            beginRemoveColumns(QModelIndex(), cloumn, cloumn);
            iter = rheaders.erase(iter);
            endRemoveColumns();
        }
        removed = indexes.size();
        tryDbBackup([&](const std::shared_ptr<GRecordRepository> &repo) {
            for (const int64_t id : dbIdsToRemove) {
                attachBackupFailureLogger(repo->deleteRecordAsync(id));
            }
        });
    });
    return removed;
}

auto AdifModel::snapshotAsync() const -> QFuture<AdifModelSnapshot> {
    return GLogConcurrent::makeFuture(
        [this]() -> AdifModelSnapshot {
            AdifModelSnapshot snap;
            snap.records.reserve(m_records.size());
            for (const auto &r : m_records) {
                snap.records.emplace_back(r.clone());
            }
            snap.columnHeaders = rheaders;
            return snap;
        },
        GLogConcurrent::MainThreadExecutor{});
}

auto AdifModel::findNext(const QModelIndex &start, const Condition &condition,
                         const std::string &field) const -> QModelIndex {
    auto ret = QModelIndex();
    if (m_records.empty()) {
        return ret;
    }
    int row = start.isValid() ? start.row() : 0;
    auto record_count = int(m_records.size());
    bool finded = false;
    for (int i = 0; i <= record_count && !finded; ++i, ++row) {
        row %= record_count;
        const auto &record = m_records.at(row);
        if (field.empty()) {
            int startColumn = 0;
            if (i == 0) {
                startColumn = start.isValid() ? start.column() : 0;
            }
            for (int j = startColumn; j < rheaders.size(); ++j) {
                auto iter = record.find(rheaders[j]);
                if (iter != record.end() && condition(iter->second->get())) {
                    ret = index(row, j);
                    finded = true;
                    break;
                }
            }
        } else {
            auto iter = record.find(field);
            if (iter != record.end() && condition(iter->second->get())) {
                int column = 0;
                for (; column < rheaders.size(); ++column) {
                    if (rheaders[column] == field) {
                        break;
                    }
                }
                ret = index(row, column);
                finded = true;
            }
        }
    }
    return ret;
}

auto AdifModel::findAll(const Condition &condition, const std::string &field) const -> QList<int> {
    QList<int> rows;
    if (m_records.empty()) {
        return rows;
    }
    for (int i = 0; i < m_records.size(); ++i) {
        const auto &record = m_records[i];
        if (field.empty()) {
            for (const auto &h : rheaders) {
                auto iter = record.find(h);
                if (iter != record.end() && condition(iter->second->get())) {
                    rows.append(i);
                    break;
                }
            }
        } else {
            auto iter = record.find(field);
            if (iter != record.end() && condition(iter->second->get())) {
                rows.append(i);
            }
        }
    }
    return rows;
};
