// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef WITH_SQLITE

#include "db_manager.h"

#include <sstream>
#include <type_traits>
#include <cmath>

#include "utility/log.h"
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
static bool ExecuteTransaction(const std::string& db_name, const Fn& fn)
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

uint64_t InsertMatch(sqlite::database& db, const std::string& game_name, const std::optional<GroupID> gid, const UserID host_uid,
        const uint64_t user_count, const uint64_t multiple)
{
    db << "INSERT INTO match (game_name, finish_time, group_id, host_user_id, user_count, multiple) VALUES (?,datetime(CURRENT_TIMESTAMP, \'localtime\'),?,?,?,?);"
       << game_name
       << (gid ? std::optional{gid->GetStr()} : std::nullopt)
       << host_uid.GetStr()
       << user_count
       << multiple;
    return db.last_insert_rowid();
}

void InsertUserIfNotExist(sqlite::database& db, const UserID& uid)
{
    db << "INSERT INTO user (user_id, birth_time) SELECT ?, datetime(CURRENT_TIMESTAMP, \'localtime\') WHERE NOT EXISTS (SELECT user_id FROM user WHERE user_id = ?);"
       << uid.GetStr() << uid.GetStr();
}

void InsertUserWithMatch(sqlite::database& db, const uint64_t match_id, const UserID& uid, const uint32_t birth_count,
        const int64_t game_score, const int64_t zero_sum_score, const int64_t top_score, const double level_score, const int64_t rank_score)
{
    db << "INSERT INTO user_with_match (match_id, user_id, birth_count, game_score, zero_sum_score, top_score, level_score, rank_score) VALUES (?,?,?,?,?,?,?,?);"
       << match_id
       << uid.GetStr()
       << birth_count
       << game_score
       << zero_sum_score
       << top_score
       << level_score
       << rank_score;
}

void InsertUserWithAchievement(sqlite::database& db, const uint64_t match_id, const UserID& uid, const uint32_t birth_count,
        const std::string& achievement_name)
{
    db << "INSERT INTO user_with_achievement (user_id, birth_count, match_id, achievement_name) VALUES (?,?,?,?);"
       << uid.GetStr()
       << birth_count
       << match_id
       << achievement_name;
}

void UpdateBirthOfUser(sqlite::database& db, const UserID& uid)
{
    db << "UPDATE user SET birth_time = datetime(CURRENT_TIMESTAMP, \'localtime\'), birth_count = birth_count + 1 "
            "WHERE user_id = ?;"
        << uid.GetStr();
}

static std::string ComparationCondition(const std::string_view& column_name, const std::string_view& op,
        const std::string_view& value)
{
    if (value.empty()) {
        return "TRUE";
    }
    return std::string() + column_name.data() + " " + op.data() + " " + value.data();
}

static std::string TimeRangeLeftCondition(const std::string_view& column_name, const std::string_view& time_range_begin)
{
    return ComparationCondition(column_name, ">=", time_range_begin);
}

static std::string TimeRangeRightCondition(const std::string_view& column_name, const std::string_view& time_range_end)
{
    return ComparationCondition(column_name, "<", time_range_end);
}

auto GetTotalScoreOfUser(sqlite::database& db, const UserID& uid, const std::string_view& time_range_begin,
        const std::string_view& time_range_end)
{
    struct
    {
        uint64_t match_count_ = 0;
        int64_t total_zero_sum_score_ = 0;
        int64_t total_top_score_ = 0;
        std::string birth_time_;
    } result;
    db << "SELECT COUNT(*), SUM(zero_sum_score), SUM(top_score), birth_time FROM user_with_match, user, match "
            "WHERE user_with_match.user_id = ? AND "
                "user_with_match.user_id = user.user_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "user_with_match.match_id = match.match_id AND "
                + TimeRangeLeftCondition("match.finish_time", time_range_begin) + " AND "
                + TimeRangeRightCondition("match.finish_time", time_range_end) + " "
        << uid.GetStr()
        >> std::tie(result.match_count_, result.total_zero_sum_score_, result.total_top_score_, result.birth_time_);
    return result;
}

uint32_t GetMatchCountOfUser(sqlite::database& db, const UserID& uid)
{
    uint32_t count = 0;
    db << "SELECT COUNT(*) FROM user_with_match "
            "WHERE user_id = ? AND birth_count = (SELECT birth_count FROM user WHERE user_id = ?);"
        << uid.GetStr() << uid.GetStr()
        >> count;
    return count;
}

uint32_t GetBirthCountOfUser(sqlite::database& db, const UserID& uid)
{
    InsertUserIfNotExist(db, uid);
    uint32_t birth_count = -1;
    db << "SELECT birth_count FROM user WHERE user_id = ?;" << uid.GetStr() >> birth_count;
    return birth_count;
}

auto GetGameHistoryOfUser(sqlite::database& db, const UserID& uid, const std::string& game_name)
{
    struct
    {
        uint64_t match_count_ = 0;
        double total_level_score_ = 0;
    } result;
    db << "SELECT COUNT(*), SUM(level_score) FROM user_with_match, match, user "
            "WHERE user_with_match.user_id = ? AND "
                "user_with_match.user_id = user.user_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "user_with_match.match_id = match.match_id AND "
                "match.game_name = ? "
        << uid.GetStr()
        << game_name
        >> std::tie(result.match_count_, result.total_level_score_);
    return result;
}

template <typename Fn>
void ForeachTotalLevelScoreOfUser(sqlite::database& db, const UserID& uid, const std::string_view& time_range_begin,
        const std::string_view& time_range_end, const Fn& fn)
{
    db << "WITH game_match AS ( "
            "SELECT game_name, level_score, finish_time "
            "FROM user_with_match, user, match "
            "WHERE user_with_match.user_id = ? AND "
                "user_with_match.user_id = user.user_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "user_with_match.match_id = match.match_id "
        ") "
        "SELECT game_history_total_level_score.game_name, time_range_match_count, history_total_level_score "
        "FROM "
            "( "
                "SELECT game_name, SUM(level_score) AS history_total_level_score "
                "FROM game_match "
                "WHERE " + TimeRangeRightCondition("finish_time", time_range_end) + " "
                "GROUP BY game_name "
            ") AS game_history_total_level_score, "
            "( "
                "SELECT game_name, COUNT(*) AS time_range_match_count "
                "FROM game_match "
                "WHERE " + TimeRangeLeftCondition("finish_time", time_range_begin) + " AND "
                + TimeRangeRightCondition("finish_time", time_range_end) + " "
                "GROUP BY game_name "
            ") AS game_time_range_match_count "
        "WHERE game_history_total_level_score.game_name = game_time_range_match_count.game_name; "
        << uid.GetStr()
        >> fn;
}

template <typename Fn>
void ForeachRecentMatchOfUser(sqlite::database& db, const UserID& uid, const uint32_t limit, const Fn& fn)
{
    db << "SELECT match.game_name, match.finish_time, match.user_count, match.multiple, user_with_match.game_score, "
                "user_with_match.zero_sum_score, user_with_match.top_score, user_with_match.level_score, user_with_match.rank_score "
            "FROM user_with_match, match, user "
            "WHERE user_with_match.user_id = ? AND "
                "user_with_match.user_id = user.user_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "user_with_match.match_id = match.match_id "
            "ORDER BY match.match_id DESC LIMIT ?"
        << uid.GetStr() << limit
        >> fn;
}

template <typename Fn>
void ForeachUserInRank(sqlite::database& db, const std::string& score_name, const std::string_view& time_range_begin,
        const std::string_view& time_range_end, const Fn& fn)
{
    db << "SELECT user.user_id, SUM(" + score_name + ") AS sum_score "
            "FROM user_with_match, user, match "
            "WHERE user_with_match.user_id = user.user_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "match.match_id = user_with_match.match_id AND "
                + TimeRangeLeftCondition("match.finish_time", time_range_begin) + " AND "
                + TimeRangeRightCondition("match.finish_time", time_range_end) + " "
            "GROUP BY user.user_id ORDER BY sum_score DESC LIMIT 10;"
       >> fn;
}

template <typename Fn>
void ForeachUserInGameLevelScoreRank(sqlite::database& db, const std::string_view& game_name, const std::string_view& time_range_begin,
        const std::string_view& time_range_end, const Fn& fn)
{
    db << "SELECT user.user_id AS user_id, "
                "SUM(user_with_match.level_score) AS total_level_score "
            "FROM user_with_match, user, match "
            "WHERE user_with_match.user_id = user.user_id AND "
                "user_with_match.match_id = match.match_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "match.game_name = ? AND "
                + TimeRangeRightCondition("match.finish_time", time_range_end) + " "
            "GROUP BY user.user_id ORDER BY total_level_score DESC LIMIT 10"
    << game_name.data()
    >> fn;
}

template <typename Fn>
void ForeachUserInGameWeightLevelScoreRank(sqlite::database& db, const std::string_view& game_name,
        const std::string_view& time_range_begin, const std::string_view& time_range_end, const Fn& fn)
{
    db << "WITH game_user_match AS ( "
                "SELECT user.user_id AS user_id, "
                    "user_with_match.level_score AS level_score, "
                    "match.finish_time AS finish_time "
                "FROM user_with_match, user, match "
                "WHERE user_with_match.user_id = user.user_id AND "
                    "user_with_match.match_id = match.match_id AND "
                    "user_with_match.birth_count = user.birth_count AND "
                    "match.game_name = ? "
            ") "
            "SELECT user_history_total_level_score.user_id AS user_id, "
                "history_total_level_score * ABS(history_total_level_score) * time_range_match_count AS weight_level_score "
            "FROM "
                "( "
                    "SELECT game_user_match.user_id, SUM(game_user_match.level_score) AS history_total_level_score "
                    "FROM game_user_match "
                    "WHERE " + TimeRangeRightCondition("game_user_match.finish_time", time_range_end) + " "
                    "GROUP BY game_user_match.user_id "
                ") AS user_history_total_level_score, "
                "( "
                    "SELECT game_user_match.user_id, COUNT(*) AS time_range_match_count "
                    "FROM game_user_match "
                    "WHERE " + TimeRangeLeftCondition("game_user_match.finish_time", time_range_begin) + " AND "
                        + TimeRangeRightCondition("game_user_match.finish_time", time_range_end) + " "
                    "GROUP BY game_user_match.user_id "
                ") AS user_time_range_match_count "
            "WHERE user_history_total_level_score.user_id = user_time_range_match_count.user_id "
            "ORDER BY weight_level_score DESC LIMIT 10"
       << game_name.data()
       >> fn;
}

template <typename Fn>
void ForeachUserInGameMatchCountRank(sqlite::database& db, const std::string_view& game_name, const std::string_view& time_range_begin,
        const std::string_view& time_range_end, const Fn& fn)
{
    db << "SELECT user.user_id AS user_id, "
                "COUNT(*) AS match_count "
            "FROM user_with_match, user, match "
            "WHERE user_with_match.user_id = user.user_id AND "
                "user_with_match.match_id = match.match_id AND "
                "user_with_match.birth_count = user.birth_count AND "
                "match.game_name = ? AND "
                + TimeRangeLeftCondition("match.finish_time", time_range_begin) + " AND "
                + TimeRangeRightCondition("match.finish_time", time_range_end) + " "
            "GROUP BY user.user_id ORDER BY match_count DESC LIMIT 10"
    << game_name.data()
    >> fn;
}

void AddHonor(sqlite::database& db, const std::string_view& description, const UserID& uid, const uint32_t birth_count)
{
    db << "INSERT INTO honor (description, user_id, birth_count, time) VALUES (?, ?, ?, datetime(CURRENT_TIMESTAMP, \'localtime\'))"
       << description.data() << uid.GetStr() << birth_count;
}

void DeleteHonor(sqlite::database& db, const int32_t id)
{
    db << "DELETE FROM honor WHERE id = ?" << id;
}

template <typename Fn>
void ForeachHonor(sqlite::database& db, const std::string& keyword, const uint32_t limit, const Fn& fn)
{
    db << "SELECT id, description, user_id, time FROM honor WHERE description like '%" + keyword + "%' ORDER BY id DESC LIMIT ?"
       << limit
       >> fn;
}

template <typename Fn>
void ForeachRecentHonorOfUser(sqlite::database& db, const UserID& uid, const uint32_t limit, const Fn& fn)
{
    db << "SELECT honor.id, honor.description, honor.user_id, honor.time FROM honor, user "
          "WHERE honor.user_id = ? AND honor.user_id = user.user_id AND honor.birth_count = user.birth_count "
          "ORDER BY honor.id DESC LIMIT ?"
       << uid.GetStr() << limit
       >> fn;
}

template <typename Fn>
void ForeachRecentAchievementOfUser(sqlite::database& db, const UserID& uid, const uint32_t limit, const Fn& fn)
{
    db << "SELECT user_with_achievement.achievement_name, match.game_name, match.finish_time "
          "FROM user_with_achievement, user, match "
          "WHERE user_with_achievement.user_id = ? AND "
              "user_with_achievement.user_id = user.user_id AND "
              "user_with_achievement.birth_count = user.birth_count AND "
              "user_with_achievement.match_id = match.match_id "
          "ORDER BY user_with_achievement.id DESC LIMIT ?"
       << uid.GetStr() << limit
       >> fn;
}

auto GetAchievementStatistic(sqlite::database& db, const UserID& uid, const std::string& game_name,
        const std::string& achievement_name)
{
    struct
    {
        std::string first_achieve_time_;
        int64_t count_ = 0;
    } result;
    db << "SELECT match.finish_time, COUNT(*) AS count FROM user_with_achievement, user, match "
          "WHERE user_with_achievement.match_id = match.match_id AND "
              "user_with_achievement.user_id = user.user_id and user.user_id = ? AND "
              "match.game_name = ? "
              "AND user_with_achievement.achievement_name = ? "
          "LIMIT 1;"
       << uid.GetStr() << game_name << achievement_name
       >> std::tie(result.first_achieve_time_, result.count_);
    return result;
}

int64_t GetAchievedUserNumber(sqlite::database& db, const std::string& game_name, const std::string& achievement_name)
{
    int64_t count = 0;
    db << "SELECT count(*) FROM "
              "("
                  "SELECT 1 FROM user_with_achievement, match "
                  "WHERE user_with_achievement.match_id = match.match_id AND "
                      "match.game_name = ? AND "
                      "user_with_achievement.achievement_name = ? "
                  "GROUP BY user_with_achievement.user_id"
              ");"
       << game_name << achievement_name
       >> count;
    return count;
}

SQLiteDBManager::SQLiteDBManager(std::string db_name) : db_name_(std::move(db_name))
{
}

SQLiteDBManager::~SQLiteDBManager() {}

void RecordMatch(sqlite::database& db, const std::string& game_name, const std::optional<GroupID> gid,
        const UserID host_uid, const uint64_t multiple, const std::vector<ScoreInfo>& score_infos,
        const std::vector<std::pair<UserID, std::string>>& achievements)
{
    const auto match_id = InsertMatch(db, game_name, gid, host_uid, score_infos.size(), multiple);
    for (const ScoreInfo& score_info : score_infos) {
        const auto birth_count = GetBirthCountOfUser(db, score_info.uid_);
        InsertUserWithMatch(db, match_id, score_info.uid_, birth_count, score_info.game_score_,
                score_info.zero_sum_score_, score_info.top_score_, score_info.level_score_, score_info.rank_score_);
    }
    for (const auto& [user_id, achievement_name] : achievements) {
        const auto birth_count = GetBirthCountOfUser(db, user_id);
        InsertUserWithAchievement(db, match_id, user_id, birth_count, achievement_name);
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
        const UserID& host_uid, const uint64_t multiple, const std::vector<std::pair<UserID, int64_t>>& game_score_infos,
        const std::vector<std::pair<UserID, std::string>>& achievements)
{
    std::vector<ScoreInfo> score_infos; // TODO: get from game_score_infos
    return ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            auto user_infos = GetUserInfoForCalScore(db, game_name, game_score_infos);
            score_infos = CalScores(user_infos, multiple);
            ::RecordMatch(db, game_name, gid, host_uid, multiple, score_infos, achievements);
            return true;
        }) ? score_infos : std::vector<ScoreInfo>();
}

UserProfile SQLiteDBManager::GetUserProfile(const UserID& uid, const std::string_view& time_range_begin,
        const std::string_view& time_range_end)
{
    UserProfile profile;
    ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            // get user total_score
            {
                const auto result = GetTotalScoreOfUser(db, uid, time_range_begin, time_range_end);
                profile.uid_ = uid;
                profile.match_count_ = result.match_count_;
                profile.total_zero_sum_score_ = result.total_zero_sum_score_;
                profile.total_top_score_ = result.total_top_score_;
                profile.birth_time_ = result.birth_time_;
            }
            ForeachTotalLevelScoreOfUser(db, uid, time_range_begin, time_range_end,
                  [&](const std::string& game_name, const uint64_t count, const double total_level_score)
                      {
                          profile.game_level_infos_.emplace_back(GameLevelInfo{
                                  .game_name_ = game_name, .count_ = count, .total_level_score_ = total_level_score});
                      });
            ForeachRecentMatchOfUser(db, uid, 10,
                  [&](const std::string& game_name, const std::string& finish_time, const uint64_t user_count,
                              const uint32_t multiple, const int64_t game_score, const int64_t zero_sum_score,
                              const int64_t top_score, const double level_score, const int64_t rank_score)
                      {
                          profile.recent_matches_.emplace_back();
                          auto& match_info = profile.recent_matches_.back();
                          match_info.game_name_ = game_name;
                          match_info.finish_time_ = finish_time;
                          match_info.user_count_ = user_count;
                          match_info.multiple_ = multiple;
                          match_info.game_score_ = game_score;
                          match_info.zero_sum_score_ = zero_sum_score;
                          match_info.top_score_ = top_score;
                          match_info.level_score_ = level_score;
                          match_info.rank_score_ = rank_score;
                      });
            ForeachRecentHonorOfUser(db, uid, 10,
                  [&](const int32_t id, std::string description, std::string uid, std::string time)
                    {
                        profile.recent_honors_.emplace_back(id, std::move(description), std::move(uid), std::move(time));
                    });
            ForeachRecentAchievementOfUser(db, uid, 10,
                  [&](std::string achievement_name, std::string game_name, std::string time)
                    {
                        profile.recent_achievements_.emplace_back(std::move(game_name), std::move(achievement_name), std::move(time));
                    });
            return true;
        });
    std::ranges::sort(profile.game_level_infos_,
            [](const auto& _1, const auto& _2) { return _1.total_level_score_ > _2.total_level_score_; });
    return profile;
}

bool SQLiteDBManager::Suicide(const UserID& uid, const uint32_t required_match_num)
{
    return ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            uint32_t posi_score_count = 0;
            ForeachRecentMatchOfUser(db, uid, required_match_num,
                  [&](const std::string& game_name, const std::string& finish_time, const uint64_t user_count,
                              const uint32_t multiple, const int64_t game_score, const int64_t zero_sum_score,
                              const int64_t top_score, const double level_score, const int64_t rank_score)
                      {
                          posi_score_count += zero_sum_score > 0;
                      });
            if (posi_score_count == required_match_num) {
                UpdateBirthOfUser(db, uid);
                return true;
            }
            return false;
        });
}

RankInfo SQLiteDBManager::GetRank(const std::string_view& time_range_begin, const std::string_view& time_range_end)
{
    RankInfo info;
    ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            ForeachUserInRank(db, "user_with_match.zero_sum_score", time_range_begin, time_range_end,
                    [&](std::string uid, const int64_t score_sum)
                    {
                        info.zero_sum_score_rank_.emplace_back(std::move(uid), score_sum);
                    });
            ForeachUserInRank(db, "user_with_match.top_score", time_range_begin, time_range_end,
                    [&](std::string uid, const int64_t score_sum)
                    {
                        info.top_score_rank_.emplace_back(std::move(uid), score_sum);
                    });
            ForeachUserInRank(db, "1", time_range_begin, time_range_end,
                    [&](std::string uid, const int64_t score_sum)
                    {
                        info.match_count_rank_.emplace_back(std::move(uid), score_sum);
                    });
            return true;
        });
    return info;
}

GameRankInfo SQLiteDBManager::GetLevelScoreRank(const std::string& game_name, const std::string_view& time_range_begin,
        const std::string_view& time_range_end)
{
    GameRankInfo info;
    ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            ForeachUserInGameLevelScoreRank(db, game_name, time_range_begin, time_range_end,
                    [&](std::string uid, const double total_level_score)
                    {
                        info.level_score_rank_.emplace_back(std::move(uid), total_level_score);
                    });
            ForeachUserInGameWeightLevelScoreRank(db, game_name, time_range_begin, time_range_end,
                    [&](std::string uid, double weight_level_score)
                    {
                        weight_level_score =
                            (1 - 2 * std::signbit(weight_level_score)) * std::sqrt(std::abs(weight_level_score));
                        info.weight_level_score_rank_.emplace_back(std::move(uid), weight_level_score);
                    });
            ForeachUserInGameMatchCountRank(db, game_name, time_range_begin, time_range_end,
                    [&](std::string uid, const int64_t match_count)
                    {
                        info.match_count_rank_.emplace_back(std::move(uid), match_count);
                    });
            return true;
        });
    return info;
}

AchievementStatisticInfo SQLiteDBManager::GetAchievementStatistic(const UserID& uid, const std::string& game_name,
            const std::string& achievement_name)
{
    AchievementStatisticInfo info;
    ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            auto result = ::GetAchievementStatistic(db, uid, game_name, std::string(achievement_name));
            info.first_achieve_time_ = std::move(result.first_achieve_time_);
            info.count_ = result.count_;
            info.achieved_user_num_ = GetAchievedUserNumber(db, game_name, achievement_name);
            return true;
        });
    return info;
}

bool SQLiteDBManager::AddHonor(const UserID& uid, const std::string_view& description)
{
    return ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            const auto birth_count = GetBirthCountOfUser(db, uid);
            ::AddHonor(db, description, uid, birth_count);
            return true;
        });
}

bool SQLiteDBManager::DeleteHonor(const int32_t id)
{
    return ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            ::DeleteHonor(db, id);
            return true;
        });
}

std::vector<HonorInfo> SQLiteDBManager::GetHonors(const std::string& keyword, const uint32_t limit)
{
    std::vector<HonorInfo> info;
    ExecuteTransaction(db_name_, [&](sqlite::database& db)
        {
            ForeachHonor(db, keyword, limit,
                [&](const int32_t id, std::string description, std::string uid, std::string time)
                {
                    info.emplace_back(id, std::move(description), std::move(uid), std::move(time));
                });
            return true;
        });
    return info;
}

std::unique_ptr<DBManagerBase> SQLiteDBManager::UseDB(const char* const db_name)
{
    std::string db_name_str(db_name);
    try {
        sqlite::database db(db_name);
        db << "CREATE TABLE IF NOT EXISTS match("
                "match_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "game_name VARCHAR(100) NOT NULL, "
                "finish_time DATETIME, "
                "group_id VARCHAR(100), "
                "host_user_id VARCHAR(100) NOT NULL, "
                "user_count BIGINT UNSIGNED NOT NULL, "
                "multiple INT UNSIGNED NOT NULL);";
        db << "CREATE TABLE IF NOT EXISTS user_with_match("
                "user_id VARCHAR(100) NOT NULL, "
                "birth_count INT UNSIGNED NOT NULL, "
                "match_id BIGINT UNSIGNED NOT NULL, "
                "game_score BIGINT NOT NULL, "
                "zero_sum_score BIGINT NOT NULL, "
                "top_score BIGINT NOT NULL, "
                "level_score DOUBLE NOT NULL, "
                "rank_score BIGINT NOT NULL, "
                "PRIMARY KEY (user_id, match_id));";
        db << "CREATE INDEX IF NOT EXISTS user_id_index ON user_with_match(user_id);";
        db << "CREATE TABLE IF NOT EXISTS user("
                "user_id VARCHAR(100) PRIMARY KEY, "
                "birth_time DATETIME, "
                "birth_count INT UNSIGNED DEFAULT 0, "
                "passwd VARCHAR(100));";
        db << "CREATE TABLE IF NOT EXISTS honor("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "description VARCHAR(200) NOT NULL, "
                "user_id VARCHAR(100) NOT NULL, "
                "birth_count INT UNSIGNED NOT NULL, "
                "time DATETIME);";
        db << "CREATE TABLE IF NOT EXISTS user_with_achievement("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "user_id VARCHAR(100) NOT NULL, "
                "birth_count INT UNSIGNED NOT NULL, "
                "match_id BIGINT UNSIGNED NOT NULL, "
                "achievement_name VARCHAR(100) NOT NULL);";
        db << "CREATE INDEX IF NOT EXISTS user_id_index ON user_with_achievement(user_id);";
        return std::unique_ptr<DBManagerBase>(new SQLiteDBManager(db_name_str));
    } catch (const sqlite::sqlite_exception& e) {
        HandleError(e);
    } catch (const std::exception& e) {
        HandleError(e);
    }
    return nullptr;
}

#endif
