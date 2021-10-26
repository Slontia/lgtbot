#include "bot_core/match.h"

#include <cassert>

#include <filesystem>

#include "utility/msg_checker.h"
#include "game_framework/game_main.h"

#include "bot_core/db_manager.h"
#include "bot_core/log.h"
#include "bot_core/match.h"
#include "bot_core/match_manager.h"

static void BoardcastMatchCanJoin(Match& match)
{
    if (match.gid().has_value()) {
        match.Boardcast() << "现在玩家可以在群里通过\"#加入\"报名比赛";
    } else {
        match.Boardcast() << "现在玩家可以通过私信我\"#加入 " << match.mid() << "\"报名比赛";
    }
}

Match::Match(BotCtx& bot, const MatchID mid, const GameHandle& game_handle, const UserID host_uid,
             const std::optional<GroupID> gid)
        : bot_(bot)
        , mid_(mid)
        , game_handle_(game_handle)
        , host_uid_(host_uid)
        , gid_(gid)
        , state_(State::NOT_STARTED)
        , options_(game_handle.make_game_options())
        , main_stage_(nullptr, [](const MainStageBase*) {}) // make when game starts
        , player_num_each_user_(1)
        , users_()
        , boardcast_sender_(
              gid.has_value() ? std::variant<MsgSender, MsgSenderBatch>(MsgSender(*gid_))
                              : std::variant<MsgSender, MsgSenderBatch>(MsgSenderBatch([this](const std::function<void(MsgSender&)>& fn)
                                        {
                                            for (auto& [_, user_info] : users_) {
                                                if (!user_info.is_left_during_game_) {
                                                    fn(user_info.sender_);
                                                }
                                            }
                                        })
                                )
          )
        , bench_to_player_num_(0)
        , multiple_(1)
        , help_cmd_(Command<void(MsgSenderBase&)>("查看游戏帮助", std::bind_front(&Match::Help_, this), VoidChecker("帮助"), OptionalDefaultChecker<BoolChecker>(false, "文字", "图片")))
{
    users_.emplace(host_uid, ParticipantUser(host_uid));
    BoardcastMatchCanJoin(*this);
}

Match::~Match() {}

bool Match::Has(const UserID uid) const { return users_.find(uid) != users_.end(); }

Match::VariantID Match::ConvertPid(const PlayerID pid) const
{
    if (!pid.IsValid()) {
        return host_uid_; // TODO: UINT64_MAX for host
    }
    return players_[pid];
}

ErrCode Match::SetBenchTo(const UserID uid, MsgSenderBase& reply, const uint64_t bench_to_player_num)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限";
        return EC_MATCH_NOT_HOST;
    }
    auto sender = reply();
    if (bench_to_player_num <= user_controlled_player_num()) {
        sender << "[警告] 当前玩家数" << user_controlled_player_num() << "已满足条件";
    } else if (game_handle_.max_player_ != 0 && bench_to_player_num > game_handle_.max_player_) {
        sender << "[错误] 设置失败：比赛人数将超过上限" << game_handle_.max_player_ << "人";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    bench_to_player_num_ = bench_to_player_num;
    sender << "设置成功：当前电脑玩家数为" << com_num();
    KickForConfigChange_();
    return EC_OK;
}

static ErrCode ConverErrCode(const StageErrCode& stage_errcode)
{
    switch (stage_errcode) {
    case StageErrCode::OK: return EC_GAME_REQUEST_OK;
    case StageErrCode::CHECKOUT: return EC_GAME_REQUEST_CHECKOUT;
    case StageErrCode::FAILED: return EC_GAME_REQUEST_FAILED;
    case StageErrCode::NOT_FOUND: return EC_GAME_REQUEST_NOT_FOUND;
    default: return EC_GAME_REQUEST_UNKNOWN;
    }
}

ErrCode Match::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg,
                       MsgSender& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (state_ == State::IS_OVER) {
        reply() << "[错误] 游戏已经结束";
        return EC_MATCH_ALREADY_OVER;
    }
    reply.SetMatch(this);
    if (MsgReader reader(msg); help_cmd_.CallIfValid(reader, reply)) {
        return EC_GAME_REQUEST_OK;
    }
    if (main_stage_) {
        // TODO: check player_num_each_user_
        const auto it = users_.find(uid);
        assert(it != users_.end());
        const auto pid = it->second.pids_[0];
        assert(state_ == State::IS_STARTED);
        const auto stage_rc = main_stage_->HandleRequest(msg.c_str(), pid, gid.has_value(), reply);
        if (stage_rc == StageErrCode::NOT_FOUND) {
            reply() << "[错误] 未预料的游戏指令，您可以通过\"帮助\"（不带#号）查看所有支持的游戏指令\n"
                        "若您想执行元指令，请尝试在请求前加\"#\"，或通过\"#帮助\"查看所有支持的元指令";
        }
        Routine_();
        return ConverErrCode(stage_rc);
    }
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限";
        return EC_MATCH_NOT_HOST;
    }
    if (!options_->SetOption(msg.c_str())) {
        reply() << "[错误] 未预料的游戏设置，您可以通过\"帮助\"（不带#号）查看所有支持的游戏设置\n"
                    "若您想执行元指令，请尝试在请求前加\"#\"，或通过\"#帮助\"查看所有支持的元指令";
        return EC_GAME_REQUEST_NOT_FOUND;
    }
    reply() << "设置成功！目前配置：" << OptionInfo_();
    KickForConfigChange_();
    return EC_GAME_REQUEST_OK;
}

ErrCode Match::GameStart(const UserID uid, const bool is_public, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (state_ == State::IS_STARTED) {
        reply() << "[错误] 开始失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    if (uid != host_uid_) {
        reply() << "[错误] 您并非房主，没有变更游戏设置的权限";
        return EC_MATCH_NOT_HOST;
    }
    const uint64_t player_num = std::max(user_controlled_player_num(), bench_to_player_num_);
    const std::filesystem::path resource_dir = std::filesystem::path(bot_.game_path()) / game_handle_.module_name_ / "";
    std::cout << resource_dir << std::endl;
    assert(main_stage_ == nullptr);
    assert(game_handle_.max_player_ == 0 || player_num <= game_handle_.max_player_);
    options_->SetPlayerNum(player_num);
    options_->SetResourceDir(resource_dir.c_str());
    if (!(main_stage_ = game_handle_.make_main_stage(reply, *options_, *this))) {
        reply() << "[错误] 开始失败：不符合游戏参数的预期";
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    state_ = State::IS_STARTED;
    Boardcast() << "游戏开始，您可以使用<帮助>命令（无#号），查看可执行命令";
    for (auto& [uid, user_info] : users_) {
        for (int i = 0; i < player_num_each_user_; ++i) {
            user_info.pids_.emplace_back(players_.size());
            players_.emplace_back(uid);
        }
        user_info.sender_.SetMatch(this);
    }
    for (ComputerID cid = 0; cid < com_num(); ++cid) {
        players_.emplace_back(cid);
    }
    std::visit([this](auto& sender) { sender.SetMatch(this); }, boardcast_sender_);
    start_time_ = std::chrono::system_clock::now();
    main_stage_->HandleStageBegin();
    Routine_(); // computer act first
    return EC_OK;
}

ErrCode Match::Join(const UserID uid, MsgSenderBase& reply)
{
    std::lock_guard<std::mutex> l(mutex_);
    if (state_ == State::IS_STARTED) {
        reply() << "[错误] 加入失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    if (users_.size() == game_handle_.max_player_) {
        reply() << "[错误] 加入失败：比赛人数已达到游戏上限";
        return EC_MATCH_ACHIEVE_MAX_PLAYER;
    }
    if (Has(uid)) {
        reply() << "[错误] 加入失败：您已加入该游戏";
        return EC_MATCH_USER_ALREADY_IN_MATCH;
    }
    if (!match_manager().BindMatch(uid, shared_from_this())) {
        reply() << "[错误] 加入失败：您已加入其他游戏，您可通过私信裁判\"#游戏信息\"查看该游戏信息";
        return EC_MATCH_USER_ALREADY_IN_OTHER_MATCH;
    }
    users_.emplace(uid, ParticipantUser(uid));
    Boardcast() << "玩家 " << At(uid) << " 加入了游戏";
    return EC_OK;
}

ErrCode Match::Leave(const UserID uid, MsgSenderBase& reply, const bool force)
{
    ErrCode rc = EC_OK;
    std::lock_guard<std::mutex> l(mutex_);
    assert(Has(uid));
    if (state_ != State::IS_STARTED) {
        match_manager().UnbindMatch(uid);
        Boardcast() << "玩家 " << At(uid) << " 退出了游戏";
        users_.erase(uid);
        if (users_.empty()) {
            Boardcast() << "所有玩家都退出了游戏，游戏解散";
            Unbind_();
        } else if (uid == host_uid_) {
            host_uid_ = users_.begin()->first;
            Boardcast() << At(host_uid_) << "被选为新房主";
        }
    } else if (force) {
        match_manager().UnbindMatch(uid);
        Boardcast() << "玩家 " << At(uid) << " 中途退出了游戏，他将不再参与后续的游戏进程";
        const auto it = users_.find(uid);
        assert(it != users_.end());
        assert(main_stage_);
        assert(!it->second.is_left_during_game_);
        it->second.is_left_during_game_ = true;
        for (const auto pid : it->second.pids_) {
            main_stage_->HandleLeave(pid);
            Routine_();
        }
    } else {
        reply() << "[错误] 退出失败：游戏已经开始，若仍要退出游戏，请使用<#强制退出游戏>命令";
        rc = EC_MATCH_ALREADY_BEGIN;
    }
    return rc;
}

MsgSenderBase& Match::BoardcastMsgSender()
{
    return std::visit([](auto& sender) -> MsgSenderBase& { return sender; }, boardcast_sender_);
}

MsgSenderBase& Match::TellMsgSender(const PlayerID pid)
{
    const auto& id = ConvertPid(pid);
    if (const auto pval = std::get_if<UserID>(&id); !pval) {
        return EmptyMsgSender::Get(); // is computer
    } else if (const auto it = users_.find(*pval); it != users_.end() && !it->second.is_left_during_game_) {
        return it->second.sender_;
    } else {
        return EmptyMsgSender::Get(); // player exit
    }
}

std::vector<Match::ScoreInfo> Match::CalScores_(const std::vector<int64_t>& scores) const
{
    const auto player_num = PlayerNum();
    assert(scores.size() == player_num);
    std::vector<Match::ScoreInfo> ret(player_num);
    const double avg_score = [scores, player_num] {
        double score_sum = 0;
        for (uint64_t pid = 0; pid < player_num; ++pid) {
            score_sum += scores[pid];
        }
        return score_sum / player_num;
    }();
    const double delta = [avg_score, scores, player_num] {
        double score_offset_sum = 0;
        for (uint64_t pid = 0; pid < player_num; ++pid) {
            double offset = scores[pid] - avg_score;
            score_offset_sum += offset > 0 ? offset : -offset;
        }
        return 1.0 / score_offset_sum;
    }();
    for (PlayerID pid = 0; pid < player_num; ++pid) {
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
    if (users_.empty()) {
        return false;
    }
    if (state_ == NOT_STARTED) {
        host_uid_ = users_.begin()->first;
        Boardcast() << At(host_uid_) << "被选为新房主";
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
            tasks.emplace_front(alert_sec, std::bind(cb, p, alert_sec));
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

void Match::ShowInfo(const std::optional<GroupID>& gid, MsgSenderBase& reply) const
{
    std::lock_guard<std::mutex> l(mutex_);
    auto sender = reply();
    sender << "游戏名称：" << game_handle().name_ << "\n";
    sender << "配置信息：" << OptionInfo_() << "\n";
    sender << "电脑数量：" << com_num() << "\n";
    sender << "游戏状态："
           << (state() == Match::State::NOT_STARTED ? "未开始" : "已开始")
           << "\n";
    sender << "房间号：";
    if (gid_.has_value()) {
        sender << *gid << "\n";
    } else {
        sender << "私密游戏"
               << "\n";
    }
    sender << "最多可参加人数：";
    if (game_handle().max_player_ == 0) {
        sender << "无限制";
    } else {
        sender << game_handle().max_player_;
    }
    sender << "人\n房主：" << Name(host_uid());
    if (state() == Match::State::IS_STARTED) {
        const auto num = PlayerNum();
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

std::string Match::OptionInfo_() const
{
    return options_->Status();
}

void Match::OnGameOver_()
{
    state_ = State::IS_OVER; // is necessary because other thread may own a match reference
    std::vector<int64_t> scores(PlayerNum());
    for (uint64_t pid = 0; pid < PlayerNum(); ++pid) {
        scores[pid] = main_stage_->PlayerScore(pid);
    }
    end_time_ = std::chrono::system_clock::now();
    {
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
    }
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
        const auto option_size = options_->Size();
        for (uint64_t i = 0; i < option_size; ++i) {
            outstr += "\n" + std::to_string(i + 1) + ". " + (text_mode ? options_->Info(i) : options_->ColoredInfo(i));
        }
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
        if (StageErrCode::OK == main_stage_->HandleComputerAct(user_controlled_num + i)) {
            ++ok_count;
        } else {
            ok_count = 0;
        }
    }
    if (main_stage_->IsOver()) {
        OnGameOver_();
    }
}

void Match::Terminate()
{
    const std::lock_guard<std::mutex> l(mutex_);
    Terminate_();
}

void Match::Terminate_()
{
    for (auto& [uid, uesr_info] : users_) {
        match_manager().UnbindMatch(uid);
    }
    users_.clear();
    Unbind_();
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
