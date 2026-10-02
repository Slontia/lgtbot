// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "bot_core/match/match_child_client.h"
#include "bot_core/match/match_common.h"
#include "bot_core/match/match_phase_base.h"
#include "match_process/match_ipc.pb.h"

namespace lgtbot {
namespace core {
namespace match {

// Builds the runtime options for the very first MatchChildClient spawn, before any
// MatchLobby object exists. Called by match_manager during eager spawn.
[[nodiscard]] MatchChildClient::RuntimeOptions MakeInitialChildRuntimeOptions(
    const MatchContext& ctx, const MatchInitOptions& init_options);

class MatchLobby : public MatchPhaseBase
{
  public:
    MatchLobby(const MatchContext& ctx, MatchMessaging* messaging,
            std::map<UserID, MatchParticipantUser>& users,
            const UserID host_uid, MatchInitOptions init_options);

    using MatchPhaseBase::Boardcast;
    using MatchPhaseBase::BoardcastAtAll;
    using MatchPhaseBase::BoardcastMsgSender;
    using MatchPhaseBase::BriefInfo;
    using MatchPhaseBase::GroupMsgSender;
    using MatchPhaseBase::UserNum;

    ErrCode SetBenchTo(const UserID uid, HostMsgSenderBase& reply, const uint64_t bench_computers_to_player_num);
    ErrCode SetFormal(const UserID uid, HostMsgSenderBase& reply, const bool is_formal);
    ErrCode Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply,
            class MatchChildClient& game_child);
    ErrCode Join(const UserID uid, HostMsgSenderBase& reply);
    ErrCode Leave(const UserID uid, HostMsgSenderBase& reply, const bool force);
    ErrCode UserInterrupt(const UserID uid, HostMsgSenderBase& reply, const bool cancel) override;

    void ShowInfo(HostMsgSenderBase& reply) const override;
    bool SwitchHost() override;

    ErrCode Terminate(const bool is_force) override;

    UserID HostUserId() const override;

    // Validates that the caller is the host. Returns EC_OK if the game may proceed,
    // otherwise an ErrCode with a message already written to `reply`. Being const
    // decouples game-start bookkeeping from participant-list preparation, which now
    // lives entirely in the lazily-rebuilt cache.
    ErrCode CheckHost(const UserID uid, HostMsgSenderBase& reply) const;

    // Const accessors that MatchRunning's construction path reads. `players_for_child` and
    // `player_ids` are aligned by index and share the same post-shuffle canonical
    // order. Both are produced by the same lazy rebuild pass driven by a `needs_rebuild`
    // flag; any mutation that changes the participant list (Join/Leave/SetBenchTo)
    // invalidates the cache.
    const std::vector<lgtbot::ipc::PlayerInfo>& PlayersForChild() const;
    const std::vector<MatchVariantID>& PlayerIds() const;
    const MatchRuntimeOptions& RuntimeOptions() const { return options_; }

  private:
    HostMsgSenderBase* GroupSenderOrNull_() override;
    MatchVariantID HostVariantId_() const override;
    uint32_t ComputerNumImpl_() const override;
    const MatchRuntimeOptions& RuntimeOptions_() const override;

    bool Has_(const UserID uid) const;
    uint32_t PlayerNum_() const;
    void EmplaceUser_(const UserID uid);
    void KickForConfigChange_();
    void InvalidatePlayerCache_() { player_cache_.needs_rebuild = true; }
    void RebuildPlayerCache_() const;

    // Derived, post-shuffle participant listing. Kept in sync via `needs_rebuild`;
    // the fields are `mutable` so the const accessors can trigger a lazy rebuild.
    struct PlayerListCache
    {
        std::vector<MatchVariantID> player_ids;                  // ordered ids for MatchRunning::players_
        std::vector<lgtbot::ipc::PlayerInfo> players_for_child;   // ordered PlayerInfo for SendStart
        bool needs_rebuild = true;
    };

    UserID host_uid_;
    MatchRuntimeOptions options_;
    mutable PlayerListCache player_cache_;
};

} // namespace match
} // namespace core
} // namespace lgtbot
