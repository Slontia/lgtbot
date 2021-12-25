#pragma once

#include <sstream>
#include <type_traits>
#include <optional>
#include <memory>
#include <vector>
#include <filesystem>

#include "bot_core/log.h"
#include "bot_core/id.h"

#ifdef _WIN32
using DBName = std::u16string;
#else
using DBName = std::string;
#endif

struct MatchProfile
{
    std::string game_name_;
    int64_t user_count_ = 0;
    int64_t game_score_ = 0;
    int64_t zero_sum_score_ = 0;
    int64_t top_score_ = 0;
};

struct UserProfile
{
    UserID uid_;
    int64_t total_zero_sum_score_ = 0;
    int64_t total_top_score_ = 0;
    int64_t match_count_ = 0;
    std::vector<MatchProfile> recent_matches_;
};

struct ScoreInfo
{
    UserID uid_;
    int64_t game_score_ = 0;
    int64_t zero_sum_score_ = 0;
    int64_t top_score_ = 0;
};

struct RankInfo
{
    std::vector<std::pair<UserID, int64_t>> zero_sum_score_rank_;
    std::vector<std::pair<UserID, int64_t>> top_score_rank_;
};

class DBManagerBase
{
   public:
    virtual ~DBManagerBase() {}
    virtual bool RecordMatch(const std::string& game_name, const std::optional<GroupID> gid, const UserID host_uid,
            const uint64_t multiple, const std::vector<ScoreInfo>& score_infos) = 0;
    virtual UserProfile GetUserProfile(const UserID uid) = 0;
    virtual bool Suicide(const UserID uid, const uint32_t required_match_num) = 0;
    virtual RankInfo GetRank() = 0;
};

#ifdef WITH_SQLITE

#define FAIL_THROW(sql_func, ...)                                                    \
    do {                                                                             \
        SQLRETURN ret = sql_func(##__VA_ARGS__);                                     \
        if (!SQL_SUCCEEDED(ret)) {                                                   \
            throw(std::stringstream() << #sql_func << " failed ret: " << ret).str(); \
        }                                                                            \
    } while (0)

class SQLiteDBManager : public DBManagerBase
{
   public:
    static std::unique_ptr<DBManagerBase> UseDB(const std::filesystem::path::value_type* sv);
    virtual ~SQLiteDBManager();
    virtual bool RecordMatch(const std::string& game_name, const std::optional<GroupID> gid, const UserID host_uid,
            const uint64_t multiple, const std::vector<ScoreInfo>& score_infos) override;
    virtual UserProfile GetUserProfile(const UserID uid) override;
    virtual bool Suicide(const UserID uid, const uint32_t required_match_num) override;
    virtual RankInfo GetRank() override;

   private:
    SQLiteDBManager(const DBName& db_name);

    DBName db_name_;
};

#endif
