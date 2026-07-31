// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match.h"

#include <cassert>

#include <filesystem>
#include <numeric>
#include <algorithm>
#include <utility> // g++12 has a bug which will cause 'exchange' is not a member of 'std'
#include <ranges>
#include <random>

#include "utility/msg_checker.h"
#include "utility/log.h"
#include "utility/empty_func.h"
#include "game_framework/game_main.h"
#include "bot_core/db_manager.h"
#include "bot_core/match.h"
#include "bot_core/match_manager.h"
#include "bot_core/score_calculation.h"
#include "bot_core/options.h"
#include "bot_core/match_child_client.h"
#include "match_process/match_ipc.pb.h"
#include "nlohmann/json.hpp"

#include <cstdlib>

#ifndef MATCH_GAME_RUNNER_PATH
#define MATCH_GAME_RUNNER_PATH "match_game_runner"
#endif

namespace {

void AppendMsgItem(MsgSenderBase::MsgSenderGuard& g, const lgtbot::ipc::MsgItem& item)
{
    switch (item.content_case()) {
    case lgtbot::ipc::MsgItem::kText:       g << item.text(); break;
    case lgtbot::ipc::MsgItem::kAtPlayerId: g << At(PlayerID{item.at_player_id()}); break;
    case lgtbot::ipc::MsgItem::kUserId:     g << Name(UserID{item.user_id()}); break;
    case lgtbot::ipc::MsgItem::kImagePath:  g << Image{item.image_path()}; break;
    case lgtbot::ipc::MsgItem::kMarkdown:   g << Markdown{item.markdown().text(), item.markdown().width()}; break;
    default: break;
    }
}

std::filesystem::path ResolveRunnerExe()
{
    if (const char* const e = std::getenv("LGTBOT_MATCH_RUNNER")) {
        return e;
    }
    return std::filesystem::path(MATCH_GAME_RUNNER_PATH);
}

class HelpTextCollector final : public MsgSenderBase
{
  public:
    explicit HelpTextCollector(std::string& out) : out_(out) {}

  private:
    void SetMatch(std::weak_ptr<const Match> /*match*/) override {}

    void Flush(std::vector<MsgFragment>&& messages) const override
    {
        if (!out_.empty()) {
            return;
        }
        for (const auto& frag : messages) {
            if (const auto* text = std::get_if<std::string>(&frag)) {
                out_ = *text;
                return;
            }
        }
    }

    std::string& out_;
};

std::filesystem::path GameLibraryPath(const BotCtx& bot, const GameHandle& gh)
{
    const auto base = std::filesystem::absolute(bot.game_path()) / gh.Info().module_name_;
#if defined(_WIN32)
    return base / "libgame.dll";
#elif defined(__APPLE__)
    return base / "libgame.dylib";
#else
    return base / "libgame.so";
#endif
}

} // namespace

Match::Match(BotCtx& bot, const MatchID mid, GameHandle& game_handle, InitOptions init_options,
        const UserID host_uid, const std::optional<GroupID> gid)
        : bot_(bot)
        , mid_(mid)
        , game_handle_(game_handle)
        , gid_(gid)
        , init_options_args_(std::move(init_options.init_options_args_))
{
    auto st = sync_.lock();
    st->host_uid_ = host_uid;
    st->applied_options_log_ = std::move(init_options.applied_options_log_);
    st->max_player_ = init_options.max_player_;
    st->multiple_ = init_options.multiple_;
    if (gid.has_value()) {
        st->group_sender_.emplace(bot.MakeMsgSender(*gid_));
    } else {
        st->group_sender_.reset();
    }
    st->options_.resource_holder_.resource_dir_ =
        (std::filesystem::absolute(bot_.game_path()) / game_handle_.Info().module_name_ / "resource" / "").string();
    st->options_.resource_holder_.saved_image_dir_ =
        (std::filesystem::absolute(bot_.image_path()) / "matches" /
         (std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "_" +
          game_handle_.Info().module_name_)).string();
    st->options_.generic_options_ = lgtbot::game::GenericOptions{
        lgtbot::game::ImmutableGenericOptions{
            .public_timer_alert_ = GET_OPTION_VALUE(*bot_.option().lock(), 计时公开提示),
            .resource_dir_ = st->options_.resource_holder_.resource_dir_.c_str(),
            .saved_image_dir_ = st->options_.resource_holder_.saved_image_dir_.c_str(),
        },
        lgtbot::game::MutableGenericOptions{
            .bench_computers_to_player_num_ = init_options.bench_computers_to_player_num_,
            .is_formal_ = init_options.is_formal_,
        }
    };
    EmplaceUser_(*st, host_uid);
}

Match::~Match() = default;

bool Match::Has_(const MatchLockedState& st, const UserID uid) const
{
    return st.users_.find(uid) != st.users_.end();
}

std::string Match::HostUserName_(const MatchLockedState& st) const
{
    return bot_.GetUserName(st.host_uid_.GetCStr(), gid_.has_value() ? gid_->GetCStr() : nullptr);
}

uint32_t Match::PlayerNum_(const MatchLockedState& st) const
{
    return std::max(static_cast<size_t>(st.options_.generic_options_.bench_computers_to_player_num_), st.users_.size());
}

uint32_t Match::ComputerNum_(const MatchLockedState& st) const
{
    return PlayerNum_(st) - static_cast<uint32_t>(st.users_.size());
}

void Match::EmplaceUser_(MatchLockedState& st, const UserID uid)
{
    const auto& ai_list = GET_OPTION_VALUE(*bot_.option().lock(), AI列表);
    const auto [it, inserted] =
        st.users_.emplace(uid, ParticipantUser(*this, uid, std::ranges::find(ai_list, uid.GetStr()) != std::end(ai_list)));
    if (inserted) {
        it->second.sender_.SetMatch(weak_from_this());
    }
}

void Match::BindMsgSenderMatch_()
{
    const auto wk = weak_from_this();
    auto st = sync_.lock();
    for (auto& [_, user_info] : st->users_) {
        user_info.sender_.SetMatch(wk);
    }
    if (st->group_sender_.has_value()) {
        st->group_sender_->SetMatch(wk);
    }
}

bool Match::IsInDeduction() const
{
    return sync_.lock_const()->is_in_deduction_;
}

size_t Match::UserNum() const
{
    return sync_.lock_const()->users_.size();
}

UserID Match::HostUserId() const
{
    return sync_.lock_const()->host_uid_;
}

Match::State Match::state() const
{
    return sync_.lock_const()->state_;
}

Match::VariantID Match::ConvertPidLocked_(const MatchLockedState& st, const PlayerID pid) const
{
    if (!pid.IsValid()) {
        return st.host_uid_; // TODO: UINT64_MAX for host
    }
    return st.players_[pid].id_;
}

Match::VariantID Match::ConvertPid(const PlayerID pid) const
{
    auto st = sync_.lock_const();
    return ConvertPidLocked_(*st, pid);
}

ErrCode Match::SetBenchTo(const UserID uid, MsgSenderBase& reply, const uint64_t bench_computers_to_player_num)
{
    auto st = sync_.lock();
    if (uid != st->host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_(*st);
        return EC_MATCH_NOT_HOST;
    }
    auto sender = reply();
    if (bench_computers_to_player_num <= st->users_.size()) {
        sender << "[警告] 当前玩家数 " << st->users_.size() << " 已满足条件";
        return EC_OK;
    }
    if (const auto max_player = MaxPlayerNum_(*st); max_player != 0 && bench_computers_to_player_num > max_player) {
        sender << "[错误] 设置失败：比赛人数将超过上限" << max_player << "人";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    st->options_.generic_options_.bench_computers_to_player_num_ = bench_computers_to_player_num;
    KickForConfigChange_(*st);
    { std::string brief; BriefInfo_(*st, brief); sender << "设置成功！\n\n" << brief; }
    return EC_OK;
}

ErrCode Match::SetFormal(const UserID uid, MsgSenderBase& reply, const bool is_formal)
{
    auto st = sync_.lock();
    if (uid != st->host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_(*st);
        return EC_MATCH_NOT_HOST;
    }
    const auto multiple = Multiple_(*st);
    if (multiple == 0) {
        reply() << "[错误] 当前配置下倍率为 0，固定为非正式游戏";
        return EC_MATCH_INVALID_CONFIG_VALUE;
    }
    st->options_.generic_options_.is_formal_ = is_formal;
    KickForConfigChange_(*st);
    if (is_formal) {
        reply() << "设置成功！当前游戏为正式游戏，倍率为 " << multiple;
    } else {
        reply() << "设置成功！当前游戏为试玩游戏";
    }
    return EC_OK;
}

ErrCode Match::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg,
                       MsgSender& reply)
{
    MatchChildClient* child = nullptr;
    PlayerID pid;
    bool is_eliminated = false;
    {
        auto st = sync_.lock();
        const auto it = st->users_.find(uid);
        if (it == st->users_.end() || it->second.state_ == ParticipantUser::State::LEFT) {
            reply() << "[错误] 您未处于游戏中或已经离开";
            return EC_MATCH_USER_NOT_IN_MATCH;
        }
        if (st->state_ == State::IS_OVER) {
            MatchLog_(WarnLog()) << "Match is over but receive request uid=" << uid << " msg=" << msg;
            reply() << "[错误] 游戏已经结束";
            return EC_MATCH_ALREADY_OVER;
        }
        if (st->state_ == State::IS_STARTING) {
            reply() << "[错误] 游戏正在开始，请稍候再试";
            return EC_MATCH_ALREADY_BEGIN;
        }
        reply.SetMatch(weak_from_this());
        if (st->state_ == State::IS_STARTED) {
            pid = it->second.pid_;
            is_eliminated = st->players_[pid].state_ == Player::State::ELIMINATED;
            child = st->game_child_.get();
        } else {
            {
                // The lobby branch returns without falling through to the help check below,
                // so route "帮助" here first; any joined user may view it, not only the host.
                MsgReader reader(msg);
                if (help_cmd_.CallIfValid(reader, reply)) {
                    return EC_GAME_REQUEST_OK;
                }
            }
            if (uid != st->host_uid_) {
                reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_(*st);
                return EC_MATCH_NOT_HOST;
            }
            uint64_t max_player = 0;
            uint32_t multiple = 0;
            // Use TryMatchOption (not SetDefaultOption) so the change stays local to this match:
            // SetDefaultOption mutates the config_runner's shared default_options_ and
            // applied_options_log_, which then leaks into every subsequent match.
            // Global defaults are only changed via %配置.
            if (!game_handle_.ConfigClient().TryMatchOption(st->applied_options_log_, msg, max_player, multiple)) {
                reply() << "[错误] 未预料的游戏设置，您可以通过「帮助」（不带" META_COMMAND_SIGN "号）查看所有支持的游戏设置\n"
                            "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN "帮助」查看所有支持的元指令";
                return EC_GAME_REQUEST_NOT_FOUND;
            }
            st->max_player_ = max_player;
            st->multiple_ = multiple;
            st->applied_options_log_.push_back(msg);
            KickForConfigChange_(*st);
            { std::string brief; BriefInfo_(*st, brief); reply() << "设置成功！\n\n" << brief; }
            return EC_GAME_REQUEST_OK;
        }
    }
    {
        MsgReader reader(msg);
        if (help_cmd_.CallIfValid(reader, reply)) {
            return EC_GAME_REQUEST_OK;
        }
    }
    if (is_eliminated) {
        reply() << "[错误] 您已经被淘汰，无法执行游戏请求";
        return EC_MATCH_ELIMINATED;
    }
    if (!child) {
        return EC_MATCH_ALREADY_OVER;
    }
    auto exec_fut = child->SendExecute(pid, gid.has_value(), msg, reply);
    if (!exec_fut) {
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    ErrCode rc = exec_fut->get();
    {
        auto st = sync_.lock();
        if (rc == EC_GAME_REQUEST_FAILED && (!st->game_child_ || st->state_ == State::IS_OVER)) {
            // Child died unexpectedly (e.g. test_game "崩溃"); treat as broken IPC/session.
            rc = EC_MATCH_UNEXPECTED_CONFIG;
        }
    }
    if (rc == EC_GAME_REQUEST_NOT_FOUND) {
        reply() << "[错误] 未预料的游戏指令，您可以通过「帮助」（不带" META_COMMAND_SIGN "号）查看所有支持的游戏指令\n"
                    "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN "帮助」查看所有支持的元指令";
    }
    return rc;
}

ErrCode Match::GameStart(const UserID uid, MsgSenderBase& reply)
{
    MatchChildClient::RuntimeOptions child_runtime_options;
    std::vector<std::string> options_to_sync;
    std::vector<lgtbot::ipc::PlayerInfo> players_for_child;
    uint32_t user_num = 0;
    const auto self = shared_from_this();
    const ChildIpcPushHandler push_handler = [self](const PushFrame& frame) { self->ApplyChildIpcFromReadThread_(frame); };
    const ChildIpcEofHandler eof_handler = [self](const bool unexpected) { self->ApplyChildEofFromReadThread_(unexpected); };

    {
        auto st = sync_.lock();
        if (st->state_ != State::NOT_STARTED) {
            reply() << "[错误] 开始失败：游戏已经开始";
            return EC_MATCH_ALREADY_BEGIN;
        }
        if (uid != st->host_uid_) {
            reply() << "[错误] 开始失败：您并非房主，没有开始游戏的权限，房主是" << HostUserName_(*st);
            return EC_MATCH_NOT_HOST;
        }

        st->players_.clear();
        for (auto& [user_id, user_info] : st->users_) {
            st->players_.emplace_back(user_id);
            user_info.sender_.SetMatch(weak_from_this());
        }
        for (ComputerID cid = 0; cid < ComputerNum_(*st); ++cid) {
            st->players_.emplace_back(cid);
        }
        if (game_handle_.Info().shuffled_player_id_) {
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(st->players_.begin(), st->players_.end(), g);
        }
        for (PlayerID pid = 0; pid.Get() < st->players_.size(); ++pid) {
            const auto user_id = std::get_if<UserID>(&st->players_[pid].id_);
            if (!user_id) {
                continue;
            }
            const auto it = st->users_.find(*user_id);
            assert(it != st->users_.end());
            it->second.pid_ = pid;
        }
        st->options_.generic_options_.user_num_ = static_cast<uint32_t>(st->users_.size());
        user_num = static_cast<uint32_t>(st->users_.size());

        child_runtime_options = MatchChildClient::RuntimeOptions{
            {
                st->options_.resource_holder_.resource_dir_,
                st->options_.resource_holder_.saved_image_dir_,
            },
            st->options_.generic_options_,
            GET_OPTION_VALUE(*bot_.option().lock(), 计时公开提示),
        };
        options_to_sync = st->applied_options_log_;

        players_for_child.reserve(st->players_.size());
        for (const auto& pl : st->players_) {
            lgtbot::ipc::PlayerInfo pi;
            if (const auto* const cid = std::get_if<ComputerID>(&pl.id_)) {
                pi.set_computer(true);
                pi.set_computer_id(cid->Get());
            } else {
                const auto& player_uid = std::get<UserID>(pl.id_);
                pi.set_computer(false);
                pi.set_display_name(bot_.GetUserName(player_uid.GetCStr(), gid_.has_value() ? gid_->GetCStr() : nullptr));
                pi.set_avatar(bot_.GetUserAvatar(player_uid.GetCStr(), 0));
            }
            players_for_child.push_back(std::move(pi));
        }

        st->state_ = State::IS_STARTING;
    }

    auto new_child = MakeMatchChildClient(ResolveRunnerExe(), GameLibraryPath(bot_, game_handle_), child_runtime_options,
            push_handler, eof_handler);
    {
        auto st = sync_.lock();
        st->game_child_ = std::move(new_child);
        if (!st->game_child_) {
            st->state_ = State::NOT_STARTED;
            reply() << "[错误] 开始失败：无法启动游戏子进程";
            return EC_MATCH_UNEXPECTED_CONFIG;
        }
    }

    MatchChildClient* child = nullptr;
    {
        auto st = sync_.lock();
        child = st->game_child_.get();
    }

    for (const auto& line : options_to_sync) {
        {
            auto st = sync_.lock();
            if (!st->game_child_) {
                st->state_ = State::NOT_STARTED;
                reply() << "[错误] 开始失败：游戏已被中断";
                return EC_MATCH_ALREADY_BEGIN;
            }
            child = st->game_child_.get();
        }
        auto opt_fut = child->SendSetOption(line);
        if (!opt_fut || opt_fut->get() != lgtbot::ipc::ResultResp::STAGE_OK) {
            auto st = sync_.lock();
            st->state_ = State::NOT_STARTED;
            st->game_child_.reset();
            reply() << "[错误] 开始失败：无法同步游戏设置到子进程";
            return EC_MATCH_UNEXPECTED_CONFIG;
        }
    }

    // Replay the preset command (from "#新游戏 <game> <args>") on top of the synced options.
    if (!init_options_args_.empty()) {
        {
            auto st = sync_.lock();
            if (!st->game_child_) {
                st->state_ = State::NOT_STARTED;
                reply() << "[错误] 开始失败：游戏已被中断";
                return EC_MATCH_ALREADY_BEGIN;
            }
            child = st->game_child_.get();
        }
        auto init_opts_fut = child->SendApplyInitOptions(init_options_args_);
        if (!init_opts_fut || init_opts_fut->get() != lgtbot::ipc::ResultResp::STAGE_OK) {
            auto st = sync_.lock();
            st->state_ = State::NOT_STARTED;
            st->game_child_.reset();
            reply() << "[错误] 开始失败：无法同步预设指令到子进程";
            return EC_MATCH_UNEXPECTED_CONFIG;
        }
    }

    {
        auto st = sync_.lock();
        if (!st->game_child_) {
            st->state_ = State::NOT_STARTED;
            reply() << "[错误] 开始失败：游戏已被中断";
            return EC_MATCH_ALREADY_BEGIN;
        }
        child = st->game_child_.get();
    }

    auto start_fut = child->SendStart(MatchId(), user_num, players_for_child);
    if (!start_fut || start_fut->get() != lgtbot::ipc::ResultResp::STAGE_OK) {
        auto st = sync_.lock();
        st->state_ = State::NOT_STARTED;
        st->game_child_.reset();
        reply() << "[错误] 开始失败：不符合游戏参数的预期";
        return EC_MATCH_UNEXPECTED_CONFIG;
    }

    {
        auto st = sync_.lock();
        if (st->state_ == State::IS_OVER) {
            st->game_child_.reset();
            return EC_OK;
        }
        st->state_ = State::IS_STARTED;
        BoardcastAtAllLocked_(*st) << "游戏开始，您可以使用「帮助」命令（不带" META_COMMAND_SIGN "号），查看可执行命令";
        {
            nlohmann::json players_json_array = nlohmann::json::array();
            for (const auto& pi : players_for_child) {
                if (pi.computer()) {
                    players_json_array.push_back(nlohmann::json{{"computer_id", pi.computer_id()}});
                } else {
                    players_json_array.push_back(nlohmann::json{{"display_name", pi.display_name()}});
                }
            }
            BoardcastAiInfoLocked_(*st) << nlohmann::json{
                    { "match_id", MatchId() },
                    { "state", "started" },
                    { "players", std::move(players_json_array) },
                }.dump();
        }
    }

    return EC_OK;
}

ErrCode Match::Join(const UserID uid, MsgSenderBase& reply)
{
    auto st = sync_.lock();
    if (st->state_ != State::NOT_STARTED) {
        reply() << "[错误] 加入失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    if (const auto max_player = MaxPlayerNum_(*st); max_player != 0 && st->users_.size() >= max_player) {
        reply() << "[错误] 加入失败：比赛人数已达到游戏上限";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    if (Has_(*st, uid)) {
        reply() << "[错误] 加入失败：您已加入该游戏";
        return EC_MATCH_USER_ALREADY_IN_MATCH;
    }
    if (!match_manager().BindMatch(uid, shared_from_this())) {
        reply() << "[错误] 加入失败：您已加入其他游戏，您可通过私信裁判「" META_COMMAND_SIGN "游戏信息」查看该游戏信息";
        return EC_MATCH_USER_ALREADY_IN_OTHER_MATCH;
    }
    EmplaceUser_(*st, uid);
    { std::string brief; BriefInfo_(*st, brief); BoardcastLocked_(*st) << "玩家 " << At(uid) << " 加入了游戏\n\n" << brief; }
    return EC_OK;
}

ErrCode Match::Leave(const UserID uid, MsgSenderBase& reply, const bool force)
{
    PlayerID leave_pid;
    std::unique_ptr<MatchChildClient> terminate_child;
    MatchChildClient* child_for_leave = nullptr;
    {
        auto st = sync_.lock();
        const auto it = st->users_.find(uid);
        if (it == st->users_.end() || it->second.state_ == ParticipantUser::State::LEFT) {
            reply() << "[错误] 退出失败：您未处于游戏中或已经离开";
            return EC_MATCH_USER_NOT_IN_MATCH;
        }
        if (st->state_ == State::IS_OVER) {
            reply() << "[错误] 退出失败：游戏已经结束";
            return EC_MATCH_ALREADY_OVER;
        }
        if (st->state_ == State::IS_STARTING) {
            reply() << "[错误] 游戏正在开始，请稍候再试";
            return EC_MATCH_ALREADY_BEGIN;
        } else if (st->state_ != State::IS_STARTED) {
            match_manager().UnbindMatch(uid);
            st->users_.erase(uid);
            reply() << "退出成功";
            { std::string brief; BriefInfo_(*st, brief); BoardcastLocked_(*st) << "玩家 " << At(uid) << " 退出了游戏\n\n" << brief; }
            if (st->users_.empty()) {
                BoardcastLocked_(*st) << "所有玩家都退出了游戏，游戏解散";
                Unbind_();
            } else if (uid == st->host_uid_) {
                st->host_uid_ = st->users_.begin()->first;
                BoardcastLocked_(*st) << At(st->host_uid_) << "被选为新房主";
            }
            return EC_OK;
        } else if (force || st->players_[it->second.pid_].state_ == Player::State::ELIMINATED) {
            match_manager().UnbindMatch(uid);
            reply() << "退出成功";
            BoardcastLocked_(*st) << "玩家 " << At(uid) << " 中途退出了游戏，他将不再参与后续的游戏进程";
            assert(st->game_child_);
            assert(it->second.state_ != ParticipantUser::State::LEFT);
            it->second.state_ = ParticipantUser::State::LEFT;
            if (std::ranges::all_of(st->users_, [](const auto& user)
                    { return user.second.state_ == ParticipantUser::State::LEFT; })) {
                BoardcastLocked_(*st) << "所有玩家都强制退出了游戏，那还玩啥玩，游戏解散，结果不会被记录";
                MatchLog_(InfoLog()) << "All users left the game";
                terminate_child = PrepareTerminate_(*st);
            } else {
                leave_pid = it->second.pid_;
                child_for_leave = st->game_child_.get();
            }
        } else {
            reply() << "[错误] 退出失败：游戏已经开始，若仍要退出游戏，请使用「" META_COMMAND_SIGN "退出 强制」命令";
            return EC_MATCH_ALREADY_BEGIN;
        }
    }
    if (terminate_child) {
        FinishTerminate_(std::move(terminate_child));
    } else if (child_for_leave) {
        if (auto leave_fut = child_for_leave->SendLeave(leave_pid); leave_fut) {
            (void)leave_fut->get();
        }
    }
    return EC_OK;
}

MsgSenderBase& Match::BoardcastMsgSenderLocked_(MatchLockedState& st)
{
    if (st.group_sender_.has_value()) {
        return *st.group_sender_;
    }
    return boardcast_private_sender_;
}

MsgSenderBase& Match::BoardcastMsgSender()
{
    auto locked = sync_.lock();
    return BoardcastMsgSenderLocked_(*locked);
}

MsgSenderBase& Match::BoardcastAiInfoMsgSenderLocked_(MatchLockedState& st)
{
    if (!st.group_sender_.has_value()) {
        return boardcast_ai_info_private_sender_;
    }
    if (std::ranges::any_of(st.users_, [](const auto& user) { return user.second.is_ai_; })) {
        return *st.group_sender_;
    }
    return EmptyMsgSender::Get();
}

MsgSenderBase& Match::BoardcastAiInfoMsgSender()
{
    auto locked = sync_.lock();
    return BoardcastAiInfoMsgSenderLocked_(*locked);
}

MsgSenderBase& Match::TellMsgSenderLocked_(MatchLockedState& st, const PlayerID pid)
{
    const auto& id = ConvertPidLocked_(st, pid);
    const auto pval = std::get_if<UserID>(&id);
    if (!pval) {
        return EmptyMsgSender::Get(); // is computer
    }
    if (const auto it = st.users_.find(*pval); it != st.users_.end() && it->second.state_ != ParticipantUser::State::LEFT) {
        return it->second.sender_;
    }
    return EmptyMsgSender::Get(); // player exit
}

MsgSenderBase& Match::TellMsgSender(const PlayerID pid)
{
    auto locked = sync_.lock();
    return TellMsgSenderLocked_(*locked, pid);
}

MsgSenderBase& Match::GroupMsgSenderLocked_(MatchLockedState& st)
{
    if (st.group_sender_.has_value()) {
        return *st.group_sender_;
    }
    return EmptyMsgSender::Get();
}

MsgSenderBase& Match::GroupMsgSender()
{
    auto locked = sync_.lock();
    return GroupMsgSenderLocked_(*locked);
}

MsgSenderBase::MsgSenderGuard Match::BoardcastLocked_(MatchLockedState& st)
{
    return BoardcastMsgSenderLocked_(st)();
}

MsgSenderBase::MsgSenderGuard Match::BoardcastAiInfoLocked_(MatchLockedState& st)
{
    return BoardcastAiInfoMsgSenderLocked_(st)();
}

MsgSenderBase::MsgSenderGuard Match::TellLocked_(MatchLockedState& st, const PlayerID pid)
{
    return TellMsgSenderLocked_(st, pid)();
}

MsgSenderBase::MsgSenderGuard Match::GroupLocked_(MatchLockedState& st)
{
    return GroupMsgSenderLocked_(st)();
}

MsgSenderBase::MsgSenderGuard Match::BoardcastAtAllLocked_(MatchLockedState& st)
{
    if (gid_.has_value()) {
        auto sender = BoardcastLocked_(st);
        for (auto& [uid, user_info] : st.users_) {
            if (user_info.state_ != ParticipantUser::State::LEFT) {
                sender << At(uid);
            }
        }
        sender << "\n";
        return sender;
    }
    return BoardcastLocked_(st);
}

const char* Match::PlayerName(const PlayerID& pid)
{
    thread_local static std::string str;
    const auto& id = ConvertPid(pid);
    if (const auto pval = std::get_if<ComputerID>(&id)) {
        return (str = "机器人" + std::to_string(pval->Get()) + "号").c_str();
    }
    return (str = bot_.GetUserName(std::get<UserID>(id).GetCStr(), gid().has_value() ? gid()->GetCStr() : nullptr)).c_str();
}

const char* Match::PlayerAvatar(const PlayerID& pid, const int32_t size)
{
    thread_local static std::string str;
    const auto& id = ConvertPid(pid);
    if (const auto pval = std::get_if<ComputerID>(&id)) {
        return "";
    }
    return (str = bot_.GetUserAvatar(std::get<UserID>(id).GetCStr(), size)).c_str();
}

MsgSenderBase::MsgSenderGuard Match::BoardcastAtAll()
{
    auto locked = sync_.lock();
    return BoardcastAtAllLocked_(*locked);
}

bool Match::SwitchHost()
{
    auto st = sync_.lock();
    return SwitchHost_(*st);
}

bool Match::SwitchHost_(MatchLockedState& st)
{
    if (st.users_.empty()) {
        MatchLog_(InfoLog()) << "SwitchHost but no users left";
        return false;
    }
    if (st.state_ == NOT_STARTED) {
        st.host_uid_ = st.users_.begin()->first;
        BoardcastLocked_(st) << At(st.host_uid_) << "被选为新房主";
        MatchLog_(InfoLog()) << "SwitchHost succeed";
    }
    return true;
}

// Must be called with sync_ held.
void Match::ApplyChildPost_(MatchLockedState& st, const PostFrame& frame)
{
    using Channel = lgtbot::ipc::PostResp::Channel;
    const auto& post = frame.post;
    MsgSenderBase::MsgSenderGuard sender = [&]() -> MsgSenderBase::MsgSenderGuard
        {
            switch (post.channel()) {
            case Channel::PostResp_Channel_BROADCAST: return BoardcastLocked_(st);
            case Channel::PostResp_Channel_GROUP:     return GroupLocked_(st);
            case Channel::PostResp_Channel_AI:        return BoardcastAiInfoLocked_(st);
            case Channel::PostResp_Channel_TELL:      return TellLocked_(st, PlayerID{post.target_pid()});
            default:                                  return BoardcastLocked_(st);
            }
        }();
    for (const auto& item : post.items()) {
        AppendMsgItem(sender, item);
    }
}

void Match::DispatchChildIpc_(MatchLockedState& st, const PushFrame& frame)
{
    std::visit([this, &st](const auto& f) {
        using T = std::decay_t<decltype(f)>;
        if constexpr (std::is_same_v<T, PostFrame>) {
            ApplyChildPost_(st, f);
        } else if constexpr (std::is_same_v<T, PlayerStateFrame>) {
            ApplyChildPlayerState(st, f.pid, f.state);
        } else if constexpr (std::is_same_v<T, GameOverFrame>) {
            ApplyChildGameOverFromScores(st, f);
        }
    }, frame);
}

std::unique_ptr<MatchChildClient> Match::DispatchChildEof_(MatchLockedState& st, const bool unexpected)
{
    if (!unexpected) {
        return nullptr;
    }
    if (st.state_ == State::IS_OVER) {
        return nullptr;
    }
    BoardcastAtAllLocked_(st) << "[错误] 游戏进程意外终止，游戏已中断";
    MatchLog_(ErrorLog()) << "Game subprocess died unexpectedly, terminating match";
    st.state_ = State::IS_OVER;
    return PrepareTerminate_(st);
}

void Match::ApplyChildIpcFromReadThread_(const PushFrame& frame)
{
    auto st = sync_.lock();
    DispatchChildIpc_(*st, frame);
}

void Match::ApplyChildEofFromReadThread_(const bool unexpected)
{
    std::unique_ptr<MatchChildClient> child;
    {
        auto st = sync_.lock();
        child = DispatchChildEof_(*st, unexpected);
    }
    child.reset();
}

void Match::StartTimer(const uint64_t /*sec*/, void* /*alert_arg*/, void (*/*alert_cb*/)(void*, uint64_t))
{
    MatchLog_(WarnLog()) << "StartTimer ignored on host Match (timer runs in game subprocess)";
}

void Match::StopTimer() {}

void Match::Eliminate(const PlayerID pid)
{
    auto st = sync_.lock();
    if (std::exchange(st->players_[pid].state_, Player::State::ELIMINATED) != Player::State::ELIMINATED) {
        TellLocked_(*st, pid) << "很遗憾，您被淘汰了，可以通过「" META_COMMAND_SIGN "退出」以退出游戏";
        const bool all_players_eliminated = std::ranges::all_of(st->players_,
                [](const auto& p) { return std::get_if<ComputerID>(&p.id_) || p.state_ == Player::State::ELIMINATED; });
        const bool has_alive_computer = std::ranges::any_of(st->players_,
                [](const auto& p) { return std::get_if<ComputerID>(&p.id_) && p.state_ != Player::State::ELIMINATED; });
        st->is_in_deduction_ = all_players_eliminated && has_alive_computer;
        MatchLog_(InfoLog()) << "Eliminate player pid=" << pid << " is_in_deduction=" << Bool2Str(st->is_in_deduction_);
    }
}

void Match::Hook(const PlayerID pid)
{
    auto st = sync_.lock();
    auto& player = st->players_[pid];
    if (player.state_ == Player::State::ACTIVE) {
        TellLocked_(*st, pid) << "您已经进入挂机状态，若其他玩家已经行动完成，裁判将不再继续等待您，执行任意游戏请求可恢复至原状态";
        player.state_ = Player::State::HOOKED;
    }
}

void Match::Activate(const PlayerID pid)
{
    auto st = sync_.lock();
    auto& player = st->players_[pid];
    if (player.state_ == Player::State::HOOKED) {
        TellLocked_(*st, pid) << "挂机状态已取消";
        player.state_ = Player::State::ACTIVE;
    }
}

void Match::ShowInfo(MsgSenderBase& reply) const
{
    reply.SetMatch(weak_from_this());
    auto st = sync_.lock_const();
    auto sender = reply();
    sender << "游戏名称：" << game_handle().Info().name_ << "\n";
    sender << "配置信息：" << OptionInfo_(*st) << "\n";
    sender << "电脑数量：" << ComputerNum_(*st) << "\n";
    sender << "游戏状态："
           << (st->state_ == Match::State::NOT_STARTED ? "未开始"
               : st->state_ == Match::State::IS_STARTING ? "正在开始"
               : "已开始")
           << "\n";
    sender << "房间号：";
    if (gid_.has_value()) {
        sender << *gid_ << "\n";
    } else {
        sender << "私密游戏" << "\n";
    }
    sender << "最多可参加人数：";
    if (const auto max_player = MaxPlayerNum_(*st); max_player == 0) {
        sender << "无限制";
    } else {
        sender << max_player;
    }
    sender << "人\n房主：" << Name(st->host_uid_);
    if (st->state_ == Match::State::IS_STARTED) {
        const auto num = st->players_.size();
        sender << "\n玩家列表：" << num << "人";
        for (uint32_t pid = 0; pid < num; ++pid) {
            sender << "\n" << pid << "号：" << Name(PlayerID{pid});
        }
    } else {
        sender << "\n当前报名玩家：" << st->users_.size() << "人";
        for (const auto& [uid, _] : st->users_) {
            sender << "\n" << Name(uid);
        }
    }
}

void Match::BriefInfo(std::string& out) const
{
    auto st = sync_.lock_const();
    BriefInfo_(*st, out);
}

void Match::BriefInfo_(const MatchLockedState& st, std::string& out) const
{
    const auto multiple = Multiple_(st);
    const char* const name = game_handle().Info().name_;
    const auto is_formal = st.options_.generic_options_.is_formal_;
    const auto usize = st.users_.size();
    const auto cnum = ComputerNum_(st);
    out = std::string("游戏名称：") + name +
        "\n- 倍率：" +
        (is_formal || multiple == 0 ? std::to_string(multiple) :
                                      "0（开启计分后为 " + std::to_string(multiple) + "）") +
        "\n- 当前用户数：" + std::to_string(usize) +
        "\n- 当前电脑数：" + std::to_string(cnum);
}

std::string Match::OptionInfo_(const MatchLockedState& st) const
{
    return game_handle_.ConfigClient().QueryMatchOptionInfo(true /* text_mode */,
            st.applied_options_log_, init_options_args_);
}

void Match::ApplyChildPlayerState(MatchLockedState& st, const PlayerID pid, const std::string& state)
{
    if (pid.Get() >= st.players_.size()) {
        return;
    }
    if (state == "eliminated") {
        if (std::exchange(st.players_[pid.Get()].state_, Player::State::ELIMINATED) != Player::State::ELIMINATED) {
            st.is_in_deduction_ = std::ranges::all_of(st.players_, [](const Player& p)
                    {
                        return std::get_if<ComputerID>(&p.id_) || p.state_ == Player::State::ELIMINATED;
                    });
            MatchLog_(InfoLog()) << "Eliminate player pid=" << pid << " is_in_deduction=" << Bool2Str(st.is_in_deduction_);
        }
    } else if (state == "hooked") {
        st.players_[pid.Get()].state_ = Player::State::HOOKED;
    } else if (state == "active") {
        st.players_[pid.Get()].state_ = Player::State::ACTIVE;
    }
}

void Match::ApplyChildGameOverFromScores(MatchLockedState& st, const GameOverFrame& frame)
{
    if (st.state_ == State::IS_OVER) {
        MatchLog_(WarnLog()) << "ApplyChildGameOverFromScores but has already been over";
        return;
    }
    const auto& scores = frame.game_over.scores();
    std::vector<std::pair<UserID, int64_t>> user_game_scores;
    std::vector<std::pair<UserID, std::string>> user_achievements;
    {
        auto sender = BoardcastLocked_(st);
        sender << "游戏结束，公布分数：\n";
        for (const auto& row : scores) {
            const auto pid = PlayerID{row.pid()};
            const auto score = static_cast<int64_t>(row.score());
            sender << At(pid) << " " << score << "\n";
            const auto id = st.players_[pid].id_;
            if (const auto pval = std::get_if<UserID>(&id); pval) {
                user_game_scores.emplace_back(*pval, score);
                for (const auto& ach_name : row.achievements()) {
                    user_achievements.emplace_back(*pval, ach_name);
                    MatchLog_(InfoLog()) << "User get achievement uid=" << *pval << " achievement=" << ach_name;
                }
            }
        }
        sender << "感谢诸位参与！";

        assert(user_game_scores.size() == st.users_.size());
        std::sort(user_game_scores.begin(), user_game_scores.end(),
                [](const auto& _1, const auto& _2) { return _1.second > _2.second; });

        static const auto show_score = [](const char* const name, const auto sc)
            {
                return std::string("[") + name + (sc > 0 ? "+" : "") + std::to_string(sc) + "] ";
            };
        if (user_game_scores.size() <= 1) {
            sender << "\n\n游戏结果不记录：因为玩家数小于 2";
#ifndef WITH_SQLITE
        } else {
            sender << "\n\n游戏结果不记录：因为未连接数据库";
        }
#else
        } else if (!bot_.db_manager()) {
            sender << "\n\n游戏结果不记录：因为未连接数据库";
        } else if (const auto multiple = Multiple_(st); !st.options_.generic_options_.is_formal_ || multiple == 0) {
            sender << "\n\n游戏结果不记录：因为该游戏为非正式游戏";
        } else if (const auto score_info =
                    bot_.db_manager()->RecordMatch(game_handle_.Info().name_, gid_, st.host_uid_,
                        multiple, user_game_scores, user_achievements);
                score_info.empty()) {
            sender << "\n\n[错误] 游戏结果写入数据库失败，请联系管理员";
            MatchLog_(ErrorLog()) << "Save database failed";
        } else {
            assert(score_info.size() == st.users_.size());
            sender << "\n\n游戏结果写入数据库成功：";
            for (const auto& info : score_info) {
                sender << "\n" << At(info.uid_) << "：" << show_score("零和", info.zero_sum_score_)
                                                        << show_score("头名", info.top_score_)
                                                        << show_score("等级", info.level_score_);

            }
            if (!user_achievements.empty()) {
                sender << "\n\n有用户获得新成就：";
                for (const auto& [user_id, achievement_name] : user_achievements) {
                    sender << "\n" << At(user_id) << "：" << achievement_name;
                }
            }
        }
#endif
    }
    st.state_ = State::IS_OVER;
    game_handle_.IncreaseActivity(st.users_.size());
    MatchLog_(InfoLog()) << "Match is over normally";
    UnbindMatchSide_(st);
}

void Match::Help_(MsgSenderBase& reply, const bool text_mode)
{
    MatchChildClient* child = nullptr;
    std::vector<std::string> applied_options_log;
    {
        auto st = sync_.lock();
        child = st->game_child_.get();
        applied_options_log = st->applied_options_log_;
    }
    if (child) {
        std::string remote;
        HelpTextCollector help_collector(remote);
        auto help_fut = child->FetchHelp(text_mode, help_collector);
        if (!help_fut || help_fut->get() != lgtbot::ipc::ResultResp::STAGE_OK) {
            reply() << "[错误] 无法从游戏进程获取帮助信息";
            return;
        }
        std::string outstr = "## 当前可使用的游戏命令\n\n### 查看信息\n1. " + help_cmd_.Info(true /* with_example */, !text_mode /* with_html_color */);
        outstr += "\n";
        outstr += remote;
        if (text_mode) {
            reply() << outstr;
        } else {
            reply() << Markdown(outstr);
        }
        return;
    }
    const std::string remote_opts = game_handle_.ConfigClient().QueryMatchOptionInfo(text_mode,
            applied_options_log, init_options_args_);
    std::string outstr = "## 当前可使用的游戏命令";
    outstr += "\n\n### 查看信息";
    outstr += "\n1. " + help_cmd_.Info(true /* with_example */, !text_mode /* with_html_color */);
    if (!remote_opts.empty()) {
        outstr += "\n\n### 配置选项";
        outstr += "\n" + remote_opts;
    }
    if (text_mode) {
        reply() << outstr;
    } else {
        reply() << Markdown(outstr);
    }
}

ErrCode Match::UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel)
{
    std::unique_ptr<MatchChildClient> child;
    {
        auto st = sync_.lock();
        const auto it = st->users_.find(uid);
        const char* const operation_str = cancel ? "取消中断" : "确定中断";
        if (it == st->users_.end() || it->second.state_ == ParticipantUser::State::LEFT) {
            reply() << "[错误] " << operation_str << "失败：您未处于游戏中或已经离开";
            return EC_MATCH_USER_NOT_IN_MATCH;
        }
        if (st->state_ == State::NOT_STARTED) {
            reply() << "[错误] " << operation_str << "失败：比赛尚未开始";
            return EC_MATCH_NOT_BEGIN;
        }
        if (st->state_ == State::IS_STARTING) {
            reply() << "[错误] " << operation_str << "失败：游戏正在开始";
            return EC_MATCH_ALREADY_BEGIN;
        }
        if (st->state_ == State::IS_OVER) {
            reply() << "[错误] " << operation_str << "失败：比赛已经结束";
            return EC_MATCH_ALREADY_OVER;
        }
        it->second.want_interrupt_ = !cancel;
        const auto remain = std::count_if(st->users_.begin(), st->users_.end(), [&st](const auto& pair)
                {
                    const auto& user = pair.second;
                    return !(user.want_interrupt_ ||
                             user.state_ == ParticipantUser::State::LEFT ||
                             st->players_[user.pid_].state_ == Player::State::HOOKED);

                });
        reply() << operation_str << "成功";
        if (remain == 0) {
            BoardcastAtAllLocked_(*st) << "全员支持中断游戏，游戏已中断，谢谢大家参与";
            MatchLog_(InfoLog()) << "Match is interrupted by users";
            child = PrepareTerminate_(*st);
        } else {
            BoardcastLocked_(*st) << "有玩家" << operation_str << "比赛，目前 " << remain << " 人尚未确定中断，所有玩家可通过「" META_COMMAND_SIGN "中断」命令确定中断比赛，或「" META_COMMAND_SIGN "中断 取消」命令取消中断比赛";
        }
    }
    if (child) {
        FinishTerminate_(std::move(child));
    }
    return EC_OK;
}

ErrCode Match::Terminate(const bool is_force)
{
    std::unique_ptr<MatchChildClient> child;
    {
        auto st = sync_.lock();
        if (is_force || st->state_ == State::NOT_STARTED || st->state_ == State::IS_STARTING) {
            BoardcastAtAllLocked_(*st) << "游戏已解散，谢谢大家参与";
            MatchLog_(InfoLog()) << "Match is terminated outside";
            if (st->state_ == State::IS_STARTING) {
                st->state_ = State::NOT_STARTED;
            }
            child = PrepareTerminate_(*st);
        } else {
            return EC_MATCH_ALREADY_BEGIN;
        }
    }
    FinishTerminate_(std::move(child));
    return EC_OK;
}

void Match::UnbindMatchSide_(MatchLockedState& st)
{
    if (st.game_child_) {
        bot_.PostCleanup([self = shared_from_this(), child = std::move(st.game_child_)]() mutable {
            child.reset();
        });
    }
    for (auto& [uid, user_info] : st.users_) {
        if (user_info.state_ != ParticipantUser::State::LEFT) {
            match_manager().UnbindMatch(uid);
        }
    }
    Unbind_();
    BoardcastAiInfoLocked_(st) << nlohmann::json{
            { "match_id", MatchId() },
            { "state", "finished" },
        }.dump();
}

std::unique_ptr<MatchChildClient> Match::PrepareTerminate_(MatchLockedState& st)
{
    // Mark the match over BEFORE tearing down the child: destroying the child triggers an EOF on its read thread,
    // and DispatchChildEof_ must see IS_OVER so the stale EOF does not run a second terminate that would unbind a successor match bound to the same group/users afterwards.
    st.state_ = State::IS_OVER;
    UnbindMatchSide_(st);
    return std::move(st.game_child_);
}

void Match::FinishTerminate_(std::unique_ptr<MatchChildClient> child)
{
    child.reset();
}

void Match::ReleaseGameChildIfOver()
{
    // No-op: game child lifecycle is now managed via PrepareTerminate_/FinishTerminate_.
}

void Match::KickForConfigChange_(MatchLockedState& st)
{
    auto sender = BoardcastLocked_(st);
    bool has_kicked = false;
    for (auto it = st.users_.begin(); it != st.users_.end(); ) {
        if (it->first != st.host_uid_ && it->second.leave_when_config_changed_) {
            sender << At(it->first);
            match_manager().UnbindMatch(it->first);
            it = st.users_.erase(it);
            has_kicked = true;
        } else {
            ++it;
        }
    }
    if (has_kicked) {
        sender << "\n游戏配置已经发生变更，请重新加入游戏";
    } else {
        sender.Release();
    }
}

void Match::Unbind_()
{
    match_manager().UnbindMatch(mid_);
    if (gid_.has_value()) {
        match_manager().UnbindMatch(*gid_);
    }
}
