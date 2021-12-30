// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once
#include <atomic>

class SpinLock
{
   public:
    void lock()
    {
        while (flag_.test_and_set(std::memory_order_acquire))
            ;
    }
    void unlock() { flag_.clear(std::memory_order_release); }

   private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};
/*
class SpinLockGuard
{
public:
  SpinLockGuard(SpinLock& lock) : lock_(lock) { lock_.Lock(); }
  ~SpinLockGuard() { lock_.Unlock(); }

private:
  SpinLock& lock_;
};
*/
