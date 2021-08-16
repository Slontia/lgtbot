#pragma once

#include <mutex>
#include <map>
#include <set>

#include "bot_core/match_manager.h"
#include "bot_core/id.h"

using GameHandleMap = std::map<std::string, std::unique_ptr<GameHandle>>;

class BotCtx
{
  public:
    BotCtx(const uint64_t this_uid, const char* const game_path, const uint64_t* const admins, const uint64_t admin_count)
            : this_uid_(this_uid),
              match_manager_(*this)
    {
        if (game_path) {
            LoadGameModules_(game_path);
        }
        if (admins && admin_count > 0) {
            LoadAdmins_(admins, admin_count);
        }
    }

    MatchManager& match_manager() { return match_manager_; }

#ifdef TEST_BOT
    auto& game_handles() { return game_handles_; }
#else
    const auto& game_handles() const { return game_handles_; }
#endif

    bool HasAdmin(const UserID uid) const { return admins_.find(uid) != admins_.end(); }

  private:
    void LoadGameModules_(const char* const games_path);
    void LoadAdmins_(const uint64_t* const admins, const uint64_t admin_count);

    const UserID this_uid_;
    std::mutex mutex_;
    GameHandleMap game_handles_;
    std::set<UserID> admins_;
    MatchManager match_manager_;
};
