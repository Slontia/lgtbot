#pragma once

#include <sstream>

#include "lgtbot.h"
#include "match.h"
#include "msg_checker.h"

extern std::string HandleMetaRequest(const UserID uid, const std::optional<GroupID> gid, MsgReader& reader);
