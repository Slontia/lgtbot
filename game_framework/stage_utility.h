// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include "bot_core/id.h"
#include "bot_core/match_base.h"
#include "game_framework/game_achievements.h"
#include "game_framework/game_options.h"
#include "game_framework/player_ready_masker.h"
#include "nlohmann/json.hpp"
#include "utility/msg_checker.h"

#include <array>

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class StageUtility;

namespace internal {

class PublicStageUtility
{
    friend class lgtbot::game::GAME_MODULE_NAME::StageUtility;
    using AchievementCounts = std::array<uint8_t, Achievement::Count()>;

  public:
    PublicStageUtility(const CustomOptions& game_options, const lgtbot::game::GenericOptions& generic_options, MatchBase& match);
    PublicStageUtility(PublicStageUtility&&) = default;

    // Message functions

    MsgSenderBase& BoardcastMsgSender() const;
    MsgSenderBase& TellMsgSender(const PlayerID pid) const;
    MsgSenderBase& GroupMsgSender() const;
    MsgSenderBase& BoardcastAiInfoMsgSender() const;

    decltype(auto) Group() const { return GroupMsgSender()(); }
    decltype(auto) Boardcast() const { return BoardcastMsgSender()(); }
    decltype(auto) Tell(const PlayerID pid) const { return TellMsgSender(pid)(); }
    void BoardcastAiInfo(nlohmann::json j);

    int SaveMarkdown(const std::string& markdown, const uint32_t width = 600);

    // Player information

    std::string PlayerName(const PlayerID pid) const { return match_.PlayerName(pid); }
    std::string PlayerAvatar(const PlayerID pid, const int32_t size) const { return match_.PlayerAvatar(pid, size); }
    //std::string PlayerAvatarPath(const PlayerID pid) const { return match_.PlayerAvatarPath(pid); }

    void Achieve(const PlayerID pid, const Achievement& achievement) { ++achievement_counts_[pid][achievement.ToUInt()]; }

    // Player ready state

    void SetReady(const PlayerID pid) { masker_.SetReady(pid); }
    void ClearReady(const PlayerID pid) { masker_.UnsetReady(pid); }

    void Eliminate(const PlayerID pid);
    void Hook(const PlayerID pid);
    void HookUnreadyPlayers();

    bool IsReady(const PlayerID pid) const { return masker_.IsReady(pid); }
    void ClearReady() { masker_.ClearReady(); }
    bool IsOkToCheckout() const { return IsInDeduction() || masker_.Ok(); }

    // Timer

    void StartTimer(const uint64_t sec);
    void StopTimer();

    const auto& TimerFinishTime() const { return timer_finish_time_; }

    // Options

    const CustomOptions& Options() const { return game_options_; }
    auto PlayerNum() const { return generic_options_.PlayerNum(); }
    const char* ResourceDir() const { return generic_options_.resource_dir_; }

    // Log

    template <typename Logger>
    auto& StageLog(Logger&& logger, const std::string_view stage_name) const
    {
        return logger << "[mid=" << match_.MatchId() << "] [game=" << match_.GameName() << "] [stage=" << stage_name << "] ";
    }

  private:
    // TODO: Stage should identify which players are computers. `match_.IsInDeduction()` condition should be removed.
    bool IsInDeduction() const { return match_.IsInDeduction() || masker_.IsAllPermanentInactive(); }

    void Leave(const PlayerID pid);

    static void TimerCallbackPublic_(void* const p, const uint64_t alert_sec);
    static void TimerCallbackPrivate_(void* const p, const uint64_t alert_sec);

    const CustomOptions& game_options_;
    const GenericOptions& generic_options_;
    MatchBase& match_;
    PlayerReadyMasker masker_;
    std::vector<AchievementCounts> achievement_counts_;
    int32_t bot_message_id_{0}; // the ID of each bot message
    int32_t saved_image_no_{0};
    std::optional<std::chrono::time_point<std::chrono::steady_clock>> timer_finish_time_;
};

class AtomicStage;
class CompoundStage;
class MainStage;

} // namespace internal

class StageUtility : public internal::PublicStageUtility
{
    friend class internal::AtomicStage;
    friend class internal::CompoundStage;
    friend class internal::MainStage;

  public:
    using PublicStageUtility::PublicStageUtility;

  private:
    using PublicStageUtility::Leave;
    using PublicStageUtility::IsInDeduction;

    void SetReadyAsComputer(const PlayerID pid) { masker_.SilentlySetReady(pid); }

    uint8_t AchievementCount(const PlayerID pid, const Achievement& achievement) const;

    void Activate(const PlayerID pid);
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot
