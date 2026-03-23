#ifndef FCCDB_H
#define FCCDB_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QCache>
#include <QHash>
#include <mutex>
#include <shared_mutex>

class FccDB {
public:
    static FccDB * instance();
    static QString dbPath();
    static QString dbFileName();
    QString lookupState(const QString & callsign);
    bool beginSearch();
    void endSearch();

private:
    FccDB() = default;
    static QString connNamePrefix();
    QCache<QString, QString> m_cache{5000};
    std::shared_mutex mutex_cache;
    QHash<quintptr, QSqlQuery> threadId2Query;
    std::shared_mutex mutex_threadId2Query;
};

#endif