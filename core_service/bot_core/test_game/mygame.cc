// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <unistd.h>
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
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
    .name_ = "测试游戏",
    .developer_ = "测试开发者",
    .description_ = "用来测试的游戏",
};

uint64_t MaxPlayerNum(const CustomOptions& options) { return GET_OPTION_VALUE(options, 最大玩家数); }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("查看测试游戏的特殊规则细节",
            []() { return "这是测试规则细节"; },
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

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly,
        MutableGenericOptions& generic_options)
{
    return true;
}

// Global synchronization for blocking commands, accessible from unit tests.
// 'g_blocked' is set true when a Block_* command begins waiting, and cleared when released.
inline std::mutex g_block_mutex;
inline std::condition_variable g_block_cv;
inline bool g_blocked{false};

class SubStage : public SubGameStage<>
{
  public:
    SubStage(MainStage& main_stage)
        : StageFsm(main_stage, "子阶段"
                , MakeStageCommand(*this, "结束", &SubStage::Over_, VoidChecker("结束子阶段"))
                , MakeStageCommand(*this, "时间到时重新计时", &SubStage::ToResetTimer_, VoidChecker("重新计时"))
                , MakeStageCommand(*this, "所有人准备好时重置准备情况", &SubStage::ToResetReadyAll_, VoidChecker("全员重新准备"), ArithChecker(0, 10))
                , MakeStageCommand(*this, "重置准备情况时将除自己外设置为准备完成", &SubStage::ToResetOthersReady_, VoidChecker("别人重新准备"))
                , MakeStageCommand(*this, "准备", &SubStage::Ready_, VoidChecker("准备"))
                , MakeStageCommand(*this, "输出电脑行动次数", &SubStage::OutputComputerActCount_, VoidChecker("电脑行动次数"), ArithChecker<uint64_t>(0, UINT64_MAX))
                , MakeStageCommand(*this, "电脑失败次数", &SubStage::ToComputerFailed_, VoidChecker("电脑失败"),
                    BasicChecker<PlayerID>(), ArithChecker<uint64_t>(0, UINT64_MAX))
                , MakeStageCommand(*this, "淘汰", &SubStage::Eliminate_, VoidChecker("淘汰"))
                , MakeStageCommand(*this, "挂机", &SubStage::Hook_, VoidChecker("挂机"))
                , MakeStageCommand(*this, "模拟进程崩溃", &SubStage::Crash_, VoidChecker("崩溃"))
                , MakeStageCommand(*this, "阻塞", &SubStage::Block_, VoidChecker("阻塞"))
                , MakeStageCommand(*this, "阻塞并准备", &SubStage::BlockAndReady_, VoidChecker("阻塞并准备"))
                , MakeStageCommand(*this, "阻塞并结束", &SubStage::BlockAndOver_, VoidChecker("阻塞并结束"))
          )
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "子阶段开始";
        Global().StartTimer(GAME_OPTION(时限));
        if (GAME_OPTION(直接结束)) {
            Global().Boardcast() << "游戏直接结束";
            for (PlayerID player_id = 0; player_id < Global().PlayerNum(); ++player_id) {
                Global().SetReady(player_id);
            }
        }
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (to_reset_timer_) {
            to_reset_timer_ = false;
            Global().StartTimer(GAME_OPTION(时限));
            Global().Boardcast() << "时间到，但是回合继续";
            return StageErrCode::OK;
        }
        Global().Boardcast() << "时间到，回合结束";
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        ++computer_act_count_;
        if (to_computer_failed_[pid] > 0) {
            reply() << "电脑行动失败，剩余次数" << (--to_computer_failed_[pid]);
            return StageErrCode::FAILED;
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        if (!to_reset_others_ready_players_.empty()) {
            for (PlayerID player_id = 0; player_id < Global().PlayerNum(); ++player_id) {
                if (to_reset_others_ready_players_.find(player_id) == to_reset_others_ready_players_.end()) {
                    Global().ClearReady(player_id);
                }
            }
            to_reset_others_ready_players_.clear();
        } else if (to_reset_ready_ > 0) {
            --to_reset_ready_;
            Global().ClearReady();
        } else {
            return StageErrCode::CHECKOUT;
        }
        if (to_reset_timer_) {
            to_reset_timer_ = false;
            Global().StartTimer(GAME_OPTION(时限));
            Global().Boardcast() << "全员行动完毕，但是回合继续";
        }
        return StageErrCode::CONTINUE;
    }

  private:
    AtomReqErrCode Ready_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return StageErrCode::READY;
    }

    AtomReqErrCode Over_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode ToResetTimer_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        to_reset_timer_ = true;
        return StageErrCode::OK;
    }

    AtomReqErrCode ToResetReadyAll_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t count)
    {
        to_reset_ready_ = count;
        return StageErrCode::OK;
    }

    AtomReqErrCode ToResetOthersReady_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        to_reset_others_ready_players_.emplace(pid);
        return StageErrCode::OK;
    }

    AtomReqErrCode ToComputerFailed_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const PlayerID failed_pid, const uint32_t count)
    {
        to_computer_failed_[failed_pid] += count;
        return StageErrCode::OK;
    }

    AtomReqErrCode OutputComputerActCount_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const uint64_t expected)
    {
        reply() << "电脑行动次数=" << computer_act_count_;
        computer_act_count_ = 0;
        return StageErrCode::READY;
    }

    AtomReqErrCode Eliminate_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Global().Eliminate(pid);
        return StageErrCode::OK;
    }

    AtomReqErrCode Hook_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Global().Hook(pid);
        return StageErrCode::OK;
    }

    AtomReqErrCode Crash_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        _exit(1);
    }

    AtomReqErrCode Block_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        {
            std::lock_guard<std::mutex> lk(g_block_mutex);
            g_blocked = true;
        }
        g_block_cv.notify_all(); // wake up WaitBlock()
        std::unique_lock<std::mutex> lk(g_block_mutex);
        g_block_cv.wait(lk, [] { return !g_blocked; }); // wait for Unblock()
        return StageErrCode::OK;
    }

    AtomReqErrCode BlockAndOver_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Block_(pid, is_public, reply);
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode BlockAndReady_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Block_(pid, is_public, reply);
        return StageErrCode::READY;
    }

    uint64_t computer_act_count_{0};
    bool to_reset_timer_{false};
    uint32_t to_reset_ready_{0};
    std::set<PlayerID> to_reset_others_ready_players_;
    std::map<PlayerID, uint32_t> to_computer_failed_;

};

class MainStage : public MainGameStage<SubStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "准备切换", &MainStage::ToCheckout_, VoidChecker("准备切换"), ArithChecker(0, 10)),
                MakeStageCommand(*this, "设置玩家分数", &MainStage::Score_, VoidChecker("分数"), ArithChecker<int64_t>(-10, 10)),
                MakeStageCommand(*this, "获得成就", &MainStage::Achievement_, VoidChecker("成就"), ArithChecker<uint8_t>(1, 10)))
        , to_checkout_(0)
        , scores_(utility.PlayerNum(), 0)
    {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        setter.Emplace<SubStage>(*this);
    }

    virtual void NextStageFsm(SubStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if (to_checkout_) {
            Global().Boardcast() << "回合结束，切换子阶段";
            --to_checkout_;
            setter.Emplace<SubStage>(*this);
            return;
        }
        Global().Boardcast() << "回合结束，游戏结束";
        for (const PlayerID pid : achievement_pids_) {
            Global().Achieve(pid, Achievement::普通成就);
        }
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return scores_[pid]; };

  private:
    CompReqErrCode ToCheckout_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t count)
    {
        to_checkout_ = count;
        return StageErrCode::OK;
    }

    CompReqErrCode Score_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t score)
    {
        scores_[pid] = score;
        return StageErrCode::OK;
    }

    CompReqErrCode Achievement_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint8_t count)
    {
        for (uint8_t i = 0; i < count; ++i) {
            achievement_pids_.emplace_back(pid);
        }
        return StageErrCode::OK;
    }

    uint32_t to_checkout_;
    std::vector<int64_t> scores_;
    std::vector<PlayerID> achievement_pids_;
};

// A main stage with no sub-stages, used to test timeout during atomic main stage request handling.
class AtomMainStage : public MainGameStage<>
{
  public:
    AtomMainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "阻塞并结束", &AtomMainStage::BlockAndOver_, VoidChecker("阻塞并结束")))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "原子主阶段开始";
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        return StageErrCode::CHECKOUT;
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return 0; }

  private:
    AtomReqErrCode BlockAndOver_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        {
            std::lock_guard<std::mutex> lk(g_block_mutex);
            g_blocked = true;
        }
        g_block_cv.notify_all();
        std::unique_lock<std::mutex> lk(g_block_mutex);
        g_block_cv.wait(lk, [] { return !g_blocked; });
        return StageErrCode::CHECKOUT;
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME
} // namespace game
} // namespace lgtbot
