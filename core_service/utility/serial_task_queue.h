// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>

// A single-threaded serial task queue. Tasks posted via Post() are executed in
// order by a dedicated background thread. The caller blocks on the returned
// future until the task completes.
//
// After Stop() is called, any pending or newly posted tasks have their promises
// broken with a std::runtime_error so callers never block forever.
class SerialTaskQueue
{
  public:
    SerialTaskQueue()
        : stopped_(false)
        , thread_([this] { Run_(); })
    {}

    ~SerialTaskQueue()
    {
        Stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    SerialTaskQueue(const SerialTaskQueue&) = delete;
    SerialTaskQueue& operator=(const SerialTaskQueue&) = delete;

    // Stop accepting new tasks and drain the queue. All pending promises that
    // have not been fulfilled are broken with a runtime_error.
    void Stop()
    {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (stopped_) {
                return;
            }
            stopped_ = true;
        }
        cv_.notify_one();
    }

    // Post a callable and return a future for its result. If the queue has
    // been stopped, the future is immediately broken.
    template <typename Fn>
    auto Post(Fn&& fn) -> std::future<std::invoke_result_t<Fn>>
    {
        using R = std::invoke_result_t<Fn>;
        auto promise = std::make_shared<std::promise<R>>();
        auto fut = promise->get_future();
        auto wrapper = [promise, fn = std::forward<Fn>(fn)]() mutable {
            try {
                if constexpr (std::is_void_v<R>) {
                    fn();
                    promise->set_value();
                } else {
                    promise->set_value(fn());
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        };
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (stopped_) {
                promise->set_exception(std::make_exception_ptr(
                        std::runtime_error("SerialTaskQueue is stopped")));
                return fut;
            }
            queue_.emplace(std::move(wrapper));
        }
        cv_.notify_one();
        return fut;
    }

  private:
    void Run_()
    {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(mutex_);
                cv_.wait(lk, [this] { return stopped_ || !queue_.empty(); });
                if (queue_.empty()) {
                    // stopped_ is true and queue is drained
                    return;
                }
                task = std::move(queue_.front());
                queue_.pop();
            }
            task();
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> queue_;
    bool stopped_;
    std::thread thread_;
};
