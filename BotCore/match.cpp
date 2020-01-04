#include "stdafx.h"
#include "match.h"
#include "log.h"
#include "match.h"
#include "../new-rock-paper-scissors/dllmain.h"
#include "../new-rock-paper-scissors/game_base.h"
#include <optional>

/* TODO:
 * 1. 将UserID GroupID MatchID用结构体封装，放到独立于项目的util/目录下，添加GetMatch_ BindMatch_ UnbindMatch_三个私有模板方法
 * 2. 添加spinlock类，以及spinlockguard类，放到util/目录下，所有公共方法加spinlock
 * 3. Game里添加spinlock，接收请求和thread回调都要加锁
 * 4. 传入GameOver接口，如果thread回调导致游戏结束，置is_over状态（is_over状态discard所有请求）调用GameOver接口，内部调用DeleteMatch方法
 */

std::map<MatchId, std::shared_ptr<Match>> MatchManager::mid2match_;
std::map<UserID, std::shared_ptr<Match>> MatchManager::uid2match_;
std::map<GroupID, std::shared_ptr<Match>> MatchManager::gid2match_;
MatchId MatchManager::next_mid_ = 0;

std::string MatchManager::NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid)
{
  if (GetMatch_(uid, uid2match_)) { return "新建游戏失败：您已加入游戏"; }
  if (gid.has_value() && GetMatch_(*gid, gid2match_)) { return "新建游戏失败：该房间已经开始游戏"; }

  const MatchId mid = NewMatchID();
  std::shared_ptr<Match> new_match = std::make_shared<Match>(mid, game_handle, uid, gid);
  
  BindMatch_(mid, mid2match_, new_match);
  BindMatch_(uid, uid2match_, new_match);
  if (gid.has_value()) { BindMatch_(*gid, gid2match_, new_match); }

  return "";
}

std::string MatchManager::StartGame(const UserID uid)
{
  const std::shared_ptr<Match>& match = GetMatch_(uid, uid2match_);
  if (!match) { return "开始游戏失败：您未加入游戏"; }
  if (match->host_uid() != uid) { return "开始游戏失败：您不是房主，没有开始游戏的权限"; }
  RETURN_IF_FAILED(match->GameStart());
  return "";
}

std::string MatchManager::AddPlayerWithMatchID(const MatchId mid, const UserID uid)
{
  const std::shared_ptr<Match> match = GetMatch_(mid, mid2match_);
  if (!match) { return "加入游戏失败：游戏ID不存在"; }
  return AddPlayer_(match, uid);
}

std::string MatchManager::AddPlayerWithGroupID(const GroupID gid, const UserID uid)
{
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

std::string MatchManager::DeletePlayer(const UserID uid)
{
  const std::shared_ptr<Match>& match = GetMatch_(uid, uid2match_);
  if (!match) { return "退出游戏失败：您未加入游戏"; }
  RETURN_IF_FAILED(match->Leave(uid));
  UnbindMatch_(uid, uid2match_);
  /* If host quit, switch host. */
  if (uid == match->host_uid() && !match->SwitchHost()) { DeleteMatch(match->mid()); }
  return "";
}

void MatchManager::DeleteMatch(const MatchId mid)
{
  const std::shared_ptr<Match> match = GetMatch_(mid, mid2match_);
  assert(match);
  match->Boardcast("游戏中止或结束");

  UnbindMatch_(mid, mid2match_);
  if (match->gid().has_value()) { UnbindMatch_(*match->gid(), gid2match_); }
  const std::set<UserID>& ready_uid_set = match->ready_uid_set();
  for (auto it = ready_uid_set.begin(); it != ready_uid_set.end(); ++ it) { UnbindMatch_(*it, uid2match_); }
}

std::shared_ptr<Match> MatchManager::GetMatch(const MatchId mid)
{
  return GetMatch_(mid, mid2match_);
}

std::shared_ptr<Match> MatchManager::GetMatch(const UserID uid, const std::optional<GroupID> gid)
{
  std::shared_ptr<Match> match = GetMatch_(uid, uid2match_);
  return (!match || (gid.has_value() && GetMatch_(*gid, gid2match_) != match)) ? nullptr : match;
}

MatchId MatchManager::NewMatchID()
{
  while (mid2match_.find(++ next_mid_) != mid2match_.end());
  return next_mid_;
}

Match::Match(const MatchId mid, const GameHandle& game_handle, const UserID host_uid, const std::optional<GroupID> gid) :
  mid_(mid), game_handle_(game_handle), host_uid_(host_uid), gid_(gid), state_(PREPARE) {}

Match::~Match()
{
  if (game_) { game_handle_.delete_game_(game_); }
}

bool Match::Has(const UserID uid) const
{
  return ready_uid_set_.find(uid) != ready_uid_set_.end();
}

std::string Match::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg)
{
  if (state_ != GAMING) { return "[错误] 当前游戏未处于进行状态"; }
  const auto it = uid2pid_.find(uid);
  assert(it != uid2pid_.end());
  game_->HandleRequest(it->second, gid.has_value(), msg.c_str());
  return "";
}

std::string Match::GameStart()
{
  if (state_ != PREPARE) { return "开始游戏失败：游戏未处于准备状态"; }
  const uint64_t player_num = ready_uid_set_.size();
  if (player_num < game_handle_.min_player_) { return "开始游戏失败：玩家人数过少"; }
  assert(game_handle_.max_player_ == 0 || player_num <= game_handle_.max_player_);
  for (UserID uid : ready_uid_set_)
  {
    uid2pid_.emplace(uid, pid2uid_.size());
    pid2uid_.push_back(uid);
  }
  state_ = GAMING;
  game_ = game_handle_.new_game_(mid_, player_num);
  return "";
}

std::string Match::Join(const UserID uid)
{
  assert(!Has(uid));
  if (state_ != PREPARE) { return "加入游戏失败：游戏已经开始"; }
  if (ready_uid_set_.size() >= game_handle_.max_player_) { return "加入游戏失败：比赛人数已达到游戏上线"; }
  ready_uid_set_.emplace(uid);
  Boardcast("玩家 " + At(uid) + " 加入了游戏");
  return "";
}

std::string Match::Leave(const UserID uid)
{
  assert(Has(uid));
  if (state_ != PREPARE) { return "退出失败：游戏已经开始"; }
  Boardcast("玩家 " + At(uid) + " 退出了游戏");
  ready_uid_set_.erase(uid);
  return "";
}

void Match::Boardcast(const std::string& msg) const
{
  if (gid_.has_value()) { SendPublicMsg(*gid_, msg); }
  else { for (const UserID uid : ready_uid_set_) { SendPrivateMsg(uid, msg); } }
}

void Match::Tell(const uint64_t pid, const std::string& msg) const
{
  SendPrivateMsg(pid2uid_[pid], msg);
}

std::string Match::At(const uint64_t pid) const
{
  return ::At(pid2uid_[pid]);
}
