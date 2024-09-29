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

#include "bot_core/msg_sender.h"
#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "二择猜拳",
    .developer_ = "森高",
    .description_ = "伸出两拳，收回一拳的猜拳游戏",
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

char type2char[] = "rpsn";

inline int Compare(const Type _1, const Type _2)
{
    if (_1 == _2 || _1 == Type::BLANK || _2 == Type::BLANK) {
        return 0;
    } else if ((static_cast<int>(_1) + 1) % 3 == static_cast<int>(_2)) {
        return -1;
    } else {
        return 1;
    }
}

inline int Compare(const Card& _1, const Card& _2)
{
    if (const int ret = Compare(_1.type_, _2.type_); ret != 0) {
        return ret;
    } else {
        return _1.point_ - _2.point_;
    }
}

enum class CardState { UNUSED, USED, CHOOSED };

std::string CardName(const Card& card)
{
    static CardChecker checker;
    return checker.ArgString(card) + (card.has_star_ ? "（星卡）" : "");
}

static std::string StarsString(const uint32_t n) {
    std::string s;
    for (uint32_t i = 0; i < n; ++i) {
        s += "★";
    }
    return s;
}

using CardMap = std::map<Card, CardState>;

static CardMap GetCardMap(const CustomOptions& option)
{
    CardMap cards;
    const auto emplace_card = [&](const auto& card) {
        const auto& king_cards = GET_OPTION_VALUE(option, 星卡);
        cards.emplace(Card{card.type_, card.point_, std::ranges::find(king_cards, card) != std::end(king_cards)}, CardState::UNUSED);
    };
    if (GET_OPTION_VALUE(option, 手牌).has_value()) {
        for (const auto& card : *GET_OPTION_VALUE(option, 手牌)) {
            emplace_card(card);
        }
        return cards;
    }
    std::array<Card, 27> shuffled_cards {
        Card{ Type::ROCK, 0 },  Card{ Type::PAPER, 0 },  Card{ Type::SCISSOR, 0 },
        Card{ Type::ROCK, 1 },  Card{ Type::PAPER, 1 },  Card{ Type::SCISSOR, 1 },
        Card{ Type::ROCK, 3 },  Card{ Type::PAPER, 3 },  Card{ Type::SCISSOR, 3 },
        Card{ Type::ROCK, 4 },  Card{ Type::PAPER, 4 },  Card{ Type::SCISSOR, 4 },
        Card{ Type::ROCK, 6 },  Card{ Type::PAPER, 6 },  Card{ Type::SCISSOR, 6 },
        Card{ Type::ROCK, 7 },  Card{ Type::PAPER, 7 },  Card{ Type::SCISSOR, 7 },
        Card{ Type::ROCK, 9 },  Card{ Type::PAPER, 9 },  Card{ Type::SCISSOR, 9 },
        Card{ Type::ROCK, 10 }, Card{ Type::PAPER, 10 }, Card{ Type::SCISSOR, 10 },
        Card{ Type::BLANK, 2},  Card{ Type::BLANK, 5 },  Card{ Type::BLANK, 8 },
    };
    std::random_device rd;
    std::mt19937 g(rd());
    std::ranges::shuffle(shuffled_cards, g);
    uint32_t type_count[k_card_type_num] = {0};
    uint32_t max_num_each_type = 4;
    for (uint32_t i = 0; cards.size() < k_round_num; ++i) {
        const auto& card = shuffled_cards[i];
        const auto card_num = ++type_count[static_cast<int>(card.type_)];
        if (card_num <= max_num_each_type) {
            emplace_card(card);
        }
        if (card_num == max_num_each_type) {
            --max_num_each_type;
        }
    }
    return cards;
}

struct Player
{
    Player(CardMap cards)
        : cards_(std::move(cards))
        , win_count_(0)
        , score_(0)
        , stars_(0)
    {}

    CardMap cards_;
    CardMap::iterator left_;
    CardMap::iterator right_;
    CardMap::iterator alter_;
    int win_count_;
    int score_;
    int stars_;
};

template <bool INCLUDE_CHOOSED>
std::string AvailableCards(const CardMap& cards)
{
    std::string str;
    for (const auto& [card, state] : cards) {
        if (state == CardState::UNUSED || (INCLUDE_CHOOSED && state == CardState::CHOOSED)) {
            if (!str.empty()) {
                str += "、";
            }
            str += CardName(card);
        }
    }
    return str;
}

static const char* const k_player_name_color = "#7092BE";

static const char* const k_non_color_fg = "#bbbbbb";
static const char* const k_non_color_bg = "#e2e2e2";

static const char* const k_color_fgs[k_card_type_num] = {
    [static_cast<int>(Type::ROCK)] = "#e5686d",
    [static_cast<int>(Type::PAPER)] = "#84d599",
    [static_cast<int>(Type::SCISSOR)] = "#5ecef0",
    [static_cast<int>(Type::BLANK)] = "black",
};

static const char* const k_color_bgs[k_card_type_num] = {
    [static_cast<int>(Type::ROCK)] = "#f9d9e5",
    [static_cast<int>(Type::PAPER)] = "#dff276",
    [static_cast<int>(Type::SCISSOR)] = "#d2ecf4",
    [static_cast<int>(Type::BLANK)] = "white",
};

static void SetPointColor(html::Box& box, const int point, const char* const fg_color, const char* const bg_color, const bool with_star)
{
    box.SetContent(std::string("<font color=") + fg_color + ">" + " **" + (point == 0 ? "-" : std::to_string(point)) + (with_star ? "★" : "") + "** " HTML_FONT_TAIL);
    box.SetColor(bg_color);
    box.SetStyle(std::string("style=\"border: 2px solid ") + fg_color + ";\"");
}

static std::string ImageStr(const std::string& image_path, const std::string& name)
{
    return std::string("![](file:///") + image_path + "/" + name + ".png)";
}

class ThreeRoundTable
{
  public:
    ThreeRoundTable(std::string image_path) : image_path_(std::move(image_path)), table_(9, 8) {}

    void Init(std::string p1_name, std::string p2_name, const uint32_t base_round)
    {
        table_.SetTableStyle(" align=\"center\" cellspacing=\"3\" cellpadding=\"0\" ");
        // 0: p1 name
        table_.MergeRight(0, 0, 8);
        table_.Get(0, 0).SetContent(" **" + p1_name + "** ");
        table_.Get(0, 0).SetColor(k_player_name_color);
        table_.Get(1, 2).SetContent(HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE);
        table_.Get(1, 5).SetContent(HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE);
        for (uint32_t i = 0; i < 3; ++i) {
            // 1: p1 choose card
            table_.Get(1, i * 3).SetContent(Image_("empty"));
            table_.Get(1, i * 3 + 1).SetContent(Image_("empty"));
            // 2: p1 choose point
            table_.Get(2, i * 3).SetColor("#C3C3C3");
            table_.Get(2, i * 3 + 1).SetColor("#C3C3C3");
            // 3: p1 score
            table_.MergeRight(3, i * 3, 2);
            table_.Get(3, i * 3).SetContent("?");
            // 4: round
            table_.MergeRight(4, i * 3, 2);
            table_.Get(4, i * 3).SetContent(" **第 " + std::to_string(base_round + i) + " 回合** ");
            table_.Get(4, i * 3).SetColor(k_player_name_color);
            // 5: p2 score
            table_.MergeRight(5, i * 3, 2);
            table_.Get(5, i * 3).SetContent("?");
            // 6: p2 choose card
            table_.Get(6, i * 3).SetContent(Image_("empty"));
            table_.Get(6, i * 3 + 1).SetContent(Image_("empty"));
            // 7: p2 choose point
            table_.Get(7, i * 3).SetColor("#C3C3C3");
            table_.Get(7, i * 3 + 1).SetColor("#C3C3C3");
        }
        // 8: p2 name
        table_.MergeRight(8, 0, 8);
        table_.Get(8, 0).SetContent(" **" + p2_name + "** ");
        table_.Get(8, 0).SetColor(k_player_name_color);
    }

    void SetCard(const PlayerID pid, const uint32_t round_idx, const bool choose_idx, const Card& card,
            const bool with_color)
    {
        const uint32_t row = 1 + pid * 5;
        const uint32_t col = round_idx * 3 + choose_idx;
        table_.Get(row, col).SetContent(Image_(type2char[static_cast<int>(card.type_)] + std::to_string(with_color)));

        if (!with_color) {
            SetPointColor(table_.Get(row + 1, col), card.point_, k_non_color_fg, k_non_color_bg, card.has_star_);
        } else {
            SetPointColor(table_.Get(row + 1, col), card.point_, k_color_fgs[static_cast<int>(card.type_)],
                    k_color_bgs[static_cast<int>(card.type_)], card.has_star_);
        }
    }

    void SetScore(const PlayerID pid, const uint32_t round_idx, const int32_t old_score, const int32_t old_stars,
            const int32_t card_point, const bool card_has_star)
    {
        const uint32_t row = 3 + pid * 2;
        const uint32_t col = round_idx * 3;
        table_.Get(row, col)
              .SetContent(std::to_string(old_score) + StarsString(old_stars) + " →" HTML_COLOR_FONT_HEADER(green) " **" +
                          std::to_string(old_score + card_point) + StarsString(old_stars + card_has_star) + "** " + HTML_FONT_TAIL);
    }

    void SetScore(const PlayerID pid, const uint32_t round_idx, const int32_t score, const int32_t stars)
    {
        const uint32_t row = 3 + pid * 2;
        const uint32_t col = round_idx * 3;
        table_.Get(row, col).SetContent(" **" + std::to_string(score) + StarsString(stars) + "** ");
    }

    std::string ToHtml() const { return "<style> .bordered-cell { border: 2px solid black; } </style>" + table_.ToString(); }

  private:
    std::string Image_(const std::string& name) const { return ImageStr(image_path_, name); }

    const std::string image_path_;
    html::Table table_;
};

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
   public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看比赛情况", &MainStage::Info_, VoidChecker("赛况")))
        , k_origin_card_map_(GetCardMap(Global().Options()))
        , players_{k_origin_card_map_, k_origin_card_map_}
        , round_(1)
        , tables_{ThreeRoundTable(Global().ResourceDir()),
                  ThreeRoundTable(Global().ResourceDir()),
                  ThreeRoundTable(Global().ResourceDir())}
    {
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;

    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    int64_t PlayerScore(const PlayerID pid) const
    {
        return players_[pid].score_;
    }

    void Battle()
    {
        const int ret = Compare(players_[0].alter_->first,
                                players_[1].alter_->first);
        {
            auto sender = Global().Boardcast();
            sender << "最终选择：\n"
                << At(PlayerID(0)) << "：" << CardName(players_[0].alter_->first) << "\n"
                << At(PlayerID(1)) << "：" << CardName(players_[1].alter_->first) << "\n\n";
            if (ret == 0) {
                sender << "平局！";
                players_[0].win_count_ = 0;
                players_[1].win_count_ = 0;
                tables_[(round_ - 1) / 3].SetScore(0, (round_ - 1) % 3, players_[0].score_, players_[0].stars_);
                tables_[(round_ - 1) / 3].SetScore(1, (round_ - 1) % 3, players_[1].score_, players_[1].stars_);
            } else {
                const PlayerID winner = ret > 0 ? 0 : 1;
                const int winner_card_point = players_[winner].alter_->first.point_;
                const int loser_card_point = players_[1 - winner].alter_->first.point_;
                const int point = loser_card_point == 0 ? -winner_card_point : loser_card_point;
                const bool has_star = players_[1 - winner].alter_->first.has_star_;
                tables_[(round_ - 1) / 3].SetScore(1 - winner, (round_ - 1) % 3, players_[1 - winner].score_, players_[1 - winner].stars_);
                tables_[(round_ - 1) / 3].SetScore(winner, (round_ - 1) % 3, players_[winner].score_, players_[winner].stars_, point, has_star);
                ++(players_[winner].win_count_);
                players_[1 - winner].win_count_ = 0;
                players_[winner].score_ += point;
                sender << ::Name(winner) << "胜利，获得 " << point << " 分";
                if (has_star) {
                    sender << "和 1 颗星";
                    players_[winner].stars_++;
                }
            }
            sender << "\n\n当前分数：\n"
                << At(PlayerID(0)) << "：" << players_[0].score_ << StarsString(players_[0].stars_) << "\n"
                << At(PlayerID(1)) << "：" << players_[1].score_ << StarsString(players_[1].stars_);
        }
    }

    uint64_t round() const { return round_; }

    const CardMap k_origin_card_map_;
    std::array<Player, 2> players_;
    std::array<ThreeRoundTable, 3> tables_;

    std::string AvailableCardsImage_(const PlayerID pid) const
    {
        constexpr static int k_table_header_rows = 2;
        html::Table table(k_table_header_rows, k_card_type_num);
        table.SetTableStyle(" align=\"center\" cellspacing=\"3\" cellpadding=\"0\" ");
        table.MergeRight(0, 0, k_card_type_num);
        table.Get(0, 0).SetContent(Global().PlayerAvatar(pid, 30) + HTML_ESCAPE_SPACE + HTML_ESCAPE_SPACE " **" + Global().PlayerName(pid) + "**");
        table.Get(0, 0).SetColor("#4fa16b");
        bool is_remains[k_card_type_num] = {false, false, false, false};
        uint32_t counts[k_card_type_num] = {0, 0, 0, 0};
        for (const auto [card, state] : players_[pid].cards_) {
            auto& count = counts[static_cast<int>(card.type_)];
            if (table.Row() == k_table_header_rows + count) {
                table.AppendRow();
            }
            const bool used = (state == CardState::USED);
            const int type_idx = static_cast<int>(card.type_);
            SetPointColor(table.Get(k_table_header_rows + count, type_idx), card.point_,
                    used ? k_non_color_fg : k_color_fgs[static_cast<int>(card.type_)],
                    used ? k_non_color_bg : k_color_bgs[type_idx], card.has_star_);
            is_remains[type_idx] |= !used;
            ++count;
        }
        for (const auto type : {Type::ROCK, Type::SCISSOR, Type::PAPER, Type::BLANK}) {
            const int type_idx = static_cast<int>(type);
            table.Get(1, type_idx).SetContent(Image_(type2char[type_idx] + std::to_string(is_remains[type_idx])));
        }
        return table.ToString();
    }

    std::string Image_(const std::string& name) const { return ImageStr(Global().ResourceDir(), name); }

    void ShowInfo()
    {
        std::string str = CardInfoStr_();
        for (uint32_t i = 0; i < (round_ - 1) / 3 + 1; ++i) {
            str += "\n\n## 第 " + std::to_string(i * 3 + 1) + "~" + std::to_string(i * 3 + 3) + " 回合\n\n" + tables_[i].ToHtml();
        }
        Global().Boardcast() << Markdown(str);
    }

   private:
    std::string CardInfoStr_() const
    {
        html::Table user_card_info_table(1, 2);
        user_card_info_table.SetTableStyle(" align=\"center\" cellspacing=\"10\" cellpadding=\"0\" ");
        user_card_info_table.Get(0, 0).SetContent(AvailableCardsImage_(0));
        user_card_info_table.Get(0, 1).SetContent(AvailableCardsImage_(1));
        return "## 手牌使用情况\n\n" + user_card_info_table.ToString();
    }

    CompReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        auto sender = reply();
        const auto show_info = [&](const PlayerID pid)
            {
                const auto& player = players_[pid];
                sender << ::Name(pid) << "\n积分：" << player.score_
                                      << "\n连胜：" << player.win_count_
                                      << "\n可用卡牌：" << AvailableCards<true>(player.cards_);
            };
        show_info(0);
        sender << "\n\n";
        show_info(1);
        return StageErrCode::OK;
    }

    uint64_t round_;
};

template <bool IS_LEFT>
class ChooseStage : public SubGameStage<>
{
   public:
    ChooseStage(MainStage& main_stage)
            : StageFsm(main_stage, IS_LEFT ? "左拳阶段" : "右拳阶段",
                    MakeStageCommand(*this, "出拳", &ChooseStage::Choose_, CardChecker()))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请从手牌中选择一张私信给裁判，作为本次出牌";
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto& player = Main().players_[pid];
        std::vector<CardMap::iterator> its;
        for (auto it = player.cards_.begin(); it != player.cards_.end(); ++it) {
            if (it->second == CardState::UNUSED) {
                its.emplace_back(it);
            }
        }
        SetCard_(pid, its[rand() % its.size()]);

        return StageErrCode::READY;
    }

    void DefaultAct()
    {
        for (PlayerID pid = 0; pid < 2U; ++pid) {
            if (Global().IsReady(pid)) {
                continue;
            }
            auto& player = Main().players_[pid];
            for (auto it = player.cards_.begin(); it != player.cards_.end(); ++it) {
                if (it->second == CardState::UNUSED) {
                    // set default value: the first unused card
                    SetCard_(pid, it);
                    Global().Tell(pid) << "已自动为您选择：" << CardName(it->first);
                    break;
                }
            }
        }
    }

  private:
    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Card& card)
    {
        // TODO: test repeat choose
        auto& player = Main().players_[pid];
        auto& cards = Main().players_[pid].cards_;
        if (is_public) {
            reply() << "[错误] 请私信裁判选择";
            return StageErrCode::FAILED;
        }
        const auto it = cards.find(card);
        if (it == cards.end()) {
            reply() << "[错误] 预料外的卡牌，您目前可用的卡牌：\n" << AvailableCards<false>(cards);
            return StageErrCode::FAILED;
        }
        if (it->second != CardState::UNUSED) {
            reply() << "[错误] 该卡牌已被使用，请换一张，您目前可用的卡牌：\n" << AvailableCards<false>(cards);
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            PlayerCard(player)->second = CardState::UNUSED;
        }
        SetCard_(pid, it);
        reply() << "选择成功";
        return StageErrCode::READY;
    }

    void SetCard_(const PlayerID pid, const CardMap::iterator it)
    {
        it->second = CardState::CHOOSED;

        auto& player = Main().players_[pid];
        PlayerCard(player) = it;

        const auto round = Main().round();
        Main().tables_[(round - 1) / 3].SetCard(pid, (round - 1) % 3, !IS_LEFT, it->first, true);
    }

    CardMap::iterator& PlayerCard(Player& player) { return IS_LEFT ? player.left_ : player.right_; }
};

class AlterStage : public SubGameStage<>
{
   public:
    AlterStage(MainStage& main_stage)
            : StageFsm(main_stage, "二择阶段",
                    MakeStageCommand(*this, "选择决胜卡牌", &AlterStage::Alter_, CardChecker()))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请从两张卡牌中选择一张私信给裁判，作为用于本回合决胜的卡牌";
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto& player = Main().players_[pid];
        SetAlter_(pid, rand() % 2 ? player.left_ : player.right_);
        return StageErrCode::READY;
    }

    void DefaultAct()
    {
        for (PlayerID pid = 0; pid < 2U; ++pid) {
            if (Global().IsReady(pid)) {
                continue;
            }
            auto& player = Main().players_[pid];
            player.alter_ = player.left_;
            Global().Tell(pid) << "已自动为您选择左拳卡牌";
        }
    }

  private:
    AtomReqErrCode Alter_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Card& card)
    {
        // TODO: test repeat choose
        auto& player = Main().players_[pid];
        auto& cards = player.cards_;
        if (is_public) {
            reply() << "[错误] 请私信裁判选择";
            return StageErrCode::FAILED;
        } else if (const auto it = cards.find(card); it == cards.end()) {
            reply() << "[错误] 预料外的卡牌，请在您所选的卡牌间二选一：\n"
                    << CardName(player.left_->first) << "、" << CardName(player.right_->first);
            return StageErrCode::FAILED;
        } else if (it->second != CardState::CHOOSED) {
            reply() << "[错误] 您未选择这张卡牌，请在您所选的卡牌间二选一：\n"
                    << CardName(player.left_->first) << "、" << CardName(player.right_->first);
            return StageErrCode::FAILED;
        } else {
            SetAlter_(pid, it);
            reply() << "选择成功";
            return StageErrCode::READY;
        }
    }

    void SetAlter_(const PlayerID pid, const CardMap::iterator it)
    {
        auto& player = Main().players_[pid];
        player.alter_ = it;
        const bool choose_left = player.alter_ == player.left_;
        Main().tables_[(Main().round() - 1) / 3].SetCard(pid, (Main().round() - 1) % 3,
                choose_left, choose_left ? player.right_->first : player.left_->first, false);
    }
};

class RoundStage : public SubGameStage<ChooseStage<true>, ChooseStage<false>, AlterStage>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "第" + std::to_string(round) + "回合")
    {
        if ((round - 1) % 3 == 0) {
            main_stage.tables_[(round - 1) / 3].Init(
                    Global().PlayerAvatar(0, 30) + HTML_ESCAPE_SPACE + HTML_ESCAPE_SPACE + Global().PlayerName(0),
                    Global().PlayerAvatar(1, 30) + HTML_ESCAPE_SPACE + HTML_ESCAPE_SPACE + Global().PlayerName(1), round);
        }
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        if (GAME_OPTION(固定左拳) && Main().round() > 1) {
            for (PlayerID pid : {0, 1}) {
                Main().tables_[(Main().round() - 1) / 3].SetCard(pid, (Main().round() - 1) % 3, 0,
                        Main().players_[pid].left_->first, true);
            }
            MakeRightChooseStage_(setter);
        } else {
            setter.Emplace<ChooseStage<true>>(Main());
        }
    }

    virtual void NextStageFsm(ChooseStage<true>& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        sub_stage.DefaultAct();
        MakeRightChooseStage_(setter);
    }

    virtual void NextStageFsm(ChooseStage<false>& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        sub_stage.DefaultAct();
        {
            auto sender = Global().Boardcast();
            const auto show_choose = [&](const PlayerID pid)
                {
                    sender << At(pid) << "\n左拳：" << CardName(Main().players_[pid].left_->first)
                                      << "\n右拳：" << CardName(Main().players_[pid].right_->first);
                };
            show_choose(0);
            sender << "\n\n";
            show_choose(1);
        }
        Main().ShowInfo();
        setter.Emplace<AlterStage>(Main());
    }

    virtual void NextStageFsm(AlterStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        sub_stage.DefaultAct();
        return;
    }

  private:
    void MakeRightChooseStage_(SubStageFsmSetter& setter)
    {
        {
            auto sender = Global().Boardcast();
            const auto show_choose = [&](const PlayerID pid)
                {
                    sender << At(pid) << "\n左拳：" << CardName(Main().players_[pid].left_->first)
                                      << "\n右拳：";
                };
            show_choose(0);
            sender << "\n\n";
            show_choose(1);
        }
        Main().ShowInfo();
        setter.Emplace<ChooseStage<false>>(Main());
    }

};

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    Global().Boardcast() << Markdown(CardInfoStr_());
    setter.Emplace<RoundStage>(*this, 1);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (reason == CheckoutReason::BY_LEAVE) {
        return;
    }

    // clear choose
    for (PlayerID pid = 0; pid < 2U; ++pid) {
        auto& player = players_[pid];
        assert(player.left_ == player.alter_ || player.right_ == player.alter_);
        assert(player.left_ != player.right_);
        player.left_->second = CardState::UNUSED;
        player.right_->second = CardState::UNUSED;
        player.alter_->second = CardState::USED;
        if (GAME_OPTION(固定左拳)) {
            if (player.right_ != player.alter_) {
                player.left_ = player.right_;
            }
            player.left_->second = CardState::CHOOSED;
        }
    }

    Battle();

    const auto check_win_count = [&](const PlayerID pid)
        {
            if (players_[pid].win_count_ == 3) {
                players_[pid].score_ = 1;
                players_[1 - pid].score_ = 0;
                Global().Boardcast() << At(pid) << "势如破竹，连下三城，中途胜利！";
                return true;
            }
            return false;
        };
    if (check_win_count(0) || check_win_count(1)) {
        ShowInfo();
        return;
    }

    if (++round_ <= 8) {
        if (!GAME_OPTION(固定左拳)) {
            ShowInfo();
        }
        setter.Emplace<RoundStage>(*this, round_);
        return;
    }

    // choose the last card
    for (PlayerID pid = 0; pid < 2U; ++pid) {
        auto& player = players_[pid];
        player.alter_ = player.left_->second != CardState::USED ? player.left_ : player.right_;
        player.left_ = player.alter_;
        player.left_->second = CardState::USED;
        tables_[2].SetCard(pid, 2, 0, player.alter_->first, true);
    }
    Global().Boardcast() << "8回合全部结束，自动结算最后一张手牌";
    Battle();

    check_win_count(0) || check_win_count(1); // the last card also need check
    ShowInfo();

    if (players_[0].stars_ != players_[1].stars_) {
        players_[0].score_ = players_[0].stars_;
        players_[1].score_ = players_[1].stars_;
    }
    return;
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

