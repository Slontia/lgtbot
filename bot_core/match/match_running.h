// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "bot_core/match/match_child_client.h"
#include "bot_core/match/match_common.h"
#include "bot_core/match/match_phase_base.h"
#include "bot_core/timer.h"
#include "match_process/match_ipc.pb.h"

namespace lgtbot {
namespace core {
namespace match {

// Owns the per-MatchRunning game timer together with its cancelled flag. Holds a bare
// Match back-reference so the timer callbacks can dispatch timeout/alert handling
// back to Match without MatchRunning keeping a persistent Match& of its own. Match
// outlives MatchRunning (Match owns MatchRunning via Internal via data_, and the Timer's
// jthread joins on destruction, synchronising the loop thread with ~Match).
class MatchTimerFactory
{
  public:
    explicit MatchTimerFactory(Match& match) : match_(match) {}

    MatchTimerFactory(const MatchTimerFactory&) = delete;
    MatchTimerFactory& operator=(const MatchTimerFactory&) = delete;
    MatchTimerFactory(MatchTimerFactory&&) = default;

    void Start(uint64_t duration_sec);
    void Stop();

  private:
    void Retire_(std::unique_ptr<Timer>&& timer);

    Match& match_;
    std::unique_ptr<Timer> game_timer_;
    std::shared_ptr<std::atomic<bool>> timer_is_over_;
    // Timers replaced while their loop thread is still running (e.g. a timeout
    // handler that re-arms the game timer). Each jthread self-joins on destruction,
    // so retiring (rather than destroying in place) avoids self-join on the loop
    // thread.
    std::vector<std::unique_ptr<Timer>> retired_;
};

class MatchRunning : public MatchPhaseBase
{
  public:
    // Constructed from the outgoing MatchLobby: the ordered player id vector and IPC-side
    // PlayerInfo list are pulled from MatchLobby's lazy cache (already post-shuffle and
    // aligned by index). MatchRunning takes ownership of `player_ids`; the caller-side
    // PlayerInfo vector is only used by ExecuteGameStart's SendStart and does not
    // need to be re-copied into MatchRunning.
    MatchRunning(const MatchContext& ctx, MatchMessaging* messaging, MatchHelpServices* help,
            std::map<UserID, MatchParticipantUser>& users,
            const UserID host_uid, MatchRuntimeOptions options,
            std::vector<MatchVariantID> player_ids,
            MatchChildClient* game_child,
            std::optional<MsgSender> group_sender,
            MatchTimerFactory timer_factory);

    using MatchPhaseBase::Boardcast;
    using MatchPhaseBase::BoardcastAtAll;
    using MatchPhaseBase::BoardcastMsgSender;
    using MatchPhaseBase::BriefInfo;
    using MatchPhaseBase::GroupMsgSender;
    using MatchPhaseBase::UserNum;

    // PID-dependent accessors — meaningful only while phase is MatchRunning, so they live
    // here rather than on MatchPhaseBase.
    HostMsgSenderBase& TellMsgSender(const PlayerID pid);
    const char* PlayerName(const PlayerID& pid);
    const char* PlayerAvatar(const PlayerID& pid, const int32_t size);
    MatchVariantID ConvertPid(const PlayerID pid) const;

    void Eliminate(const PlayerID pid);
    void Hook(const PlayerID pid);
    void Activate(const PlayerID pid);

    bool IsInDeduction() const;

    ErrCode ExecuteRequest(const UserID uid, const std::optional<GroupID> gid, const std::string& msg,
            MsgSender& reply, const PushHandler& on_push);
    ErrCode LeaveBeforeChild(const UserID uid, HostMsgSenderBase& reply, const bool force,
            const PushHandler& on_push);
    ErrCode UserInterrupt(const UserID uid, HostMsgSenderBase& reply, const bool cancel) override;

    void ShowInfo(HostMsgSenderBase& reply) const override;
    bool SwitchHost() override;

    ErrCode Terminate(const bool is_force) override;

    UserID HostUserId() const override { return host_uid_; }
    bool is_over() const { return is_over_.load(std::memory_order_acquire); }
    const MatchRuntimeOptions& RuntimeOptions() const { return options_; }

    void FetchHelp(HostMsgSenderBase& reply, const bool text_mode);
    void ApplyChildPushFrame(const PushFrame& frame);
    void HandleChildEof();

  private:
    HostMsgSenderBase* GroupSenderOrNull_() override;
    MatchVariantID HostVariantId_() const override;
    uint32_t ComputerNumImpl_() const override;
    const MatchRuntimeOptions& RuntimeOptions_() const override;

    void ApplyChildPost_(const lgtbot::ipc::PostResp& post);
    void ApplyChildPlayerState(const PlayerID pid, const std::string& state);
    void ApplyChildGameOverFromScores(const lgtbot::ipc::GameOverResp& game_over);
    void UnbindMatchSide_();
    void UpdateDeductionFlag_();

    // Called synchronously from ApplyChildPushFrame on receipt of a TimerStartResp.
    // No Match& parameter: timer_factory_ already holds the Match back-reference
    // needed by its callbacks.
    void HandleTimerStart(uint64_t duration_sec);
    void HandleTimerStop();

    MatchHelpServices* help_{nullptr};
    std::optional<MsgSender> group_sender_;
    const UserID host_uid_;
    MatchRuntimeOptions options_;

    // PlayerIDs assigned by iteration order over player_ids_; participant state (per
    // MatchPlayer::state_) is MatchRunning's own concern and does not need to travel with
    // MatchLobby's cache. Player ids are owned by MatchRunning so that transitioning through
    // phase_ does not require the Internal-level std::vector<MatchPlayer> plumbing.
    std::vector<MatchPlayer> players_;

    std::atomic<bool> is_over_{false};
    std::atomic<bool> is_in_deduction_{false};

    MatchChildClient* game_child_{nullptr};

    MatchTimerFactory timer_factory_;
};

// Builds a PushHandler that forwards each push frame to MatchRunning::ApplyChildPushFrame.
// The lazy overload is used by SendStart, where MatchRunning is constructed (via
// get_running) inside SendRequestAndRead_ on the first frame; the eager overload
// covers the common case where a MatchRunning& already exists.
inline PushHandler MakeCallback(std::function<MatchRunning&()> get_running)
{
    return [get_running = std::move(get_running)](const PushFrame& frame) {
        get_running().ApplyChildPushFrame(frame);
    };
}

inline PushHandler MakeCallback(MatchRunning& running)
{
    return [&running](const PushFrame& frame) { running.ApplyChildPushFrame(frame); };
}

} // namespace match
} // namespace core
} // namespace lgtbot
