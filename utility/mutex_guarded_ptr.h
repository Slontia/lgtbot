// Copyright (c) 2023-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <chrono>
#include <mutex>
#include <utility>

enum class mutex_guard_lock_type { unique_mutable, unique_const, shared_const };

template <mutex_guard_lock_type k_type>
struct mutex_guard_lock_helper;

template <>
struct mutex_guard_lock_helper<mutex_guard_lock_type::shared_const>
{
    static void lock(auto& mutex) { mutex.lock_shared(); }

    static bool try_lock(auto& mutex) { return mutex.try_lock_shared(); }

    template <typename Rep, class Period>
    static bool try_lock(auto& mutex, const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        return mutex.try_lock_shared_for(timeout_duration);
    }

    template <typename Clock, class Duration>
    static bool try_lock(auto& mutex, const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        return mutex.try_lock_shared_until(timeout_time);
    }

    static void unlock(auto& mutex) { mutex.unlock_shared(); }
};

template <mutex_guard_lock_type k_type>
requires (k_type == mutex_guard_lock_type::unique_mutable || k_type == mutex_guard_lock_type::unique_const)
struct mutex_guard_lock_helper<k_type>
{
    static void lock(auto& mutex) { mutex.lock(); }

    static bool try_lock(auto& mutex) { return mutex.try_lock(); }

    template <typename Rep, class Period>
    static bool try_lock(auto& mutex, const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        return mutex.try_lock_for(timeout_duration);
    }

    template <typename Clock, class Duration>
    static bool try_lock(auto& mutex, const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        return mutex.try_lock_until(timeout_time);
    }

    static void unlock(auto& mutex) { mutex.unlock(); }
};

// RAII guard: caller acquires the lock, then passes mutex and object references.
// Default-constructed guard holds null pointers and releases nothing.
template <typename T, typename Mutex, mutex_guard_lock_type k_type>
class mutex_guarded_ptr
{
    template <typename, typename, mutex_guard_lock_type>
    friend class mutex_guarded_ptr;

  public:
    mutex_guarded_ptr() noexcept = default;
    mutex_guarded_ptr(std::nullptr_t) noexcept : mutex_guarded_ptr{} {}

    mutex_guarded_ptr(Mutex& mutex, T& obj) noexcept : mutex_(&mutex), obj_(&obj) {}

    mutex_guarded_ptr(const mutex_guarded_ptr&) = delete;

    mutex_guarded_ptr(const mutex_guarded_ptr& other)
        requires (k_type == mutex_guard_lock_type::shared_const)
        : mutex_guarded_ptr{other.mutex_, other.obj_}
    {
        if (mutex_) {
            mutex_guard_lock_helper<k_type>::lock(*mutex_);
        }
    }

    mutex_guarded_ptr(mutex_guarded_ptr&& other) noexcept { swap(other); }

    mutex_guarded_ptr(mutex_guarded_ptr< T, Mutex, mutex_guard_lock_type::unique_mutable>&& other) noexcept
        requires (k_type == mutex_guard_lock_type::unique_const)
        : mutex_guarded_ptr{other.mutex_, other.obj_}
    {
        other.mutex_ = nullptr;
        other.obj_ = nullptr;
    }

    ~mutex_guarded_ptr() { reset(); }

    operator bool() const noexcept { return mutex_ != nullptr; }
    bool operator==(std::nullptr_t) const noexcept { return mutex_ == nullptr; }

    mutex_guarded_ptr& operator=(const mutex_guarded_ptr&) = delete;

    mutex_guarded_ptr& operator=(mutex_guarded_ptr&& other) noexcept
    {
        mutex_guarded_ptr(std::move(other)).swap(*this);
        return *this;
    }

    mutex_guarded_ptr& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    T& operator*() const noexcept { return *obj_; }
    T* operator->() const noexcept { return obj_; }

    void reset() noexcept
    {
        if (mutex_) {
            mutex_guard_lock_helper<k_type>::unlock(*mutex_);
            mutex_ = nullptr;
            obj_ = nullptr;
        }
    }

    void swap(mutex_guarded_ptr& other) noexcept
    {
        std::swap(mutex_, other.mutex_);
        std::swap(obj_, other.obj_);
    }

  private:
    Mutex* mutex_{nullptr};
    T* obj_{nullptr};
};
