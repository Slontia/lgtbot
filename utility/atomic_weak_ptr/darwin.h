// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <memory>
#include <pthread.h>

// libc++ on Apple platforms does not provide std::atomic<std::weak_ptr> yet.
// MsgSender::Flush is read-heavy (Lock), SetMatch is rare (Store/Exchange), so use a
// reader-writer lock instead of a plain mutex.
template <typename T>
class AtomicWeakPtr
{
  public:
    AtomicWeakPtr() { pthread_rwlock_init(&rw_, nullptr); }

    explicit AtomicWeakPtr(std::weak_ptr<T> wk) : wk_(std::move(wk)) { pthread_rwlock_init(&rw_, nullptr); }

    AtomicWeakPtr(const AtomicWeakPtr&) = delete;
    AtomicWeakPtr& operator=(const AtomicWeakPtr&) = delete;

    ~AtomicWeakPtr() { pthread_rwlock_destroy(&rw_); }

    void Store(std::weak_ptr<T> wk)
    {
        WriteGuard guard(rw_);
        wk_ = std::move(wk);
    }

    std::weak_ptr<T> Exchange(std::weak_ptr<T> wk)
    {
        WriteGuard guard(rw_);
        auto old = std::move(wk_);
        wk_ = std::move(wk);
        return old;
    }

    std::shared_ptr<T> Lock() const
    {
        ReadGuard guard(rw_);
        return wk_.lock();
    }

  private:
    class ReadGuard
    {
      public:
        explicit ReadGuard(pthread_rwlock_t& rw) : rw_(rw) { pthread_rwlock_rdlock(&rw_); }
        ~ReadGuard() { pthread_rwlock_unlock(&rw_); }

      private:
        pthread_rwlock_t& rw_;
    };

    class WriteGuard
    {
      public:
        explicit WriteGuard(pthread_rwlock_t& rw) : rw_(rw) { pthread_rwlock_wrlock(&rw_); }
        ~WriteGuard() { pthread_rwlock_unlock(&rw_); }

      private:
        pthread_rwlock_t& rw_;
    };

    mutable pthread_rwlock_t rw_;
    std::weak_ptr<T> wk_;
};
