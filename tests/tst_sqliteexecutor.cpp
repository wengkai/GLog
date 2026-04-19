// tst_sqliteexecutor.cpp
#include <QtTest>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <atomic>

#include "Concurrent.h"
#include "SqliteDbExecutor.h"

class SqliteDbExecutorTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_basicAsync();
    void test_databaseOperations();
    void test_threadAffinity();
    void test_nestedTransactionSuccess();
    void test_nestedTransactionRollback();
    void test_transactionRAII();
    void test_exceptionPropagation();
    void test_concurrentPressure();

  private:
    // 辅助函数：阻塞等待 future 完成
    template <typename T> void waitForFuture(const QFuture<T> &future) {
        if (future.isFinished()) return;
        QEventLoop loop;
        QFutureWatcher<T> watcher;
        watcher.setFuture(future);
        QObject::connect(&watcher, &QFutureWatcher<T>::finished, &loop, &QEventLoop::quit);
        loop.exec();
    }

    std::unique_ptr<SqliteDbExecutor> m_executor;
    QString m_dbPath;
    QTemporaryFile m_tempFile;
};

void SqliteDbExecutorTest::initTestCase() {
    // 创建临时数据库文件
    if (!m_tempFile.open()) {
        QFAIL("无法创建临时文件");
    }
    m_dbPath = m_tempFile.fileName();
    m_tempFile.close(); // 关闭文件，让 SQLite 独占访问

    // 初始化执行器
    m_executor = std::make_unique<SqliteDbExecutor>(m_dbPath);

    // 创建测试表（使用 makeFuture 异步执行）
    auto future = GLogConcurrent::makeFuture(
        [this]() {
            auto &db = m_executor->database();
            QSqlQuery query(db);
            if (!query.exec(
                    "CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, value TEXT)")) {
                throw std::runtime_error("建表失败: " + query.lastError().text().toStdString());
            }
            // 清空表（保证每个测试独立）
            if (!query.exec("DELETE FROM test")) {
                throw std::runtime_error("清空表失败: " + query.lastError().text().toStdString());
            }
        },
        *m_executor);

    waitForFuture(future);
    QVERIFY2(!future.isCanceled(), "初始化任务被取消");
    if (future.isValid()) {
        // 如果有异常，会被重新抛出
        try {
            future.waitForFinished();
        } catch (const std::exception &e) {
            QFAIL((QString("初始化异常: ") + e.what()).toUtf8().data());
        }
    }
}

void SqliteDbExecutorTest::cleanupTestCase() {
    // 等待所有后台任务结束（执行器析构时会清理线程）
    m_executor.reset();
    // 临时文件自动删除
}

void SqliteDbExecutorTest::test_basicAsync() {
    // 测试 makeFuture 基础异步计算
    QElapsedTimer timer;
    timer.start();

    auto future = GLogConcurrent::makeFuture([]() -> int {
        QThread::msleep(100);
        return 42;
    });

    QVERIFY(!future.isFinished());
    waitForFuture(future);
    QVERIFY(future.isFinished());
    QCOMPARE(future.result(), 42);
    QVERIFY(timer.elapsed() >= 100);
}

void SqliteDbExecutorTest::test_databaseOperations() {
    // 测试基本的数据库插入与查询
    auto insertFuture = GLogConcurrent::makeFuture(
        [this]() {
            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.prepare("INSERT INTO test (id, value) VALUES (?, ?)");
            query.addBindValue(1);
            query.addBindValue("hello");
            if (!query.exec()) {
                throw std::runtime_error("插入失败: " + query.lastError().text().toStdString());
            }
            return query.lastInsertId().toInt();
        },
        *m_executor);

    waitForFuture(insertFuture);
    QCOMPARE(insertFuture.result(), 1);

    // 查询验证
    auto queryFuture = GLogConcurrent::makeFuture(
        [this]() -> QString {
            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("SELECT value FROM test WHERE id = 1");
            if (query.next()) {
                return query.value(0).toString();
            }
            return QString();
        },
        *m_executor);

    waitForFuture(queryFuture);
    QCOMPARE(queryFuture.result(), QString("hello"));
}

void SqliteDbExecutorTest::test_threadAffinity() {
    // 验证任务确实在 SqliteDbExecutor 的私有线程中执行
    QThread *executorThread = nullptr;

    // 获取执行器线程指针
    auto getThreadFuture = GLogConcurrent::makeFuture(
        [&executorThread]() { executorThread = QThread::currentThread(); }, *m_executor);
    waitForFuture(getThreadFuture);
    QVERIFY(executorThread != nullptr);

    // 再次提交一个任务，验证线程相同
    auto secondFuture = GLogConcurrent::makeFuture(
        [executorThread]() { QCOMPARE(QThread::currentThread(), executorThread); }, *m_executor);
    waitForFuture(secondFuture);
    QVERIFY(secondFuture.isFinished());
}

void SqliteDbExecutorTest::test_nestedTransactionSuccess() {
    // 嵌套事务成功提交
    auto future = GLogConcurrent::makeFuture(
        [this]() {
            // 外层事务
            auto trans1 = m_executor->createTransaction();

            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("INSERT INTO test (id, value) VALUES (10, 'outer')");

            {
                // 内层事务
                auto trans2 = m_executor->createTransaction();
                query.exec("INSERT INTO test (id, value) VALUES (11, 'inner')");
                bool innerCommitted = trans2.commit();
                if (!innerCommitted) {
                    throw std::runtime_error("内层事务提交失败");
                }
            }

            bool outerCommitted = trans1.commit();
            if (!outerCommitted) {
                throw std::runtime_error("外层事务提交失败");
            }
        },
        *m_executor);

    waitForFuture(future);
    // 无异常
    QVERIFY(future.isFinished());

    // 验证数据存在
    auto queryFuture = GLogConcurrent::makeFuture(
        [this]() -> int {
            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("SELECT COUNT(*) FROM test WHERE id IN (10, 11)");
            if (query.next()) {
                return query.value(0).toInt();
            }
            return 0;
        },
        *m_executor);
    waitForFuture(queryFuture);
    QCOMPARE(queryFuture.result(), 2);
}

void SqliteDbExecutorTest::test_nestedTransactionRollback() {
    // 内层事务标记 rollbackOnly，外层 commit 应失败，数据回滚
    auto future = GLogConcurrent::makeFuture(
        [this]() {
            auto trans1 = m_executor->createTransaction();

            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("INSERT INTO test (id, value) VALUES (20, 'should_rollback')");

            {
                auto trans2 = m_executor->createTransaction();
                query.exec("INSERT INTO test (id, value) VALUES (21, 'inner_rollback')");
                trans2.setRollbackOnly(); // 标记必须回滚
                // 不调用 commit，RAII 会处理回滚
            }

            // 外层尝试提交，应该失败
            bool committed = trans1.commit();
            return committed;
        },
        *m_executor);

    waitForFuture(future);
    QVERIFY(future.isFinished());
    QCOMPARE(future.result(), false); // commit 应返回 false

    // 验证数据未插入
    auto queryFuture = GLogConcurrent::makeFuture(
        [this]() -> int {
            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("SELECT COUNT(*) FROM test WHERE id IN (20, 21)");
            if (query.next()) {
                return query.value(0).toInt();
            }
            return 0;
        },
        *m_executor);
    waitForFuture(queryFuture);
    QCOMPARE(queryFuture.result(), 0);
}

void SqliteDbExecutorTest::test_transactionRAII() {
    // 测试 RAII：事务对象析构但未 commit，应自动回滚
    auto future = GLogConcurrent::makeFuture(
        [this]() {
            {
                auto trans = m_executor->createTransaction();
                auto &db = m_executor->database();
                QSqlQuery query(db);
                query.exec("INSERT INTO test (id, value) VALUES (30, 'raii_rollback')");
                // 故意不调用 commit，离开作用域触发析构回滚
            }

            // 再次查询，应该看不到刚插入的数据
            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("SELECT COUNT(*) FROM test WHERE id = 30");
            if (query.next()) {
                return query.value(0).toInt();
            }
            return -1;
        },
        *m_executor);

    waitForFuture(future);
    QCOMPARE(future.result(), 0);
}

void SqliteDbExecutorTest::test_exceptionPropagation() {
    // 测试任务中抛出异常能否通过 QFuture 正确传递
    auto future = GLogConcurrent::makeFuture(
        [this]() {
            throw std::runtime_error("测试异常");
            return 0;
        },
        *m_executor);

    waitForFuture(future);
    QVERIFY(future.isFinished());

    bool exceptionCaught = false;
    try {
        future.waitForFinished(); // 会重新抛出异常
    } catch (const std::exception &e) {
        exceptionCaught = true;
        QCOMPARE(QString(e.what()), QString("测试异常"));
    }
    QVERIFY(exceptionCaught);
}

void SqliteDbExecutorTest::test_concurrentPressure() {
    // 并发压力测试：连续提交大量插入任务，验证顺序执行且无锁错误
    const int taskCount = 500;
    QList<QFuture<void>> futures;

    for (int i = 0; i < taskCount; ++i) {
        auto future = GLogConcurrent::makeFuture(
            [this, i]() {
                auto &db = m_executor->database();
                QSqlQuery query(db);
                query.prepare("INSERT INTO test (id, value) VALUES (?, ?)");
                query.addBindValue(1000 + i);
                query.addBindValue(QString("task_%1").arg(i));
                if (!query.exec()) {
                    throw std::runtime_error("插入失败: " + query.lastError().text().toStdString());
                }
            },
            *m_executor);
        futures.append(future);
    }

    // 等待所有任务完成
    for (auto &f : futures) {
        waitForFuture(f);
        QVERIFY(!f.isCanceled());
    }

    // 验证插入的总行数
    auto countFuture = GLogConcurrent::makeFuture(
        [this]() -> int {
            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("SELECT COUNT(*) FROM test WHERE id >= 1000");
            if (query.next()) {
                return query.value(0).toInt();
            }
            return 0;
        },
        *m_executor);

    waitForFuture(countFuture);
    QCOMPARE(countFuture.result(), taskCount);
}

QTEST_MAIN(SqliteDbExecutorTest)
#include "tst_sqliteexecutor.moc"