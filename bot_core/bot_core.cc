// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core.h"

#include <fstream>
#include <filesystem>

#if WITH_GLOG
#include <glog/logging.h>
#endif

#include "utility/msg_checker.h"
#include "utility/log.h"
#include "game_framework/game_main.h"
#include "bot_core/db_manager.h"
#include "bot_core/match.h"
#include "bot_core/message_handlers.h"
#include "bot_core/msg_sender.h"

#include "sqlite_modern_cpp.h"

static_assert(sizeof(META_COMMAND_SIGN) == 2, "The META_COMMAND_SIGN string must contain one character");
static_assert(sizeof(ADMIN_COMMAND_SIGN) == 2, "The ADMIN_COMMAND_SIGN string must contain one character");

static ErrCode HandleRequest(BotCtx& bot, const std::optional<GroupID> gid, const UserID uid, const std::string& msg,
                             MsgSender& reply)
{
    if (std::string first_arg; !(std::stringstream(msg) >> first_arg) || first_arg.empty()) {
        reply() << "[错误] 我不理解，所以你是想表达什么？";
        return EC_REQUEST_EMPTY;
    } else {
        switch (first_arg[0]) {
        case META_COMMAND_SIGN[0]:
            return HandleMetaRequest(bot, uid, gid, msg, reply);
        case ADMIN_COMMAND_SIGN[0]:
            if (!bot.HasAdmin(uid)) {
                reply() << "[错误] 您未持有管理员权限";
                return EC_REQUEST_NOT_ADMIN;
            }
            return HandleAdminRequest(bot, uid, gid, msg, reply);
        default:
            std::shared_ptr<Match> match = bot.match_manager().GetMatch(uid);
            if (!match) {
                reply() << "[错误] 您未参与游戏\n"
                           "若您想执行元指令，请尝试在请求前加\"" META_COMMAND_SIGN "\"，或通过\"" META_COMMAND_SIGN "帮助\"查看所有支持的元指令";
                return EC_MATCH_USER_NOT_IN_MATCH;
            }
            if (match->gid() != gid && gid.has_value()) {
                reply() << "[错误] 您未在本群参与游戏\n";
                "若您想执行元指令，请尝试在请求前加\"" META_COMMAND_SIGN "\"，或通过\"" META_COMMAND_SIGN "帮助\"查看所有支持的元指令";
                return EC_MATCH_NOT_THIS_GROUP;
            }
            return match->Request(uid, gid, msg, reply);
        }
    }
}

LGTBot_Option LGTBot_InitOptions()
{
    LGTBot_Option options;
    memset(&options, 0, sizeof(options));
    return options;
}

void* LGTBot_Create(const LGTBot_Option* const options, const char** const p)
{
    static bool is_inited = false;
    if (!is_inited) {
#ifdef WITH_GLOG
        google::InitGoogleLogging("lgtbot");
#endif
        std::srand(std::chrono::steady_clock::now().time_since_epoch().count());
        is_inited = true;
    }
    const auto set_errmsg = [p](const char* const errmsg)
        {
            if (p) {
                *p = errmsg;
            }
        };
    if (!options) {
        set_errmsg("the pointer to the bot options is NULL");
        return nullptr;
    }
    const auto bot = BotCtx::Create(*options);
    if (const char* const* const errmsg = std::get_if<const char*>(&bot)) {
        set_errmsg(*errmsg);
        return nullptr;
    }
    InfoLog() << "Create the bot successfully, addr:" << std::get<BotCtx*>(bot);
    return std::get<BotCtx*>(bot);
}

void LGTBot_Release(void* const bot_p)
{
    InfoLog() << "Releasing the bot in Release, addr:" << bot_p;
    delete static_cast<BotCtx*>(bot_p);
}

int LGTBot_ReleaseIfNoProcessingGames(void* const bot_p)
{
    if (!bot_p) {
        ErrorLog() << "Release the bot with null pointer";
        return false;
    }
    BotCtx& bot = *static_cast<BotCtx*>(bot_p);
    const auto matches = bot.match_manager().Matches();
    if (std::ranges::any_of(matches, [](const auto& match) { return match->state() == Match::State::IS_STARTED; })) {
        InfoLog() << "ReleaseIfNoProcessingGames failed because there are processing games";
        return false;
    }
    InfoLog() << "Releasing the bot in ReleaseIfNoProcessingGames, addr:" << bot_p;
    std::ranges::for_each(matches, [](const auto& match) { match->Terminate(true); });
    delete &bot;
    return true;
}

ErrCode LGTBot_HandlePrivateRequest(void* const bot_p, const char* const uid, const char* const msg)
{
    if (!bot_p) {
        ErrorLog() << "Handle private request not init failed uid=" << uid << " msg=\"" << msg << "\"";
        return EC_NOT_INIT;
    }
    DebugLog() << "Handle private request uid=" << uid << " msg=\"" << msg << "\"";
    BotCtx& bot = *static_cast<BotCtx*>(bot_p);
    MsgSender sender = bot.MakeMsgSender(UserID{uid});
    return HandleRequest(bot, std::nullopt, uid, msg, sender);
}

class PublicReplyMsgSender : public MsgSender
{
  public:
    PublicReplyMsgSender(MsgSender&& msg_sender, UserID uid) : MsgSender(std::move(msg_sender)), uid_(std::move(uid)) {}

    virtual MsgSenderGuard operator()() override
    {
        MsgSenderGuard guard(*this);
        // TODO: quote the message
        guard << At(uid_) << "\n";
        return guard;
    }

  private:
    const UserID uid_;
};

ErrCode LGTBot_HandlePublicRequest(void* const bot_p, const char* const gid, const char* const uid, const char* const msg)
{
    if (!bot_p) {
        ErrorLog() << "Handle public request not init failed uid=" << uid << " gid=" << gid << " msg=" << msg;
        return EC_NOT_INIT;
    }
    DebugLog() << "Handle public request uid=" << uid << " gid=" << gid << " msg=" << msg;
    BotCtx& bot = *static_cast<BotCtx*>(bot_p);
    PublicReplyMsgSender sender(bot.MakeMsgSender(GroupID{gid}), UserID{uid});
    return HandleRequest(bot, gid, uid, msg, sender);
}

