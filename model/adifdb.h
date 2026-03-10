#ifndef ADIFDB_H
#define ADIFDB_H

#include "glogparser.h"
#include "record.h"
#include "ctydb.h"
#include <QStandardItemModel>
#include <QMimeData>
#include <QList>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <shared_mutex>
#include <mutex>
#include <iostream>

class AdifModel;

class MapWidget;

class AdifModelC : public QObject {
    Q_OBJECT

private:
    AdifModel* model;

public:
    explicit AdifModelC(AdifModel* model, QObject* parent = nullptr);

    
public slots:
    void openFile(QString filename);
    void appendFile(QString filename, bool remove = false);
    void insertFile(int row, QString filename);
    void saveAs(QString filename);
    void newViewWithRows(QModelIndexList indexes);
    void pasteRows(const QMimeData* mimeData);
    void copyRows(const QModelIndexList indexes);
    void findNext(QModelIndex current, QString key, QString value, bool isReg = false);
    void selectAll(QString key, QString value, bool isReg = false);
    void deselectAll(QString key, QString value, bool isReg = false);

signals:
    void modelUpdated();
    void saveDone();
    void setCilpboard(QMimeData* mimeData);
    void foundNext(QModelIndex index);
    void selectRows(QList<int> rows);
    void deselectRows(QList<int> rows);

};

class AdifModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    using Record = GRecord;
    static std::vector<std::string> getDefaultSortModel(const std::string& field);
    
private:
    std::vector<Record> records{};
    std::set<std::string> sheaders{};
    std::vector<std::string> rheaders{};
    std::shared_mutex mutex{};
    friend class AdifModelC;
    friend class MapWidget;
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
    void _toCsv(std::ostream& stream) const;
    void _toAdif(std::ostream& stream) const;

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

    template<typename Condition>
    QModelIndex findNext(const QModelIndex& start, Condition condition, const std::string& field = "") const {
        auto ret = QModelIndex();
        auto& m = const_cast<decltype(mutex)&>(mutex);
        std::shared_lock<decltype(mutex)> lock(m);
        if (records.empty()) { 
            return ret;
        }
        int row = start.isValid() ? start.row() : 0;
        bool finded = false;
        for (int i = 0; i <= records.size() && !finded; ++i, ++row) {
            row %= records.size();
            auto& record = records.at(row);
            if (field.empty()) {
                int startColumn = 0;
                if (i == 0) {
                    startColumn = start.isValid() ? start.column() : 0;
                }
                for (int j = startColumn; j < rheaders.size(); ++j) {
                    auto iter = record.find(rheaders[j]);
                    if (iter != record.end() && condition(iter->second)) {
                        ret = index(row, j);
                        finded = true;
                        break;
                    }
                }
            }
            else {
                auto iter = record.find(field);
                if (iter != record.end() && condition(iter->second)) {
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
    template<typename Condition>
    QList<int> findAll(Condition condition, const std::string& field = "") const {
        QList<int> rows;
        auto& m = const_cast<decltype(mutex)&>(mutex);
        std::shared_lock<decltype(mutex)> lock(m);
        if (records.empty()) {
            return rows;
        }
        for (int i = 0; i < records.size(); ++i) {
            auto& record = records[i];
            if (field.empty()) {
                for (auto& h : rheaders) {
                    auto iter = record.find(h);
                    if (iter != record.end() && condition(iter->second)) {
                        rows.append(i);
                        break;
                    }
                }
            }
            else {
                auto iter = record.find(field);
                if (iter != record.end() && condition(iter->second)) {
                    rows.append(i);
                }
            }
        }
        return rows;
    };
    virtual ~AdifModel();

    bool addHeader(const std::string& header);

    template <typename RecordIter>
    void addRecords(const RecordIter& begin, const RecordIter& end) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        beginInsertRows(QModelIndex(), records.size(), records.size() + (end - begin) - 1);
        for (auto iter {begin}; iter != end; ++iter) {
            _addRecord(iter->begin(), iter->end());
        }
        lock.unlock();
        endInsertRows();
    }

    template <typename RecordIter>
    void insertRecords(int row, const RecordIter& begin, const RecordIter& end) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        if (row == -1) row = records.size();
        beginInsertRows(QModelIndex(), row, row + (end - begin) - 1);
        _insertRecords(row, begin, end);
        lock.unlock();
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
        lock.unlock();
        endResetModel();
    }

    void clear();

    std::string records2StdString(const std::set<int>& rows) const;
    void toCsv(std::ostream& stream) const;
    void toAdif(std::ostream& stream) const;

    void openFile(const QString& filename);
    void appendFile(const QString& filename);
    void insertFile(int row, const QString& filename);
    bool saveAs(QString filename) const;

public slots:
    void deleteRows(QModelIndexList indexes);
    void sortSelectedColumn(int column, Qt::SortOrder order);
    void removeSelectedColumn(int column);
    void mapCallSignInView(bool keepOrigin);

signals:
    //void appendFileSignal(QString filename);
    void insertFileSignal(int row, QString filename);
    void mapCallSignInViewBegin();
    void mapCallSignInViewEnd(int failCount, int confictCount);

};




#endif