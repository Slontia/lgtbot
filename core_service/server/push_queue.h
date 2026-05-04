#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

#include "lgtbot_service.pb.h"

// Blocking FIFO for Subscribe(stream). One consumer per PushQueue (single subscriber per platform for MVP).
class PushQueue {
  public:
    void Push(lgtbot::PushEvent ev)
    {
        std::lock_guard<std::mutex> lk(mu_);
        q_.push(std::move(ev));
        cv_.notify_one();
    }

    // Returns false after Close() drains the queue.
    bool PopBlocking(lgtbot::PushEvent& out)
    {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [&] { return closed_ || !q_.empty(); });
        if (q_.empty()) {
            return false;
        }
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    void Close()
    {
        std::lock_guard<std::mutex> lk(mu_);
        closed_ = true;
        cv_.notify_all();
    }

  private:
    std::mutex mu_;
    std::condition_variable cv_;
    std::queue<lgtbot::PushEvent> q_;
    bool closed_{false};
};
