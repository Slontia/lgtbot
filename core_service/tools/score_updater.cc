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
        std::map<UserID, UserHistoryInfo>& user_history_infos)
{
    std::vector<UserInfoForCalScore> user_infos;
    db << "SELECT user_id, birth_count, game_score FROM user_with_match WHERE match_id = ?;" << match_id
       >> [&](std::string user_id, const uint64_t birth_count, const int64_t game_score)
            {
                auto& user_history_info = user_history_infos[user_id];
                user_history_info.TryRebirth(birth_count);
                const auto& game_history_info = user_history_info.GetGameHistory(game_name);
                user_infos.emplace_back(
                        std::move(user_id), game_score, game_history_info.count_, game_history_info.level_score_sum_);
            };
    return user_infos;
}

void UpdateMatchScore(sqlite::database& db, const uint64_t match_id, const std::vector<ScoreInfo>& score_infos)
{
    for (const auto& info : score_infos) {
        db << "UPDATE user_with_match SET zero_sum_score = ?, top_score = ?, level_score = ?, rank_score = ? "
              "WHERE match_id = ? AND user_id = ?;"
           << info.zero_sum_score_ << info.top_score_ << info.level_score_ << info.rank_score_ << match_id << info.uid_.GetStr();
        std::cout << "uid=" << info.uid_
            << "\tmid=" << match_id
            << "\tzero_sum_score=" << info.zero_sum_score_
            << "\ttop_score=" << info.top_score_
            << "\tlevel_score=" << info.level_score_
            << "\trank_score=" << info.rank_score_ << std::endl;
    }
}

void UpdateUserHistoryInfo(const std::string& game_name, const std::vector<ScoreInfo>& score_infos,
        std::map<UserID, UserHistoryInfo>& user_history_infos)
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
        std::map<UserID, UserHistoryInfo> user_history_infos;
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

