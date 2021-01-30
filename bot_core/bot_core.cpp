#include "message_handlers.h"
#include "bot_core.h"
#include "log.h"
#include "match.h"
#include "db_manager.h"
#include "util.h"
#include "load_game_modules.h"
#include "utility/msg_sender.h"
#include "game_framework/game_main.h"
#include "utility/msg_checker.h"
#include <gflags/gflags.h>
#include <fstream>

const int32_t LGT_AC = -1;
const UserID INVALID_USER_ID = 0;
const GroupID INVALID_GROUP_ID = 0;

std::mutex g_mutex;
std::map<std::string, std::unique_ptr<GameHandle>> g_game_handles;
std::set<UserID> g_admins;

NEW_MSG_SENDER_CALLBACK g_new_msg_sender_cb = nullptr;
DELETE_MSG_SENDER_CALLBACK g_delete_msg_sender_cb = nullptr;
UserID g_this_uid = INVALID_USER_ID;

MatchManager match_manager;

static void LoadAdmins()
{
  std::ifstream admin_file(".\\admins.txt");
  if (admin_file)
  {
    for (uint64_t admin_uid; admin_file >> admin_uid; )
    {
      InfoLog() << "管理员：" << admin_uid;
      g_admins.emplace(admin_uid);
    }
  }
}

template <UniRef<MsgSenderWrapper> SenderRef>
static ErrCode HandleRequest(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, SenderRef&& sender)
{
  if (std::string first_arg; !(std::stringstream(msg) >> first_arg) || first_arg.empty())
  {
    sender << "[错误] 我不理解，所以你是想表达什么？";
    return EC_REQUEST_EMPTY;
  }
  else
  {
    switch (first_arg[0])
    {
    case '#':
      return HandleMetaRequest(uid, gid, MsgReader(msg), sender);
    case '%':
      if (g_admins.find(uid) == g_admins.end())
      {
        sender << "[错误] 您未持有管理员权限";
        return EC_REQUEST_NOT_ADMIN;
      }
      return HandleAdminRequest(uid, gid, MsgReader(msg), sender);
    default:
      std::shared_ptr<Match> match = MatchManager::GetMatch(uid, gid);
      if (!match)
      {
        sender << "[错误] 您未参与游戏";
        return EC_MATCH_USER_NOT_IN_MATCH;
      }
      if (match->gid() != gid && gid.has_value())
      {
        sender << "[错误] 您未在本群参与游戏";
        return EC_MATCH_NOT_THIS_GROUP;
      }
      return match->Request(uid, gid, msg, &sender);
    }
  }
}

bool /*__cdecl*/ BOT_API::Init(
    const UserID this_uid,
    const NEW_MSG_SENDER_CALLBACK new_msg_sender_cb,
    const DELETE_MSG_SENDER_CALLBACK delete_msg_sender_cb,
    int argc, char** argv)
{
  if (this_uid == INVALID_USER_ID || !new_msg_sender_cb || !delete_msg_sender_cb) { return false; }
  g_this_uid = this_uid;
  g_new_msg_sender_cb = new_msg_sender_cb;
  g_delete_msg_sender_cb = delete_msg_sender_cb;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  ::google::InitGoogleLogging(argv[0]);
  LoadGameModules();
  LoadAdmins();
  return true;
}

ErrCode /*__cdecl*/ BOT_API::HandlePrivateRequest(const UserID uid, const char* const msg)
{
  return HandleRequest(uid, {}, msg, ToUser(uid));
}

ErrCode /*__cdecl*/ BOT_API::HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg)
{
  return HandleRequest(uid, gid, msg, ToGroup(gid) << AtMsg(uid) << "\n");
}

ErrCode /*__cdecl*/ BOT_API::ConnectDatabase(const char* const addr, const char* const user, const char* const passwd, const char* const db_name, const char** errmsg)
{
  ErrCode code;
  std::tie(code, *errmsg) = DBManager::ConnectDB(addr, user, passwd, db_name);
  return code;
}
