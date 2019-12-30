#include "stdafx.h"
#include "lgtbot.h"
#include "message_iterator.h"
#include "message_handlers.h"
#include "../new-rock-paper-scissors/dllmain.h"

#include <list>
#include <sstream>
#include <memory>

#define CQ_CODE_LEN 256

char cq_code_buf[CQ_CODE_LEN];

#define RETURN_IF_FAILED(str) \
do\
{\
  if (const std::string& err = (str); !err.empty())\
    return err;\
} while (0);

MatchManager match_manager;

QQ LGTBOT::this_qq = INVALID_QQ;
AT_CALLBACK LGTBOT::at_cq = nullptr;
MSG_CALLBACK LGTBOT::send_private_msg_callback = nullptr;
MSG_CALLBACK LGTBOT::send_group_msg_callback = nullptr;
MSG_CALLBACK LGTBOT::send_discuss_msg_callback = nullptr;

std::string LGTBOT::at_str(const QQ& qq)
{
  at_cq(qq, cq_code_buf, CQ_CODE_LEN);
  return std::string(cq_code_buf);
}

void LGTBOT::set_this_qq(const QQ& qq)
{
  this_qq = qq;
}

void LGTBOT::set_at_cq_callback(AT_CALLBACK fun)
{
  at_cq = fun;
}

void LGTBOT::set_send_private_msg_callback(MSG_CALLBACK fun)
{
  send_private_msg_callback = fun;
}

void LGTBOT::set_send_group_msg_callback(MSG_CALLBACK fun)
{
  send_group_msg_callback = fun;
}

void LGTBOT::set_send_discuss_msg_callback(MSG_CALLBACK fun)
{
  send_discuss_msg_callback = fun;
}

bool LGTBOT::is_at(const std::string& msg)
{
  return msg.find(at_str(this_qq)) != std::string::npos;
}

void LGTBOT::send_private_msg(const QQ& usr_qq, const std::string& msg)
{
  send_private_msg_callback(usr_qq, msg.c_str());
}

void LGTBOT::send_group_msg(const QQ& group_qq, const std::string& msg)
{
  send_group_msg_callback(group_qq, msg.c_str());
}

void LGTBOT::send_discuss_msg(const QQ& discuss_qq, const std::string& msg)
{
  send_discuss_msg_callback(discuss_qq, msg.c_str());
}

void LGTBOT::send_group_msg(const QQ& group_qq, const std::string& msg, const QQ& to_usr_qq)
{
  send_group_msg(group_qq, at_str(to_usr_qq) + msg);
}

void LGTBOT::send_discuss_msg(const QQ& discuss_qq, const std::string& msg, const QQ& to_usr_qq)
{
  send_discuss_msg(discuss_qq, at_str(to_usr_qq) + msg);
}

std::vector<GameHandle> getPlugins() {
  std::vector<GameHandle> ret;

  // 在plugins目录中查找dll文件并将文件信息保存在fileData中
  WIN32_FIND_DATA fileData;
  HANDLE fileHandle = FindFirstFile(L"plugins/*.dll", &fileData);

  if (fileHandle == (void*)ERROR_INVALID_HANDLE ||
    fileHandle == (void*)ERROR_FILE_NOT_FOUND) {
    // 没有找到任何dll文件，返回空vector
    return {};
  }

  // 循环加载plugins目录中的所有dll文件
  do {
    // 将dll加载到当前进程的地址空间中
    std::wstring dll_path = L"./plugins/" + std::wstring(fileData.cFileName);
    HINSTANCE mod = LoadLibrary(dll_path.c_str());

    if (!mod)
    {
      /* TODO: LOG(dll_path + "load failed"); */
      continue;
    }

    // 从dll句柄中获取getObj和getName的函数地址
    typedef int (__cdecl *Init)(const boardcast, const tell, const at, const game_over);
    typedef char* (__cdecl *GameInfo)(uint64_t* const, uint64_t* const);
    typedef Game* (__cdecl *NewGame)(const uint64_t);
    typedef int (__cdecl *ReleaseGame)(Game* const);

    Init init = (Init) GetProcAddress(mod, "Init");
    GameInfo game_info = (GameInfo) GetProcAddress(mod, "GameInfo");
    NewGame new_game = (NewGame) GetProcAddress(mod, "NewGame");
    ReleaseGame release_game = (ReleaseGame) GetProcAddress(mod, "GetProcAddress");

    if (!init || !game_info || !new_game || !release_game)
    {
      //LOG("Invalid Plugin DLL: some interface not be defined.");
      continue;
    }

    uint64_t min_player = 0;
    uint64_t max_player = 0;
    char* name = game_info(&min_player, &max_player);
    if (!name)
    {
      //LOG("Cannot get game game");
      continue;
    }
    if (min_player == 0 || max_player < min_player)
    {
      //LOG("Invalid" min_player, max_player)
      continue;
    }
    
    ret.emplace_back(name, min_player, max_player, new_game, release_game, mod);

    //LOG(name" loaded!\n");
  } while (FindNextFile(fileHandle, &fileData));

  std::clog << std::endl;

  // 关闭文件句柄
  FindClose(fileHandle);
  return ret;
}

void LGTBOT::LoadGames()
{
  g_games_.clear();
  /* TODO: load games from dlls */
}

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

bool LGTBOT::Request(const MessageType& msg_type, const QQ& src_qq, const QQ& usr_qq, char* msg_c)
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
