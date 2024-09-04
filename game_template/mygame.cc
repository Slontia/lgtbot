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

// 0 indicates no max-player limits
uint64_t MaxPlayerNum(const MyGameOptions& options) { return 0; }

// The default score multiple for the game. The value of 0 denotes a testing game.
// We recommend to increase the multiple by one for every 7~8 minutes the game lasts.
uint32_t Multiple(const MyGameOptions& options) { return 0; }

const GameProperties k_properties {
    // The game name which should be unique among all the games.
    .name_ = "测试游戏",

    // The game developer which can be shown in the game list image.
    .developer_ = "佚名",

    // The game description which can be shown in the game list image.
    .description_ = "暂无游戏描述",

    // The true value indicates each user may be assigned with different player IDs in different matches.
    .shuffled_player_id_ = false,
};

// The default generic options.
const MutableGenericOptions k_default_generic_options;

// The commands for showing more rules information. Users can get the information by "#规则 <game name> <rule command>...".
const std::vector<RuleCommand> k_rule_commands = {};

// The commands for initialize the options when starting a game by "#新游戏 <game name> <init options command>..."
const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (MyGameOptions& game_options, MutableGenericOptions& generic_options)
            {
                // Set the target player numbers when an user start the game with the "单机" argument.
                // It is ok to make `k_init_options_commands` empty.
                generic_options.bench_computers_to_player_num_ = 10;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// The function is invoked before a game starts. You can make final adaption for the options.
// The return value of false denotes failure to start a game.
bool AdaptOptions(MsgSenderBase& reply, MyGameOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility), MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
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
        // Returning `OK` means the game stage
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
        // Returning `CHECKOUT` means the current stage will be over.
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << Global().PlayerName(pid) << "退出游戏";
        // Returning `CONTINUE` means the current stage will be continued.
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        return GetScoreInternal_(pid, reply, std::rand() % 1000);
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Global().Boardcast() << "所有玩家准备完成";
        // Returning `CONTINUE` means the current stage will be continued.
        // If `CONTINUE` is returned but all players keep ready, this function will be invoked again.
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
        // Returning `READY` means the player is ready. The current stage will be over when all surviving players are ready.
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

