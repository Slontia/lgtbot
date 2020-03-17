#pragma once
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>
#include <chrono>

class Timer
{
public:
  Timer(const uint64_t sec, const std::function<void()>& handle) : is_over_(false)
  {
    thread_ = std::thread([this, sec, handle]()
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_for(lock, std::chrono::seconds(sec), [this]() { return is_over_.load(); });
      if (!is_over_)
      {
        std::thread t(handle);
        t.detach();
      }
    }); /* make sure thread_ is inited last */
  }
  ~Timer()
  {
    if (thread_.joinable())
    {
      is_over_.store(true);
      cv_.notify_all();
      thread_.join();
    }
  }

private:
  std::condition_variable cv_;
  std::mutex mutex_;
  std::atomic<bool> is_over_;
  std::thread thread_;
};
