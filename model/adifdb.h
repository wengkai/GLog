#ifndef ADIFDB_H
#define ADIFDB_H

#include <QFuture>
#include <QList>
#include <QMimeData>
#include <QPersistentModelIndex>
#include <QStandardItemModel>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>
#include "adifparse_service.h"
#include "ctydb.h"
#include "record.h"
#include "recordrepository.h"

#include "Concurrent.h"
#include "Synchronize.h"

class AdifFileService;

/**
 * @brief Deep-cloned read-only snapshot of an {@link AdifModel}.
 *
 * Produced by {@link AdifModel::snapshotAsync}. Both fields are fully
 * owned copies (rows via `GRecord::clone()`, headers via vector copy), so
 * the snapshot can be moved across threads without aliasing the model's
 * internal state.
 */
struct AdifModelSnapshot {
    std::vector<GRecord> records;
    std::vector<std::string> columnHeaders;
};

class AdifModel : public QAbstractTableModel {
    Q_OBJECT

  public:
    using Record = GRecord;
    static std::vector<std::string> getDefaultSortModel(const std::string &field);

  private:
    std::vector<Record> m_records{};
    std::unordered_set<std::string> sheaders{};
    std::vector<std::string> rheaders{};
    // mutable std::shared_mutex mutex{};
    std::shared_ptr<GRecordRepository> m_db_backup;
    mutable GLogConcurrent::FIFOBackendThreadExecutor m_fifo_exec;
    AdifFileService *m_file_service = nullptr;

    auto rheaders_begin() {
        return (rheaders.begin() + static_cast<std::ptrdiff_t>(sheaders.size()));
    }
    auto rheaders_begin() const {
        return (rheaders.cbegin() + static_cast<std::ptrdiff_t>(sheaders.size()));
    }

    void _clear();
    bool _addHeader(std::string header);
    std::string _records2StdString(const std::set<int> &rows) const;

    template <typename Backup> void tryDbBackup(Backup &&backup) {
        if (m_db_backup) {
            std::invoke(std::forward<Backup>(backup), m_db_backup);
        }
    }

    void _resetRecords(std::vector<Record> new_record, std::unordered_set<std::string> new_headers);

    void _insertRecords(int where, std::vector<Record> new_record,
                        std::unordered_set<std::string> new_headers);

    std::pair<bool, std::vector<std::string>::const_iterator>
    _addHeaderCheck(const std::string &header) const;
    bool _addHeaderNoSignal(std::string header);

    int64_t persistFileIdForBackup() const;

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
    QModelIndex findNext(const QModelIndex &start, const Condition &condition,
                         const std::string &field = "") const;
    QList<int> findAll(const Condition &condition, const std::string &field = "") const;

    virtual ~AdifModel();

    bool addHeader(std::string header);

    void clear();

    void setDbBackup(std::shared_ptr<GRecordRepository> backup) { m_db_backup = std::move(backup); }
    auto getDbBackup() const { return m_db_backup; }

    /**
     * @brief Clone rows for SQLite upsert. Must be invoked on the GUI thread (same thread as the
     * model).
     * @param rowCount Number of rows; if < 0, clones from @p startRow through the last row.
     */
    std::vector<GRecord> cloneRecordsForPersistenceSlice(int startRow, int rowCount) const;

    /**
     * @brief Asynchronously produces a deep-cloned snapshot of the model.
     *
     * Callable from any thread. The snapshot task is dispatched through
     * {@link GLogConcurrent::MainThreadExecutor} so the rows and headers
     * are copied while the model is quiescent on the GUI thread; the
     * resulting {@link AdifModelSnapshot} contains row clones and a copy
     * of the current column header order, with no aliasing of the
     * model's internal state.
     *
     * Background callers should consume the future via continuations
     * (`then(...)`); only blocking on the future from a non-GUI thread is
     * safe (the GUI thread must remain free to service the posted task).
     */
    QFuture<AdifModelSnapshot> snapshotAsync() const;

    void setFileService(AdifFileService *service) { m_file_service = service; }

    void applyFullLoad(AdifParseService::ParseRes &&res);
    void applyInsertAt(int row, AdifParseService::ParseRes &&res);

    std::string records2StdString(const std::set<int> &rows) const;

    GLogConcurrent::FIFOBackendThreadExecutor *getFIFOBackendThreadExecutor() {
        return &m_fifo_exec;
    }

    void insertRecords(int where, std::vector<Record> new_record);

    void insertRecords(int where, std::vector<Record> new_record,
                       std::unordered_set<std::string> new_headers);

    // 以特定字段组为基准返回重复记录
    std::vector<std::vector<QPersistentModelIndex>>
    findDuplicates(const std::vector<std::string> &fields) const;

    size_t deleteRowsPersistent(const std::vector<QPersistentModelIndex> &indexes);
    bool mergeRows(const QModelIndex &major, const QModelIndexList &indexes);
    QFuture<bool> mergeRowsAsync(QModelIndex major, QModelIndexList indexes);
    QFuture<size_t> deleteRowsAsync(QModelIndexList indexes);

  public slots:
    size_t deleteRows(QModelIndexList indexes);
    void sortSelectedColumn(int column, Qt::SortOrder order);
    void removeSelectedColumn(int column);
    void mapCallSignInView(bool keepOrigin);

    void newViewWithRows(const QModelIndexList &indexes) const;
    void pasteRows(const QMimeData *mimeData);
    void copyRows(const QModelIndexList &indexes);
    void findNextS(QModelIndex current, const QString &key, const QString &value, bool isReg);
    void selectAll(const QString &key, const QString &value, bool isReg = false);
    void deselectAll(const QString &key, const QString &value, bool isReg = false);

  signals:
    void mapCallSignInViewBegin();
    void mapCallSignInViewEnd(int failCount, int confictCount);
    void setCilpboard(QMimeData *mimeData);
    void foundNext(QModelIndex index);
    void selectRows(QList<int> rows);
    void deselectRows(QList<int> rows);
};

#endif
