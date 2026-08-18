// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "bot_core/match_child_client.h"
#include "bot_core/match_env.h"
#include "bot_core/match_phase_common.h"
#include "bot_core/match_types.h"
#include "bot_core/timer.h"
#include "match_process/match_ipc.pb.h"

class Running : public MatchPhaseCommon
{
  public:
    Running(const MatchContext& ctx, MatchMessaging* messaging, MatchHelpServices* help,
            std::map<UserID, MatchParticipantUser>& users, std::vector<MatchPlayer>& players,
            LobbyStartSnapshot snapshot, MatchChildClient* game_child,
            std::optional<MsgSender> group_sender);

    using MatchPhaseCommon::Boardcast;
    using MatchPhaseCommon::BoardcastAtAll;
    using MatchPhaseCommon::BoardcastMsgSender;
    using MatchPhaseCommon::BriefInfo;
    using MatchPhaseCommon::ConvertPid;
    using MatchPhaseCommon::GroupMsgSender;
    using MatchPhaseCommon::PlayerAvatar;
    using MatchPhaseCommon::PlayerName;
    using MatchPhaseCommon::TellMsgSender;
    using MatchPhaseCommon::UserNum;

    void Eliminate(const PlayerID pid);
    void Hook(const PlayerID pid);
    void Activate(const PlayerID pid);

    bool IsInDeduction() const;

    ErrCode ExecuteRequest(const UserID uid, const std::optional<GroupID> gid, const std::string& msg,
            MsgSender& reply, const std::weak_ptr<const class Match>& match_wk);
    ErrCode LeaveBeforeChild(const UserID uid, HostMsgSenderBase& reply, const bool force);
    ErrCode UserInterrupt(const UserID uid, HostMsgSenderBase& reply, const bool cancel) override;

    void ShowInfo(HostMsgSenderBase& reply, const std::weak_ptr<const class Match>& match_wk) const override;
    bool SwitchHost() override;

    ErrCode Terminate(const bool is_force) override;

    UserID HostUserId() const override { return host_uid_; }
    bool is_over() const { return is_over_.load(std::memory_order_acquire); }


    void FetchHelp(HostMsgSenderBase& reply, const bool text_mode);
    void ApplyChildPushFrame(const PushFrame& frame);
    void HandleChildEof();
    void BindMatch(std::weak_ptr<class Match> wk);

  private:
    HostMsgSenderBase* GroupSenderOrNull_() override;
    MatchVariantID HostVariantId_() const override;
    uint32_t ComputerNumImpl_() const override;
    const MatchRuntimeOptions& RuntimeOptions_() const override;

    void ApplyChildPost_(const PostFrame& frame);
    void ApplyChildPlayerState(const PlayerID pid, const std::string& state);
    void ApplyChildGameOverFromScores(const GameOverFrame& frame);
    void UnbindMatchSide_();
    void UpdateDeductionFlag_();

    MatchHelpServices* help_{nullptr};
    std::optional<MsgSender> group_sender_;
    const UserID host_uid_;
    const MatchRuntimeOptions options_;
    const std::vector<std::string> applied_options_log_;

    std::atomic<bool> is_over_{false};
    std::atomic<bool> is_in_deduction_{false};

    MatchChildClient* game_child_{nullptr};

    std::unique_ptr<Timer>              game_timer_;
    std::shared_ptr<std::atomic<bool>> timer_is_over_;
    std::weak_ptr<class Match> weak_match_;

    void HandleTimerStart(uint64_t duration_sec);
    void HandleTimerStop();
};
