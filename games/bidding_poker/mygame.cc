// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <functional>
#include <memory>
#include <set>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "game_util/bidding.h"
#include "game_util/poker.h"

namespace poker = lgtbot::game_util::poker;
namespace bidding = lgtbot::game_util::bidding;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {
const GameProperties k_properties { 
    .name_ = "投标波卡",
    .developer_ = "森高",
    .description_ = "通过投标和拍卖提升波卡牌型的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; } /* 0 means no max-player limits */
uint32_t Multiple(const CustomOptions& options)
{
    return GET_OPTION_VALUE(options, 种子).empty() ? GET_OPTION_VALUE(options, 回合数) : 0;
}
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为" << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 10;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

template <poker::CardType k_type>
struct Player
{
    Player(const PlayerID pid, const uint32_t coins) : pid_(pid), coins_(coins), bonus_coins_(0) {}
    PlayerID pid_;
    int32_t coins_;
    int32_t bonus_coins_;
    poker::Hand<k_type> hand_;
};

template <poker::CardType k_type>
class MainStage;

template <poker::CardType k_type>
class MainBidStage;

template <poker::CardType k_type>
class RoundStage;

template <poker::CardType k_type>
using PokerItems = std::vector<std::pair<std::optional<PlayerID>, std::set<poker::Card<k_type>>>>;

template <poker::CardType k_type, typename... SubStages>
using SubGameStage = StageFsm<MainStage<k_type>, SubStages...>;

template <poker::CardType k_type, typename... SubStages>
using MainGameStage = StageFsm<void, SubStages...>;

template <poker::CardType k_type>
class MainStage : public MainGameStage<k_type, MainBidStage<k_type>, RoundStage<k_type>>
{
    using StageFsm = MainGameStage<k_type, MainBidStage<k_type>, RoundStage<k_type>>;

  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "通过文字查看各玩家手牌及金币情况", &MainStage::Status_, VoidChecker("赛况"), VoidChecker("文字")))
        , round_(0)
    {
        for (PlayerID pid = 0; pid < this->Global().PlayerNum(); ++pid) {
            players_.emplace_back(pid, GAME_OPTION(初始金币数));
        }
    }

    virtual void FirstStageFsm(StageFsm::SubStageFsmSetter setter) override;

    virtual void NextStageFsm(MainBidStage<k_type>& sub_stage, const CheckoutReason reason, StageFsm::SubStageFsmSetter setter) override;

    virtual void NextStageFsm(RoundStage<k_type>& sub_stage, const CheckoutReason reason, StageFsm::SubStageFsmSetter setter) override;

    int64_t PlayerScore(const PlayerID pid) const { return players_[pid].coins_ + players_[pid].bonus_coins_; }

    PokerItems<k_type>& poker_items() { return poker_items_; }
    const PokerItems<k_type>& poker_items() const { return poker_items_; }
    std::vector<Player<k_type>>& players() { return players_; }
    const std::vector<Player<k_type>>& players() const { return players_; }

    void UpdatePlayerScore()
    {
        using DeckElement = std::pair<PlayerID, poker::Deck<k_type>>;

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
            const auto lost_coins = get_percent(players_[pid].coins_, GAME_OPTION(惩罚比例));
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

	// const double increase_scale = static_cast<double>(GAME_OPTION(奖励比例)) / 100;
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

    static std::string PokersHtml(const std::set<poker::Card<k_type>>& pokers)
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
                (discarder_id.has_value() ? (this->Global().PlayerAvatar(*discarder_id, 30) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE + this->Global().PlayerName(*discarder_id)) : "（无）") + "**\n\n" HTML_FONT_TAIL "</center>";
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
                    s += this->Global().PlayerAvatar(*discarder_id, 30) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE + this->Global().PlayerName(*discarder_id);
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
        for (const auto& player : players) {
            s += "### " + this->Global().PlayerAvatar(player.pid_, 40) + HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE + std::to_string(++rank) + " 位：" +
                this->Global().PlayerName(player.pid_) + "（预计得分：**" + std::to_string(player.coins_ + player.bonus_coins_) + "**）\n\n";
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
            round_ > GAME_OPTION(回合数) ? "终局" : "第 " + std::to_string(round_) + " 回合";
        return "<center>\n\n## " + round_info +
            "\n\n</center>\n\n<center>\n\n**奖池金币数：" + std::to_string(BonusCoins_()) + "**</center>\n\n";
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        auto sender = reply();
        sender << "奖池金币数：" << BonusCoins_() << "枚\n";
        for (const auto& player : players_) {
            sender << "\n" << Name(player.pid_) << "：" << player.coins_ << "枚金币";
            sender << "\n手牌：" << player.hand_;
        }
        return StageErrCode::OK;
    }

    uint32_t BonusCoins_() const
    {
        return std::accumulate(players_.begin(), players_.end(), GAME_OPTION(初始金币数) * this->Global().PlayerNum(),
                [](const uint32_t _1, const Player<k_type>& _2) { return _1 - _2.coins_; });
    }

    PokerItems<k_type> poker_items_;
    std::vector<Player<k_type>> players_;
    uint32_t round_;
};

template <poker::CardType k_type>
class BidStage : public SubGameStage<k_type>
{
    using StageFsm = SubGameStage<k_type>;

  public:
    BidStage(MainStage<k_type>& main_stage, std::string&& name, const std::optional<PlayerID>& discarder, std::set<poker::Card<k_type>>& pokers)
        : StageFsm(main_stage, std::move(name),
                MakeStageCommand(*this, "投标", &BidStage::Bid_,
                    ArithChecker<uint32_t>(0, main_stage.players().size() * GET_OPTION_VALUE(main_stage.Global().Options(), 初始金币数), "金币数")),
                MakeStageCommand(*this, "跳过本轮投标", &BidStage::Cancel_, VoidChecker("pass"))),
        discarder_(discarder), pokers_(pokers), bidding_manager_(main_stage.players().size()), bid_count_(0)
    {}

    void OnStageBegin()
    {
        auto sender = this->Global().Boardcast();
        sender << this->Name() << "：";
        for (const auto& poker : pokers_) {
            sender << poker << " ";
        }
        sender << "\n拍卖人：";
        if (discarder_.has_value()) {
            sender << At(*discarder_);
            this->Global().SetReady(*discarder_); // discarder need not bid
        } else {
            sender << "（无）";
        }
        for (const auto& player : this->Main().players()) {
            if (player.coins_ == 0) {
                this->Global().SetReady(player.pid_); // players have no coins need not bid
            }
        }
        sender << "\n投标开始，请私信裁判进行投标";
        this->Global().StartTimer(GAME_OPTION(投标时间));
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        const auto max_bid_coins = this->Main().players()[pid].coins_ / 4;
        if (max_bid_coins > 0) {
            Bid_(pid, false, reply, std::rand() % max_bid_coins + 1);
        }
        return StageErrCode::READY;
    }

    // TODO: Hook unready players
    CheckoutErrCode OnStageTimeout() { return CheckoutErrCode::Condition(BidOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE); }
    CheckoutErrCode OnStageOver() { return CheckoutErrCode::Condition(BidOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE); }

  private:
    bool BidOver_()
    {
        const auto return_item = [&]()
            {
                if (discarder_.has_value()) {
                    for (const auto& poker : pokers_) {
                        this->Main().players()[*discarder_].hand_.Add(poker);
                    }
                    pokers_.clear();
                }
                this->Main().UpdatePlayerScore();
            };
        const auto ret = bidding_manager_.BidOver(this->Global().BoardcastMsgSender());
        if (ret.second.size() == 0) {
            return_item();
            return true;
        } else if (ret.second.size() == 1) {
            assert(ret.first.has_value());
            const auto& max_chip = *ret.first;
            auto& winner = this->Main().players()[ret.second[0]];
            winner.coins_ -= max_chip;
            for (const auto& poker : pokers_) {
                winner.hand_.Add(poker);
            }
            if (discarder_.has_value()) {
                this->Main().players()[*discarder_].coins_ += max_chip;
            }
            pokers_.clear();
            this->Global().Boardcast() << "恭喜" << At(ret.second[0]) << "中标";
            this->Main().UpdatePlayerScore();
            return true;
        } else if ((++bid_count_) == GAME_OPTION(投标轮数)) {
            this->Global().Boardcast() << "投标轮数达到最大值，本商品流标";
            return_item();
            return true;
        } else {
            auto sender = this->Global().Boardcast();
            sender << "最大金额投标者有多名玩家，分别是：";
            for (const auto& winner : ret.second) {
                sender << "\n" << At(winner);
                this->Global().ClearReady(winner);
            }
            sender << "\n开始新的一轮投标，这些玩家可在此轮中重新投标（投标额不得少于上一轮）";
            if (bid_count_ + 1 == GAME_OPTION(投标轮数)) {
                sender << "\n注意：若本轮仍未能决出中标者，则商品流标";
            }
            this->Global().StartTimer(GAME_OPTION(投标时间));
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
        if (this->Global().IsReady(pid)) {
            reply() << "投标失败：您已经决定跳过，或进行过投标";
            return StageErrCode::FAILED;
        }
        if (chip == 0) {
            reply() << "投标失败：投标金币数必须大于0";
            return StageErrCode::FAILED;
        }
        if (chip > this->Main().players()[pid].coins_) {
            reply() << "投标失败：投标金币数超过了您所持有的金币数，当前持有" << this->Main().players()[pid].coins_ << "枚";
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
        if (this->Global().IsReady(pid)) {
            reply() << "跳过失败：您已经决定跳过，或进行过投标";
            return StageErrCode::FAILED;
        }
        return AtomReqErrCode::Condition(bidding_manager_.Bid(pid, std::nullopt, reply()), StageErrCode::READY, StageErrCode::FAILED);
    }

    const std::optional<PlayerID>& discarder_;
    std::set<poker::Card<k_type>>& pokers_;
    bidding::BiddingManager<uint32_t> bidding_manager_;
    uint32_t bid_count_;
};

template <poker::CardType k_type>
class MainBidStage : public SubGameStage<k_type, BidStage<k_type>>
{
    using StageFsm = SubGameStage<k_type, BidStage<k_type>>;

  public:
    MainBidStage(MainStage<k_type>& main_stage)
        : StageFsm(main_stage, "投标阶段",
                MakeStageCommand(*this, "通过图片查看各玩家手牌及金币情况", &MainBidStage::Status_, VoidChecker("赛况")))
        , index_(0)
    {}

    virtual void FirstStageFsm(StageFsm::SubStageFsmSetter setter) override
    {
        static const auto cmp = [](const PokerItems<k_type>::value_type& _1, const PokerItems<k_type>::value_type& _2) {
            return _1.second.size() < _2.second.size() ||
                   (_1.second.size() == _2.second.size() && *_1.second.begin() < *_2.second.begin());
        };
        assert(!this->Main().poker_items().empty());
        std::sort(this->Main().poker_items().begin(), this->Main().poker_items().end(), cmp);
        {
            auto sender = this->Global().Boardcast();
            sender << this->Name() << "开始，请私信裁判进行投标，商品列表：";
            for (uint32_t i = 0; i < this->Main().poker_items().size(); ++i) {
                auto& poker_item = this->Main().poker_items()[i];
                sender << "\n" << (i + 1) << "号商品：";
                for (const auto& poker : poker_item.second) {
                    sender << poker << " ";
                }
            }
        }
        CreateBidStage(setter);
    }

    virtual void NextStageFsm(BidStage<k_type>& sub_stage, const CheckoutReason reason, StageFsm::SubStageFsmSetter setter) override
    {
        if ((++index_) == this->Main().poker_items().size()) {
            return;
        }
        CreateBidStage(setter);
    }

  private:
    std::string InfoHtml_() const
    {
        return this->Main().TitleHtml() + "\n\n" + this->Main().ItemInfoHtml(index_) + "\n\n" + this->Main().PlayerInfoHtml();
    }

    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        this->Global().Boardcast() << Markdown(InfoHtml_());
        return StageErrCode::OK;
    }

    void CreateBidStage(StageFsm::SubStageFsmSetter& setter)
    {
        this->Global().Boardcast() << Markdown(InfoHtml_());
        setter.template Emplace<BidStage<k_type>>(this->Main(), std::to_string(index_ + 1) + "号商品", this->Main().poker_items()[index_].first,
                this->Main().poker_items()[index_].second);
    }

    uint32_t index_;
};

template <poker::CardType k_type>
class DiscardStage : public SubGameStage<k_type>
{
    using StageFsm = SubGameStage<k_type>;

  public:
    DiscardStage(MainStage<k_type>& main_stage)
        : StageFsm(main_stage, "弃牌阶段",
                MakeStageCommand(*this, "查看各玩家手牌及金币情况", &DiscardStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "本回合不进行弃牌", &DiscardStage::Cancel_, VoidChecker("pass")),
                MakeStageCommand(*this, "决定本回合**所有的**弃牌", &DiscardStage::Discard_, RepeatableChecker<AnyArg>("扑克", "红3")))
    {}

    void OnStageBegin()
    {
        this->Global().Boardcast() << Markdown(InfoHtml_());
        this->Global().Boardcast() << "弃牌阶段开始，请一次性私信裁判**所有的**弃牌，当到达时间限制，或所有玩家皆选择完毕后，回合结束。"
                       "\n回合结束前您可以随意更改您的选择。";
        this->Global().StartTimer(GAME_OPTION(弃牌时间));
        for (const auto& player : this->Main().players()) {
            if (player.hand_.Empty()) {
                this->Global().SetReady(player.pid_); // need not discard
            }
        }
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        if (std::rand() % 2) {
            return StageErrCode::READY;
        }
        const auto best_deck = this->Main().players()[pid].hand_.BestDeck();
        std::vector<std::string> discard_poker_strs;
        const auto shuffled_pokers = poker::ShuffledPokers<k_type>();
        for (const auto& poker : shuffled_pokers) {
            // should not break the best deck
            if (this->Main().players()[pid].hand_.Has(poker) &&
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
        return this->Main().TitleHtml() + "\n\n" + this->Main().NonPlayerItemInfoHtml() + "\n\n" + this->Main().PlayerInfoHtml();
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        this->Global().Boardcast() << Markdown(InfoHtml_());
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
        std::set<poker::Card<k_type>> pokers_to_discard;
        for (const auto& poker_str : poker_strs) {
            std::stringstream ss;
            if (const auto poker = poker::Parse<std::string, std::stringstream&, k_type>(poker_str, ss); !poker.has_value()) {
                reply() << "弃牌失败：非法的扑克名“" << poker_str << "”，" << ss.str();
                return StageErrCode::FAILED;
            } else if (!this->Main().players()[pid].hand_.Has(*poker)) {
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
        for (auto& [discarder, pokers] : this->Main().poker_items()) {
            if (discarder.has_value() && *discarder == pid) {
                pokers = std::move(pokers_to_discard);
                return StageErrCode::READY;
            }
        }
        this->Main().poker_items().emplace_back(pid, std::move(pokers_to_discard));
        return StageErrCode::READY;
    }

    AtomReqErrCode Cancel_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "失败：请私信裁判取消弃牌";
            return StageErrCode::FAILED;
        }
        for (auto& [discarder, pokers] : this->Main().poker_items()) {
            if (discarder.has_value() && *discarder == pid) {
                pokers.clear();
            }
        }
        reply() << "您当前选择不弃牌";
        return StageErrCode::READY;
    }

    CheckoutErrCode OnStageTimeout()
    {
        return StageErrCode::CHECKOUT;
    }
};

template <poker::CardType k_type>
class RoundStage : public SubGameStage<k_type, DiscardStage<k_type>, MainBidStage<k_type>>
{
    using StageFsm = SubGameStage<k_type, DiscardStage<k_type>, MainBidStage<k_type>>;

  public:
    RoundStage(MainStage<k_type>& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第" + std::to_string(round) + "回合")
    {}

    virtual void FirstStageFsm(StageFsm::SubStageFsmSetter setter) override
    {
        this->Global().Boardcast() << this->Name() << "开始！";
        setter.template Emplace<DiscardStage<k_type>>(this->Main());
    }

    virtual void NextStageFsm(DiscardStage<k_type>& sub_stage, const CheckoutReason reason, StageFsm::SubStageFsmSetter setter) override
    {
        std::erase_if(this->Main().poker_items(), [](const PokerItems<k_type>::value_type& item) { return item.second.empty(); });
        if (this->Main().poker_items().empty()) {
            this->Global().Boardcast() << "无商品，跳过投标阶段";
            return;
        }
        for (const auto& [discarder_pid, pokers] : this->Main().poker_items()) {
            if (discarder_pid.has_value()) {
                auto& discarder = this->Main().players()[*discarder_pid];
                for (const auto& poker : pokers) {
                    discarder.hand_.Remove(poker);
                }
            }
        }
        this->Main().UpdatePlayerScore();
        setter.template Emplace<MainBidStage<k_type>>(this->Main());
    }

    virtual void NextStageFsm(MainBidStage<k_type>& sub_stage, const CheckoutReason reason, StageFsm::SubStageFsmSetter setter) override
    {
        return;
    }
};

template <poker::CardType k_type>
void MainStage<k_type>::FirstStageFsm(StageFsm::SubStageFsmSetter setter)
{
    int pos = 0;
    const auto shuffled_pokers = poker::ShuffledPokers<k_type>(GAME_OPTION(种子));
    const auto emplace_pockers = [this, &shuffled_pokers, &pos](const int num)
        {
            poker_items_.emplace_back(std::nullopt, std::set<poker::Card<k_type>>(shuffled_pokers.begin() + pos, shuffled_pokers.begin() + pos + num));
            pos += num;
        };
    assert(shuffled_pokers.size() == 40 || shuffled_pokers.size() == 52);
    while ((shuffled_pokers.size() - pos) % 5) {
        emplace_pockers(6);
    }
    while (pos < shuffled_pokers.size()) {
        emplace_pockers(5);
    }
    setter.template Emplace<MainBidStage<k_type>>(*this);
}

template <poker::CardType k_type>
void MainStage<k_type>::NextStageFsm(MainBidStage<k_type>& sub_stage, const CheckoutReason reason, typename MainStage<k_type>::StageFsm::SubStageFsmSetter setter)
{
    setter.template Emplace<RoundStage<k_type>>(*this, ++round_);
}

template <poker::CardType k_type>
void MainStage<k_type>::NextStageFsm(RoundStage<k_type>& sub_stage, const CheckoutReason reason, MainStage<k_type>::StageFsm::SubStageFsmSetter setter)
{
    if ((++round_) <= GAME_OPTION(回合数)) {
        setter.template Emplace<RoundStage<k_type>>(*this, round_);
        return;
    }
    this->Global().Boardcast() << Markdown(TitleHtml() + "\n\n" + PlayerInfoHtml());
}

internal::MainStage* MakeMainStage(MainStageFactory factory)
{
    if (GET_OPTION_VALUE(factory.GetGameOptions(), 卡牌) == poker::CardType::BOKAA) {
        return factory.Create<MainStage<poker::CardType::BOKAA>>();
    } else if (GET_OPTION_VALUE(factory.GetGameOptions(), 卡牌) == poker::CardType::POKER) {
        return factory.Create<MainStage<poker::CardType::POKER>>();
    } else {
        assert(false);
        return nullptr;
    }
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot
