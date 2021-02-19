#include "message_handlers.h"
#include "bot_core.h"
#include "log.h"
#include "match.h"
#include "db_manager.h"
#include "util.h"
#include "utility/msg_sender.h"
#include "game_framework/game_main.h"
#include "utility/msg_checker.h"
#include <fstream>

extern void LoadGameModules();

const int32_t LGT_AC = -1;
const UserID INVALID_USER_ID = 0;
const GroupID INVALID_GROUP_ID = 0;

void BotCtx::LoadAdmins_(const uint64_t* const admins, const uint64_t admin_count)
{
  if (admins == nullptr) { return; }
  for (uint64_t i = 0; i < admin_count; ++ i)
  {
    InfoLog() << "New administor: " << admins[i];
    admins_.emplace(admins[i]);
  }
}

template <typename Reply>
static ErrCode HandleRequest(BotCtx& bot, const std::optional<GroupID> gid, const UserID uid, const std::string& msg, Reply&& reply)
{
  if (std::string first_arg; !(std::stringstream(msg) >> first_arg) || first_arg.empty())
  {
    reply() << "[错误] 我不理解，所以你是想表达什么？";
    return EC_REQUEST_EMPTY;
  }
  else
  {
    switch (first_arg[0])
    {
    case '#':
      return HandleMetaRequest(bot, uid, gid, MsgReader(msg), reply);
    case '%':
      if (!bot.HasAdmin(uid))
      {
        reply() << "[错误] 您未持有管理员权限";
        return EC_REQUEST_NOT_ADMIN;
      }
      return HandleAdminRequest(bot, uid, gid, MsgReader(msg), reply);
    default:
      std::shared_ptr<Match> match = bot.match_manager().GetMatch(uid, gid);
      if (!match)
      {
        reply() << "[错误] 您未参与游戏";
        return EC_MATCH_USER_NOT_IN_MATCH;
      }
      if (match->gid() != gid && gid.has_value())
      {
        reply() << "[错误] 您未在本群参与游戏";
        return EC_MATCH_NOT_THIS_GROUP;
      }
      return match->Request(uid, gid, msg, reply);
    }
  }
}

void* /*__cdecl*/ BOT_API::Init(
    const UserID this_uid,
    const NEW_MSG_SENDER_CALLBACK new_msg_sender_cb,
    const DELETE_MSG_SENDER_CALLBACK delete_msg_sender_cb,
    const char* const game_path,
    const uint64_t* const admins,
    const uint64_t admin_count)
{
  if (this_uid == INVALID_USER_ID || !new_msg_sender_cb || !delete_msg_sender_cb || !game_path)
  {
    return nullptr;
  }
  return new BotCtx(this_uid, new_msg_sender_cb, delete_msg_sender_cb, game_path, admins, admin_count);
}

void /*__cdelcl*/ BOT_API::Release(void* const bot) { delete static_cast<BotCtx*>(bot); }

ErrCode /*__cdecl*/ BOT_API::HandlePrivateRequest(void* const bot_p, const UserID uid, const char* const msg)
{
  if (!bot_p) { return EC_NOT_INIT; }
  BotCtx& bot = *static_cast<BotCtx*>(bot_p);
  return HandleRequest(bot, {}, uid, msg, [uid, &bot] { return bot.ToUser(uid); });
}

ErrCode /*__cdecl*/ BOT_API::HandlePublicRequest(void* const bot_p, const GroupID gid, const UserID uid, const char* const msg)
{
  if (!bot_p) { return EC_NOT_INIT; }
  BotCtx& bot = *static_cast<BotCtx*>(bot_p);
  return HandleRequest(bot, gid, uid, msg,
      [uid, gid, &bot] {
        auto sender = bot.ToGroup(gid);
        sender << AtMsg(uid) << "\n";
        return sender;
      });
}

ErrCode /*__cdecl*/ BOT_API::ConnectDatabase(void* const bot_p, const char* const addr, const char* const user, const char* const passwd, const char* const db_name, const char** errmsg)
{
  ErrCode code;
  std::tie(code, *errmsg) = DBManager::ConnectDB(addr, user, passwd, db_name);
  if (const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager())
  {
    for (auto& [name, info] : static_cast<BotCtx*>(bot_p)->game_handles())
    {
      info->game_id_ = db_manager->GetGameIDWithName(name);
    }
  }
  return code;
}
