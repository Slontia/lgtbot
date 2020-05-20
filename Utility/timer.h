#pragma once
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>
#include <chrono>
#include <list>

class Timer
{
public:
  using TaskSet = std::list<std::pair<uint64_t, std::function<void()>>>;
  Timer(const uint64_t sec, std::function<void()>&& handle) : Timer({ { sec, std::move(handle) } }) {}
  Timer(TaskSet&& tasks) : is_over_(false)
  {
    thread_ = std::thread([this, t = std::move(tasks)]()
    {
      std::unique_lock<std::mutex> lock(mutex_);
      for (const auto& [sec, handle] : t)
      {
				cv_.wait_for(lock, std::chrono::seconds(sec), [this]() { return is_over_.load(); });
				if (!is_over_)
				{
					std::thread t(handle);
					t.detach();
				}
        else
        {
          break;
        }
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




