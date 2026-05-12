#ifndef SYNCHRONIZE_H
#define SYNCHRONIZE_H

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

template <typename T> class Unsynchronized {

  public:
    using MyType = std::remove_reference_t<std::remove_const_t<T>>;
    using MyConstType = const MyType;
    Unsynchronized() = default;
    template <typename U> Unsynchronized(U &&v) : value(std::forward<U>(v)) {}
    template <typename F> auto write(F &&func) -> std::invoke_result_t<F, MyType &> {
        return std::invoke(func, value);
    }

    template <typename F> auto read(F &&func) const -> std::invoke_result_t<F, MyConstType &> {
        return std::invoke(func, value);
    }

    MyType *requireWrite() { return &value; }
    const MyType *requireRead() const { return &value; }

  protected:
    MyType value;
};

template <typename T> class Synchronized : private Unsynchronized<T> {
    using MyBase = Unsynchronized<T>;

  public:
    using MyType = typename MyBase::MyType;
    using MyConstType = typename MyBase::MyConstType;
    using MyBase::MyBase;
    template <typename F> auto write(F &&func) -> std::invoke_result_t<F, MyType &> {
        std::unique_lock guard(m_lock);
        return MyBase::write(std::forward<F>(func));
    }

    template <typename F> auto read(F &&func) const -> std::invoke_result_t<F, MyConstType &> {
        std::shared_lock guard(m_lock);
        return MyBase::read(std::forward<F>(func));
    }

    class WriteProxy {
        std::unique_lock<std::shared_mutex> lock;
        T &ref;

      public:
        WriteProxy(std::shared_mutex &m, T &v) : lock(m), ref(v) {}
        T *operator->() { return &ref; }
        T &operator*() { return ref; }
    };

    class ReadProxy {
        std::shared_lock<std::shared_mutex> lock;
        const T &ref;

      public:
        ReadProxy(std::shared_mutex &m, const T &v) : lock(m), ref(v) {}
        const T *operator->() const { return &ref; }
        const T &operator*() const { return ref; }
    };

    [[nodiscard]] WriteProxy requireWrite() { return {m_lock, MyBase::value}; }
    [[nodiscard]] ReadProxy requireRead() const { return {m_lock, MyBase::value}; }

  private:
    mutable std::shared_mutex m_lock;
};

#endif