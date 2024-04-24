// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match_manager.h"

#include <cassert>

#include "bot_core/msg_sender.h"
#include "bot_core/match.h"

std::pair<ErrCode, std::shared_ptr<Match>> MatchManager::NewMatch(GameHandle& game_handle,
                                                                  const std::string_view init_options_args,
                                                                  const UserID& uid,
                                                                  const std::optional<GroupID> gid,
                                                                  MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (GetMatch_(uid)) {
        reply() << "[错误] 建立失败：您已加入游戏";
        return {EC_MATCH_USER_ALREADY_IN_MATCH, nullptr};
    }
    if (gid.has_value() && GetMatch_(*gid)) {
        // We has tried terminating the game outside this funciton.
        // This case may happen when another user creates a new match after terminating.
        reply() << "[错误] 建立失败：该房间已经开始游戏";
        return {EC_MATCH_ALREADY_BEGIN, nullptr};
    }
    const MatchID mid = NewMatchID_();
    auto options = game_handle.CopyDefaultGameOptions();
    const lgtbot::game::InitOptionsResult start_mode =
        init_options_args.empty() ? lgtbot::game::InitOptionsResult::NEW_MULTIPLE_USERS_MODE_GAME
                                  : game_handle.Info().handle_init_options_command_fn_(
                                          init_options_args.data(), options.game_options_.get(), &options.generic_options_);
    if (start_mode == lgtbot::game::InitOptionsResult::INVALID_INIT_OPTIONS_COMMAND) {
        // TODO: show all valid preset commands
        reply() << "[错误] 建立失败：非法的预设指令，您可以通过「" META_COMMAND_SIGN "规则 "
                << game_handle.Info().name_ << "」查看所有的预设指令";
        return {EC_INVALID_ARGUMENT, nullptr};
    }
    const auto new_match = std::make_shared<Match>(bot_, mid, game_handle, std::move(options), uid, gid);
    BindMatch_(mid, new_match);
    BindMatch_(uid, new_match);
    if (gid.has_value()) {
        BindMatch_(*gid, new_match);
    }
    if (start_mode == lgtbot::game::InitOptionsResult::NEW_SINGLE_USER_MODE_GAME) {
        // Start game directly for single-player mode.
        const auto ret = new_match->GameStart(uid, reply);
        if (ret != EC_OK) {
            return {ret, nullptr};
        }
    }
    return {EC_OK, new_match};
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
