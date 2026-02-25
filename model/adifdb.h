#ifndef ADIFDB_H
#define ADIFDB_H

#include <QStandardItemModel>
#include <vector>
#include <string>
#include <map>
#include <shared_mutex>

class AdifModel : public QAbstractTableModel
{
    Q_OBJECT
    
private:
    std::vector<std::map<std::string, std::string>> records{};
    std::vector<std::string> rheaders{};
    std::shared_mutex mutex{};
    
    template <typename PairIter>
    void _addRecord(const PairIter& begin, const PairIter& end) {
        decltype(records)::value_type r{};
        for (auto iter { begin }; iter != end; ++iter) {
            auto k { iter->first };
            auto& v = iter->second;
            for (auto& c : k) if ('A' <= c && c <= 'Z') c = c - 'A' + 'a';
            r[k] = v;
            _addHeader(k);
        }
        records.push_back(r);
    }; 
    void _clear();
    bool _addHeader(const std::string& header);
    
public:
    explicit AdifModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual ~AdifModel();

    bool addHeader(const std::string& header);

    template <typename RecordIter>
    void addRecords(const RecordIter& begin, const RecordIter& end) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        emit beginInsertRows(QModelIndex(), records.size(), records.size() + (end - begin) - 1);
        for (auto iter {begin}; iter != end; ++iter) {
            _addRecord(iter->begin(), iter->end());
        }
        emit endInsertRows();
    }

    template <typename RecordIter>
    void setRecords(const RecordIter& begin, const RecordIter& end) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        emit beginResetModel();
        _clear();
        for (auto iter {begin}; iter != end; ++iter) {
            _addRecord(iter->begin(), iter->end());
        }
        emit endResetModel();
    }

    void clear();
};




#endif