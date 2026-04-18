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
        , host_uid_(host_uid)
        , gid_(gid)
        , applied_options_log_(std::move(init_options.applied_options_log_))
        , group_sender_(gid.has_value() ? std::optional<MsgSender>(bot.MakeMsgSender(*gid_, this)) : std::nullopt)
{
    options_.resource_holder_.resource_dir_ =
        (std::filesystem::absolute(bot_.game_path()) / game_handle_.Info().module_name_ / "resource" / "").string();
    options_.resource_holder_.saved_image_dir_ =
        (std::filesystem::absolute(bot_.image_path()) / "matches" /
         (std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "_" +
          game_handle_.Info().module_name_)).string();
    options_.generic_options_ = lgtbot::game::GenericOptions{
        lgtbot::game::ImmutableGenericOptions{
            .public_timer_alert_ = GET_OPTION_VALUE(*bot_.option().Lock(), 计时公开提示),
            .resource_dir_ = options_.resource_holder_.resource_dir_.c_str(),
            .saved_image_dir_ = options_.resource_holder_.saved_image_dir_.c_str(),
        },
        lgtbot::game::MutableGenericOptions{
            .bench_computers_to_player_num_ = init_options.bench_computers_to_player_num_,
            .is_formal_ = static_cast<bool>(init_options.is_formal_),
        }
    };
    EmplaceUser_(host_uid);
}

Match::~Match()
{
    // Normally read_thread_ has already been moved into the cleanup queue by UnbindMatchSide_.
    // However, if BotCtx is force-destroyed while a game is in progress (e.g. test teardown),
    // we must stop and join it here.
    if (game_child_) {
        game_child_->SignalStop();
    }
    if (read_thread_.joinable()) {
        read_thread_.join();
    }
}

bool Match::Has_(const UserID uid) const { return users_.find(uid) != users_.end(); }

std::string Match::HostUserName_() const
{
    return bot_.GetUserName(host_uid_.GetCStr(), gid_.has_value() ? gid_->GetCStr() : nullptr);
}

uint32_t Match::PlayerNum_() const
{
    return std::max(static_cast<size_t>(options_.generic_options_.bench_computers_to_player_num_), users_.size());
}

uint32_t Match::ComputerNum_() const
{
    return PlayerNum_() - users_.size();
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
    { std::string brief; BriefInfo_(brief); sender << "设置成功！\n\n" << brief; }
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

ErrCode Match::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg,
                       MsgSender& reply)
{
    std::unique_lock<std::mutex> l(mutex_);
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
    if (state_ == State::IS_STARTED) {
        const auto pid = it->second.pid_;
        const bool is_eliminated = players_[pid].state_ == Player::State::ELIMINATED;
        // Release mutex before any IPC or blocking call (read thread may callback into Match).
        l.unlock();
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
        if (!game_child_) {
            return EC_MATCH_ALREADY_OVER;
        }
        const ErrCode rc = game_child_->SendExecute(pid, gid.has_value(), msg,
            [&reply](const lgtbot::ipc::ReplyResp& resp) {
                auto g = reply();
                for (const auto& item : resp.items()) {
                    AppendMsgItem(g, item);
                }
            });
        if (rc == EC_GAME_REQUEST_NOT_FOUND) {
            reply() << "[错误] 未预料的游戏指令，您可以通过「帮助」（不带" META_COMMAND_SIGN "号）查看所有支持的游戏指令\n"
                        "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN "帮助」查看所有支持的元指令";
        }
        return rc;
    }
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限，房主是" << HostUserName_();
        return EC_MATCH_NOT_HOST;
    }
    uint64_t max_player = 0;
    uint32_t multiple = 0;
    if (!game_handle_.ConfigClient().SetDefaultOption(msg, max_player, multiple)) {
        reply() << "[错误] 未预料的游戏设置，您可以通过「帮助」（不带" META_COMMAND_SIGN "号）查看所有支持的游戏设置\n"
                    "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN "帮助」查看所有支持的元指令";
        return EC_GAME_REQUEST_NOT_FOUND;
    }
    game_handle_.UpdateCachedLimits(max_player, multiple);
    applied_options_log_.push_back(msg);
    KickForConfigChange_();
    { std::string brief; BriefInfo_(brief); reply() << "设置成功！\n\n" << brief; }
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

    // fill players
    players_.clear();
    for (auto& [uid, user_info] : users_) {
        players_.emplace_back(uid);
        user_info.sender_.SetMatch(this);
    }
    for (ComputerID cid = 0; cid < ComputerNum_(); ++cid) {
        players_.emplace_back(cid);
    }
    if (game_handle_.Info().shuffled_player_id_) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(players_.begin(), players_.end(), g);
    }
    for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
        const auto user_id = std::get_if<UserID>(&players_[pid].id_);
        if (!user_id) {
            continue;
        }
        const auto it = users_.find(*user_id);
        assert(it != users_.end());
        it->second.pid_ = pid;
    }
    options_.generic_options_.user_num_ = static_cast<uint32_t>(users_.size());

    game_child_ = std::make_unique<MatchChildClient>(ResolveRunnerExe(), GameLibraryPath(bot_, game_handle_));
    if (!game_child_->ok()) {
        reply() << "[错误] 开始失败：无法启动游戏子进程";
        game_child_.reset();
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    if (!game_child_->SendInit(options_.resource_holder_.resource_dir_, options_.resource_holder_.saved_image_dir_,
                GET_OPTION_VALUE(*bot_.option().Lock(), 计时公开提示), options_.generic_options_.bench_computers_to_player_num_,
                options_.generic_options_.is_formal_)) {
        reply() << "[错误] 开始失败：子进程初始化失败";
        game_child_.reset();
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    for (const auto& line : applied_options_log_) {
        if (!game_child_->SendSetOption(line)) {
            reply() << "[错误] 开始失败：无法同步游戏设置到子进程";
            game_child_.reset();
            return EC_MATCH_UNEXPECTED_CONFIG;
        }
    }
    std::vector<lgtbot::ipc::PlayerInfo> players_for_child;
    players_for_child.reserve(players_.size());
    for (const auto& pl : players_) {
        lgtbot::ipc::PlayerInfo pi;
        if (const auto* const cid = std::get_if<ComputerID>(&pl.id_)) {
            pi.set_computer(true);
            pi.set_computer_id(cid->Get());
        } else {
            const auto& uid = std::get<UserID>(pl.id_);
            pi.set_computer(false);
            pi.set_display_name(bot_.GetUserName(uid.GetCStr(), gid_.has_value() ? gid_->GetCStr() : nullptr));
            pi.set_avatar(bot_.GetUserAvatar(uid.GetCStr(), 0));
        }
        players_for_child.push_back(std::move(pi));
    }
    {
        std::vector<PushFrame> push_frames;
        if (!game_child_->SendStart(MatchId(), static_cast<uint32_t>(users_.size()),
                                    players_for_child, push_frames)) {
            reply() << "[错误] 开始失败：不符合游戏参数的预期";
            game_child_.reset();
            return EC_MATCH_UNEXPECTED_CONFIG;
        }
        // Dispatch push frames collected during start (we still hold mutex here).
        for (const auto& pf : push_frames) {
            std::visit([this](const auto& f) {
                using T = std::decay_t<decltype(f)>;
                if constexpr (std::is_same_v<T, PostFrame>) {
                    ApplyChildPost_(f);
                } else if constexpr (std::is_same_v<T, PlayerStateFrame>) {
                    ApplyChildPlayerState(f.pid, f.state);
                } else if constexpr (std::is_same_v<T, GameOverFrame>) {
                    ApplyChildGameOverFromScores(f);
                }
            }, pf);
        }
    }

    if (state_ == State::IS_OVER) {
        // Game ended immediately during start (e.g. single-user mode with instant finish)
        game_child_.reset();
        return EC_OK;
    }
    state_ = State::IS_STARTED;
    StartReadThread_();
    BoardcastAtAll() << "游戏开始，您可以使用「帮助」命令（不带" META_COMMAND_SIGN "号），查看可执行命令";
    {
        nlohmann::json players_json_array = nlohmann::json::array();
        for (const auto& pi : players_for_child) {
            if (pi.computer()) {
                players_json_array.push_back(nlohmann::json{{"computer_id", pi.computer_id()}});
            } else {
                players_json_array.push_back(nlohmann::json{{"display_name", pi.display_name()}});
            }
        }
        BoardcastAiInfo() << nlohmann::json{
                { "match_id", MatchId() },
                { "state", "started" },
                { "players", std::move(players_json_array) },
            }.dump();
    }

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
    { std::string brief; BriefInfo_(brief); Boardcast() << "玩家 " << At(uid) << " 加入了游戏\n\n" << brief; }
    return EC_OK;
}

ErrCode Match::Leave(const UserID uid, MsgSenderBase& reply, const bool force)
{
    PlayerID leave_pid;
    std::unique_ptr<MatchChildClient> terminate_child;
    bool send_leave_ipc = false;
    {
        std::lock_guard<std::mutex> l(mutex_);
        const auto it = users_.find(uid);
        if (it == users_.end() || it->second.state_ == ParticipantUser::State::LEFT) {
            reply() << "[错误] 退出失败：您未处于游戏中或已经离开";
            return EC_MATCH_USER_NOT_IN_MATCH;
        }
        if (state_ == State::IS_OVER) {
            reply() << "[错误] 退出失败：游戏已经结束";
            return EC_MATCH_ALREADY_OVER;
        } else if (state_ != State::IS_STARTED) {
            match_manager().UnbindMatch(uid);
            users_.erase(uid);
            reply() << "退出成功";
            { std::string brief; BriefInfo_(brief); Boardcast() << "玩家 " << At(uid) << " 退出了游戏\n\n" << brief; }
            if (users_.empty()) {
                Boardcast() << "所有玩家都退出了游戏，游戏解散";
                Unbind_();
            } else if (uid == host_uid_) {
                host_uid_ = users_.begin()->first;
                Boardcast() << At(host_uid_) << "被选为新房主";
            }
            return EC_OK;
        } else if (force || players_[it->second.pid_].state_ == Player::State::ELIMINATED) {
            match_manager().UnbindMatch(uid);
            reply() << "退出成功";
            Boardcast() << "玩家 " << At(uid) << " 中途退出了游戏，他将不再参与后续的游戏进程";
            assert(game_child_);
            assert(it->second.state_ != ParticipantUser::State::LEFT);
            it->second.state_ = ParticipantUser::State::LEFT;
            if (std::ranges::all_of(users_, [](const auto& user) { return user.second.state_ == ParticipantUser::State::LEFT; })) {
                Boardcast() << "所有玩家都强制退出了游戏，那还玩啥玩，游戏解散，结果不会被记录";
                MatchLog_(InfoLog()) << "All users left the game";
                terminate_child = PrepareTerminate_();
            } else {
                leave_pid = it->second.pid_;
                send_leave_ipc = true;
            }
        } else {
            reply() << "[错误] 退出失败：游戏已经开始，若仍要退出游戏，请使用「" META_COMMAND_SIGN "退出 强制」命令";
            return EC_MATCH_ALREADY_BEGIN;
        }
    }
    if (terminate_child) {
        FinishTerminate_(std::move(terminate_child));
    } else if (send_leave_ipc) {
        // Call SendLeave without holding mutex (read thread may callback into Match).
        (void)game_child_->SendLeave(leave_pid);
    }
    return EC_OK;
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
// Must be called WITH mutex_ held.
void Match::ApplyChildPost_(const PostFrame& frame)
{
    using Channel = lgtbot::ipc::PostResp::Channel;
    const auto& post = frame.post;
    MsgSenderBase::MsgSenderGuard sender = [&]() -> MsgSenderBase::MsgSenderGuard
        {
            switch (post.channel()) {
            case Channel::PostResp_Channel_BROADCAST: return Boardcast();
            case Channel::PostResp_Channel_GROUP:     return GroupMsgSender()();
            case Channel::PostResp_Channel_AI:        return BoardcastAiInfo();
            case Channel::PostResp_Channel_TELL:      return Tell(PlayerID{post.target_pid()});
            default:                                  return Boardcast();
            }
        }();
    for (const auto& item : post.items()) {
        AppendMsgItem(sender, item);
    }
}

void Match::StartReadThread_()
{
    assert(game_child_);
    struct Callbacks : IMatchChildCallbacks {
        explicit Callbacks(Match& m) : match(m) {}
        void OnPost(const PostFrame& f) override {
            std::lock_guard<std::mutex> l(match.mutex_);
            match.ApplyChildPost_(f);
        }
        void OnPlayerState(const PlayerStateFrame& f) override {
            std::lock_guard<std::mutex> l(match.mutex_);
            match.ApplyChildPlayerState(f.pid, f.state);
        }
        void OnGameOver(const GameOverFrame& f) override {
            std::lock_guard<std::mutex> l(match.mutex_);
            match.ApplyChildGameOverFromScores(f);
        }
        void OnEof(const bool unexpected) override {
            if (!unexpected) {
                return;
            }
            std::unique_ptr<MatchChildClient> child;
            {
                std::lock_guard<std::mutex> l(match.mutex_);
                if (match.state_ == State::IS_OVER) {
                    return;
                }
                match.BoardcastAtAll() << "[错误] 游戏进程意外终止，游戏已中断";
                match.MatchLog_(ErrorLog()) << "Game subprocess died unexpectedly, terminating match";
                // PrepareTerminate_ calls UnbindMatchSide_ which posts a cleanup task to join
                // read_thread_.  After this returns, read_thread_ ownership is transferred to
                // the cleanup queue, so we must NOT join it here (we are the read thread).
                child = match.PrepareTerminate_();
            }
            child.reset();
        }
        Match& match;
    };
    // Capture the child pointer by value: PrepareTerminate_/UnbindMatchSide_ may move
    // game_child_ out of Match before this thread runs, but the MatchChildClient object
    // stays alive until the cleanup queue joins this thread and resets the moved unique_ptr.
    MatchChildClient* const child_for_read = game_child_.get();
    read_thread_ = std::thread([child_for_read, cbs = Callbacks{*this}]() mutable {
        if (child_for_read) {
            child_for_read->RunReadLoop(cbs);
        }
    });
}

// Must be called WITHOUT holding mutex_.
void Match::StopReadThread_()
{
    if (read_thread_.joinable()) {
        read_thread_.join();
    }
}

void Match::StartTimer(const uint64_t /*sec*/, void* /*alert_arg*/, void (*/*alert_cb*/)(void*, uint64_t))
{
    MatchLog_(WarnLog()) << "StartTimer ignored on host Match (timer runs in game subprocess)";
}

void Match::StopTimer() {}

void Match::Eliminate(const PlayerID pid)
{
    if (std::exchange(players_[pid].state_, Player::State::ELIMINATED) != Player::State::ELIMINATED) {
        Tell(pid) << "很遗憾，您被淘汰了，可以通过「" META_COMMAND_SIGN "退出」以退出游戏";
        // Enter deduction mode only if all players are eliminated AND there is at least one alive computer player
        const bool all_players_eliminated = std::ranges::all_of(players_,
                [](const auto& p) { return std::get_if<ComputerID>(&p.id_) || p.state_ == Player::State::ELIMINATED; });
        const bool has_alive_computer = std::ranges::any_of(players_,
                [](const auto& p) { return std::get_if<ComputerID>(&p.id_) && p.state_ != Player::State::ELIMINATED; });
        is_in_deduction_ = all_players_eliminated && has_alive_computer;
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

void Match::BriefInfo(std::string& out) const
{
    std::lock_guard<std::mutex> l(mutex_);
    BriefInfo_(out);
}

void Match::BriefInfo_(std::string& out) const
{
    const auto multiple = Multiple_();
    const char* const name = game_handle().Info().name_;
    const auto is_formal = options_.generic_options_.is_formal_;
    const auto usize = users_.size();
    const auto cnum = ComputerNum_();
    out = std::string("游戏名称：") + name +
        "\n- 倍率：" +
        (is_formal || multiple == 0 ? std::to_string(multiple) :
                                      "0（开启计分后为 " + std::to_string(multiple) + "）") +
        "\n- 当前用户数：" + std::to_string(usize) +
        "\n- 当前电脑数：" + std::to_string(cnum);
}

std::string Match::OptionInfo_() const
{
    return game_handle_.ConfigClient().QueryOptionInfo(true /* text_mode */);
}

void Match::ApplyChildPlayerState(const PlayerID pid, const std::string& state)
{
    if (pid.Get() >= players_.size()) {
        return;
    }
    if (state == "eliminated") {
        if (std::exchange(players_[pid.Get()].state_, Player::State::ELIMINATED) != Player::State::ELIMINATED) {
            is_in_deduction_ = std::ranges::all_of(players_, [](const Player& p)
                    {
                        return std::get_if<ComputerID>(&p.id_) || p.state_ == Player::State::ELIMINATED;
                    });
            MatchLog_(InfoLog()) << "Eliminate player pid=" << pid << " is_in_deduction=" << Bool2Str(is_in_deduction_);
        }
    } else if (state == "hooked") {
        players_[pid.Get()].state_ = Player::State::HOOKED;
    } else if (state == "active") {
        players_[pid.Get()].state_ = Player::State::ACTIVE;
    }
}

void Match::ApplyChildGameOverFromScores(const GameOverFrame& frame)
{
    if (state_ == State::IS_OVER) {
        MatchLog_(WarnLog()) << "ApplyChildGameOverFromScores but has already been over";
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
            const auto id = ConvertPid(pid);
            if (const auto pval = std::get_if<UserID>(&id); pval) {
                user_game_scores.emplace_back(*pval, score);
                for (const auto& ach_name : row.achievements()) {
                    user_achievements.emplace_back(*pval, ach_name);
                    MatchLog_(InfoLog()) << "User get achievement uid=" << *pval << " achievement=" << ach_name;
                }
            }
        }
        sender << "感谢诸位参与！";

        assert(user_game_scores.size() == users_.size());
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
    state_ = State::IS_OVER;
    game_handle_.IncreaseActivity(users_.size());
    MatchLog_(InfoLog()) << "Match is over normally";
    UnbindMatchSide_();
}

void Match::Help_(MsgSenderBase& reply, const bool text_mode)
{
    if (game_child_) {
        std::string remote;
        if (!game_child_->FetchHelp(text_mode, remote)) {
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
    const std::string remote_opts = game_handle_.ConfigClient().QueryOptionInfo(text_mode);
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
            child = PrepareTerminate_();
        } else {
            Boardcast() << "有玩家" << operation_str << "比赛，目前 " << remain << " 人尚未确定中断，所有玩家可通过「" META_COMMAND_SIGN "中断」命令确定中断比赛，或「" META_COMMAND_SIGN "中断 取消」命令取消中断比赛";
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
        const std::lock_guard<std::mutex> l(mutex_);
        if (is_force || state_ == State::NOT_STARTED) {
            BoardcastAtAll() << "游戏已解散，谢谢大家参与";
            MatchLog_(InfoLog()) << "Match is terminated outside";
            child = PrepareTerminate_();
        } else {
            return EC_MATCH_ALREADY_BEGIN;
        }
    }
    FinishTerminate_(std::move(child));
    return EC_OK;
}

void Match::UnbindMatchSide_()
{
    // If the read thread is still running, tell the subprocess to shut down so the read
    // thread unblocks and exits, then post a cleanup task to join it.
    // Both read_thread_ and game_child_ are moved into the lambda so game_child_ is
    // destroyed *after* the thread has exited (the read thread runs inside
    // game_child_->RunReadLoop and must not outlive game_child_).
    //
    // In the normal game-over path this is called from the read thread itself.  We use
    // SendShutdown() (which sends a Shutdown proto without touching WaitForResponse_) so
    // that any pending SendExecute in flight can still receive its ResultFrame before the
    // subprocess exits.  Callers that want to kill the subprocess immediately (forced
    // termination, crash, teardown) should call SignalStop() *before* UnbindMatchSide_().
    if (read_thread_.joinable()) {
        if (game_child_) {
            game_child_->CloseInput(); // EOF on subprocess stdin causes it to exit cleanly
        }
        bot_.PostCleanup([self = shared_from_this(), t = std::move(read_thread_),
                          child = std::move(game_child_)]() mutable {
            if (t.joinable()) t.join();
            child.reset(); // destroy after thread has exited
            // self (and thus ~Match) destructs here, on the cleanup thread — safe.
        });
    }
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

std::unique_ptr<MatchChildClient> Match::PrepareTerminate_()
{
    // Called under mutex_. Signal stop but do NOT join (that must happen without the lock).
    if (game_child_) {
        game_child_->SignalStop();
    }
    UnbindMatchSide_();
    return std::move(game_child_);
}

void Match::FinishTerminate_(std::unique_ptr<MatchChildClient> child)
{
    // Called WITHOUT mutex_. Join the read thread so callbacks have finished, then drop child.
    StopReadThread_(); // join (SignalStop was already called in PrepareTerminate_)
    child.reset();     // destructor closes the subprocess
}

void Match::ReleaseGameChildIfOver()
{
    // No-op: game child lifecycle is now managed via PrepareTerminate_/FinishTerminate_.
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
