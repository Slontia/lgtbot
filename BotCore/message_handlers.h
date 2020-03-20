#pragma once
#include "msg_checker.h"

extern std::string HandleMetaRequest(const UserID uid, const std::optional<GroupID> gid, MsgReader& reader);
