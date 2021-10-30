#pragma once

#ifdef WITH_MYSQL

#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>

#include <sstream>
#include <type_traits>

#include "log.h"
#include "match.h"

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
    int64_t user_count_;
    int64_t game_score_;
    int64_t zero_sum_score_;
    int64_t top_score_;
};

struct UserProfit
{
    UserID uid_;
    int64_t total_zero_sum_score_;
    int64_t total_top_score_;
    int64_t match_count_;
    std::vector<MatchProfit> recent_matches_;
};

class DBManagerBase
{

};

class DBManager
{
   public:
    DBManager(std::unique_ptr<sql::Connection>&& connection);
    ~DBManager();

    static std::pair<ErrCode, const char*> ConnectDB(const std::string& addr, const std::string& user,
            const std::string& passwd, const std::string& db_name);

    static bool DisconnectDB();

    static const std::unique_ptr<DBManager>& GetDBManager();

    bool RecordMatch(const uint64_t game_id, const std::optional<GroupID> gid, const UserID host_uid,
            const uint64_t multiple, const std::vector<Match::ScoreInfo>& score_infos);

    std::optional<UserProfit> GetUserProfit(const UserID uid);

    uint64_t GetGameIDWithName(const std::string_view game_name);

    uint64_t ReleaseGame(const std::string_view game_name);

   private:
    static std::tuple<ErrCode, const char*, std::unique_ptr<sql::Connection>> BuildConnection(const std::string& addr,
            const std::string& user, const std::string& passwd);

    static std::pair<ErrCode, const char*> UseDB(sql::Connection& connection, const std::string& db_name);

    static std::unique_ptr<DBManager> g_db_manager_;
    std::unique_ptr<sql::Connection> connection_;
};

#endif
