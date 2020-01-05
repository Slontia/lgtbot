#pragma once
#include <atomic>

class SpinLock
{
public:
  void Lock() { while (flag.test_and_set(std::memory_order_acquire)); }
  void Unlock() { flag.clear(std::memory_order_release); }

 private:
   std::atomic_flag flag_ = ATOMIC_VAR_INIT;
};

class SpinLockGuard
{
public:
  SpinLockGuard(SpinLock& lock) : lock_(lock) { lock_.Lock(); }
  ~SpinLockGuard() { lock_.Unlock(); }

private:
  SpinLock& lock_;
};
