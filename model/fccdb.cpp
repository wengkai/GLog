#include "fccdb.h"
#include <QFile>
#include <QThread>

FccDB *FccDB::instance() {
    static auto *fccdb = new FccDB();
    return fccdb;
}

QString FccDB::dbPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + dbFileName();
}

QString FccDB::dbFileName() { return "fcc_amateur.db"; }

QString FccDB::connNamePrefix() { return "fcc_search_connection"; }

QString FccDB::lookupState(const QString &callsign) {
    if (callsign.isEmpty()) {
        return QString();
    }

    {
        std::unique_lock<decltype(mutex_cache)> lock0(mutex_cache);
        if (auto *pres = m_cache.object(callsign); pres) {
            return *pres;
        }
    }

    auto threadId = quintptr(QThread::currentThreadId());
    QString res;

    {
        std::shared_lock<decltype(mutex_threadId2Query)> lock(mutex_threadId2Query);
        auto it = threadId2Query.constFind(threadId);
        Q_ASSERT(it != threadId2Query.end());
        auto &query = const_cast<QSqlQuery &>(it.value());
        query.bindValue(":call", callsign);
        if ((query.exec() && query.next())) {
            res = query.value(0).toString();
        }
    }

    {
        std::unique_lock<decltype(mutex_cache)> lock0(mutex_cache);
        m_cache.insert(callsign, new QString(res));
    }

    return res;
}

bool FccDB::beginSearch() {
    if (!QFile::exists(dbPath())) {
        return false;
    }
    auto threadId = quintptr(QThread::currentThreadId());
    auto connName = QString("%1_%2").arg(connNamePrefix()).arg(threadId);
    if (!QSqlDatabase::contains(connName)) {
        auto db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath());
        db.setConnectOptions("QSQLITE_OPEN_READONLY");
        if (!db.open()) {
            return false;
        }
        QSqlQuery query(db);
        query.exec("PRAGMA synchronous = OFF;");
        query.exec("PRAGMA journal_mode = WAL;");
        query.exec("PRAGMA query_only = ON;");
        query.exec("PRAGMA cache_size = -4000;");
        if (!query.prepare("SELECT state FROM licenses WHERE callsign = :call LIMIT 1")) {
            db.close();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }
        std::unique_lock<decltype(mutex_threadId2Query)> lock(mutex_threadId2Query);
        threadId2Query.emplace(threadId, std::move(query));
    }
    return true;
}

void FccDB::endSearch() {
    auto threadId = quintptr(QThread::currentThreadId());
    {
        std::unique_lock<decltype(mutex_threadId2Query)> lock(mutex_threadId2Query);
        threadId2Query.remove((threadId));
    }
    auto connName = QString("%1_%2").arg(connNamePrefix()).arg(threadId);
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase::database(connName).close();
        QSqlDatabase::removeDatabase(connName);
    }
}
