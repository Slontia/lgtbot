// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "bot_core/bot_ctx.h"
#include "bot_core/game_handle.h"
#include "bot_core/id.h"
#include "bot_core/msg_sender.h"
#include "game_framework/game_main.h"

class MsgReader;

namespace lgtbot {
namespace core {
namespace match {

class Match;

using MatchVariantID = std::variant<UserID, ComputerID>;

enum MatchState { MATCH_NOT_STARTED = 'N', MATCH_IS_STARTING = 'G', MATCH_IS_STARTED = 'S', MATCH_IS_OVER = 'O' };

enum class UserPresence : char { ACTIVE = 'A', LEFT = 'L' };

struct MatchInitOptions
{
    uint32_t bench_computers_to_player_num_{0};
    bool is_formal_{true};
};

struct MatchRuntimeOptions
{
    struct ResourceHolder
    {
        std::string resource_dir_;
        std::string saved_image_dir_;
    };

    ResourceHolder resource_holder_;
    lgtbot::game::GenericOptions generic_options_;
};

struct MatchParticipantUser
{
    MatchParticipantUser(UserID uid, MsgSender sender)
        : uid_(std::move(uid))
        , sender_(std::move(sender))
    {}

    MatchParticipantUser(MatchParticipantUser&& other) noexcept
        : uid_(std::move(other.uid_))
        , pid_(other.pid_)
        , sender_(std::move(other.sender_))
        , presence_(other.presence_.load(std::memory_order_relaxed))
        , want_interrupt_(other.want_interrupt_.load(std::memory_order_relaxed))
        , leave_when_config_changed_(other.leave_when_config_changed_)
    {}

    MatchParticipantUser& operator=(MatchParticipantUser&& other) noexcept
    {
        if (this != &other) {
            uid_ = std::move(other.uid_);
            pid_ = other.pid_;
            sender_ = std::move(other.sender_);
            presence_.store(other.presence_.load(std::memory_order_relaxed), std::memory_order_relaxed);
            want_interrupt_.store(other.want_interrupt_.load(std::memory_order_relaxed), std::memory_order_relaxed);
            leave_when_config_changed_ = other.leave_when_config_changed_;
        }
        return *this;
    }

    MatchParticipantUser(const MatchParticipantUser&) = delete;
    MatchParticipantUser& operator=(const MatchParticipantUser&) = delete;

    UserID uid_;
    PlayerID pid_{UINT32_MAX};
    mutable MsgSender sender_;
    std::atomic<UserPresence> presence_{UserPresence::ACTIVE};
    std::atomic<bool> want_interrupt_{false};
    bool leave_when_config_changed_{true};
};

enum class PlayerState : char { ACTIVE = 'A', ELIMINATED = 'E', HOOKED = 'H' };

struct MatchPlayer
{
    explicit MatchPlayer(MatchVariantID id) : id_(std::move(id)) {}

    MatchPlayer(MatchPlayer&& other) noexcept
        : id_(std::move(other.id_))
        , state_(other.state_.load(std::memory_order_relaxed))
    {}

    MatchPlayer& operator=(MatchPlayer&& other) noexcept
    {
        if (this != &other) {
            id_ = std::move(other.id_);
            state_.store(other.state_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }

    MatchPlayer(const MatchPlayer&) = delete;
    MatchPlayer& operator=(const MatchPlayer&) = delete;

    bool IsEliminated() const { return state_.load(std::memory_order_acquire) == PlayerState::ELIMINATED; }

    bool IsHooked() const { return state_.load(std::memory_order_acquire) == PlayerState::HOOKED; }

    PlayerState State() const { return state_.load(std::memory_order_acquire); }

    void SetState(const PlayerState state) { state_.store(state, std::memory_order_release); }

    PlayerState ExchangeState(const PlayerState state) { return state_.exchange(state, std::memory_order_acq_rel); }

    MatchVariantID id_;
    std::atomic<PlayerState> state_{PlayerState::ACTIVE};
};

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

} // namespace match
} // namespace core
} // namespace lgtbot
