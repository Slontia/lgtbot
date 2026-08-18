// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cstdint>
#include <functional>
#include <utility>

#include "bot_core/timer.h"
#include "utility/empty_func.h"

namespace {

constexpr uint64_t kMinGameTimerAlertSec = 10;

} // namespace

inline Timer::TaskSet BuildGameTimerTasks(const uint64_t sec, std::function<void(uint64_t)> alert_handler,
        std::function<void()> timeout_handler)
{
    Timer::TaskSet tasks;
    if (sec == 0) {
        return tasks;
    }
    if (kMinGameTimerAlertSec > sec / 2) {
        tasks.emplace_front(sec, [timeout_handler = std::move(timeout_handler)](const uint64_t /*sec*/) {
            timeout_handler();
        });
        return tasks;
    }
    tasks.emplace_front(kMinGameTimerAlertSec, [timeout_handler = std::move(timeout_handler)](const uint64_t /*sec*/) {
        timeout_handler();
    });
    uint64_t sum_alert_sec = kMinGameTimerAlertSec;
    for (uint64_t alert_sec = kMinGameTimerAlertSec; sum_alert_sec < sec / 2; sum_alert_sec += alert_sec, alert_sec *= 2) {
        tasks.emplace_front(alert_sec, [alert_handler, alert_sec](const uint64_t /*sec*/) { alert_handler(alert_sec); });
    }
    tasks.emplace_front(sec - sum_alert_sec, g_empty_func);
    return tasks;
}
