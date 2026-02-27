#ifndef ADIFDB_H
#define ADIFDB_H

#include "glogparser.h"
#include <QStandardItemModel>
#include <QMimeData>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <shared_mutex>

class AdifModel;

class AdifModelC : public QObject {
    Q_OBJECT

private:
    AdifModel* model;

public:
    explicit AdifModelC(AdifModel* model, QObject* parent = nullptr);

    
public slots:
    void openFile(QString filename);
    void appendFile(QString filename);
    void insertFile(int row, QString filename);

signals:
    void modelUpdated();

};

class AdifModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    using Record = std::map<std::string, std::string>;
    static std::string record2StdString(const Record& record);
    
private:
    std::vector<Record> records{};
    std::vector<std::string> rheaders{};
    std::shared_mutex mutex{};
    friend class AdifModelC;
    AdifModelC* control = nullptr;

    GLOG_PARSER::GLogParserDriver driver{  };
    GLOG_PARSER::Parser parser{ &driver };
    
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
    template <typename PairIter>
    void _insertRecord(int row, const PairIter& begin, const PairIter& end) {
        decltype(records)::value_type r{};
        for (auto iter { begin }; iter != end; ++iter) {
            auto k { iter->first };
            auto& v = iter->second;
            for (auto& c : k) if ('A' <= c && c <= 'Z') c = c - 'A' + 'a';
            r[k] = v;
            _addHeader(k);
        }
        records.insert(records.begin() + row, r);
    };
    void _clear();
    bool _addHeader(const std::string& header);
    std::string _records2StdString(const std::set<int>& rows) const;

    template <typename RecordIter>
    void _insertRecords(int row, const RecordIter& begin, const RecordIter& end) {
        for (auto iter{ begin }; iter != end; ++iter, ++row) {
            _insertRecord(row, iter->begin(), iter->end());
        }
    };
    
public:
    explicit AdifModel(QObject *parent = nullptr);
    AdifModelC* getControl();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    virtual ~AdifModel();

    bool addHeader(const std::string& header);

    template <typename RecordIter>
    void addRecords(const RecordIter& begin, const RecordIter& end) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        beginInsertRows(QModelIndex(), records.size(), records.size() + (end - begin) - 1);
        for (auto iter {begin}; iter != end; ++iter) {
            _addRecord(iter->begin(), iter->end());
        }
        endInsertRows();
    }

    template <typename RecordIter>
    void insertRecords(int row, const RecordIter& begin, const RecordIter& end) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        if (row == -1) row = records.size();
        beginInsertRows(QModelIndex(), row, row + (end - begin) - 1);
        _insertRecords(row, begin, end);
        endInsertRows();
    }

    template <typename RecordIter>
    void setRecords(const RecordIter& begin, const RecordIter& end) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        beginResetModel();
        _clear();
        for (auto iter {begin}; iter != end; ++iter) {
            _addRecord(iter->begin(), iter->end());
        }
        endResetModel();
    }

    void clear();

    std::string records2StdString(const std::set<int>& rows) const;

    void openFile(const QString& filename);
    void appendFile(const QString& filename);
    void insertFile(int row, const QString& filename);

public slots:
    void deleteRows(QModelIndexList indexes);

signals:
    void appendFileSignal(QString filename);
    void insertFileSignal(int row, QString filename);

};




#endif