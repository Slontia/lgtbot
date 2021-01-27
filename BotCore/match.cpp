#include "match.h"
#include "log.h"
#include "match.h"
#include "db_manager.h"
#include "util.h"
#include "GameFramework/game_main.h"
#include "GameFramework/game_base.h"
#include <assert.h>

std::map<MatchId, std::shared_ptr<Match>> MatchManager::mid2match_;
std::map<UserID, std::shared_ptr<Match>> MatchManager::uid2match_;
std::map<GroupID, std::shared_ptr<Match>> MatchManager::gid2match_;
MatchId MatchManager::next_mid_ = 0;
SpinLock MatchManager::spinlock_; // to lock match map

std::string MatchManager::NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid, const bool skip_config)
{
  std::lock_guard<SpinLock> l(spinlock_);
  if (GetMatch_(uid, uid2match_)) { return "新建游戏失败：您已加入游戏"; }
  if (gid.has_value() && GetMatch_(*gid, gid2match_)) { return "新建游戏失败：该房间已经开始游戏"; }

  const MatchId mid = NewMatchID_();
  std::shared_ptr<Match> new_match = std::make_shared<Match>(mid, game_handle, uid, gid, skip_config);
  
  RETURN_IF_FAILED(AddPlayer_(new_match, uid));
  BindMatch_(mid, mid2match_, new_match);
  if (gid.has_value()) { BindMatch_(*gid, gid2match_, new_match); }

  return "";
}

std::string MatchManager::ConfigOver(const UserID uid, const std::optional<GroupID> gid)
{
  std::lock_guard<SpinLock> l(spinlock_);
  const std::shared_ptr<Match>& match = GetMatch_(uid, uid2match_);
  if (!match) { return "结束配置失败：您未加入游戏"; }
  else if (match->host_uid() != uid) { return "结束配置失败：您不是房主，没有结束配置的权限"; }
  else if (!match->IsPrivate() && !gid.has_value()) { return "结束配置失败：请公开结束配置"; }
  else if (match->gid() != gid) { return "结束配置失败：您未在该房间建立游戏"; }
  RETURN_IF_FAILED(match->GameConfigOver());
  return "";
}

std::string MatchManager::StartGame(const UserID uid, const std::optional<GroupID> gid)
{
  std::lock_guard<SpinLock> l(spinlock_);
  const std::shared_ptr<Match>& match = GetMatch_(uid, uid2match_);
  if (!match) { return "开始游戏失败：您未加入游戏"; }
  else if (match->host_uid() != uid) { return "开始游戏失败：您不是房主，没有开始游戏的权限"; }
  else if (!match->IsPrivate() && !gid.has_value()) { return "开始游戏失败：请公开开始游戏"; }
  else if (match->gid() != gid) { return "开始游戏失败：您未在该房间建立游戏"; }
  RETURN_IF_FAILED(match->GameStart());
  return "";
}

std::string MatchManager::AddPlayerToPrivateGame(const MatchId mid, const UserID uid)
{
  std::lock_guard<SpinLock> l(spinlock_);
  const std::shared_ptr<Match> match = GetMatch_(mid, mid2match_);
  if (!match) { return "加入游戏失败：游戏ID不存在"; }
  else if (!match->IsPrivate()) { return "加入游戏失败：该游戏属于公开比赛，请前往房间加入游戏"; }
  return AddPlayer_(match, uid);
}

std::string MatchManager::AddPlayerToPublicGame(const GroupID gid, const UserID uid)
{
  std::lock_guard<SpinLock> l(spinlock_);
  const std::shared_ptr<Match> match = GetMatch_(gid, gid2match_);
  if (!match) { return "加入游戏失败：该房间未进行游戏"; }
  return AddPlayer_(match, uid);
}

std::string MatchManager::AddPlayer_(const std::shared_ptr<Match>& match, const UserID uid)
{
  assert(match);
  if (GetMatch_(uid, uid2match_)) { return "加入游戏失败：您已加入其它游戏，请先退出"; }
  RETURN_IF_FAILED(match->Join(uid));
  BindMatch_(uid, uid2match_, match);
  return "";
}

std::string MatchManager::DeletePlayer(const UserID uid, const std::optional<GroupID> gid)
{
  std::lock_guard<SpinLock> l(spinlock_);
  const std::shared_ptr<Match>& match = GetMatch_(uid, uid2match_);
  if (!match) { return "退出游戏失败：您未加入游戏"; }
  else if (!match->IsPrivate() && !gid.has_value()) { return "退出游戏失败：请公开退出游戏"; }
  else if (match->gid() != gid) { return "退出游戏失败：您未加入本房间游戏"; }
  RETURN_IF_FAILED(match->Leave(uid));
  UnbindMatch_(uid, uid2match_);
  /* If host quit, switch host. */
  if (uid == match->host_uid() && !match->SwitchHost()) { DeleteMatch_(match->mid()); }
  return "";
}

void MatchManager::DeleteMatch(const MatchId mid)
{
  std::lock_guard<SpinLock> l(spinlock_);
  return DeleteMatch_(mid);
}

void MatchManager::DeleteMatch_(const MatchId mid)
{
  if (const std::shared_ptr<Match> match = GetMatch_(mid, mid2match_); match)
  {
	UnbindMatch_(mid, mid2match_);
	if (match->gid().has_value()) { UnbindMatch_(*match->gid(), gid2match_); }
	const std::set<UserID>& ready_uid_set = match->ready_uid_set();
	for (auto it = ready_uid_set.begin(); it != ready_uid_set.end(); ++ it) { UnbindMatch_(*it, uid2match_); }
  }
}

std::shared_ptr<Match> MatchManager::GetMatch(const MatchId mid)
{
  std::lock_guard<SpinLock> l(spinlock_);
  return GetMatch_(mid, mid2match_);
}

std::shared_ptr<Match> MatchManager::GetMatch(const UserID uid, const std::optional<GroupID> gid)
{
  std::lock_guard<SpinLock> l(spinlock_);
  std::shared_ptr<Match> match = GetMatch_(uid, uid2match_);
  return (!match || (gid.has_value() && GetMatch_(*gid, gid2match_) != match)) ? nullptr : match;
}

std::shared_ptr<Match> MatchManager::GetMatchWithGroupID(const GroupID gid)
{
  std::lock_guard<SpinLock> l(spinlock_);
  return GetMatch_(gid, gid2match_);
}

void MatchManager::ForEachMatch(const std::function<void(const std::shared_ptr<Match>)> handle)
{
  std::lock_guard<SpinLock> l(spinlock_);
  for (const auto&[_, match] : mid2match_) { handle(match); }
}

MatchId MatchManager::NewMatchID_()
{
  while (mid2match_.find(++ next_mid_) != mid2match_.end());
  return next_mid_;
}

template <typename IDType>
std::shared_ptr<Match> MatchManager::GetMatch_(const IDType id, const std::map<IDType, std::shared_ptr<Match>>& id2match)
{
  const auto it = id2match.find(id);
  return (it == id2match.end()) ? nullptr : it->second;
}

template <typename IDType>
void MatchManager::BindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match, std::shared_ptr<Match> match)
{
  if (!id2match.emplace(id, match).second) { assert(false); }
}

template <typename IDType>
void MatchManager::UnbindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match)
{
  id2match.erase(id);
}

Match::Match(const MatchId mid, const GameHandle& game_handle, const UserID host_uid, const std::optional<GroupID> gid, const bool skip_config)
  : mid_(mid), game_handle_(game_handle), host_uid_(host_uid), gid_(gid), state_(skip_config ? State::NOT_STARTED : State::IN_CONFIGURING),
  game_(game_handle_.new_game_(this), game_handle_.delete_game_), multiple_(1) {}

Match::~Match() {}

bool Match::Has(const UserID uid) const
{
  return ready_uid_set_.find(uid) != ready_uid_set_.end();
}

std::string Match::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg)
{
  if (state_ == State::NOT_STARTED) { return "[错误] 当前阶段等待玩家加入，无法执行游戏请求"; }
  if (state_ == State::IN_CONFIGURING) { game_->HandleRequest(0, gid.has_value(), msg.c_str()); }
  else
  {
	const auto it = uid2pid_.find(uid);
	assert(it != uid2pid_.end());
	game_->HandleRequest(it->second, gid.has_value(), msg.c_str());
  }
  return "";
}

std::string Match::GameConfigOver()
{
  if (state_ != State::IN_CONFIGURING) { return "结束配置失败：游戏未处于配置状态"; }
  state_ = State::NOT_STARTED;
  return "配置结束，现在玩家可以加入比赛！";
}

std::string Match::GameStart()
{
  if (state_ == State::IN_CONFIGURING) { return "开始游戏失败：游戏正处于配置阶段，请结束配置，等待玩家加入后再开始游戏"; }
  if (state_ == State::IS_STARTED) { return "开始游戏失败：游戏已经开始"; }
  const uint64_t player_num = ready_uid_set_.size();
  if (player_num < game_handle_.min_player_) { return "开始游戏失败：玩家人数过少"; }
  assert(game_handle_.max_player_ == 0 || player_num <= game_handle_.max_player_);
  if (!game_->StartGame(player_num)) { return "开始游戏失败：不符合游戏参数的预期"; }

  for (UserID uid : ready_uid_set_)
  {
    uid2pid_.emplace(uid, pid2uid_.size());
    pid2uid_.push_back(uid);
  }
  state_ = State::IS_STARTED;
  Boardcast() << "游戏开始，您可以使用<帮助>命令（无#号），查看可执行命令";
  start_time_ = std::chrono::system_clock::now();
  return "";
}

std::string Match::Join(const UserID uid)
{
  assert(!Has(uid));
  if (state_ == State::IN_CONFIGURING && uid != host_uid_) { return "加入游戏失败：房主正在配置游戏参数"; }
  if (state_ == State::IS_STARTED) { return "加入游戏失败：游戏已经开始"; }
  if (ready_uid_set_.size() >= game_handle_.max_player_) { return "加入游戏失败：比赛人数已达到游戏上线"; }
  ready_uid_set_.emplace(uid);
  Boardcast() << "玩家 " << AtMsg(uid) << " 加入了游戏";
  return "";
}

std::string Match::Leave(const UserID uid)
{
  assert(Has(uid));
  if (state_ == State::IS_STARTED) { return "退出失败：游戏已经开始"; }
  Boardcast() << "玩家 " << AtMsg(uid) << " 退出了游戏";
  ready_uid_set_.erase(uid);
  return "";
}

MsgSenderWrapperBatch Match::Boardcast() const
{
  std::vector<MsgSenderWrapper> senders;
  if (gid_.has_value())
  {
    senders.emplace_back(ToGroup(*gid_));
  }
  else
  {
    for (const UserID uid : ready_uid_set_)
    {
      senders.emplace_back(ToUser(uid));
    }
  }
  return MsgSenderWrapperBatch(std::move(senders));
}

MsgSenderWrapper Match::Tell(const uint64_t pid) const
{
  return ToUser(pid2uid(pid));
}

void Match::GameOver(const int64_t scores[])
{
  if (!scores)
  {
    Boardcast() << "游戏中断，该局游戏成绩无效，感谢诸位参与！";
    return;
  }
  end_time_ = std::chrono::system_clock::now();
  assert(ready_uid_set_.size() == pid2uid_.size());
  std::vector<Match::ScoreInfo> score_info = CalScores_(scores);
  auto sender = Boardcast();
  sender << "游戏结束，公布分数：\n";
  for (uint64_t pid = 0; pid < pid2uid_.size(); ++pid) { sender << AtMsg(pid2uid_[pid]) << " " << scores[pid] << "\n"; }
  sender << "感谢诸位参与！";
  if (auto& db_manager = DBManager::GetDBManager(); db_manager != nullptr)
  {
    std::optional<uint64_t> game_id = game_handle_.game_id_.load();
    if (game_id.has_value() && !db_manager->RecordMatch(*game_id, gid_, host_uid_, multiple_, score_info))
    {
      sender << "\n错误：游戏结果写入数据库失败，请联系管理员";
    }
  }
}

std::vector<Match::ScoreInfo> Match::CalScores_(const int64_t scores[]) const
{
  const uint64_t player_num = pid2uid_.size();
  std::vector<Match::ScoreInfo> ret(player_num);
  const double avg_score = [scores, player_num]
  {
    double score_sum = 0;
    for (uint64_t pid = 0; pid < player_num; ++pid) { score_sum += scores[pid]; }
    return score_sum / player_num;
  }();
  const double delta = [avg_score, scores, player_num]
  {
    double score_offset_sum = 0;
    for (uint64_t pid = 0; pid < player_num; ++pid)
    {
      double offset = scores[pid] - avg_score;
      score_offset_sum += offset > 0 ? offset : -offset;
    }
    return 1.0 / score_offset_sum;
  }();
  for (uint64_t pid = 0; pid < player_num; ++pid)
  {
    Match::ScoreInfo& score_info = ret[pid];
    score_info.uid_ = pid2uid_[pid];
    score_info.game_score_ = scores[pid];
    score_info.zero_sum_match_score_ = delta * (scores[pid] - avg_score);
    score_info.poss_match_score_ = 0;
  }
  return ret;
}

bool Match::SwitchHost()
{
  if (ready_uid_set_.empty()) { return false; }
  host_uid_ = *ready_uid_set_.begin();
  Boardcast() << AtMsg(host_uid_) << "被选为新房主";
  return true;
}

void Match::StartTimer(const uint64_t sec)
{
  static const uint64_t kMinAlertSec = 10;
  if (sec == 0) { return; }
  Timer::TaskSet tasks;
  std::shared_ptr<bool> stage_is_over = std::make_shared<bool>(false);
  std::function<void(Timer*)> deleter = [stage_is_over](Timer* timer)
  {
    *stage_is_over = true;
    delete timer;
  };
  std::function<void()> timeout_handler = [match = shared_from_this(), stage_is_over]()
  {
    match->game_->HandleTimeout(stage_is_over.get());
  };
  if (kMinAlertSec > sec / 2)
  {
    tasks.emplace_front(sec, timeout_handler);
  }
  else
  {
    tasks.emplace_front(kMinAlertSec, timeout_handler);
    uint64_t sum_alert_sec = kMinAlertSec;
    for (uint64_t alert_sec = kMinAlertSec; sum_alert_sec < sec / 2; sum_alert_sec += alert_sec, alert_sec *= 2)
    {
      tasks.emplace_front(alert_sec, [this, alert_sec]
          {
            Boardcast() << "剩余时间" << (alert_sec / 60) << "分" << (alert_sec % 60) << "秒";
          });
    }
    tasks.emplace_front(sec - sum_alert_sec, [] {});
  }
  timer_ = std::unique_ptr<Timer, std::function<void(Timer*)>>(new Timer(std::move(tasks)), std::move(deleter));
}

void Match::StopTimer()
{
  timer_ = nullptr;
}

std::string Match::OptionInfo() const
{
  return game_->OptionInfo();
}
