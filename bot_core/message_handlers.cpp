#include "message_handlers.h"
#include "match.h"
#include "log.h"
#include "utility/msg_checker.h"
#include "db_manager.h"
#include "bot_core.h"
#include "util.h"

static const auto make_command = [](std::string&& description, const auto& cb, auto&&... checkers) -> std::shared_ptr<MetaCommand>
{
  return MakeCommand<ErrCode(const UserID, const std::optional<GroupID>, const replier_t)>(std::move(description), cb, std::move(checkers)...);
};

static ErrCode help(const UserID uid, const std::optional<GroupID> gid, const replier_t reply,
    const std::vector<std::shared_ptr<MetaCommand>>& cmds, const std::string& type)
{
  auto sender = reply();
  sender << "[可使用的" << type << "指令]";
  int i = 0;
  for (const std::shared_ptr<MetaCommand>& cmd : cmds)
  {
    sender << "\n[" << (++i) << "] " << cmd->Info();
  }
  return EC_OK;
}

ErrCode HandleRequest(const UserID uid, const std::optional<GroupID> gid, MsgReader& reader,
    const replier_t reply, const std::vector<std::shared_ptr<MetaCommand>>& cmds,
    const std::string_view type)
{
  reader.Reset();
  for (const std::shared_ptr<MetaCommand>& cmd : cmds)
  {
    const std::optional<ErrCode> errcode = cmd->CallIfValid(reader, std::tuple{ uid, gid, reply });
    if (errcode.has_value()) { return *errcode; }
  }
  reply() << "[错误] 未预料的" << type << "指令";
  return EC_REQUEST_NOT_FOUND;
}

static ErrCode show_gamelist(const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
{
  int i = 0;
  if (g_game_handles.empty())
  {
    reply() << "未载入任何游戏";
    return EC_OK;
  }
  reply() << "游戏列表：";
  for (const auto& [name, _] : g_game_handles)
  {
    reply() << "\n" << (++i) << ". " << name;
  }
  return EC_OK;
}

template <bool skip_config>
static ErrCode new_game(const UserID uid, const std::optional<GroupID> gid, const replier_t reply,
    const std::string& gamename)
{
  const auto it = g_game_handles.find(gamename);
  if (it == g_game_handles.end())
  {
    reply() << "[错误] 创建失败：未知的游戏名";
    return EC_REQUEST_UNKNOWN_GAME;
  };
  return MatchManager::NewMatch(*it->second, uid, gid, skip_config, reply);
}

static ErrCode config_over(const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
{
  return MatchManager::ConfigOver(uid, gid, reply);
}

static ErrCode start_game(const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
{
  return MatchManager::StartGame(uid, gid, reply);
}

static ErrCode leave(const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
{
  return MatchManager::DeletePlayer(uid, gid, reply);
}

static ErrCode join_private(const UserID uid, const std::optional<GroupID> gid, const replier_t reply, const MatchId match_id)
{
  if (gid.has_value())
  {
    reply() << "[错误] 加入失败：请私信裁判加入私密游戏，或去掉比赛ID以加入当前房间游戏";
    return EC_MATCH_NEED_REQUEST_PRIVATRE;
  };
  return MatchManager::AddPlayerToPrivateGame(match_id, uid, reply);
}

static ErrCode join_public(const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
{
  if (!gid.has_value())
  {
    reply() << "[错误] 加入失败：若要加入私密游戏，请指明比赛ID";
    return EC_MATCH_NEED_ID;
  };
  return MatchManager::AddPlayerToPublicGame(*gid, uid, reply);
}

static ErrCode show_private_matches(const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
{
  uint64_t count = 0;
  auto sender = reply();
  MatchManager::ForEachMatch([&sender, &count](const std::shared_ptr<Match> match)
  {
    if (match->IsPrivate() && match->state() == Match::State::NOT_STARTED)
    {
      ++count;
      sender << match->game_handle().name_ << " - [房主ID] " << match->host_uid() << " - [比赛ID] " << match->mid() << "\n";
    }
  });
  if (count == 0)
  {
    sender << "当前无未开始的私密比赛";
  }
  else
  {
    sender << "共" << count << "场";
  }
  return EC_OK;
}

static ErrCode show_match_status(const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
{
  // TODO: make it thread safe
  std::shared_ptr<Match> match = MatchManager::GetMatch(uid, gid);
  if (!match && gid.has_value()) { match = MatchManager::GetMatchWithGroupID(*gid); }
  if (!match && gid.has_value())
  {
    reply() << "[错误] 查看失败：该房间未进行游戏";
    return EC_MATCH_GROUP_NOT_IN_MATCH;
  };
  if (!match && !gid.has_value())
  {
    reply() << "[错误] 查看失败：您未加入游戏";
    return EC_MATCH_USER_NOT_IN_MATCH;
  };
  auto sender = reply();
  sender << "游戏名称：" << match->game_handle().name_ << "\n";
  sender << "配置信息：" << match->OptionInfo() << "\n";
  sender << "游戏状态：" << (match->state() == Match::State::IN_CONFIGURING ? "配置中" :
                       match->state() == Match::State::NOT_STARTED ? "未开始" : "已开始") << "\n";
  sender << "房间号：";
  if (match->gid().has_value()) { sender << *gid << "\n"; }
  else { sender << "私密游戏" << "\n"; }
  sender << "可参加人数：" << match->game_handle().min_player_;
  if (const uint64_t max_player = match->game_handle().max_player_; max_player > match->game_handle().min_player_) { sender << "~" << max_player; }
  else if (max_player == 0) { sender << "+"; }
  sender << "人" << "\n";
  sender << "房主：" << match->host_uid() << "\n";
  const std::set<uint64_t>& ready_uid_set = match->ready_uid_set();
  sender << "已参加玩家：" << ready_uid_set.size() << "人";
  for (const uint64_t uid : ready_uid_set) { sender << "\n" << uid; }
  return EC_OK;
}

static ErrCode show_rule(const UserID uid, const std::optional<GroupID> gid, const replier_t reply, const std::string& gamename)
{
  const auto it = g_game_handles.find(gamename);
  if (it == g_game_handles.end())
  {
    reply() << "[错误] 查看失败：未知的游戏名";
    return EC_REQUEST_UNKNOWN_GAME;
  };
  auto sender = reply();
  sender << "可参加人数：" << it->second->min_player_;
  if (const uint64_t max_player = it->second->max_player_; max_player > it->second->min_player_) { sender << "~" << max_player; }
  else if (max_player == 0) { sender << "+"; }
  sender << "人\n";
  sender << "详细规则：\n";
  sender << it->second->rule_;
  return EC_OK;
}

static ErrCode show_profile(const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
{
  const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager();
  if (db_manager == nullptr)
  {
    reply() << "[错误] 查看失败：未连接数据库";
    return EC_DB_NOT_CONNECTED;
  };
  db_manager->GetUserProfit(uid); // TODO: pass sender
  return EC_OK;
}

const std::vector<std::shared_ptr<MetaCommand>> meta_cmds =
{
  make_command("查看帮助", [](const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
      { return help(uid, gid, reply, meta_cmds, "元"); }, VoidChecker("#帮助")),
  make_command("查看个人信息", show_profile, VoidChecker("#个人信息")),
  make_command("查看游戏列表", show_gamelist, VoidChecker("#游戏列表")),
  make_command("查看游戏规则", show_rule, VoidChecker("#规则"), AnyArg("游戏名称", "某游戏名")),
  make_command("查看当前所有未开始的私密比赛", show_private_matches, VoidChecker("#私密游戏列表")),
  make_command("查看已加入，或该房间正在进行的比赛信息", show_match_status, VoidChecker("#游戏信息")),

  make_command("在当前房间建立公开游戏，或私信bot以建立私密游戏", new_game<true>, VoidChecker("#新游戏"), AnyArg("游戏名称", "某游戏名")),
  make_command("在当前房间建立公开游戏，或私信bot以建立私密游戏，并进行游戏参数的配置", new_game<false>, VoidChecker("#配置新游戏"), AnyArg("游戏名称", "某游戏名")),
  make_command("完成游戏参数配置后，允许玩家进入房间", config_over, VoidChecker("#配置完成")),
  make_command("房主开始游戏", start_game, VoidChecker("#开始游戏")),
  make_command("加入当前房间的公开游戏", join_public, VoidChecker("#加入游戏")),
  make_command("私信bot以加入私密游戏", join_private, VoidChecker("#加入游戏"), BasicChecker<MatchId>("私密比赛编号")),
  make_command("在游戏开始前退出游戏", leave, VoidChecker("#退出游戏")),
};

static ErrCode release_game(const UserID uid, const std::optional<GroupID> gid, const replier_t reply, const std::string& gamename)
{
  const auto it = g_game_handles.find(gamename); 
  if (it == g_game_handles.end())
  {
    reply() << "[错误] 发布失败：未知的游戏名";
    return EC_REQUEST_UNKNOWN_GAME;
  }
  std::optional<uint64_t> game_id = it->second->game_id_.load(); 
  if (game_id.has_value())
  {
    reply() << "[错误] 发布失败：游戏已发布，ID为" << *game_id;
    return EC_GAME_ALREADY_RELEASE;
  }
  const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager(); 
  if (db_manager == nullptr)
  {
    reply() << "[错误] 发布失败：未连接数据库";
    return EC_DB_NOT_CONNECTED;
  };
  if (!(game_id = db_manager->ReleaseGame(gamename)).has_value())
  {
    reply() << "[错误] 发布失败：发布失败，请查看错误日志";
    return EC_DB_RELEASE_GAME_FAILED;
  };
  it->second->game_id_.store(game_id);
  reply() << "发布成功，游戏\'" << gamename << "\'的ID为" << *game_id;
  return EC_OK;
}

static ErrCode interrupt_match(const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
{
  if (!gid.has_value())
  {
    reply() << "[错误] 游戏失败：需要在房间中使用该指令";
    return EC_MATCH_NEED_REQUEST_PUBLIC;
  };
  const auto match = MatchManager::GetMatchWithGroupID(*gid);
  if (!match)
  {
    reply() << "[错误] 游戏失败：该房间未进行游戏";
    return EC_MATCH_GROUP_NOT_IN_MATCH;
  };
  return MatchManager::DeleteMatch(match->mid());
}

const std::vector<std::shared_ptr<MetaCommand>> admin_cmds =
{
  make_command("查看帮助", [](const UserID uid, const std::optional<GroupID> gid, const replier_t reply)
      { return help(uid, gid, reply, admin_cmds, "管理"); }, VoidChecker("%帮助")),
  make_command("发布游戏，写入游戏信息到数据库", release_game, VoidChecker("%发布游戏"), AnyArg("游戏名称", "某游戏名")),
  make_command("强制中断公开比赛", interrupt_match, VoidChecker("%中断游戏"))
};

