#include "stdafx.h"
#include "lgtbot.h"
#include "message_iterator.h"
#include "message_handlers.h"
#include "../new-rock-paper-scissors/dllmain.h"
#include "../new-rock-paper-scissors/msg_checker.h"
#include "dllmain.h"
#include "log.h"

#include <list>
#include <sstream>
#include <memory>
#include <optional>

MatchManager match_manager;

bool __cdecl Init(const UserID this_uid, const PRIVATE_MSG_CALLBACK pri_msg_cb, const PUBLIC_MSG_CALLBACK pub_msg_cb, const AT_CALLBACK at_cb)
{
  if (this_uid == INVALID_USER_ID || !pri_msg_cb || !pub_msg_cb || !at_cb) { return false; }
  g_this_uid = this_uid;
  g_send_pri_msg_cb = pri_msg_cb;
  g_send_pub_msg_cb = pub_msg_cb;
  g_at_cb = at_cb;
  LoadGameModules();
  return true;
}

void __cdecl HandlePrivateRequest(const UserID uid, const char* const msg)
{
  SendPrivateMsg(uid, HandleRequest(uid, {}, msg));
}

void __cdecl HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg)
{
  SendPublicMsg(gid, At(uid) + HandleRequest(uid, gid, msg));
}

static bool IsAtMe(const std::string& msg)
{
  return msg.find(At(g_this_uid)) != std::string::npos;
}

static std::optional<GameHandle> LoadGame(HINSTANCE mod)
{
  if (!mod)
  {
    LOG_ERROR("Load mod failed");
    return {};
  }

  typedef int (__cdecl *Init)(const boardcast, const tell, const at, const game_over);
  typedef char* (__cdecl *GameInfo)(uint64_t* const, uint64_t* const);
  typedef GameBase* (__cdecl *NewGame)(const uint64_t);
  typedef int (__cdecl *ReleaseGame)(GameBase* const);

  Init init = (Init)GetProcAddress(mod, "Init");
  GameInfo game_info = (GameInfo)GetProcAddress(mod, "GameInfo");
  NewGame new_game = (NewGame)GetProcAddress(mod, "NewGame");
  ReleaseGame release_game = (ReleaseGame)GetProcAddress(mod, "GetProcAddress");

  if (!init || !game_info || !new_game || !release_game)
  {
    LOG_ERROR("Invalid Plugin DLL: some interface not be defined.");
    return {};
  }
  uint64_t min_player = 0;
  uint64_t max_player = 0;
  char* name = game_info(&min_player, &max_player);
  if (!name)
  {
    LOG_ERROR("Cannot get game game");
    return {};
  }
  if (min_player == 0 || max_player < min_player)
  {
    LOG_ERROR("Invalid" + std::to_string(min_player) + std::to_string(max_player));
    return {};
  }
  return GameHandle(name, min_player, max_player, new_game, release_game, mod);
}

static void LoadGameModules()
{
  WIN32_FIND_DATA file_data;
  HANDLE file_handle = FindFirstFile(L"plugins/*.dll", &file_data);
  if (file_handle == (void*)ERROR_INVALID_HANDLE || file_handle == (void*)ERROR_FILE_NOT_FOUND) { return; }
  do {
    std::wstring dll_path = L"./plugins/" + std::wstring(file_data.cFileName);
    std::optional<GameHandle> game_handle = LoadGame(LoadLibrary(dll_path.c_str()));
    if (game_handle.has_value())
    {
      LOG_ERROR(std::string(dll_path.begin(), dll_path.end()) + " loaded failed!\n");
    }
    else
    {
      LOG_INFO(std::string(dll_path.begin(), dll_path.end()) + " loaded success!\n");
      g_game_handles.emplace(game_handle->name_, *game_handle);
    }    
  } while (FindNextFile(file_handle, &file_data));
  FindClose(file_handle);
}

static std::string HandleMetaRequest(const UserID uid, const std::optional<GroupID> gid, MsgReader& reader)
{
  typedef MsgCommand<std::string(const UserID, const std::optional<GroupID>)> MetaCommand;
  static const auto make_meta_command = [](auto... args) -> std::shared_ptr<MetaCommand> { return MakeCommand<std::string(const UserID, const std::optional<GroupID>)>(args...);  };
  static const std::vector<std::shared_ptr<MetaCommand>> meta_cmds =
  {
    make_meta_command(show_gamelist, std::make_unique<VoidChecker>("游戏列表")),
    make_meta_command(new_game, std::make_unique<VoidChecker>("新游戏"), std::make_unique<AnyArg>("游戏名称", "某游戏名"), std::make_unique<BoolChecker>("公开", "私密")),
    make_meta_command(start_game, std::make_unique<VoidChecker>("开始游戏")),
    make_meta_command(leave, std::make_unique<VoidChecker>("退出比赛")),
    make_meta_command(join, std::make_unique<VoidChecker>("加入比赛"), std::make_unique<BasicChecker<MatchId, true>>("赛事编号")),
  };
  reader.Reset();
  for (const std::shared_ptr<MetaCommand>& cmd : meta_cmds)
  {
    if (std::optional<std::string> reply_msg = cmd->CallIfValid(reader, std::tuple{ uid, gid })) { return *reply_msg; }
  }
  return "[错误] 未预料的元请求";
}

static std::string HandleRequest(const UserID uid, const std::optional<GroupID> gid, const std::string& msg)
{
  if (msg.empty()) { return; }
  std::lock_guard<std::mutex> lock(g_mutex);
  return (msg[0] == '#') ? HandleMetaRequest(uid, gid, MsgReader(msg.substr(1, msg.size()))) : match_manager.Request(uid, gid, MsgReader(msg));
}
