#include "bot_core/score_calculate.h"

#include <numeric>

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

static void CalTopScore(std::vector<UserInfoForCalScore>& user_infos)
{
    const int64_t user_num = user_infos.size();
    struct ScoreRecorder
    {
        int64_t score_ = 0;
        uint64_t count_ = 0;
    };
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

static std::vector<ScoreInfo> MakeScoreInfo(const std::vector<UserInfoForCalScore>& user_infos, const uint16_t multiple)
{
    std::vector<ScoreInfo> ret;
    for (auto& info : user_infos) {
        ret.emplace_back();
        ret.back().uid_ = info.uid_;
        ret.back().game_score_ = info.game_score_;
        ret.back().zero_sum_score_ = info.zero_sum_score_ * multiple;
        ret.back().top_score_ = info.top_score_ * multiple;
    }
    return ret;
}

std::vector<ScoreInfo> CalScores(std::vector<UserInfoForCalScore>& user_infos, const uint16_t multiple)
{
    CalZeroSumScore(user_infos);
    CalTopScore(user_infos);
    return MakeScoreInfo(user_infos, multiple);
}

