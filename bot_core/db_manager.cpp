#ifdef WITH_MYSQL

#include "db_manager.h"

#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>

#include <sstream>
#include <type_traits>

#include "log.h"
#include "match.h"

std::unique_ptr<DBManager> DBManager::g_db_manager_ = nullptr;

thread_local static std::string errmsg_;
static const char* MakeErrorMsg(const sql::SQLException& e)
{
    return (errmsg_ = std::to_string(e.getErrorCode()) + ": " + e.what()).c_str();
}


#define FAIL_THROW(sql_func, ...)                                                    \
    do {                                                                             \
        SQLRETURN ret = sql_func(##__VA_ARGS__);                                     \
        if (!SQL_SUCCEEDED(ret)) {                                                   \
            throw(std::stringstream() << #sql_func << " failed ret: " << ret).str(); \
        }                                                                            \
    } while (0)

class Transaction
{
    public:
    enum StmtType : bool { STMT_QUERY, STMT_UPDATE };

    static bool ExecuteTransaction(sql::Connection& connection, const std::function<bool(Transaction&)>& f)
    {
        if (!connection.isValid() && !connection.reconnect()) {
            ErrorLog() << "execute transaction failed lost connection";
            return false;
        }
        Transaction trans(connection);
        try {
            if (f(trans)) {
                connection.commit();
                DebugLog() << "commit transaction sql=\'" << trans.sql_recorder_.str() << "\'";
                return true;
            } else {
                connection.rollback();
                DebugLog() << "rollback transaction sql=\'" << trans.sql_recorder_.str() << "\'";
                return false;
            }
        } catch (sql::SQLException e) {
            ErrorLog() << "execute transaction failed errmsg=" << MakeErrorMsg(e) << " sql=\'"
                        << trans.sql_recorder_.str() << "\'";
            return false;
        }
    }

    template <typename... StmtPart>
    bool Exist(StmtPart&&... part)
    {
        return Execute<STMT_QUERY>(std::forward<StmtPart>(part)...)->rowsCount() > 0;
    }

    template <StmtType type, typename... StmtPart>
    std::conditional_t<type == STMT_UPDATE, void, std::unique_ptr<sql::ResultSet>> ExecuteOneRow(StmtPart&&... part)
    {
        auto ret = Execute<type>(std::forward<StmtPart>(part)...);
        if constexpr (type == STMT_UPDATE) {
            if (ret != 1) {
                throw sql::SQLException("Update one row failed: row count = " + std::to_string(ret));
            }
            return;
        } else {
            if (!ret) {
                throw sql::SQLException("Null result");
            }
            if (ret->rowsCount() != 1) {
                throw sql::SQLException("Query one row failed: row count = " + std::to_string(ret->rowsCount()));
            }
            if (!ret->next()) {
                throw sql::SQLException("Next result failed");
            }
            return ret;
        }
    }

    template <typename... StmtPart>
    std::unique_ptr<sql::ResultSet> ExecuteQueryLessThanOneRow(StmtPart&&... part)
    {
        auto ret = Execute<STMT_QUERY>(std::forward<StmtPart>(part)...);
        if (!ret) {
            throw sql::SQLException("Null result");
        }
        if (ret->rowsCount() > 1) {
            throw sql::SQLException("Query less than one row failed: row count = " +
                                    std::to_string(ret->rowsCount()));
        }
        if (ret->rowsCount() == 0) {
            return nullptr;
        }
        if (!ret->next()) {
            throw sql::SQLException("Next result failed");
        }
        return ret;
    }

    template <StmtType type, typename... StmtPart>
    std::conditional_t<type == STMT_UPDATE, int, std::unique_ptr<sql::ResultSet>> Execute(StmtPart&&... part)
    {
        std::stringstream ss;
        (ss << ... << part);
        std::string sql = ss.str();
        sql_recorder_ << sql << "; ";
        if constexpr (type == STMT_UPDATE) {
            return stmt_->executeUpdate(sql);
        } else {
            return std::unique_ptr<sql::ResultSet>(stmt_->executeQuery(sql));
        }
    }

    std::unique_ptr<sql::ResultSet> GetLastInsertID();

    private:
    Transaction(sql::Connection& connection);
    ~Transaction();

    sql::Connection& connection_;
    std::unique_ptr<sql::Statement> stmt_;
    std::stringstream sql_recorder_;
};

DBManager::DBManager(std::unique_ptr<sql::Connection>&& connection) : connection_(std::move(connection))
{
    assert(connection_);
}

DBManager::~DBManager() {}

std::pair<ErrCode, const char*> DBManager::ConnectDB(const std::string& addr, const std::string& user,
                                                     const std::string& passwd, const std::string& db_name)
{
    if (g_db_manager_) {
        return {EC_DB_ALREADY_CONNECTED, nullptr};
    }
    ErrCode code = EC_OK;
    const char* errmsg = nullptr;
    std::unique_ptr<sql::Connection> connection = nullptr;
    if (std::tie(code, errmsg, connection) = BuildConnection(addr, user, passwd); code != EC_OK) {
        return {code, errmsg};
    }
    assert(connection != nullptr);
    if (std::tie(code, errmsg) = UseDB(*connection, db_name); code != EC_OK) {
        return {code, errmsg};
    }
    g_db_manager_ = std::make_unique<DBManager>(std::move(connection));
    return {EC_OK, nullptr};
}

bool DBManager::DisconnectDB() { return std::exchange(g_db_manager_, nullptr) != nullptr; }

const std::unique_ptr<DBManager>& DBManager::GetDBManager() { return g_db_manager_; }

bool DBManager::RecordMatch(const uint64_t game_id, const std::optional<GroupID> gid, const UserID host_uid,
                    const uint64_t multiple, const std::vector<Match::ScoreInfo>& score_infos)
{
    assert(score_infos.size() >= 2);
    return Transaction::ExecuteTransaction(*connection_, [&](Transaction& trans) {
        trans.ExecuteOneRow<trans.STMT_UPDATE>(
                "INSERT INTO match (game_id, group_id, host_user_id, user_count, multiple) VALUES (",
                    game_id, ", ",
                    gid.has_value() ? std::to_string(*gid) : "NULL", ", ",
                    host_uid, ", ",
                    score_infos.size(), ", ",
                    multiple, ")");
        const uint64_t match_id = trans.GetLastInsertID()->getUInt64("match_id");
        for (const Match::ScoreInfo& score_info : score_infos) {
            trans.ExecuteOneRow<trans.STMT_UPDATE>(
                    "INSERT INTO user (user_id) VALUES (", score_info.uid_, ") "
                    "WHERE NOT EXISTS user_id = ", score_info.uid_);
            trans.ExecuteOneRow<trans.STMT_UPDATE>(
                    "INSERT INTO match_player (match_id, user_id, game_score, zero_sum_match_score, top_score) VALUES (",
                        match_id, ", ",
                        score_info.uid_, ", ",
                        score_info.game_score_, ", ",
                        score_info.zero_sum_score_, ", ",
                        score_info.top_score_, ")");
        }
        return true;
    });
}

std::optional<UserProfit> DBManager::GetUserProfit(const UserID uid)
{
    std::optional<UserProfit> profit;
    Transaction::ExecuteTransaction(*connection_, [&](Transaction& trans) {
        auto total_ret = trans.ExecuteQueryLessThanOneRow(
                "SELECT "
                    "COUNT(*) AS match_count, "
                    "SUM(zero_sum_score) AS total_zero_sum_score, "
                    "SUM(top_score) AS total_top_score "
                "FROM user_with_match WHERE user_id = ", uid);
        if (!total_ret) {
            return true;
        }
        profit.emplace();
        profit->uid_ = uid;
        profit->total_zero_sum_score_ = total_ret->getBoolean("total_zero_sum_score");
        profit->total_top_score_ = total_ret->getBoolean("total_top_score");
        profit->match_count_ = total_ret->getBoolean("match_count");

        auto recent_match_ret = trans.Execute<trans.STMT_QUERY>(
                "SELECT "
                    "game.name AS game_name, "
                    "match.user_count AS user_count, "
                    "user_with_match.game_score AS game_score, "
                    "user_with_match.zero_sum_score AS zero_sum_score, "
                    "user_with_match.top_score AS top_score "
                "FROM user_with_match, match, game "
                "WHERE user_id = ", uid, " AND user_with_match.match_id = match.match_id AND match.game_id = game.game_id "
                "DESC");

        for (int i = 0; i < 10 && recent_match_ret->next(); ++i) {
            profit->recent_matches_.emplace_back();
            profit->recent_matches_.back().game_name_ = recent_match_ret->getString("game_name");
            profit->recent_matches_.back().user_count_ = recent_match_ret->getUInt64("user_count");
            profit->recent_matches_.back().game_score_ = recent_match_ret->getInt64("game_score");
            profit->recent_matches_.back().zero_sum_score_ = recent_match_ret->getInt64("zero_sum_score");
            profit->recent_matches_.back().top_score_ = recent_match_ret->getInt64("top_score");
        }

        return true;
    });
    return profit;
}

uint64_t DBManager::GetGameIDWithName(const std::string_view game_name)
{
    uint64_t game_id = 0;
    Transaction::ExecuteTransaction(*connection_, [&](Transaction& trans) {
        auto ret =
                trans.ExecuteQueryLessThanOneRow("SELECT game_id FROM game WHERE name = \'", game_name, "\'");
        if (ret) {
            game_id = ret->getUInt64("game_id");
            InfoLog() << "GetGameIDWithName succeed game_name=" << game_name << ", game_id=" << game_id;
        } else {
            ErrorLog() << "GetGameIDWithName failed game_name=" << game_name;
        }
        return true;
    });
    return game_id;
}

uint64_t DBManager::ReleaseGame(const std::string_view game_name)
{
    uint64_t game_id = 0;
    Transaction::ExecuteTransaction(*connection_,
                                    [&](Transaction& trans) {
                                        trans.ExecuteOneRow<trans.STMT_UPDATE>(
                                                "INSERT INTO game (name) VALUES (\'", game_name, "\')");
                                        game_id = trans.GetLastInsertID()->getUInt64(1);
                                        return true;
                                    });
    return game_id;
}

std::tuple<ErrCode, const char*, std::unique_ptr<sql::Connection>> DBManager::BuildConnection(const std::string& addr,
                                                                                            const std::string& user,
                                                                                            const std::string& passwd)
{
    try {
        sql::mysql::MySQL_Driver* driver(sql::mysql::get_mysql_driver_instance());
        if (!driver) {
            return {EC_UNEXPECTED_ERROR, nullptr, nullptr};
        }
        std::unique_ptr<sql::Connection> connection(driver->connect(addr, user, passwd));
        if (!connection) {
            return {EC_UNEXPECTED_ERROR, nullptr, nullptr};
        }
        connection->setAutoCommit(false);
        return {EC_OK, nullptr, std::move(connection)};
    } catch (sql::SQLException e) {
        switch (e.getErrorCode()) {
        case 1044: /* ER_DBACCESS_DENIED_ERROR */
        case 1045: /* ER_ACCESS_DENIED_ERROR */
            return {EC_DB_CONNECT_DENIED, MakeErrorMsg(e), nullptr};
        default:
            return {EC_DB_CONNECT_FAILED, MakeErrorMsg(e), nullptr};
        }
    } catch (std::exception e) {
        return {EC_UNEXPECTED_ERROR, (errmsg_ = e.what()).c_str(), nullptr};
    }
}

std::pair<ErrCode, const char*> DBManager::UseDB(sql::Connection& connection, const std::string& db_name)
{
    try {
        std::unique_ptr<sql::Statement> stmt(connection.createStatement());
        stmt->execute("CREATE DATABASE IF NOT EXISTS " + db_name);
        stmt->execute("USE " + db_name);
        std::unique_ptr<sql::ResultSet> ret(stmt->executeQuery(
                "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = \'" + db_name + "\'; "));
        ret->next();
        if (ret->getUInt64(1) == 0) { // if db is empty
            stmt->execute(
                    "CREATE TABLE match("
                        "match_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
                        "game_id BIGINT UNSIGNED NOT NULL, "
                        "finish_time DATETIME DEFAULT CURRENT_TIMESTAMP, "
                        "group_id BIGINT UNSIGNED, "
                        "host_user_id BIGINT UNSIGNED NOT NULL, "
                        "user_count BIGINT UNSIGNED NOT NULL, "
                        "multiple INT UNSIGNED NOT NULL)");
            stmt->execute(
                    "CREATE TABLE user_with_match( "
                        "user_id BIGINT UNSIGNED NOT NULL, "
                        "match_id BIGINT UNSIGNED NOT NULL, "
                        "game_score BIGINT NOT NULL, "
                        "zero_sum_score BIGINT NOT NULL, "
                        "top_score BIGINT NOT NULL, "
                        "PRIMARY KEY (user_id, match_id)");
            stmt->execute(
                    "CREATE INDEX user_id_index ON match_user(user_id)");
            stmt->execute(
                    "CREATE TABLE game("
                        "game_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
                        "name VARCHAR(100) NOT NULL UNIQUE, "
                        "release_time DATETIME DEFAULT CURRENT_TIMESTAMP)");
            stmt->execute(
                    "CREATE TABLE user("
                        "user_id BIGINT UNSIGNED PRIMARY KEY, "
                        "register_time DATETIME DEFAULT CURRENT_TIMESTAMP, "
                        "passwd VARCHAR(100))");
            stmt->execute(
                    "CREATE TABLE achievement("
                        "achi_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
                        "achi_name VARCHAR(100) NOT NULL, "
                        "description VARCHAR(1000))");
            stmt->execute(
                    "CREATE TABLE user_with_achievement("
                        "user_id BIGINT UNSIGNED NOT NULL, "
                        "achi_id BIGINT UNSIGNED NOT NULL)");
        }
        return {EC_OK, nullptr};
    } catch (sql::SQLException e) {
        return {EC_DB_INIT_FAILED, MakeErrorMsg(e)};
    }
}



std::unique_ptr<sql::ResultSet> Transaction::GetLastInsertID()
{
    return ExecuteOneRow<STMT_QUERY>("SELECT LAST_INSERT_ID()");
}

Transaction::Transaction(sql::Connection& connection) : connection_(connection), stmt_(connection.createStatement()) {}

Transaction::~Transaction() {}

#endif
