// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <numeric>
#include <random>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "game_util/unity_chess.h"
#include "utility/coding.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;


const GameProperties k_properties {
    .name_ = "喷色战士", // the game name which should be unique among all the games
    .developer_ = "森高",
    .description_ = "同时落子的通过三连珠进行棋盘染色的棋类游戏",
};
const uint64_t k_max_player = game_util::unity_chess::k_max_player; // 0 indicates no max-player limits
uint64_t MaxPlayerNum(const CustomOptions& options) { return k_max_player; }
uint32_t Multiple(const CustomOptions& options) { return 2; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 3;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "放弃一手落子", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                    &MainStage::Pass_, VoidChecker("pass")),
                MakeStageCommand(*this, "落子", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                    &MainStage::Set_, AnyArg("坐标")))
        , board_(GAME_OPTION(尺寸),
                BonusCoordinates_(GAME_OPTION(奖励格百分比), GAME_OPTION(尺寸)),
                Global().ResourceDir())
        , player_scores_{0}
        , round_(0)
        , any_player_set_chess_(false)
    {
    }

    virtual void OnStageBegin() override
    {
        StoreHtml_(board_.Settlement());
        Global().Boardcast() << Markdown(html_);
        Global().Boardcast() << "游戏开始，请所有玩家私信裁判坐标（如「B3」）或「pass」以行动";
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

  private:
    std::vector<Coordinate> BonusCoordinates_(const uint32_t rate, const uint32_t size)
    {
        if (rate == 0) {
            return {};
        }
        std::vector<uint32_t> coordinates(size * size);
        std::iota(coordinates.begin(), coordinates.end(), 0);
        std::ranges::shuffle(coordinates, std::mt19937 {std::random_device {}()});

        const uint32_t bonus_count = size * size * rate / 100;
        std::vector<Coordinate> result(bonus_count);
        for (int i = 0; i < bonus_count; ++i) {
            result[i].row_ = coordinates[i] / size;
            result[i].col_ = coordinates[i] % size;
        }
        return result;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        const auto decode_res = DecodePos(str);
        if (const auto* const errstr = std::get_if<std::string>(&decode_res)) {
            reply() << "落子失败：" << *errstr;
            return StageErrCode::FAILED;
        }
        const auto [row, col] = std::get<std::pair<uint32_t, uint32_t>>(decode_res);
        if (!board_.Set(Coordinate(row, col), pid)) {
            reply() << "落子失败：" << str << " 位置无法落子，请试试其它位置吧";
            return StageErrCode::FAILED;
        }
        reply() << "落子成功";
        any_player_set_chess_ = true;
        return StageErrCode::READY;
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << "您放弃落子";
        return StageErrCode::READY;
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(html_);
        return StageErrCode::OK;
    }

    bool Settlement_()
    {
        const auto result = board_.Settlement();
        player_scores_ = std::move(result.color_counts_);
        StoreHtml_(result);
        Global().Boardcast() << Markdown(html_);
        if (result.available_area_count_ == 0) {
            Global().Boardcast() << "棋盘无空余可落子位置，游戏结束";
            return true;
        }
        if (++round_ == GAME_OPTION(回合数)) {
            Global().Boardcast() << "达到最大回合数限制，游戏结束";
            return true;
        }
        if (!any_player_set_chess_) {
            Global().Boardcast() << "所有玩家选择了 pass，游戏结束";
            return true;
        }
        any_player_set_chess_ = false;
        Global().Boardcast() << "游戏继续，请所有玩家私信裁判坐标（如「B3」）或「pass」以行动";
        Global().ClearReady();
        Global().StartTimer(GAME_OPTION(时限));
        return false;
    }

    void StoreHtml_(const game_util::unity_chess::SettlementResult& result) {
        html::Table table(2, Global().PlayerNum() * 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"3\" ");
        for (auto player_id = 0; player_id < Global().PlayerNum(); ++player_id) {
            table.MergeDown(0, player_id * 2, 2);
            char color_field[] = "000";
            color_field[player_id] = '1';
            table.Get(0, player_id * 2).SetContent(std::string("![](file:///") + Global().ResourceDir() + "/0_" +
                    color_field + "_" + color_field + ".png)");
            table.Get(0, player_id * 2 + 1).SetContent(Global().PlayerName(player_id));
            table.Get(1, player_id * 2 + 1).SetContent("占领格数：" HTML_COLOR_FONT_HEADER(red) +
                    std::to_string(result.color_counts_[player_id] / 2) + (result.color_counts_[player_id] % 2 ? ".5" : "") + HTML_FONT_TAIL);
        }
        html_ = "## 第 " + std::to_string(round_ + 1) + " / " + std::to_string(GAME_OPTION(回合数)) +
            " 回合\n\n" + table.ToString() + "\n<br>\n" + result.html_;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().Boardcast() << "回合超时，下面公布结果";
        return CheckoutErrCode::Condition(Settlement_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        board_.RandomSet(pid);
        any_player_set_chess_ = true;
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Global().Boardcast() << "全员行动完毕，下面公布结果";
        return CheckoutErrCode::Condition(Settlement_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    game_util::unity_chess::Board board_;
    std::array<uint32_t, game_util::unity_chess::k_max_player> player_scores_;
    std::string html_;
    int round_;
    bool any_player_set_chess_;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

