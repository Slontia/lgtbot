// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <thread>
#include <iostream> //

class Timer
{
   public:
    using TaskSet = std::list<std::pair<uint64_t, std::function<void()>>>;
    Timer(TaskSet&& tasks) : is_over_(false)
    {
#ifdef TEST_BOT
        {
            std::lock_guard<std::mutex> l(Timer::mutex_);
            ++remaining_thread_count_;
        }
#endif
        thread_ = std::thread([this, t = std::move(tasks)]()
            {
                std::unique_lock<std::mutex> lock(mutex_);
                for (const auto& [sec, handle] : t) {
                    cv_.wait_for(lock, std::chrono::seconds(sec), [this]()
                            {
#ifdef TEST_BOT
                                return is_over_.load() || skip_timer_;
#else
                                return is_over_.load();
#endif
                            });
                    if (!is_over_) {
                        // We need call handle async, because handle may release timer which will cause deadlock
#ifdef TEST_BOT
                        ++remaining_thread_count_;
#endif
                        std::thread([handle]
                                {
                                    handle();
#ifdef TEST_BOT
                                    std::lock_guard<std::mutex> l(Timer::mutex_);
                                    if (0 == --remaining_thread_count_) {
                                        remaining_thread_cv_.notify_all();
                                    }
#endif
                                }).detach();
                    } else {
                        break;
                    }
                }
#ifdef TEST_BOT
                if (0 == --remaining_thread_count_) {
                    remaining_thread_cv_.notify_all();
                }
#endif
            }); /* make sure thread_ is inited last */
    }
    Timer(const Timer&) = delete;
    Timer(Timer&&) = delete;
    ~Timer()
    {
        if (thread_.joinable()) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                is_over_.store(true);
            }
            cv_.notify_all();
            thread_.join();
        }
    }

#ifdef TEST_BOT
    static std::condition_variable cv_;
    static bool skip_timer_;
    static std::condition_variable remaining_thread_cv_;
    static uint64_t remaining_thread_count_;
    static std::mutex mutex_;

  private:
#else
  private:
    std::condition_variable cv_;
    std::mutex mutex_;
#endif
    std::atomic<bool> is_over_;
    std::thread thread_;
};
