// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "bot_core/match_child_client.h"
#include "bot_core/match_env.h"
#include "bot_core/match_phase_common.h"
#include "bot_core/match_types.h"
#include "match_process/match_ipc.pb.h"

struct LobbyGameStartPlan
{
    MatchChildClient::RuntimeOptions child_runtime_options;
    std::vector<std::string> options_to_sync;
    std::vector<lgtbot::ipc::PlayerInfo> players_for_child;
    uint32_t user_num{0};
};

struct LobbyRunningHandoff
{
    LobbyStartSnapshot snapshot;
};

class Lobby : public MatchPhaseCommon
{
  public:
    Lobby(const MatchContext& ctx, MatchMessaging* messaging,
            std::map<UserID, MatchParticipantUser>& users, std::vector<MatchPlayer>& players,
            const UserID host_uid, MatchInitOptions init_options);

    using MatchPhaseCommon::Boardcast;
    using MatchPhaseCommon::BoardcastAtAll;
    using MatchPhaseCommon::BoardcastMsgSender;
    using MatchPhaseCommon::BriefInfo;
    using MatchPhaseCommon::BindMsgSenderMatch;
    using MatchPhaseCommon::ConvertPid;
    using MatchPhaseCommon::GroupMsgSender;
    using MatchPhaseCommon::PlayerAvatar;
    using MatchPhaseCommon::PlayerName;
    using MatchPhaseCommon::TellMsgSender;
    using MatchPhaseCommon::UserNum;

    ErrCode SetBenchTo(const UserID uid, MsgSenderBase& reply, const uint64_t bench_computers_to_player_num);
    ErrCode SetFormal(const UserID uid, MsgSenderBase& reply, const bool is_formal);
    ErrCode Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply,
            const std::weak_ptr<const class Match>& match_wk);
    ErrCode Join(const UserID uid, MsgSenderBase& reply);
    ErrCode Leave(const UserID uid, MsgSenderBase& reply, const bool force);
    ErrCode UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel) override;

    void ShowInfo(MsgSenderBase& reply, const std::weak_ptr<const class Match>& match_wk) const override;
    bool SwitchHost() override;

    ErrCode Terminate(const bool is_force) override;

    UserID HostUserId() const override;

    std::optional<LobbyGameStartPlan> BeginGameStart(const UserID uid, MsgSenderBase& reply,
            const std::weak_ptr<const class Match>& match_wk, ErrCode& err_out) &&;
    void RollbackPreparedStart() &&;
    std::optional<LobbyRunningHandoff> IntoRunning() &&;

  private:
    MsgSenderBase* GroupSenderOrNull_() override;
    MatchVariantID HostVariantId_() const override;
    uint32_t ComputerNumImpl_() const override;
    const MatchRuntimeOptions& RuntimeOptions_() const override;

    bool Has_(const UserID uid) const;
    uint32_t PlayerNum_() const;
    void EmplaceUser_(const UserID uid);
    void KickForConfigChange_();

    UserID host_uid_;
    MatchRuntimeOptions options_;
    std::vector<std::string> applied_options_log_;
};
