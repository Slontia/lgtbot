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

#include "game_framework/game_stage.h"
#include "utility/html.h"
#include "game_util/alchemist.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

const std::string k_game_name = "炼金术士";
const uint64_t k_max_player = 0; /* 0 means no max-player limits */
const uint64_t k_multiple = 1;
const std::string k_developer = "森高";
const std::string k_description = "通过放置卡牌，让卡牌连成直线获得积分，比拼分数高低的游戏";

static int WinScoreThreshold(const bool mode) { return mode ? 200 : 10; }

std::string GameOption::StatusInfo() const
{
    std::string str = std::string("\n「") + (GET_VALUE(模式) ? "竞技" : "经典") + "」模式\n每回合" +
        std::to_string(GET_VALUE(局时)) + "秒\n当有玩家达到" +
        std::to_string(WinScoreThreshold(GET_VALUE(模式))) + "分，或游戏已经进行了" +
        std::to_string(GET_VALUE(回合数)) + "回合时，游戏结束\n卡片包含" +
        std::to_string(GET_VALUE(颜色)) + "种颜色和" +
        std::to_string(GET_VALUE(点数)) + "种点数，每种相同卡片共有" +
        (GET_VALUE(副数) == 0 ? "无数" : " " + std::to_string(GET_VALUE(副数)) + " ") + "张\n";
    if (GET_VALUE(种子).empty()) {
        str += "未指定种子";
    } else {
        str += "种子：" + GET_VALUE(种子);
    }
    return str;
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    const auto card_num = GET_VALUE(颜色) * GET_VALUE(点数) * GET_VALUE(副数);
    if (GET_VALUE(回合数) > card_num && GET_VALUE(副数) > 0) {
        reply() << "回合数" << GET_VALUE(回合数) << "不能大于卡片总数量" << card_num;
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 1; }

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
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match)
        , round_(0)
    {
        for (uint64_t i = 0; i < option.PlayerNum(); ++i) {
            players_.emplace_back(option.ResourceDir(), GET_OPTION_VALUE(option, 模式));
        }

        const std::string& seed_str = GET_OPTION_VALUE(option, 种子);
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

        if (GET_OPTION_VALUE(option, 副数) > 0) {
            for (uint32_t color_idx = 0; color_idx < GET_OPTION_VALUE(option, 颜色); ++color_idx) {
                for (uint32_t point_idx = 0; point_idx < GET_OPTION_VALUE(option, 点数); ++point_idx) {
                    for (uint32_t i = 0; i < GET_OPTION_VALUE(option, 副数); ++i) {
                        cards_.emplace_back(std::in_place, k_colors[color_idx], k_points[point_idx]);
                    }
                }
            }
            for (uint32_t i = 0; i < GET_OPTION_VALUE(option, 副数); ++i) {
                cards_.emplace_back(std::nullopt); // add stone
            }
            std::shuffle(cards_.begin(), cards_.end(), g);
        } else {
            std::uniform_int_distribution<uint32_t> distrib(0, GET_OPTION_VALUE(option, 颜色) * GET_OPTION_VALUE(option, 点数));
            for (uint32_t i = 0; i < GET_OPTION_VALUE(option, 回合数); ++i) {
                const auto k = distrib(g);
                if (k == 0) {
                    cards_.emplace_back(std::nullopt); // add stone
                } else {
                    cards_.emplace_back(std::in_place, k_colors[(k - 1) % GET_OPTION_VALUE(option, 颜色)],
                            k_points[(k - 1) / GET_OPTION_VALUE(option, 点数)]);
                }
            }
        }

        // TODO: handle fushu is zero

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

    virtual VariantSubStage OnStageBegin() override;

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override
    {
        return players_[pid].score_;
    }

    std::string BoardHtml(std::string str)
    {
        html::Table table(players_.size() / 2 + 1, 2);
        table.SetTableStyle(" align=\"center\" cellpadding=\"20\" cellspacing=\"0\" ");
        for (PlayerID pid = 0; pid < players_.size(); ++pid) {
            table.Get(pid / 2, pid % 2).SetContent("\n\n### " + PlayerAvatar(pid, 40) + "&nbsp;&nbsp; " + PlayerName(pid) +
                    "\n\n### " HTML_COLOR_FONT_HEADER(green) "当前积分：" +
                    std::to_string(players_[pid].score_) + " / " +
                    std::to_string(WinScoreThreshold(GET_OPTION_VALUE(option(), 模式))) + HTML_FONT_TAIL "\n\n" +
                    players_[pid].board_->ToHtml());
        }
        if (players_.size() % 2) {
            table.MergeRight(table.Row() - 1, 0, 2);
        }
        return str + table.ToString();
    }

    std::vector<Player> players_;

  private:
    VariantSubStage NewStage_();

    void MatchOver_();

    uint32_t round_;
    std::vector<std::optional<game_util::alchemist::Card>> cards_;
};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round, const std::optional<game_util::alchemist::Card>& card)
            : GameStage(main_stage, "第" + std::to_string(round) + "回合",
                MakeStageCommand("查看本回合开始时盘面情况，可用于图片重发", &RoundStage::Info_, VoidChecker("赛况")),
                MakeStageCommand("跳过该回合行动", &RoundStage::Pass_, VoidChecker("pass")),
                MakeStageCommand("设置卡片", &RoundStage::Set_, AnyArg("坐标", "C5")))
            , card_(card)
            , board_html_(main_stage.BoardHtml("## 第 " + std::to_string(round) + " / " + std::to_string(GET_OPTION_VALUE(option(), 回合数)) + " 回合"))
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "本回合卡片如下，请公屏或私信裁判设置坐标：";
        SendInfo(BoardcastMsgSender());
        StartTimer(GET_OPTION_VALUE(option(), 局时));
    }

  private:
    CheckoutErrCode OnTimeout()
    {
        HookUnreadyPlayers();
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
        auto& player = main_stage().players_[pid];
        if (IsReady(pid)) {
            reply() << "您已经设置过，无法重复设置";
            return StageErrCode::FAILED;
        }
        const auto coor = ToCoor(reply, coor_str);
        if (!coor.has_value()) {
            return StageErrCode::FAILED;
        }
        if (card_.has_value()) {
            const auto ret = player.board_->SetOrClearLine(coor->first, coor->second, *card_, GET_OPTION_VALUE(option(), 模式));
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

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown(board_html_);
        sender() << Image(std::string(option().ResourceDir() + (card_.has_value() ? card_->ImageName() : "erase")) + ".png");
    }

    const std::optional<game_util::alchemist::Card> card_;
    const std::string board_html_;
};

MainStage::VariantSubStage MainStage::NewStage_()
{
    const auto round = round_;
    const auto& card = cards_[round];
    return std::make_unique<RoundStage>(*this, ++round_, card);
}

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    return NewStage_();
}

void MainStage::MatchOver_()
{
    Boardcast() << Markdown(BoardHtml("## 终局"));
    if (!GET_OPTION_VALUE(option(), 种子).empty() || GET_OPTION_VALUE(option(), 颜色) < 6 ||
            GET_OPTION_VALUE(option(), 点数) < 6) {
        return; // in this case, we do not save achievement
    }
    for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
        const auto result = players_[pid].board_->GetAreaStatistic();
        if (result.card_count_ == 0 && result.stone_count_ == 0 &&
                players_[pid].score_ >= WinScoreThreshold(GET_OPTION_VALUE(option(), 模式))) {
            global_info().Achieve(pid, Achievement::片甲不留);
        }
        if (result.stone_count_ == 2 && players_[pid].score_ >= WinScoreThreshold(GET_OPTION_VALUE(option(), 模式))) {
            global_info().Achieve(pid, Achievement::纹丝不动);
        }
        if (!GET_OPTION_VALUE(option(), 模式) && players_[pid].score_ >= 12) {
            global_info().Achieve(pid, Achievement::超额完成（经典）);
        }
        if (GET_OPTION_VALUE(option(), 模式) && players_[pid].score_ >= 220) {
            global_info().Achieve(pid, Achievement::超额完成（竞技）);
        }
    }
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if (round_ == GET_OPTION_VALUE(option(), 回合数)) {
        Boardcast() << "游戏达到最大回合数，游戏结束";
        MatchOver_();
        return {};
    }
    if (std::any_of(players_.begin(), players_.end(),
                [&](const Player& player) { return player.score_ >= WinScoreThreshold(GET_OPTION_VALUE(option(), 模式)); })) {
        Boardcast() << "有玩家达到胜利分数，游戏结束";
        MatchOver_();
        return {};
    }
    return NewStage_();
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
