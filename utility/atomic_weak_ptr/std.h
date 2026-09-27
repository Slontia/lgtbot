// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <atomic>
#include <memory>

template <typename T>
class AtomicWeakPtr
{
  public:
    AtomicWeakPtr() = default;
    explicit AtomicWeakPtr(std::weak_ptr<T> wk) : wk_(std::move(wk)) {}

    void Store(std::weak_ptr<T> wk) { wk_.store(std::move(wk)); }

    std::weak_ptr<T> Exchange(std::weak_ptr<T> wk) { return wk_.exchange(std::move(wk)); }

    std::shared_ptr<T> Lock() const { return wk_.load().lock(); }

  private:
    std::atomic<std::weak_ptr<T>> wk_;
};
