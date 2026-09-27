// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <thread>

namespace lgtbot::core {

class Timer
{
   public:
    using TaskSet = std::list<std::pair<uint64_t, std::function<void(uint64_t)>>>;

    // Runs each task synchronously on `thread_` after its scheduled delay.
    //
    // Two design points worth noting:
    //
    // 1. Tick timing is anchored to a monotonic start point (`steady_clock`) so that
    //    the wall time at which task N fires is `start + sum(sec[0..N])` — independent
    //    of how long previous handlers ran. Under the older `wait_for` scheme, each
    //    tick's delay was measured from when the previous handler *finished*, causing
    //    accumulated drift proportional to handler execution time.
    //
    // 2. The handler is invoked synchronously on `thread_` (not on a detached child).
    //    Safety of this depends on the caller-side invariant that the handler must
    //    not synchronously trigger this Timer's own destruction (otherwise ~Timer
    //    would join its own thread — a self-deadlock). In this project that invariant
    //    is upheld by routing the terminal `Match::Unbind_` produced inside the
    //    handler through `MatchManager::ScheduleUnbind`, which runs on a separate
    //    worker thread; see the comment on `MatchManager::ScheduleUnbind`.
    Timer(TaskSet&& tasks) : is_over_(false)
    {
        thread_ = std::jthread([this, t = std::move(tasks)]() {
            std::unique_lock<std::mutex> lock(mutex_);
            const auto start = std::chrono::steady_clock::now();
            auto deadline = start;
            for (const auto& [sec, handle] : t) {
                deadline += std::chrono::seconds(sec);
                cv_.wait_until(lock, deadline, [this]() {
#ifdef TEST_BOT
                    return is_over_.load() || skip_timer_;
#else
                    return is_over_.load();
#endif
                });
                if (is_over_) {
                    break;
                }
                // Release the lock so the handler, which typically acquires
                // Match::data_.lock() and issues IPC, does not contend with
                // ~Timer's brief mutex_ hold used to signal is_over_.
                lock.unlock();
                handle(sec);
                lock.lock();
            }
        }); /* make sure thread_ is inited last */
    }
    Timer(const Timer&) = delete;
    Timer(Timer&&) = delete;
    ~Timer()
    {
        // Signal the loop thread to abandon its remaining tasks; the jthread member
        // then requests stop and joins it on destruction, so no manual join is needed.
        RequestStop();
    }

    // Wake the loop thread and tell it to abandon its remaining tasks. Safe to call
    // from any thread, including the loop thread itself.
    void RequestStop()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            is_over_.store(true);
        }
        cv_.notify_all();
    }

#ifdef TEST_BOT
    static std::condition_variable cv_;
    static bool skip_timer_;
    static std::mutex mutex_;

  private:
#else
  private:
    std::condition_variable cv_;
    std::mutex mutex_;
#endif
    std::atomic<bool> is_over_;
    std::jthread thread_;
};

} // namespace lgtbot::core
