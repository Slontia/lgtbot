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

std::string new_game(const UserID uid, const std::optional<GroupID> gid, const std::string& gamename, const bool for_public_match)
{
  std::string err_msg;
  const auto it = g_game_handles.find(gamename);
  if (it == g_game_handles.end()) { err_msg = "创建游戏失败：未知的游戏名"; }
  else if (gid.has_value()) { err_msg = MatchManager::NewMatch(*it->second, uid, gid); }
  else if (for_public_match) { err_msg = "创建游戏失败：公开类型游戏不支持私信建立，请在群或讨论组中公屏建立"; }
  else { err_msg = MatchManager::NewMatch(*it->second, uid, uid); }
  return err_msg.empty() ? "创建游戏成功" : "[错误] " + err_msg;
}

std::string start_game(const UserID uid, const std::optional<GroupID> gid)
{
  std::string err_msg = MatchManager::StartGame(uid);
  return err_msg.empty() ? "游戏开始" : "[错误] " + err_msg;
}

std::string leave(const UserID uid, const std::optional<GroupID> gid)
{
  std::string err_msg = MatchManager::DeletePlayer(uid);
  return err_msg.empty() ? "退出成功" : "[错误] " + err_msg;
}

std::string join(const UserID uid, const std::optional<GroupID> gid, std::optional<MatchId> match_id)
{
  std::string err_msg;
  if (match_id.has_value()) { err_msg = MatchManager::AddPlayerWithMatchID(*match_id, uid); }
  else
  {
    if (!gid.has_value()) { err_msg = "请指定比赛ID，或公屏报名"; }
    else { err_msg = MatchManager::AddPlayerWithGroupID(*gid, uid); }
  }
  return err_msg.empty() ? "加入成功" : "[错误] " + err_msg;
}
