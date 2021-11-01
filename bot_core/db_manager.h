#pragma once

#ifdef WITH_SQLITE

#include <sstream>
#include <type_traits>
#include <optional>

#include "bot_core/log.h"
#include "bot_core/id.h"

#define FAIL_THROW(sql_func, ...)                                                    \
    do {                                                                             \
        SQLRETURN ret = sql_func(##__VA_ARGS__);                                     \
        if (!SQL_SUCCEEDED(ret)) {                                                   \
            throw(std::stringstream() << #sql_func << " failed ret: " << ret).str(); \
        }                                                                            \
    } while (0)

struct MatchProfit
{
    std::string game_name_;
    int64_t user_count_ = 0;
    int64_t game_score_ = 0;
    int64_t zero_sum_score_ = 0;
    int64_t top_score_ = 0;
};

struct UserProfit
{
    UserID uid_;
    int64_t total_zero_sum_score_ = 0;
    int64_t total_top_score_ = 0;
    int64_t match_count_ = 0;
    std::vector<MatchProfit> recent_matches_;
};

struct ScoreInfo {
    UserID uid_;
    int64_t game_score_ = 0;
    int64_t zero_sum_score_ = 0;
    int64_t top_score_ = 0;
};

class DBManager
{
   public:
    static bool UseDB(const std::string& sv, std::optional<DBManager>& db_manager);

    ~DBManager();

    bool RecordMatch(const uint64_t game_id, const std::optional<GroupID> gid, const UserID host_uid,
            const uint64_t multiple, const std::vector<ScoreInfo>& score_infos);

    std::optional<UserProfit> GetUserProfit(const UserID uid);

    uint64_t GetGameIDWithName(const std::string& game_name);

    void ReleaseGame(const std::string& game_name);

   private:
    DBManager(const std::string& db_name);

    std::string db_name_;
};

#endif
