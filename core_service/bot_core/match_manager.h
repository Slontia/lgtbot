// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <optional>
#include <bitset>
#include <memory>
#include <map>
#include <functional>
#include <variant>
#include <mutex>
#include <atomic>

#include "bot_core/bot_core.h"
#include "bot_core/id.h"

class Match;
class BotCtx;
class MsgSenderBase;
class GameHandle;

class MatchManager
{
   public:
    MatchManager(BotCtx& bot) : bot_(bot), next_mid_(0) {}

    ErrCode NewMatch(GameHandle& game_handle, const std::string_view init_options_args, const UserID& uid,
            const std::optional<GroupID> gid, MsgSenderBase& reply);

    template <typename IdType>
    std::shared_ptr<Match> GetMatch(const IdType id)
    {
        std::lock_guard<std::mutex> l(mutex_);
        return GetMatch_(id);
    }

    std::vector<std::shared_ptr<Match>> Matches() const;

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

    BotCtx& bot_;
    mutable std::mutex mutex_;
    template <typename IdType> using Id2Map = std::map<IdType, std::shared_ptr<Match>>;
    std::tuple<Id2Map<UserID>, Id2Map<MatchID>, Id2Map<GroupID>> id2match_;
    template <typename IdType> Id2Map<IdType>& id2match() { return std::get<Id2Map<IdType>>(id2match_); }
    template <typename IdType> const Id2Map<IdType>& id2match() const { return std::get<Id2Map<IdType>>(id2match_); }
    MatchID next_mid_;
};
