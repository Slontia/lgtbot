// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match/match.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <ranges>
#include <utility>

#include "utility/log.h"
#include "utility/overloaded.h"
#include "bot_core/db_manager.h"
#include "bot_core/match/match_manager.h"
#include "bot_core/options.h"
#include "match_process/match_ipc.pb.h"
#include "utility/empty_func.h"

namespace lgtbot {
namespace core {
namespace match {

namespace {

constexpr uint64_t kMinGameTimerAlertSec = 10;

Timer::TaskSet BuildGameTimerTasks(const uint64_t sec, std::function<void(uint64_t)> alert_handler,
        std::function<void()> timeout_handler)
{
    Timer::TaskSet tasks;
    if (sec == 0) {
        return tasks;
    }
    if (kMinGameTimerAlertSec > sec / 2) {
        tasks.emplace_front(sec, [timeout_handler = std::move(timeout_handler)](const uint64_t /*sec*/) {
            timeout_handler();
        });
        return tasks;
    }
    tasks.emplace_front(kMinGameTimerAlertSec, [timeout_handler = std::move(timeout_handler)](const uint64_t /*sec*/) {
        timeout_handler();
    });
    uint64_t sum_alert_sec = kMinGameTimerAlertSec;
    for (uint64_t alert_sec = kMinGameTimerAlertSec; sum_alert_sec < sec / 2; sum_alert_sec += alert_sec, alert_sec *= 2) {
        tasks.emplace_front(alert_sec, [alert_handler, alert_sec](const uint64_t /*sec*/) { alert_handler(alert_sec); });
    }
    tasks.emplace_front(sec - sum_alert_sec, g_empty_func);
    return tasks;
}

} // namespace

MatchRunning::MatchRunning(const MatchContext& ctx, MatchMessaging* messaging, MatchHelpServices* help,
        std::map<UserID, MatchParticipantUser>& users,
        const UserID host_uid, MatchRuntimeOptions options,
        std::vector<MatchVariantID> player_ids,
        MatchChildClient* game_child,
        std::optional<MsgSender> group_sender,
        MatchTimerFactory timer_factory)
        : MatchPhaseBase(ctx, messaging, users)
        , help_(help)
        , group_sender_(std::move(group_sender))
        , host_uid_(host_uid)
        , options_(std::move(options))
        , game_child_(game_child)
        , timer_factory_(std::move(timer_factory))
{
    players_.reserve(player_ids.size());
    for (PlayerID pid = 0; pid.Get() < player_ids.size(); ++pid) {
        auto& id = player_ids[pid.Get()];
        if (const auto* uid = std::get_if<UserID>(&id)) {
            const auto it = users_.find(*uid);
            assert(it != users_.end());
            it->second.pid_ = pid;
        }
        players_.emplace_back(std::move(id));
    }
    options_.generic_options_.user_num_ = static_cast<uint32_t>(users_.size());
}

MatchVariantID MatchRunning::ConvertPid(const PlayerID pid) const
{
    if (!pid.IsValid()) {
        return HostVariantId_();
    }
    return players_[pid.Get()].id_;
}

HostMsgSenderBase& MatchRunning::TellMsgSender(const PlayerID pid)
{
    const auto& id = ConvertPid(pid);
    const auto pval = std::get_if<UserID>(&id);
    if (!pval) {
        return HostEmptyMsgSender::Get();
    }
    if (const auto it = users_.find(*pval); it != users_.end() && UserIsActive(it->second)) {
        return it->second.sender_;
    }
    return HostEmptyMsgSender::Get();
}

const char* MatchRunning::PlayerName(const PlayerID& pid)
{
    thread_local static std::string str;
    const auto& id = ConvertPid(pid);
    if (const auto pval = std::get_if<ComputerID>(&id)) {
        return (str = "机器人" + std::to_string(pval->Get()) + "号").c_str();
    }
    return (str = ctx_.bot.GetUserName(std::get<UserID>(id).GetCStr(),
                              ctx_.gid.has_value() ? ctx_.gid->GetCStr() : nullptr))
            .c_str();
}

const char* MatchRunning::PlayerAvatar(const PlayerID& pid, const int32_t size)
{
    thread_local static std::string str;
    const auto& id = ConvertPid(pid);
    if (std::get_if<ComputerID>(&id)) {
        return "";
    }
    return (str = ctx_.bot.GetUserAvatar(std::get<UserID>(id).GetCStr(), size)).c_str();
}

HostMsgSenderBase* MatchRunning::GroupSenderOrNull_()
{
    if (group_sender_.has_value()) {
        return &*group_sender_;
    }
    return nullptr;
}

MatchVariantID MatchRunning::HostVariantId_() const { return host_uid_; }

uint32_t MatchRunning::ComputerNumImpl_() const
{
    const auto player_num = std::max(static_cast<size_t>(options_.generic_options_.bench_computers_to_player_num_),
            users_.size());
    return static_cast<uint32_t>(player_num - users_.size());
}

const MatchRuntimeOptions& MatchRunning::RuntimeOptions_() const { return options_; }

bool MatchRunning::IsInDeduction() const { return is_in_deduction_.load(std::memory_order_acquire); }

void MatchRunning::UpdateDeductionFlag_()
{
    const bool all_players_eliminated = std::ranges::all_of(players_, [](const MatchPlayer& p) {
        return std::get_if<ComputerID>(&p.id_) || p.IsEliminated();
    });
    const bool has_alive_computer = std::ranges::any_of(players_, [](const MatchPlayer& p) {
        return std::get_if<ComputerID>(&p.id_) && !p.IsEliminated();
    });
    is_in_deduction_.store(all_players_eliminated && has_alive_computer, std::memory_order_release);
}

void MatchRunning::Eliminate(const PlayerID pid)
{
    auto& player = players_[pid.Get()];
    if (player.ExchangeState(PlayerState::ELIMINATED) != PlayerState::ELIMINATED) {
        TellMsgSender(pid)() << "很遗憾，您被淘汰了，可以通过「" META_COMMAND_SIGN "退出」以退出游戏";
        UpdateDeductionFlag_();
        LogMatch(InfoLog(), ctx_, host_uid_) << "Eliminate player pid=" << pid
                                        << " is_in_deduction=" << Bool2Str(IsInDeduction());
    }
}

void MatchRunning::Hook(const PlayerID pid)
{
    auto& player = players_[pid.Get()];
    if (player.State() == PlayerState::ACTIVE) {
        TellMsgSender(pid)() << "您已经进入挂机状态，若其他玩家已经行动完成，裁判将不再继续等待您，执行任意游戏请求可恢复至原状态";
        player.SetState(PlayerState::HOOKED);
    }
}

void MatchRunning::Activate(const PlayerID pid)
{
    auto& player = players_[pid.Get()];
    if (player.State() == PlayerState::HOOKED) {
        TellMsgSender(pid)() << "挂机状态已取消";
        player.SetState(PlayerState::ACTIVE);
    }
}

ErrCode MatchRunning::ExecuteRequest(const UserID uid, const std::optional<GroupID> gid, const std::string& msg,
        MsgSender& reply, const PushHandler& on_push)
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
    const PlayerID pid = it->second.pid_;
    const bool is_eliminated = players_[pid.Get()].IsEliminated();
    // The in-game help command is routed by Internal::ExecuteRequest before phase
    // dispatch; routing it here would re-enter Match's data_ lock and deadlock.
    if (is_eliminated) {
        reply() << "[错误] 您已经被淘汰，无法执行游戏请求";
        return EC_MATCH_ELIMINATED;
    }
    if (!game_child_) {
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    ChildMessageReplyHandler handler(reply, this);
    const auto result = game_child_->SendExecute(pid, gid.has_value(), msg, handler, on_push);
    if (!result) {
        HandleChildEof();
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    ErrCode out = *result;
    if (out == EC_GAME_REQUEST_FAILED && is_over_.load(std::memory_order_acquire)) {
        out = EC_MATCH_UNEXPECTED_CONFIG;
    }
    return out;
}

ErrCode MatchRunning::LeaveBeforeChild(const UserID uid, HostMsgSenderBase& reply, const bool force,
        const PushHandler& on_push)
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
    if (game_child_) {
        if (!game_child_->SendLeave(leave_pid, on_push)) {
            HandleChildEof();
        }
    }
    return EC_OK;
}

ErrCode MatchRunning::UserInterrupt(const UserID uid, HostMsgSenderBase& reply, const bool cancel)
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

ErrCode MatchRunning::Terminate(const bool is_force)
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

bool MatchRunning::SwitchHost() { return false; }

void MatchRunning::ShowInfo(HostMsgSenderBase& reply) const
{
    ShowInfoHeader_(reply, "已开始");
    auto sender = reply();
    const auto num = players_.size();
    sender << "\n玩家列表：" << num << "人";
    for (uint32_t pid = 0; pid < num; ++pid) {
        sender << "\n" << pid << "号：";
        const auto& id = players_[pid].id_;
        if (const auto pval = std::get_if<ComputerID>(&id)) {
            sender << "机器人" << pval->Get() << "号";
        } else {
            sender << Name(std::get<UserID>(id));
        }
    }
}

void MatchRunning::FetchHelp(HostMsgSenderBase& reply, const bool text_mode)
{
    if (!game_child_) {
        reply() << "[错误] 无法从游戏进程获取帮助信息";
        return;
    }
    std::string remote;
    const auto stage = game_child_->FetchHelp(text_mode, remote);
    if (!stage || *stage != lgtbot::ipc::ResultResp::STAGE_OK) {
        reply() << "[错误] 无法从游戏进程获取帮助信息";
        return;
    }
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
}

void MatchRunning::ApplyChildPushFrame(const PushFrame& frame)
{
    std::visit(overloaded{
        [this](std::reference_wrapper<const lgtbot::ipc::PostResp> ref)
            { ApplyChildPost_(ref.get()); },
        [this](std::reference_wrapper<const lgtbot::ipc::PlayerStateResp> ref)
            { ApplyChildPlayerState(PlayerID{ref.get().pid()}, ref.get().state()); },
        [this](std::reference_wrapper<const lgtbot::ipc::GameOverResp> ref)
            { ApplyChildGameOverFromScores(ref.get()); },
        [this](std::reference_wrapper<const lgtbot::ipc::TimerStartResp> ref)
            { HandleTimerStart(ref.get().duration_sec()); },
        [this](std::reference_wrapper<const lgtbot::ipc::TimerStopResp>)
            { HandleTimerStop(); },
    }, frame);
}

void MatchRunning::HandleTimerStart(const uint64_t duration_sec)
{
    timer_factory_.Start(duration_sec);
}

void MatchRunning::HandleTimerStop()
{
    timer_factory_.Stop();
}

void MatchTimerFactory::Start(const uint64_t duration_sec)
{
    timer_is_over_ = std::make_shared<std::atomic<bool>>(false);
    auto timeout_handler = [match_ptr = &match_, tio = timer_is_over_]() {
        if (tio->load(std::memory_order_acquire)) {
            return;
        }
        match_ptr->HandleGameTimeout_(tio);
    };
    auto alert_handler = [match_ptr = &match_, tio = timer_is_over_](const uint64_t remaining_sec) {
        if (tio->load(std::memory_order_acquire)) {
            return;
        }
        match_ptr->data_.lock()->ExecuteAlert(remaining_sec, tio, *match_ptr);
    };
    Retire_(std::move(game_timer_));
    game_timer_ = std::make_unique<Timer>(
        BuildGameTimerTasks(duration_sec, std::move(alert_handler), std::move(timeout_handler)));
}

void MatchTimerFactory::Stop()
{
    if (timer_is_over_) {
        timer_is_over_->store(true, std::memory_order_release);
        timer_is_over_ = nullptr;
    }
    Retire_(std::move(game_timer_));
}

// The timer loop thread may intern a TimerStart that calls Start() again (game
// re-arming its timer from within the timeout handler). Replacing game_timer_ in
// place would destroy the running Timer on its own thread and self-join. Retire
// instead: request stop here, and leave the jthread to join on destruction.
void MatchTimerFactory::Retire_(std::unique_ptr<Timer>&& timer)
{
    if (!timer) {
        return;
    }
    timer->RequestStop();
    retired_.push_back(std::move(timer));
}

void MatchRunning::ApplyChildPost_(const lgtbot::ipc::PostResp& post)
{
    using Channel = lgtbot::ipc::PostResp::Channel;
    HostMsgSenderBase& base_sender = [&]() -> HostMsgSenderBase& {
        switch (post.channel()) {
        case Channel::PostResp_Channel_BROADCAST: return BoardcastMsgSender();
        case Channel::PostResp_Channel_GROUP:     return GroupMsgSender();
        case Channel::PostResp_Channel_TELL:      return TellMsgSender(PlayerID{post.target_pid()});
        default:                                  return BoardcastMsgSender();
        }
    }();
    ChildMessageReplyHandler handler(base_sender, this);
    for (const auto& item : post.items()) {
        AppendMsgItem(handler, item);
    }
}

void MatchRunning::ApplyChildPlayerState(const PlayerID pid, const std::string& state)
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

void MatchRunning::ApplyChildGameOverFromScores(const lgtbot::ipc::GameOverResp& game_over)
{
    if (is_over_.load(std::memory_order_acquire)) {
        LogMatch(WarnLog(), ctx_, host_uid_) << "ApplyChildGameOverFromScores but has already been over";
        return;
    }
    const auto& scores = game_over.scores();
    std::vector<std::pair<UserID, int64_t>> user_game_scores;
    std::vector<std::pair<UserID, std::string>> user_achievements;
    {
        auto sender = Boardcast();
        sender << "游戏结束，公布分数：\n";
        for (const auto& row : scores) {
            const auto pid = PlayerID{row.pid()};
            const auto score = static_cast<int64_t>(row.score());
            const auto id = players_[pid.Get()].id_;
            sender << "[" << pid.Get() << "号：";
            if (const auto cval = std::get_if<ComputerID>(&id)) {
                sender << "机器人" << cval->Get() << "号";
            } else {
                sender << At(std::get<UserID>(id));
            }
            sender << "] " << score << "\n";
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

void MatchRunning::HandleChildEof()
{
    BoardcastAtAll() << "[错误] 游戏进程意外终止，游戏已中断";
    LogMatch(ErrorLog(), ctx_, host_uid_) << "Game subprocess died unexpectedly, terminating match";
    is_over_.store(true, std::memory_order_release);
    UnbindMatchSide_();
}

void MatchRunning::UnbindMatchSide_()
{
    is_over_.store(true, std::memory_order_release);
}

} // namespace match
} // namespace core
} // namespace lgtbot
