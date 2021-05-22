#include "bot_core.h"

#include <fstream>

#include "db_manager.h"
#include "game_framework/game_main.h"
#include "log.h"
#include "match.h"
#include "message_handlers.h"
#include "util.h"
#include "utility/msg_checker.h"
#include "utility/msg_sender.h"

extern void LoadGameModules();

const int32_t LGT_AC = -1;

void BotCtx::LoadAdmins_(const uint64_t* const admins, const uint64_t admin_count)
{
    if (admins == nullptr) {
        return;
    }
    for (uint64_t i = 0; i < admin_count; ++i) {
        InfoLog() << "New administor: " << admins[i];
        admins_.emplace(admins[i]);
    }
}

template <typename Reply>
static ErrCode HandleRequest(BotCtx& bot, const std::optional<GroupID> gid, const UserID uid, const std::string& msg,
                             Reply&& reply)
{
    if (std::string first_arg; !(std::stringstream(msg) >> first_arg) || first_arg.empty()) {
        reply() << "[错误] 我不理解，所以你是想表达什么？";
        return EC_REQUEST_EMPTY;
    } else {
        switch (first_arg[0]) {
        case '#':
            return HandleMetaRequest(bot, uid, gid, MsgReader(msg), reply);
        case '%':
            if (!bot.HasAdmin(uid)) {
                reply() << "[错误] 您未持有管理员权限";
                return EC_REQUEST_NOT_ADMIN;
            }
            return HandleAdminRequest(bot, uid, gid, MsgReader(msg), reply);
        default:
            std::shared_ptr<Match> match = bot.match_manager().GetMatch(uid, gid);
            if (!match) {
                reply() << "[错误] 您未参与游戏\n"
                           "若您想执行元指令，请尝试在请求前加\"#\"，或通过\"#帮助\"查看所有支持的元指令";
                return EC_MATCH_USER_NOT_IN_MATCH;
            }
            if (match->gid() != gid && gid.has_value()) {
                reply() << "[错误] 您未在本群参与游戏\n";
                "若您想执行元指令，请尝试在请求前加\"#\"，或通过\"#帮助\"查看所有支持的元指令";
                return EC_MATCH_NOT_THIS_GROUP;
            }
            return match->Request(uid, gid, msg, reply);
        }
    }
}

void* /*__cdecl*/ BOT_API::Init(const uint64_t this_uid, const NEW_MSG_SENDER_CALLBACK new_msg_sender_cb,
                                const DELETE_MSG_SENDER_CALLBACK delete_msg_sender_cb, const char* const game_path,
                                const uint64_t* const admins, const uint64_t admin_count)
{
    if (this_uid == 0 || !new_msg_sender_cb || !delete_msg_sender_cb || !game_path) {
        return nullptr;
    }
    return new BotCtx(this_uid, new_msg_sender_cb, delete_msg_sender_cb, game_path, admins, admin_count);
}

void /*__cdelcl*/ BOT_API::Release(void* const bot) { delete static_cast<BotCtx*>(bot); }

ErrCode /*__cdecl*/ BOT_API::HandlePrivateRequest(void* const bot_p, const uint64_t uid, const char* const msg)
{
    if (!bot_p) {
        return EC_NOT_INIT;
    }
    BotCtx& bot = *static_cast<BotCtx*>(bot_p);
    return HandleRequest(bot, {}, uid, msg, [uid, &bot] { return bot.ToUser(uid); });
}

ErrCode /*__cdecl*/ BOT_API::HandlePublicRequest(void* const bot_p, const uint64_t gid, const uint64_t uid,
                                                 const char* const msg)
{
    if (!bot_p) {
        return EC_NOT_INIT;
    }
    BotCtx& bot = *static_cast<BotCtx*>(bot_p);
    return HandleRequest(bot, gid, uid, msg, [uid, gid, &bot] {
        auto sender = bot.ToGroup(gid);
        sender << AtMsg(uid) << "\n";
        return sender;
    });
}

#ifdef WITH_MYSQL
ErrCode /*__cdecl*/ BOT_API::ConnectDatabase(void* const bot_p, const char* const addr, const char* const user,
                                             const char* const passwd, const char* const db_name, const char** errmsg)
{
    ErrCode code;
    std::tie(code, *errmsg) = DBManager::ConnectDB(addr, user, passwd, db_name);
    if (const std::unique_ptr<DBManager>& db_manager = DBManager::GetDBManager()) {
        for (auto& [name, info] : static_cast<BotCtx*>(bot_p)->game_handles()) {
            info->set_game_id(db_manager->GetGameIDWithName(name));
        }
    }
    return code;
}
#endif
