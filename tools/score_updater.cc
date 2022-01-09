// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <gflags/gflags.h>

#include <iostream>
#include <map>

#include "bot_core/score_calculation.h"

#include "sqlite_modern_cpp.h"

DEFINE_string(db_path, "", "The path of db file");

struct GameHistory
{
    uint64_t count_ = 0;
    double level_score_sum_ = 0;
};

class UserHistoryInfo
{
  public:

    UserHistoryInfo() : birth_count_(0) {}

    void TryRebirth(const uint32_t match_birth_count)
    {
        if (match_birth_count > birth_count_) {
            game_histories_.clear();
            birth_count_ = match_birth_count;
        }
    }

    void Record(const std::string game_name, const double level_score)
    {
        auto& game_history = game_histories_.emplace(game_name, GameHistory()).first->second;
        ++game_history.count_;
        game_history.level_score_sum_ += level_score;
    }

    const GameHistory GetGameHistory(const std::string& game_name) const
    {
        if (const auto it = game_histories_.find(game_name); it != game_histories_.end()) {
            return it->second;
        } else {
            return {};
        }
    }

  private:
    uint64_t birth_count_;
    std::map<std::string, GameHistory> game_histories_;
};

std::vector<UserInfoForCalScore> LoadMatch(sqlite::database& db, const uint64_t match_id, const std::string game_name,
        std::map<uint64_t, UserHistoryInfo>& user_history_infos)
{
    std::vector<UserInfoForCalScore> user_infos;
    db << "SELECT user_id, birth_count, game_score FROM user_with_match WHERE match_id = ?;" << match_id
       >> [&](const uint64_t user_id, const uint64_t birth_count, const int64_t game_score)
            {
                auto& user_history_info = user_history_infos[user_id];
                user_history_info.TryRebirth(birth_count);
                const auto& game_history_info = user_history_info.GetGameHistory(game_name);
                user_infos.emplace_back(
                        user_id, game_score, game_history_info.count_, game_history_info.level_score_sum_);
            };
    return user_infos;
}

void UpdateMatchScore(sqlite::database& db, const uint64_t match_id, const std::vector<ScoreInfo>& score_infos)
{
    for (const auto& info : score_infos) {
        db << "UPDATE user_with_match SET zero_sum_score = ?, top_score = ?, level_score = ? "
              "WHERE match_id = ? AND user_id = ?;"
           << info.zero_sum_score_ << info.top_score_ << info.level_score_ << match_id << info.uid_;
        std::cout << "update: " << info.zero_sum_score_ << " " << info.top_score_ << " " << info.level_score_ << " " << match_id << " " << info.uid_ << std::endl;
    }
}

void UpdateUserHistoryInfo(const std::string& game_name, const std::vector<ScoreInfo>& score_infos,
        std::map<uint64_t, UserHistoryInfo>& user_history_infos)
{
    for (auto& info : score_infos) {
        user_history_infos[info.uid_].Record(game_name, info.level_score_);
    }
}

int main(int argc, char** argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_db_path.empty()) {
        std::cerr << "[ERROR] db_path should not be empty" << std::endl;
        return 1;
    }
    try {
        std::map<uint64_t, UserHistoryInfo> user_history_infos;
        sqlite::database db(FLAGS_db_path);
        db << "BEGIN;";
        db << "SELECT match_id, game_name, multiple from match;"
           >> [&](const uint64_t match_id, const std::string& game_name, const uint32_t multiple)
                {
                    auto user_infos = LoadMatch(db, match_id, game_name, user_history_infos);
                    if (user_infos.size() > 1) {
                        const auto score_infos = CalScores(user_infos, multiple);
                        UpdateMatchScore(db, match_id, score_infos);
                        UpdateUserHistoryInfo(game_name, score_infos, user_history_infos);
                    }
                };
        db << "COMMIT;";
    } catch (const sqlite::sqlite_exception& e) {
        std::cerr << "[ERROR] DB error " << e.get_code() << ": " << e.what() << ", during " << e.get_sql() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] DB error " << e.what() << std::endl;
    }
    return 0;
}


// 查询排名：
// select match.game_name, user_with_match.user_id, user_with_match.birth_count, SUM(user_with_match.level_score) as score from user, user_with_match, match where user_with_match.match_id = match.match_id AND user.user_id = user_with_match.user_id AND user.birth_count = user_with_match.birth_count group by user.user_id, game_name, user.birth_count order by match.game_name, score desc;
//
// 没有体现出差距带来的影响：
// select match.match_id, user_id, level_score from user_with_match, match where user_with_match.match_id = match.match_id AND user_id = 372542780 AND game_name = 'LIE';
// select match.match_id, user_id, level_score from user_with_match, match where user_with_match.match_id = match.match_id AND user_id = 654867229 AND game_name = '决胜五子';
//
// expected 加起来应该是 1 才对
// 87 1 0.0828298 79.7938
// 96 0 0.174279 -16.7308
//
// select match.game_name, user_with_match.user_id, user_with_match.birth_count, SUM(user_with_match.level_score) as score, COUNT(*) from user, user_with_match, match where user_with_match.match_id = match.match_id AND user.user_id = user_with_match.user_id AND user.birth_count = user_with_match.birth_count group by user.user_id, game_name, user.birth_count order by match.game_name, score desc;
