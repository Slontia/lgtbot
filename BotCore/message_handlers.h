#pragma once

#include <sstream>

#include "lgtbot.h"
#include "message_iterator.h"
#include "match.h"

std::string show_gamelist(const UserID uid, const std::optional<GroupID> gid);
std::string new_game(const UserID uid, const std::optional<GroupID> gid, const std::string& gamename, const bool for_public_match);
std::string start_game(const UserID uid, const std::optional<GroupID> gid);
std::string leave(const UserID uid, const std::optional<GroupID> gid);
std::string join(const UserID uid, const std::optional<GroupID> gid, std::optional<MatchId> match_id);
