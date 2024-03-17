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
    PublicStageUtility(const GameOption& option, MatchBase& match);
    PublicStageUtility(const PublicStageUtility&) = default;

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

    const MyGameOption& Options() const { return option_; }
    uint64_t PlayerNum() const { return option_.PlayerNum(); }
    const char* ResourceDir() const { return option_.ResourceDir(); }

    // Log

    template <typename Logger>
    Logger& StageLog(Logger&& logger, const std::string_view stage_name) const
    {
        return logger << "[mid=" << match_.MatchId() << "] [game=" << match_.GameName() << "] [stage=" << stage_name << "] ";
    }

  private:
    bool IsInDeduction() const { return masker_.IsAllPermanentInactive(); }

    void Leave(const PlayerID pid);

    static void TimerCallbackPublic_(void* const p, const uint64_t alert_sec);
    static void TimerCallbackPrivate_(void* const p, const uint64_t alert_sec);

    const GameOption& option_;
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
