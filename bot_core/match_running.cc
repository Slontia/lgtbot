// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <ranges>
#include <utility>

#include "utility/log.h"
#include "bot_core/db_manager.h"
#include "bot_core/match_manager.h"
#include "bot_core/match_internal.h"
#include "bot_core/options.h"
#include "match_process/match_ipc.pb.h"

Running::Running(const MatchContext& ctx, MatchMessaging* messaging, MatchHelpServices* help,
        std::map<UserID, MatchParticipantUser>& users, std::vector<MatchPlayer>& players,
        LobbyStartSnapshot snapshot, MatchChildClient* game_child,
        std::optional<MsgSender> group_sender)
        : MatchPhaseCommon(ctx, messaging, users, players)
        , help_(help)
        , group_sender_(std::move(group_sender))
        , host_uid_(snapshot.host_uid_)
        , options_(std::move(snapshot.options_))
        , applied_options_log_(std::move(snapshot.applied_options_log_))
        , game_child_(game_child)
{}

MsgSenderBase* Running::GroupSenderOrNull_()
{
    if (group_sender_.has_value()) {
        return &*group_sender_;
    }
    return nullptr;
}

MatchVariantID Running::HostVariantId_() const { return host_uid_; }

uint32_t Running::ComputerNumImpl_() const
{
    const auto player_num = std::max(static_cast<size_t>(options_.generic_options_.bench_computers_to_player_num_),
            users_.size());
    return static_cast<uint32_t>(player_num - users_.size());
}

const MatchRuntimeOptions& Running::RuntimeOptions_() const { return options_; }

bool Running::IsInDeduction() const { return is_in_deduction_.load(std::memory_order_acquire); }

bool Running::SendStart(const uint64_t match_id, const uint32_t user_num,
        const std::vector<lgtbot::ipc::PlayerInfo>& players)
{
    auto start_fut = game_child_->SendStart(match_id, user_num, players);
    if (!start_fut) {
        return false;
    }
    return std::move(*start_fut).get() == lgtbot::ipc::ResultResp::STAGE_OK;
}

void Running::UpdateDeductionFlag_()
{
    const bool all_players_eliminated = std::ranges::all_of(players_, [](const MatchPlayer& p) {
        return std::get_if<ComputerID>(&p.id_) || p.IsEliminated();
    });
    const bool has_alive_computer = std::ranges::any_of(players_, [](const MatchPlayer& p) {
        return std::get_if<ComputerID>(&p.id_) && !p.IsEliminated();
    });
    is_in_deduction_.store(all_players_eliminated && has_alive_computer, std::memory_order_release);
}

void Running::Eliminate(const PlayerID pid)
{
    auto& player = players_[pid.Get()];
    if (player.ExchangeState(PlayerState::ELIMINATED) != PlayerState::ELIMINATED) {
        MakeTellGuard_(pid) << "很遗憾，您被淘汰了，可以通过「" META_COMMAND_SIGN "退出」以退出游戏";
        UpdateDeductionFlag_();
        LogMatch(InfoLog(), ctx_, host_uid_) << "Eliminate player pid=" << pid
                                        << " is_in_deduction=" << Bool2Str(IsInDeduction());
    }
}

void Running::Hook(const PlayerID pid)
{
    auto& player = players_[pid.Get()];
    if (player.State() == PlayerState::ACTIVE) {
        MakeTellGuard_(pid) << "您已经进入挂机状态，若其他玩家已经行动完成，裁判将不再继续等待您，执行任意游戏请求可恢复至原状态";
        player.SetState(PlayerState::HOOKED);
    }
}

void Running::Activate(const PlayerID pid)
{
    auto& player = players_[pid.Get()];
    if (player.State() == PlayerState::HOOKED) {
        MakeTellGuard_(pid) << "挂机状态已取消";
        player.SetState(PlayerState::ACTIVE);
    }
}

ErrCode Running::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply,
        const std::weak_ptr<const Match>& match_wk)
{
    const auto it = users_.find(uid);
    if (it == users_.end() || !UserIsActive(it->second)) {
        reply() << "[错误] 您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (is_over_.load(std::memory_order_acquire)) {
        LogMatch(WarnLog(), ctx_, host_uid_) << "Match is over but receive request uid=" << uid << " msg=" << msg;
        reply() << "[错误] 游戏已经结束";
        return EC_MATCH_ALREADY_OVER;
    }
    reply.SetMatch(match_wk);
    const PlayerID pid = it->second.pid_;
    const bool is_eliminated = players_[pid.Get()].IsEliminated();
    {
        MsgReader reader(msg);
        if (help_->try_help_command(reader, reply)) {
            return EC_GAME_REQUEST_OK;
        }
    }
    if (is_eliminated) {
        reply() << "[错误] 您已经被淘汰，无法执行游戏请求";
        return EC_MATCH_ELIMINATED;
    }
    auto exec_fut = game_child_->SendExecute(pid, gid.has_value(), msg, reply);
    if (!exec_fut) {
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    ErrCode rc = std::move(*exec_fut).get();
    if (rc == EC_GAME_REQUEST_FAILED && is_over_.load(std::memory_order_acquire)) {
        rc = EC_MATCH_UNEXPECTED_CONFIG;
    }
    if (rc == EC_GAME_REQUEST_NOT_FOUND) {
        reply() << "[错误] 未预料的游戏指令，您可以通过「帮助」（不带" META_COMMAND_SIGN
                   "号）查看所有支持的游戏指令\n"
                   "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN
                   "帮助」查看所有支持的元指令";
    }
    return rc;
}

ErrCode Running::Leave(const UserID uid, MsgSenderBase& reply, const bool force)
{
    const auto it = users_.find(uid);
    if (it == users_.end() || !UserIsActive(it->second)) {
        reply() << "[错误] 退出失败：您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (is_over_.load(std::memory_order_acquire)) {
        reply() << "[错误] 退出失败：游戏已经结束";
        return EC_MATCH_ALREADY_OVER;
    }
    const PlayerID leave_pid = it->second.pid_;
    const bool eliminated = players_[leave_pid.Get()].IsEliminated();
    if (!force && !eliminated) {
        reply() << "[错误] 退出失败：游戏已经开始，若仍要退出游戏，请使用「" META_COMMAND_SIGN "退出 强制」命令";
        return EC_MATCH_ALREADY_BEGIN;
    }
    reply() << "退出成功";
    Boardcast() << "玩家 " << At(uid) << " 中途退出了游戏，他将不再参与后续的游戏进程";
    it->second.presence_.store(UserPresence::LEFT, std::memory_order_release);
    const bool all_left = std::ranges::all_of(users_, [this](const auto& user) {
        return !UserIsActive(user.second);
    });
    if (all_left) {
        Boardcast() << "所有玩家都强制退出了游戏，那还玩啥玩，游戏解散，结果不会被记录";
        LogMatch(InfoLog(), ctx_, host_uid_) << "All users left the game";
        UnbindMatchSide_();
        return EC_OK;
    }
    if (auto leave_fut = game_child_->SendLeave(leave_pid); leave_fut) {
        (void)std::move(*leave_fut).get();
    }
    return EC_OK;
}

ErrCode Running::UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel)
{
    const char* const operation_str = cancel ? "取消中断" : "确定中断";
    const auto it = users_.find(uid);
    if (it == users_.end() || !UserIsActive(it->second)) {
        reply() << "[错误] " << operation_str << "失败：您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (is_over_.load(std::memory_order_acquire)) {
        reply() << "[错误] " << operation_str << "失败：比赛已经结束";
        return EC_MATCH_ALREADY_OVER;
    }
    it->second.want_interrupt_.store(!cancel, std::memory_order_release);
    const auto remain = std::count_if(users_.begin(), users_.end(), [this](const auto& pair) {
        const auto& user = pair.second;
        if (!UserIsActive(user)) {
            return false;
        }
        if (user.want_interrupt_.load(std::memory_order_acquire)) {
            return false;
        }
        return !players_[user.pid_.Get()].IsHooked();
    });
    reply() << operation_str << "成功";
    if (remain == 0) {
        BoardcastAtAll() << "全员支持中断游戏，游戏已中断，谢谢大家参与";
        LogMatch(InfoLog(), ctx_, host_uid_) << "Match is interrupted by users";
        UnbindMatchSide_();
    } else {
        Boardcast() << "有玩家" << operation_str << "比赛，目前 " << remain << " 人尚未确定中断，所有玩家可通过「"
                    << META_COMMAND_SIGN << "中断」命令确定中断比赛，或「" << META_COMMAND_SIGN
                    << "中断 取消」命令取消中断比赛";
    }
    return EC_OK;
}

ErrCode Running::Terminate(const bool is_force)
{
    if (!is_force) {
        return EC_MATCH_ALREADY_BEGIN;
    }
    if (is_over_.load(std::memory_order_acquire)) {
        return EC_OK;
    }
    BoardcastAtAll() << "游戏已解散，谢谢大家参与";
    LogMatch(InfoLog(), ctx_, host_uid_) << "Match is terminated outside";
    UnbindMatchSide_();
    return EC_OK;
}

bool Running::SwitchHost() { return false; }

void Running::ShowInfo(MsgSenderBase& reply, const std::weak_ptr<const Match>& match_wk) const
{
    ShowInfoHeader_(reply, match_wk, "已开始");
    auto sender = reply();
    const auto num = players_.size();
    sender << "\n玩家列表：" << num << "人";
    for (uint32_t pid = 0; pid < num; ++pid) {
        sender << "\n" << pid << "号：" << Name(PlayerID{pid});
    }
}

void Running::FetchHelp(MsgSenderBase& reply, const bool text_mode)
{
    std::string remote;
    HelpTextCollector help_collector(remote);
    auto help_fut = game_child_->FetchHelp(text_mode, help_collector);
    if (help_fut && std::move(*help_fut).get() == lgtbot::ipc::ResultResp::STAGE_OK) {
        std::string outstr = "## 当前可使用的游戏命令\n\n### 查看信息\n1. " +
                             help_->help_command_info(true /* with_example */,
                                     !text_mode /* with_html_color */);
        outstr += "\n";
        outstr += remote;
        if (text_mode) {
            reply() << outstr;
        } else {
            reply() << Markdown(outstr);
        }
        return;
    }
    reply() << "[错误] 无法从游戏进程获取帮助信息";
}

void Running::ApplyChildPost_(const PostFrame& frame)
{
    using Channel = lgtbot::ipc::PostResp::Channel;
    const auto& post = frame.post;
    MsgSenderBase::MsgSenderGuard sender = [&]() -> MsgSenderBase::MsgSenderGuard {
        switch (post.channel()) {
        case Channel::PostResp_Channel_BROADCAST: return Boardcast();
        case Channel::PostResp_Channel_GROUP:     return MakeGroupGuard_();
        case Channel::PostResp_Channel_TELL:      return MakeTellGuard_(PlayerID{post.target_pid()});
        default:                                  return Boardcast();
        }
    }();
    for (const auto& item : post.items()) {
        AppendMsgItem(sender, item);
    }
}

void Running::ApplyChildPlayerState(const PlayerID pid, const std::string& state)
{
    if (pid.Get() >= players_.size()) {
        return;
    }
    auto& player = players_[pid.Get()];
    if (state == "eliminated") {
        if (player.ExchangeState(PlayerState::ELIMINATED) != PlayerState::ELIMINATED) {
            UpdateDeductionFlag_();
            LogMatch(InfoLog(), ctx_, host_uid_) << "Eliminate player pid=" << pid
                                            << " is_in_deduction=" << Bool2Str(IsInDeduction());
        }
    } else if (state == "hooked") {
        player.SetState(PlayerState::HOOKED);
    } else if (state == "active") {
        player.SetState(PlayerState::ACTIVE);
    }
}

void Running::ApplyChildGameOverFromScores(const GameOverFrame& frame)
{
    if (is_over_.load(std::memory_order_acquire)) {
        LogMatch(WarnLog(), ctx_, host_uid_) << "ApplyChildGameOverFromScores but has already been over";
        return;
    }
    const auto& scores = frame.game_over.scores();
    std::vector<std::pair<UserID, int64_t>> user_game_scores;
    std::vector<std::pair<UserID, std::string>> user_achievements;
    {
        auto sender = Boardcast();
        sender << "游戏结束，公布分数：\n";
        for (const auto& row : scores) {
            const auto pid = PlayerID{row.pid()};
            const auto score = static_cast<int64_t>(row.score());
            sender << At(pid) << " " << score << "\n";
            const auto id = players_[pid.Get()].id_;
            if (const auto pval = std::get_if<UserID>(&id); pval) {
                user_game_scores.emplace_back(*pval, score);
                for (const auto& ach_name : row.achievements()) {
                    user_achievements.emplace_back(*pval, ach_name);
                    LogMatch(InfoLog(), ctx_, host_uid_) << "User get achievement uid=" << *pval
                                                    << " achievement=" << ach_name;
                }
            }
        }
        sender << "感谢诸位参与！";

        assert(user_game_scores.size() == users_.size());
        std::sort(user_game_scores.begin(), user_game_scores.end(),
                [](const auto& _1, const auto& _2) { return _1.second > _2.second; });

        static const auto show_score = [](const char* const name, const auto sc) {
            return std::string("[") + name + (sc > 0 ? "+" : "") + std::to_string(sc) + "] ";
        };
        if (user_game_scores.size() <= 1) {
            sender << "\n\n游戏结果不记录：因为玩家数小于 2";
#ifndef WITH_SQLITE
        } else {
            sender << "\n\n游戏结果不记录：因为未连接数据库";
        }
#else
        } else if (!ctx_.bot.db_manager()) {
            sender << "\n\n游戏结果不记录：因为未连接数据库";
        } else if (const auto multiple = ctx_.Multiple(); !options_.generic_options_.is_formal_ || multiple == 0) {
            sender << "\n\n游戏结果不记录：因为该游戏为非正式游戏";
        } else if (const auto score_info = ctx_.bot.db_manager()->RecordMatch(ctx_.game_handle.Info().name_, ctx_.gid, host_uid_,
                               multiple, user_game_scores, user_achievements);
                score_info.empty()) {
            sender << "\n\n[错误] 游戏结果写入数据库失败，请联系管理员";
            LogMatch(ErrorLog(), ctx_, host_uid_) << "Save database failed";
        } else {
            assert(score_info.size() == users_.size());
            sender << "\n\n游戏结果写入数据库成功：";
            for (const auto& info : score_info) {
                sender << "\n" << At(info.uid_) << "：" << show_score("零和", info.zero_sum_score_)
                       << show_score("头名", info.top_score_) << show_score("等级", info.level_score_);
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
    is_over_.store(true, std::memory_order_release);
    ctx_.game_handle.IncreaseActivity(users_.size());
    LogMatch(InfoLog(), ctx_, host_uid_) << "Match is over normally";
    UnbindMatchSide_();
}

void Running::ApplyChildIpcFromReadThread(const PushFrame& frame)
{
    std::visit([this](const auto& f) {
        using T = std::decay_t<decltype(f)>;
        if constexpr (std::is_same_v<T, PostFrame>) {
            ApplyChildPost_(f);
        } else if constexpr (std::is_same_v<T, PlayerStateFrame>) {
            ApplyChildPlayerState(f.pid, f.state);
        } else if constexpr (std::is_same_v<T, GameOverFrame>) {
            ApplyChildGameOverFromScores(f);
        }
    }, frame);
}

void Running::ApplyChildEofFromReadThread(const bool unexpected)
{
    if (!unexpected) {
        return;
    }
    if (is_over_.load(std::memory_order_acquire)) {
        return;
    }
    BoardcastAtAll() << "[错误] 游戏进程意外终止，游戏已中断";
    LogMatch(ErrorLog(), ctx_, host_uid_) << "Game subprocess died unexpectedly, terminating match";
    is_over_.store(true, std::memory_order_release);
    UnbindMatchSide_();
}

void Running::UnbindMatchSide_()
{
    is_over_.store(true, std::memory_order_release);
}

void Running::ReleaseGameChildIfOver() {}
