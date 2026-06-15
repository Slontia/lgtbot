// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <random>
#include <ranges>

#include "utility/log.h"
#include "bot_core/match_manager.h"
#include "bot_core/options.h"
#include "match_process/match_ipc.pb.h"

namespace {

MatchRuntimeOptions MakeLobbyRuntimeOptions(const MatchContext& ctx, const MatchInitOptions& init_options)
{
    MatchRuntimeOptions options;
    options.resource_holder_.resource_dir_ =
        (std::filesystem::absolute(ctx.bot.game_path()) / ctx.game_handle.Info().module_name_ / "resource" / "")
                .string();
    options.resource_holder_.saved_image_dir_ =
        (std::filesystem::absolute(ctx.bot.image_path()) / "matches" /
         (std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "_" +
          ctx.game_handle.Info().module_name_))
                .string();
    options.generic_options_ = lgtbot::game::GenericOptions{
        lgtbot::game::ImmutableGenericOptions{
            .public_timer_alert_ = GET_OPTION_VALUE(*ctx.bot.option().lock(), 计时公开提示),
            .resource_dir_ = options.resource_holder_.resource_dir_.c_str(),
            .saved_image_dir_ = options.resource_holder_.saved_image_dir_.c_str(),
        },
        lgtbot::game::MutableGenericOptions{
            .bench_computers_to_player_num_ = init_options.bench_computers_to_player_num_,
            .is_formal_ = init_options.is_formal_,
        }
    };
    return options;
}

} // namespace

Lobby::Lobby(const MatchContext& ctx, MatchMessaging* messaging,
        std::map<UserID, MatchParticipantUser>& users, std::vector<MatchPlayer>& players,
        const UserID host_uid, MatchInitOptions init_options)
        : MatchPhaseCommon(ctx, messaging, users, players)
        , host_uid_(host_uid)
        , options_(MakeLobbyRuntimeOptions(ctx, init_options))
        , applied_options_log_(std::move(init_options.applied_options_log_))
{}

MsgSenderBase* Lobby::GroupSenderOrNull_()
{
    if ((*messaging_->group_sender).has_value()) {
        return &*(*messaging_->group_sender);
    }
    return nullptr;
}

MatchVariantID Lobby::HostVariantId_() const { return host_uid_; }

uint32_t Lobby::PlayerNum_() const
{
    return std::max(static_cast<size_t>(options_.generic_options_.bench_computers_to_player_num_), users_.size());
}

uint32_t Lobby::ComputerNumImpl_() const { return PlayerNum_() - static_cast<uint32_t>(users_.size()); }

const MatchRuntimeOptions& Lobby::RuntimeOptions_() const { return options_; }

bool Lobby::Has_(const UserID uid) const { return users_.find(uid) != users_.end(); }

void Lobby::EmplaceUser_(const UserID uid)
{
    const auto [it, inserted] = users_.emplace(uid, MatchParticipantUser(uid, ctx_.bot.MakeMsgSender(uid)));
    if (inserted) {
        it->second.uid_ = uid;
    }
}

UserID Lobby::HostUserId() const { return host_uid_; }

ErrCode Lobby::SetBenchTo(const UserID uid, MsgSenderBase& reply, const uint64_t bench_computers_to_player_num)
{
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << ctx_.HostUserName(host_uid_);
        return EC_MATCH_NOT_HOST;
    }
    auto sender = reply();
    if (bench_computers_to_player_num <= users_.size()) {
        sender << "[警告] 当前玩家数 " << users_.size() << " 已满足条件";
        return EC_OK;
    }
    if (const auto max_player = ctx_.MaxPlayerNum(); max_player != 0 && bench_computers_to_player_num > max_player) {
        sender << "[错误] 设置失败：比赛人数将超过上限" << max_player << "人";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    options_.generic_options_.bench_computers_to_player_num_ = bench_computers_to_player_num;
    KickForConfigChange_();
    { std::string brief; BriefInfo(brief); sender << "设置成功！\n\n" << brief; }
    return EC_OK;
}

ErrCode Lobby::SetFormal(const UserID uid, MsgSenderBase& reply, const bool is_formal)
{
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << ctx_.HostUserName(host_uid_);
        return EC_MATCH_NOT_HOST;
    }
    const auto multiple = ctx_.Multiple();
    if (multiple == 0) {
        reply() << "[错误] 当前配置下倍率为 0，固定为非正式游戏";
        return EC_MATCH_INVALID_CONFIG_VALUE;
    }
    options_.generic_options_.is_formal_ = is_formal;
    KickForConfigChange_();
    if (is_formal) {
        reply() << "设置成功！当前游戏为正式游戏，倍率为 " << multiple;
    } else {
        reply() << "设置成功！当前游戏为试玩游戏";
    }
    return EC_OK;
}

ErrCode Lobby::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply,
        const std::weak_ptr<const Match>& match_wk)
{
    const auto it = users_.find(uid);
    if (it == users_.end() || !UserIsActive(it->second)) {
        reply() << "[错误] 您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    reply.SetMatch(match_wk);
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << ctx_.HostUserName(host_uid_);
        return EC_MATCH_NOT_HOST;
    }
    uint64_t max_player = 0;
    uint32_t multiple = 0;
    if (!ctx_.game_handle.ConfigClient().SetDefaultOption(msg, max_player, multiple)) {
        reply() << "[错误] 未预料的游戏设置，您可以通过「帮助」（不带" META_COMMAND_SIGN "号）查看所有支持的游戏设置\n"
                   "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN
                   "帮助」查看所有支持的元指令";
        return EC_GAME_REQUEST_NOT_FOUND;
    }
    ctx_.game_handle.UpdateCachedLimits(max_player, multiple);
    applied_options_log_.push_back(msg);
    KickForConfigChange_();
    { std::string brief; BriefInfo(brief); reply() << "设置成功！\n\n" << brief; }
    return EC_GAME_REQUEST_OK;
}

std::optional<LobbyGameStartPlan> Lobby::BeginGameStart(const UserID uid, MsgSenderBase& reply,
        const std::weak_ptr<const Match>& match_wk, ErrCode& err_out) &&
{
    if (uid != host_uid_) {
        reply() << "[错误] 开始失败：您并非房主，没有开始游戏的权限，房主是" << ctx_.HostUserName(host_uid_);
        err_out = EC_MATCH_NOT_HOST;
        return std::nullopt;
    }

    players_.clear();
    for (auto& [user_id, user_info] : users_) {
        players_.emplace_back(user_id);
        user_info.sender_.SetMatch(match_wk);
    }
    for (ComputerID cid = 0; cid < ComputerNumImpl_(); ++cid) {
        players_.emplace_back(cid);
    }
    if (ctx_.game_handle.Info().shuffled_player_id_) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(players_.begin(), players_.end(), g);
    }
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        const auto user_id = std::get_if<UserID>(&players_[pid.Get()].id_);
        if (!user_id) {
            continue;
        }
        const auto it = users_.find(*user_id);
        assert(it != users_.end());
        it->second.pid_ = pid;
    }
    options_.generic_options_.user_num_ = static_cast<uint32_t>(users_.size());

    LobbyGameStartPlan plan;
    plan.user_num = static_cast<uint32_t>(users_.size());
    plan.child_runtime_options = MatchChildClient::RuntimeOptions{
        {
            options_.resource_holder_.resource_dir_,
            options_.resource_holder_.saved_image_dir_,
        },
        options_.generic_options_,
        GET_OPTION_VALUE(*ctx_.bot.option().lock(), 计时公开提示),
    };
    plan.options_to_sync = applied_options_log_;
    plan.players_for_child.reserve(players_.size());
    for (const auto& pl : players_) {
        lgtbot::ipc::PlayerInfo pi;
        if (const auto* const cid = std::get_if<ComputerID>(&pl.id_)) {
            pi.set_computer(true);
            pi.set_computer_id(cid->Get());
        } else {
            const auto& player_uid = std::get<UserID>(pl.id_);
            pi.set_computer(false);
            pi.set_display_name(
                    ctx_.bot.GetUserName(player_uid.GetCStr(), ctx_.gid.has_value() ? ctx_.gid->GetCStr() : nullptr));
            pi.set_avatar(ctx_.bot.GetUserAvatar(player_uid.GetCStr(), 0));
        }
        plan.players_for_child.push_back(std::move(pi));
    }

    err_out = EC_OK;
    return plan;
}

void Lobby::RollbackPreparedStart() && { players_.clear(); }

std::optional<LobbyRunningHandoff> Lobby::IntoRunning() &&
{
    if (players_.empty()) {
        return std::nullopt;
    }
    LobbyRunningHandoff handoff;
    handoff.snapshot.host_uid_ = host_uid_;
    handoff.snapshot.options_ = options_;
    handoff.snapshot.applied_options_log_ = std::move(applied_options_log_);
    return handoff;
}

ErrCode Lobby::Join(const UserID uid, MsgSenderBase& reply)
{
    if (const auto max_player = ctx_.MaxPlayerNum(); max_player != 0 && users_.size() >= max_player) {
        reply() << "[错误] 加入失败：比赛人数已达到游戏上限";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    if (Has_(uid)) {
        reply() << "[错误] 加入失败：您已加入该游戏";
        return EC_MATCH_USER_ALREADY_IN_MATCH;
    }
    EmplaceUser_(uid);
    { std::string brief; BriefInfo(brief); Boardcast() << "玩家 " << At(uid) << " 加入了游戏\n\n" << brief; }
    return EC_OK;
}

ErrCode Lobby::Leave(const UserID uid, MsgSenderBase& reply, const bool /*force*/)
{
    const auto it = users_.find(uid);
    if (it == users_.end() || !UserIsActive(it->second)) {
        reply() << "[错误] 退出失败：您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    users_.erase(uid);
    reply() << "退出成功";
    { std::string brief; BriefInfo(brief); Boardcast() << "玩家 " << At(uid) << " 退出了游戏\n\n" << brief; }
    if (users_.empty()) {
        Boardcast() << "所有玩家都退出了游戏，游戏解散";
    } else if (uid == host_uid_) {
        host_uid_ = users_.begin()->first;
        Boardcast() << At(host_uid_) << "被选为新房主";
    }
    return EC_OK;
}

ErrCode Lobby::UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel)
{
    const auto it = users_.find(uid);
    const char* const operation_str = cancel ? "取消中断" : "确定中断";
    if (it == users_.end() || !UserIsActive(it->second)) {
        reply() << "[错误] " << operation_str << "失败：您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    reply() << "[错误] " << operation_str << "失败：比赛尚未开始";
    return EC_MATCH_NOT_BEGIN;
}

ErrCode Lobby::Terminate(const bool /*is_force*/)
{
    BoardcastAtAll() << "游戏已解散，谢谢大家参与";
    LogMatch(InfoLog(), ctx_, host_uid_) << "Match is terminated outside";
    return EC_OK;
}

bool Lobby::SwitchHost()
{
    if (users_.empty()) {
        LogMatch(InfoLog(), ctx_, host_uid_) << "SwitchHost but no users left";
        return false;
    }
    host_uid_ = users_.begin()->first;
    Boardcast() << At(host_uid_) << "被选为新房主";
    LogMatch(InfoLog(), ctx_, host_uid_) << "SwitchHost succeed";
    return true;
}

void Lobby::ShowInfo(MsgSenderBase& reply, const std::weak_ptr<const Match>& match_wk) const
{
    ShowInfoHeader_(reply, match_wk, "未开始");
    auto sender = reply();
    sender << "\n当前报名玩家：" << users_.size() << "人";
    for (const auto& [uid, _] : users_) {
        sender << "\n" << Name(uid);
    }
}

void Lobby::KickForConfigChange_()
{
    std::vector<UserID> kicked;
    for (auto it = users_.begin(); it != users_.end(); ) {
        if (it->first != host_uid_ && it->second.leave_when_config_changed_) {
            kicked.push_back(it->first);
            ctx_.bot.match_manager().UnbindMatch(it->first);
            it = users_.erase(it);
        } else {
            ++it;
        }
    }
    if (kicked.empty()) {
        return;
    }
    messaging_->private_broadcast_scratch->reset();
    auto sender = Boardcast();
    for (const auto uid : kicked) {
        sender << At(uid);
    }
    sender << "\n游戏配置已经发生变更，请重新加入游戏";
}
