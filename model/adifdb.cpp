#include "adifdb.h"
#include <QApplication>
#include <QByteArrayView>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <type_traits>
#include "CaseInsensitiveLess.h"
#include "Concurrent.h"
#include "fccdb.h"

#ifdef _WIN32
#include <io.h>
#define IS_PIPED (!_isatty(_fileno(stdin)))
#else
#include <unistd.h>
#define IS_PIPED (!isatty(fileno(stdin)))
#endif

static QTextStream &operator<<(QTextStream &stream, const std::string &str) {
    stream << QString::fromStdString(str);
    return stream;
}

static QTextStream &operator<<(QTextStream &stream, std::string_view sv) {
    stream << QString::fromUtf8(sv);
    return stream;
}

template <typename Ostream>
void AdifModel::_toCsv(Ostream &stream, std::vector<Record> p_records,
                       std::vector<std::string> p_rheaders) {
    auto iter1 = p_rheaders.begin();
    while (true) {
        stream << (*iter1);
        ++iter1;
        if (iter1 == p_rheaders.end()) {
            break;
        }
        stream << ',';
    }
    stream << "\n";

    auto iter2 = p_records.begin();
    while (true) {
        auto &record = *iter2;
        for (auto h = p_rheaders.begin(); h != p_rheaders.end(); ++h) {
            if (h != p_rheaders.begin()) {
                stream << ',';
            }
            auto iter3 = record.find(*h);
            if (iter3 == record.end()) {
                continue;
            }
            stream << iter3->second;
        }
        ++iter2;
        if (iter2 == p_records.end()) {
            break;
        }
        stream << "\n";
    }
}

template <typename Ostream>
void AdifModel::_toAdif(Ostream &stream, std::vector<Record> p_records) {
    for (const auto &record : p_records) {
        stream << record << "\n";
    }
}

void AdifModel::newViewWithRows(const QModelIndexList &indexes) const {
    auto *mineData = mimeData(indexes);
    GLogConcurrent::makeFuture([text = mineData->data("text/plain")]() {
        QProcess *process = new QProcess();
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

auto AdifModel::addHeader(std::string header) -> QFuture<bool> {
    return GLogConcurrent::makeFuture(
        [this, header = std::move(header)]() { return _addHeader(std::move(header)); },
        GLogConcurrent::MainThreadExecutor{});
}

auto AdifModel::_addHeader(std::string header) -> bool {
    if (sheaders.count(header) != 0) {
        return false;
    }
    auto iter = std::lower_bound(rheaders_begin(), rheaders.end(), header);
    if (iter != rheaders.end() && *iter == header) {
        return false;
    }
    auto column = static_cast<int>(iter - rheaders_begin());
    beginInsertColumns(QModelIndex(), column, column);
    rheaders.insert(iter, std::move(header));
    endInsertColumns();
    return true;
}

auto AdifModel::_records2StdString(const std::set<int> &rows) const -> std::string {
    std::stringstream stream{};
    for (const auto &row : rows) {
        if (0 <= row && row < records.size()) {
            stream << records[row] << "\n";
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
    if (records.empty()) {
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
    for (auto &record : records) {
        auto call = record.at("call")->get();
        buf = QString::fromUtf8(call.data(), qsizetype(call.size()));
        CtyDB::normalizeCallSign(buf);
        auto result = ctydb->lookUpCallSign(QStringView(buf));
        if (result.first->valid) {
            record.addOrSetPair(testField, (result.first->name + result.second).toStdString());
            // CQZ, ITUZ, CONT, COUNTRY
            writeByKeepOriginFlag(record, "cqz", QString::number(result.first->cq).toStdString());
            writeByKeepOriginFlag(record, "ituz", QString::number(result.first->itu).toStdString());
            writeByKeepOriginFlag(record, "cont", result.first->continent.toStdString());
            writeByKeepOriginFlag(record, "country", result.first->name.toStdString());
        } else {
            record.addOrSetPair(testField, result.second.toStdString());
            ++failCount;
        }
    }

    emit layoutChanged();
    emit mapCallSignInViewEnd(failCount, confictCount);
}

void AdifModel::_clear() {
    records.clear();
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

auto AdifModel::rowCount(const QModelIndex &parent) const -> int { return int(records.size()); }

auto AdifModel::columnCount(const QModelIndex &parent) const -> int { return int(rheaders.size()); }

auto AdifModel::data(const QModelIndex &index, int role) const -> QVariant {
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }

    if (!index.isValid()) {
        return {};
    }

    if (index.column() < rheaders.size() && index.row() < records.size()) {
        const auto &key = rheaders.at(index.column());
        const auto &row = records.at(index.row());
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
        if (index.row() < records.size() && index.column() < rheaders.size()) {
            auto &field = rheaders[index.column()];
            if (field == "qso_date" || field == "time_on") {
                return false;
            }
            auto res = records[index.row()].addOrSetPair(field, v);
            dataChanged(index, index);
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

    if (data->hasText()) {
        std::string s = data->text().toStdString();
        insertStringDataAsync(beginRow, std::move(s))
            .then(this,
                  [](std::vector<std::string> errors) {

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
                insertFileAsync(beginRow, file)
                    .then(this,
                          [](std::vector<std::string> errors) {

                          })
                    .onFailed(this, [](const std::exception &e) {

                    });
                handled = true;
            }
        }
    }

    return handled;
}

auto AdifModel::insertRows(int row, int count, const QModelIndex &parent) -> bool {
    // std::unique_lock<decltype(mutex)> lock(mutex);
    // if (count > 0 && row >= 0 && row <= records.size()) {
    //     beginInsertRows(parent, row, row + count - 1);
    //     while (count--) {
    //         records.insert(records.begin() + row, decltype(records)::value_type{});
    //     }
    //     endInsertRows();
    // }
    return false;
}

auto AdifModel::removeRows(int row, int count, const QModelIndex &parent) -> bool {
    if (row >= 0 && count > 0 && row + count <= records.size()) {
        beginRemoveRows(parent, row, row + count - 1);
        records.erase(records.begin() + row, records.begin() + row + count);
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
        std::rotate(records.begin() + sourceRow, records.begin() + sourceRow + count,
                    records.begin() + destinationChild);
    }
    if (destinationChild < sourceRow) {
        std::rotate(records.begin() + destinationChild, records.begin() + sourceRow,
                    records.begin() + sourceRow + count);
    }
    endMoveRows();
    return true;
}

void AdifModel::sort(int column, Qt::SortOrder order) {
    if (0 <= column && column < rheaders.size()) {
        auto &col = rheaders[column];
        std::vector<std::string> fields = getDefaultSortModel(col);
        auto less = [=](const decltype(records)::value_type &a,
                        const decltype(records)::value_type &b) -> bool {
            return GRecord::less(a, b, fields);
        };
        beginResetModel();
        if (order == Qt::AscendingOrder) {
            std::sort(records.begin(), records.end(),
                      [=](const decltype(records)::value_type &a,
                          const decltype(records)::value_type &b) -> bool { return less(a, b); });
        } else {
            std::sort(records.begin(), records.end(),
                      [=](const decltype(records)::value_type &a,
                          const decltype(records)::value_type &b) -> bool { return less(b, a); });
        }
        endResetModel();
    }
}

AdifModel::~AdifModel() = default;

void AdifModel::clear() {
    QMetaObject::invokeMethod(
        this,
        [this]() mutable {
            beginResetModel();
            _clear();
            endResetModel();
        },
        Qt::AutoConnection);
}

auto AdifModel::records2StdString(const std::set<int> &rows) const -> std::string {
    return _records2StdString(rows);
}

void AdifModel::toCsv(std::ostream &stream) const { _toCsv(stream, records, rheaders); }

void AdifModel::toAdif(std::ostream &stream) const { _toAdif(stream, records); }

auto AdifModel::diffEntNameCountForAward() const -> AdifModel::AwardRes {
    AwardRes res;
    auto *ctydb = CtyDB::instance();
    std::shared_lock<decltype(ctydb->mutex)> lock0(ctydb->mutex);
    if (records.empty()) {
        return res;
    }
    std::map<QString, int> counterDXCC;
    std::map<QString, int> counterWAC_ARRL;
    std::map<QString, int> counterWAC_NOTARRL;
    std::map<int, int> counterCQZ;
    std::map<QString, int> counterWAS;
    auto *fccdb = FccDB::instance();
    bool WASSearchVaild = fccdb->beginSearch();
    QString buf;
    for (const auto &record : records) {
        auto call = record.at("call")->get();
        buf = QString::fromUtf8(call.data(), qsizetype(call.size()));
        CtyDB::normalizeCallSign(buf);
        auto result = ctydb->lookUpCallSign(QStringView(buf));
        if (result.first->valid && result.first->ARRL_sponsored) {
            ++counterDXCC[result.first->name];
        }
        if (result.first->valid && result.first->ARRL_sponsored) {
            ++counterWAC_ARRL[result.first->continent];
        }
        if (result.first->valid) {
            ++counterWAC_NOTARRL[result.first->continent];
        }
        if (result.first->valid) {
            ++counterCQZ[result.first->cq];
        }
        if (WASSearchVaild) {
            auto state = fccdb->lookupState(buf);
            if (state.size() == 2) {
                ++counterWAS[state];
            }
        }
    }
    if (WASSearchVaild) {
        fccdb->endSearch();
        res.WAS = QString::number(counterWAS.size());
    }
    res.DXCC = QString::number(counterDXCC.size());
    res.WAC_ARRL = QString::number(counterWAC_ARRL.size());
    res.WAC_NOTARRL = QString::number(counterWAC_NOTARRL.size());
    res.CQZ = QString::number(counterCQZ.size());
    return res;
}

auto AdifModel::deleteRowsPersistent(std::vector<QPersistentModelIndex> indexes) -> size_t {
    QModelIndexList rows;
    for (auto &index : indexes) {
        if (index.isValid()) {
            rows.append(index);
        }
    }
    return deleteRows(std::move(rows));
}

bool AdifModel::mergeRows(QModelIndex major, QModelIndexList indexes) {
    if (!major.isValid() || indexes.size() == 0) {
        return false;
    }
    std::vector<Record> merged_records;
    merged_records.reserve(indexes.size());
    for (auto &index : indexes) {
        if (!index.isValid()) {
            continue;
        }
        merged_records.push_back(records[index.row()]);
    }
    auto major_row = major.row();
    if (!records[major_row].merge(merged_records)) {
        return false;
    }
    QMetaObject::invokeMethod(this, [this, major_row]() mutable {
        emit dataChanged(createIndex(major_row, 0), createIndex(major_row, columnCount()));
    });
    return true;
}

QFuture<bool> AdifModel::mergeRowsAsync(QModelIndex major, QModelIndexList indexes) {
    return GLogConcurrent::makeFuture(
        [this, major, indexes]() -> bool { return mergeRows(major, indexes); }, m_fifo_exec);
}

QFuture<size_t> AdifModel::deleteRowsAsync(QModelIndexList indexes) {
    return GLogConcurrent::makeFuture([this, indexes]() -> size_t { return deleteRows(indexes); },
                                      m_fifo_exec);
}

auto AdifModel::fromRawData(bool success,
                            std::vector<std::vector<std::pair<std::string, std::string>>> raw_data)
    -> ParseRes {
    ParseRes ret{success, {}, {}};
    if (!success) {
        return ret;
    }
    auto &m_records = ret.parse_records;
    auto &m_headers = ret.parse_headers;
    m_records.reserve(raw_data.size());
    for (auto &row : raw_data) {
        if (row.empty()) {
            continue;
        }
        Record m_record;
        bool valid = false;
        for (auto &[key, value] : row) {
            bool inserted = m_record.addOrSetPairAndNormalizeKey(key, std::move(value));
            if (inserted) {
                valid = true;
                m_headers.insert(std::move(key));
                continue;
            }
        }
        if (!valid) {
            continue;
        }
        m_records.push_back(std::move(m_record));
    }
    return ret;
}

auto AdifModel::parse(std::istream &in, std::vector<std::string> &errors, ParseMode mode)
    -> ParseRes {

    std::unique_ptr<GLOG_PARSER::DriverUnsynchronized> driver;

    switch (mode) {
    case ParseMode::Batch:
        driver = std::make_unique<GLOG_PARSER::DriverUnsynchronized>(
            std::make_unique<GLOG_PARSER::LexerBatch>());
        break;

    case ParseMode::Interactive:
        driver = std::make_unique<GLOG_PARSER::DriverUnsynchronized>(
            std::make_unique<GLOG_PARSER::LexerInteractive>());
        break;

    default:
        assert(false && "Invalid mode");
        break;
    }

    GLOG_PARSER::Parser parser{driver.get()};
    driver->switch_streams(in, std::cerr);
    auto success = parser.parse() == 0;

    ParseRes ret{success, {}, {}};
    if (!success) {
        return ret;
    }

    ret = driver->data.write([&](auto &data) { return fromRawData(success, std::move(data)); });
    driver->errors.write([&](auto &d_errors) { std::swap(errors, d_errors); });

    return ret;
}

void AdifModel::_resetRecords(std::vector<Record> new_record,
                              std::unordered_set<std::string> new_headers) {
    QMetaObject::invokeMethod(
        this,
        [this, new_record = std::move(new_record), new_headers = std::move(new_headers)]() mutable {
            beginResetModel();
            _clear();
            records = std::move(new_record);
            for (auto &header : new_headers) {
                _addHeader(std::move(header));
            }
            endResetModel();
        },
        Qt::AutoConnection);
}

void AdifModel::_insertRecords(int where, std::vector<Record> new_record,
                               std::unordered_set<std::string> new_headers) {

    QMetaObject::invokeMethod(
        this,
        [this, where, new_record = std::move(new_record),
         new_headers = std::move(new_headers)]() mutable {
            for (auto &header : new_headers) {
                _addHeader(std::move(header));
            }
            if (!new_record.empty()) {
                int row = (where < 0 || where > (int)records.size()) ? (int)records.size() : where;
                beginInsertRows(QModelIndex(), row, row + (int)new_record.size() - 1);
                records.insert(records.begin() + row, std::make_move_iterator(new_record.begin()),
                               std::make_move_iterator(new_record.end()));
                endInsertRows();
            }
        },
        Qt::AutoConnection);
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

auto AdifModel::openFile(const QString &filename, bool remove) -> std::vector<std::string> {
    QFileInfo file(filename);
    std::ifstream in(file.filesystemAbsoluteFilePath());
    if (!in) {
        throw std::runtime_error("Can not open file");
    }
    std::vector<std::string> errors;
    auto [parse_res, result_records, result_headers] = parse(in, errors, ParseMode::Batch);
    if (parse_res) {
        _resetRecords(std::move(result_records), std::move(result_headers));
    }
    in.close();
    if (remove) {
        QFile::remove(QFileInfo(filename).absoluteFilePath());
    }
    if (!parse_res) {
        throw std::runtime_error(errors.empty() ? "Unknown parse error" : errors.back());
    }
    return errors;
}

auto AdifModel::openFileAsync(const QString &filename, bool remove)
    -> QFuture<std::vector<std::string>> {
    return GLogConcurrent::makeFuture(
        [this, filename = filename, remove](QPromise<std::vector<std::string>> &promise) {
            promise.addResult(openFile(filename, remove));
        },
        m_fifo_exec);
}

auto AdifModel::insertFile(int row, const QString &filename, bool remove)
    -> std::vector<std::string> {
    QFileInfo file(filename);
    std::ifstream in(file.filesystemAbsoluteFilePath());
    if (!in) {
        throw std::runtime_error("Can not open file");
    }
    std::vector<std::string> errors;
    auto [parse_res, result_records, result_headers] = parse(in, errors, ParseMode::Batch);
    if (parse_res) {
        _insertRecords(row, std::move(result_records), std::move(result_headers));
    }
    in.close();
    if (remove) {
        QFile::remove(QFileInfo(filename).absoluteFilePath());
    }
    if (!parse_res) {
        throw std::runtime_error(errors.empty() ? "Unknown parse error" : errors.back());
    }
    return errors;
}

auto AdifModel::insertFileAsync(int where, const QString &filename, bool remove)
    -> QFuture<std::vector<std::string>> {
    return GLogConcurrent::makeFuture(
        [this, filename = filename, where, remove](QPromise<std::vector<std::string>> &promise) {
            promise.addResult(insertFile(where, filename, remove));
        },
        m_fifo_exec);
}

std::vector<std::string> AdifModel::insertStringData(int row, std::string data) {
    std::stringstream in(data);
    std::vector<std::string> errors;
    auto [parse_res, result_records, result_headers] = parse(in, errors, ParseMode::Batch);
    if (parse_res) {
        _insertRecords(row, std::move(result_records), std::move(result_headers));
    }
    if (!parse_res) {
        throw std::runtime_error(errors.empty() ? "Unknown parse error" : errors.back());
    }
    return errors;
}

auto AdifModel::findDuplicates(const std::vector<std::string> &fields) const
    -> std::vector<std::vector<QPersistentModelIndex>> {
    if (records.empty() || fields.empty()) {
        return {};
    }

    // 1. Grouping Phase
    auto m_less = [&](const Record &a, const Record &b) { return Record::less(a, b, fields); };
    // Key: Record, Value: List of row integers (saves memory over QModelIndex)
    std::map<Record, std::vector<int>, decltype(m_less)> groups(m_less);
    for (int i = 0; i < static_cast<int>(records.size()); ++i) {
        groups[records[i]].push_back(i);
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

auto AdifModel::insertStringDataAsync(int row, std::string data)
    -> QFuture<std::vector<std::string>> {
    return GLogConcurrent::makeFuture(
        [this, row, data = std::move(data)]() { return insertStringData(row, std::move(data)); },
        m_fifo_exec);
}

void AdifModel::saveAs(const QString &filename) const {
    if (records.empty()) {
        return;
    }
    std::vector<Record> p_records;
    p_records.reserve(records.size());
    for (const auto &record : records) {
        p_records.emplace_back(record.clone());
    }
    auto headersCopy = rheaders;
    GLogConcurrent::makeFuture(
        [filename = filename, p_records = std::move(p_records),
         p_rheaders = std::move(headersCopy)]() mutable {
            QSaveFile outFile(filename);
            if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                throw std::runtime_error("Cannot open file for writing");
            }
            QTextStream stream(&outFile);
            bool handled = false;
            if (filename.endsWith(".csv", Qt::CaseInsensitive)) {
                AdifModel::_toCsv(stream, std::move(p_records), std::move(p_rheaders));
                handled = true;
            }
            if (filename.endsWith(".adi", Qt::CaseInsensitive) ||
                filename.endsWith(".adif", Qt::CaseInsensitive)) {
                AdifModel::_toAdif(stream, std::move(p_records));
                handled = true;
            }
            if (!handled) {
                throw std::runtime_error("Unsupported file extension");
            }
            if (!outFile.commit()) {
                throw std::runtime_error("Failed to commit changes to file");
            }
        },
        m_fifo_exec)
        .then(QCoreApplication::instance(),
              []() { QMessageBox::information(nullptr, tr("Save"), tr("Succeed")); })
        .onFailed(QCoreApplication::instance(), [](const std::exception &e) {
            QMessageBox::critical(nullptr, tr("Save"), tr("Failed: ") + e.what());
        });
}

void AdifModel::sortSelectedColumn(int column, Qt::SortOrder order) { sort(column, order); }

void AdifModel::removeSelectedColumn(int column) {
    if (column < sheaders.size() || column >= rheaders.size()) {
        return;
    }
    beginRemoveColumns(QModelIndex(), column, column);
    auto field = rheaders.at(column);
    for (auto &record : records) {
        record.tryRemoveField(field);
    }
    rheaders.erase(rheaders.begin() + column);
    endRemoveColumns();
}

auto AdifModel::deleteRows(QModelIndexList indexes) -> size_t {
    indexes.erase(std::remove_if(indexes.begin(), indexes.end(),
                                 [](const QModelIndex &idx) { return !idx.isValid(); }),
                  indexes.end());
    if (indexes.empty()) {
        return 0;
    }
    std::sort(indexes.begin(), indexes.end(),
              [](const QModelIndex &a, const QModelIndex &b) { return a.row() < b.row(); });
    indexes.erase(
        std::unique(indexes.begin(), indexes.end(),
                    [](const QModelIndex &a, const QModelIndex &b) { return a.row() == b.row(); }),
        indexes.end());
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
        QMetaObject::invokeMethod(this, [this, deletebegin, deleteend]() mutable {
            emit beginRemoveRows(QModelIndex(), deletebegin, deleteend - 1);
        });
        records.erase(records.begin() + deletebegin, records.begin() + deleteend);
        QMetaObject::invokeMethod(this, [this]() mutable { emit endRemoveRows(); });
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
        for (auto &record : records) {
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
        QMetaObject::invokeMethod(this, [this, cloumn]() mutable {
            emit beginRemoveColumns(QModelIndex(), cloumn, cloumn);
        });
        iter = rheaders.erase(iter);
        QMetaObject::invokeMethod(this, [this]() mutable { emit endRemoveColumns(); });
    }
    return indexes.size();
}

QModelIndex AdifModel::findNext(const QModelIndex &start, Condition condition,
                                const std::string &field) const {
    auto ret = QModelIndex();
    if (records.empty()) {
        return ret;
    }
    int row = start.isValid() ? start.row() : 0;
    bool finded = false;
    for (int i = 0; i <= records.size() && !finded; ++i, ++row) {
        row %= records.size();
        auto &record = records.at(row);
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

QList<int> AdifModel::findAll(Condition condition, const std::string &field) const {
    QList<int> rows;
    if (records.empty()) {
        return rows;
    }
    for (int i = 0; i < records.size(); ++i) {
        auto &record = records[i];
        if (field.empty()) {
            for (auto &h : rheaders) {
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
