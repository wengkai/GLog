#ifndef ADIFDB_H
#define ADIFDB_H

#include <QFuture>
#include <QList>
#include <QMimeData>
#include <QStandardItemModel>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>
#include "ctydb.h"
#include "glogparser.h"
#include "record.h"
#include "recordrepository.h"

#include "Concurrent.h"

class MapWidget;

class AdifModel : public QAbstractTableModel {
    Q_OBJECT

  public:
    using Record = GRecord;
    static std::vector<std::string> getDefaultSortModel(const std::string &field);

  private:
    std::vector<Record> records{};
    std::unordered_set<std::string> sheaders{};
    std::vector<std::string> rheaders{};
    // mutable std::shared_mutex mutex{};
    std::shared_ptr<GRecordRepository> m_db_backup;
    mutable GLogConcurrent::FIFOBackendThreadExecutor m_fifo_exec;
    friend class MapWidget;

    auto rheaders_begin() {
        return (rheaders.begin() + static_cast<std::ptrdiff_t>(sheaders.size()));
    }
    auto rheaders_begin() const {
        return (rheaders.cbegin() + static_cast<std::ptrdiff_t>(sheaders.size()));
    }

    void _clear();
    bool _addHeader(std::string header);
    std::string _records2StdString(const std::set<int> &rows) const;
    template <typename Ostream>
    static void _toCsv(Ostream &stream, std::vector<Record> p_records,
                       std::vector<std::string> p_rheaders);
    template <typename Ostream> static void _toAdif(Ostream &stream, std::vector<Record> p_records);

    template <typename Backup> void tryDbBackup(Backup &&backup) {
        if (m_db_backup) {
            std::invoke(std::forward<Backup>(backup), m_db_backup);
        }
    }

    void _resetRecords(std::vector<Record> new_record, std::unordered_set<std::string> new_headers);

    void _insertRecords(int where, std::vector<Record> new_record,
                        std::unordered_set<std::string> new_headers);

    std::vector<std::string> openFile(const QString &filename, bool remove = false);
    std::vector<std::string> insertFile(int row, const QString &filename, bool remove = false);
    std::vector<std::string> insertStringData(int row, std::string data);

  public:
    explicit AdifModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                      const QModelIndex &parent) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destinationParent, int destinationChild) override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    using Condition = std::function<bool(const std::string &)>;
    QModelIndex findNext(const QModelIndex &start, Condition condition,
                         const std::string &field = "") const;
    QList<int> findAll(Condition condition, const std::string &field = "") const;

    virtual ~AdifModel();

    QFuture<bool> addHeader(std::string header);

    void clear();

    void setDbBackup(std::shared_ptr<GRecordRepository> backup) { m_db_backup = std::move(backup); }
    auto getDbBackup() const { return m_db_backup; }

    std::string records2StdString(const std::set<int> &rows) const;
    void toCsv(std::ostream &stream) const;
    void toAdif(std::ostream &stream) const;

    QFuture<std::vector<std::string>> openFileAsync(const QString &filename, bool remove = false);
    QFuture<std::vector<std::string>> insertFileAsync(int where, const QString &filename,
                                                      bool remove = false);
    QFuture<std::vector<std::string>> insertStringDataAsync(int row, std::string data);

    void insertRecords(int where, std::vector<Record> new_record);

    void insertRecords(int where, std::vector<Record> new_record,
                       std::unordered_set<std::string> new_headers);

    struct ParseRes {
        bool parse_res = false;
        std::vector<Record> parse_records;
        std::unordered_set<std::string> parse_headers;
    };
    enum class ParseMode { Batch, Interactive };
    static ParseRes parse(std::istream &in, std::vector<std::string> &errors, ParseMode mode);
    static ParseRes
    fromRawData(bool success,
                std::vector<std::vector<std::pair<std::string, std::string>>> raw_data);
    GLogConcurrent::FIFOBackendThreadExecutor *getFIFOBackendThreadExecutor() {
        return &m_fifo_exec;
    }

    struct AwardRes {
        QString DXCC = "0";
        QString WAC_ARRL = "0";
        QString WAC_NOTARRL = "0";
        QString CQZ = "0";
        QString WAS = "Invaild";
    };
    AwardRes diffEntNameCountForAward() const;

  public slots:
    void deleteRows(QModelIndexList indexes);
    void sortSelectedColumn(int column, Qt::SortOrder order);
    void removeSelectedColumn(int column);
    void mapCallSignInView(bool keepOrigin);

    void saveAs(const QString &filename) const;
    void newViewWithRows(const QModelIndexList &indexes) const;
    void pasteRows(const QMimeData *mimeData);
    void copyRows(const QModelIndexList &indexes);
    void findNextS(QModelIndex current, const QString &key, const QString &value, bool isReg);
    void selectAll(const QString &key, const QString &value, bool isReg = false);
    void deselectAll(const QString &key, const QString &value, bool isReg = false);

  signals:
    void mapCallSignInViewBegin();
    void mapCallSignInViewEnd(int failCount, int confictCount);
    void saveDone() const;
    void setCilpboard(QMimeData *mimeData);
    void foundNext(QModelIndex index);
    void selectRows(QList<int> rows);
    void deselectRows(QList<int> rows);
};

#endif