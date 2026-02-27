#include "adifdb.h"
#include <QFileInfo>
#include <QUrl>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <algorithm>

AdifModelC::AdifModelC(AdifModel *model, QObject *parent) : model(model), QObject(parent)
{
}

void AdifModelC::openFile(QString filename)
{
    model->openFile(filename);
    emit modelUpdated();
}

void AdifModelC::appendFile(QString filename)
{
    model->appendFile(filename);
    emit modelUpdated();
}

void AdifModelC::insertFile(int row, QString filename)
{
    model->insertFile(row, filename);
    emit modelUpdated();
}

bool AdifModel::addHeader(const std::string &header)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    return _addHeader(header);
}

bool AdifModel::_addHeader(const std::string &header)
{
    auto& h = header;
    auto iter = std::lower_bound(rheaders.begin(), rheaders.end(), h);
    if (iter != rheaders.end() && *iter == h) {
        return false;
    }
    beginInsertColumns(QModelIndex(), iter - rheaders.begin(), iter - rheaders.begin());
    rheaders.insert(iter, h);
    endInsertColumns();
    return true;
}

std::string AdifModel::_records2StdString(const std::set<int> &rows) const
{
    std::stringstream stream {};
    for (auto& row : rows) {
        if (0 <= row && row < records.size()) {
            stream << record2StdString(records[row]) << "<EOR>\n";
        }
    }
    return stream.str();
}

std::string AdifModel::record2StdString(const Record &record)
{
    std::stringstream stream {};
    for (auto& pair : record) {
        stream << '<';
        stream << pair.first;
        stream << ':';
        stream << pair.second.size();
        stream << '>';
        stream << pair.second;
    }
    return stream.str();
}

void AdifModel::_clear()
{
    records.clear();
    rheaders.clear();
}

AdifModel::AdifModel(QObject *parent) : QAbstractTableModel(parent)
{
    control = new AdifModelC(this);
}

AdifModelC *AdifModel::getControl()
{
    return control;
}

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

    if (m.try_lock_shared()) {
        QVariant ret{};
        if (index.column() < rheaders.size()
            && index.row() < records.size()
            )  {
            auto& k = rheaders.at(index.column());
            auto& r = records.at(index.row());
            auto iter = r.find(k);
            if (iter != r.end()) {
                ret = QString::fromStdString(iter->second);
            }
        }
        m.unlock_shared();
        return ret;
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
        if (m.try_lock_shared()) {
            if (section < rheaders.size()) {
                ret = QString::fromStdString(rheaders.at(section));
            }
            m.unlock_shared();
        }
        return ret;
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
    auto text = mimeData->text();
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
            emit insertFileSignal(row, file);
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
    endMoveRows();
    return true;
}

void AdifModel::sort(int column, Qt::SortOrder order)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    if (0 <= column && column < rheaders.size()) {
        auto& col = rheaders[column];
        auto less = [=](const decltype(records)::value_type& a, const decltype(records)::value_type& b)->bool {
            auto pa = a.find(col);
            auto pb = b.find(col);
            if (pa == a.end() && pb == b.end()) {
                return false;
            }
            if (pb == b.end()) {
                return true;
            }
            if (pa == a.end()) {
                return false;
            }
            return pa->second < pb->second;
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
    endResetModel();
}

std::string AdifModel::records2StdString(const std::set<int> &rows) const
{
    auto& m = const_cast<decltype(mutex)&>(mutex);
    std::shared_lock<decltype(mutex)> lock(m);
    return _records2StdString(rows);
}

void AdifModel::openFile(const QString& filename)
{
    QFileInfo file(filename);
    std::ifstream in(file.filesystemAbsoluteFilePath());
    if (in) {
        driver.switch_streams(in, std::cerr);
        if (!parser.parse())
            setRecords(driver.data.rbegin(), driver.data.rend());
    }
    in.close();
}

void AdifModel::appendFile(const QString& filename)
{
    QFileInfo file(filename);
    std::ifstream in(file.filesystemAbsoluteFilePath());
    if (in) {
        driver.switch_streams(in, std::cerr);
        if (!parser.parse())
            addRecords(driver.data.rbegin(), driver.data.rend());
    }
    in.close();
}

void AdifModel::insertFile(int row, const QString &filename)
{
    QFileInfo file(filename);
    std::ifstream in(file.filesystemAbsoluteFilePath());
    if (in) {
        driver.switch_streams(in, std::cerr);
        if (!parser.parse())
            insertRecords(row, driver.data.rbegin(), driver.data.rend());
    }
    in.close();
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
        endRemoveRows();
        if (idx == 0) {
            break;
        }
        --idx;
        deletebegin = indexes.at(idx).row();
        deleteend = deletebegin + 1;
    }
    auto iter = rheaders.begin();
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
        beginRemoveColumns(QModelIndex(), iter - rheaders.begin(), iter - rheaders.begin());
        iter = rheaders.erase(iter);
        endRemoveColumns();
    }
}
