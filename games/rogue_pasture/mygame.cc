// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "game_util/rogue_pasture.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "肉鸽牧场",
    .developer_ = "宽容",
    .description_ = "放牧得分的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 1; }
uint32_t Multiple(const CustomOptions& options) { return 0; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options) { return true; }

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};
// ========== GAME STAGES ==========

class BuyStage;
class GrazingStage;

class MainStage : public MainGameStage<BuyStage, GrazingStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , round_(0)
    {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;

    virtual void NextStageFsm(BuyStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(GrazingStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    int64_t PlayerScore(const PlayerID pid) const { return pasture.GetScore(); }

    Pasture pasture;

  private:
    uint32_t round_;
};

class BuyStage : public SubGameStage<>
{
  public:
    BuyStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "购买阶段" + std::to_string(round),
                MakeStageCommand(*this, "查看本回合开始时的情况，可用于图片重发", &BuyStage::Info_, VoidChecker("赛况")),
                MakeStageCommand(*this, "购买动物", &BuyStage::Set_, RepeatableChecker<AnyArg>("动物", "老虎")))
            , round_(round)
            , time_(0)
    {}

    virtual void OnStageBegin() override
    {
        NewCard();
    }

  private:

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::vector<std::string>& animals)
    {
        int n = buy_list.size();
        std::vector<int> vis(n, 0);
        int count = 0;
        if (animals.size() != Main().pasture.GetBuyCount())
        {
            reply() << "参数个数错误";
            return StageErrCode::FAILED;
        }
        for (const std::string& animal: animals)
        {
            for (int i = 0; i < n; i++)
            {
                if (animal == buy_list[i])
                {
                    if (vis[i])
                    {
                        reply() << "输入了重复的动物";
                        return StageErrCode::FAILED;
                    }
                    vis[i] = 1;
                    count++;
                }
            }
        }
        if (animals.size() != count)
        {
            reply() << "输入动物错误";
            return StageErrCode::FAILED;
        }
        for (int i = 0; i < n; i++)
        {
            if (vis[i])
            {
                Main().pasture.AddAnimal(buy_list[i]);
            }
        }
        if (round_ == 1 && ++time_ < 3) {
            NewCard();
            return StageErrCode::OK;
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(markdown);
        return StageErrCode::OK;
    }

    void NewCard()
    {
        buy_list = Main().pasture.ShuffleN();
        markdown = "# 【" + Name() + "】<br>";
        markdown += "当前牧场：<br>";
        markdown += ToString(Main().pasture.GetAnimal()) + "<br>";
        markdown += "可购买 " + std::to_string(Main().pasture.GetBuyCount()) + " 只：";
        markdown += GetTable();
        Global().Boardcast() << Markdown(markdown);
        Global().StartTimer(GAME_OPTION(局时));
    }

    std::string GetTable()
    {
        if (buy_list.size() == 3) 
        {
            html::Table table(1, 3);
            table.SetTableStyle("align=\"center\" cellpadding=\"20\" cellspacing=\"0\"");
            table.Get(0, 0).SetContent(std::string("![](file:///") + Global().ResourceDir() + buy_list[0] + ".jpg)");
            table.Get(0, 1).SetContent(std::string("![](file:///") + Global().ResourceDir() + buy_list[1] + ".jpg)");
            table.Get(0, 2).SetContent(std::string("![](file:///") + Global().ResourceDir() + buy_list[2] + ".jpg)");
            return table.ToString();
        }
        else if (buy_list.size() == 6) 
        {
            html::Table table(2, 3);
            table.SetTableStyle("align=\"center\" cellpadding=\"20\" cellspacing=\"0\"");
            table.Get(0, 0).SetContent(std::string("![](file:///") + Global().ResourceDir() + buy_list[0] + ".jpg)");
            table.Get(0, 1).SetContent(std::string("![](file:///") + Global().ResourceDir() + buy_list[1] + ".jpg)");
            table.Get(0, 2).SetContent(std::string("![](file:///") + Global().ResourceDir() + buy_list[2] + ".jpg)");
            table.Get(1, 0).SetContent(std::string("![](file:///") + Global().ResourceDir() + buy_list[3] + ".jpg)");
            table.Get(1, 1).SetContent(std::string("![](file:///") + Global().ResourceDir() + buy_list[4] + ".jpg)");
            table.Get(1, 2).SetContent(std::string("![](file:///") + Global().ResourceDir() + buy_list[5] + ".jpg)");
            return table.ToString();
        }
        return "";
    }

    std::vector<std::string> buy_list;
    std::string markdown;
    uint32_t round_;
    uint32_t time_;
};

class GrazingStage : public SubGameStage<>
{
  public:
    GrazingStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "放牧阶段" + std::to_string(round),
                MakeStageCommand(*this, "查看本回合开始时的情况，可用于图片重发", &GrazingStage::Info_, VoidChecker("赛况")),
                MakeStageCommand(*this, "购买动物", &GrazingStage::Set_, RepeatableChecker<AnyArg>("动物", "老虎")))
    {}

    virtual void OnStageBegin() override
    {
        Main().pasture.Rand();
        mInfo = "当前牧场：" + ToString(Main().pasture.GetAnimal()) + "\n";
        mInfo += "抽取到了：" + ToString(Main().pasture.GetGrazing()) + "\n";
        if (Main().pasture.GetRemoveCount() > 0) {
            mInfo += "请从以下动物中移除" + std::to_string(Main().pasture.GetRemoveCount()) + "个：\n";
            mInfo += ToString(Main().pasture.Rest());
            Global().Boardcast() << mInfo;
            Global().StartTimer(GAME_OPTION(局时));
        } else {
            mInfo += Main().pasture.Grazing();
            Global().Boardcast() << mInfo;
            Global().SetReady(0);
        }
    }

  private:

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::vector<std::string>& animals)
    {
        std::map<std::string, int> rest = Main().pasture.Rest();
        if (animals.size() != Main().pasture.GetRemoveCount())
        {
            reply() << "参数个数错误";
            return StageErrCode::FAILED;
        }
        for (const std::string& animal: animals)
        {
            if (rest.count(animal) and rest[animal] >= 1)
            {
                rest[animal] -= 1;
            }
            else
            {
                reply() << "不正确的输入";
                return StageErrCode::FAILED;
            }
        }
        for (const std::string& animal: animals)
        {
            Main().pasture.RemoveAnimal(animal);
        }
        mInfo = Main().pasture.Grazing();
        Global().Boardcast() << mInfo;
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << mInfo;
        return StageErrCode::OK;
    }

    std::string mInfo;
};


void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    setter.Emplace<BuyStage>(*this, ++round_);
}

void MainStage::NextStageFsm(BuyStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (reason == CheckoutReason::BY_TIMEOUT || reason == CheckoutReason::BY_LEAVE) {
        Global().Boardcast() << "游戏结束";
        return;
    }
    setter.Emplace<GrazingStage>(*this, round_);
    return;
}

void MainStage::NextStageFsm(GrazingStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (reason == CheckoutReason::BY_TIMEOUT || reason == CheckoutReason::BY_LEAVE) {
        Global().Boardcast() << "游戏结束";
        return;
    }
    if (round_ < 10) {
        setter.Emplace<BuyStage>(*this, ++round_);
        return;
    }
    Global().Boardcast() << "游戏结束，您的牧场：\n" + ToString(pasture.GetAnimal());
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

