// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <type_traits>

#include "utility/msg_checker.h"

#include "bot_core/bot_core.h"
#include "bot_core/msg_sender.h"

class BotCtx;

using MetaUserFuncType = ErrCode(BotCtx&, const UserID, const std::optional<GroupID>&, MsgSenderBase& reply);
using MetaCommand = Command<MetaUserFuncType>;

ErrCode HandleMetaRequest(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, const std::string& msg,
                          MsgSender& reply);

ErrCode HandleAdminRequest(BotCtx& bot, const UserID uid, const std::optional<GroupID>& gid, const std::string& msg,
                           MsgSender& reply);
