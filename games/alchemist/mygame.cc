// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "game_util/alchemist.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "炼金术士",
    .developer_ = "森高",
    .description_ = "通过放置卡牌，让卡牌连成直线获得积分，比拼分数高低的游戏",
};

uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; } /* 0 means no max-player limits */
uint32_t Multiple(const CustomOptions& options)
{
    return GET_OPTION_VALUE(options, 种子).empty() ? std::min(2U, GET_OPTION_VALUE(options, 回合数) / 10) : 0;
}
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

static int WinScoreThreshold(const bool mode) { return mode ? 200 : 10; }

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    const auto card_num = GET_OPTION_VALUE(game_options, 颜色) * GET_OPTION_VALUE(game_options, 点数) * GET_OPTION_VALUE(game_options, 副数);
    if (GET_OPTION_VALUE(game_options, 回合数) > card_num && GET_OPTION_VALUE(game_options, 副数) > 0) {
        reply() << "回合数" << GET_OPTION_VALUE(game_options, 回合数) << "不能大于卡片总数量" << card_num;
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 1;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

const std::array<game_util::alchemist::Color, 6> k_colors {
    game_util::alchemist::Color::RED,
    game_util::alchemist::Color::BLUE,
    game_util::alchemist::Color::YELLOW,
    game_util::alchemist::Color::GREY,
    game_util::alchemist::Color::ORANGE,
    game_util::alchemist::Color::PURPLE
};

const std::array<game_util::alchemist::Point, 6> k_points {
    game_util::alchemist::Point::ONE,
    game_util::alchemist::Point::TWO,
    game_util::alchemist::Point::THREE,
    game_util::alchemist::Point::FOUR,
    game_util::alchemist::Point::FIVE,
    game_util::alchemist::Point::SIX,
};

struct Player
{
    Player(std::string resource_path, const int style) : score_(0), board_(new game_util::alchemist::Board(std::move(resource_path), style)) {}
    Player(Player&&) = default;
    int32_t score_;
    std::unique_ptr<game_util::alchemist::Board> board_;
};

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , round_(0)
    {
        for (uint64_t i = 0; i < Global().PlayerNum(); ++i) {
            players_.emplace_back(Global().ResourceDir(), GAME_OPTION(模式));
        }

        const std::string& seed_str = GAME_OPTION(种子);
        std::variant<std::random_device, std::seed_seq> rd;
        std::mt19937 g([&]
            {
                if (seed_str.empty()) {
                    auto& real_rd = rd.emplace<std::random_device>();
                    return std::mt19937(real_rd());
                } else {
                    auto& real_rd = rd.emplace<std::seed_seq>(seed_str.begin(), seed_str.end());
                    return std::mt19937(real_rd);
                }
            }());

        if (GAME_OPTION(副数) > 0) {
            for (uint32_t color_idx = 0; color_idx < GAME_OPTION(颜色); ++color_idx) {
                for (uint32_t point_idx = 0; point_idx < GAME_OPTION(点数); ++point_idx) {
                    for (uint32_t i = 0; i < GAME_OPTION(副数); ++i) {
                        cards_.emplace_back(std::in_place, k_colors[color_idx], k_points[point_idx]);
                    }
                }
            }
            for (uint32_t i = 0; i < GAME_OPTION(副数); ++i) {
                cards_.emplace_back(std::nullopt); // add stone
            }
            std::shuffle(cards_.begin(), cards_.end(), g);
        } else {
            std::uniform_int_distribution<uint32_t> distrib(0, GAME_OPTION(颜色) * GAME_OPTION(点数));
            for (uint32_t i = 0; i < GAME_OPTION(回合数); ++i) {
                const auto k = distrib(g);
                if (k == 0) {
                    cards_.emplace_back(std::nullopt); // add stone
                } else {
                    cards_.emplace_back(std::in_place, k_colors[(k - 1) % GAME_OPTION(颜色)],
                            k_points[(k - 1) / GAME_OPTION(点数)]);
                }
            }
        }

        std::uniform_int_distribution<uint32_t> distrib(0, game_util::alchemist::Board::k_size - 1);
        const auto set_stone = [this](const uint32_t row, const uint32_t col)
            {
                for (auto& player : players_) {
                    player.board_->SetStone(row, col);
                }
            };
        const auto row_1 = distrib(g);
        const auto col_1 = distrib(g);
        auto row_2 = distrib(g);
        auto col_2 = distrib(g);
        while (row_1 == row_2 && col_1 == col_2) {
            row_2 = distrib(g);
            col_2 = distrib(g);
        }
        set_stone(row_1, col_1);
        set_stone(row_2, col_2);
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;

    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override
    {
        return players_[pid].score_;
    }

    std::string BoardHtml(std::string str)
    {
        html::Table table(players_.size() / 2 + 1, 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"20\" cellspacing=\"0\" ");
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            table.Get(pid / 2, pid % 2).SetContent("\n\n### " + Global().PlayerAvatar(pid, 40) + "&nbsp;&nbsp; " + Global().PlayerName(pid) +
                    "\n\n### " HTML_COLOR_FONT_HEADER(green) "当前积分：" +
                    std::to_string(players_[pid].score_) + " / " +
                    std::to_string(WinScoreThreshold(GAME_OPTION(模式))) + HTML_FONT_TAIL "\n\n" +
                    players_[pid].board_->ToHtml());
        }
        if (players_.size() % 2) {
            table.MergeRight(table.Row() - 1, 0, 2);
        }
        return str + table.ToString();
    }

    std::vector<Player> players_;

  private:
    void NewStage_(SubStageFsmSetter& setter);

    void MatchOver_();

    uint32_t round_;
    std::vector<std::optional<game_util::alchemist::Card>> cards_;
};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round, const std::optional<game_util::alchemist::Card>& card)
            : StageFsm(main_stage, "第" + std::to_string(round) + "回合",
                MakeStageCommand(*this, "查看本回合开始时盘面情况，可用于图片重发", &RoundStage::Info_, VoidChecker("赛况")),
                MakeStageCommand(*this, "跳过该回合行动", &RoundStage::Pass_, VoidChecker("pass")),
                MakeStageCommand(*this, "设置卡片", &RoundStage::Set_, AnyArg("坐标", "C5")))
            , card_(card)
            , board_html_(main_stage.BoardHtml("## 第 " + std::to_string(round) + " / " + std::to_string(GAME_OPTION(回合数)) + " 回合"))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "本回合卡片为 " << (card_.has_value() ? card_->String() : "骷髅") << "，请公屏或私信裁判设置坐标：";
        SendInfo(Global().BoardcastMsgSender());
        Global().StartTimer(GAME_OPTION(局时));
    }

  private:
    CheckoutErrCode OnStageTimeout()
    {
        Global().HookUnreadyPlayers();
        return StageErrCode::CHECKOUT;
    }

    std::optional<std::pair<uint32_t, uint32_t>> ToCoor(MsgSenderBase& reply, const std::string& coor_str)
    {
        if (coor_str.size() != 2) {
            reply() << "[错误] 非法的坐标长度 " << coor_str.size() << " ，应为 2";
            return std::nullopt;
        }
        const char row_c = coor_str[0];
        const char col_c = coor_str[1];
        std::pair<uint32_t, uint32_t> coor;
        if ('A' <= row_c && row_c < 'A' + game_util::alchemist::Board::k_size) {
            coor.first = row_c - 'A';
        } else if ('a' <= row_c && row_c < 'a' + game_util::alchemist::Board::k_size) {
            coor.first = row_c - 'a';
        } else {
            reply() << "[错误] 非法的横坐标「" << row_c << "」，应在 A 和 "
                    << static_cast<char>('A' + game_util::alchemist::Board::k_size - 1) << " 之间";
            return std::nullopt;
        }
        if ('1' <= col_c && col_c < '1' + game_util::alchemist::Board::k_size) {
            coor.second = col_c - '1';
        } else {
            reply() << "[错误] 非法的纵坐标「" << col_c << "」，应在 1 和 "
                    << static_cast<char>('1' + game_util::alchemist::Board::k_size - 1) << " 之间";
            return std::nullopt;
        }
        return coor;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& coor_str)
    {
        auto& player = Main().players_[pid];
        if (Global().IsReady(pid)) {
            reply() << "您已经设置过，无法重复设置";
            return StageErrCode::FAILED;
        }
        const auto coor = ToCoor(reply, coor_str);
        if (!coor.has_value()) {
            return StageErrCode::FAILED;
        }
        if (card_.has_value()) {
            const auto ret = player.board_->SetOrClearLine(coor->first, coor->second, *card_, GAME_OPTION(模式));
            if (ret == game_util::alchemist::FAIL_ALREADY_SET) {
                reply() << "[错误] 该位置已被占用，试试其它位置吧";
                return StageErrCode::FAILED;
            } else if (ret == game_util::alchemist::FAIL_NON_ADJ_CARDS) {
                reply() << "[错误] 该位置旁边没有符文或石头，不允许空放，试试其它位置吧";
                return StageErrCode::FAILED;
            } else if (ret == game_util::alchemist::FAIL_ADJ_CARDS_MISMATCH) {
                reply() << "[错误] 该位置相邻符文非法，须满足颜色和点数至少一种相同，试试其它位置吧";
                return StageErrCode::FAILED;
            } else if (ret == 0) {
                reply() << "设置成功！";
                return StageErrCode::READY;
            } else {
                reply() << "设置成功！本次操作收获 " << ret << " 点积分";
                player.score_ += ret;
                return StageErrCode::READY;
            }
        } else {
            if (player.board_->Unset(coor->first, coor->second)) {
                reply() << "清除成功！";
                return StageErrCode::READY;
            } else {
                reply() << "[错误] 清除失败，该位置为空，试试其它位置吧";
                return StageErrCode::FAILED;
            }
        }
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << "您选择跳过该回合";
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        SendInfo(reply);
        return StageErrCode::OK;
    }

    std::string ImageName() const
    {
        return card_.has_value() ? card_->ImageName() : "erase";
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown(board_html_);
        sender() << Image(std::string(Global().ResourceDir() + ImageName()) + ".png");
    }

    const std::optional<game_util::alchemist::Card> card_;
    const std::string board_html_;
};

void MainStage::NewStage_(SubStageFsmSetter& setter)
{
    const auto round = round_;
    const auto& card = cards_[round];
    setter.Emplace<RoundStage>(*this, ++round_, card);
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    return NewStage_(setter);
}

void MainStage::MatchOver_()
{
    Global().Boardcast() << Markdown(BoardHtml("## 终局"));
    if (!GAME_OPTION(种子).empty() || GAME_OPTION(颜色) < 6 ||
            GAME_OPTION(点数) < 6) {
        return; // in this case, we do not save achievement
    }
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        const auto result = players_[pid].board_->GetAreaStatistic();
        if (result.card_count_ == 0 && result.stone_count_ == 0 &&
                players_[pid].score_ >= WinScoreThreshold(GAME_OPTION(模式))) {
            Global().Achieve(pid, Achievement::片甲不留);
        }
        if (result.stone_count_ == 2 && players_[pid].score_ >= WinScoreThreshold(GAME_OPTION(模式))) {
            Global().Achieve(pid, Achievement::纹丝不动);
        }
        if (!GAME_OPTION(模式) && players_[pid].score_ >= 12) {
            Global().Achieve(pid, Achievement::超额完成（经典）);
        }
        if (GAME_OPTION(模式) && players_[pid].score_ >= 220) {
            Global().Achieve(pid, Achievement::超额完成（竞技）);
        }
    }
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (round_ == GAME_OPTION(回合数)) {
        Global().Boardcast() << "游戏达到最大回合数，游戏结束";
        MatchOver_();
        return;
    }
    if (std::any_of(players_.begin(), players_.end(),
                [&](const Player& player) { return player.score_ >= WinScoreThreshold(GAME_OPTION(模式)); })) {
        Global().Boardcast() << "有玩家达到胜利分数，游戏结束";
        MatchOver_();
        return;
    }
    return NewStage_(setter);
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

