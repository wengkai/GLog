#ifndef SQLITEDBEXECUTOR_H
#define SQLITEDBEXECUTOR_H

#include <QDebug>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QUuid>
#include <atomic>
#include <functional>
#include <memory>

#include "app_export.h"

class SqliteDbExecutor;

// 改进后的事务 RAII 类，支持正确嵌套与错误传播
struct GLOGKIT_API SqlTransaction {
    friend class SqliteDbExecutor;

  public:
    ~SqlTransaction();

    bool commit();

    // 手动标记事务必须回滚（例如捕获异常后调用）
    void setRollbackOnly() { *m_rollbackOnly = true; }

    SqlTransaction(const SqlTransaction &) = delete;
    SqlTransaction &operator=(const SqlTransaction &) = delete;

  private:
    SqlTransaction(QSqlDatabase &d, std::atomic<long long> &count,
                   std::shared_ptr<bool> rollbackFlag);

    QSqlDatabase &m_db;
    std::atomic<long long> *m_countPtr;
    std::shared_ptr<bool> m_rollbackOnly;
    bool m_active;
    bool m_committed;
};

/**
 * @brief SQLite 数据库专用执行器
 */
class GLOGKIT_API SqliteDbExecutor {
  public:
    explicit SqliteDbExecutor(const QString &dbPath);
    ~SqliteDbExecutor();

    SqliteDbExecutor(const SqliteDbExecutor &) = delete;
    SqliteDbExecutor &operator=(const SqliteDbExecutor &) = delete;

    /**
     * @brief 在任务闭包内创建事务对象
     */
    SqlTransaction createTransaction();

    template <typename Callable> void start(Callable &&functionToRun);

    QSqlDatabase &database();

  private:
    class GLOGKIT_API Worker : public QObject {
      public:
        Worker(QString dbPath, QString connName);
        ~Worker();

        QSqlDatabase &database() { return m_db; }

      public slots:
        void init(); // 在子线程中初始化数据库连接

      private:
        QString m_dbPath;
        QString m_connectionName;
        QSqlDatabase m_db;
    };

    std::unique_ptr<QThread> m_thread;
    Worker *m_workerRaw = nullptr; // 原始指针，由 deleteLater 管理生命周期
    QString m_connectionName;

    std::atomic<long long> m_transactionCount{0};
    std::shared_ptr<bool> m_currentRollbackFlag;
};

template <typename Callable> void SqliteDbExecutor::start(Callable &&functionToRun) {
    // Qt6.7+:
    // invokeMethod(QObject *context, Functor &&function, Qt::ConnectionType type, Args
    // &&... arguments)
    QMetaObject::invokeMethod(
        m_workerRaw, [task = std::forward<Callable>(functionToRun)]() mutable { task(); },
        Qt::QueuedConnection);
}

#endif // SQLITEDBEXECUTOR_H