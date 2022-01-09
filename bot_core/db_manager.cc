// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef WITH_SQLITE

#include "db_manager.h"

#include <sstream>
#include <type_traits>

#include "bot_core/log.h"
#include "bot_core/match.h"
#include "bot_core/score_calculation.h"

#include "sqlite_modern_cpp.h"

static void HandleError(const sqlite::sqlite_exception& e)
{
    ErrorLog() << "DB error " << e.get_code() << ": " << e.what() << ", during " << e.get_sql();
}

static void HandleError(const std::exception& e)
{
    ErrorLog() << "DB error " << e.what();
}

template <typename Fn>
static bool ExecuteTransaction(const DBName& db_name, const Fn& fn)
{
    try {
        sqlite::database db(db_name);
        db << "BEGIN;";
        if (fn(db)) {
            db << "COMMIT;";
            return true;
        } else {
            db << "ROLLBACK";
            return false;
        }
    } catch (const sqlite::sqlite_exception& e) {
        HandleError(e);
    } catch (const std::exception& e) {
        HandleError(e);
    }
    return false;
}


#define FAIL_THROW(sql_func, ...)                                                    \
    do {                                                                             \
        SQLRETURN ret = sql_func(##__VA_ARGS__);                                     \
        if (!SQL_SUCCEEDED(ret)) {                                                   \
            throw(std::stringstream() << #sql_func << " failed ret: " << ret).str(); \
        }                                                                            \
    } while (0)

uint64_t InsertMatch(sqlite::database& db, const std::string& game_name, const std::optional<GroupID> gid, const UserID host_uid,
        const uint64_t user_count, const uint64_t multiple)
{
    db << "INSERT INTO match (game_name, group_id, host_user_id, user_count, multiple) VALUES (?,?,?,?,?);"
       << game_name
       << gid
       << host_uid.Get()
       << user_count
       << multiple;
    return db.last_insert_rowid();
}

void InsertUserIfNotExist(sqlite::database& db, const UserID& uid)
{
    db << "INSERT INTO user (user_id) SELECT ? WHERE NOT EXISTS (SELECT user_id FROM user WHERE user_id = ?);"
       << uid.Get() << uid.Get();
}

void InsertUserWithMatch(sqlite::database& db, const uint64_t match_id, const UserID& uid, const uint32_t birth_count,
        const int64_t game_score, const int64_t zero_sum_score, const int64_t top_score, const double level_score)
{
    db << "INSERT INTO user_with_match (match_id, user_id, birth_count, game_score, zero_sum_score, top_score, level_score) VALUES (?,?,?,?,?,?,?);"
       << match_id
       << uid.Get()
       << birth_count
       << game_score
       << zero_sum_score
       << top_score
       << level_score;
}

void UpdateBirthOfUser(sqlite::database& db, const UserID& uid)
{
    db << "UPDATE user SET birth_time = datetime(CURRENT_TIMESTAMP, \'localtime\'), birth_count = birth_count + 1 "
            "WHERE user_id = ?;"
        << uid.Get();
}

auto GetTotalScoreOfUser(sqlite::database& db, const UserID& uid)
{
    struct
    {
        uint64_t match_count_ = 0;
        int64_t total_zero_sum_score_ = 0;
        int64_t total_top_score_ = 0;
    } result;
    db << "SELECT COUNT(*), SUM(zero_sum_score), SUM(top_score) FROM user_with_match, user "
            "WHERE user_with_match.user_id = ? AND "
                "user_with_match.user_id = user.user_id AND "
                "user_with_match.birth_count = user.birth_count;"
        << uid.Get()
        >> std::tie(result.match_count_, result.total_zero_sum_score_, result.total_top_score_);
    return result;
}

uint32_t GetMatchCountOfUser(sqlite::database& db, const UserID& uid)
{
    uint32_t count = 0;
    db << "SELECT COUNT(*) FROM user_with_match "
            "WHERE user_id = ? AND birth_count = (SELECT birth_count FROM user WHERE user_id = ?);"
        << uid.Get() << uid.Get()
        >> count;
    return count;
}

uint32_t GetBirthCountOfUser(sqlite::database& db, const UserID& uid)
{
    uint32_t birth_count = -1;
    db << "SELECT birth_count FROM user WHERE user_id = ?;" << uid.Get() >> birth_count;
    return birth_count;
}

auto GetGameHistoryOfUser(sqlite::database& db, const UserID& uid, const std::string& game_name)
{
    struct
    {
        uint64_t match_count_;
        double total_level_score_;
    } result;
    db << "SELECT COUNT(*), SUM(level_score) FROM user_with_match, match, user "
            "WHERE user_with_match.user_id = ? AND "
                "user_with_match.user_id = user.user_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "user_with_match.match_id = match.match_id AND "
                "match.game_name = ? "
        << uid.Get()
        << game_name
        >> std::tie(result.match_count_, result.total_level_score_);
    return result;
}

template <typename Fn>
void ForeachTotalLevelScoreOfUser(sqlite::database& db, const UserID& uid, const Fn& fn)
{
    db << "SELECT game_name, COUNT(*), SUM(level_score) FROM user_with_match, match, user "
            "WHERE user_with_match.user_id = ? AND "
                "user_with_match.user_id = user.user_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "user_with_match.match_id = match.match_id "
            "GROUP BY game_name;"
        << uid.Get()
        >> fn;
}

template <typename Fn>
void ForeachRecentMatchOfUser(sqlite::database& db, const UserID& uid, const Fn& fn)
{
    db << "SELECT match.game_name, match.user_count, user_with_match.game_score, "
                "user_with_match.zero_sum_score, user_with_match.top_score, user_with_match.level_score "
            "FROM user_with_match, match, user "
            "WHERE user_with_match.user_id = ? AND "
                "user_with_match.user_id = user.user_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "user_with_match.match_id = match.match_id "
            "ORDER BY match.match_id DESC LIMIT 10"
        << uid.Get()
        >> fn;
}

template <typename Fn>
void ForeachUserInRank(sqlite::database& db, const std::string& score_name, const Fn& fn)
{
    db << "SELECT user.user_id, SUM(user_with_match." + score_name + ") AS sum_score "
            "FROM user_with_match, user "
            "WHERE user_with_match.user_id = user.user_id AND user_with_match.birth_count = user.birth_count "
            "GROUP BY user.user_id ORDER BY sum_score DESC LIMIT 10;"
       >> fn;
}

template <typename Fn>
void ForeachUserInGameLevelScoreRank(sqlite::database& db, const std::string& game_name, const Fn& fn)
{
    db << "SELECT user_id, total_level_score, match_count FROM ("
                "SELECT user.user_id AS user_id, "
                    "SUM(user_with_match.level_score) AS total_level_score, "
                    "COUNT(*) AS match_count "
                "FROM user_with_match, user, match "
                "WHERE user_with_match.user_id = user.user_id AND "
                    "user_with_match.match_id = match.match_id AND "
                    "user_with_match.birth_count = user.birth_count AND "
                    "match.game_name = ? "
                "GROUP BY user.user_id ORDER BY total_level_score DESC) "
            "WHERE match_count >= " + std::to_string(k_show_grade_required_match_count) + " LIMIT 10"
    << game_name
    >> fn;
}

SQLiteDBManager::SQLiteDBManager(const DBName& db_name) : db_name_(db_name)
{
}

SQLiteDBManager::~SQLiteDBManager() {}

void RecordMatch(sqlite::database& db, const std::string& game_name, const std::optional<GroupID> gid,
        const UserID host_uid, const uint64_t multiple, const std::vector<ScoreInfo>& score_infos)
{
    const auto match_id = InsertMatch(db, game_name, gid, host_uid, score_infos.size(), multiple);
    for (const ScoreInfo& score_info : score_infos) {
        InsertUserIfNotExist(db, score_info.uid_);
        const auto birth_count = GetBirthCountOfUser(db, score_info.uid_);
        InsertUserWithMatch(db, match_id, score_info.uid_, birth_count, score_info.game_score_,
                score_info.zero_sum_score_, score_info.top_score_, score_info.level_score_);
    }
}

std::vector<UserInfoForCalScore> GetUserInfoForCalScore(sqlite::database& db, const std::string& game_name,
        const std::vector<std::pair<UserID, int64_t>>& game_score_infos)
{
    std::vector<UserInfoForCalScore> user_infos;
    for (const auto& [uid, game_score] : game_score_infos) {
        const auto history_info = GetGameHistoryOfUser(db, uid, game_name);
        user_infos.emplace_back(uid, game_score, history_info.match_count_, history_info.total_level_score_);
    }
    return user_infos;
}

std::vector<ScoreInfo> SQLiteDBManager::RecordMatch(const std::string& game_name, const std::optional<GroupID> gid,
        const UserID host_uid, const uint64_t multiple, const std::vector<std::pair<UserID, int64_t>>& game_score_infos)
{
    std::vector<ScoreInfo> score_infos; // TODO: get from game_score_infos
    return ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            auto user_infos = GetUserInfoForCalScore(db, game_name, game_score_infos);
            score_infos = CalScores(user_infos, multiple);
            ::RecordMatch(db, game_name, gid, host_uid, multiple, score_infos);
            return true;
        }) ? score_infos : std::vector<ScoreInfo>();
}

UserProfile SQLiteDBManager::GetUserProfile(const UserID uid)
{
    UserProfile profile;
    ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            // get user total_score
            {
                const auto result = GetTotalScoreOfUser(db, uid);
                profile.uid_ = uid;
                profile.match_count_ = result.match_count_;
                profile.total_zero_sum_score_ = result.total_zero_sum_score_;
                profile.total_top_score_ = result.total_top_score_;
            }
            ForeachTotalLevelScoreOfUser(db, uid,
                  [&](const std::string& game_name, const uint64_t count, const double total_level_score)
                      {
                          profile.game_level_infos_.emplace(
                              game_name, GameLevelInfo{.count_ = count, .total_level_score_ = total_level_score});
                      });
            ForeachRecentMatchOfUser(db, uid,
                  [&](const std::string& game_name, const uint64_t user_count, const int64_t game_score,
                              const int64_t zero_sum_score, const int64_t top_score, const double level_score)
                      {
                          profile.recent_matches_.emplace_back();
                          profile.recent_matches_.back().game_name_ = game_name;
                          profile.recent_matches_.back().user_count_ = user_count;
                          profile.recent_matches_.back().game_score_ = game_score;
                          profile.recent_matches_.back().zero_sum_score_ = zero_sum_score;
                          profile.recent_matches_.back().top_score_ = top_score;
                          profile.recent_matches_.back().level_score_ = level_score;
                      });
            return true;
        });
    return profile;
}

bool SQLiteDBManager::Suicide(const UserID uid, const uint32_t required_match_num)
{
    return ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            if (GetMatchCountOfUser(db, uid) >= required_match_num) {
                UpdateBirthOfUser(db, uid);
                return true;
            }
            return false;
        });
}

RankInfo SQLiteDBManager::GetRank()
{
    RankInfo info;
    ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            ForeachUserInRank(db, "zero_sum_score", [&](const uint64_t uid, const int64_t score_sum)
                    {
                        info.zero_sum_score_rank_.emplace_back(uid, score_sum);
                    });
            ForeachUserInRank(db, "top_score", [&](const uint64_t uid, const int64_t score_sum)
                    {
                        info.top_score_rank_.emplace_back(uid, score_sum);
                    });
            return true;
        });
    return info;
}

std::vector<std::pair<UserID, double>> SQLiteDBManager::GetLevelScoreRank(const std::string& game_name)
{
    std::vector<std::pair<UserID, double>> vec;
    ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            ForeachUserInGameLevelScoreRank(db, game_name, [&](const uint64_t uid, const double total_level_score)
                    {
                        vec.emplace_back(uid, total_level_score);
                    });
            return true;
        });
    return vec;
}

std::unique_ptr<DBManagerBase> SQLiteDBManager::UseDB(const std::filesystem::path::value_type* const db_name)
{
#ifdef _WIN32
    std::wstring db_name_wstr(db_name);
    std::u16string db_name_str(db_name_wstr.begin(), db_name_wstr.end()); 
#else
    std::string db_name_str(db_name);
#endif
    try {
        sqlite::database db(db_name_str);
        db << "CREATE TABLE IF NOT EXISTS match("
                "match_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "game_name VARCHAR(100) NOT NULL, "
                "finish_time DATETIME DEFAULT CURRENT_TIMESTAMP, "
                "group_id BIGINT UNSIGNED, "
                "host_user_id BIGINT UNSIGNED NOT NULL, "
                "user_count BIGINT UNSIGNED NOT NULL, "
                "multiple INT UNSIGNED NOT NULL);";
        db << "CREATE TABLE IF NOT EXISTS user_with_match("
                "user_id BIGINT UNSIGNED NOT NULL, "
                "birth_count INT UNSIGNED NOT NULL, "
                "match_id BIGINT UNSIGNED NOT NULL, "
                "game_score BIGINT NOT NULL, "
                "zero_sum_score BIGINT NOT NULL, "
                "top_score BIGINT NOT NULL, "
                "level_score DOUBLE NOT NULL, "
                "PRIMARY KEY (user_id, match_id));";
        db << "CREATE INDEX IF NOT EXISTS user_id_index ON user_with_match(user_id);";
        db << "CREATE TABLE IF NOT EXISTS user("
                "user_id BIGINT UNSIGNED PRIMARY KEY, "
                "birth_time DATETIME, "
                "birth_count INT UNSIGNED DEFAULT 0, "
                "passwd VARCHAR(100));";
        db << "CREATE TABLE IF NOT EXISTS achievement("
                "achi_id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
                "achi_name VARCHAR(100) NOT NULL, "
                "description VARCHAR(1000));";
        db << "CREATE TABLE IF NOT EXISTS user_with_achievement("
                "user_id BIGINT UNSIGNED NOT NULL, "
                "achi_id BIGINT UNSIGNED NOT NULL);";
        return std::unique_ptr<DBManagerBase>(new SQLiteDBManager(db_name_str));
    } catch (const sqlite::sqlite_exception& e) {
        HandleError(e);
    } catch (const std::exception& e) {
        HandleError(e);
    }
    return nullptr;
}

#endif