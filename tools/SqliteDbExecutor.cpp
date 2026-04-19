#include "SqliteDbExecutor.h"
#include <QCoreApplication>

SqlTransaction::~SqlTransaction() {
    if (--(*m_countPtr) == 0) {
        // 只要任一层未提交或标记为错误，最终物理操作必须是回滚
        if (m_active && !m_committed) {
            m_db.rollback();
            qWarning()
                << "SqliteDbExecutor: Transaction auto-rolled back due to incomplete commit.";
        }
        // 即使 committed 为 true，若 rollbackOnly 为 true 也要回滚
        else if (*m_rollbackOnly && m_active) {
            m_db.rollback();
            qWarning()
                << "SqliteDbExecutor: Transaction rolled back because rollbackOnly flag is set.";
        }
    }
}

auto SqlTransaction::commit() -> bool {
    if (!m_active || m_committed) return false;

    // 若已被标记为必须回滚，禁止提交
    if (*m_rollbackOnly) {
        m_committed = false; // 确保析构回滚
        return false;
    }

    // 只有最外层事务真正执行数据库 Commit
    if (*m_countPtr == 1) {
        m_committed = m_db.commit();
        if (!m_committed) {
            qWarning() << "SqliteDbExecutor: Commit failed:" << m_db.lastError().text();
        }
        m_active = !m_committed;
        return m_committed;
    } else {
        // 内层嵌套仅标记逻辑成功
        m_committed = true;
        return true;
    }
}

SqlTransaction::SqlTransaction(QSqlDatabase &d, std::atomic<long long> &count,
                               std::shared_ptr<bool> rollbackFlag)
    : m_db(d), m_countPtr(&count), m_rollbackOnly(rollbackFlag), m_active(true),
      m_committed(false) {
    // 当计数器从0变1时，开启物理事务
    if ((*m_countPtr)++ == 0) {
        m_active = m_db.transaction();
        if (!m_active) {
            qWarning() << "SqliteDbExecutor: Failed to begin transaction:"
                       << m_db.lastError().text();
            *m_rollbackOnly = true; // 开启失败，后续操作均应无效
        }
    }
}

static QString generateConnectionName() {
    return QStringLiteral("sqlite_exec_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// ==================== Worker 实现 ====================
SqliteDbExecutor::Worker::Worker(const QString &dbPath, const QString &connName)
    : m_dbPath(dbPath), m_connectionName(connName) {
    // 构造函数在主线程运行，不做任何数据库操作
}

SqliteDbExecutor::Worker::~Worker() {
    // 析构发生在子线程（因为使用了 deleteLater）
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    qDebug() << "SqliteDbExecutor: Worker destroyed in thread" << QThread::currentThread();
}

void SqliteDbExecutor::Worker::init() {
    // 此槽在子线程事件循环中执行，数据库连接安全创建于子线程
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) {
        qWarning() << "SqliteDbExecutor: Failed to open database" << m_dbPath
                   << "Error:" << m_db.lastError().text();
        return;
    }

    // SQLite 优化设置
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA journal_mode = WAL;");
    pragma.exec("PRAGMA synchronous = NORMAL;");
    pragma.exec("PRAGMA foreign_keys = ON;");
    qDebug() << "SqliteDbExecutor: Database opened in thread" << QThread::currentThread();
}

// ==================== SqliteDbExecutor 实现 ====================
SqliteDbExecutor::SqliteDbExecutor(const QString &dbPath)
    : m_thread(std::make_unique<QThread>()), m_connectionName(generateConnectionName()) {
    // 创建 Worker（主线程）
    m_workerRaw = new Worker(dbPath, m_connectionName);
    m_workerRaw->moveToThread(m_thread.get());

    // 关键：连接 started 信号到 init 槽，确保初始化在子线程执行
    QObject::connect(m_thread.get(), &QThread::started, m_workerRaw, &Worker::init);
    QObject::connect(m_thread.get(), &QThread::finished, m_workerRaw, &QObject::deleteLater);

    m_thread->start();
}

SqliteDbExecutor::~SqliteDbExecutor() {
    m_thread->quit();
    m_thread->wait(); // 等待子线程退出，deleteLater 会自动销毁 Worker
}

SqlTransaction SqliteDbExecutor::createTransaction() {
    Q_ASSERT_X(QThread::currentThread() == m_thread.get(), "SqliteDbExecutor::createTransaction",
               "Transactions must be created within the executor thread");
    // 如果计数为 0，说明是顶层事务，创建新 flag
    if (m_transactionCount == 0) {
        m_currentRollbackFlag = std::make_shared<bool>(false);
    }
    // 所有嵌套事务共享同一个 flag
    return SqlTransaction(database(), m_transactionCount, m_currentRollbackFlag);
}

QSqlDatabase &SqliteDbExecutor::database() {
    Q_ASSERT_X(QThread::currentThread() == m_thread.get(), "SqliteDbExecutor::database",
               "database() must be called within the worker thread");
    return m_workerRaw->database();
}