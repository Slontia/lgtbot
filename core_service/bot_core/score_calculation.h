// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <vector>

#include "bot_core/id.h"
#include "bot_core/db_manager.h"

static constexpr const auto k_zero_sum_score_multi = 1000;
static constexpr const auto k_top_score_multi = 10;

struct UserInfoForCalScore
{
    UserInfoForCalScore(const UserID uid, const int64_t game_score, const uint64_t match_count,
            const double level_score_sum)
        : uid_(uid)
        , game_score_(game_score)
        , match_count_(match_count)
        , level_score_sum_(level_score_sum)
        , zero_sum_score_(0)
        , top_score_(0)
        , level_score_(0)
        , rank_score_(0)
    {}
    UserID uid_;
    int64_t game_score_;
    uint64_t match_count_;
    double level_score_sum_;
    int64_t zero_sum_score_; // to fill
    int64_t top_score_; // to fill
    double level_score_; // to fill
    int64_t rank_score_; // to fill
};

std::vector<ScoreInfo> CalScores(std::vector<UserInfoForCalScore>& scores, const uint16_t multiple = 1);
