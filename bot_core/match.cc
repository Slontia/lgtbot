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
#include "game_framework/game_main.h"
#include "bot_core/db_manager.h"
#include "bot_core/match.h"
#include "bot_core/match_manager.h"
#include "bot_core/score_calculation.h"
#include "bot_core/options.h"
#include "nlohmann/json.hpp"

Match::Match(BotCtx& bot, const MatchID mid, GameHandle& game_handle, const UserID host_uid,
             const std::optional<GroupID> gid)
        : bot_(bot)
        , mid_(mid)
        , game_handle_(game_handle)
        , host_uid_(host_uid)
        , gid_(gid)
        , state_(State::NOT_STARTED)
        , options_(game_handle.make_game_options())
        , main_stage_(nullptr, [](const lgtbot::game::MainStageBase*) {}) // make when game starts
        , player_num_each_user_(1)
        , users_()
        , boardcast_private_sender_(MsgSenderBatchHandler(*this, false))
        , boardcast_ai_info_private_sender_(MsgSenderBatchHandler(*this, true))
        , group_sender_(gid.has_value() ? std::optional<MsgSender>(bot.MakeMsgSender(*gid_, this)) : std::nullopt)
        , bench_to_player_num_(0)
        , multiple_(game_handle.multiple_)
        , help_cmd_(Command<void(MsgSenderBase&)>("查看游戏帮助", std::bind_front(&Match::Help_, this), VoidChecker("帮助"), OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")))
#ifdef TEST_BOT
        , before_handle_timeout_(false)
#endif
        , is_in_deduction_(false)
{
    EmplaceUser_(host_uid);
}

Match::~Match() {}

bool Match::Has_(const UserID uid) const { return users_.find(uid) != users_.end(); }

std::string Match::HostUserName_() const
{
    return bot_.GetUserName(host_uid_.GetCStr(), gid_.has_value() ? gid_->GetCStr() : nullptr);
}

uint64_t Match::ComputerNum_() const
{
    return std::max(int64_t(0), static_cast<int64_t>(bench_to_player_num_ - user_controlled_player_num()));
}

void Match::EmplaceUser_(const UserID uid)
{
    const auto locked_option = bot_.option().Lock();
    const auto& ai_list = GET_OPTION_VALUE(locked_option.Get(), AI列表);
    users_.emplace(uid, ParticipantUser(*this, uid, std::ranges::find(ai_list, uid.GetStr()) != std::end(ai_list)));
}

Match::VariantID Match::ConvertPid(const PlayerID pid) const
{
    if (!pid.IsValid()) {
        return host_uid_; // TODO: UINT64_MAX for host
    }
    return players_[pid].id_;
}

ErrCode Match::SetBenchTo(const UserID uid, MsgSenderBase& reply, std::optional<uint64_t> bench_to_player_num)
{
    if (!bench_to_player_num.has_value()) {
        bench_to_player_num.emplace(options_->BestPlayerNum());
    }
    std::lock_guard<std::mutex> l(mutex_);
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_();
        return EC_MATCH_NOT_HOST;
    }
    auto sender = reply();
    if (*bench_to_player_num <= user_controlled_player_num()) {
        sender << "[警告] 当前玩家数 " << user_controlled_player_num() << " 已满足条件";
        return EC_OK;
    } else if (game_handle_.max_player_ != 0 && *bench_to_player_num > game_handle_.max_player_) {
        sender << "[错误] 设置失败：比赛人数将超过上限" << game_handle_.max_player_ << "人";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    bench_to_player_num_ = *bench_to_player_num;
    KickForConfigChange_();
    sender << "设置成功！\n\n" << BriefInfo_();
    return EC_OK;
}

ErrCode Match::CheckMultipleAllowed_(const UserID uid, MsgSenderBase& reply, const uint32_t multiple) const
{
    if (!bot_.db_manager()) {
        return EC_OK; // always allow when database is not connected
    }
    const auto required_zero_sum_score = k_zero_sum_score_multi * multiple * 2;
    const auto required_top_score = k_top_score_multi * multiple * 2;
    const auto profit = bot_.db_manager()->GetUserProfile(uid, "", ""); // get history total score
    if (multiple <= game_handle_.multiple_ || multiple == 1) {
        return EC_OK;
    } else if (required_zero_sum_score > profit.total_zero_sum_score_ || required_top_score > profit.total_top_score_) {
        reply() << "[错误] 您的分数未达到要求，零和总分至少需要达到 "
                << required_zero_sum_score << "（当前 " << profit.total_zero_sum_score_ << "），"
                << "头名总分至少需要达到 "
                << required_top_score << "（当前 " << profit.total_top_score_ << "）";
        return EC_MATCH_SCORE_NOT_ENOUGH;
    } else if (profit.recent_matches_.size() < multiple ||
            std::any_of(profit.recent_matches_.begin(), profit.recent_matches_.begin() + multiple,
                [](const auto& match) { return match.multiple_ != 1; })) {
        reply() << "[错误] 您近期需连续完成 " << multiple << " 场单倍率的比赛";
        return EC_MATCH_SINGLE_MULTT_MATCH_NOT_ENOUGH;
    }
    return EC_OK;
}

ErrCode Match::SetMultiple(const UserID uid, MsgSenderBase& reply, const uint32_t multiple)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_();
        return EC_MATCH_NOT_HOST;
    }
    if (multiple == multiple_) {
        reply() << "[警告] 赔率 " << multiple << " 和目前配置相同";
        return EC_OK;
    }
    if (const auto ret = CheckMultipleAllowed_(uid, reply, multiple); ret != EC_OK) {
        return ret;
    }
    multiple_ = multiple;
    KickForConfigChange_();
    if (multiple == 0) {
        reply() << "设置成功！当前游戏为试玩游戏";
    } else {
        reply() << "设置成功！当前倍率为 " << multiple;
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
        MatchLog(WarnLog()) << "Match is over but receive request uid=" << uid << " msg=" << msg;
        reply() << "[错误] 游戏已经结束";
        return EC_MATCH_ALREADY_OVER;
    }
    reply.SetMatch(this);
    if (MsgReader reader(msg); help_cmd_.CallIfValid(reader, reply)) {
        return EC_GAME_REQUEST_OK;
    }
    if (main_stage_) {
        // TODO: check player_num_each_user_
        const auto pid = it->second.pids_[0];
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
    if (!options_->SetOption(msg.c_str())) {
        reply() << "[错误] 未预料的游戏设置，您可以通过「帮助」（不带" META_COMMAND_SIGN "号）查看所有支持的游戏设置\n"
                    "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN "帮助」查看所有支持的元指令";
        return EC_GAME_REQUEST_NOT_FOUND;
    }
    KickForConfigChange_();
    reply() << "设置成功！目前配置：" << OptionInfo_() << "\n\n" << BriefInfo_();
    return EC_GAME_REQUEST_OK;
}

ErrCode Match::GameStart(const UserID uid, const bool is_public, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (state_ != State::NOT_STARTED) {
        reply() << "[错误] 开始失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_();
        return EC_MATCH_NOT_HOST;
    }
    nlohmann::json players_json_array;
    players_.clear();
    for (auto& [uid, user_info] : users_) {
        for (int i = 0; i < player_num_each_user_; ++i) {
            user_info.pids_.emplace_back(players_.size());
            players_.emplace_back(uid);
            players_json_array.push_back(nlohmann::json{
                        { "user_id", uid.GetStr() }
                    });
        }
        user_info.sender_.SetMatch(this);
    }
    for (ComputerID cid = 0; cid < ComputerNum_(); ++cid) {
        players_.emplace_back(cid);
        players_json_array.push_back(nlohmann::json{
                    { "computer_id", static_cast<uint64_t>(cid) }
                });
    }
    const uint64_t player_num = std::max(user_controlled_player_num(), bench_to_player_num_);
    const std::string resource_dir = (std::filesystem::absolute(bot_.game_path()) / game_handle_.module_name_ / "").string();
    const std::string saved_image_dir =
        (std::filesystem::absolute(bot_.image_path()) / "matches" /
         (std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "_" + game_handle_.module_name_)).string();
    assert(main_stage_ == nullptr);
    assert(game_handle_.max_player_ == 0 || player_num <= game_handle_.max_player_);
    options_->SetPlayerNum(player_num);
    options_->SetResourceDir(resource_dir.c_str());
    options_->SetSavedImageDir(saved_image_dir.c_str());
    options_->global_options_.public_timer_alert_ = GET_OPTION_VALUE(bot_.option().Lock().Get(), 计时公开提示);
    if (!(main_stage_ = game_handle_.make_main_stage(reply, *options_, *this))) {
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
    if (users_.size() == game_handle_.max_player_) {
        reply() << "[错误] 加入失败：比赛人数已达到游戏上限";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    if (Has_(uid)) {
        reply() << "[错误] 加入失败：您已加入该游戏";
        return EC_MATCH_USER_ALREADY_IN_MATCH;
    }
    if (const auto ret = CheckMultipleAllowed_(uid, reply, multiple_); ret != EC_OK) {
        return ret;
    }
    if (!match_manager().BindMatch(uid, shared_from_this())) {
        reply() << "[错误] 加入失败：您已加入其他游戏，您可通过私信裁判「" META_COMMAND_SIGN "游戏信息」查看该游戏信息";
        return EC_MATCH_USER_ALREADY_IN_OTHER_MATCH;
    }
    EmplaceUser_(uid);
    Boardcast() << "玩家 " << At(uid) << " 加入了游戏\n\n" << BriefInfo_();
    return EC_OK;
}

template <typename Fn>
bool Match::AllControlledPlayerState_(const ParticipantUser& user, Fn&& fn) const
{
    return std::ranges::all_of(user.pids_, [this, &fn](const auto pid) { return fn(players_[pid].state_); });
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
    } else if (force || AllControlledPlayerState_(it->second, [](const auto s) { return s == Player::State::ELIMINATED; })) {
        match_manager().UnbindMatch(uid);
        reply() << "退出成功";
        Boardcast() << "玩家 " << At(uid) << " 中途退出了游戏，他将不再参与后续的游戏进程";
        assert(main_stage_);
        assert(it->second.state_ != ParticipantUser::State::LEFT);
        it->second.state_ = ParticipantUser::State::LEFT;
        if (std::ranges::all_of(users_, [](const auto& user) { return user.second.state_ == ParticipantUser::State::LEFT; })) {
            Boardcast() << "所有玩家都强制退出了游戏，那还玩啥玩，游戏解散，结果不会被记录";
            MatchLog(InfoLog()) << "All users left the game";
            Terminate_();
        } else {
            for (const auto pid : it->second.pids_) {
                main_stage_->HandleLeave(pid);
                Routine_();
            }
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
        MatchLog(InfoLog()) << "SwitchHost but no users left";
        return false;
    }
    if (state_ == NOT_STARTED) {
        host_uid_ = users_.begin()->first;
        Boardcast() << At(host_uid_) << "被选为新房主";
        MatchLog(InfoLog()) << "SwitchHost succeed";
    }
    return true;
}

// REQUIRE: should be protected by mutex_
void Match::StartTimer(const uint64_t sec, void* p, void(*cb)(void*, uint64_t))
{
    static const uint64_t kMinAlertSec = 10;
    if (sec == 0) {
        return;
    }
    StopTimer();
    timer_is_over_ = std::make_shared<bool>(false);
    // We must store a timer_is_over because it may be reset to true when new timer begins.
    const auto timeout_handler = [this, timer_is_over = timer_is_over_, match_wk = weak_from_this()]()
        {
            // Handle a reference because match may be removed from match_manager if timeout cause game over.
            auto match = match_wk.lock();
            if (!match) {
                MatchLog(WarnLog()) << "Timer timeout but match has been already released";
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
                MatchLog(DebugLog()) << "Timer timeout";
                match->main_stage_->HandleTimeout();
                match->Routine_();
            } else {
                MatchLog(WarnLog()) << "Timer timeout but timer has been already over";
            }
        };
    Timer::TaskSet tasks;
    if (kMinAlertSec > sec / 2) {
        tasks.emplace_front(sec, timeout_handler);
    } else {
        tasks.emplace_front(kMinAlertSec, timeout_handler);
        uint64_t sum_alert_sec = kMinAlertSec;
        for (uint64_t alert_sec = kMinAlertSec; sum_alert_sec < sec / 2; sum_alert_sec += alert_sec, alert_sec *= 2) {
            tasks.emplace_front(alert_sec, [cb, p, alert_sec, timer_is_over = timer_is_over_, match_wk = weak_from_this()]()
                    {
                        auto match = match_wk.lock();
                        if (!match) {
                            match->MatchLog(WarnLog()) << "Timer alert but match is released sec=" << alert_sec;
                            return; // match is released
                        }
                        std::lock_guard<std::mutex> l(match->mutex_);
                        if (!*timer_is_over) {
                            match->MatchLog(DebugLog()) << "Timer alert sec=" << alert_sec;
                            cb(p, alert_sec);
                        } else {
                            match->MatchLog(WarnLog()) << "Timer alert but timer has been already over sec=" << alert_sec;
                        }
                    });
        }
        tasks.emplace_front(sec - sum_alert_sec, [] {});
    }
    timer_ = std::make_unique<Timer>(std::move(tasks)); // start timer
}

// This function can be invoked event if timer is not started.
// REQUIRE: should be protected by mutex_
void Match::StopTimer()
{
    if (timer_is_over_ != nullptr) {
        MatchLog(DebugLog()) << "Timer stop";
        *timer_is_over_ = true;
    } else {
        MatchLog(WarnLog()) << "Timer stop but timer has been already over";
    }
    timer_ = nullptr; // stop timer
}

void Match::Eliminate(const PlayerID pid)
{
    if (std::exchange(players_[pid].state_, Player::State::ELIMINATED) != Player::State::ELIMINATED) {
        // TODO: check all players of the user
        Tell(pid) << "很遗憾，您被淘汰了，可以通过「" META_COMMAND_SIGN "退出」以退出游戏";
        is_in_deduction_ = std::ranges::all_of(players_,
                [](const auto& p) { return std::get_if<ComputerID>(&p.id_) || p.state_ == Player::State::ELIMINATED; });
        MatchLog(InfoLog()) << "Eliminate player pid=" << pid << " is_in_deduction=" << Bool2Str(is_in_deduction_);
    }
}

void Match::Hook(const PlayerID pid)
{
    auto& player = players_[pid];
    if (player.state_ == Player::State::ACTIVE) {
        // TODO: check all players of the user
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
    sender << "游戏名称：" << game_handle().name_ << "\n";
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
    if (game_handle().max_player_ == 0) {
        sender << "无限制";
    } else {
        sender << game_handle().max_player_;
    }
    sender << "人\n房主：" << Name(host_uid_);
    if (state() == Match::State::IS_STARTED) {
        const auto num = players_.size();
        sender << "\n玩家列表：" << num << "人";
        for (uint64_t pid = 0; pid < num; ++pid) {
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
    return "游戏名称：" + game_handle().name_ +
        "\n- 倍率：" + std::to_string(multiple_) +
        "\n- 当前用户数：" + std::to_string(user_controlled_player_num()) +
        "\n- 当前电脑数：" + std::to_string(ComputerNum_());
}

std::string Match::OptionInfo_() const
{
    std::string result = options_->Status();
    const char* const* option_content = options_->Content();
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
        MatchLog(WarnLog()) << "OnGameOver_ but has already been over";
        return;
    }
    std::vector<std::pair<UserID, int64_t>> user_game_scores;
    std::vector<std::pair<UserID, std::string>> user_achievements;
    {
        auto sender = Boardcast();
        sender << "游戏结束，公布分数：\n";
        for (PlayerID pid = 0; pid < players_.size(); ++pid) {
            const auto score = main_stage_->PlayerScore(pid);
            sender << At(pid) << " " << score << "\n";
            const auto id = ConvertPid(pid);
            if (const auto pval = std::get_if<UserID>(&id); pval) {
                user_game_scores.emplace_back(*pval, score);
                const char* const* const achievements = main_stage_->VerdictateAchievements(pid);
                for (const char* const* achievement_name_p = achievements; *achievement_name_p; ++achievement_name_p) {
                    user_achievements.emplace_back(*pval, *achievement_name_p);
                    MatchLog(InfoLog()) << "User get achievement uid=" << *pval << " achievement=" << *achievement_name_p;
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
        } else if (!bot_.db_manager()) {
            sender << "\n\n游戏结果不记录：因为未连接数据库";
        } else if (multiple_ == 0) {
            sender << "\n\n游戏结果不记录：因为该游戏为试玩游戏";
        } else if (const auto score_info =
                    bot_.db_manager()->RecordMatch(game_handle_.name_, gid_, host_uid_, multiple_, user_game_scores,
                        user_achievements);
                score_info.empty()) {
            sender << "\n\n[错误] 游戏结果写入数据库失败，请联系管理员";
            MatchLog(ErrorLog()) << "Save database failed";
        } else {
            assert(score_info.size() == users_.size());
            sender << "\n\n游戏结果写入数据库成功：";
            for (const auto& info : score_info) {
                sender << "\n" << At(info.uid_) << "：" << show_score("零和", info.zero_sum_score_)
                                                        << show_score("头名", info.top_score_)
                                                        << show_score("等级", info.level_score_);

            }
        }
    }
    if (!user_achievements.empty()) {
        auto sender = Boardcast();
        sender << "有用户获得新成就：";
        for (const auto& [user_id, achievement_name] : user_achievements) {
            sender << "\n" << At(user_id) << "：" << achievement_name;
        }
    }
    state_ = State::IS_OVER; // is necessary because other thread may own a match reference
    game_handle_.activity_ += users_.size();
    MatchLog(InfoLog()) << "Match is over normally";
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
        outstr += options_->Info(true, !text_mode);
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
    const uint64_t user_controlled_num = users_.size() * player_num_each_user_;
    const uint64_t computer_num = players_.size() - user_controlled_num;
    uint64_t ok_count = 0;
    for (uint64_t i = 0; !main_stage_->IsOver() && ok_count < computer_num; i = (i + 1) % computer_num) {
        const auto pid = user_controlled_num + i;
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
    if (it == users_.end() && it->second.state_ == ParticipantUser::State::LEFT) {
        reply() << "[错误] 中断失败：您未处于游戏中或已经离开";
        return EC_MATCH_USER_NOT_IN_MATCH;
    }
    it->second.want_interrupt_ = !cancel;
    const auto remain = std::count_if(users_.begin(), users_.end(), [this](const auto& pair)
            {
                const auto& user = pair.second;
                return !(user.want_interrupt_ ||
                         user.state_ == ParticipantUser::State::LEFT ||
                         AllControlledPlayerState_(user, [](const auto s) { return s != Player::State::ACTIVE; }));

            });
    reply() << "中断成功";
    if (remain == 0) {
        BoardcastAtAll() << "全员支持中断游戏，游戏已中断，谢谢大家参与";
        MatchLog(InfoLog()) << "Match is interrupted by users";
        Terminate_();
    } else {
        Boardcast() << "有玩家" << (cancel ? "取消" : "确定") << "中断游戏，目前 " << remain << " 人尚未确定中止，所有玩家可通过「" META_COMMAND_SIGN "中断」命令确定中断游戏，或「" META_COMMAND_SIGN "中断 取消」命令取消中断游戏";
    }

    return EC_OK;
}

ErrCode Match::Terminate(const bool is_force)
{
    const std::lock_guard<std::mutex> l(mutex_);
    if (is_force || state_ == State::NOT_STARTED) {
        BoardcastAtAll() << "游戏已解散，谢谢大家参与";
        MatchLog(InfoLog()) << "Match is terminated outside";
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
