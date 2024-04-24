// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <mutex>
#include <map>
#include <set>
#include <optional>

#include "bot_core/match_manager.h"
#include "bot_core/id.h"
#include "bot_core/db_manager.h"
#include "bot_core/options.h"
#include "utility/lock_wrapper.h"
#include "nlohmann/json.hpp"

#include <dirent.h>

using GameHandleMap = std::map<std::string, GameHandle>;

class BotCtx
{
  public:
    BotCtx(const BotCtx&) = delete;
    BotCtx(BotCtx&&) = delete;

    static std::variant<BotCtx*, const char*> Create(const LGTBot_Option& options);

    MatchManager& match_manager() { return match_manager_; }

    auto& game_handles() { return game_handles_; }
    const auto& game_handles() const { return game_handles_; }

    bool HasAdmin(const UserID uid) const { return admins_.find(uid) != admins_.end(); }

    const std::string& game_path() const { return game_path_; }

    const std::string& image_path() const { return image_path_; }

#ifdef WITH_SQLITE
    DBManagerBase* db_manager() const { return db_manager_.get(); }
#endif

    auto& option() { return mutable_bot_options_; }

    const auto& option() const { return mutable_bot_options_; }

    bool UpdateBotConfig(const std::string& option_name, const std::vector<std::string>& option_args);

    bool UpdateGameConfig(const std::string& game_name, const std::string& option_name,
            const std::vector<std::string>& option_args);

    bool UpdateGameDefaultFormal(const std::string& game_name, const bool formal);

    // TODO: I don't know why, if I put the definition into bot_ctx.cc, the compiler will report 'undefined reference' in MSYS2.
    std::string GetUserName(const char* const user_id, const char* const group_id) const
    {
        constexpr static uint64_t k_buffer_size = 128;
        char buffer[k_buffer_size];
        assert(user_id);
        if (group_id) {
            callbacks_.get_user_name_in_group(handler_, buffer, k_buffer_size, group_id, user_id);
        } else {
            callbacks_.get_user_name(handler_, buffer, k_buffer_size, user_id);
        }
        return buffer;
    }

    std::string GetUserAvatar(const char* const user_id, const int32_t size) const;

    MsgSender MakeMsgSender(const UserID& user_id, Match* const match = nullptr) const;
    MsgSender MakeMsgSender(const GroupID& user_id, Match* const match = nullptr) const;

#ifndef TEST_BOT
  private:
#endif
    BotCtx(std::string game_path,
           std::string conf_path,
           std::string image_path,
           LGTBot_Callback callbacks,
           GameHandleMap game_handles,
           std::set<UserID> admins,
#ifdef WITH_SQLITE
           std::unique_ptr<DBManagerBase> db_manager_,
#endif
           MutableBotOption mutable_bot_options,
           nlohmann::json config_json,
           void* const handler);

    // The passed `BotOption` in constructor can be destructed soon, we must store the string.
    std::string game_path_;
    std::string conf_path_;
    std::string image_path_;
    LGTBot_Callback callbacks_;
    GameHandleMap game_handles_;
    std::set<UserID> admins_;
#ifdef WITH_SQLITE
    std::unique_ptr<DBManagerBase> db_manager_;
#endif
    LockWrapper<MutableBotOption> mutable_bot_options_;
    LockWrapper<nlohmann::json> config_json_;
    void* const handler_;

    MatchManager match_manager_;
    mutable std::mutex mutex_;
};
