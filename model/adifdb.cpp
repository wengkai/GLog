#include "adifdb.h"
#include <cstdlib>
#include <iostream>
#include <algorithm>

bool AdifModel::addHeader(const std::string &header)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    return _addHeader(header);
}

bool AdifModel::_addHeader(const std::string &header)
{
    auto& h = header;
    auto iter = std::lower_bound(rheaders.begin(), rheaders.end(), h);
    if (iter != rheaders.end() && *iter == h) {
        return false;
    }
    emit beginInsertColumns(QModelIndex(), iter - rheaders.begin(), iter - rheaders.begin());
    rheaders.insert(iter, h);
    emit endInsertColumns();
    return true;
}

void AdifModel::_clear()
{
    records.clear();
    rheaders.clear();
}

AdifModel::AdifModel(QObject *parent) : QAbstractTableModel(parent)
{
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
    auto& m = const_cast<std::shared_mutex&>(mutex);
    if (role != Qt::DisplayRole)
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
    auto& m = const_cast<std::shared_mutex&>(mutex);
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

AdifModel::~AdifModel()
{
}

void AdifModel::clear()
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    emit beginResetModel();
    _clear();
    emit endResetModel();
}
