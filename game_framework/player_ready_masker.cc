// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/player_ready_masker.h"

#include "utility/log.h"

#include <cassert>
#include <numeric>
#include <algorithm>

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

namespace internal {

PlayerReadyMasker::PlayerReadyMasker(const uint64_t match_id, const char* const game_name, const size_t size)
    : states_(size), log_header_{"[mid=" + std::to_string(match_id) + "] [game=" + game_name + "] "}
{
}

void PlayerReadyMasker::SilentlySetReady(const size_t index)
{
    Log_(DebugLog()) << "Begin setting ready silently, index: " << index << ", " << ToString_();
    states_[index].is_ready_ = true;
    // We do not set `any_ready_` flag when computers completing actions. The reason is computers act before users. If
    // all users are temporarily inactive, we should ensure one of them have the opportunity to take action again. By
    // not setting this flag, stage will not be checked out until timeout or one of the user take action again.
    Log_(DebugLog()) << "Finish setting ready silently, index: " << index << ", " << ToString_();
}

void PlayerReadyMasker::SetReady(const size_t index)
{
    Log_(DebugLog()) << "Begin setting ready, index: " << index << ", " << ToString_();
    any_ready_ = true;
    states_[index].is_ready_ = true;
    Log_(DebugLog()) << "Finish setting ready, index: " << index << ", " << ToString_();
}

void PlayerReadyMasker::UnsetReady(const size_t index)
{
    Log_(DebugLog()) << "Begin unsetting ready, index: " << index << ", " << ToString_();
    states_[index].is_ready_ = false;
    // If the unset ready player is temporarily inactive, we need wait for him until timeout.
    any_ready_ = false;
    Log_(DebugLog()) << "Finish unsetting ready, index: " << index << ", " << ToString_();
}

void PlayerReadyMasker::ClearReady()
{
    Log_(DebugLog()) << "Begin clearing ready, " << ToString_();
    for (auto& state : states_) {
        state.is_ready_ = false;
    }
    any_ready_ = false;
    Log_(DebugLog()) << "Finish clearing ready, " << ToString_();
}

void PlayerReadyMasker::SetTemporaryInactive(const size_t index)
{
    Inactivate_(index, ActiveState::TEMPORARILY_INACTIVE);
}

void PlayerReadyMasker::SetPermanentInactive(const size_t index)
{
    Inactivate_(index, ActiveState::PERMANENTLY_INACTIVE);
    if (std::ranges::all_of(states_, [](const State state) { return state.active_state_ == ActiveState::PERMANENTLY_INACTIVE; })) {
        is_all_permanent_inactive_ = true;
        Log_(WarnLog()) << "Begin deduction, index: " << index << ", " << ToString_();
    }
}

bool PlayerReadyMasker::SetActive(const size_t index)
{
    auto& state = states_[index];
    assert(state.active_state_ != ActiveState::PERMANENTLY_INACTIVE);
    if (state.active_state_ == ActiveState::TEMPORARILY_INACTIVE) {
        Log_(DebugLog()) << "Begin setting active, index: " << index << ", " << ToString_();
        state.active_state_ = ActiveState::ACTIVE;
        Log_(DebugLog()) << "Finish setting active, index: " << index << ", " << ToString_();
        return true;
    }
    return false;
}

bool PlayerReadyMasker::Ok() const
{
    static const auto ready_or_permanent_inactive =
        [](const State state) { return state.active_state_ == ActiveState::PERMANENTLY_INACTIVE || state.is_ready_; };
    static const auto ready_or_inactive =
        [](const State state) { return state.active_state_ != ActiveState::ACTIVE || state.is_ready_; };
    return is_all_permanent_inactive_ || std::ranges::all_of(states_, ready_or_permanent_inactive) ||
        (std::ranges::all_of(states_, ready_or_inactive) && any_ready_);
}

void PlayerReadyMasker::Inactivate_(const size_t index, const ActiveState new_active_state)
{
    assert(new_active_state != ActiveState::ACTIVE);
    auto& active_state = states_[index].active_state_;
    if (active_state == ActiveState::ACTIVE) {
        Log_(DebugLog()) << "Inactivate game stage mask begin index: " << index << ", " << ToString_();
        any_ready_ = true;
        Log_(DebugLog()) << "Inactivate game stage mask finish index: " << index << ", " << ToString_();
    }
    active_state = new_active_state;
}

std::string PlayerReadyMasker::StateToString_(const PlayerReadyMasker::State state)
{
    constexpr const char* k_active_state_strs[] = {
        [static_cast<std::underlying_type_t<ActiveState>>(ActiveState::ACTIVE)] = "",
        [static_cast<std::underlying_type_t<ActiveState>>(ActiveState::TEMPORARILY_INACTIVE)] = ", temp_inactive",
        [static_cast<std::underlying_type_t<ActiveState>>(ActiveState::PERMANENTLY_INACTIVE)] = ", inactive",
    };
    return "ready: " + std::to_string(state.is_ready_) +
        k_active_state_strs[static_cast<std::underlying_type_t<ActiveState>>(state.active_state_)];
}

std::string PlayerReadyMasker::ToString_()
{
    return std::accumulate(states_.begin(), states_.end(), std::string{},
            [i = 0](std::string str, const State state) mutable
            {
                return std::move(str) + "[" + std::to_string(i++) + "] {" + StateToString_(state) + "}, ";
            }) + "any_ready: " + std::to_string(any_ready_);
}

} // namespace internal

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot
