// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

template <typename Func>
class Defer
{
  public:
    explicit Defer(Func func) : func_(func) {}
    ~Defer() { func_(); }

  private:
    Func func_;
};
