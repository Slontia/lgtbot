#pragma once
#include <type_traits>

#include "bot_core.h"
#include "util.h"
#include "msg_sender.h"
#include "utility/msg_checker.h"

using MetaUserFuncType = ErrCode(BotCtx&, const UserID, const std::optional<GroupID>&, MsgSenderBase& reply);
using MetaCommand = Command<MetaUserFuncType>;

extern const std::vector<MetaCommand> meta_cmds;
extern const std::vector<MetaCommand> admin_cmds;

ErrCode HandleMetaRequest(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, const std::string& msg,
                          MsgSender& reply);

ErrCode HandleAdminRequest(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, const std::string& msg,
                           MsgSender& reply);
