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

  protected:
    template <typename T> void waitForFuture(const QFuture<T> &future) {
        if (future.isFinished()) {
            return;
        }
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
    if (!m_tempFile.open()) {
        QFAIL("Failed to init test");
    }
    m_dbPath = m_tempFile.fileName();
    m_tempFile.close();

    m_executor = std::make_unique<SqliteDbExecutor>(m_dbPath);

    auto future = GLogConcurrent::makeFuture(
        [this]() {
            auto &db = m_executor->database();
            QSqlQuery query(db);
            if (!query.exec(
                    "CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, value TEXT)")) {
                throw std::runtime_error("Failed to create table: " +
                                         query.lastError().text().toStdString());
            }
            if (!query.exec("DELETE FROM test")) {
                throw std::runtime_error("Failed to delete table: " +
                                         query.lastError().text().toStdString());
            }
        },
        *m_executor);

    waitForFuture(future);
    QVERIFY2(!future.isCanceled(), "Initialization Canceled");
    if (future.isValid()) {
        try {
            future.waitForFinished();
        } catch (const std::exception &e) {
            QFAIL((QString("Failed to init: ") + e.what()).toUtf8().data());
        }
    }
}

void SqliteDbExecutorTest::cleanupTestCase() { m_executor.reset(); }

void SqliteDbExecutorTest::test_basicAsync() {
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
    auto insertFuture = GLogConcurrent::makeFuture(
        [this]() {
            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.prepare("INSERT INTO test (id, value) VALUES (?, ?)");
            query.addBindValue(1);
            query.addBindValue("hello");
            if (!query.exec()) {
                throw std::runtime_error("Failed: " + query.lastError().text().toStdString());
            }
            return query.lastInsertId().toInt();
        },
        *m_executor);

    waitForFuture(insertFuture);
    QCOMPARE(insertFuture.result(), 1);

    auto queryFuture = GLogConcurrent::makeFuture(
        [this]() -> QString {
            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("SELECT value FROM test WHERE id = 1");
            if (query.next()) {
                return query.value(0).toString();
            }
            return {};
        },
        *m_executor);

    waitForFuture(queryFuture);
    QCOMPARE(queryFuture.result(), QString("hello"));
}

void SqliteDbExecutorTest::test_threadAffinity() {
    QThread *executorThread = nullptr;

    auto getThreadFuture = GLogConcurrent::makeFuture(
        [&executorThread]() { executorThread = QThread::currentThread(); }, *m_executor);
    waitForFuture(getThreadFuture);
    QVERIFY(executorThread != nullptr);

    auto secondFuture = GLogConcurrent::makeFuture(
        [executorThread]() { QCOMPARE(QThread::currentThread(), executorThread); }, *m_executor);
    waitForFuture(secondFuture);
    QVERIFY(secondFuture.isFinished());
}

void SqliteDbExecutorTest::test_nestedTransactionSuccess() {
    auto future = GLogConcurrent::makeFuture(
        [this]() {
            auto trans1 = m_executor->createTransaction();

            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("INSERT INTO test (id, value) VALUES (10, 'outer')");

            {
                auto trans2 = m_executor->createTransaction();
                query.exec("INSERT INTO test (id, value) VALUES (11, 'inner')");
                bool innerCommitted = trans2.commit();
                if (!innerCommitted) {
                    throw std::runtime_error("inner transaction failed");
                }
            }

            bool outerCommitted = trans1.commit();
            if (!outerCommitted) {
                throw std::runtime_error("outer transaction failed");
            }
        },
        *m_executor);

    waitForFuture(future);
    QVERIFY(future.isFinished());

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
    auto future = GLogConcurrent::makeFuture(
        [this]() {
            auto trans1 = m_executor->createTransaction();

            auto &db = m_executor->database();
            QSqlQuery query(db);
            query.exec("INSERT INTO test (id, value) VALUES (20, 'should_rollback')");

            {
                auto trans2 = m_executor->createTransaction();
                query.exec("INSERT INTO test (id, value) VALUES (21, 'inner_rollback')");
                trans2.setRollbackOnly();
            }

            bool committed = trans1.commit();
            return committed;
        },
        *m_executor);

    waitForFuture(future);
    QVERIFY(future.isFinished());
    QCOMPARE(future.result(), false);

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
    auto future = GLogConcurrent::makeFuture(
        [this]() {
            {
                auto trans = m_executor->createTransaction();
                auto &db = m_executor->database();
                QSqlQuery query(db);
                query.exec("INSERT INTO test (id, value) VALUES (30, 'raii_rollback')");
            }

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
    auto future = GLogConcurrent::makeFuture(
        [this]() {
            throw std::runtime_error("test exception");
            return 0;
        },
        *m_executor);

    waitForFuture(future);
    QVERIFY(future.isFinished());

    bool exceptionCaught = false;
    try {
        future.waitForFinished();
    } catch (const std::exception &e) {
        exceptionCaught = true;
        QCOMPARE(QString(e.what()), QString("test exception"));
    }
    QVERIFY(exceptionCaught);
}

void SqliteDbExecutorTest::test_concurrentPressure() {
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
                    throw std::runtime_error("Failed: " + query.lastError().text().toStdString());
                }
            },
            *m_executor);
        futures.append(future);
    }

    for (auto &f : futures) {
        waitForFuture(f);
        QVERIFY(!f.isCanceled());
    }

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