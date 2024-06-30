// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match_manager.h"

#include <cassert>

#include "bot_core/msg_sender.h"
#include "bot_core/match.h"

static ErrCode StartGame(const lgtbot::game::InitOptionsResult start_mode, const UserID& uid, Match& match, MsgSenderBase& reply)
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
        sender << "\n\n" << match.BriefInfo();
    }
    return EC_OK;
}

ErrCode MatchManager::NewMatch(GameHandle& game_handle, const std::string_view init_options_args, const UserID& uid,
        const std::optional<GroupID> gid, MsgSenderBase& reply)
{
    lgtbot::game::InitOptionsResult start_mode = lgtbot::game::InitOptionsResult::NEW_MULTIPLE_USERS_MODE_GAME;
    std::shared_ptr<Match> new_match;
    {
        std::lock_guard<std::mutex> l(mutex_);
        if (GetMatch_(uid)) {
            reply() << "[错误] 建立失败：您已加入游戏";
            return EC_MATCH_USER_ALREADY_IN_MATCH;
        }
        if (gid.has_value() && GetMatch_(*gid)) {
            // We has tried terminating the game outside this funciton.
            // This case may happen when another user creates a new match after terminating.
            reply() << "[错误] 建立失败：该房间已经开始游戏";
            return EC_MATCH_ALREADY_BEGIN;
        }
        const MatchID mid = NewMatchID_();
        auto options = game_handle.CopyDefaultGameOptions();
        if (!init_options_args.empty()) {
            start_mode = game_handle.Info().handle_init_options_command_fn_(init_options_args.data(), options.game_options_.get(),
                    &options.generic_options_);
        }
        if (start_mode == lgtbot::game::InitOptionsResult::INVALID_INIT_OPTIONS_COMMAND) {
            // TODO: show all valid preset commands
            reply() << "[错误] 建立失败：非法的预设指令，您可以通过「" META_COMMAND_SIGN "规则 "
                    << game_handle.Info().name_ << "」查看所有的预设指令";
            return EC_INVALID_ARGUMENT;
        }
        new_match = std::make_shared<Match>(bot_, mid, game_handle, std::move(options), uid, gid);
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
