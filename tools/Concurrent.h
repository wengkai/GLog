#ifndef CONCURRENT_H
#define CONCURRENT_H

#include <QCoreApplication>
#include <QFuture>
#include <QPointer>
#include <QPromise>
#include <QThread>
#include <QThreadPool>
#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

namespace GLogConcurrent {

struct DefaultExecutor {
    template <typename Callable> inline void start(Callable &&functionToRun) {
        QThreadPool::globalInstance()->start(std::forward<Callable>(functionToRun));
    }
};

struct MainThreadExecutor {
    template <typename Callable> inline void start(Callable &&functionToRun) {
        QMetaObject::invokeMethod(QCoreApplication::instance(),
                                  std::forward<Callable>(functionToRun), Qt::AutoConnection);
    }
};

/**
 * Run @p f on the GUI thread and block the caller until it finishes (MainThreadExecutor +
 * waitForFinished). Safe from worker threads; on the GUI thread the callable runs inline.
 * If you call from the GUI thread while it is blocked waiting on the calling thread, you
 * may deadlock.
 */
template <class F> inline void runOnMainThreadSync(F &&f) {
    makeFuture(std::forward<F>(f), MainThreadExecutor{});
}

class FIFOBackendThreadExecutor {

    QThreadPool m_thread_pool;

  public:
    explicit FIFOBackendThreadExecutor() { m_thread_pool.setMaxThreadCount(1); }
    template <typename Callable> inline void start(Callable &&functionToRun) {
        m_thread_pool.start(std::forward<Callable>(functionToRun));
    }
    // destructor will process events until all tasks are finished
    ~FIFOBackendThreadExecutor() { waitForDone(); }
    void waitForDone() { m_thread_pool.waitForDone(); }
    void waitForDoneAndProcessEvents() {
        while (m_thread_pool.activeThreadCount() > 0) {
            QCoreApplication::processEvents();
        }
    }
};

// INTERNAL_MAKE_TASK+_executeTask
template <typename Func, typename T> inline auto _createRunner(Func &&f, QPromise<T> &&promise) {
    return [f = static_cast<std::decay_t<Func>>(std::forward<Func>(f)),
            promise = std::move(promise)]() mutable {
        {
            struct Finalizer {
                QPromise<T> &p;
                ~Finalizer() { p.finish(); }
            } finalizer{promise};

            try {
                promise.start();
                if (promise.isCanceled()) return;

                // f = static_cast<std::decay_t<Func>>(std::forward<Func>(f))
                if constexpr (std::is_invocable_v<Func, QPromise<T> &>) {
                    f(promise);
                } else if constexpr (std::is_void_v<std::invoke_result_t<Func>>) {
                    f();
                } else {
                    promise.addResult(f());
                }
            } catch (...) {
                promise.setException(std::current_exception());
            }
        }
    };
}

template <typename Executor> struct ExecTraits {
    template <typename E, typename Func, typename PromiseType>
    static inline void doStart(E &&exec, Func &&f, PromiseType &&promise) {
        exec.start(_createRunner(std::forward<Func>(f), std::move(promise)));
    }
};

template <typename Executor, typename Callable, typename = void>
struct Has_Member_start : std::false_type {};

template <typename Executor, typename Callable>
struct Has_Member_start<
    Executor, Callable,
    std::void_t<decltype(std::declval<Executor &>().start(std::declval<Callable>()))>>
    : std::true_type {};

// template <typename Executor> struct ExecTraits<Executor *> {
//     template <typename E, typename Func, typename PromiseType>
//     static inline void doStart(E *exec, Func &&f, PromiseType &&promise) {
//         exec->start(_createRunner(std::forward<Func>(f), std::move(promise)));
//     }
// };

// template <typename Executor, typename Callable>
// struct Has_Member_start<
//     Executor *, Callable,
//     std::void_t<decltype(std::declval<Executor *>()->start(std::declval<Callable>()))>>
//     : std::true_type {};

struct Make_Future_Without_Context_T {};
template <typename Func, typename Executor = DefaultExecutor,
          typename = std::enable_if_t<std::is_invocable_v<Func>>>
inline auto makeFuture(Func &&f, Executor &&exec = {}, Make_Future_Without_Context_T = {}) {
    static_assert(Has_Member_start<Executor, Func>::value,
                  "Executor must have Executor::start(Func)");
    using DecayedFunction = typename std::decay_t<Func>;
    QPromise<std::invoke_result_t<DecayedFunction>> promise;
    auto ret = promise.future();
    ExecTraits<std::decay_t<Executor>>::doStart(exec, std::forward<Func>(f), std::move(promise));
    return ret;
}

struct Make_Future_With_Context_T {};
template <typename Type, typename Func, typename Executor = DefaultExecutor,
          typename = std::enable_if_t<std::is_invocable_v<Func, QPromise<Type> &>>>
inline auto makeFuture(Func &&f, Executor &&exec = {}, Make_Future_With_Context_T = {}) {
    static_assert(Has_Member_start<Executor, Func>::value,
                  "Executor must have Executor::start(Func)");
    using DecayedFunction = typename std::decay_t<Func>;
    using ReturnType = std::invoke_result_t<DecayedFunction, QPromise<Type> &>;
    static_assert(std::is_same_v<void, ReturnType>, "Bad Runable.");
    QPromise<Type> promise;
    auto ret = promise.future();
    ExecTraits<std::decay_t<Executor>>::doStart(exec, std::forward<Func>(f), std::move(promise));
    return ret;
}

template <typename T> struct Promise_Traits;

template <typename T> struct Promise_Traits<QPromise<T>> {
    using type = T;
};

template <typename T> using Promise_Traits_T = typename Promise_Traits<T>::type;

template <typename T> struct Function_Traits;

template <typename ReturnType, typename PromiseType>
struct Function_Traits<ReturnType(PromiseType &)> {
    using type = Promise_Traits_T<PromiseType>;
};

template <typename ReturnType, typename PromiseType>
struct Function_Traits<ReturnType (*)(PromiseType &)> {
    using type = Promise_Traits_T<PromiseType>;
};

template <typename Class> struct Function_Traits : Function_Traits<decltype(&Class::operator())> {};

template <typename Class, typename ReturnType, typename PromiseType>
struct Function_Traits<ReturnType (Class::*)(PromiseType &)> {
    using type = Promise_Traits_T<PromiseType>;
};

template <typename Class, typename ReturnType, typename PromiseType>
struct Function_Traits<ReturnType (Class::*)(PromiseType &) const> {
    using type = Promise_Traits_T<PromiseType>;
};

template <typename Func> using Function_Traits_T = typename Function_Traits<Func>::type;

template <typename Func, typename Executor = DefaultExecutor,
          typename = std::enable_if_t<!std::is_invocable_v<Func>>>
inline auto makeFuture(Func &&f, Executor &&exec = {}) {
    return makeFuture<Function_Traits_T<std::decay_t<Func>>>(
        std::forward<Func>(f), std::forward<Executor>(exec), Make_Future_With_Context_T{});
}

} // namespace GLogConcurrent

#endif
