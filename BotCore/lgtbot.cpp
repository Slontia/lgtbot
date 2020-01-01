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

bool __cdecl HandlePrivateRequest(const UserID uid, const char* const msg)
{

}

bool __cdecl HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg)
{

}

static bool IsAtMe(const std::string& msg)
{
  return msg.find(At(g_this_uid)) != std::string::npos;
}

static std::optional<GameHandle> LoadGame(HINSTANCE mod)
{
  if (!mod)
  {
    /* TODO: LOG(dll_path + "load failed"); */
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
    LOG_INFO("Invalid Plugin DLL: some interface not be defined.");
    return {};
  }
  uint64_t min_player = 0;
  uint64_t max_player = 0;
  char* name = game_info(&min_player, &max_player);
  if (!name)
  {
    LOG_INFO("Cannot get game game");
    return {};
  }
  if (min_player == 0 || max_player < min_player)
  {
    LOG_INFO("Invalid" + std::to_string(min_player) + std::to_string(max_player));
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

typedef std::shared_ptr<MsgCommand<void(const UserID, const std::optional<GroupID>)>> MetaCommand;

template <typename ...Args>
MetaCommand MakeMetaCommand(Args... args)
{
  return std::make_shared<void(const UserID, const std::optional<GroupID>)>(args...);
}

std::vector<MetaCommand> cmds =
{
  MakeMetaCommand(show_gamelist, std::make_unique<VoidChecker>("游戏列表")),
  MakeMetaCommand(new_game, std::make_unique<VoidChecker>("新游戏"), std::make_unique<AnyArg>("游戏名称", "某游戏名"), std::make_unique<BoolChecker>("公开", "私密")),
  MakeMetaCommand(start_game, std::make_unique<VoidChecker>("开始游戏")),
  MakeMetaCommand(leave, std::make_unique<VoidChecker>("退出游戏"))
};

/* msg is not empty */
void global_request(MessageIterator& msg)
{
  assert(msg.has_next());
  const std::string front_msg = msg.get_next();
  if (front_msg == "游戏大厅") 
  {

  }
  else if (front_msg == "游戏列表")
  {
    show_gamelist(msg);
  }
  else if (front_msg == "规则")
  {

  }
  else if (front_msg == "新游戏") // 新游戏 <游戏名> (公开|私密)
  {
    new_game(msg);
  }
  else if (front_msg == "帮助")
  {

  }
  else if (front_msg == "开始") // host only
  {
    start_game(msg);
  }
  else if (front_msg == "加入") // 加入 [比赛编号]
  {
    join(msg);
  }
  else if (front_msg == "退出")
  {
    leave(msg);
  }
  else
  {
    msg.Reply("你说你马呢");
  }
}

bool Request(const MessageType& msg_type, const QQ& src_qq, const QQ& usr_qq, char* msg_c)
{
  std::string msg(msg_c);
  if (msg.empty()) return false;

  bool is_global = false;
  size_t last_pos = 0, pos = 0;

  /* If it is a global request, we should move char pointer behind '#'. */
  if (msg[0] == '#')
  {
    last_pos = 1;
    is_global = true;
  }

  std::vector<std::string> substrs;
  while (true)
  {
    /* Read next word. */
    pos = msg.find(" ", last_pos);
    if (last_pos != pos) substrs.push_back(msg.substr(last_pos, pos - last_pos));

    /* If finish reading, break. */
    if (pos == std::string::npos) break;
    last_pos = pos + 1;
  }
  if (substrs.empty()) return true;

  /* Make Message Iterator. */
  std::shared_ptr<MessageIterator> msgit_p =
    (msg_type == PRIVATE_MSG) ? std::dynamic_pointer_cast<MessageIterator>(std::make_shared<PrivateMessageIterator>(usr_qq, std::move(substrs))) :
    (msg_type == GROUP_MSG) ? std::dynamic_pointer_cast<MessageIterator>(std::make_shared<GroupMessageIterator>(usr_qq, std::move(substrs), src_qq)) :
    (msg_type == DISCUSS_MSG) ? std::dynamic_pointer_cast<MessageIterator>(std::make_shared<DiscussMessageIterator>(usr_qq, std::move(substrs), src_qq)) :
    nullptr;
  assert(msgit_p != nullptr);
  MessageIterator& msgit = *msgit_p;

  mutex.lock();

  /* Distribute msg to global handle or match handle. */
  if (is_global) { global_request(msgit); }
  else { match_manager.Request(msgit); }
    
  mutex.unlock();

  return true;
}
