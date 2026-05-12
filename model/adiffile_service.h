#ifndef ADIFFILE_SERVICE_H
#define ADIFFILE_SERVICE_H

#include <QFuture>
#include <QObject>
#include <QString>
#include <cstdint>
#include <string>
#include <vector>

class AdifModel;

/** File/string load and save orchestration around {@link AdifModel}. */
class AdifFileService : public QObject {
    Q_OBJECT

  public:
    explicit AdifFileService(AdifModel *model, QObject *parent = nullptr);

    QFuture<std::vector<std::string>> openFileAsync(const QString &filename, bool remove = false);
    QFuture<std::vector<std::string>> insertFileAsync(int where, const QString &filename,
                                                      bool remove = false);
    QFuture<std::vector<std::string>> insertStringDataAsync(int row, std::string data);

    void saveAs(const QString &filename);

    /** Active SQLite `files.id` for rows merged into this session; -1 if none. */
    int64_t activePersistedFileId() const { return m_activeFileId; }

  signals:
    void saveDone() const;
    void saveFailed(QString message) const;

  private:
    std::vector<std::string> openFile(const QString &filename, bool remove);
    std::vector<std::string> insertFile(int row, const QString &filename, bool remove);
    std::vector<std::string> insertStringData(int row, const std::string &data);

    AdifModel *m_model = nullptr;
    int64_t m_activeFileId = -1;
};

#endif
