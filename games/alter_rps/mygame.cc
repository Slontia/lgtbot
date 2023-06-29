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

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "二择猜拳";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const uint64_t k_multiple = 1;
const std::string k_developer = "森高";
const std::string k_description = "伸出两拳，收回一拳的猜拳游戏";

std::string GameOption::StatusInfo() const
{
    return "";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== GAME STAGES ==========

char type2char[] = "rps";

inline int Compare(const Type _1, const Type _2)
{
    if (_1 == _2) {
        return 0;
    } else if ((static_cast<int>(_1) + 1) % 3 == static_cast<int>(_2)) {
        return -1;
    } else {
        return 1;
    }
}

inline int Compare(const Card& _1, const Card& _2)
{
    if (_1.type_ != _2.type_) {
        return Compare(_1.type_, _2.type_);
    } else {
        return _1.point_ - _2.point_;
    }
}

enum class CardState { UNUSED, USED, CHOOSED };

std::string CardName(const Card& card)
{
    static CardChecker checker;
    return checker.ArgString(card);

}

using CardMap = std::map<Card, CardState>;

static CardMap GetCardMap(const GameOption& option)
{
    CardMap cards;
    if (GET_OPTION_VALUE(option, 手牌).has_value()) {
        for (const auto& card : *GET_OPTION_VALUE(option, 手牌)) {
            cards.emplace(card, CardState::UNUSED);
        }
        return cards;
    }
    std::random_device rd;
    std::mt19937 g(rd());
    static const std::array<int, 10> k_points = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const auto fill_cards_one_type = [&cards, &g](const Type type, const uint32_t num)
        {
            auto shuffled_points = k_points;
            std::ranges::shuffle(shuffled_points, g);
            for (uint32_t i = 0; i < num; ++i) {
                cards.emplace(Card{.type_ = type, .point_ = shuffled_points[i]}, CardState::UNUSED);
            }
        };
    const auto fill_cards = [&fill_cards_one_type](const uint32_t rock_num, const uint32_t paper_num, const uint32_t scissor_num)
        {
            assert(rock_num + paper_num + scissor_num == 9);
            fill_cards_one_type(Type::ROCK, rock_num);
            fill_cards_one_type(Type::PAPER, paper_num);
            fill_cards_one_type(Type::SCISSOR, scissor_num);
        };
    if (const auto rand_value = rand() % 4; rand_value == 0) {
        fill_cards(2, 3, 4);
    } else if (rand_value == 1) {
        fill_cards(4, 3, 2);
    } else {
        fill_cards(3, 3, 3);
    }
    return cards;
}

struct Player
{
    Player(CardMap cards)
        : cards_(std::move(cards))
        , win_count_(0)
        , score_(0)
    {}

    CardMap cards_;
    CardMap::iterator left_;
    CardMap::iterator right_;
    CardMap::iterator alter_;
    int win_count_;
    int score_;
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

static const char* const k_non_color_font_header = HTML_COLOR_FONT_HEADER(#7f7f7f);
static const char* const k_non_color_bg = "#c3c3c3";

static const char* const k_color_font_headers[3] = {
    [static_cast<int>(Type::ROCK)] = HTML_COLOR_FONT_HEADER(#ed1c24),
    [static_cast<int>(Type::PAPER)] = HTML_COLOR_FONT_HEADER(#22b14c),
    [static_cast<int>(Type::SCISSOR)] = HTML_COLOR_FONT_HEADER(#00a2e8),
};

static const char* const k_color_bgs[3] = {
    [static_cast<int>(Type::ROCK)] = "#ffd6e5",
    //[static_cast<int>(Type::PAPER)] = "#b5e61d",
    [static_cast<int>(Type::PAPER)] = "#dff276",
    [static_cast<int>(Type::SCISSOR)] = "#caedf5",
};

static void SetPointColor(html::Box& box, const int point, const char* const font_color_header, const char* const bg_color)
{
    box.SetContent(std::string(font_color_header) + " **+ " + std::to_string(point) + "** " HTML_FONT_TAIL);
    box.SetColor(bg_color);
}

static std::string ImageStr(const std::string& image_path, const std::string& name)
{
    return std::string("![](file:///") + image_path + "/" + name + ".png)";
}

class ThreeRoundTable
{
  public:
    ThreeRoundTable(std::string image_path) : image_path_(std::move(image_path)), table_(9, 6) {}

    void Init(std::string p1_name, std::string p2_name, const uint32_t base_round)
    {
        table_.SetTableStyle(" align=\"center\" cellspacing=\"3\" cellpadding=\"0\" ");
        // 0: p1 name
        table_.MergeRight(0, 0, 6);
        table_.Get(0, 0).SetContent(" **" + p1_name + "** ");
        table_.Get(0, 0).SetColor(k_player_name_color);
        for (uint32_t i = 0; i < 3; ++i) {
            // 1: p1 choose card
            table_.Get(1, i * 2).SetContent(Image_("empty"));
            table_.Get(1, i * 2 + 1).SetContent(Image_("empty"));
            // 2: p1 choose point
            table_.Get(2, i * 2).SetColor("#C3C3C3");
            table_.Get(2, i * 2 + 1).SetColor("#C3C3C3");
            // 3: p1 score
            table_.MergeRight(3, i * 2, 2);
            table_.Get(3, i * 2).SetContent("?");
            // 4: round
            table_.MergeRight(4, i * 2, 2);
            table_.Get(4, i * 2).SetContent(" **第 " + std::to_string(base_round + i) + " 回合** ");
            table_.Get(4, i * 2).SetColor(k_player_name_color);
            // 5: p2 score
            table_.MergeRight(5, i * 2, 2);
            table_.Get(5, i * 2).SetContent("?");
            // 6: p2 choose card
            table_.Get(6, i * 2).SetContent(Image_("empty"));
            table_.Get(6, i * 2 + 1).SetContent(Image_("empty"));
            // 7: p2 choose point
            table_.Get(7, i * 2).SetColor("#C3C3C3");
            table_.Get(7, i * 2 + 1).SetColor("#C3C3C3");
        }
        // 8: p2 name
        table_.MergeRight(8, 0, 6);
        table_.Get(8, 0).SetContent(" **" + p2_name + "** ");
        table_.Get(8, 0).SetColor(k_player_name_color);
    }

    void SetCard(const PlayerID pid, const uint32_t round_idx, const bool choose_idx, const Card& card,
            const bool with_color)
    {
        const uint32_t row = 1 + pid * 5;
        const uint32_t col = round_idx * 2 + choose_idx;
        table_.Get(row, col).SetContent(Image_(type2char[static_cast<int>(card.type_)] + std::to_string(with_color)));

#define ELSE_IF_SET_COLOR(type) \
        else if (card.type_ == Type::type) { \
            SetPointColor(table_.Get(row + 1, col), card.point_, k_color_font_headers[static_cast<int>(Type::type)], \
                    k_color_bgs[static_cast<int>(Type::type)]); \
        }

        if (!with_color) {
            SetPointColor(table_.Get(row + 1, col), card.point_, k_non_color_font_header, k_non_color_bg);
        } ELSE_IF_SET_COLOR(ROCK) ELSE_IF_SET_COLOR(SCISSOR) ELSE_IF_SET_COLOR(PAPER)
#undef ELSE_IF_SET_COLOR
    }

    void SetScore(const PlayerID pid, const uint32_t round_idx, const uint32_t old_score, const uint32_t point)
    {
        const uint32_t row = 3 + pid * 2;
        const uint32_t col = round_idx * 2;
        table_.Get(row, col)
              .SetContent(std::to_string(old_score) + " →" HTML_COLOR_FONT_HEADER(green) " **" +
                          std::to_string(old_score + point) + "** " + HTML_FONT_TAIL);
    }

    void SetScore(const PlayerID pid, const uint32_t round_idx, const uint32_t score)
    {
        const uint32_t row = 3 + pid * 2;
        const uint32_t col = round_idx * 2;
        table_.Get(row, col).SetContent(" **" + std::to_string(score) + "** ");
    }

    std::string ToHtml() const { return table_.ToString(); }

  private:
    std::string Image_(const std::string& name) const { return ImageStr(image_path_, name); }

    const std::string image_path_;
    html::Table table_;
};

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
   public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("查看比赛情况", &MainStage::Info_, VoidChecker("赛况")))
        , k_origin_card_map_(GetCardMap(option))
        , players_{k_origin_card_map_, k_origin_card_map_}
        , round_(1)
        , tables_{ThreeRoundTable(option.ResourceDir()),
                  ThreeRoundTable(option.ResourceDir()),
                  ThreeRoundTable(option.ResourceDir())}
    {
    }

    virtual VariantSubStage OnStageBegin() override;

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    int64_t PlayerScore(const PlayerID pid) const
    {
        return players_[pid].score_;
    }

    void Battle()
    {
        const int ret = Compare(players_[0].alter_->first,
                                players_[1].alter_->first);
        {
            auto sender = Boardcast();
            sender << "最终选择：\n"
                << At(PlayerID(0)) << "：" << CardName(players_[0].alter_->first) << "\n"
                << At(PlayerID(1)) << "：" << CardName(players_[1].alter_->first) << "\n\n";
            if (ret == 0) {
                sender << "平局！";
                players_[0].win_count_ = 0;
                players_[1].win_count_ = 0;
                tables_[(round_ - 1) / 3].SetScore(0, (round_ - 1) % 3, players_[0].score_);
                tables_[(round_ - 1) / 3].SetScore(1, (round_ - 1) % 3, players_[1].score_);
            } else {
                const PlayerID winner = ret > 0 ? 0 : 1;
                const int point = players_[1 - winner].alter_->first.point_;
                tables_[(round_ - 1) / 3].SetScore(1 - winner, (round_ - 1) % 3, players_[1 - winner].score_);
                tables_[(round_ - 1) / 3].SetScore(winner, (round_ - 1) % 3, players_[winner].score_, point);
                ++(players_[winner].win_count_);
                players_[1 - winner].win_count_ = 0;
                players_[winner].score_ += point;
                sender << Name(winner) << "胜利，获得" << point << "分";
            }
            sender << "\n\n当前分数：\n"
                << At(PlayerID(0)) << "：" << players_[0].score_ << "\n"
                << At(PlayerID(1)) << "：" << players_[1].score_;
        }
        ShowInfo();
    }

    uint64_t round() const { return round_; }

    const CardMap k_origin_card_map_;
    std::array<Player, 2> players_;
    std::array<ThreeRoundTable, 3> tables_;

    std::string AvailableCardsImage_(const PlayerID pid) const
    {
        constexpr static int k_table_header_rows = 2;
        html::Table table(k_table_header_rows, 3);
        table.SetTableStyle(" align=\"center\" cellspacing=\"3\" cellpadding=\"0\" ");
        table.MergeRight(0, 0, 3);
        table.Get(0, 0).SetContent(PlayerAvatar(pid, 30) + HTML_ESCAPE_SPACE + HTML_ESCAPE_SPACE + PlayerName(pid));
        table.Get(0, 0).SetColor("#519c53");
        bool is_remains[3] = {false, false, false};
        uint32_t counts[3] = {0, 0, 0};
        for (const auto [card, state] : players_[pid].cards_) {
            auto& count = counts[static_cast<int>(card.type_)];
            if (table.Row() == k_table_header_rows + count) {
                table.AppendRow();
            }
            const bool used = (state == CardState::USED);
            const int type_idx = static_cast<int>(card.type_);
            SetPointColor(table.Get(k_table_header_rows + count, type_idx), card.point_,
                    used ? k_non_color_font_header : k_color_font_headers[type_idx],
                    used ? k_non_color_bg : k_color_bgs[type_idx]);
            is_remains[type_idx] |= !used;
            ++count;
        }
        for (const auto type : {Type::ROCK, Type::SCISSOR, Type::PAPER}) {
            const int type_idx = static_cast<int>(type);
            table.Get(1, type_idx).SetContent(Image_(type2char[type_idx] + std::to_string(is_remains[type_idx])));
        }
        return table.ToString();
    }

    std::string Image_(const std::string& name) const { return ImageStr(option().ResourceDir(), name); }

    void ShowInfo()
    {
        std::string str = CardInfoStr_();
        for (uint32_t i = 0; i < (round_ - 1) / 3 + 1; ++i) {
            str += "\n\n## 第 " + std::to_string(i * 3 + 1) + "~" + std::to_string(i * 3 + 3) + " 回合\n\n" + tables_[i].ToHtml();
        }
        Boardcast() << Markdown(str);
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
                sender << Name(pid) << "\n积分：" << player.score_
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
            : GameStage(main_stage, IS_LEFT ? "左拳阶段" : "右拳阶段",
                    MakeStageCommand("出拳", &ChooseStage::Choose_, CardChecker()))
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "请从手牌中选择一张私信给裁判，作为本次出牌";
        StartTimer(GET_OPTION_VALUE(option(), 时限));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto& player = main_stage().players_[pid];
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
        for (PlayerID pid = 0; pid < uint64_t(2); ++pid) {
            if (IsReady(pid)) {
                continue;
            }
            auto& player = main_stage().players_[pid];
            for (auto it = player.cards_.begin(); it != player.cards_.end(); ++it) {
                if (it->second == CardState::UNUSED) {
                    // set default value: the first unused card
                    SetCard_(pid, it);
                    Tell(pid) << "已自动为您选择：" << CardName(it->first);
                    break;
                }
            }
        }
    }

  private:
    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Card& card)
    {
        // TODO: test repeat choose
        auto& player = main_stage().players_[pid];
        auto& cards = main_stage().players_[pid].cards_;
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
        if (IsReady(pid)) {
            PlayerCard(player)->second = CardState::UNUSED;
        }
        SetCard_(pid, it);
        reply() << "选择成功";
        return StageErrCode::READY;
    }

    void SetCard_(const PlayerID pid, const CardMap::iterator it)
    {
        it->second = CardState::CHOOSED;

        auto& player = main_stage().players_[pid];
        PlayerCard(player) = it;

        const auto round = main_stage().round();
        main_stage().tables_[(round - 1) / 3].SetCard(pid, (round - 1) % 3, !IS_LEFT, it->first, true);
    }

    CardMap::iterator& PlayerCard(Player& player) { return IS_LEFT ? player.left_ : player.right_; }
};

class AlterStage : public SubGameStage<>
{
   public:
    AlterStage(MainStage& main_stage)
            : GameStage(main_stage, "二择阶段",
                    MakeStageCommand("选择决胜卡牌", &AlterStage::Alter_, CardChecker()))
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "请从两张卡牌中选择一张私信给裁判，作为用于本回合决胜的卡牌";
        StartTimer(GET_OPTION_VALUE(option(), 时限));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto& player = main_stage().players_[pid];
        SetAlter_(pid, rand() % 2 ? player.left_ : player.right_);
        return StageErrCode::READY;
    }

    void DefaultAct()
    {
        for (PlayerID pid = 0; pid < uint64_t(2); ++pid) {
            if (IsReady(pid)) {
                continue;
            }
            auto& player = main_stage().players_[pid];
            player.alter_ = player.left_;
            Tell(pid) << "已自动为您选择左拳卡牌";
        }
    }

  private:
    AtomReqErrCode Alter_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Card& card)
    {
        // TODO: test repeat choose
        auto& player = main_stage().players_[pid];
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
        auto& player = main_stage().players_[pid];
        player.alter_ = it;
        const bool choose_left = player.alter_ == player.left_;
        main_stage().tables_[(main_stage().round() - 1) / 3].SetCard(pid, (main_stage().round() - 1) % 3,
                choose_left, choose_left ? player.right_->first : player.left_->first, false);
    }
};

class RoundStage : public SubGameStage<ChooseStage<true>, ChooseStage<false>, AlterStage>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : GameStage(main_stage, "第" + std::to_string(round) + "回合")
    {
        if ((round - 1) % 3 == 0) {
            main_stage.tables_[(round - 1) / 3].Init(
                    PlayerAvatar(0, 30) + HTML_ESCAPE_SPACE + HTML_ESCAPE_SPACE + PlayerName(0),
                    PlayerAvatar(1, 30) + HTML_ESCAPE_SPACE + HTML_ESCAPE_SPACE + PlayerName(1), round);
        }
    }

    virtual VariantSubStage OnStageBegin() override
    {
        return std::make_unique<ChooseStage<true>>(main_stage());
    }

    virtual VariantSubStage NextSubStage(ChooseStage<true>& sub_stage, const CheckoutReason reason) override
    {
        sub_stage.DefaultAct();
        {
            auto sender = Boardcast();
            const auto show_choose = [&](const PlayerID pid)
                {
                    sender << At(pid) << "\n左拳：" << CardName(main_stage().players_[pid].left_->first)
                                      << "\n右拳：";
                };
            show_choose(0);
            sender << "\n\n";
            show_choose(1);
        }
        main_stage().ShowInfo();
        return std::make_unique<ChooseStage<false>>(main_stage());
    }

    virtual VariantSubStage NextSubStage(ChooseStage<false>& sub_stage, const CheckoutReason reason) override
    {
        sub_stage.DefaultAct();
        {
            auto sender = Boardcast();
            const auto show_choose = [&](const PlayerID pid)
                {
                    sender << At(pid) << "\n左拳：" << CardName(main_stage().players_[pid].left_->first)
                                      << "\n右拳：" << CardName(main_stage().players_[pid].right_->first);
                };
            show_choose(0);
            sender << "\n\n";
            show_choose(1);
        }
        main_stage().ShowInfo();
        return std::make_unique<AlterStage>(main_stage());
    }

    virtual VariantSubStage NextSubStage(AlterStage& sub_stage, const CheckoutReason reason) override
    {
        sub_stage.DefaultAct();
        return {};
    }
};

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    Boardcast() << Markdown(CardInfoStr_());
    return std::make_unique<RoundStage>(*this, 1);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if (reason == CheckoutReason::BY_LEAVE) {
        return {};
    }

    // clear choose
    for (PlayerID pid = 0; pid < uint64_t(2); ++pid) {
        players_[pid].left_->second = CardState::UNUSED;
        players_[pid].right_->second = CardState::UNUSED;
        players_[pid].alter_->second = CardState::USED;
    }

    Battle();

    const auto check_win_count = [&](const PlayerID pid)
        {
            if (players_[pid].win_count_ == 3) {
                players_[pid].score_ = 1;
                players_[1 - pid].score_ = 0;
                Boardcast() << At(pid) << "势如破竹，连下三城，中途胜利！";
                return true;
            }
            return false;
        };
    if (check_win_count(0) || check_win_count(1)) {
        return {};
    }

    if (++round_ <= 8) {
        return std::make_unique<RoundStage>(*this, round_);
    }

    // choose the last card
    for (PlayerID pid = 0; pid < uint64_t(2); ++pid) {
        auto& player = players_[pid];
        player.alter_ = player.left_->second == CardState::UNUSED ? player.left_ : player.right_;
        player.left_ = player.alter_;
        tables_[2].SetCard(pid, 2, 0, player.alter_->first, true);
    }
    Boardcast() << "8回合全部结束，自动结算最后一张手牌";
    Battle();

    check_win_count(0) || check_win_count(1); // the last card also need check
    return {};
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
