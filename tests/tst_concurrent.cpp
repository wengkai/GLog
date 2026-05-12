// tst_concurrent.cpp
#include <QtTest>
#include <QAtomicInt>
#include <QFutureWatcher>
#include <QThread>
#include <chrono>
#include <functional>
#include <thread>

// 引入待测头文件
#include "Concurrent.h"

using namespace GLogConcurrent;

// 自定义 Executor 用于测试，记录 start 调用次数
struct SpyExecutor {
    QAtomicInt callCount{0};
    std::function<void()> lastTask;

    template <typename Callable> void start(Callable &&f) {
        callCount.ref();
        // 立即在后台执行（模拟线程池）
        QThreadPool::globalInstance()->start(std::forward<Callable>(f));
    }
};

class TestGLogConcurrent : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase() {
        // 可预留一些初始化操作
    }

    void cleanupTestCase() {
        // 等待所有后台线程结束
        QThreadPool::globalInstance()->waitForDone();
    }

    // 基本功能：无参数，返回 int
    void testSimpleReturn() {
        auto future = makeFuture([]() -> int {
            QThread::msleep(50);
            return 42;
        });

        QFutureWatcher<int> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<int>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QVERIFY(future.isFinished());
        QCOMPARE(future.result(), 42);
    }

    // 基本功能：void 返回类型
    void testVoidReturn() {
        QAtomicInt flag{0};
        auto future = makeFuture([&flag]() {
            QThread::msleep(50);
            flag = 1;
        });

        QFutureWatcher<void> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<void>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QVERIFY(future.isFinished());
        QCOMPARE(flag.loadRelaxed(), 1);
    }

    // 带 QPromise<int> 上下文的函数：手动添加结果
    void testPromiseManualResult() {
        auto future = makeFuture<int>([](QPromise<int> &promise) {
            QThread::msleep(50);
            promise.addResult(100);
            promise.addResult(200);
        });

        QFutureWatcher<int> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<int>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QVERIFY(future.isFinished());
        QCOMPARE(future.resultCount(), 2);
        QCOMPARE(future.resultAt(0), 100);
        QCOMPARE(future.resultAt(1), 200);
    }

    // 带 QPromise<int> 上下文的函数：使用进度报告
    void testPromiseProgress() {
        QFutureWatcher<int> watcher;
        QSignalSpy resultSpy(&watcher, &QFutureWatcher<int>::resultReadyAt);
        QSignalSpy progressSpy(&watcher, &QFutureWatcher<int>::progressValueChanged);
        QSignalSpy finishedSpy(&watcher, &QFutureWatcher<int>::finished);

        watcher.setFuture(makeFuture<int>([](QPromise<int> &promise) {
            promise.start(); // 显式启动（内部也会调用）
            for (int i = 1; i <= 5; ++i) {
                if (promise.isCanceled()) {
                    return;
                }
                QThread::msleep(20);
                promise.setProgressValue(i * 20);
                promise.addResult(i);
            }
        }));

        auto future = watcher.future();

        finishedSpy.wait(1000);
        QVERIFY(future.isFinished());
        QCOMPARE(future.resultCount(), 5);
        QVERIFY(progressSpy.count() > 0);
        QCOMPARE(future.progressValue(), 100);
    }

    // 异常处理（根据 Qt 源码，reportException 会设置 Canceled 状态）
    void testException() {
        auto future = makeFuture([]() -> int {
            QThread::msleep(50);
            throw std::runtime_error("Test exception");
            return 0;
        });

        QFutureWatcher<int> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<int>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QVERIFY(future.isFinished());
        QVERIFY(future.isCanceled()); // Qt 内部在报告异常时会同时设置 Canceled 标志
        try {
            future.result(); // 应当抛出异常
            QFAIL("Expected exception not thrown");
        } catch (const std::runtime_error &e) {
            QCOMPARE(QString(e.what()), QString("Test exception"));
        } catch (...) {
            QFAIL("Unexpected exception type");
        }
    }

    // 取消操作：在任务执行中取消
    void testCancel() {
        QPromise<int> *capturedPromise = nullptr;

        auto future = makeFuture<int>([&capturedPromise](QPromise<int> &promise) {
            capturedPromise = &promise;
            promise.start();
            for (int i = 0; i < 10; ++i) {
                if (promise.isCanceled()) {
                    return;
                }
                QThread::msleep(50);
                promise.addResult(i);
            }
        });

        QFutureWatcher<int> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<int>::finished);
        watcher.setFuture(future);

        // 等待任务启动
        QTest::qWait(30);
        QVERIFY(capturedPromise != nullptr);

        future.cancel();
        spy.wait(200);
        QVERIFY(future.isFinished());
        QVERIFY(future.isCanceled());
        QVERIFY(future.resultCount() < 10); // 未完成所有迭代
    }

    // 测试默认 Executor (QThreadPool)
    void testDefaultExecutor() {
        auto future = makeFuture([]() -> QString {
            QThread::msleep(50);
            return QStringLiteral("default");
        });

        QFutureWatcher<QString> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<QString>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QVERIFY(future.isFinished());
        QCOMPARE(future.result(), QStringLiteral("default"));
    }

    // 测试自定义 Executor
    void testCustomExecutor() {
        SpyExecutor spyExec;

        auto future = makeFuture(
            []() -> int {
                QThread::msleep(50);
                return 77;
            },
            spyExec);

        QFutureWatcher<int> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<int>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QCOMPARE(spyExec.callCount.loadRelaxed(), 1);
        QVERIFY(future.isFinished());
        QCOMPARE(future.result(), 77);
    }

    // 测试函数对象（有 operator()）
    void testFunctionObject() {
        struct Functor {
            int operator()() const {
                QThread::msleep(50);
                return 123;
            }
        };

        Functor f;
        auto future = makeFuture(f);

        QFutureWatcher<int> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<int>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QVERIFY(future.isFinished());
        QCOMPARE(future.result(), 123);
    }

    // 测试通过函数签名自动推导类型（重载 makeFuture 中无标签的情况）
    void testAutoDeductionViaSignature() {
        // 定义一个接受 QPromise<QString> & 的函数对象
        auto task = [](QPromise<QString> &p) {
            p.addResult("hello");
            p.addResult("world");
        };

        // 使用不带标签的 makeFuture，应自动推导为 makeFuture<QString>
        auto future = makeFuture(task);

        QFutureWatcher<QString> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<QString>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QVERIFY(future.isFinished());
        QCOMPARE(future.resultCount(), 2);
        QCOMPARE(future.resultAt(0), QStringLiteral("hello"));
        QCOMPARE(future.resultAt(1), QStringLiteral("world"));
    }

    // 测试静态断言：Executor 无 start 方法时编译失败（此处仅验证可以编译通过，不写负向测试）
    void testStaticAssertionsPass() {
        // 默认 Executor 有 start 方法，能通过断言
        auto future1 = makeFuture([]() { return 1; });
        QVERIFY(!future1.isCanceled());

        // SpyExecutor 有 start 方法，能通过
        SpyExecutor e;
        auto future2 = makeFuture([]() { return 2; }, e);
        QVERIFY(!future2.isCanceled());
    }

    // 测试任务中多次调用 addResult
    void testMultipleResults() {
        auto future = makeFuture<QString>([](QPromise<QString> &p) {
            for (int i = 0; i < 3; ++i) {
                p.addResult(QString("item%1").arg(i));
                QThread::msleep(10);
            }
        });

        QFutureWatcher<QString> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<QString>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QVERIFY(future.isFinished());
        QCOMPARE(future.results(), QVector<QString>({"item0", "item1", "item2"}));
    }

    // 压力测试：并发多个任务
    void testConcurrentMultipleTasks() {
        const int taskCount = 20;
        QList<QFuture<int>> futures;
        SpyExecutor exec;

        for (int i = 0; i < taskCount; ++i) {
            futures.append(makeFuture(
                [i]() -> int {
                    QThread::msleep(20);
                    return i * i;
                },
                exec));
        }

        for (int i = 0; i < taskCount; ++i) {
            QFutureWatcher<int> watcher;
            QSignalSpy spy(&watcher, &QFutureWatcher<int>::finished);
            watcher.setFuture(futures[i]);
            spy.wait(1000);
            QVERIFY(futures[i].isFinished());
            QCOMPARE(futures[i].result(), i * i);
        }

        QCOMPARE(exec.callCount.loadRelaxed(), taskCount);
    }

    // 测试在任务中抛出异常后 promise.finish 仍被调用（Finalizer 保证）
    // 同时根据 Qt 源码，异常会设置 Canceled 状态
    void testExceptionStillFinishesPromise() {
        auto future = makeFuture<int>([](QPromise<int> &p) {
            p.start();
            throw std::runtime_error("error in promise task");
        });

        QFutureWatcher<int> watcher;
        QSignalSpy spy(&watcher, &QFutureWatcher<int>::finished);
        watcher.setFuture(future);

        spy.wait(500);
        QVERIFY(future.isFinished());
        QVERIFY(future.isCanceled()); // 异常导致取消状态
        bool caught = false;
        try {
            future.result();
        } catch (const std::runtime_error &) {
            caught = true;
        }
        QVERIFY(caught);
    }
};

QTEST_MAIN(TestGLogConcurrent)
#include "tst_concurrent.moc"