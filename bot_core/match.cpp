#include "bot_core/match.h"

#include <cassert>

#include "utility/msg_checker.h"
#include "game_framework/game_main.h"

#include "bot_core/db_manager.h"
#include "bot_core/log.h"
#include "bot_core/match.h"
#include "bot_core/match_manager.h"

static void BoardcastMatchCanJoin(Match& match)
{
    if (match.gid().has_value()) {
        match.Boardcast() << "现在玩家可以在群里通过\"#加入游戏\"报名比赛";
    } else {
        match.Boardcast() << "现在玩家可以通过私信我\"#加入游戏 " << match.mid() << "\"报名比赛";
    }
}

Match::Match(BotCtx& bot, const MatchID mid, const GameHandle& game_handle, const UserID host_uid,
             const std::optional<GroupID> gid, const std::bitset<MatchFlag::Count()>& flags)
        : bot_(bot)
        , mid_(mid)
        , game_handle_(game_handle)
        , host_uid_(host_uid)
        , gid_(gid)
        , state_(flags.test(MatchFlag::配置) ? State::IN_CONFIGURING : State::NOT_STARTED)
        , flags_(flags)
        , options_(game_handle.make_game_options())
        , main_stage_(nullptr, [](const MainStageBase*) {}) // make when game starts
        , player_num_each_user_(1)
        , boardcast_sender_(
              gid.has_value() ? std::variant<MsgSender, BoardcastMsgSender>(MsgSender(*gid_))// : std::variant<MsgSender, BoardcastMsgSender>(MsgSender(*gid_))
                              : std::variant<MsgSender, BoardcastMsgSender>(BoardcastMsgSender(
                                      ready_uid_set_,
                                      [](std::pair<const UserID, MsgSender>& pair) -> MsgSender& { return pair.second; })
                                )
          )
        , com_num_(0)
        , multiple_(1)
        , help_cmd_(Command<void(MsgSenderBase&)>("查看游戏帮助", std::bind_front(&Match::Help_, this), VoidChecker("帮助")))
{
    if (!flags.test(MatchFlag::配置)) {
        BoardcastMatchCanJoin(*this);
    }
}

Match::~Match() {}

bool Match::Has(const UserID uid) const { return ready_uid_set_.find(uid) != ready_uid_set_.end(); }

Match::VariantID Match::ConvertPid(const PlayerID pid) const
{
    if (!pid.IsValid()) {
        return host_uid_; // TODO: UINT64_MAX for host
    }
    return players_[pid];
}

ErrCode Match::GameSetComNum(MsgSenderBase& reply, const uint64_t com_num)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (!flags_.test(MatchFlag::电脑)) {
        reply() << "[错误] 设置失败：该比赛不允许设置电脑玩家";
        return EC_MATCH_FORBIDDEN_OPERATION;
    }
    if (com_num > com_num_ && !SatisfyMaxPlayer_(com_num - com_num_)) {
        reply() << "[错误] 设置失败：比赛人数将超过上限" << game_handle_.max_player_ << "人";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    com_num_ = com_num;
    reply() << "[错误] 设置成功：当前电脑玩家数为" << com_num_;
    return EC_OK;
}

static ErrCode ConverErrCode(const StageBase::StageErrCode& stage_errcode)
{
    switch (stage_errcode) {
    case StageBase::StageErrCode::OK: return EC_GAME_REQUEST_OK;
    case StageBase::StageErrCode::CHECKOUT: return EC_GAME_REQUEST_CHECKOUT;
    case StageBase::StageErrCode::FAILED: return EC_GAME_REQUEST_FAILED;
    case StageBase::StageErrCode::NOT_FOUND: return EC_GAME_REQUEST_NOT_FOUND;
    default: return EC_GAME_REQUEST_UNKNOWN;
    }
}

ErrCode Match::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg,
                       MsgSender& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (state_ == State::NOT_STARTED) {
        reply() << "[错误] 当前阶段等待玩家加入，无法执行游戏请求\n"
                   "若您想执行元指令，请尝试在请求前加\"#\"，或通过\"#帮助\"查看所有支持的元指令";
        return EC_MATCH_NOT_BEGIN;
    } else if (state_ == State::IS_OVER) {
        reply() << "[错误] 游戏已经结束";
        return EC_MATCH_ALREADY_OVER;
    }
    reply.SetMatch(this);
    if (MsgReader reader(msg); help_cmd_.CallIfValid(reader, reply)) {
        return EC_GAME_REQUEST_OK;
    }
    if (main_stage_) {
        const auto it = uid2pid_.find(uid);
        assert(it != uid2pid_.end());
        const auto pid = it->second;
        assert(state_ == State::IS_STARTED);
        const auto stage_rc = main_stage_->HandleRequest(msg.c_str(), pid, gid.has_value(), reply);
        if (stage_rc == StageBase::StageErrCode::NOT_FOUND) {
            reply() << "[错误] 未预料的游戏指令，您可以通过\"帮助\"（不带#号）查看所有支持的游戏指令\n"
                        "若您想执行元指令，请尝试在请求前加\"#\"，或通过\"#帮助\"查看所有支持的元指令";
        }
        Routine_();
        return ConverErrCode(stage_rc);
    }
    if (!options_->SetOption(msg.c_str())) {
        assert(state_ == State::IN_CONFIGURING);
        reply() << "[错误] 未预料的游戏设置，您可以通过\"帮助\"（不带#号）查看所有支持的游戏设置\n"
                    "若您想执行元指令，请尝试在请求前加\"#\"，或通过\"#帮助\"查看所有支持的元指令";
        return EC_GAME_REQUEST_NOT_FOUND;
    }
    reply() << "设置成功！目前配置：" << OptionInfo_();
    return EC_GAME_REQUEST_OK;
}

ErrCode Match::GameConfigOver(MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (state_ != State::IN_CONFIGURING) {
        reply() << "[错误] 结束配置失败：游戏未处于配置状态";
        return EC_MATCH_NOT_IN_CONFIG;
    }
    state_ = State::NOT_STARTED;
    BoardcastMatchCanJoin(*this);
    return EC_OK;
}

ErrCode Match::GameStart(const bool is_public, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (state_ == State::IN_CONFIGURING) {
        reply() << "[错误] 开始失败：游戏正处于配置阶段，请结束配置，等待玩家加入后再开始游戏";
        return EC_MATCH_IN_CONFIG;
    }
    if (state_ == State::IS_STARTED) {
        reply() << "[错误] 开始失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    const uint64_t player_num = ready_uid_set_.size() * player_num_each_user_ + com_num_;
    assert(main_stage_ == nullptr);
    assert(game_handle_.max_player_ == 0 || player_num <= game_handle_.max_player_);
    options_->SetPlayerNum(player_num);
    if (!(main_stage_ = game_handle_.make_main_stage(reply, *options_))) {
        reply() << "[错误] 开始失败：不符合游戏参数的预期";
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    state_ = State::IS_STARTED;
    Boardcast() << "游戏开始，您可以使用<帮助>命令（无#号），查看可执行命令";
    main_stage_->Init(this);
    for (auto& [uid, sender] : ready_uid_set_) {
        uid2pid_.emplace(uid, players_.size());
        players_.emplace_back(uid);
        sender.SetMatch(this);
    }
    for (ComputerID cid = 0; cid < com_num_; ++cid) {
        players_.emplace_back(cid);
    }
    std::visit([this](auto& sender) { sender.SetMatch(this); }, boardcast_sender_);
    start_time_ = std::chrono::system_clock::now();
    return EC_OK;
}

ErrCode Match::Join(const UserID uid, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    assert(!Has(uid));
    if (state_ == State::IN_CONFIGURING && uid != host_uid_) {
        reply() << "[错误] 加入失败：房主正在配置游戏参数";
        return EC_MATCH_IN_CONFIG;
    }
    if (state_ == State::IS_STARTED) {
        reply() << "[错误] 加入失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    if (!SatisfyMaxPlayer_(1)) {
        reply() << "[错误] 加入失败：比赛人数已达到游戏上限";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    ready_uid_set_.emplace(uid, MsgSender(uid));
    Boardcast() << "玩家 " << At(uid) << " 加入了游戏";
    return EC_OK;
}

ErrCode Match::Leave(const UserID uid, MsgSenderBase& reply, const bool force)
{
    std::lock_guard<std::mutex> l(mutex_);
    assert(Has(uid));
    if (state_ != State::IS_STARTED) {
        Boardcast() << "玩家 " << At(uid) << " 退出了游戏";
        ready_uid_set_.erase(uid);
        return EC_OK;
    }
    if (force) {
        Boardcast() << "玩家 " << At(uid) << " 中途退出了游戏，他将不再参与后续的游戏进程";
        ready_uid_set_.erase(uid);
        return EC_OK;
    }
    reply() << "[错误] 退出失败：游戏已经开始，若仍要退出游戏，请使用<#强制退出游戏>命令";
    return EC_MATCH_ALREADY_BEGIN;
}

ErrCode Match::LeaveMidway(const UserID uid, const bool is_public)
{
    if (state_ != State::IS_STARTED) {
        return EC_OK;
    }
    const auto it = uid2pid_.find(uid);
    assert(it != uid2pid_.end());
    const auto pid = it->second;
    assert(main_stage_);
    const auto rc = ConverErrCode(main_stage_->HandleLeave(pid));
    Routine_();
    return rc;
}

MsgSenderBase::MsgSenderGuard Match::Boardcast()
{
    return std::visit([](auto& sender) { return sender(); }, boardcast_sender_);
}

MsgSenderBase::MsgSenderGuard Match::Tell(const uint64_t pid)
{
    const auto& id = ConvertPid(pid);
    if (const auto pval = std::get_if<UserID>(&id); !pval) {
        return EmptyMsgSender::Get()(); // is computer
    } else if (const auto it = ready_uid_set_.find(*pval); it != ready_uid_set_.end()) {
        return (it->second)();
    } else {
        return EmptyMsgSender::Get()(); // player exit
    }
}

std::vector<Match::ScoreInfo> Match::CalScores_(const int64_t scores[]) const
{
    const uint64_t user_num = uid2pid_.size();
    std::vector<Match::ScoreInfo> ret(user_num);
    const double avg_score = [scores, user_num] {
        double score_sum = 0;
        for (uint64_t pid = 0; pid < user_num; ++pid) {
            score_sum += scores[pid];
        }
        return score_sum / user_num;
    }();
    const double delta = [avg_score, scores, user_num] {
        double score_offset_sum = 0;
        for (uint64_t pid = 0; pid < user_num; ++pid) {
            double offset = scores[pid] - avg_score;
            score_offset_sum += offset > 0 ? offset : -offset;
        }
        return 1.0 / score_offset_sum;
    }();
    for (PlayerID pid = 0; pid < user_num; ++pid) {
        if (const auto pval = std::get_if<UserID>(&players_[pid])) {
            Match::ScoreInfo& score_info = ret[pid];
            score_info.uid_ = *pval;
            score_info.game_score_ = scores[pid];
            score_info.zero_sum_match_score_ = delta * (scores[pid] - avg_score);
            score_info.poss_match_score_ = 0;
        }
    }
    return ret;
}

bool Match::SwitchHost()
{
    if (ready_uid_set_.empty()) {
        return false;
    }
    if (state_ == NOT_STARTED) {
        host_uid_ = ready_uid_set_.begin()->first;
        Boardcast() << At(host_uid_) << "被选为新房主";
    }
    return true;
}

// REQUIRE: should be protected by mutex_
void Match::StartTimer(const uint64_t sec)
{
    static const uint64_t kMinAlertSec = 10;
    if (sec == 0) {
        return;
    }
    Timer::TaskSet tasks;
    timer_is_over_ = std::make_shared<bool>(false);
    // We must store a timer_is_over because it may be reset to true when new timer begins.
    const auto timeout_handler = [timer_is_over = timer_is_over_, match_wk = weak_from_this()]() {
        // Handle a reference because match may be removed from match_manager if timeout cause game over.
        auto match = match_wk.lock();
        if (!match) {
            return; // match is released
        }
        // Timeout event should not be triggered during request handling, so we need lock here.
        // timer_is_over also should protected in lock. Otherwise, a rquest may be handled after checking timer_is_over and before timeout_timer lock match.
        std::lock_guard<std::mutex> l(match->mutex_);
        // If stage has been finished by other request, timeout event should not be triggered again, so we check stage_is_over_ here.
        if (!*timer_is_over) {
            match->main_stage_->HandleTimeout();
            match->Routine_();
        }
    };
    if (kMinAlertSec > sec / 2) {
        tasks.emplace_front(sec, timeout_handler);
    } else {
        tasks.emplace_front(kMinAlertSec, timeout_handler);
        uint64_t sum_alert_sec = kMinAlertSec;
        for (uint64_t alert_sec = kMinAlertSec; sum_alert_sec < sec / 2; sum_alert_sec += alert_sec, alert_sec *= 2) {
            tasks.emplace_front(alert_sec, [this, alert_sec] {
                Boardcast() << "剩余时间" << (alert_sec / 60) << "分" << (alert_sec % 60) << "秒";
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
        *timer_is_over_ = true;
    }
    timer_ = nullptr; // stop timer
}

std::string Match::OptionInfo() const
{
    std::lock_guard<std::mutex> l(mutex_);
    return OptionInfo_();
}

std::string Match::OptionInfo_() const
{
    return options_->Status();
}

bool Match::SatisfyMaxPlayer_(const uint64_t new_player_num) const
{
    // TODO: player_num_each_user_
    // If is in config, should consider host as player
    return game_handle_.max_player_ == 0 ||
           ready_uid_set_.size() + com_num_ + new_player_num <= game_handle_.max_player_;
}

void Match::OnGameOver_()
{
    state_ = State::IS_OVER; // is necessary because other thread may own a match reference
    std::vector<int64_t> scores(options_->PlayerNum());
    for (uint64_t pid = 0; pid < options_->PlayerNum(); ++pid) {
        scores[pid] = main_stage_->PlayerScore(pid);
    }
    end_time_ = std::chrono::system_clock::now();
    auto sender = Boardcast();
    sender << "游戏结束，公布分数：\n";
    for (PlayerID pid = 0; pid < PlayerNum(); ++pid) {
        sender << At(pid) << " " << scores[pid] << "\n";
    }
    sender << "感谢诸位参与！";
#ifdef WITH_MYSQL
    const std::vector<Match::ScoreInfo> score_info = CalScores_(scores);
    if (auto& db_manager = DBManager::GetDBManager(); !db_manager) {
        sender << "\n[警告] 未连接数据库，游戏结果不会被记录";
    } else if (std::optional<uint64_t> game_id = game_handle_.game_id_.load(); !game_id.has_value()) {
        sender << "\n[警告] 该游戏未发布，游戏结果不会被记录";
    } else if (!db_manager->RecordMatch(*game_id, gid_, host_uid_, multiple_, score_info)) {
        sender << "\n[错误] 游戏结果写入数据库失败，请联系管理员";
    } else {
        sender << "\n游戏结果写入数据库成功！";
    }
#endif
    match_manager().DeleteMatch(mid_);
}

void Match::Help_(MsgSenderBase& reply)
{
    auto sender = reply();
    if (main_stage_) {
        sender << "\n[当前阶段]\n";
        main_stage_->StageInfo(sender);
        sender << "\n\n";
    }
    sender << "[当前可使用游戏命令]";
    sender << "\n[1] " << help_cmd_.Info();
    if (main_stage_) {
        main_stage_->CommandInfo(1, sender);
    } else {
        const auto option_size = options_->Size();
        for (uint64_t i = 0; i < option_size; ++i) {
            sender << "\n[" << (i + 1) << "] " << options_->Info(i);
        }
    }
}

void Match::Routine_()
{
    if (main_stage_->IsOver()) {
        OnGameOver_();
        return;
    }
    const uint64_t user_controlled_num = ready_uid_set_.size() * player_num_each_user_;
    main_stage_->HandleComputerAct(user_controlled_num, user_controlled_num + com_num_);
    if (main_stage_->IsOver()) {
        OnGameOver_();
    }
}
