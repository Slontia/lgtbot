// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match_env.h"

const char* MatchContext::GameName() const { return game_handle.Info().name_; }

uint64_t MatchContext::MaxPlayerNum() const { return game_handle.CachedMaxPlayer(); }

uint32_t MatchContext::Multiple() const { return game_handle.CachedMultiple(); }

std::string MatchContext::HostUserName(const UserID host_uid) const
{
    return bot.GetUserName(host_uid.GetCStr(), gid.has_value() ? gid->GetCStr() : nullptr);
}

std::string MatchContext::OptionInfo(const bool text_mode) const
{
    return game_handle.ConfigClient().QueryOptionInfo(text_mode);
}
