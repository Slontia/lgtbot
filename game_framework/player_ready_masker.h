// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <vector>
#include <string>

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

namespace internal {

class PlayerReadyMasker
{
    enum class ActiveState
    {
        ACTIVE,                // The player is active. We need wait his action.
        TEMPORARILY_INACTIVE,  // The player is temporarily unable to act. We need not to wait his action if some other players has completed action.
        PERMANENTLY_INACTIVE,  // The player is Permanently unable to act. We no longer need not to wait his action.
    };

    struct State
    {
        bool is_ready_ = false;  // Whether the player has finished current stage's action.
                                 // `is_ready_` can be true even if `active_state_` is not `ACTIVE` because the `is_ready_` flag can be set by stage.
        ActiveState active_state_ = ActiveState::ACTIVE;
    };

  public:
    PlayerReadyMasker(const uint64_t match_id, const char* const game_name, const size_t size);

    // Set/unset ready flag

    void SilentlySetReady(const size_t index);
    void SetReady(const size_t index);
    void UnsetReady(const size_t index);
    void ClearReady();

    // Set/unset active state

    void SetTemporaryInactive(const size_t index);
    void SetPermanentInactive(const size_t index);
    bool SetActive(const size_t index);

    // check

    bool IsAllPermanentInactive() const { return is_all_permanent_inactive_; }
    bool IsReady(const size_t index) const { return states_[index].is_ready_; }
    bool IsInactive(const size_t index) const { return states_[index].active_state_ != ActiveState::ACTIVE; }
    bool IsTemporaryInactive(const size_t index) const { return states_[index].active_state_ == ActiveState::TEMPORARILY_INACTIVE; }

    // The returned value of true indicates we no longer need to wait more players to get ready.
    bool Ok() const;

  private:
    void Inactivate_(const size_t index, const ActiveState new_active_state);

    static std::string StateToString_(const State state);
    std::string ToString_();

    template <typename Logger>
    Logger& Log_(Logger&& logger) { return logger << log_header_; }

    std::vector<State> states_;
    bool any_ready_{false};                  // be true if any user complete action
    bool is_all_permanent_inactive_{false};  // not necessary, to prevent always checking all ready flags
    std::string log_header_;
};

} // namespace internal

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot
