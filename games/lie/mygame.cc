// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>

#include "game_framework/stage.h"
#include "game_framework/util.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "LIE",
    .developer_ = "森高",
    .description_ = "双方猜测数字的简单游戏",
};

uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } /* 0 means no max-player limits */
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class MyTable
{
  public:
    struct Result
    {
        PlayerID loser_;
        PlayerID questioner_;
        int actual_number_;
        bool is_lie_;
    };

    MyTable(const CustomOptions& option, const std::string_view resource_dir)
        : option_(option)
        , resource_dir_{resource_dir}
        , table_(GET_OPTION_VALUE(option, 失败数量) * 2 + 3, GET_OPTION_VALUE(option, 数字种类))
        , player_nums_{std::vector<int>(GET_OPTION_VALUE(option, 数字种类), 0),
                        std::vector<int>(GET_OPTION_VALUE(option, 数字种类), 0)}
    {
        table_.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        table_.MergeRight(0, 0, GET_OPTION_VALUE(option, 数字种类));
        table_.MergeRight(table_.Row() - 1, 0, GET_OPTION_VALUE(option, 数字种类));
        const auto mid_row = GET_OPTION_VALUE(option, 失败数量) + 1;
        for (uint32_t col = 0; col < GET_OPTION_VALUE(option, 数字种类); ++col) {
            for (uint32_t i = 1; i <= GET_OPTION_VALUE(option, 失败数量); ++i) {
                table_.Get(mid_row + i, col).SetContent(Image_("blank"));
                table_.Get(mid_row - i, col).SetContent(Image_("blank"));
            }
        }
    }

    void SetName(std::string p1_name, std::string p2_name)
    {
        table_.Get(0, 0).SetContent(HTML_COLOR_FONT_HEADER(#345a88) " **" + p1_name + "** " HTML_FONT_TAIL);
        table_.Get(table_.Row() - 1, 0).SetContent(HTML_COLOR_FONT_HEADER(#880015) " **" + p2_name + "** " HTML_FONT_TAIL);
    }

    void Lose(const Result result)
    {
        if (last_result_.has_value()) {
            DrawLose_(false);
        }
        last_result_.emplace(result);
        ++player_nums_[result.loser_][result.actual_number_ - 1];
        DrawLose_(true);
    }

    bool CheckOver(const PlayerID questioner)
    {
        bool has_all_num = true;
        for (uint32_t num_idx = 0; num_idx < GET_OPTION_VALUE(option_, 数字种类); ++ num_idx) {
            const int count = player_nums_[static_cast<uint64_t>(questioner)][num_idx];
            if (count >= GET_OPTION_VALUE(option_, 失败数量)) {
                for (int i = 0; i < count; ++i) {
                    table_.Get(GET_OPTION_VALUE(option_, 失败数量) + 1 + (questioner == 1 ? 1 : -1) * (i + 1), num_idx)
                          .SetContent(Image_("death"));
                }
                return true;
            } else if (count == 0) {
                has_all_num = false;
            }
        }
        if (has_all_num) {
            for (uint32_t num_idx = 0; num_idx < GET_OPTION_VALUE(option_, 数字种类); ++ num_idx) {
                table_.Get(GET_OPTION_VALUE(option_, 失败数量) + 1 + (questioner == 1 ? 1 : -1), num_idx)
                      .SetContent(Image_("death"));
            }
        }
        return has_all_num;
    }

    std::string ToHtml(const PlayerID pid)
    {
        if (pid == 0) {
            table_.Get(0, 0).SetColor("#7092be"); // light blue
            table_.Get(table_.Row() - 1, 0).SetColor("white");
        } else {
            table_.Get(0, 0).SetColor("white");
            table_.Get(table_.Row() - 1, 0).SetColor("#b97a57"); // light red
        }
        return table_.ToString();
    }

  private:
    std::string Image_(const std::string_view name) const
    {
        return std::string("![](file:///") + resource_dir_.data() + "/" + name.data() + ".png)";
    }

    void DrawLose_(const bool with_light)
    {
        const int32_t direct = last_result_->loser_ == 1 ? 1 : -1;
        const int32_t offset = player_nums_[last_result_->loser_][last_result_->actual_number_ - 1];
        const char color = last_result_->questioner_ == 1 ? 'r' : 'b';
        const char* const lie = last_result_->is_lie_ ? "lie_" : "truth_";
        table_.Get(GET_OPTION_VALUE(option_, 失败数量) + 1 + direct * offset, last_result_->actual_number_ - 1)
              .SetContent(Image_(std::string(with_light ? "light_" : "") + lie + color));
        for (uint32_t col = 0; col < GET_OPTION_VALUE(option_, 数字种类); ++col) {
            table_.Get(GET_OPTION_VALUE(option_, 失败数量) + 1, col)
                  .SetContent(Image_("point_" + std::to_string(col + 1) + (last_result_->loser_ == 1 ? "_r" : "_b")));
        }
    }

    const CustomOptions& option_;
    std::string_view resource_dir_;
    html::Table table_;
    std::array<std::vector<int>, 2> player_nums_;
    std::optional<Result> last_result_;
};

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
   public:
    MainStage(StageUtility&& utility);
    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    int64_t PlayerScore(const PlayerID pid) const;
    MyTable& table() { return table_; }

   private:
    bool JudgeOver();
    void Info_()
    {
        Global().Boardcast() << Markdown("## 第" + std::to_string(round_) + "回合\n\n" + table_.ToHtml(questioner_));
    }

    MyTable table_;
    PlayerID questioner_;
    uint64_t round_;
    std::array<std::vector<int>, 2> player_nums_;
};

class NumberStage : public SubGameStage<>
{
   public:
    NumberStage(MainStage& main_stage, const PlayerID questioner, int& actual_number, int& lie_number)
            : StageFsm(main_stage, "设置数字阶段",
                    MakeStageCommand(*this, "设置数字", &NumberStage::Number_,
                        ArithChecker<int>(1, GET_OPTION_VALUE(main_stage.Global().Options(), 数字种类), "实际数字"),
                        ArithChecker<int>(1, GET_OPTION_VALUE(main_stage.Global().Options(), 数字种类), "提问数字")))
            , questioner_(questioner)
            , actual_number_(actual_number)
            , lie_number_(lie_number)
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请" << At(questioner_) << "设置数字";
        Global().SetReady(1 - questioner_);
        Global().StartTimer(60);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (questioner_ == pid) {
            actual_number_ = std::rand() % GAME_OPTION(数字种类) + 1;
            lie_number_ = std::rand() % 5 >= 2 ? std::rand() % GAME_OPTION(数字种类) + 1
                                               : actual_number_; // 50% same
            return StageErrCode::READY;
        }
        return StageErrCode::OK;
    }

   private:
    AtomReqErrCode Number_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int actual_number,
            const int lie_number)
    {
        if (pid != questioner_) {
            reply() << "[错误] 本回合您为猜测者，无法设置数字";
            return StageErrCode::FAILED;
        }
        if (is_public) {
            reply() << "[错误] 请私信裁判选择数字，公开选择无效";
            return StageErrCode::FAILED;
        }
        actual_number_ = actual_number;
        lie_number_ = lie_number;
        return StageErrCode::CHECKOUT;
    }

    const PlayerID questioner_;
    int& actual_number_;
    int& lie_number_;
};

class GuessStage : public SubGameStage<>
{
   public:
    GuessStage(MainStage& main_stage, const PlayerID guesser)
            : StageFsm(main_stage, "猜测阶段",
                    MakeStageCommand(*this, "猜测", &GuessStage::Guess_, BoolChecker("质疑", "相信"))),
              guesser_(guesser),
              doubt_(false) // default value
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请" << At(guesser_) << "选择「相信」或「质疑」";
        Global().SetReady(1 - guesser_);
        Global().StartTimer(60);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (guesser_ == pid) {
            doubt_ = std::rand() % 2;
            return StageErrCode::CHECKOUT;
        }
        return StageErrCode::OK;
    }

    bool doubt() const { return doubt_; }

   private:
    AtomReqErrCode Guess_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool doubt)
    {
        if (pid != guesser_) {
            reply() << "[错误] 本回合您为提问者，无法猜测";
            return StageErrCode::FAILED;
        }
        doubt_ = doubt;
        return StageErrCode::CHECKOUT;
    }

    const PlayerID guesser_;
    bool doubt_;
};

class RoundStage : public SubGameStage<NumberStage, GuessStage>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round, const uint64_t questioner)
            : StageFsm(main_stage, "第" + std::to_string(round) + "回合"),
              questioner_(questioner),
              actual_number_(1), // default value
              lie_number_(1), // default value
              loser_(0)
    {}

    PlayerID loser() const { return loser_; }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        setter.Emplace<NumberStage>(Main(), questioner_, actual_number_, lie_number_);
    }

    virtual void NextStageFsm(NumberStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if (reason == CheckoutReason::BY_LEAVE) {
            return;
        }
        Global().Tell(questioner_) << (reason == CheckoutReason::BY_TIMEOUT ? "设置超时，视为" : "设置成功")
                          << "\n实际数字：" << actual_number_ << "\n提问数字：" << lie_number_;
        Global().Boardcast() << At(questioner_) << "提问数字" << lie_number_;
        setter.Emplace<GuessStage>(Main(), 1 - questioner_);
    }

    virtual void NextStageFsm(GuessStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        const bool doubt = sub_stage.doubt();
        if (reason == CheckoutReason::BY_TIMEOUT) {
            Global().Tell(questioner_) << "选择超时，默认为相信";
        }
        const bool suc = doubt ^ (actual_number_ == lie_number_);
        loser_ = suc ? questioner_ : PlayerID{1 - questioner_};
        // add(loser_, questioner_, actual_number_, is_lie)
        Main().table().Lose(MyTable::Result{loser_, questioner_, actual_number_, actual_number_ != lie_number_});
        auto boardcast = Global().Boardcast();
        boardcast << "实际数字为" << actual_number_ << "，"
                  << (doubt ? "怀疑" : "相信") << (suc ? "成功" : "失败") << "，"
                  << "玩家" << At(loser_) << "获得数字" << actual_number_ << "\n数字获得情况：\n"
                  << At(PlayerID(0)) << "：" << At(PlayerID(1));
        return;
    }

   private:
    const PlayerID questioner_;
    int actual_number_;
    int lie_number_;
    PlayerID loser_;
};

MainStage::MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , table_(utility.Options(), utility.ResourceDir())
        , questioner_(0)
        , round_(1)
{
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    table_.SetName(Global().PlayerAvatar(0, 30) + HTML_ESCAPE_SPACE + HTML_ESCAPE_SPACE + Global().PlayerName(0),
            Global().PlayerAvatar(1, 30) + HTML_ESCAPE_SPACE + HTML_ESCAPE_SPACE + Global().PlayerName(1));
    setter.Emplace<RoundStage>(*this, 1, std::rand() % 2);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    questioner_ = sub_stage.loser();
    if (JudgeOver()) {
        return;
    }
    setter.Emplace<RoundStage>(*this, ++round_, questioner_);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const
{
    return pid == questioner_ ? 0 : 1;
}

bool MainStage::JudgeOver()
{
    const bool is_over = table_.CheckOver(questioner_);
    Info_();
    return is_over;
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

