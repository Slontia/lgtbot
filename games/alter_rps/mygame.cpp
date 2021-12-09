#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"
#include "utility/html.h"

const std::string k_game_name = "二择猜拳";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

std::string GameOption::StatusInfo() const
{
    return "";
}

bool GameOption::IsValid(MsgSenderBase& reply) const
{
    if (PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== GAME STAGES ==========

enum class Type : int { ROCK = 0, PAPER = 1, SCISSOR = 2 };

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

struct Card
{
    Type type_;
    int point_;
};

inline int Compare(const Card& _1, const Card& _2)
{
    if (_1.type_ != _2.type_) {
        return Compare(_1.type_, _2.type_);
    } else {
        return _1.point_ - _2.point_;
    }
}

enum class CardState { UNUSED, USED, CHOOSED };

using CardMap = std::map<std::string_view, std::pair<Card, CardState>>;

struct Player
{
    Player()
        : cards_{
            { "石头1" , { Card{Type::ROCK,    1}, CardState::UNUSED} },
            { "石头2" , { Card{Type::ROCK,    2}, CardState::UNUSED} },
            { "石头4" , { Card{Type::ROCK,    4}, CardState::UNUSED} },
            { "布1" ,   { Card{Type::PAPER,   1}, CardState::UNUSED} },
            { "布2" ,   { Card{Type::PAPER,   2}, CardState::UNUSED} },
            { "布4" ,   { Card{Type::PAPER,   4}, CardState::UNUSED} },
            { "剪刀1" , { Card{Type::SCISSOR, 1}, CardState::UNUSED} },
            { "剪刀2" , { Card{Type::SCISSOR, 2}, CardState::UNUSED} },
            { "剪刀4" , { Card{Type::SCISSOR, 4}, CardState::UNUSED} },
          }
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
std::string AvailableCards(const std::map<std::string_view, std::pair<Card, CardState>>& cards)
{
    std::string str;
    for (const auto& [name, card_info] : cards) {
        if (card_info.second == CardState::UNUSED || (INCLUDE_CHOOSED && card_info.second == CardState::CHOOSED)) {
            if (!str.empty()) {
                str += "、";
            }
            str += name;
        }
    }
    return str;
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
        table_.Get(0, 0).SetColor("#7092BE");
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
            table_.Get(4, i * 2).SetColor("#7092BE");
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
        table_.Get(8, 0).SetColor("#7092BE");
    }

    void SetCard(const PlayerID pid, const uint32_t round_idx, const bool choose_idx, const Card& card,
            const bool with_color)
    {
        const uint32_t row = 1 + pid * 5;
        const uint32_t col = round_idx * 2 + choose_idx;
        table_.Get(row, col).SetContent(Image_(type2char[static_cast<int>(card.type_)] + std::to_string(with_color)));
#define SET_POINT_COLOR(text_color, bg_color) \
do { \
    table_.Get(row + 1, col) \
          .SetContent(HTML_COLOR_FONT_HEADER(text_color) " **+ " + std::to_string(card.point_) + "** " HTML_FONT_TAIL); \
    table_.Get(row + 1, col).SetColor(#bg_color); \
} while (0)
        if (!with_color) {
            SET_POINT_COLOR(#7f7f7f, #c3c3c3);
        } else if (card.type_ == Type::PAPER) {
            SET_POINT_COLOR(#22b14c, #b5e61d);
        } else if (card.type_ == Type::SCISSOR) {
            SET_POINT_COLOR(#00a2e8, #caedf5);
        } else if (card.type_ == Type::ROCK) {
            SET_POINT_COLOR(#ed1c24, #ffd6e5);
        }
#undef SET_POINT_COLOR
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
    std::string Image_(std::string name) const
    {
        return std::string("![](file://") + image_path_ + "/" + std::move(name) + ".png)";
    }

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
        , round_(1)
        , tables_{ThreeRoundTable(option.ResourceDir()),
                  ThreeRoundTable(option.ResourceDir()),
                  ThreeRoundTable(option.ResourceDir())}
    {}

    virtual VariantSubStage OnStageBegin() override;

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    int64_t PlayerScore(const PlayerID pid) const
    {
        return players_[pid].score_;
    }

    void Battle()
    {
        const int ret = Compare(players_[0].alter_->second.first,
                                players_[1].alter_->second.first);
        {
            auto sender = Boardcast();
            sender << "最终选择：\n"
                << At(PlayerID(0)) << "：" << players_[0].alter_->first << "\n"
                << At(PlayerID(1)) << "：" << players_[1].alter_->first << "\n\n";
            if (ret == 0) {
                sender << "平局！";
                players_[0].win_count_ = 0;
                players_[1].win_count_ = 0;
                tables_[(round_ - 1) / 3].SetScore(0, (round_ - 1) % 3, players_[0].score_);
                tables_[(round_ - 1) / 3].SetScore(1, (round_ - 1) % 3, players_[1].score_);
            } else {
                const PlayerID winner = ret > 0 ? 0 : 1;
                const int point = players_[1 - winner].alter_->second.first.point_;
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

    std::array<Player, 2> players_;
    std::array<ThreeRoundTable, 3> tables_;

    void ShowInfo()
    {
        std::string str;
        for (uint32_t i = 0; i < (round_ - 1) / 3 + 1; ++i) {
            str += "\n\n## 第 " + std::to_string(i * 3 + 1) + "~" + std::to_string(i * 3 + 3) + " 回合\n\n" + tables_[i].ToHtml();
        }
        Boardcast() << Markdown(str);
    }

   private:
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

    virtual void OnPlayerLeave(const PlayerID pid) override
    {
        leaver_ = pid;
        players_[pid].score_ = 0;
        players_[1 - pid].score_ = 1;
    }

    uint64_t round_;
    std::optional<PlayerID> leaver_;
};

template <bool IS_LEFT>
class ChooseStage : public SubGameStage<>
{
   public:
    ChooseStage(MainStage& main_stage)
            : GameStage(main_stage, IS_LEFT ? "左拳阶段" : "右拳阶段",
                    MakeStageCommand("出拳", &ChooseStage::Choose_, AnyArg("卡牌", "石头2")))
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "请从手牌中选择一张私信给裁判，作为本次出牌";
        StartTimer(60);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto& player = main_stage().players_[pid];
        std::vector<CardMap::iterator> its;
        for (auto it = player.cards_.begin(); it != player.cards_.end(); ++it) {
            if (it->second.second == CardState::UNUSED) {
                its.emplace_back(it);
            }
        }
        SetCard_(pid, its[rand() % its.size()]);

        return StageErrCode::READY;
    }

   private:
    virtual CheckoutErrCode OnTimeout() override
    {
        for (PlayerID pid = 0; pid < uint64_t(2); ++pid) {
            if (IsReady(pid)) {
                continue;
            }
            auto& player = main_stage().players_[pid];
            for (auto it = player.cards_.begin(); it != player.cards_.end(); ++it) {
                if (it->second.second == CardState::UNUSED) {
                    // set default value: the first unused card
                    SetCard_(pid, it);
                    Tell(pid) << "您选择超时，已自动为您选择：" << it->first;
                    break;
                }
            }
        }
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        // TODO: test repeat choose
        auto& player = main_stage().players_[pid];
        auto& cards = main_stage().players_[pid].cards_;
        if (is_public) {
            reply() << "[错误] 请私信裁判选择";
            return StageErrCode::FAILED;
        }
        const auto it = cards.find(str);
        if (it == cards.end()) {
            reply() << "[错误] 预料外的卡牌名，您目前可用的卡牌：\n" << AvailableCards<false>(cards);
            return StageErrCode::FAILED;
        }
        if (it->second.second != CardState::UNUSED) {
            reply() << "[错误] 该卡牌已被使用，请换一张，您目前可用的卡牌：\n" << AvailableCards<false>(cards);
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            (IS_LEFT ? player.left_ : player.right_)->second.second = CardState::UNUSED;
        }
        SetCard_(pid, it);
        reply() << "选择成功";
        return StageErrCode::READY;
    }

    void SetCard_(const PlayerID pid, const CardMap::iterator it)
    {
        it->second.second = CardState::CHOOSED;

        auto& player = main_stage().players_[pid];
        (IS_LEFT ? player.left_ : player.right_) = it;

        const auto round = main_stage().round();
        main_stage().tables_[(round - 1) / 3].SetCard(pid, (round - 1) % 3, !IS_LEFT, it->second.first, true);
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override { return StageErrCode::CHECKOUT; }
};

class AlterStage : public SubGameStage<>
{
   public:
    AlterStage(MainStage& main_stage)
            : GameStage(main_stage, "二择阶段",
                    MakeStageCommand("选择决胜卡牌", &AlterStage::Alter_, AnyArg("卡牌", "石头2")))
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "请从两张卡牌中选择一张私信给裁判，作为用于本回合决胜的卡牌";
        StartTimer(60);
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

   private:
    virtual CheckoutErrCode OnTimeout() override
    {
        for (PlayerID pid = 0; pid < uint64_t(2); ++pid) {
            if (IsReady(pid)) {
                continue;
            }
            auto& player = main_stage().players_[pid];
            player.alter_ = player.left_;
            Tell(pid) << "您选择超时，已自动为您选择左拳卡牌";
        }
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode Alter_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        // TODO: test repeat choose
        auto& player = main_stage().players_[pid];
        auto& cards = player.cards_;
        if (is_public) {
            reply() << "[错误] 请私信裁判选择";
            return StageErrCode::FAILED;
        } else if (const auto it = cards.find(str); it == cards.end()) {
            reply() << "[错误] 预料外的卡牌名，请在您所选的卡牌间二选一：\n"
                    << player.left_->first << "、" << player.right_->first;
            return StageErrCode::FAILED;
        } else if (it->second.second != CardState::CHOOSED) {
            reply() << "[错误] 您未选择这张卡牌，请在您所选的卡牌间二选一：\n"
                    << player.left_->first << "、" << player.right_->first;
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
                choose_left, choose_left ? player.right_->second.first : player.left_->second.first, false);
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override { return StageErrCode::CHECKOUT; }
};

class RoundStage : public SubGameStage<ChooseStage<true>, ChooseStage<false>, AlterStage>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : GameStage(main_stage, "第" + std::to_string(round) + "回合")
    {
        if ((round - 1) % 3 == 0) {
            main_stage.tables_[(round - 1) / 3].Init(PlayerName(0), PlayerName(1), round);
        }
    }

    virtual VariantSubStage OnStageBegin() override
    {
        return std::make_unique<ChooseStage<true>>(main_stage());
    }

    virtual VariantSubStage NextSubStage(ChooseStage<true>& sub_stage, const CheckoutReason reason) override
    {
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        {
            auto sender = Boardcast();
            const auto show_choose = [&](const PlayerID pid)
                {
                    sender << At(pid) << "\n左拳：" << main_stage().players_[pid].left_->first
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
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        {
            auto sender = Boardcast();
            const auto show_choose = [&](const PlayerID pid)
                {
                    sender << At(pid) << "\n左拳：" << main_stage().players_[pid].left_->first
                                    << "\n右拳：" << main_stage().players_[pid].right_->first;
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
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        main_stage().Battle();
        return {};
    }
};

MainStage::VariantSubStage MainStage::OnStageBegin() { return std::make_unique<RoundStage>(*this, 1); }

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if (reason == CheckoutReason::BY_LEAVE) {
        return {};
    }

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

    // clear choose
    for (PlayerID pid = 0; pid < uint64_t(2); ++pid) {
        players_[pid].left_->second.second = CardState::UNUSED;
        players_[pid].right_->second.second = CardState::UNUSED;
        players_[pid].alter_->second.second = CardState::USED;
    }
    if (++round_ <= 8) {
        return std::make_unique<RoundStage>(*this, round_);
    }

    // choose the last card
    for (PlayerID pid = 0; pid < uint64_t(2); ++pid) {
        auto& player = players_[pid];
        player.alter_ = player.left_->second.second == CardState::UNUSED ? player.left_ : player.right_;
        player.left_ = player.alter_;
        tables_[2].SetCard(pid, 2, 0, player.alter_->second.first, true);
    }
    Boardcast() << "8回合全部结束，自动结算最后一张手牌";
    Battle();

    check_win_count(0) || check_win_count(1); // the last card also need check
    return {};
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match)
{
    if (!options.IsValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}

