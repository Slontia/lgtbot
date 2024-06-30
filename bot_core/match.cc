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

#include "utility/msg_checker.h"
#include "utility/log.h"
#include "utility/empty_func.h"
#include "game_framework/game_main.h"
#include "bot_core/db_manager.h"
#include "bot_core/match.h"
#include "bot_core/match_manager.h"
#include "bot_core/score_calculation.h"
#include "bot_core/options.h"
#include "nlohmann/json.hpp"

Match::Match(BotCtx& bot, const MatchID mid, GameHandle& game_handle, GameHandle::Options options,
        const UserID host_uid, const std::optional<GroupID> gid)
        : bot_(bot)
        , mid_(mid)
        , game_handle_(game_handle)
        , host_uid_(host_uid)
        , gid_(gid)
        , options_{
            .game_options_ = std::move(options.game_options_),
            .generic_options_{
                lgtbot::game::ImmutableGenericOptions{},
                lgtbot::game::MutableGenericOptions{std::move(options.generic_options_)}
            },
          }
        , group_sender_(gid.has_value() ? std::optional<MsgSender>(bot.MakeMsgSender(*gid_, this)) : std::nullopt)
{
    EmplaceUser_(host_uid);
}

bool Match::Has_(const UserID uid) const { return users_.find(uid) != users_.end(); }

std::string Match::HostUserName_() const
{
    return bot_.GetUserName(host_uid_.GetCStr(), gid_.has_value() ? gid_->GetCStr() : nullptr);
}

uint32_t Match::ComputerNum_() const
{
    const auto bench_computers_to_player_num = options_.generic_options_.bench_computers_to_player_num_;
    if (bench_computers_to_player_num <= users_.size()) {
        return 0;
    }
    return bench_computers_to_player_num - users_.size();
}

void Match::EmplaceUser_(const UserID uid)
{
    const auto& ai_list = GET_OPTION_VALUE(*bot_.option().Lock(), AI列表);
    users_.emplace(uid, ParticipantUser(*this, uid, std::ranges::find(ai_list, uid.GetStr()) != std::end(ai_list)));
}

Match::VariantID Match::ConvertPid(const PlayerID pid) const
{
    if (!pid.IsValid()) {
        return host_uid_; // TODO: UINT64_MAX for host
    }
    return players_[pid].id_;
}

ErrCode Match::SetBenchTo(const UserID uid, MsgSenderBase& reply, const uint64_t bench_computers_to_player_num)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_();
        return EC_MATCH_NOT_HOST;
    }
    auto sender = reply();
    if (bench_computers_to_player_num <= users_.size()) {
        sender << "[警告] 当前玩家数 " << users_.size() << " 已满足条件";
        return EC_OK;
    }
    if (const auto max_player = MaxPlayerNum_(); max_player != 0 && bench_computers_to_player_num > max_player) {
        sender << "[错误] 设置失败：比赛人数将超过上限" << max_player << "人";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    options_.generic_options_.bench_computers_to_player_num_ = bench_computers_to_player_num;
    KickForConfigChange_();
    sender << "设置成功！\n\n" << BriefInfo_();
    return EC_OK;
}

ErrCode Match::SetFormal(const UserID uid, MsgSenderBase& reply, const bool is_formal)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_();
        return EC_MATCH_NOT_HOST;
    }
    const auto multiple = Multiple_();
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

static ErrCode ConverErrCode(const StageErrCode& stage_errcode)
{
    switch (stage_errcode) {
    case StageErrCode::OK: return EC_GAME_REQUEST_OK;
    case StageErrCode::CHECKOUT: return EC_GAME_REQUEST_CHECKOUT;
    case StageErrCode::FAILED: return EC_GAME_REQUEST_FAILED;
    case StageErrCode::CONTINUE: return EC_GAME_REQUEST_CONTINUE;
    case StageErrCode::NOT_FOUND: return EC_GAME_REQUEST_NOT_FOUND;
    default: return EC_GAME_REQUEST_UNKNOWN;
    }
}

ErrCode Match::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg,
                       MsgSender& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    const auto it = users_.find(uid);
    if (it == users_.end() || it->second.state_ == ParticipantUser::State::LEFT) {
        reply() << "[错误] 您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (state_ == State::IS_OVER) {
        MatchLog_(WarnLog()) << "Match is over but receive request uid=" << uid << " msg=" << msg;
        reply() << "[错误] 游戏已经结束";
        return EC_MATCH_ALREADY_OVER;
    }
    reply.SetMatch(this);
    if (MsgReader reader(msg); help_cmd_.CallIfValid(reader, reply)) {
        return EC_GAME_REQUEST_OK;
    }
    if (main_stage_) {
        const auto pid = it->second.pid_;
        assert(state_ == State::IS_STARTED);
        if (players_[pid].state_ == Player::State::ELIMINATED) {
            reply() << "[错误] 您已经被淘汰，无法执行游戏请求";
            return EC_MATCH_ELIMINATED;
        }
        const auto stage_rc = main_stage_->HandleRequest(msg.c_str(), pid, gid.has_value(), reply);
        if (stage_rc == StageErrCode::NOT_FOUND) {
            reply() << "[错误] 未预料的游戏指令，您可以通过「帮助」（不带" META_COMMAND_SIGN "号）查看所有支持的游戏指令\n"
                        "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN "帮助」查看所有支持的元指令";
        }
        Routine_();
        return ConverErrCode(stage_rc);
    }
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_();
        return EC_MATCH_NOT_HOST;
    }
    if (!options_.game_options_->SetOption(msg.c_str())) {
        reply() << "[错误] 未预料的游戏设置，您可以通过「帮助」（不带" META_COMMAND_SIGN "号）查看所有支持的游戏设置\n"
                    "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN "帮助」查看所有支持的元指令";
        return EC_GAME_REQUEST_NOT_FOUND;
    }
    KickForConfigChange_();
    reply() << "设置成功！目前配置：" << OptionInfo_() << "\n\n" << BriefInfo_();
    return EC_GAME_REQUEST_OK;
}

ErrCode Match::GameStart(const UserID uid, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (state_ != State::NOT_STARTED) {
        reply() << "[错误] 开始失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    if (uid != host_uid_) {
        reply() << "[错误] 开始失败：您并非房主，没有开始游戏的权限，房主是" << HostUserName_();
        return EC_MATCH_NOT_HOST;
    }
    nlohmann::json players_json_array;
    players_.clear();
    for (auto& [uid, user_info] : users_) {
        user_info.pid_ = players_.size();
        players_.emplace_back(uid);
        players_json_array.push_back(nlohmann::json{
                    { "user_id", uid.GetStr() }
                });
        user_info.sender_.SetMatch(this);
    }
    for (ComputerID cid = 0; cid < ComputerNum_(); ++cid) {
        players_.emplace_back(cid);
        players_json_array.push_back(nlohmann::json{
                    { "computer_id", static_cast<uint64_t>(cid) }
                });
    }
    options_.resource_holder_.resource_dir_ =
        (std::filesystem::absolute(bot_.game_path()) / game_handle_.Info().module_name_ / "resource" / "").string();
    options_.resource_holder_.saved_image_dir_ =
        (std::filesystem::absolute(bot_.image_path()) / "matches" /
         (std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "_" + game_handle_.Info().module_name_)).string();
    options_.generic_options_.resource_dir_ = options_.resource_holder_.resource_dir_.c_str();
    options_.generic_options_.saved_image_dir_ = options_.resource_holder_.saved_image_dir_.c_str();
    options_.generic_options_.public_timer_alert_ = GET_OPTION_VALUE(*bot_.option().Lock(), 计时公开提示);
    options_.generic_options_.user_num_ = static_cast<uint32_t>(users_.size());
    assert(main_stage_ == nullptr);
    if (!(main_stage_ = game_handle_.MakeMainStage(reply, *options_.game_options_, options_.generic_options_, *this))) {
        reply() << "[错误] 开始失败：不符合游戏参数的预期";
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    state_ = State::IS_STARTED;
    BoardcastAtAll() << "游戏开始，您可以使用「帮助」命令（不带" META_COMMAND_SIGN "号），查看可执行命令";
    BoardcastAiInfo() << nlohmann::json{
            { "match_id", MatchId() },
            { "state", "started" },
            { "players", std::move(players_json_array) },
        }.dump();
    main_stage_->HandleStageBegin();
    Routine_(); // computer act first
    return EC_OK;
}

ErrCode Match::Join(const UserID uid, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (state_ != State::NOT_STARTED) {
        reply() << "[错误] 加入失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    if (const auto max_player = MaxPlayerNum_(); max_player != 0 && users_.size() >= max_player) {
        reply() << "[错误] 加入失败：比赛人数已达到游戏上限";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    if (Has_(uid)) {
        reply() << "[错误] 加入失败：您已加入该游戏";
        return EC_MATCH_USER_ALREADY_IN_MATCH;
    }
    if (!match_manager().BindMatch(uid, shared_from_this())) {
        reply() << "[错误] 加入失败：您已加入其他游戏，您可通过私信裁判「" META_COMMAND_SIGN "游戏信息」查看该游戏信息";
        return EC_MATCH_USER_ALREADY_IN_OTHER_MATCH;
    }
    EmplaceUser_(uid);
    Boardcast() << "玩家 " << At(uid) << " 加入了游戏\n\n" << BriefInfo_();
    return EC_OK;
}

ErrCode Match::Leave(const UserID uid, MsgSenderBase& reply, const bool force)
{
    ErrCode rc = EC_OK;
    std::lock_guard<std::mutex> l(mutex_);
    const auto it = users_.find(uid);
    if (it == users_.end() || it->second.state_ == ParticipantUser::State::LEFT) {
        reply() << "[错误] 退出失败：您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (state_ == State::IS_OVER) {
        reply() << "[错误] 退出失败：游戏已经结束";
        rc = EC_MATCH_ALREADY_OVER;
    } else if (state_ != State::IS_STARTED) {
        match_manager().UnbindMatch(uid);
        users_.erase(uid);
        reply() << "退出成功";
        Boardcast() << "玩家 " << At(uid) << " 退出了游戏\n\n" << BriefInfo_();
        if (users_.empty()) {
            Boardcast() << "所有玩家都退出了游戏，游戏解散";
            Unbind_();
        } else if (uid == host_uid_) {
            host_uid_ = users_.begin()->first;
            Boardcast() << At(host_uid_) << "被选为新房主";
        }
    } else if (force || players_[it->second.pid_].state_ == Player::State::ELIMINATED) {
        match_manager().UnbindMatch(uid);
        reply() << "退出成功";
        Boardcast() << "玩家 " << At(uid) << " 中途退出了游戏，他将不再参与后续的游戏进程";
        assert(main_stage_);
        assert(it->second.state_ != ParticipantUser::State::LEFT);
        it->second.state_ = ParticipantUser::State::LEFT;
        if (std::ranges::all_of(users_, [](const auto& user) { return user.second.state_ == ParticipantUser::State::LEFT; })) {
            Boardcast() << "所有玩家都强制退出了游戏，那还玩啥玩，游戏解散，结果不会被记录";
            MatchLog_(InfoLog()) << "All users left the game";
            Terminate_();
        } else {
            main_stage_->HandleLeave(it->second.pid_);
            Routine_();
        }
    } else {
        reply() << "[错误] 退出失败：游戏已经开始，若仍要退出游戏，请使用「" META_COMMAND_SIGN "退出 强制」命令";
        rc = EC_MATCH_ALREADY_BEGIN;
    }
    return rc;
}

MsgSenderBase& Match::BoardcastMsgSender()
{
    if (group_sender_.has_value()) {
        return *group_sender_;
    } else {
        return boardcast_private_sender_;
    }
}

MsgSenderBase& Match::BoardcastAiInfoMsgSender()
{
    if (!group_sender_.has_value()) {
        return boardcast_ai_info_private_sender_;
    } else if (std::ranges::any_of(users_, [](const auto& user) { return user.second.is_ai_; })) {
        return *group_sender_;
    } else {
        return EmptyMsgSender::Get();
    }
}

MsgSenderBase& Match::TellMsgSender(const PlayerID pid)
{
    const auto& id = ConvertPid(pid);
    if (const auto pval = std::get_if<UserID>(&id); !pval) {
        return EmptyMsgSender::Get(); // is computer
    } else if (const auto it = users_.find(*pval); it != users_.end() && it->second.state_ != ParticipantUser::State::LEFT) {
        return it->second.sender_;
    } else {
        return EmptyMsgSender::Get(); // player exit
    }
}

MsgSenderBase& Match::GroupMsgSender()
{
    if (group_sender_.has_value()) {
        return *group_sender_;
    } else {
        return EmptyMsgSender::Get();
    }
}

const char* Match::PlayerName(const PlayerID& pid)
{
    thread_local static std::string str;
    const auto& id = ConvertPid(pid);
    if (const auto pval = std::get_if<ComputerID>(&id)) {
        return (str = "机器人" + std::to_string(*pval) + "号").c_str();
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
    if (gid().has_value()) {
        auto sender = Boardcast();
        for (auto& [uid, user_info] : users_) {
            if (user_info.state_ != ParticipantUser::State::LEFT) {
                sender << At(uid);
            }
        }
        sender << "\n";
        return sender;
    } else {
        return BoardcastMsgSender()();
    }
}

bool Match::SwitchHost()
{
    if (users_.empty()) {
        MatchLog_(InfoLog()) << "SwitchHost but no users left";
        return false;
    }
    if (state_ == NOT_STARTED) {
        host_uid_ = users_.begin()->first;
        Boardcast() << At(host_uid_) << "被选为新房主";
        MatchLog_(InfoLog()) << "SwitchHost succeed";
    }
    return true;
}

// REQUIRE: should be protected by mutex_
void Match::TimerController::Start(Match& match, const uint64_t sec, void* alert_arg,
        void(*alert_cb)(void*, uint64_t))
{
    static const uint64_t kMinAlertSec = 10;
    if (sec == 0) {
        return;
    }
    Stop(match);
    timer_is_over_ = std::make_shared<bool>(false);

    // We must store a timer_is_over because it may be reset to true when new timer begins.
    const auto timeout_handler = [this, timer_is_over = timer_is_over_, match_wk = match.weak_from_this()](const uint64_t /*sec*/)
        {
            // Handle a reference because match may be removed from match_manager if timeout cause game over.
            auto match = match_wk.lock();
            if (!match) {
                return; // match is released
            }
#ifdef TEST_BOT
            {
                std::lock_guard<std::mutex> l(before_handle_timeout_mutex_);
                before_handle_timeout_ = true;
            }
            before_handle_timeout_cv_.notify_all();

#endif
            // Timeout event should not be triggered during request handling, so we need lock here.
            // timer_is_over also should protected in lock. Otherwise, a rquest may be handled after checking timer_is_over and before timeout_timer lock match.
            std::lock_guard<std::mutex> l(match->mutex_);

#ifdef TEST_BOT
            {
                std::lock_guard<std::mutex> l(before_handle_timeout_mutex_);
                before_handle_timeout_ = false;
            }
#endif
            // If stage has been finished by other request, timeout event should not be triggered again, so we check stage_is_over_ here.
            // Should NOT use this->timer_is_over_ here which may be belong to a new timer.
            if (!*timer_is_over) {
                match->MatchLog_(DebugLog()) << "Timer timeout";
                match->main_stage_->HandleTimeout();
                match->Routine_();
            } else {
                match->MatchLog_(WarnLog()) << "Timer timeout but timer has been already over";
            }
        };

    const auto alert_handler =
        [alert_cb, alert_arg, timer_is_over = timer_is_over_, match_wk = match.weak_from_this()](const uint64_t alert_sec)
        {
            auto match = match_wk.lock();
            if (!match) {
                match->MatchLog_(WarnLog()) << "Timer alert but match is released sec=" << alert_sec;
                return; // match is released
            }
            std::lock_guard<std::mutex> l(match->mutex_);
            if (!*timer_is_over) {
                match->MatchLog_(DebugLog()) << "Timer alert sec=" << alert_sec;
                alert_cb(alert_arg, alert_sec);
            } else {
                match->MatchLog_(WarnLog()) << "Timer alert but timer has been already over sec=" << alert_sec;
            }
        };

    Timer::TaskSet timeup_tasks;
    if (kMinAlertSec > sec / 2) {
        // time is short, we need not alert
        timeup_tasks.emplace_front(sec, timeout_handler);
    } else {
        timeup_tasks.emplace_front(kMinAlertSec, timeout_handler);
        uint64_t sum_alert_sec = kMinAlertSec;
        for (uint64_t alert_sec = kMinAlertSec; sum_alert_sec < sec / 2; sum_alert_sec += alert_sec, alert_sec *= 2) {
            timeup_tasks.emplace_front(alert_sec, alert_handler);
        }
        timeup_tasks.emplace_front(sec - sum_alert_sec, g_empty_func);
    }
    timer_ = std::make_unique<Timer>(std::move(timeup_tasks)); // start timer
}

// This function can be invoked event if timer is not started.
// REQUIRE: should be protected by mutex_
// RETURN: the remain milliseconds. The valud of zero indicates that the timer has already been over.
void Match::TimerController::Stop(const Match& match)
{
    assert(!((timer_is_over_ == nullptr) ^ (timer_ == nullptr)));
    if (timer_is_over_ == nullptr) {
        match.MatchLog_(WarnLog()) << "Timer stop but timer has been already over";
        return;
    }
    *timer_is_over_ = true;
    timer_is_over_ = nullptr;
    timer_ = nullptr;
    match.MatchLog_(DebugLog()) << "Timer stop";
}

void Match::StartTimer(const uint64_t sec, void* alert_arg, void(*alert_cb)(void*, uint64_t))
{
    return timer_cntl_.Start(*this, sec, alert_arg, alert_cb);
}

void Match::StopTimer() { return timer_cntl_.Stop(*this); }

void Match::Eliminate(const PlayerID pid)
{
    if (std::exchange(players_[pid].state_, Player::State::ELIMINATED) != Player::State::ELIMINATED) {
        Tell(pid) << "很遗憾，您被淘汰了，可以通过「" META_COMMAND_SIGN "退出」以退出游戏";
        is_in_deduction_ = std::ranges::all_of(players_,
                [](const auto& p) { return std::get_if<ComputerID>(&p.id_) || p.state_ == Player::State::ELIMINATED; });
        MatchLog_(InfoLog()) << "Eliminate player pid=" << pid << " is_in_deduction=" << Bool2Str(is_in_deduction_);
    }
}

void Match::Hook(const PlayerID pid)
{
    auto& player = players_[pid];
    if (player.state_ == Player::State::ACTIVE) {
        Tell(pid) << "您已经进入挂机状态，若其他玩家已经行动完成，裁判将不再继续等待您，执行任意游戏请求可恢复至原状态";
        player.state_ = Player::State::HOOKED;
    }
}

void Match::Activate(const PlayerID pid)
{
    auto& player = players_[pid];
    if (player.state_ == Player::State::HOOKED) {
        // TODO: check all players of the user
        Tell(pid) << "挂机状态已取消";
        player.state_ = Player::State::ACTIVE;
    }
}

void Match::ShowInfo(MsgSenderBase& reply) const
{
    reply.SetMatch(this);
    std::lock_guard<std::mutex> l(mutex_);
    auto sender = reply();
    sender << "游戏名称：" << game_handle().Info().name_ << "\n";
    sender << "配置信息：" << OptionInfo_() << "\n";
    sender << "电脑数量：" << ComputerNum_() << "\n";
    sender << "游戏状态："
           << (state() == Match::State::NOT_STARTED ? "未开始" : "已开始")
           << "\n";
    sender << "房间号：";
    if (gid_.has_value()) {
        sender << *gid_ << "\n";
    } else {
        sender << "私密游戏" << "\n";
    }
    sender << "最多可参加人数：";
    if (const auto max_player = MaxPlayerNum_(); max_player == 0) {
        sender << "无限制";
    } else {
        sender << max_player;
    }
    sender << "人\n房主：" << Name(host_uid_);
    if (state() == Match::State::IS_STARTED) {
        const auto num = players_.size();
        sender << "\n玩家列表：" << num << "人";
        for (uint32_t pid = 0; pid < num; ++pid) {
            sender << "\n" << pid << "号：" << Name(PlayerID{pid});
        }
    } else {
        sender << "\n当前报名玩家：" << users_.size() << "人";
        for (const auto& [uid, _] : users_) {
            sender << "\n" << Name(uid);
        }
    }
}

std::string Match::BriefInfo() const
{
    std::lock_guard l(mutex_);
    return BriefInfo_();
}

std::string Match::BriefInfo_() const
{
    const auto multiple = Multiple_();
    return "游戏名称：" + game_handle().Info().name_ +
        "\n- 倍率：" +
        (options_.generic_options_.is_formal_ || multiple == 0 ? std::to_string(multiple) :
                                                                 "0（开启计分后为 " + std::to_string(multiple) + "）") +
        "\n- 当前用户数：" + std::to_string(users_.size()) +
        "\n- 当前电脑数：" + std::to_string(ComputerNum_());
}

std::string Match::OptionInfo_() const
{
    std::string result;
    const char* const* option_content = options_.game_options_->ShortInfo();
    while (*option_content) {
        result += "\n- ";
        result += *option_content;
        ++option_content;
    }
    return result;
}

void Match::OnGameOver_()
{
    if (state_ == State::IS_OVER) {
        MatchLog_(WarnLog()) << "OnGameOver_ but has already been over";
        return;
    }
    std::vector<std::pair<UserID, int64_t>> user_game_scores;
    std::vector<std::pair<UserID, std::string>> user_achievements;
    {
        auto sender = Boardcast();
        sender << "游戏结束，公布分数：\n";
        for (PlayerID pid = 0; pid < static_cast<uint32_t>(players_.size()); ++pid) {
            const auto score = main_stage_->PlayerScore(pid);
            sender << At(pid) << " " << score << "\n";
            const auto id = ConvertPid(pid);
            if (const auto pval = std::get_if<UserID>(&id); pval) {
                user_game_scores.emplace_back(*pval, score);
                const char* const* const achievements = main_stage_->VerdictateAchievements(pid);
                for (const char* const* achievement_name_p = achievements; *achievement_name_p; ++achievement_name_p) {
                    user_achievements.emplace_back(*pval, *achievement_name_p);
                    MatchLog_(InfoLog()) << "User get achievement uid=" << *pval << " achievement=" << *achievement_name_p;
                }
            }
        }
        sender << "感谢诸位参与！";

        assert(user_game_scores.size() == users_.size());
        std::sort(user_game_scores.begin(), user_game_scores.end(),
                [](const auto& _1, const auto& _2) { return _1.second > _2.second; });

        static const auto show_score = [](const char* const name, const auto score)
            {
                return std::string("[") + name + (score > 0 ? "+" : "") + std::to_string(score) + "] ";
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
        } else if (const auto multiple = Multiple_(); !options_.generic_options_.is_formal_ || multiple == 0) {
            sender << "\n\n游戏结果不记录：因为该游戏为非正式游戏";
        } else if (const auto score_info =
                    bot_.db_manager()->RecordMatch(game_handle_.Info().name_, gid_, host_uid_,
                        multiple, user_game_scores, user_achievements);
                score_info.empty()) {
            sender << "\n\n[错误] 游戏结果写入数据库失败，请联系管理员";
            MatchLog_(ErrorLog()) << "Save database failed";
        } else {
            assert(score_info.size() == users_.size());
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
    state_ = State::IS_OVER; // is necessary because other thread may own a match reference
    game_handle_.IncreaseActivity(users_.size());
    MatchLog_(InfoLog()) << "Match is over normally";
    Terminate_();
}

void Match::Help_(MsgSenderBase& reply, const bool text_mode)
{
    std::string outstr;
    if (main_stage_) {
        outstr += "## 阶段信息\n\n";
        outstr += main_stage_->StageInfoC();
        outstr += "\n\n";
    }
    outstr += "## 当前可使用的游戏命令";
    outstr += "\n\n### 查看信息";
    outstr += "\n1. " + help_cmd_.Info(true /* with_example */, !text_mode /* with_html_color */);
    if (main_stage_) {
        outstr += main_stage_->CommandInfoC(text_mode);
    } else {
        outstr += "\n\n ### 配置选项";
        outstr += options_.game_options_->Info(true, !text_mode);
    }
    if (text_mode) {
        reply() << outstr;
    } else {
        reply() << Markdown(outstr);
    }
}

void Match::Routine_()
{
    if (main_stage_->IsOver()) {
        OnGameOver_();
        return;
    }
    const uint64_t computer_num = players_.size() - users_.size();
    uint64_t ok_count = 0;
    for (uint64_t i = 0; !main_stage_->IsOver() && ok_count < computer_num; i = (i + 1) % computer_num) {
        const auto pid = users_.size() + i;
        if (players_[pid].state_ == Player::State::ELIMINATED || StageErrCode::OK == main_stage_->HandleComputerAct(pid, false)) {
            ++ok_count;
        } else {
            ok_count = 0;
        }
    }
    if (main_stage_->IsOver()) {
        OnGameOver_();
    }
}

ErrCode Match::UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel)
{
    const std::lock_guard<std::mutex> l(mutex_);
    const auto it = users_.find(uid);
    const char* const operation_str = cancel ? "取消中断" : "确定中断";
    if (it == users_.end() && it->second.state_ == ParticipantUser::State::LEFT) {
        reply() << "[错误] " << operation_str << "失败：您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    if (state_ == State::NOT_STARTED) {
        reply() << "[错误] " << operation_str << "失败：比赛尚未开始";
        return EC_MATCH_NOT_BEGIN;
    }
    if (state_ == State::IS_OVER) {
        reply() << "[错误] " << operation_str << "失败：比赛已经结束";
        return EC_MATCH_ALREADY_OVER;
    }
    it->second.want_interrupt_ = !cancel;
    const auto remain = std::count_if(users_.begin(), users_.end(), [this](const auto& pair)
            {
                const auto& user = pair.second;
                return !(user.want_interrupt_ ||
                         user.state_ == ParticipantUser::State::LEFT ||
                         players_[user.pid_].state_ == Player::State::HOOKED);

            });
    reply() << operation_str << "成功";
    if (remain == 0) {
        BoardcastAtAll() << "全员支持中断游戏，游戏已中断，谢谢大家参与";
        MatchLog_(InfoLog()) << "Match is interrupted by users";
        Terminate_();
    } else {
        Boardcast() << "有玩家" << operation_str << "比赛，目前 " << remain << " 人尚未确定中断，所有玩家可通过「" META_COMMAND_SIGN "中断」命令确定中断比赛，或「" META_COMMAND_SIGN "中断 取消」命令取消中断比赛";
    }

    return EC_OK;
}

ErrCode Match::Terminate(const bool is_force)
{
    const std::lock_guard<std::mutex> l(mutex_);
    if (is_force || state_ == State::NOT_STARTED) {
        BoardcastAtAll() << "游戏已解散，谢谢大家参与";
        MatchLog_(InfoLog()) << "Match is terminated outside";
        Terminate_();
        return EC_OK;
    } else {
        return EC_MATCH_ALREADY_BEGIN;
    }
}

void Match::Terminate_()
{
    for (auto& [uid, user_info] : users_) {
        if (user_info.state_ != ParticipantUser::State::LEFT) {
            match_manager().UnbindMatch(uid);
        }
    }
    Unbind_();
    BoardcastAiInfo() << nlohmann::json{
            { "match_id", MatchId() },
            { "state", "finished" },
        }.dump();
}

void Match::KickForConfigChange_()
{
    auto sender = Boardcast();
    bool has_kicked = false;
    for (auto it = users_.begin(); it != users_.end(); ) {
        if (it->first != host_uid_ && it->second.leave_when_config_changed_) {
            sender << At(it->first);
            match_manager().UnbindMatch(it->first);
            it = users_.erase(it);
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
