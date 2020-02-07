#pragma once

#include <sstream>

#include "lgtbot.h"
#include "match.h"

std::string show_gamelist(const UserID uid, const std::optional<GroupID> gid);
std::string new_game(const UserID uid, const std::optional<GroupID> gid, const std::string& gamename);
std::string start_game(const UserID uid, const std::optional<GroupID> gid);
std::string leave(const UserID uid, const std::optional<GroupID> gid);
std::string join_private(const UserID uid, const std::optional<GroupID> gid, const MatchId match_id);
std::string join_public(const UserID uid, const std::optional<GroupID> gid);
