// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "bot_core/bot_ctx.h"
#include "bot_core/game_handle.h"
#include "bot_core/id.h"
#include "bot_core/msg_sender.h"

class Match;
class MsgReader;

struct MatchContext
{
    BotCtx& bot;
    const MatchID mid;
    GameHandle& game_handle;
    const std::optional<GroupID> gid;

    uint64_t MatchId() const { return mid; }
    const char* GameName() const;
    uint64_t MaxPlayerNum() const;
    uint32_t Multiple() const;
    std::string HostUserName(const UserID host_uid) const;
    std::string OptionInfo(const bool text_mode = true) const;
};

struct MatchCallbacks
{
    std::function<bool(const UserID uid)> bind_user;
    std::function<void(const UserID uid)> unbind_user;
    std::function<void()> unbind_match;
};

struct MatchMessaging
{
    std::optional<MsgSender>* group_sender{nullptr};
    std::unique_ptr<HostMsgSenderBase>* private_broadcast_scratch{nullptr};
};

struct MatchHelpServices
{
    std::function<bool(MsgReader& reader, MsgSender& reply)> try_help_command;
    std::function<std::string(const bool with_example, const bool with_html_color)> help_command_info;
    std::function<void(HostMsgSenderBase& reply, const bool text_mode)> fetch_lobby_help;
};

template <typename Logger>
Logger&& LogMatch(Logger&& logger, const MatchContext& ctx, const UserID host_uid)
{
    logger << "[mid=" << ctx.MatchId() << "] ";
    if (ctx.gid.has_value()) {
        logger << "[gid=" << *ctx.gid << "] ";
    } else {
        logger << "[no gid] ";
    }
    logger << "[game=" << ctx.GameName() << "] [host_uid=" << host_uid << "] ";
    return std::forward<Logger>(logger);
}
