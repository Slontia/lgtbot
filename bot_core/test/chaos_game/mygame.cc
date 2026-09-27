// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <unistd.h>
#include <random>
#include <vector>

#include "bot_core/msg_sender.h"
#include "game_framework/game_options.h"
#include "game_framework/stage.h"
#include "game_framework/util.h"

namespace lgtbot {
namespace game {
namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties{
    .name_ = "混沌游戏",
    .developer_ = "测试开发者",
    .description_ = "用来做并发混沌测试的游戏",
};

uint64_t MaxPlayerNum(const CustomOptions& options) { return GET_OPTION_VALUE(options, 最大玩家数); }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("查看混沌游戏的规则细节",
            []() { return "玩家只有一个行动命令，游戏中行为由概率决定。"; },
            VoidChecker("细节")),
};

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("单机模式",
            [](CustomOptions&, MutableGenericOptions&) { return NewGameMode::SINGLE_USER; },
            VoidChecker("单机")),
    InitOptionsCommand("多人模式",
            [](CustomOptions&, MutableGenericOptions&) { return NewGameMode::MULTIPLE_USERS; },
            VoidChecker("多人")),
};

bool AdaptOptions(ChildMsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly,
        MutableGenericOptions& generic_options)
{
    if (GET_OPTION_VALUE(game_options, 拒绝开始)) {
        reply() << "混沌游戏拒绝了开始";
        return false;
    }
    return true;
}

class SubStage : public SubGameStage<>
{
  public:
    SubStage(MainStage& main_stage)
        : StageFsm(main_stage, "子阶段"
                , MakeStageCommand(*this, "行动", &SubStage::Act_, VoidChecker("行动")))
    {
        rng_.seed(static_cast<uint32_t>(GAME_OPTION(随机种子)));
    }

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "混沌子阶段开始";
        if (GAME_OPTION(时限) > 0) {
            Global().StartTimer(GAME_OPTION(时限));
        }
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (reset_timer_) {
            reset_timer_ = false;
            Global().StartTimer(GAME_OPTION(时限));
            Global().Boardcast() << "时间到，回合继续";
            return StageErrCode::CONTINUE;
        }
        Global().Boardcast() << "时间到，回合结束";
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        if (reset_timer_) {
            reset_timer_ = false;
            Global().StartTimer(GAME_OPTION(时限));
            return StageErrCode::CONTINUE;
        }
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        // Computers always finish their act immediately.
        return StageErrCode::READY;
    }

  private:
    bool Roll_(const uint64_t percent)
    {
        return std::uniform_int_distribution<uint64_t>(0, 99)(rng_) < percent;
    }

    AtomReqErrCode Act_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply);

    std::mt19937 rng_;
    bool reset_timer_{false};
};

class MainStage : public MainGameStage<SubStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , scores_(utility.PlayerNum(), 0)
    {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        setter.Emplace<SubStage>(*this);
    }

    virtual void NextStageFsm(SubStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        Global().Boardcast() << "混沌游戏结束";
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return scores_[pid]; }

    void AddScore(const PlayerID pid, const int64_t delta) { scores_[pid] += delta; }

  private:
    std::vector<int64_t> scores_;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

AtomReqErrCode SubStage::Act_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
{
    // CRASH first: it exits the subprocess before any reply is composed.
    if (Roll_(GAME_OPTION(崩溃概率))) {
        _exit(1);
    }
    if (Roll_(GAME_OPTION(直接结束概率))) {
        reply() << "直接结束";
        return StageErrCode::CHECKOUT;
    }
    if (Roll_(GAME_OPTION(失败概率))) {
        reply() << "行动失败";
        return StageErrCode::FAILED;
    }
    if (Roll_(GAME_OPTION(淘汰概率))) {
        Global().Eliminate(pid);
        return StageErrCode::READY;
    }
    if (Roll_(GAME_OPTION(挂机概率))) {
        Global().Hook(pid);
        return StageErrCode::OK;
    }
    if (Roll_(GAME_OPTION(重计时概率))) {
        reset_timer_ = true;
        return StageErrCode::OK;
    }
    if (Roll_(GAME_OPTION(成就概率))) {
        Global().Achieve(pid, Achievement::混沌成就);
        return StageErrCode::OK;
    }
    if (Roll_(GAME_OPTION(加分概率))) {
        const auto delta = std::uniform_int_distribution<int64_t>(1, 10)(rng_);
        Main().AddScore(pid, delta);
        return StageErrCode::OK;
    }
    return StageErrCode::READY;
}

} // namespace GAME_MODULE_NAME
} // namespace game
} // namespace lgtbot