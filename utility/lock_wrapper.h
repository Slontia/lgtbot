// Copyright (c) 2023-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <chrono>
#include <utility>

#include "utility/mutex_guarded_ptr.h"

// Wraps T with a mutex. Call lock() / lock_const() / lock_shared() to obtain a RAII guard that
// dereferences the inner object. Neither copyable nor movable.
template <typename T, typename Mutex = std::mutex>
class mutex_protect_wrapper
{
  public:
    using lock_type = mutex_guard_lock_type;
    using locked_ptr = mutex_guarded_ptr<T, Mutex, lock_type::unique_mutable>;
    using const_locked_ptr = mutex_guarded_ptr<const T, Mutex, lock_type::unique_const>;
    using shared_locked_ptr = mutex_guarded_ptr<const T, Mutex, lock_type::shared_const>;

    using object_type = T;
    using mutex_type = Mutex;

    template <typename... Args>
    explicit mutex_protect_wrapper(Args&&... args) : obj_(std::forward<Args>(args)...) {}

    mutex_protect_wrapper(const mutex_protect_wrapper&) = delete;
    mutex_protect_wrapper(mutex_protect_wrapper&&) = delete;
    mutex_protect_wrapper& operator=(const mutex_protect_wrapper&) = delete;
    mutex_protect_wrapper& operator=(mutex_protect_wrapper&&) = delete;

    auto lock() { return lock_<lock_type::unique_mutable>(); }
    auto lock() const { return lock_<lock_type::unique_const>(); }

    auto try_lock() { return try_lock_<lock_type::unique_mutable>(); }
    auto try_lock() const { return try_lock_<lock_type::unique_const>(); }

    template <typename Rep, class Period>
    auto try_lock_for(const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        return try_lock_<lock_type::unique_mutable>(timeout_duration);
    }

    template <typename Clock, class Duration>
    auto try_lock_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        return try_lock_<lock_type::unique_mutable>(timeout_time);
    }

    auto lock_const() { return lock_<lock_type::unique_const>(); }
    auto lock_const() const { return lock_<lock_type::unique_const>(); }

    auto try_lock_const() { return try_lock_<lock_type::unique_const>(); }
    auto try_lock_const() const { return try_lock_<lock_type::unique_const>(); }

    template <typename Rep, class Period>
    auto try_lock_const_for(const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        return try_lock_<lock_type::unique_const>(timeout_duration);
    }

    template <typename Clock, class Duration>
    auto try_lock_const_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        return try_lock_<lock_type::unique_const>(timeout_time);
    }

    auto lock_shared() { return lock_<lock_type::shared_const>(); }
    auto lock_shared() const { return lock_<lock_type::shared_const>(); }

    auto try_lock_shared() { return try_lock_<lock_type::shared_const>(); }
    auto try_lock_shared() const { return try_lock_<lock_type::shared_const>(); }

    template <typename Rep, class Period>
    auto try_lock_shared_for(const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        return try_lock_<lock_type::shared_const>(timeout_duration);
    }

    template <typename Clock, class Duration>
    auto try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        return try_lock_<lock_type::shared_const>(timeout_time);
    }

  private:
    template <lock_type k_type>
    using guarded_ptr = std::conditional_t<k_type == lock_type::unique_mutable, locked_ptr,
            std::conditional_t<k_type == lock_type::unique_const, const_locked_ptr, shared_locked_ptr>>;

    template <lock_type k_type>
    auto lock_()
    {
        mutex_guard_lock_helper<k_type>::lock(mutex_);
        return guarded_ptr<k_type>{mutex_, obj_};
    }

    template <lock_type k_type>
    auto lock_() const
    {
        mutex_guard_lock_helper<k_type>::lock(mutex_);
        return guarded_ptr<k_type>{mutex_, obj_};
    }

    template <lock_type k_type, typename... Args>
    auto try_lock_(Args&&... args)
    {
        if (!mutex_guard_lock_helper<k_type>::try_lock(mutex_, std::forward<Args>(args)...)) {
            return guarded_ptr<k_type>{};
        }
        return guarded_ptr<k_type>{mutex_, obj_};
    }

    template <lock_type k_type, typename... Args>
    auto try_lock_(Args&&... args) const
    {
        if (!mutex_guard_lock_helper<k_type>::try_lock(mutex_, std::forward<Args>(args)...)) {
            return guarded_ptr<k_type>{};
        }
        return guarded_ptr<k_type>{mutex_, obj_};
    }

    mutable Mutex mutex_;
    T obj_;
};
