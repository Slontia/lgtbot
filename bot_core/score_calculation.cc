// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/score_calculation.h"

#include <numeric>
#include <map>
#include <cmath>
#include <iostream>

static void CalZeroSumScore(std::vector<UserInfoForCalScore>& user_infos)
{
    const int64_t user_num = user_infos.size();
    const auto accum_score = [&](const auto& fn)
        {
            return std::accumulate(user_infos.begin(), user_infos.end(), 0,
                    [&](const int64_t sum, const auto& info) { return fn(info.game_score_) + sum; });
        };
    const int64_t sum_score = accum_score([](const int64_t score) { return score; });
    const int64_t abs_sum_score =
        accum_score([&](const int64_t score) { return std::abs(score * user_num - sum_score); });
    for (auto& info : user_infos) {
        info.zero_sum_score_ = abs_sum_score == 0 ?  0 :
            (info.game_score_ * user_num - sum_score) * user_num * k_zero_sum_score_multi / abs_sum_score;
    }
}

struct ScoreRecorder
{
    int64_t score_ = 0;
    uint64_t count_ = 0;
};

static void CalTopScore(std::vector<UserInfoForCalScore>& user_infos)
{
    const int64_t user_num = user_infos.size();
    ScoreRecorder min_recorder;
    ScoreRecorder max_recorder;
    const auto record_fn = [&](const int64_t score, ScoreRecorder& recorder, const auto& op)
        {
            if (recorder.count_ == 0 || op(score, recorder.score_)) {
                recorder.score_ = score;
                recorder.count_ = 1;
            } else if (score == recorder.score_) {
                ++recorder.count_;
            }
        };
    for (const auto& info : user_infos) {
        record_fn(info.game_score_, min_recorder, std::less());
        record_fn(info.game_score_, max_recorder, std::greater());
    }
    const auto top_score_fn = [&](const int64_t score, const ScoreRecorder& recorder)
        {
            return score == recorder.score_ ? user_num * k_top_score_multi / recorder.count_ : 0;
        };
    for (auto& info : user_infos) {
        info.top_score_ += top_score_fn(info.game_score_, max_recorder);
        info.top_score_ -= top_score_fn(info.game_score_, min_recorder);
    }
}

static std::map<int64_t, uint64_t> CalLevelScoreRank(const std::vector<UserInfoForCalScore>& user_infos)
{
    std::map<int64_t, uint64_t> result; // second is count or rank_score
    for (const auto& info : user_infos) {
        ++(result.emplace(info.game_score_, 0).first->second);
    }
    uint64_t base = 0;
    for (auto& [game_score, rank_score] : result) {
        rank_score += base;
        base += (rank_score - base) * 2;
    }
    return result;
}

static void CalLevelScore(std::vector<UserInfoForCalScore>& user_infos)
{
    const double avg_level_score_sum = std::accumulate(user_infos.begin(), user_infos.end(), double(0),
            [&](const double sum, const auto& info) { return info.level_score_sum_ + sum; }) / user_infos.size();
    const auto rank_scores = CalLevelScoreRank(user_infos);
    for (auto& info : user_infos) {
        const double multiple = info.match_count_ > 400 ? 20 : 100 - 0.2 * static_cast<double>(info.match_count_);
        const double actual_rank_score = double(rank_scores.find(info.game_score_)->second - 1) / (2 * user_infos.size() - 2);
        const double expected_rank_score = double(1) / (double(1) + std::pow(10, (avg_level_score_sum - info.level_score_sum_) / 200));
        info.level_score_ = multiple * (actual_rank_score - expected_rank_score);
    }
}

static std::vector<ScoreInfo> MakeScoreInfo(const std::vector<UserInfoForCalScore>& user_infos, const uint16_t multiple)
{
    std::vector<ScoreInfo> ret;
    for (auto& info : user_infos) {
        ret.emplace_back();
        ret.back().uid_ = info.uid_;
        ret.back().game_score_ = info.game_score_;
        ret.back().zero_sum_score_ = info.zero_sum_score_ * multiple;
        ret.back().top_score_ = info.top_score_ * multiple;
        ret.back().level_score_ = info.level_score_;
    }
    return ret;
}

std::vector<ScoreInfo> CalScores(std::vector<UserInfoForCalScore>& user_infos, const uint16_t multiple)
{
    CalZeroSumScore(user_infos);
    CalTopScore(user_infos);
    CalLevelScore(user_infos);
    return MakeScoreInfo(user_infos, multiple);
}

