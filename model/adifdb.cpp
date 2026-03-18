#include "adifdb.h"
#include "Concurrent.h"
#include "fccdb.h"
#include <QFileInfo>
#include <QUrl>
#include <QDir>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QProcess>
#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QMessageBox>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <type_traits>

#define THIS_CONNECT_Q(signal, slot) QObject::connect(this, &std::remove_reference_t<decltype(*this)>::signal, this, &std::remove_reference_t<decltype(*this)>::slot, Qt::QueuedConnection)

void AdifModel::newViewWithRows(QModelIndexList indexes)
{
    auto mineData = mimeData(indexes);
    auto text = mineData->data("text/plain");
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        tempFile.write(text);
        tempFile.flush();
        auto tempFilePath = tempFile.fileName();
        tempFile.close();
        auto program = QCoreApplication::applicationFilePath();
        QStringList arguments;
        arguments << "-i" << tempFilePath << "--remove";
        QProcess::startDetached(program, arguments);
    }
}

void AdifModel::pasteRows(const QMimeData *mimeData)
{
    dropMimeData(mimeData, Qt::DropAction::CopyAction, -1, -1, QModelIndex());
}

void AdifModel::copyRows(const QModelIndexList indexes)
{
    auto mimeData = new QMimeData();
    mimeData->setData("text/plain", this->mimeData(indexes)->data("text/plain"));
    emit setCilpboard(mimeData); 
}

void AdifModel::findNextS(QModelIndex current, QString key, QString value, bool isReg)
{
    auto sv = value.toStdString();
    auto sk = key.toStdString();
    if (!isReg) {
        auto index = this->findNext(current, [=](const std::string& v) { return v.find(sv) != std::string::npos; }, sk);
        emit foundNext(index);
        return;
    }
    QRegularExpression re(value);
    auto match = [=](const std::string& v) { return re.match(QString::fromStdString(v)).hasMatch(); };
    auto index = this->findNext(current, match, sk);
    emit foundNext(index);
}

void AdifModel::selectAll(QString key, QString value, bool isReg)
{
    auto sv = value.toStdString();
    auto sk = key.toStdString();
    if (!isReg) {
        auto rows = this->findAll([=](const std::string& v) { return v.find(sv) != std::string::npos; }, sk);
        emit selectRows(rows);
        return;
    }
    QRegularExpression re(value);
    auto match = [=](const std::string& v) { return re.match(QString::fromStdString(v)).hasMatch(); };
    auto rows = this->findAll(match, sk);
    emit selectRows(rows);
}

void AdifModel::deselectAll(QString key, QString value, bool isReg)
{
    auto sv = value.toStdString();
    auto sk = key.toStdString();
    if (!isReg) {
        auto rows = this->findAll([=](const std::string& v) { return v.find(sv) != std::string::npos; }, sk);
        emit deselectRows(rows);
        return;
    }
    QRegularExpression re(value);
    auto match = [=](const std::string& v) { return re.match(QString::fromStdString(v)).hasMatch(); };
    auto rows = this->findAll(match, sk);
    emit deselectRows(rows);
}

bool AdifModel::addHeader(const std::string &header)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    return _addHeader(header);
}

bool AdifModel::_addHeader(const std::string &header)
{
    auto& h = header;
    if (sheaders.find(h) != sheaders.end()) {
        return false;
    }
    auto iter = std::lower_bound((rheaders.begin() + sheaders.size()), rheaders.end(), h);
    if (iter != rheaders.end() && *iter == h) {
        return false;
    }
    beginInsertColumnsWrap(QModelIndex(), iter - (rheaders.begin() + sheaders.size()), iter - (rheaders.begin() + sheaders.size()));
    rheaders.insert(iter, h);
    endInsertColumnsWrap();
    return true;
}

std::string AdifModel::_records2StdString(const std::set<int> &rows) const
{
    std::stringstream stream {};
    for (auto& row : rows) {
        if (0 <= row && row < records.size()) {
            stream << records[row] << "\n";
        }
    }
    return stream.str();
}

std::vector<std::string> AdifModel::getDefaultSortModel(const std::string &field)
{
    std::vector<std::string> ret {field};
    if (field == "qso_date") {
        ret.push_back("time_on");
        return ret;
    }
    if (field == "time_on") {
        ret.push_back("qso_date");
        return ret;
    }
    ret.push_back("qso_date");
    ret.push_back("time_on");
    return ret;
}

void AdifModel::mapCallSignInView(bool keepOrigin)
{
    if (records.empty()) {
        return;
    }
    std::string testField = "z_glog_resolved_1";
    addHeader(testField);
    auto ctydb = CtyDB::instance();
    emit mapCallSignInViewBegin();
    std::shared_lock<decltype(ctydb->mutex)> lock0(ctydb->mutex);
    std::unique_lock<decltype(mutex)> lock1(mutex);
    
    int failCount = 0;
    int confictCount = 0;
    auto writeByKeepOriginFlag = [&](std::string& dest, const std::string& s) {
        if (!dest.empty() && dest != s) {
            ++confictCount;
            if (keepOrigin)
                return;
        }
        dest = s;
    };
    
    QString buf;
    for (auto& record : records) {
        auto & call = record["call"];
        buf = QString::fromUtf8(call.data(), call.size());
        ctydb->normalizeCallSign(buf);
        auto result = ctydb->lookUpCallSign(QStringView(buf));
        if (result.first->vaild) {
            record[testField] = (result.first->name + result.second).toStdString();
            // CQZ, ITUZ, CONT, COUNTRY
            writeByKeepOriginFlag(record["cqz"], QString::number(result.first->cq).toStdString());
            writeByKeepOriginFlag(record["ituz"], QString::number(result.first->itu).toStdString());
            writeByKeepOriginFlag(record["cont"], result.first->continent.toStdString());
            writeByKeepOriginFlag(record["country"], result.first->name.toStdString());
        }
        else {
            record[testField] = result.second.toStdString();
            ++failCount;
        }
    }

    lock1.unlock();
    emit layoutChanged();
    emit mapCallSignInViewEnd(failCount, confictCount);
}

void AdifModel::_clear()
{
    records.clear();
    while (rheaders.size() > GRecord::RESOLVE_HEADERS_COUNT) {
        rheaders.pop_back();
    }
}

AdifModel::AdifModel(QObject *parent) : QAbstractTableModel(parent)
{
    for (int i = 0; i < GRecord::RESOLVE_HEADERS_COUNT; ++i) {
        rheaders.push_back(GRecord::RESOLVE_HEADERS[i]);
        sheaders.insert(GRecord::RESOLVE_HEADERS[i]);
    }
    // control = new AdifModelC(this);
    THIS_CONNECT_Q(beginInsertColumnsWrap, beginInsertColumnsWrapS);
    THIS_CONNECT_Q(endInsertColumnsWrap, endInsertColumnsWrapS);
    THIS_CONNECT_Q(beginInsertRowsWrap, beginInsertRowsWrapS);
    THIS_CONNECT_Q(endInsertRowsWrap, endInsertRowsWrapS);
    // THIS_CONNECT_Q(beginRemoveRowsWrap, beginRemoveRowsWrapS);
    // THIS_CONNECT_Q(endRemoveRowsWrap, endRemoveRowsWrapS);
    // THIS_CONNECT_Q(beginRemoveColumnsWrap, beginRemoveColumnsWrapS);
    // THIS_CONNECT_Q(endRemoveColumnsWrap, endRemoveColumnsWrapS);
    THIS_CONNECT_Q(beginResetModelWrap, beginResetModelWrapS);
    THIS_CONNECT_Q(endResetModelWrap, endResetModelWrapS);
}

// AdifModelC *AdifModel::getControl()
// {
//     return control;
// }

int AdifModel::rowCount(const QModelIndex &parent) const
{
    return records.size();
}

int AdifModel::columnCount(const QModelIndex &parent) const
{
    return rheaders.size();
}

QVariant AdifModel::data(const QModelIndex &index, int role) const
{
    auto& m = const_cast<decltype(mutex)&>(mutex);
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    std::shared_lock<decltype(mutex)> lock(m, std::defer_lock);
    if (lock.try_lock()) {
        QVariant ret{};
        if (index.column() < rheaders.size()
            && index.row() < records.size()
            )  {
            auto& k = rheaders.at(index.column());
            auto& r = records.at(index.row());
            auto iter = r.find(k);
            if (iter != r.end()) {
                return QString::fromStdString(iter->second);
            }
        }
    }

    return QVariant();
}

QVariant AdifModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    auto& m = const_cast<decltype(mutex)&>(mutex);
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal) {
        QVariant ret{};
        std::shared_lock<decltype(mutex)> lock(m, std::defer_lock);
        if (lock.try_lock()) {
            if (section < rheaders.size()) {
                return QString::fromStdString(rheaders.at(section));
            }
        }
    } 
    return section + 1;
}

bool AdifModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        std::unique_lock<decltype(mutex)> lock(mutex);
        auto v = value.toString().toStdString();
        if (index.row() < records.size() && index.column() < rheaders.size()) {
            records[index.row()][rheaders[index.column()]] = v;
            lock.unlock();
            dataChanged(index, index);
            return true;
        }
    }

    return false;
}

Qt::ItemFlags AdifModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

Qt::DropActions AdifModel::supportedDropActions() const
{
    return QAbstractTableModel::supportedDropActions() | Qt::DropAction::MoveAction;
}

QStringList AdifModel::mimeTypes() const
{
    auto ret = QAbstractTableModel::mimeTypes();
    ret.append("text/plain");
    ret.append("text/uri-list");
    return ret;
}

QMimeData *AdifModel::mimeData(const QModelIndexList &indexes) const
{
    auto& m = const_cast<decltype(mutex)&>(mutex);
    std::shared_lock<decltype(mutex)> lock(m);
    QMimeData *mimeData = QAbstractTableModel::mimeData(indexes);
    std::set<int> rows;
    for (auto& index : indexes) {
        if (index.isValid()) {
            rows.insert(index.row());
        }
    }
    mimeData->setText(QString::fromStdString(_records2StdString(rows)));
    return mimeData;
}

bool AdifModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    while (data->hasText()) {
        if ((0 <= row && row <= records.size())
            || row == -1 && column == -1
            ) {
            auto s = data->text().toStdString();
            std::stringstream stream(s);
            driver.switch_streams(stream, std::cerr);
            if (!parser.parse()) {
                insertRecords(row, driver.data.rbegin(), driver.data.rend());
                return true;
            }
        }
        break;
    }
    while (data->hasUrls()) {
        auto urls = data->urls();
        if (urls.empty()) {
            break;
        }
        for (auto url = urls.rbegin(); url != urls.rend(); ++url) {
            QString file = url->toLocalFile();
            if (file.isEmpty()) {
                continue;
            }
            insertFile(row, file);
        }
        break;
    }
    return QAbstractTableModel::dropMimeData(data, action, row, column, parent);
}

bool AdifModel::insertRows(int row, int count, const QModelIndex &parent)
{
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

bool AdifModel::removeRows(int row, int count, const QModelIndex &parent)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    if (row >= 0 && count > 0 && row + count <= records.size()) {
        beginRemoveRows(parent, row, row + count - 1);
        records.erase(records.begin() + row, records.begin() + row + count);
        lock.unlock();
        endRemoveRows();
        return true;
    }
    return false;
}

bool AdifModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    if (! beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild)) {
        endMoveRows();
        return false;
    }
    if (destinationChild > sourceRow) {
        std::rotate(records.begin() + sourceRow, records.begin() + sourceRow + count, records.begin() + destinationChild);
    }
    if (destinationChild < sourceRow) {
        std::rotate(records.begin() + destinationChild, records.begin() + sourceRow, records.begin() + sourceRow + count);
    }
    lock.unlock();
    endMoveRows();
    return true;
}

void AdifModel::sort(int column, Qt::SortOrder order)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    if (0 <= column && column < rheaders.size()) {
        auto& col = rheaders[column];
        std::vector<std::string> fields = getDefaultSortModel(col);
        auto less = [=](const decltype(records)::value_type& a, const decltype(records)::value_type& b)->bool {
            return GRecord::less(a, b, fields);
        };
        beginResetModel();
        if (order == Qt::AscendingOrder) {
            std::sort(records.begin(), records.end(), [=](const decltype(records)::value_type& a, const decltype(records)::value_type& b)->bool {
                return less(a, b);
            });
        } else {
            std::sort(records.begin(), records.end(), [=](const decltype(records)::value_type& a, const decltype(records)::value_type& b)->bool {
                return less(b, a);
            });
        }
        lock.unlock();
        endResetModel();
    }
    
}

AdifModel::~AdifModel()
{
}

void AdifModel::clear()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    beginResetModel();
    _clear();
    lock.unlock();
    endResetModel();
}

std::string AdifModel::records2StdString(const std::set<int> &rows) const
{
    auto& m = const_cast<decltype(mutex)&>(mutex);
    std::shared_lock<decltype(mutex)> lock(m);
    return _records2StdString(rows);
}

void AdifModel::toCsv(std::ostream &stream) const
{
    auto& m = const_cast<decltype(mutex)&>(mutex);
    std::shared_lock<decltype(mutex)> lock(m);
    _toCsv(stream);
}

void AdifModel::toAdif(std::ostream &stream) const
{
    auto& m = const_cast<decltype(mutex)&>(mutex);
    std::shared_lock<decltype(mutex)> lock(m);
    _toAdif(stream);
}

AdifModel::AwardRes AdifModel::diffEntNameCountForAward() const
{
    AwardRes res;
    auto ctydb = CtyDB::instance();
    std::shared_lock<decltype(ctydb->mutex)> lock0(ctydb->mutex);
    auto& m = const_cast<decltype(mutex)&>(mutex);
    std::shared_lock<decltype(mutex)> lock1(m);
    if (records.empty()) {
        return res;
    }
    std::map<QString, int> counterDXCC;
    std::map<QString, int> counterWAC_ARRL;
    std::map<QString, int> counterWAC_NOTARRL;
    std::map<int, int> counterCQZ;
    std::map<QString, int> counterWAS;
    auto fccdb = FccDB::instance();
    bool WASSearchVaild = fccdb->beginSearch();
    QString buf;
    for (auto & record : records) {
        auto & call = record.at("call");
        buf = QString::fromUtf8(call.data(), call.size());
        ctydb->normalizeCallSign(buf);
        auto result = ctydb->lookUpCallSign(QStringView(buf));
        if (result.first->vaild && result.first->ARRL_sponsored) {
            ++counterDXCC[result.first->name];
        }
        if (result.first->vaild && result.first->ARRL_sponsored) {
            ++counterWAC_ARRL[result.first->continent];
        }
        if (result.first->vaild) {
            ++counterWAC_NOTARRL[result.first->continent];
        }
        if (result.first->vaild) {
            ++counterCQZ[result.first->cq];
        }
        auto state = fccdb->lookupState(buf);
        if (state.size() == 2) {
            ++counterWAS[state];
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

void AdifModel::_toCsv(std::ostream &stream) const
{
    auto iter1 = rheaders.begin();
    while (true) {
        stream << *iter1;
        ++iter1;
        if (iter1 == rheaders.end()) {
            break;
        }
        stream << ',';
    }
    stream << "\n";

    auto iter2 = records.begin();
    while (true) {
        auto record = *iter2;
        for (auto & func : GRecordOutputFilters) {
            func(record);
        }
        for (auto h = rheaders.begin(); h != rheaders.end(); ++h) {
            if (h != rheaders.begin()) {
                stream << ',';
            }
            auto iter3 = record.find(*h);
            if (iter3 == record.end()) {
                continue;
            }
            stream << iter3->second;
        }
        ++iter2;
        if (iter2 == records.end()) {
            break;
        }
        stream << "\n";
    }
}

void AdifModel::_toAdif(std::ostream &stream) const
{
    for (auto& record : records) {
        stream << record << "\n";
    }
}

void AdifModel::openFile(QString filename)
{
    auto future = GLogConcurrent::makeFuture([=]() {
        QFileInfo file(filename);
        std::ifstream in(file.filesystemAbsoluteFilePath());
        bool parse_res = false;
        if (in) {
            driver.switch_streams(in, std::cerr);
            parse_res = !parser.parse();
            if (parse_res) {
                std::unique_lock<std::shared_mutex> lock(mutex);
                beginResetModelWrap();
                _setRecords(driver.data.rbegin(), driver.data.rend());
                endResetModelWrap();
            }  
        }
        in.close();
        return parse_res;
    });
    future.then([=](decltype(future) future){
        auto open_result = future.result();
    });
}

void AdifModel::appendFile(QString filename, bool remove)
{
    auto future = GLogConcurrent::makeFuture([=]() {
        QFileInfo file(filename);
        std::ifstream in(file.filesystemAbsoluteFilePath());
        bool parse_res = false;
        if (in) {
            driver.switch_streams(in, std::cerr);
            parse_res = !parser.parse();
            if (parse_res) {
                addRecords(driver.data.rbegin(), driver.data.rend());
            }  
        }
        in.close();
        if (remove) {
            QFile::remove(QFileInfo(filename).absoluteFilePath());
        }
        return parse_res;
    });
    future.then([=](decltype(future) future){
        auto open_result = future.result();
    });
}

void AdifModel::insertFile(int row, QString filename)
{
    auto future = GLogConcurrent::makeFuture([=](QPromise<bool> & p) {
        QFileInfo file(filename);
        std::ifstream in(file.filesystemAbsoluteFilePath());
        bool parse_res = false;
        if (in) {
            driver.switch_streams(in, std::cerr);
            parse_res = !parser.parse();
            if (parse_res) {
                insertRecords(row, driver.data.rbegin(), driver.data.rend());
            }  
        }
        in.close();
        p.addResult(parse_res);
    });
    future.then([=](decltype(future) future){
        auto open_result = future.result();
    });
}

void AdifModel::saveAs(QString filename) const
{
    if (records.empty()) {
        return;
    }
    auto future = GLogConcurrent::makeFuture([=]() {
        QFileInfo file(filename);
        std::ofstream out(file.filesystemAbsoluteFilePath());
        if (out) {
            if (filename.endsWith(".csv")) {
                toCsv(out);
                out.close();
                emit saveDone();
                return true;
            }
            if (filename.endsWith(".adi") || filename.endsWith(".adif")) {
                toAdif(out);
                out.close();
                emit saveDone();
                return true;
            }
        }
        out.close();
        return false;
    });
    future.then([=](decltype(future) future){

    });
}

void AdifModel::sortSelectedColumn(int column, Qt::SortOrder order)
{
    sort(column, order);
}

void AdifModel::removeSelectedColumn(int column)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    if (column < sheaders.size() || column >= rheaders.size()) {
        return;
    }
    beginRemoveColumns(QModelIndex(), column, column);
    auto field = rheaders.at(column);
    for (auto& record : records) {
        auto iter = record.find(field);
        if (iter != record.end()) {
            record.erase(iter);
        }
    }
    rheaders.erase(rheaders.begin() + column);
    lock.unlock();
    endRemoveColumns();
}

void AdifModel::deleteRows(QModelIndexList indexes)
{
    if (indexes.empty()) {
        return;
    }
    std::sort(indexes.begin(), indexes.end(), [=](const QModelIndex& a, const QModelIndex& b) { return a.row() < b.row(); });
    std::unique_lock<decltype(mutex)> lock(mutex);
    auto idx = indexes.size() - 1;
    int deletebegin = indexes.at(idx).row();
    int deleteend = deletebegin + 1;
    while (true) {
        if (idx > 0 && indexes.at(idx - 1).row() >= 0 && indexes.at(idx - 1).row() == deletebegin - 1) {
            --deletebegin;
            --idx;
            continue;
        }
        beginRemoveRows(QModelIndex(), deletebegin, deleteend - 1);
        records.erase(records.begin() + deletebegin, records.begin() + deleteend);
        lock.unlock();
        endRemoveRows();
        lock.lock();
        if (idx == 0) {
            break;
        }
        --idx;
        deletebegin = indexes.at(idx).row();
        deleteend = deletebegin + 1;
    }
    auto iter = (rheaders.begin() + sheaders.size());
    while (iter != rheaders.end()) {
        bool vaild = false;
        for (auto& record : records) {
            if (record.find(*iter) != record.end()) {
                vaild = true;
                break;
            }
        }
        if (vaild) {
            ++iter;
            continue;
        }
        beginRemoveColumns(QModelIndex(), iter - (rheaders.begin() + sheaders.size()), iter - (rheaders.begin() + sheaders.size()));
        iter = rheaders.erase(iter);
        lock.unlock();
        endRemoveColumns();
        lock.lock();
    }
}

void AdifModel::beginInsertColumnsWrapS(QModelIndex parent, int first, int last)
{
    beginInsertColumns(parent, first, last);
}

void AdifModel::endInsertColumnsWrapS()
{
    endInsertColumns();
}

void AdifModel::beginInsertRowsWrapS(QModelIndex parent, int first, int last)
{
    beginInsertRows(parent, first, last);
}

void AdifModel::endInsertRowsWrapS()
{
    endInsertRows();
}

// void AdifModel::beginRemoveRowsWrapS(QModelIndex parent, int first, int last)
// {
//     beginRemoveRows(parent, first, last);
// }

// void AdifModel::endRemoveRowsWrapS()
// {
//     endRemoveRows();
// }

// void AdifModel::beginRemoveColumnsWrapS(QModelIndex parent, int first, int last)
// {
//     beginRemoveColumns(parent, first, last);
// }

// void AdifModel::endRemoveColumnsWrapS()
// {
//     endRemoveColumns();
// }

void AdifModel::beginResetModelWrapS()
{
    beginResetModel();
}

void AdifModel::endResetModelWrapS()
{
    endResetModel();
}
