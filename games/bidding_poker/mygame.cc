// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "game_framework/game_achievements.h"
#include "utility/msg_checker.h"
#include "utility/html.h"
#include "game_util/bidding.h"
#include "game_util/poker.h"

const std::string k_game_name = "投标波卡";
const uint64_t k_max_player = 0; /* 0 means no max-player limits */
const uint64_t k_multiple = 1;
const std::string k_developer = "森高";
const std::string k_description = "通过投标和拍卖提升波卡牌型的游戏";

std::string GameOption::StatusInfo() const
{
    std::stringstream ss;
    ss << "每名玩家初始金币" << GET_VALUE(初始金币数) << "枚，"
       << "共" << GET_VALUE(回合数) << "回合，"
       << "每回合弃牌时间" << GET_VALUE(弃牌时间) << "秒，"
       << "每件商品投标时间" << GET_VALUE(投标时间) << "秒，"
       << GET_VALUE(投标轮数) << "轮投标后若仍有复数玩家投标额最高，则流标\n";
    if (GET_VALUE(种子).empty()) {
        ss << "未指定种子";
    } else {
        ss << "种子：" << GET_VALUE(种子);
    }
    return ss.str();
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为" << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 10; }

// ========== GAME STAGES ==========

struct Player
{
    Player(const PlayerID pid, const uint32_t coins) : pid_(pid), coins_(coins), bonus_coins_(0) {}
    PlayerID pid_;
    int32_t coins_;
    int32_t bonus_coins_;
    poker::Hand hand_;
};

class MainBidStage;
class RoundStage;
using PokerItems = std::vector<std::pair<std::optional<PlayerID>, std::set<poker::Poker>>>;

class MainStage : public MainGameStage<MainBidStage, RoundStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match,
                MakeStageCommand("通过文字查看各玩家手牌及金币情况", &MainStage::Status_, VoidChecker("赛况"), VoidChecker("文字")))
        , round_(0)
    {
        for (PlayerID pid = 0; pid < option.PlayerNum(); ++pid) {
            players_.emplace_back(pid, GET_OPTION_VALUE(option, 初始金币数));
        }
    }

    virtual VariantSubStage OnStageBegin() override;

    virtual VariantSubStage NextSubStage(MainBidStage& sub_stage, const CheckoutReason reason) override;

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    int64_t PlayerScore(const PlayerID pid) const { return players_[pid].coins_ + players_[pid].bonus_coins_; }

    PokerItems& poker_items() { return poker_items_; }
    const PokerItems& poker_items() const { return poker_items_; }
    std::vector<Player>& players() { return players_; }
    const std::vector<Player>& players() const { return players_; }

    void UpdatePlayerScore()
    {
        using DeckElement = std::pair<PlayerID, poker::Deck>;

        std::vector<DeckElement> best_decks;
        std::vector<PlayerID> no_deck_players;
        for (const auto& player : players_) {
            if (const auto& deck = player.hand_.BestDeck(); deck.has_value()) {
                best_decks.emplace_back(player.pid_, *deck);
            } else {
                no_deck_players.emplace_back(player.pid_);
            }
        }
        for (auto& player : players_) {
            player.bonus_coins_ = 0;
        }
        if (best_decks.empty()) {
            return;
        }

        std::sort(best_decks.begin(), best_decks.end(), [](const DeckElement& _1, const DeckElement& _2) { return _1.second > _2.second; });

        static const auto get_percent = [](const auto value, const uint32_t percent) { return value - value * (100 - percent) / 100; };

        uint32_t bonus_coins = BonusCoins_();

        // decrease coins
        const auto decrese_coins = [&](const PlayerID pid)
        {
            const auto lost_coins = get_percent(players_[pid].coins_, GET_OPTION_VALUE(option(), 惩罚比例));
            bonus_coins += lost_coins;
            players_[pid].bonus_coins_ -= lost_coins;
        };
        if (no_deck_players.empty()) {
            decrese_coins(best_decks.back().first);
        } else {
            for (const auto& loser_pid : no_deck_players) {
                decrese_coins(loser_pid);
            }
        }

	// const double increase_scale = static_cast<double>(GET_OPTION_VALUE(option(), 奖励比例)) / 100;
        // const double logical_sum = (std::pow(increase_scale, best_deck.size()) - 1) / (increase_scale - 1);
	// std::cout << "logical: " << logical_sum << std::endl;
	// auto remains_bonus_coins = bonus_coins;

        // increase coins
        for (uint64_t i = 0; i < best_decks.size(); ++i) {
            // const auto increase_coins = std::min(std::pow(increase_scale, i) * increase;
            const auto add_coins = get_percent(bonus_coins, 50);
            players_[best_decks[i].first].bonus_coins_ += add_coins;
            bonus_coins -= add_coins;
        }
        players_[best_decks.back().first].bonus_coins_ += bonus_coins;
    }

    std::string NonPlayerItemInfoHtml() const
    {
        bool empty = true;
        std::string s;
        s += "### 未中标的系统商品\n";
        for (const auto& [discarder_id, pokers] : poker_items_) {
            if (!pokers.empty() && !discarder_id.has_value()) {
                empty = false;
                s += "\n- ";
                for (const auto& poker : pokers) {
                    s += poker.ToHtml() + HTML_ESCAPE_SPACE;
                }
            }
        }
        if (empty) {
            s += "\n（无）";
        }
        return s;
    }

    static std::string PokersHtml(const std::set<poker::Poker>& pokers)
    {
        std::string s;
        for (const auto& poker : pokers) {
            s += poker.ToHtml() + HTML_ESCAPE_SPACE;
        }
        return s;
    }

    std::string ItemInfoHtml(uint64_t index) const
    {
        std::string s;
        s += "### 当前商品\n\n";
        {
            const auto& [discarder_id, pokers] = poker_items_[index];
            s += "<center><font size=\"4\"> " + std::to_string(index + 1) + " 号商品：" + PokersHtml(pokers) + " </font></center>";
            s += "<center>" HTML_COLOR_FONT_HEADER(blue) "\n\n**拍卖人：" +
                (discarder_id.has_value() ? (PlayerAvatar(*discarder_id, 30) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE + PlayerName(*discarder_id)) : "（无）") + "**\n\n" HTML_FONT_TAIL "</center>";
        }
        s += "\n\n### 剩余商品\n";
        if (++index == poker_items_.size()) {
            s += "\n（无）";
        } else {
            for (; index < poker_items_.size(); ++index) {
                const auto& [discarder_id, pokers] = poker_items_[index];
                s += "\n- " + std::to_string(index + 1) + " 号商品：" + PokersHtml(pokers);
                s += "（拍卖人：";
                if (discarder_id.has_value()) {
                    s += PlayerAvatar(*discarder_id, 30) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE + PlayerName(*discarder_id);
                } else {
                    s += "无";
                }
                s += "）";
            }
        }
        return s;
    }

    std::string PlayerInfoHtml() const
    {
        std::string s;
        auto players = players_;
        std::sort(players.begin(), players.end(), [](const auto& _1, const auto& _2)
                {
                    return _1.coins_ + _1.bonus_coins_ > _2.coins_ + _2.bonus_coins_;
                });
        uint32_t rank = 0;
        for (const Player& player : players) {
            s += "### " + PlayerAvatar(player.pid_, 40) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE + std::to_string(++rank) + " 位：" +
                PlayerName(player.pid_) + "（预计得分：**" + std::to_string(player.coins_ + player.bonus_coins_) + "**）\n\n";
            s += "<center>\n\n**可用金币：" HTML_COLOR_FONT_HEADER(blue) + std::to_string(player.coins_) +
                 HTML_FONT_TAIL HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE "终局奖惩：";
            if (player.bonus_coins_ > 0) {
                s += HTML_COLOR_FONT_HEADER(green) "+" + std::to_string(player.bonus_coins_) + HTML_FONT_TAIL;
            } else if (player.bonus_coins_ < 0) {
                s += HTML_COLOR_FONT_HEADER(red) + std::to_string(player.bonus_coins_) + HTML_FONT_TAIL;
            } else {
                s += "0";
            }
            s += "**\n\n</center>\n<center><font size=\"4\">\n\n";
            if (const auto& deck = player.hand_.BestDeck(); !deck.has_value()) {
                s += HTML_COLOR_FONT_HEADER(red) " **未组成牌型** " HTML_FONT_TAIL;
            } else {
                s += "**";
                for (const auto& poker : deck->pokers_) {
                    s += poker.ToHtml() + HTML_ESCAPE_SPACE;
                }
                s += HTML_COLOR_FONT_HEADER(blue) "（";
                s += deck->TypeName();
                s += "）" HTML_FONT_TAIL "**";
            }
            s += "\n\n</font></center>\n\n<center><font size=\"2\"> ";
            if (player.hand_.Empty()) {
                s += "（无手牌）";
            } else {
                s += player.hand_.ToHtml();
            }
            s +=" </font></center>\n\n";
        }
        return s;
    }

    std::string TitleHtml() const
    {
        const std::string round_info =
            round_ == 0                         ? "开局" :
            round_ > GET_OPTION_VALUE(option(), 回合数) ? "终局" : "第 " + std::to_string(round_) + " 回合";
        return "<center>\n\n## " + round_info +
            "\n\n</center>\n\n<center>\n\n**奖池金币数：" + std::to_string(BonusCoins_()) + "**</center>\n\n";
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        auto sender = reply();
        sender << "奖池金币数：" << BonusCoins_() << "枚\n";
        for (const Player& player : players_) {
            sender << "\n" << Name(player.pid_) << "：" << player.coins_ << "枚金币";
            sender << "\n手牌：" << player.hand_;
        }
        return StageErrCode::OK;
    }

    uint32_t BonusCoins_() const
    {
        return std::accumulate(players_.begin(), players_.end(), GET_OPTION_VALUE(option(), 初始金币数) * option().PlayerNum(),
                [](const uint32_t _1, const Player& _2) { return _1 - _2.coins_; });
    }

    PokerItems poker_items_;
    std::vector<Player> players_;
    uint32_t round_;
};

class BidStage : public SubGameStage<>
{
  public:
    BidStage(MainStage& main_stage, std::string&& name, const std::optional<PlayerID>& discarder, std::set<poker::Poker>& pokers)
        : GameStage(main_stage, std::move(name),
                MakeStageCommand("投标", &BidStage::Bid_, ArithChecker<uint32_t>(0, main_stage.players().size() * GET_OPTION_VALUE(main_stage.option(), 初始金币数), "金币数")),
                MakeStageCommand("跳过本轮投标", &BidStage::Cancel_, VoidChecker("pass"))),
        discarder_(discarder), pokers_(pokers), bidding_manager_(main_stage.players().size()), bid_count_(0)
    {}

    void OnStageBegin()
    {
        auto sender = Boardcast();
        sender << name_ << "：";
        for (const auto& poker : pokers_) {
            sender << poker << " ";
        }
        sender << "\n拍卖人：";
        if (discarder_.has_value()) {
            sender << At(*discarder_);
            SetReady(*discarder_); // discarder need not bid
        } else {
            sender << "（无）";
        }
        for (const auto& player : main_stage().players()) {
            if (player.coins_ == 0) {
                SetReady(player.pid_); // players have no coins need not bid
            }
        }
        sender << "\n投标开始，请私信裁判进行投标";
        StartTimer(GET_OPTION_VALUE(option(), 投标时间));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        const auto max_bid_coins = main_stage().players()[pid].coins_ / 4;
        if (max_bid_coins > 0) {
            Bid_(pid, false, reply, std::rand() % max_bid_coins + 1);
        }
        return StageErrCode::READY;
    }

    // TODO: Hook unready players
    CheckoutErrCode OnTimeout() { return CheckoutErrCode::Condition(BidOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE); }
    void OnAllPlayerReady() { BidOver_(); }

  private:
    bool BidOver_()
    {
        const auto return_item = [&]()
            {
                if (discarder_.has_value()) {
                    for (const auto& poker : pokers_) {
                        main_stage().players()[*discarder_].hand_.Add(poker);
                    }
                    pokers_.clear();
                }
                main_stage().UpdatePlayerScore();
            };
        const auto ret = bidding_manager_.BidOver(BoardcastMsgSender());
        if (ret.second.size() == 0) {
            return_item();
            return true;
        } else if (ret.second.size() == 1) {
            assert(ret.first.has_value());
            const auto& max_chip = *ret.first;
            auto& winner = main_stage().players()[ret.second[0]];
            winner.coins_ -= max_chip;
            for (const auto& poker : pokers_) {
                winner.hand_.Add(poker);
            }
            if (discarder_.has_value()) {
                main_stage().players()[*discarder_].coins_ += max_chip;
            }
            pokers_.clear();
            Boardcast() << "恭喜" << At(ret.second[0]) << "中标";
            main_stage().UpdatePlayerScore();
            return true;
        } else if ((++bid_count_) == GET_OPTION_VALUE(option(), 投标轮数)) {
            Boardcast() << "投标轮数达到最大值，本商品流标";
            return_item();
            return true;
        } else {
            auto sender = Boardcast();
            sender << "最大金额投标者有多名玩家，分别是：";
            for (const auto& winner : ret.second) {
                sender << "\n" << At(winner);
                ClearReady(winner);
            }
            sender << "\n开始新的一轮投标，这些玩家可在此轮中重新投标（投标额不得少于上一轮）";
            if (bid_count_ + 1 == GET_OPTION_VALUE(option(), 投标轮数)) {
                sender << "\n注意：若本轮仍未能决出中标者，则商品流标";
            }
            StartTimer(GET_OPTION_VALUE(option(), 投标时间));
            return false;
        }
    }

    AtomReqErrCode Bid_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t chip)
    {
        if (discarder_.has_value() && pid == *discarder_) {
            reply() << "投标失败：您是该商品的拍卖者，不可以参与投标";
            return StageErrCode::FAILED;
        }
        if (is_public) {
            reply() << "投标失败：请私信裁判进行投标";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "投标失败：您已经决定跳过，或进行过投标";
            return StageErrCode::FAILED;
        }
        if (chip == 0) {
            reply() << "投标失败：投标金币数必须大于0";
            return StageErrCode::FAILED;
        }
        if (chip > main_stage().players()[pid].coins_) {
            reply() << "投标失败：投标金币数超过了您所持有的金币数，当前持有" << main_stage().players()[pid].coins_ << "枚";
            return StageErrCode::FAILED;
        }
        return AtomReqErrCode::Condition(bidding_manager_.Bid(pid, chip, reply()), StageErrCode::READY, StageErrCode::FAILED);
    }

    AtomReqErrCode Cancel_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (discarder_.has_value() && pid == *discarder_) {
            reply() << "跳过失败：您是该商品的拍卖者，本来就无法参与投标，不需要跳过";
            return StageErrCode::FAILED;
        }
        if (is_public) {
            reply() << "跳过失败：请私信裁判跳过";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "跳过失败：您已经决定跳过，或进行过投标";
            return StageErrCode::FAILED;
        }
        return AtomReqErrCode::Condition(bidding_manager_.Bid(pid, std::nullopt, reply()), StageErrCode::READY, StageErrCode::FAILED);
    }

    const std::optional<PlayerID>& discarder_;
    std::set<poker::Poker>& pokers_;
    BiddingManager<uint32_t> bidding_manager_;
    uint32_t bid_count_;
};

class MainBidStage : public SubGameStage<BidStage>
{
  public:
    MainBidStage(MainStage& main_stage)
        : GameStage(main_stage, "投标阶段",
                MakeStageCommand("通过图片查看各玩家手牌及金币情况", &MainBidStage::Status_, VoidChecker("赛况")))
        , index_(0)
    {}

    virtual VariantSubStage OnStageBegin() override
    {
        static const auto cmp = [](const PokerItems::value_type& _1, const PokerItems::value_type& _2) {
            return _1.second.size() < _2.second.size() ||
                   (_1.second.size() == _2.second.size() && *_1.second.begin() < *_2.second.begin());
        };
        assert(!main_stage().poker_items().empty());
        std::sort(main_stage().poker_items().begin(), main_stage().poker_items().end(), cmp);
        {
            auto sender = Boardcast();
            sender << name_ << "开始，请私信裁判进行投标，商品列表：";
            for (uint32_t i = 0; i < main_stage().poker_items().size(); ++i) {
                auto& poker_item = main_stage().poker_items()[i];
                sender << "\n" << (i + 1) << "号商品：";
                for (const auto& poker : poker_item.second) {
                    sender << poker << " ";
                }
            }
        }
        return CreateBidStage();
    }

    virtual VariantSubStage NextSubStage(BidStage& sub_stage, const CheckoutReason reason) override
    {
        if ((++index_) == main_stage().poker_items().size()) {
            return {};
        }
        return CreateBidStage();
    }

  private:
    std::string InfoHtml_() const
    {
        return main_stage().TitleHtml() + "\n\n" + main_stage().ItemInfoHtml(index_) + "\n\n" + main_stage().PlayerInfoHtml();
    }

    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Boardcast() << Markdown(InfoHtml_());
        return StageErrCode::OK;
    }

    VariantSubStage CreateBidStage()
    {
        Boardcast() << Markdown(InfoHtml_());
        return std::make_unique<BidStage>(main_stage(), std::to_string(index_ + 1) + "号商品", main_stage().poker_items()[index_].first,
                main_stage().poker_items()[index_].second);
    }

    uint32_t index_;
};

class DiscardStage : public SubGameStage<>
{
  public:
    DiscardStage(MainStage& main_stage)
        : GameStage(main_stage, "弃牌阶段",
                MakeStageCommand("查看各玩家手牌及金币情况", &DiscardStage::Status_, VoidChecker("赛况")),
                MakeStageCommand("本回合不进行弃牌", &DiscardStage::Cancel_, VoidChecker("pass")),
                MakeStageCommand("决定本回合**所有的**弃牌", &DiscardStage::Discard_, RepeatableChecker<AnyArg>("扑克", "红3")))
    {}

    void OnStageBegin()
    {
        Boardcast() << Markdown(InfoHtml_());
        Boardcast() << "弃牌阶段开始，请一次性私信裁判**所有的**弃牌，当到达时间限制，或所有玩家皆选择完毕后，回合结束。"
                       "\n回合结束前您可以随意更改您的选择。";
        StartTimer(GET_OPTION_VALUE(option(), 弃牌时间));
        for (const auto& player : main_stage().players()) {
            if (player.hand_.Empty()) {
                SetReady(player.pid_); // need not discard
            }
        }
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        if (std::rand() % 2) {
            return StageErrCode::READY;
        }
        const auto best_deck = main_stage().players()[pid].hand_.BestDeck();
        std::vector<std::string> discard_poker_strs;
        std::vector<poker::Poker> shuffled_pokers = poker::ShuffledPokers();
        for (const auto& poker : shuffled_pokers) {
            // should not break the best deck
            if (main_stage().players()[pid].hand_.Has(poker) &&
                (!best_deck.has_value() || std::find(best_deck->pokers_.begin(), best_deck->pokers_.end(), poker) == best_deck->pokers_.end())) {
                discard_poker_strs.emplace_back(poker.ToString());
                if (discard_poker_strs.size() == 3) {
                    break;
                }
            }
        }
        Discard_(pid, false, reply, discard_poker_strs);
        return StageErrCode::READY;
    }

  private:
    std::string InfoHtml_() const
    {
        return main_stage().TitleHtml() + "\n\n" + main_stage().NonPlayerItemInfoHtml() + "\n\n" + main_stage().PlayerInfoHtml();
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Boardcast() << Markdown(InfoHtml_());
        return StageErrCode::OK;
    }

    AtomReqErrCode Discard_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::vector<std::string>& poker_strs)
    {
        if (is_public) {
            reply() << "弃牌失败：请私信裁判进行弃牌";
            return StageErrCode::FAILED;
        }
        if (poker_strs.empty()) {
            reply() << "弃牌失败：弃牌为空，若您不考虑弃牌，请使用「不弃牌」指令";
            return StageErrCode::FAILED;
        }
        std::set<poker::Poker> pokers_to_discard;
        for (const auto& poker_str : poker_strs) {
            std::stringstream ss;
            if (const auto poker = poker::Parse(poker_str, ss); !poker.has_value()) {
                reply() << "弃牌失败：非法的扑克名“" << poker_str << "”，" << ss.str();
                return StageErrCode::FAILED;
            } else if (!main_stage().players()[pid].hand_.Has(*poker)) {
                reply() << "弃牌失败：您未持有「" << poker_str << "」这张扑克牌";
                return StageErrCode::FAILED;
            } else {
                pokers_to_discard.emplace(*poker);
            }
        }
        auto sender = reply();
        sender << "弃牌成功，您当前的弃牌为：";
        for (const auto& poker : pokers_to_discard) {
            sender << " " << poker;
        }
        for (auto& [discarder, pokers] : main_stage().poker_items()) {
            if (discarder.has_value() && *discarder == pid) {
                pokers = std::move(pokers_to_discard);
                return StageErrCode::READY;
            }
        }
        main_stage().poker_items().emplace_back(pid, std::move(pokers_to_discard));
        return StageErrCode::READY;
    }

    AtomReqErrCode Cancel_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "失败：请私信裁判取消弃牌";
            return StageErrCode::FAILED;
        }
        for (auto& [discarder, pokers] : main_stage().poker_items()) {
            if (discarder.has_value() && *discarder == pid) {
                pokers.clear();
            }
        }
        reply() << "您当前选择不弃牌";
        return StageErrCode::READY;
    }

    CheckoutErrCode OnTimeout()
    {
        return StageErrCode::CHECKOUT;
    }
};

class RoundStage : public SubGameStage<DiscardStage, MainBidStage>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : GameStage(main_stage, "第" + std::to_string(round) + "回合")
    {}

    virtual VariantSubStage OnStageBegin() override
    {
        Boardcast() << name_ << "开始！";
        return std::make_unique<DiscardStage>(main_stage());
    }

    virtual VariantSubStage NextSubStage(DiscardStage& sub_stage, const CheckoutReason reason) override
    {
        std::erase_if(main_stage().poker_items(), [](const PokerItems::value_type& item) { return item.second.empty(); });
        if (main_stage().poker_items().empty()) {
            Boardcast() << "无商品，跳过投标阶段";
            return {};
        }
        for (const auto& [discarder_pid, pokers] : main_stage().poker_items()) {
            if (discarder_pid.has_value()) {
                auto& discarder = main_stage().players()[*discarder_pid];
                for (const auto& poker : pokers) {
                    discarder.hand_.Remove(poker);
                }
            }
        }
        main_stage().UpdatePlayerScore();
        return std::make_unique<MainBidStage>(main_stage());
    }

    virtual VariantSubStage NextSubStage(MainBidStage& sub_stage, const CheckoutReason reason) override
    {
        return {};
    }
};

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    const auto shuffled_pokers = poker::ShuffledPokers(GET_OPTION_VALUE(option(), 种子));
    assert(shuffled_pokers.size() == 40);
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 0, shuffled_pokers.begin() + 5));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 5, shuffled_pokers.begin() + 10));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 10, shuffled_pokers.begin() + 15));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 15, shuffled_pokers.begin() + 20));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 20, shuffled_pokers.begin() + 25));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 25, shuffled_pokers.begin() + 30));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 30, shuffled_pokers.begin() + 35));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 35, shuffled_pokers.begin() + 40));
    return std::make_unique<MainBidStage>(*this);
}

MainStage::VariantSubStage MainStage::NextSubStage(MainBidStage& sub_stage, const CheckoutReason reason)
{
    return std::make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if ((++round_) <= GET_OPTION_VALUE(option(), 回合数)) {
        return std::make_unique<RoundStage>(*this, round_);
    }
    Boardcast() << Markdown(TitleHtml() + "\n\n" + PlayerInfoHtml());
    return {};
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match)
{
    if (!options.ToValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}
