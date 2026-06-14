// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <memory>
#include <mutex>

template <typename T>
class AtomicWeakPtr
{
  public:
    AtomicWeakPtr() = default;
    explicit AtomicWeakPtr(std::weak_ptr<T> wk) : wk_(std::move(wk)) {}

    void Store(std::weak_ptr<T> wk)
    {
        std::lock_guard lock(mu_);
        wk_ = std::move(wk);
    }

    std::weak_ptr<T> Exchange(std::weak_ptr<T> wk)
    {
        std::lock_guard lock(mu_);
        auto old = std::move(wk_);
        wk_ = std::move(wk);
        return old;
    }

    std::shared_ptr<T> Lock() const
    {
        std::lock_guard lock(mu_);
        return wk_.lock();
    }

  private:
    mutable std::mutex mu_;
    std::weak_ptr<T> wk_;
};
