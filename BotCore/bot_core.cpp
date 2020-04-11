#include "stdafx.h"
#include "message_handlers.h"
#include "msg_checker.h"
#include "bot_core.h"
#include "log.h"
#include "match.h"
#include "db_manager.h"
#include "../GameFrameWork/dllmain.h"

const int32_t LGT_AC = -1;
const UserID INVALID_USER_ID = 0;
const GroupID INVALID_GROUP_ID = 0;

std::mutex g_mutex;
std::map<std::string, std::unique_ptr<GameHandle>> g_game_handles;
std::set<UserID> g_admins;

AT_CALLBACK g_at_cb = nullptr;
PRIVATE_MSG_CALLBACK g_send_pri_msg_cb = nullptr;
PUBLIC_MSG_CALLBACK g_send_pub_msg_cb = nullptr;
UserID g_this_uid = INVALID_USER_ID;

MatchManager match_manager;

static void BoardcastPlayers(void* match, const char* const msg)
{
  static_cast<Match*>(match)->BoardcastPlayers(msg);
}

static void TellPlayer(void* match, const uint64_t pid, const char* const msg)
{
  static_cast<Match*>(match)->TellPlayer(pid, msg);
}

static void AtPlayer(void* match, const uint64_t pid, char* buf, const uint64_t len)
{
  static_cast<Match*>(match)->AtPlayer(pid, buf, len);
}

static void MatchGameOver(void* match, const int64_t scores[])
{
  static_cast<Match*>(match)->GameOver(scores);
  MatchManager::DeleteMatch(static_cast<Match*>(match)->mid());
}

static void StartTimer(void* match, const uint64_t sec)
{
  static_cast<Match*>(match)->StartTimer(sec);
}

static void StopTimer(void* match)
{
  static_cast<Match*>(match)->StopTimer();
}

static std::unique_ptr<GameHandle> LoadGame(HINSTANCE mod)
{
  if (!mod)
  {
    LOG_ERROR("Load mod failed");
    return nullptr;
  }

  typedef int (__cdecl *Init)(const boardcast, const tell, const at, const game_over, const start_timer, const stop_timer);
  typedef char* (__cdecl *GameInfo)(uint64_t*, uint64_t*, const char**);
  typedef GameBase* (__cdecl *NewGame)(void* const match, const uint64_t);
  typedef int (__cdecl *DeleteGame)(GameBase* const);

  Init init = (Init)GetProcAddress(mod, "Init");
  GameInfo game_info = (GameInfo)GetProcAddress(mod, "GameInfo");
  NewGame new_game = (NewGame)GetProcAddress(mod, "NewGame");
  DeleteGame delete_game = (DeleteGame)GetProcAddress(mod, "DeleteGame");

  if (!init || !game_info || !new_game || !delete_game)
  {
    LOG_ERROR("Invalid Plugin DLL: some interface not be defined." + std::to_string((bool)new_game));
    return nullptr;
  }

  if (!init(&BoardcastPlayers, &TellPlayer, &AtPlayer, &MatchGameOver, &StartTimer, &StopTimer))
  {
    LOG_ERROR("Init failed");
    return nullptr;
  }

  const char* rule = nullptr;
  uint64_t min_player = 0;
  uint64_t max_player = 0;
  char* name = game_info(&min_player, &max_player, &rule);
  if (!name)
  {
    LOG_ERROR("Cannot get game game");
    return nullptr;
  }
  if (min_player == 0 || max_player < min_player)
  {
    LOG_ERROR("Invalid" + std::to_string(min_player) + std::to_string(max_player));
    return nullptr;
  }
  std::optional<uint64_t> game_id;
  if (const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager(); db_manager != nullptr)
  {
    game_id = db_manager->GetGameIDWithName(name);
  }
  return std::make_unique<GameHandle>(game_id, name, min_player, max_player, rule, new_game, delete_game, mod);
}

static void LoadGameModules()
{
  WIN32_FIND_DATA file_data;
  HANDLE file_handle = FindFirstFile(L".\\plugins\\*.dll", &file_data);
  if (file_handle == INVALID_HANDLE_VALUE) { return; }
  do {
    std::wstring dll_path = L".\\plugins\\" + std::wstring(file_data.cFileName);
    std::unique_ptr<GameHandle> game_handle = LoadGame(LoadLibrary(dll_path.c_str()));
    if (!game_handle)
    {
      LOG_ERROR(std::string(dll_path.begin(), dll_path.end()) + " loaded failed!\n");
    }
    else
    {
      g_game_handles.emplace(game_handle->name_, std::move(game_handle));
      LOG_INFO(std::string(dll_path.begin(), dll_path.end()) + " loaded success!\n");
    }    
  } while (FindNextFile(file_handle, &file_data));
  FindClose(file_handle);
  LOG_INFO("共加载游戏个数：" + std::to_string(g_game_handles.size()));
}

static void LoadAdmins()
{
  std::ifstream admin_file(".\\admins.txt");
  if (admin_file)
  {
    for (uint64_t admin_uid; admin_file >> admin_uid; )
    {
      g_admins.emplace(admin_uid);
    }
  }
}

static std::string HandleRequest(const UserID uid, const std::optional<GroupID> gid, const std::string& msg)
{
  if (std::string first_arg; !(std::stringstream(msg) >> first_arg) || first_arg.empty()) { return "[错误] 空白消息"; }
  else
  {
    switch (first_arg[0])
    {
    case '#': return HandleMetaRequest(uid, gid, MsgReader(msg));
    case '%':
      if (g_admins.find(uid) == g_admins.end()) { return "[错误] 您未持有管理员权限"; }
      return HandleAdminRequest(uid, gid, MsgReader(msg));
    default:
      std::shared_ptr<Match> match = MatchManager::GetMatch(uid, gid);
      if (!match) { return "[错误] 您未参与游戏"; }
      if (match->gid() != gid && gid.has_value()) { return "[错误] 您未在本群参与游戏"; }
      return match->Request(uid, gid, msg);
    }
  }
}

bool __cdecl BOT_API::Init(const UserID this_uid, const PRIVATE_MSG_CALLBACK pri_msg_cb, const PUBLIC_MSG_CALLBACK pub_msg_cb, const AT_CALLBACK at_cb)
{
  if (this_uid == INVALID_USER_ID || !pri_msg_cb || !pub_msg_cb || !at_cb) { return false; }
  g_this_uid = this_uid;
  g_send_pri_msg_cb = pri_msg_cb;
  g_send_pub_msg_cb = pub_msg_cb;
  g_at_cb = at_cb;
  LoadGameModules();
  LoadAdmins();
  return true;
}

void __cdecl BOT_API::HandlePrivateRequest(const UserID uid, const char* const msg)
{
  if (const std::string reply_msg = HandleRequest(uid, {}, msg); !reply_msg.empty()) { SendPrivateMsg(uid, reply_msg); }
}

void __cdecl BOT_API::HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg)
{
  if (const std::string reply_msg = HandleRequest(uid, gid, msg); !reply_msg.empty()) { SendPublicMsg(gid, At(uid) + "\n" + reply_msg); };
}

ErrCode __cdecl BOT_API::ConnectDatabase(const char* const addr, const char* const user, const char* const passwd, const char* const db_name, const bool create_if_not_found, const char** errmsg)
{
  ErrCode code;
  std::tie(code, *errmsg) = DBManager::ConnectDB(addr, user, passwd, db_name, create_if_not_found);
  return code;
}
