// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match/match_manager.h"

#include <cassert>
#include <cstdio>

#include "bot_core/msg_sender.h"
#include "bot_core/match/match.h"
#include "bot_core/match/match_child_client.h"
#include "bot_core/match/match_common.h"
#include "bot_core/match/match_lobby.h"
#include "match_process/match_ipc.pb.h"

namespace lgtbot {
namespace core {
namespace match {

static ErrCode StartGame(const lgtbot::game::InitOptionsResult start_mode, const UserID& uid, Match& match, HostMsgSenderBase& reply)
{
    if (start_mode == lgtbot::game::InitOptionsResult::NEW_SINGLE_USER_MODE_GAME) {
        // Start game directly for single-player mode.
        const auto ret = match.GameStart(uid, reply);
        if (ret != EC_OK) {
            return ret;
        }
    } else {
        auto sender = match.Boardcast();
        if (match.gid().has_value()) {
            sender << "现在玩家可以在群里通过「" META_COMMAND_SIGN "加入」报名比赛，房主也可以通过「帮助」（不带"
                META_COMMAND_SIGN "号）查看所有支持的游戏设置";
        } else {
            sender << "现在玩家可以通过私信我「" META_COMMAND_SIGN "加入 " << match.MatchId()
                << "」报名比赛，您也可以通过「帮助」（不带" META_COMMAND_SIGN "号）查看所有支持的游戏设置";
        }
        sender << "\n\n";
        {
            std::string brief;
            match.BriefInfo(brief);
            sender << brief;
        }
    }
    return EC_OK;
}

ErrCode MatchManager::NewMatch(GameHandle& game_handle, const std::string_view init_options_args, const UserID& uid,
        const std::optional<GroupID> gid, HostMsgSenderBase& reply)
{
    lgtbot::game::InitOptionsResult start_mode = lgtbot::game::InitOptionsResult::NEW_MULTIPLE_USERS_MODE_GAME;
    std::shared_ptr<Match> new_match;
    {
        std::lock_guard<std::mutex> l(mutex_);
        // A stale IS_OVER match should already be self-unbinding (synchronously via
        // Match::Unbind_ from Terminate/Leave/… or via UnbindWorker for the timer
        // handler path). Treating "user still bound" as an error rather than
        // reaching into MatchManager's maps here avoids the race where our
        // manual UnbindMatch_ would remove entries that Match::Unbind_ or the
        // worker will later touch — and, worse, would clobber a newly bound
        // successor match if the ids get reused before those paths complete.
        if (GetMatch_(uid)) {
            reply() << "[错误] 建立失败：您已加入游戏";
            return EC_MATCH_USER_ALREADY_IN_MATCH;
        }
        if (gid.has_value() && GetMatch_(*gid)) {
            reply() << "[错误] 建立失败：该房间已经开始游戏";
            return EC_MATCH_ALREADY_BEGIN;
        }
        const MatchID mid = NewMatchID_();
        uint64_t max_player = game_handle.CachedMaxPlayer();
        uint32_t multiple = game_handle.CachedMultiple();
        uint32_t bench = 0;
        bool is_formal = game_handle.ConfigClient().QueryDefaultFormal();
        if (!init_options_args.empty()) {
            start_mode = game_handle.ConfigClient().InitOptions(std::string(init_options_args), max_player, multiple, bench, is_formal);
            if (start_mode == lgtbot::game::InitOptionsResult::INVALID_INIT_OPTIONS_COMMAND) {
                // TODO: show all valid preset commands
                reply() << "[错误] 建立失败：非法的预设指令，您可以通过「" META_COMMAND_SIGN "规则 "
                        << game_handle.Info().name_ << "」查看所有的预设指令";
                return EC_INVALID_ARGUMENT;
            }
            game_handle.UpdateCachedLimits(max_player, multiple);
        }
        Match::InitOptions options;
        options.bench_computers_to_player_num_ = bench;
        options.is_formal_ = is_formal;

        // Eagerly spawn the child before Match creation. Match holds the child as an
        // invariant thereafter, so no lazy path or replay buffer is needed.
        const MatchContext ctx{bot_, mid, game_handle, gid};
        auto child = MakeMatchChildClient(ResolveRunnerExe(),
                GameLibraryPath(bot_, game_handle),
                MakeInitialChildRuntimeOptions(ctx, options));
        if (!child) {
            reply() << "[错误] 建立失败：无法启动游戏子进程";
            return EC_MATCH_UNEXPECTED_CONFIG;
        }
        for (const auto& line : game_handle.ConfigClient().GetAppliedLog()) {
            const auto stage = child->SendSetOption(line);
            if (!stage || *stage != lgtbot::ipc::ResultResp::STAGE_OK) {
                reply() << "[错误] 建立失败：无法同步游戏设置到子进程";
                return EC_MATCH_UNEXPECTED_CONFIG;
            }
        }
        new_match = std::make_shared<Match>(bot_, mid, game_handle,
                std::move(options), uid, gid, std::move(child));
        BindMatch_(mid, new_match);
        BindMatch_(uid, new_match);
        if (gid.has_value()) {
            BindMatch_(*gid, new_match);
        }
    }
    return StartGame(start_mode, uid, *new_match, reply);
}

std::vector<std::shared_ptr<Match>> MatchManager::Matches() const
{
    std::lock_guard<std::mutex> l(mutex_);
    std::vector<std::shared_ptr<Match>> matches;
    for (const auto& [_, match] : id2match<MatchID>()) {
        matches.emplace_back(match);
    }
    return matches;
}

MatchID MatchManager::NewMatchID_()
{
    const auto& mid2match = id2match<MatchID>();
    while (mid2match.find(++next_mid_) != mid2match.end())
        ;
    return next_mid_;
}

bool MatchManager::HasMatch() const
{
    return std::apply([&](const auto& ...id2match) { return (!id2match.empty() || ...); }, id2match_);
}

void MatchManager::ScheduleUnbind(std::shared_ptr<Match> match)
{
    unbind_worker_.Schedule(std::move(match));
}

MatchManager::UnbindWorker::UnbindWorker(MatchManager& mgr)
    : mgr_(mgr)
    , thread_([this] { Loop_(); })
{}

MatchManager::UnbindWorker::~UnbindWorker()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void MatchManager::UnbindWorker::Schedule(std::shared_ptr<Match> match)
{
    if (!match) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(match));
    }
    cv_.notify_one();
}

void MatchManager::UnbindWorker::Loop_()
{
    for (;;) {
        std::shared_ptr<Match> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
            if (queue_.empty()) {
                // stop_ is true and no more work — exit.
                return;
            }
            task = std::move(queue_.front());
            queue_.pop();
        }
        // Process outside the queue lock so ~Match's cascade (which may take time,
        // e.g. join Timer's own loop_thread) does not block Schedule callers.
        Process_(task);
        // `task` destructs at loop-iteration end. If the map erases below dropped
        // MatchManager's last shared_ptr, this drop triggers ~Match here — on this
        // worker thread, never on the timer's loop thread.
    }
}

void MatchManager::UnbindWorker::Process_(const std::shared_ptr<Match>& match)
{
    std::lock_guard<std::mutex> lock(mgr_.mutex_);

    // Address-equality check is essential here. Between ScheduleUnbind (invoked by
    // the timer handler) and this task being dequeued, a synchronous request from
    // the user — e.g. Match::Terminate — may have already unbound the same Match's
    // mid/gid via Match::Unbind_(). If the user then started a fresh match in the
    // same group, MatchManager::gid → NewMatch is now bound. A blind erase of the
    // gid entry here would silently unbind the innocent new match, breaking group
    // dispatch for the just-started game. We therefore erase only when the map
    // entry still refers to this exact Match; otherwise we skip.
    auto& mid_map = mgr_.id2match<MatchID>();
    if (const auto it = mid_map.find(static_cast<MatchID>(match->MatchId()));
            it != mid_map.end() && it->second == match) {
        mid_map.erase(it);
    }
    if (const auto gid = match->gid()) {
        auto& gid_map = mgr_.id2match<GroupID>();
        if (const auto it = gid_map.find(*gid);
                it != gid_map.end() && it->second == match) {
            gid_map.erase(it);
        }
    }
}

} // namespace match
} // namespace core
} // namespace lgtbot
