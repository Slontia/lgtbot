// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

#include "game_util/poker_squares.h"

namespace ps = lgtbot::game_util::poker_squares;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

// 0 indicates no max-player limits
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; }

// The default score multiple for the game. The value of 0 denotes a testing game.
// We recommend to increase the multiple by one for every 7~8 minutes the game lasts.
uint32_t Multiple(const CustomOptions& options) { return GET_OPTION_VALUE(options, 种子).empty() ? 2 : 0; }
const GameProperties k_properties { 
    .name_ = "扑克矩阵",
    .developer_ = "森高",
    .description_ = "将扑克放入 5 × 5 矩阵中的合适位置，尽可能在各横纵对角凑出更大牌型的游戏",
};

const MutableGenericOptions k_default_generic_options;

// The commands for showing more rules information. Users can get the information by "#规则 <game name> <rule command>...".
const std::vector<RuleCommand> k_rule_commands = {};

// The commands for initialize the options when starting a game by "#新游戏 <game name> <init options command>..."
const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 1;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// The function is invoked before a game starts. You can make final adaption for the options.
// The return value of false denotes failure to start a game.
bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    return true;
}

// ========== GAME STAGES ==========

constexpr int32_t k_red_bonus_num = 1;
constexpr int32_t k_black_bonus_num = 1;
constexpr int32_t k_double_score_num = 1;
constexpr int32_t k_skip_next_num = 2;
constexpr int32_t k_show_extra_cards_num = 2;
constexpr int32_t k_round_num = ps::k_square_size * ps::k_square_size + k_skip_next_num;

class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "设置卡牌", &MainStage::Fill_, AnyArg("位置字符", "d")))
        , cards_(
#ifdef TEST_BOT
                GAME_OPTION(洗牌)
#else
                true
#endif
                ? game_util::poker::ShuffledPokers<ps::k_card_type>(GAME_OPTION(种子)) : game_util::poker::UnshuffledPokers<ps::k_card_type>())
        , poker_squares_(MakePokerSquares_(Global().ResourceDir(), Global().PlayerNum(), GAME_OPTION(种子),
#ifdef TEST_BOT
                GAME_OPTION(洗牌)
#else
                true
#endif
                ))
    {
    }

    virtual void OnStageBegin() override
    {
        html_ = MakeHtml_();
        Global().Boardcast() << Markdown{html_};
        Global().Boardcast() << "游戏开始，请私信或公屏打出要放置的位置（单个小写字母）";
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        return CheckoutErrCode::Condition(FinishRound_(), CheckoutErrCode::CHECKOUT, CheckoutErrCode::CONTINUE);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        poker_squares_[pid].FillRandomly(*current_card_iter_);
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return CheckoutErrCode::Condition(FinishRound_(), CheckoutErrCode::CHECKOUT, CheckoutErrCode::CONTINUE);
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return ps::GetScore(poker_squares_[pid].GetStatistic()); }

  private:
    static std::array<ps::GridType, ps::k_grid_num> MakeGridTypes_(const std::string_view seed_sv, const bool shuffle)
    {
        std::array<ps::GridType, ps::k_grid_num> result;
        result.fill(ps::GridType::empty);
        const auto shuffled_indexes = [&seed_sv, shuffle]()
            {
                auto result = MakeArray<ps::k_grid_num>([](const auto i) { return i; });
                if (!shuffle) {
                    return result;
                }
                if (seed_sv.empty()) {
                    std::random_device rd;
                    std::mt19937 g(rd());
                    std::ranges::shuffle(result, g);
                } else {
                    std::seed_seq seed(seed_sv.begin(), seed_sv.end());
                    std::mt19937 g(seed);
                    std::ranges::shuffle(result, g);
                }
                return result;
            }();
        auto it = shuffled_indexes.begin();
        const auto set_grid_type = [&](const int32_t num, const ps::GridType type)
            {
                std::for_each(it, std::next(it, num), [&result, type](const auto i ) { result[i] = type; });
                it += num;
            };
        set_grid_type(k_red_bonus_num, ps::GridType::red_bonus);
        set_grid_type(k_black_bonus_num, ps::GridType::black_bonus);
        set_grid_type(k_double_score_num, ps::GridType::double_score);
        set_grid_type(k_skip_next_num, ps::GridType::skip_next);
        return result;
    }

    static std::vector<ps::PokerSquare> MakePokerSquares_(const std::string& resource_dir, const uint32_t player_num, const std::string_view seed_sv, const bool shuffle)
    {
        std::vector<ps::PokerSquare> result;
        result.reserve(player_num);
        const auto grid_types = MakeGridTypes_(seed_sv, shuffle);
        std::ranges::for_each(std::views::iota(0U, player_num),
                [&](...) { result.emplace_back(resource_dir, grid_types); });
        return result;
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown{html_};
        return StageErrCode::OK;
    }

    AtomReqErrCode Fill_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& position)
    {
        if (Global().IsReady(pid)) {
            reply() << "执行失败：您正处于过牌状态或已经完成行动";
            return StageErrCode::FAILED;
        }
        if (position.size() != 1 || position[0] < 'a' || position[0] > 'y') {
            reply() << "执行失败：非法的输入「" << position << "」，期望为单个 a ~ y 范围的字符";
            return StageErrCode::FAILED;
        }
        const auto old_score = ps::GetScore(poker_squares_[pid].GetStatistic());
        if (!poker_squares_[pid].Fill(position[0] - 'a', *current_card_iter_)) {
            reply() << "执行失败：当前位置无法放置卡牌";
            return StageErrCode::FAILED;
        }
        const auto score_variation = ps::GetScore(poker_squares_[pid].GetStatistic()) - old_score;
        if (score_variation > 0) {
            reply() << "放置卡牌成功，本次操作收获 " << score_variation << " 分";
        } else {
            reply() << "放置卡牌成功";
        }
        return StageErrCode::READY;
    }

    // The returned value of true indicates the game has finished.
    bool FinishRound_()
    {
        // The order is important.
        ++round_;
        HandleUnreadyPlayers_();
        ++current_card_iter_;
        html_ = MakeHtml_();
        // Clearing ready should be performed after updating html because it will reset the skip_next state.
        ClearReadyForNonSkipNextPlayers_();
        Global().Boardcast() << Markdown{html_};
        if (round_ >= k_round_num) {
            if (GAME_OPTION(种子).empty()) {
                std::ranges::for_each(std::views::iota(0U, Global().PlayerNum()), [this](const PlayerID pid) { AddAchievements_(pid); });
            }
            return true;
        }
        Global().Boardcast() << "游戏继续，请非过牌状态玩家私信或公屏打出要放置的位置（单个小写字母）";
        Global().StartTimer(GAME_OPTION(时限));
        return false;
    }

    void AddAchievements_(const PlayerID pid)
    {
        const auto& square = poker_squares_[pid];
        const auto& statistic = square.GetStatistic();
        switch (statistic.pattern_type_bitset_.count() - statistic.pattern_type_bitset_[game_util::poker::PatternType::HIGH_CARD]) {
            case 8:
                Global().Achieve(pid, Achievement::专业收藏家);
            case 7:
                Global().Achieve(pid, Achievement::收藏家);
            case 6:
                Global().Achieve(pid, Achievement::入门收藏家);
        }
        if (GetScore(statistic) >= 1200) {
            Global().Achieve(pid, Achievement::渐入佳境);
        }
        if (GetScore(statistic) >= 1600) {
            Global().Achieve(pid, Achievement::得心应手);
        }
        if (GetScore(statistic) >= 2000) {
            Global().Achieve(pid, Achievement::出神入化);
        }
        if (GetScore(statistic) >= 2400) {
            Global().Achieve(pid, Achievement::天人合一);
        }
        bool found_high_card = false;
        bool found_roral_straight_flush = false;
        for (const auto& deck : square.GetDecks()) {
            found_high_card |= deck.type_ == game_util::poker::PatternType::HIGH_CARD;
            found_roral_straight_flush |= game_util::poker::IsRoralStraightFlush(deck);
        }
        if (!found_high_card) {
            Global().Achieve(pid, Achievement::左右逢源);
        }
        if (found_roral_straight_flush) {
            Global().Achieve(pid, Achievement::皇家同花顺);
        }
    }

    void HandleUnreadyPlayers_()
    {
        const auto defaultly_fill = [this](const PlayerID pid)
            {
                auto& square = poker_squares_[pid];
                const auto old_score = ps::GetScore(square.GetStatistic());
                auto sender = Global().Boardcast();
                sender << Global().PlayerName(pid) << "因为未做选择，自动填入空位置 " << static_cast<char>('a' + square.FillInOrder(*current_card_iter_));
                const auto score_variation = ps::GetScore(square.GetStatistic()) - old_score;
                if (score_variation > 0) {
                    sender << "，意外收获 " << score_variation << " 点积分";
                }
            };
        const auto is_unready = [this](const PlayerID pid) { return !Global().IsReady(pid); };
        std::ranges::for_each(std::views::iota(0U, Global().PlayerNum()) | std::views::filter(is_unready), defaultly_fill);
        Global().HookUnreadyPlayers();
    }

    void ClearReadyForNonSkipNextPlayers_()
    {
        Global().ClearReady();
        const auto skip_next = [this](const PlayerID pid) { return poker_squares_[pid].ResetSkipNext(); };
        std::ranges::for_each(std::views::iota(0U, Global().PlayerNum()) | std::views::filter(skip_next),
                [this](const PlayerID pid) { return Global().SetReady(pid); });
    }

    std::string MakeHtml_() const
    {
        std::string str = "## 第 " + std::to_string(round_ + 1) + " 回合\n\n";
        str += "<div align=\"center\">\n\n";

        // append current cards
        const auto append_card = [&](const ps::Card card, const int32_t size)
            {
                str += "<img src=\"file:///";
                str += Global().ResourceDir();
                str += ps::GetCardImgName(card).data();
                str += ".png\" style=\"width:";
                str += std::to_string(size);
                str += "px; height:";
                str += std::to_string(size);
                str += "px;\"/>";
            };
        str += "### 当前卡牌： ";
        append_card(*current_card_iter_, 64);
        for (uint32_t i = 0; i < k_show_extra_cards_num; ++i) {
            append_card(*(current_card_iter_ + 1 + i), 48);
        }

        { // append pattern scores info
            html::Table table(2, game_util::poker::PatternType::Count());
            table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"5\" cellspacing=\"1\" ");
            int i = 0;
            for (auto pattern : game_util::poker::PatternType::Members()) {
                table.Get(0, i).SetContent(game_util::poker::GetPatternTypeName(pattern));
                table.Get(1, i).SetContent(std::to_string(ps::GetPatternPoint(pattern)));
                i++;
            }
            str += "\n\n**各牌型对应点数**\n\n";
            str += table.ToString();
        }

        { // append grid type info
            html::Table table(0, 4);
            table.SetTableStyle(" align=\"center\" border=\"1px solid #ccc\" cellpadding=\"5\" cellspacing=\"1\" ");
            bool append_row = true;
            const auto append_grid_type_info =
                [&table, resource_dir = Global().ResourceDir(), &append_row]
                    (const ps::GridType grid_type, const char* const description)
                {
                    if (append_row) {
                        table.AppendRow();
                    }
                    append_row = !append_row;
                    table.GetLastRow(append_row * 2).SetContent(
                            std::string("<img src=\"file:///") + resource_dir + grid_type.ToString() +
                            ".png\" style=\"height:40px; width=40px;\">");
                    table.GetLastRow(append_row * 2 + 1).SetContent(std::string(HTML_SIZE_FONT_HEADER(2)) + description + HTML_FONT_TAIL);
                };
            append_grid_type_info(ps::GridType::black_bonus, "放置「黑桃」或「梅花」卡牌时，获得「20 × 卡牌点数」分");
            append_grid_type_info(ps::GridType::red_bonus, "放置「红桃」或「方板」卡牌时，获得「20 × 卡牌点数」分");
            append_grid_type_info(ps::GridType::double_score, "本回合完成的牌型，得分倍率加 1");
            append_grid_type_info(ps::GridType::skip_next, "跳过下一张卡牌的放置");
            str += "\n\n**各放置点特殊效果**\n\n";
            str += table.ToString();
        }

        str += "</div>";

        { // append squares
            html::Table table(poker_squares_.size() / 2 + 1, 2);
            table.SetTableStyle(" align=\"center\" cellpadding=\"15\" cellspacing=\"0\"");
            for (PlayerID pid = 0; pid.Get() < poker_squares_.size(); ++pid) {
                table.Get(pid / 2, pid % 2).SetContent("### " + Global().PlayerAvatar(pid, 40) + "&nbsp;&nbsp; " + Global().PlayerName(pid) +
                        + "\n\n" + poker_squares_[pid].ToHtml());
            }
            if (poker_squares_.size() % 2) {
                table.MergeRight(table.Row() - 1, 0, 2);
            }
            str += table.ToString();
        }
        return str;
    }

    int round_{0};
    std::array<ps::Card, ps::k_card_num> cards_;
    decltype(cards_)::iterator current_card_iter_{cards_.begin()};
    std::vector<ps::PokerSquare> poker_squares_;
    std::string html_;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

