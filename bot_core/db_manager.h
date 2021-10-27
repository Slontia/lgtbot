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

class DBManager
{
   public:
    DBManager(std::unique_ptr<sql::Connection>&& connection) : connection_(std::move(connection))
    {
        assert(connection_);
    }
    ~DBManager() {}

    static std::pair<ErrCode, const char*> ConnectDB(const std::string& addr, const std::string& user,
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

    static bool DisconnectDB() { return std::exchange(g_db_manager_, nullptr) != nullptr; }

    static const std::unique_ptr<DBManager>& GetDBManager() { return g_db_manager_; }

    bool RecordMatch(const uint64_t game_id, const std::optional<uint64_t> group_id, const UserID host_uesr_id,
                     const uint64_t multiple, const std::vector<Match::ScoreInfo>& score_infos)
    {
        assert(score_infos.size() >= 2);
        return Transaction::ExecuteTransaction(*connection_, [&](Transaction& trans) {
            trans.ExecuteOneRow<trans.STMT_UPDATE>(
                    "INSERT INTO match_info (game_id, group_id, host_user_id, multiple) VALUES (", game_id, ", ",
                    group_id.has_value() ? std::to_string(*group_id) : "NULL", ", ", host_uesr_id, ", ", multiple, ")");
            const uint64_t match_id = trans.GetLastInsertID()->getUInt64(1);
            for (const Match::ScoreInfo& score_info : score_infos) {
                if (!trans.Exist("SELECT * FROM user_info WHERE user_id = ", score_info.uid_)) {
                    trans.ExecuteOneRow<trans.STMT_UPDATE>("INSERT INTO user_info (user_id) VALUES (", score_info.uid_,
                                                           ")");
                }
                trans.ExecuteOneRow<trans.STMT_UPDATE>(
                        "UPDATE user_info SET "
                        "total_posi_match_score = total_posi_match_score + ",
                        score_info.poss_match_score_,
                        ", "
                        "match_count = match_count + ",
                        1,
                        " "
                        "WHERE user_id = ",
                        score_info.uid_);
                if (score_info.zero_sum_match_score_ > 0) {
                    trans.ExecuteOneRow<trans.STMT_UPDATE>(
                            "UPDATE user_info SET "
                            "total_zero_sum_match_score = total_zero_sum_match_score + ",
                            score_info.zero_sum_match_score_ * multiple,
                            ", "
                            "win_combo = win_combo + 1, ",
                            "highest_win_combo = GREATEST(win_combo, highest_win_combo), "
                            "logical_match_count = logical_match_count + ",
                            multiple,
                            ", "
                            "win_logical_match_count = win_logical_match_count + ",
                            multiple,
                            ", "
                            "is_new_player = FALSE "
                            "WHERE user_id = ",
                            score_info.uid_);
                } else if (score_info.zero_sum_match_score_ < 0) {
                    trans.Execute<trans.STMT_UPDATE>(
                            "UPDATE user_info SET "
                            "total_zero_sum_match_score = total_zero_sum_match_score + ",
                            score_info.zero_sum_match_score_,
                            ", "
                            "win_combo = 0, ",
                            "logical_match_count = logical_match_count + ", multiple,
                            ", "
                            "lose_logical_match_count = lose_logical_match_count + ",
                            multiple, " WHERE user_id = ", score_info.uid_, " AND NOT is_new_player");
                }
                trans.ExecuteOneRow<trans.STMT_UPDATE>(
                        "INSERT INTO match_player (match_id, user_id, game_score, zero_sum_match_score, "
                        "posi_match_score) VALUES (",
                        match_id, ", ", score_info.uid_, ", ", score_info.game_score_, ", ",
                        score_info.zero_sum_match_score_, ", ", score_info.poss_match_score_, ")");
            }
            return true;
        });
    }

    void GetUserProfit(const UserID uid, MsgSenderBase& reply)
    {
        Transaction::ExecuteTransaction(*connection_, [&](Transaction& trans) {
            auto ret = trans.ExecuteQueryLessThanOneRow("SELECT * FROM user_info WHERE user_id = ", uid);
            if (!ret) {
                reply() << "您还未参与过游戏";
            } else {
                auto sender = reply();
                if (ret->getBoolean("is_new_player")) {
                    sender << "（新手保护中）\n";
                }
                sender << "零和积分：" << ret->getDouble("total_zero_sum_match_score");
                sender << "\n正积分：" << ret->getDouble("total_posi_match_score");
                sender << "\n逻辑比赛局数（胜/负/平）：" << ret->getUInt64("logical_match_count") << " ("
                       << ret->getUInt64("win_logical_match_count") << "/" << ret->getUInt64("lose_logical_match_count")
                       << "/"
                       << ret->getUInt64("logical_match_count") - ret->getUInt64("win_logical_match_count") -
                                  ret->getUInt64("lose_logical_match_count")
                       << ")";
                sender << "\n当前连胜/最高连胜：" << ret->getUInt64("win_combo") << "/"
                       << ret->getUInt64("highest_win_combo");
                sender << "\n实际总比赛局数：" << ret->getUInt64("match_count");
                sender << "\n注册时间：" << ret->getString("register_time").c_str();
            }
            return true;
        });
    }

    uint64_t GetGameIDWithName(const std::string_view game_name)
    {
        uint64_t game_id = 0;
        Transaction::ExecuteTransaction(*connection_, [&](Transaction& trans) {
            auto ret =
                    trans.ExecuteQueryLessThanOneRow("SELECT game_id FROM game_info WHERE name = \'", game_name, "\'");
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

    uint64_t ReleaseGame(const std::string_view game_name)
    {
        uint64_t game_id = 0;
        Transaction::ExecuteTransaction(*connection_,
                                        [&](Transaction& trans) {
                                            trans.ExecuteOneRow<trans.STMT_UPDATE>(
                                                    "INSERT INTO game_info (name) VALUES (\'", game_name, "\')");
                                            game_id = trans.GetLastInsertID()->getUInt64(1);
                                            return true;
                                        });
        return game_id;
    }

   private:
    static std::tuple<ErrCode, const char*, std::unique_ptr<sql::Connection>> BuildConnection(const std::string& addr,
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

    static std::pair<ErrCode, const char*> UseDB(sql::Connection& connection, const std::string& db_name)
    {
        try {
            std::unique_ptr<sql::Statement> stmt(connection.createStatement());
            stmt->execute("CREATE DATABASE IF NOT EXISTS " + db_name);
            stmt->execute("USE " + db_name);
            std::unique_ptr<sql::ResultSet> ret(stmt->executeQuery(
                    "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = \'" + db_name + "\'; "));
            ret->next();
            if (ret->getUInt64(1) == 0) {
                stmt->execute(
                        "CREATE TABLE match_info("
                        "match_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
                        "game_id BIGINT UNSIGNED NOT NULL, "
                        "finish_time DATETIME DEFAULT CURRENT_TIMESTAMP, "
                        "group_id BIGINT UNSIGNED, "
                        "host_user_id BIGINT UNSIGNED NOT NULL, "
                        "multiple INT UNSIGNED NOT NULL)");
                stmt->execute(
                        "CREATE TABLE match_player( "
                        "match_id BIGINT UNSIGNED NOT NULL, "
                        "user_id BIGINT UNSIGNED NOT NULL, "
                        "game_score INT NOT NULL, "
                        "zero_sum_match_score DOUBLE NOT NULL, "
                        "posi_match_score BIGINT UNSIGNED NOT NULL)");
                stmt->execute(
                        "CREATE TABLE game_info("
                        "game_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
                        "name VARCHAR(100) NOT NULL UNIQUE, "
                        "release_time DATETIME DEFAULT CURRENT_TIMESTAMP)");
                stmt->execute(
                        "CREATE TABLE user_info("
                        "user_id BIGINT UNSIGNED PRIMARY KEY, "
                        "is_new_player BOOLEAN DEFAULT TRUE, "
                        "total_zero_sum_match_score DOUBLE DEFAULT 0, "
                        "total_posi_match_score BIGINT DEFAULT 0, "
                        "match_count BIGINT UNSIGNED DEFAULT 0, "
                        "logical_match_count BIGINT UNSIGNED DEFAULT 0, "
                        "win_logical_match_count BIGINT UNSIGNED DEFAULT 0, "
                        "lose_logical_match_count BIGINT UNSIGNED DEFAULT 0, "
                        "win_combo BIGINT UNSIGNED DEFAULT 0, "
                        "highest_win_combo BIGINT UNSIGNED DEFAULT 0, "
                        "register_time DATETIME DEFAULT CURRENT_TIMESTAMP, "
                        "passwd VARCHAR(100))");
                stmt->execute(
                        "CREATE TABLE achievement_info("
                        "achi_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
                        "achi_name VARCHAR(100) NOT NULL, "
                        "description VARCHAR(1000))");
                stmt->execute(
                        "CREATE TABLE user_achievement("
                        "user_id BIGINT UNSIGNED NOT NULL, "
                        "achi_id BIGINT UNSIGNED NOT NULL)");
            }
            return {EC_OK, nullptr};
        } catch (sql::SQLException e) {
            return {EC_DB_INIT_FAILED, MakeErrorMsg(e)};
        }
    }

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

        std::unique_ptr<sql::ResultSet> GetLastInsertID()
        {
            return ExecuteOneRow<STMT_QUERY>("SELECT LAST_INSERT_ID()");
        }

       private:
        Transaction(sql::Connection& connection) : connection_(connection), stmt_(connection.createStatement()) {}
        ~Transaction() {}

        sql::Connection& connection_;
        std::unique_ptr<sql::Statement> stmt_;
        std::stringstream sql_recorder_;
    };

    static const char* MakeErrorMsg(const sql::SQLException& e)
    {
        return (errmsg_ = std::to_string(e.getErrorCode()) + ": " + e.what()).c_str();
    }

    static thread_local std::string errmsg_;
    static std::unique_ptr<DBManager> g_db_manager_;
    std::unique_ptr<sql::Connection> connection_;
};

#endif
