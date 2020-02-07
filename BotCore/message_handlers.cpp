#include "stdafx.h"

#include <sstream>
#include <tuple>
#include <optional>

#include "message_handlers.h"
#include "match.h"
#include "lgtbot.h"
#include "log.h"

std::string show_gamelist(const UserID uid, const std::optional<GroupID> gid)
{
  std::stringstream ss;
  int i = 0;
  ss << "游戏列表：";
  for (const auto& [name, _] : g_game_handles)
  {
    ss << std::endl << (++i) << ".\t" << name;
  }
  return ss.str();
}

std::string new_game(const UserID uid, const std::optional<GroupID> gid, const std::string& gamename)
{
  std::string err_msg;
  const auto it = g_game_handles.find(gamename);
  if (it == g_game_handles.end()) { err_msg = "创建游戏失败：未知的游戏名"; }
  else if (gid.has_value()) { err_msg = MatchManager::NewMatch(*it->second, uid, gid); }
  else { err_msg = MatchManager::NewMatch(*it->second, uid, gid); }
  return err_msg.empty() ? "创建游戏成功" : "[错误] " + err_msg;
}

std::string start_game(const UserID uid, const std::optional<GroupID> gid)
{
  std::string err_msg = MatchManager::StartGame(uid, gid);
  return err_msg.empty() ? "" : "[错误] " + err_msg;
}

std::string leave(const UserID uid, const std::optional<GroupID> gid)
{
  std::string err_msg = MatchManager::DeletePlayer(uid, gid);
  return err_msg.empty() ? "" : "[错误] " + err_msg;
}

std::string join_private(const UserID uid, const std::optional<GroupID> gid, const MatchId match_id)
{
  std::string err_msg;
  if (gid.has_value()) { err_msg = "加入游戏失败：请私信裁判加入私密游戏，或去掉比赛ID以加入当前房间游戏"; }
  else { err_msg = MatchManager::AddPlayerToPrivateGame(match_id, uid); }
  return err_msg.empty() ? "" : "[错误] " + err_msg;
}

std::string join_public(const UserID uid, const std::optional<GroupID> gid)
{
  std::string err_msg;
  if (!gid.has_value()) { err_msg = "加入私密游戏，请指明比赛ID"; }
  else { err_msg = MatchManager::AddPlayerToPublicGame(*gid, uid); }
  return err_msg.empty() ? "加入成功" : "[错误] " + err_msg;
}