// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/timer.h"

namespace lgtbot::core {

#ifdef TEST_BOT

std::condition_variable Timer::cv_;
bool Timer::skip_timer_ = false;
std::mutex Timer::mutex_;

#endif

} // namespace lgtbot::core
