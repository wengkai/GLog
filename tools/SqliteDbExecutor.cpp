#include "SqliteDbExecutor.h"
#include <QCoreApplication>
#include <utility>

SqlTransaction::~SqlTransaction() {
    if (--(*m_countPtr) == 0) {
        if (m_active && !m_committed) {
            m_db.rollback();
            qWarning()
                << "SqliteDbExecutor: Transaction auto-rolled back due to incomplete commit.";
        } else if (*m_rollbackOnly && m_active) {
            m_db.rollback();
            qWarning()
                << "SqliteDbExecutor: Transaction rolled back because rollbackOnly flag is set.";
        }
    }
}

auto SqlTransaction::commit() -> bool {
    if (!m_active || m_committed) {
        return false;
    }

    if (*m_rollbackOnly) {
        m_committed = false;
        return false;
    }

    if (*m_countPtr == 1) {
        m_committed = m_db.commit();
        if (!m_committed) {
            qWarning() << "SqliteDbExecutor: Commit failed:" << m_db.lastError().text();
        }
        m_active = !m_committed;
        return m_committed;
    }
    m_committed = true;
    return true;
}

SqlTransaction::SqlTransaction(QSqlDatabase &d, std::atomic<long long> &count,
                               std::shared_ptr<bool> rollbackFlag)
    : m_db(d), m_countPtr(&count), m_rollbackOnly(std::move(rollbackFlag)), m_active(true),
      m_committed(false) {
    if ((*m_countPtr)++ == 0) {
        m_active = m_db.transaction();
        if (!m_active) {
            qWarning() << "SqliteDbExecutor: Failed to begin transaction:"
                       << m_db.lastError().text();
            *m_rollbackOnly = true;
        }
    }
}

static auto generateConnectionName() -> QString {
    return QStringLiteral("sqlite_exec_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// ==================== Worker ====================
SqliteDbExecutor::Worker::Worker(QString dbPath, QString connName)
    : m_dbPath(std::move(dbPath)), m_connectionName(std::move(connName)) {}

SqliteDbExecutor::Worker::~Worker() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    qDebug() << "SqliteDbExecutor: Worker destroyed in thread" << QThread::currentThread();
}

void SqliteDbExecutor::Worker::init() {
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) {
        qWarning() << "SqliteDbExecutor: Failed to open database" << m_dbPath
                   << "Error:" << m_db.lastError().text();
        return;
    }

    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA journal_mode = WAL;");
    pragma.exec("PRAGMA synchronous = NORMAL;");
    pragma.exec("PRAGMA foreign_keys = ON;");
    qDebug() << "SqliteDbExecutor: Database opened in thread" << QThread::currentThread();
}

// ==================== SqliteDbExecutor ====================
SqliteDbExecutor::SqliteDbExecutor(const QString &dbPath)
    : m_thread(std::make_unique<QThread>()), m_connectionName(generateConnectionName()) {
    m_workerRaw = new Worker(dbPath, m_connectionName);
    m_workerRaw->moveToThread(m_thread.get());

    QObject::connect(m_thread.get(), &QThread::started, m_workerRaw, &Worker::init);
    QObject::connect(m_thread.get(), &QThread::finished, m_workerRaw, &QObject::deleteLater);

    m_thread->start();
}

SqliteDbExecutor::~SqliteDbExecutor() {
    m_thread->quit();
    m_thread->wait();
}

auto SqliteDbExecutor::createTransaction() -> SqlTransaction {
    Q_ASSERT_X(QThread::currentThread() == m_thread.get(), "SqliteDbExecutor::createTransaction",
               "Transactions must be created within the executor thread");
    if (m_transactionCount == 0) {
        m_currentRollbackFlag = std::make_shared<bool>(false);
    }
    return {database(), m_transactionCount, m_currentRollbackFlag};
}

auto SqliteDbExecutor::database() -> QSqlDatabase & {
    Q_ASSERT_X(QThread::currentThread() == m_thread.get(), "SqliteDbExecutor::database",
               "database() must be called within the worker thread");
    return m_workerRaw->database();
}