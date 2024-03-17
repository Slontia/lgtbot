// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const std::string k_game_name = "测试游戏"; // the game name which should be unique among all the games
const uint64_t k_max_player = 0; // 0 indicates no max-player limits
const uint64_t k_multiple = 0; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "佚名";
const std::string k_description = "暂无游戏描述";
const std::vector<RuleCommand> k_rule_commands = {};

std::string GameOption::StatusInfo() const
{
    return "";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 10; }

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(const StageUtility& utility)
        : StageFsm(utility, MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
    {
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << "这里输出当前游戏情况";
        // Returning |OK| means the game stage
        return StageErrCode::OK;
    }

    int round_;
};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合",
                MakeStageCommand(*this, "获得分数", &RoundStage::GetScore_, ArithChecker<int64_t>(0, 1000, "分数")))
    {
    }

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(时限));
        Global().Boardcast() << Name() << "开始";
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().Boardcast() << Name() << "超时结束";
        // Returning |CHECKOUT| means the current stage will be over.
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << Global().PlayerName(pid) << "退出游戏";
        // Returning |CONTINUE| means the current stage will be continued.
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        return GetScoreInternal_(pid, reply, std::rand() % 1000);
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Global().Boardcast() << "所有玩家准备完成";
        // Returning |CONTINUE| means the current stage will be continued.
        // If |CONTINUE| is returned but all players keep ready, this function will be invoked again.
        return StageErrCode::CONTINUE;
    }

  private:
    AtomReqErrCode GetScore_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t score)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判获取分数";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经获取过分数了";
            return StageErrCode::FAILED;
        }
        return GetScoreInternal_(pid, reply, score);
    }

    AtomReqErrCode GetScoreInternal_(const PlayerID pid, MsgSenderBase& reply, const int64_t score)
    {
        auto& player_score = Main().player_scores_[pid];
        player_score += score;
        reply() << "获取成功，您获得了 " << score << " 分，当前共 " << player_score << " 分";
        // Returning |READY| means the player is ready. The current stage will be over when all surviving players are ready.
        return StageErrCode::READY;
    }
};

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    setter.Emplace<RoundStage>(*this, ++round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if ((++round_) <= GAME_OPTION(回合数)) {
        setter.Emplace<RoundStage>(*this, round_);
        return;
    }
    Global().Boardcast() << "游戏结束";
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

