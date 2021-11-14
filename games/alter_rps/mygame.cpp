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

namespace rps {

enum class Type { ROCK = 0, PAPER = 1, SCISSOR = 2 };

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

}

struct Card
{
    rps::Type type_;
    int point_;
};

inline int Compare(const Card& _1, const Card& _2)
{
    if (_1.type_ != _2.type_) {
        return rps::Compare(_1.type_, _2.type_);
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
            { "石头1" , { Card{rps::Type::ROCK,    1}, CardState::UNUSED} },
            { "石头2" , { Card{rps::Type::ROCK,    2}, CardState::UNUSED} },
            { "石头4" , { Card{rps::Type::ROCK,    4}, CardState::UNUSED} },
            { "布1" ,   { Card{rps::Type::PAPER,   1}, CardState::UNUSED} },
            { "布2" ,   { Card{rps::Type::PAPER,   2}, CardState::UNUSED} },
            { "布4" ,   { Card{rps::Type::PAPER,   4}, CardState::UNUSED} },
            { "剪刀1" , { Card{rps::Type::SCISSOR, 1}, CardState::UNUSED} },
            { "剪刀2" , { Card{rps::Type::SCISSOR, 2}, CardState::UNUSED} },
            { "剪刀4" , { Card{rps::Type::SCISSOR, 4}, CardState::UNUSED} },
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

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
   public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("查看比赛情况", &MainStage::Info_, VoidChecker("赛况")))
        , round_(1)
        , table_(2, 6)
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
                ColorTable_(0, false);
                ColorTable_(1, false);
            } else {
                const PlayerID winner = ret > 0 ? 0 : 1;
                ++(players_[winner].win_count_);
                players_[1 - winner].win_count_ = 0;
                const int point = players_[1 - winner].alter_->second.first.point_;
                players_[winner].score_ += point;
                sender << Name(winner) << "胜利，获得" << point << "分";
                ColorTable_(winner, true);
                ColorTable_(1 - winner, false);
            }
            sender << "\n\n当前分数：\n"
                << At(PlayerID(0)) << "：" << players_[0].score_ << "\n"
                << At(PlayerID(1)) << "：" << players_[1].score_;
        }
        Boardcast() << Markdown(table_.ToString());
    }

    std::array<Player, 2> players_;
    Table table_;

   private:
    void ColorTable_(const PlayerID pid, const bool is_winner)
    {
        const char* const color_name = is_winner ? "AntiqueWhite" : "LightGrey";
        const auto& player = players_[pid];
        // For last battle, the alter will be set to left. So we cannot compare alter with right
        const int offset = player.alter_ == player.left_ ? 0 : 1;
        table_.GetLastRow(pid * 3 + offset).SetColor(color_name);
        table_.GetLastRow(pid * 3 + 2).SetContent(
            is_winner ? HTML_COLOR_FONT_HEADER(Red) + std::to_string(players_[pid].score_) + HTML_FONT_TAIL
                      : std::to_string(players_[pid].score_));
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

        main_stage().table_.GetLastRow(pid * 3 + (IS_LEFT ? 0 : 1)).SetContent(it->first);
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
        player.alter_ = rand() % 2 ? player.left_ : player.right_;
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
            player.alter_ = it;
            reply() << "选择成功";
            return StageErrCode::READY;
        }
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override { return StageErrCode::CHECKOUT; }
};

class RoundStage : public SubGameStage<ChooseStage<true>, ChooseStage<false>, AlterStage>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : GameStage(main_stage, "第" + std::to_string(round) + "回合")
    {}

    virtual VariantSubStage OnStageBegin() override
    {
        main_stage().table_.AppendRow();
        return std::make_unique<ChooseStage<true>>(main_stage());
    }

    virtual VariantSubStage NextSubStage(ChooseStage<true>& sub_stage, const CheckoutReason reason) override
    {
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        auto sender = Boardcast();
        const auto show_choose = [&](const PlayerID pid)
            {
                sender << At(pid) << "\n左拳：" << main_stage().players_[pid].left_->first
                                  << "\n右拳：";
            };
        show_choose(0);
        sender << "\n\n";
        show_choose(1);
        return std::make_unique<ChooseStage<false>>(main_stage());
    }

    virtual VariantSubStage NextSubStage(ChooseStage<false>& sub_stage, const CheckoutReason reason) override
    {
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        auto sender = Boardcast();
        const auto show_choose = [&](const PlayerID pid)
            {
                sender << At(pid) << "\n左拳：" << main_stage().players_[pid].left_->first
                                  << "\n右拳：" << main_stage().players_[pid].right_->first;
            };
        show_choose(0);
        sender << "\n\n";
        show_choose(1);
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

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    for (int i = 0; i < 6; ++i) {
        table_.Get(1, i).SetColor("Aqua");
    }
    table_.Get(0, 1).SetContent(PlayerName(0));
    table_.Get(0, 4).SetContent(PlayerName(1));
    table_.Get(1, 0).SetContent("左拳");
    table_.Get(1, 3).SetContent("左拳");
    table_.Get(1, 1).SetContent("右拳");
    table_.Get(1, 4).SetContent("右拳");
    table_.Get(1, 2).SetContent("积分");
    table_.Get(1, 5).SetContent("积分");
    return std::make_unique<RoundStage>(*this, 1);
}

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
    if (round_ < 8) {
        return std::make_unique<RoundStage>(*this, ++round_);
    }

    // choose the last card
    table_.AppendRow();
    for (PlayerID pid = 0; pid < uint64_t(2); ++pid) {
        auto& player = players_[pid];
        player.alter_ = player.left_->second.second == CardState::UNUSED ? player.left_ : player.right_;
        player.left_ = player.alter_;
        table_.GetLastRow(pid * 3).SetContent(player.alter_->first);
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

