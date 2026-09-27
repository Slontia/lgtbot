// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <atomic>
#include <bitset>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <variant>

#include "bot_core/bot_core.h"
#include "bot_core/id.h"
#include "bot_core/msg_sender.h"

namespace lgtbot::core {
class BotCtx;
class GameHandle;
} // namespace lgtbot::core

namespace lgtbot {
namespace core {
namespace match {

class Match;

class MatchManager
{
   public:
    MatchManager(BotCtx& bot) : bot_(bot), next_mid_(0) {}

    ErrCode NewMatch(GameHandle& game_handle, const std::string_view init_options_args, const UserID& uid,
            const std::optional<GroupID> gid, HostMsgSenderBase& reply);

    template <typename IdType>
    std::shared_ptr<Match> GetMatch(const IdType id)
    {
        std::lock_guard<std::mutex> l(mutex_);
        return GetMatch_(id);
    }

    std::vector<std::shared_ptr<Match>> Matches() const;

    // Total number of matches created so far. Monotonically increasing (next_mid_
    // only increments, never reuses), so the per-second delta is the creation
    // throughput — used by the chaos test as a sysbench-style throughput metric.
    uint64_t CreatedCount() const { std::lock_guard<std::mutex> l(mutex_); return next_mid_.Get(); }

    template <typename IdType>
    bool BindMatch(const IdType id, std::shared_ptr<Match> match)
    {
        std::lock_guard<std::mutex> l(mutex_);
        return BindMatch_(id, std::move(match));
    }

    template <typename IdType>
    void UnbindMatch(const IdType id)
    {
        std::lock_guard<std::mutex> l(mutex_);
        UnbindMatch_(id);
    }

    // Deferred unbind for the timer handler path. The handler runs on Timer::thread_;
    // if it dropped MatchManager's last shared_ptr synchronously, the ensuing ~Match
    // would cascade into ~Timer whose thread_.join() would join its own thread — a
    // self-deadlock. Offloading the unbind to a dedicated worker thread lets ~Match
    // run there instead, keeping the timer's loop free of destruction-cascade
    // responsibilities. All other Match unbind paths (Terminate, Leave, Request, …)
    // are invoked with a caller-held shared_ptr and remain synchronous via Unbind_.
    void ScheduleUnbind(std::shared_ptr<Match> match);

    bool HasMatch() const;

   private:
    void DeleteMatch_(const MatchID id);

    template <typename IdType>
    std::shared_ptr<Match> GetMatch_(const IdType id)
    {
        const auto it = id2match<IdType>().find(id);
        return (it == id2match<IdType>().end()) ? nullptr : it->second;
    }

    template <typename IdType>
    bool BindMatch_(const IdType id, std::shared_ptr<Match> match)
    {
        return id2match<IdType>().emplace(id, match).second;
    }

    template <typename IdType>
    void UnbindMatch_(const IdType id)
    {
        id2match<IdType>().erase(id);
    }

    MatchID NewMatchID_();

    class UnbindWorker
    {
      public:
        explicit UnbindWorker(MatchManager& mgr);
        ~UnbindWorker();

        UnbindWorker(const UnbindWorker&) = delete;
        UnbindWorker& operator=(const UnbindWorker&) = delete;

        void Schedule(std::shared_ptr<Match> match);

      private:
        void Loop_();
        void Process_(const std::shared_ptr<Match>& match);

        MatchManager& mgr_;
        std::mutex mutex_;
        std::condition_variable cv_;
        std::queue<std::shared_ptr<Match>> queue_;
        bool stop_{false};
        std::thread thread_;
    };

    BotCtx& bot_;
    mutable std::mutex mutex_;
    template <typename IdType> using Id2Map = std::map<IdType, std::shared_ptr<Match>>;
    std::tuple<Id2Map<UserID>, Id2Map<MatchID>, Id2Map<GroupID>> id2match_;
    template <typename IdType> Id2Map<IdType>& id2match() { return std::get<Id2Map<IdType>>(id2match_); }
    template <typename IdType> const Id2Map<IdType>& id2match() const { return std::get<Id2Map<IdType>>(id2match_); }
    MatchID next_mid_;

    // Must be declared last so it is constructed after `mutex_` / `id2match_` (its
    // worker thread references them via `mgr_`) and destroyed first (dtor joins the
    // thread before the referenced state disappears).
    UnbindWorker unbind_worker_{*this};
};

} // namespace match
} // namespace core
} // namespace lgtbot
