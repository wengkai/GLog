#ifndef CONCURRENT_H
#define CONCURRENT_H

#include <functional>
#include <type_traits>
#include <QFuture>
#include <QPromise>
#include <QThread>
#include <QThreadPool>
#include <exception>

namespace GLogConcurrent {

struct DefaultExecutor 
{
    template <typename Callable>
    inline void start(Callable &&functionToRun) {
        QThreadPool::globalInstance()->start(std::forward<Callable>(functionToRun));
    }
};

template <typename Executor, typename Callable, typename = void>
struct Has_Member_start : std::false_type {};

template <typename Executor, typename Callable>
struct Has_Member_start<Executor, Callable, std::void_t<decltype(std::declval<Executor>().start(std::declval<Callable>()))>> : std::true_type {};

struct Exec_With_Context_T{};
template<typename Func, typename PromiseType, typename = std::enable_if_t<std::is_invocable_v<Func, PromiseType&>>>
inline void _executeTask(Func&& f, PromiseType& promise, Exec_With_Context_T = {}) {
    std::invoke(std::forward<Func>(f), promise); 
}

struct Exec_With_Return_And_No_Context_T{};
template<typename Func, typename PromiseType, typename = std::enable_if_t<!std::is_invocable_v<Func, PromiseType&> && !std::is_void_v<std::invoke_result_t<Func>>>>
inline void _executeTask(Func&& f, PromiseType& promise, Exec_With_Return_And_No_Context_T = {}) {
    promise.addResult(std::invoke(std::forward<Func>(f)));
}

struct Exec_Without_Return_And_No_Context_T{};
template<typename Func, typename PromiseType, typename = std::enable_if_t<!std::is_invocable_v<Func, PromiseType&> && std::is_void_v<std::invoke_result_t<Func>>>>
inline void _executeTask(Func&& f, PromiseType& promise, Exec_Without_Return_And_No_Context_T = {}) {
    std::invoke(std::forward<Func>(f));
}

struct Make_Future_Without_Context_T{};
template<typename Func, typename Executor = DefaultExecutor, typename = std::enable_if_t<std::is_invocable_v<Func>>>
inline auto makeFuture(Func &&f, Executor exec = {}, Make_Future_Without_Context_T = {})
{
    static_assert(Has_Member_start<Executor, Func>::value, "Executor must have Executor::start(Func)");
    using DecayedFunction = typename std::decay_t<Func>;
    QPromise<std::invoke_result_t<DecayedFunction>> promise;
    auto ret = promise.future();
    exec.start([f = static_cast<DecayedFunction>(std::forward<Func>(f)), promise = (std::move(promise))]() mutable {
        try {
            promise.start();
            if (promise.isCanceled()) {
                promise.finish();
                return;
            }
            _executeTask(f, promise);
            promise.finish();
        } catch (...) {
            promise.setException(std::current_exception());
            promise.finish();
        }
    });
    return ret;
}

struct Make_Future_With_Context_T{};
template<typename Type, typename Func, typename Executor = DefaultExecutor, typename = std::enable_if_t<std::is_invocable_v<Func, QPromise<Type>&>>>
inline auto makeFuture(Func &&f, Executor exec = {}, Make_Future_With_Context_T = {})
{
    static_assert(Has_Member_start<Executor, Func>::value, "Executor must have Executor::start(Func)");
    using DecayedFunction = typename std::decay_t<Func>;
    using ReturnType = std::invoke_result_t<DecayedFunction, QPromise<Type>&>;
    static_assert(
        std::is_same_v<void, ReturnType>, 
        "Bad Runable.");
    QPromise<Type> promise;
    auto ret = promise.future();
    exec.start([f = static_cast<DecayedFunction>(std::forward<Func>(f)), promise = (std::move(promise))]() mutable {
        try {
            promise.start();
            if (promise.isCanceled()) {
                promise.finish();
                return;
            }
            _executeTask(f, promise);
            promise.finish();
        } catch (...) {
            promise.setException(std::current_exception());
            promise.finish();
        }
    });
    return ret;
}

template<typename T>
struct Promise_Traits;

template<typename T>
struct Promise_Traits<QPromise<T>> {
    using type = T;
};

template<typename T>
using Promise_Traits_T = typename Promise_Traits<T>::type;

template<typename T>
struct Function_Traits;

template<typename ReturnType, typename PromiseType>
struct Function_Traits<ReturnType(PromiseType&)> {
    using type = Promise_Traits_T<PromiseType>;
};

template<typename ReturnType, typename PromiseType>
struct Function_Traits<ReturnType(*)(PromiseType&)> {
    using type = Promise_Traits_T<PromiseType>;
};

template<typename Class>
struct Function_Traits : Function_Traits<decltype(&Class::operator())> {}; 

template<typename Class, typename ReturnType, typename PromiseType>
struct Function_Traits<ReturnType(Class::*)(PromiseType&)> { 
    using type = Promise_Traits_T<PromiseType>;
};

template<typename Class, typename ReturnType, typename PromiseType>
struct Function_Traits<ReturnType(Class::*)(PromiseType&) const> { 
    using type = Promise_Traits_T<PromiseType>;
};

template<typename Func>
using Function_Traits_T = typename Function_Traits<Func>::type;

template<typename Func, typename Executor = DefaultExecutor, typename = std::enable_if_t<!std::is_invocable_v<Func>>>
inline auto makeFuture(Func &&f, Executor exec = {})
{
    return makeFuture<Function_Traits_T<std::decay_t<Func>>>(std::forward<Func>(f), exec, Make_Future_With_Context_T{});
}

}

#endif
