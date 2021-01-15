#include "message_handlers.h"
#include "bot_core.h"
#include "log.h"
#include "match.h"
#include "db_manager.h"
#include "util.h"
#include "load_game_modules.h"
#include "GameFramework/dllmain.h"
#include "Utility/msg_checker.h"
#include <gflags/gflags.h>
#include <fstream>

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

bool /*__cdecl*/ BOT_API::Init(const UserID this_uid, const PRIVATE_MSG_CALLBACK pri_msg_cb, const PUBLIC_MSG_CALLBACK pub_msg_cb, const AT_CALLBACK at_cb, int argc, char** argv)
{
  if (this_uid == INVALID_USER_ID || !pri_msg_cb || !pub_msg_cb || !at_cb) { return false; }
  std::locale::global(std::locale(""));
  g_this_uid = this_uid;
  g_send_pri_msg_cb = pri_msg_cb;
  g_send_pub_msg_cb = pub_msg_cb;
  g_at_cb = at_cb;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  ::google::InitGoogleLogging(argv[0]);
  LoadGameModules();
  LoadAdmins();
  return true;
}

void /*__cdecl*/ BOT_API::HandlePrivateRequest(const UserID uid, const char* const msg)
{
  if (const std::string reply_msg = HandleRequest(uid, {}, msg); !reply_msg.empty()) { SendPrivateMsg(uid, reply_msg); }
}

void /*__cdecl*/ BOT_API::HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg)
{
  if (const std::string reply_msg = HandleRequest(uid, gid, msg); !reply_msg.empty()) { SendPublicMsg(gid, At(uid) + "\n" + reply_msg); };
}

ErrCode /*__cdecl*/ BOT_API::ConnectDatabase(const char* const addr, const char* const user, const char* const passwd, const char* const db_name, const char** errmsg)
{
  ErrCode code;
  std::tie(code, *errmsg) = DBManager::ConnectDB(addr, user, passwd, db_name);
  return code;
}
