// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <numeric>
#include <random>

#include "game_framework/game_stage.h"
#include "utility/html.h"
#include "game_util/unity_chess.h"
#include "utility/coding.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "喷色战士"; // the game name which should be unique among all the games
const uint64_t k_max_player = game_util::unity_chess::k_max_player; // 0 indicates no max-player limits
const uint64_t k_multiple = 0; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "森高";
const std::string k_description = "同时落子的通过三连珠进行棋盘染色的棋类游戏";
const std::vector<RuleCommand> k_rule_commands = {};

std::string GameOption::StatusInfo() const { return ""; }

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 3; }

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand("放弃一手落子", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                    &MainStage::Pass_, VoidChecker("pass")),
                MakeStageCommand("落子", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                    &MainStage::Set_, AnyArg("坐标")))
        , board_(GET_OPTION_VALUE(option, 尺寸),
                BonusCoordinates_(GET_OPTION_VALUE(option, 奖励格百分比), GET_OPTION_VALUE(option, 尺寸)),
                option.ResourceDir())
        , player_scores_{0}
        , round_(0)
        , any_player_set_chess_(false)
    {
    }

    virtual void OnStageBegin() override
    {
        StoreHtml_(board_.Settlement());
        Boardcast() << Markdown(html_);
        Boardcast() << "游戏开始，请所有玩家私信裁判坐标（如「B3」）或「pass」以行动";
        StartTimer(GET_OPTION_VALUE(option(), 时限));
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
        Boardcast() << Markdown(html_);
        if (result.available_area_count_ == 0) {
            Boardcast() << "棋盘无空余可落子位置，游戏结束";
            return true;
        }
        if (++round_ == GET_OPTION_VALUE(option(), 回合数)) {
            Boardcast() << "达到最大回合数限制，游戏结束";
            return true;
        }
        if (!any_player_set_chess_) {
            Boardcast() << "所有玩家选择了 pass，游戏结束";
            return true;
        }
        any_player_set_chess_ = false;
        Boardcast() << "游戏继续，请所有玩家私信裁判坐标（如「B3」）或「pass」以行动";
        ClearReady();
        StartTimer(GET_OPTION_VALUE(option(), 时限));
        return false;
    }

    void StoreHtml_(const game_util::unity_chess::SettlementResult& result) {
        html::Table table(2, option().PlayerNum() * 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"3\" ");
        for (auto player_id = 0; player_id < option().PlayerNum(); ++player_id) {
            table.MergeDown(0, player_id * 2, 2);
            char color_field[] = "000";
            color_field[player_id] = '1';
            table.Get(0, player_id * 2).SetContent(std::string("![](file:///") + option().ResourceDir() + "/0_" +
                    color_field + "_" + color_field + ".png)");
            table.Get(0, player_id * 2 + 1).SetContent(PlayerName(player_id));
            table.Get(1, player_id * 2 + 1).SetContent("占领格数：" HTML_COLOR_FONT_HEADER(red) +
                    std::to_string(result.color_counts_[player_id] / 2) + (result.color_counts_[player_id] % 2 ? ".5" : "") + HTML_FONT_TAIL);
        }
        html_ = "## 第 " + std::to_string(round_ + 1) + " / " + std::to_string(GET_OPTION_VALUE(option(), 回合数)) +
            " 回合\n\n" + table.ToString() + "\n<br>\n" + result.html_;
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        Boardcast() << "回合超时，下面公布结果";
        return CheckoutErrCode::Condition(Settlement_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        board_.RandomSet(pid);
        any_player_set_chess_ = true;
        return StageErrCode::READY;
    }

    virtual void OnAllPlayerReady() override
    {
        Boardcast() << "全员行动完毕，下面公布结果";
        Settlement_();
    }

    game_util::unity_chess::Board board_;
    std::array<uint32_t, game_util::unity_chess::k_max_player> player_scores_;
    std::string html_;
    int round_;
    bool any_player_set_chess_;
};

} // namespace lgtbot

} // namespace game

} // namespace lgtbot

#include "game_framework/make_main_stage.h"
