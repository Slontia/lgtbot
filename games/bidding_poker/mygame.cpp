#include <array>
#include <functional>
#include <memory>
#include <set>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"

#include "game_util/bidding.h"
#include "game_util/poker.h"
#include "game_util/util.h"

const std::string k_game_name = "投标扑克";
const uint64_t k_max_player = 0; /* 0 means no max-player limits */

const std::string GameOption::StatusInfo() const
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

struct Player
{
    Player(const PlayerID pid, const uint32_t coins) : pid_(pid), coins_(coins) {}
    const PlayerID pid_;
    int32_t coins_;
    poker::Hand hand_;
};

class MainBidStage;
class RoundStage;
using PokerItems = std::vector<std::pair<std::optional<PlayerID>, std::set<poker::Poker>>>;

class MainStage : public MainGameStage<MainBidStage, RoundStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match, "主阶段", MakeStageCommand("场况", &MainStage::Status_, VoidChecker("场况")))
        , round_(0)
    {
        for (PlayerID pid = 0; pid < option.PlayerNum(); ++pid) {
            players_.emplace_back(pid, option.GET_VALUE(初始金币数));
        }
    }

    virtual VariantSubStage OnStageBegin() override;

    virtual VariantSubStage NextSubStage(MainBidStage& sub_stage, const CheckoutReason reason) override;

    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    int64_t PlayerScore(const PlayerID pid) const { return players_[pid].coins_; }

    PokerItems& poker_items() { return poker_items_; }
    const PokerItems& poker_items() const { return poker_items_; }
    std::vector<Player>& players() { return players_; }
    const std::vector<Player>& players() const { return players_; }

  private:
    CompStageErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
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
        return std::accumulate(players_.begin(), players_.end(), option().GET_VALUE(初始金币数) * option().PlayerNum(),
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
                MakeStageCommand("投标", &BidStage::Bid_, ArithChecker<uint32_t>(0, main_stage.players().size() * main_stage.option().GET_VALUE(初始金币数), "金币数")),
                MakeStageCommand("撤标", &BidStage::Cancel_, VoidChecker("撤标"))),
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
        } else {
            sender << "（无）";
        }
        sender << "\n投标开始，请私信裁判进行投标";
        StartTimer(option().GET_VALUE(投标时间));
    }


    AtomStageErrCode OnTimeout() { return BidOver_(); }
    AtomStageErrCode OnAllPlayerReady() { return BidOver_(); }

  private:
    AtomStageErrCode BidOver_()
    {
        auto sender = Boardcast();
        const auto ret = bidding_manager_.BidOver(sender);
        if (ret.second.size() == 1) {
            assert(ret.first.has_value());
            const auto& max_chip = *ret.first;
            auto& winner = main_stage().players()[ret.second[0]];
            winner.coins_ -= max_chip;
            for (const auto& poker : pokers_) {
                winner.hand_.Add(poker);
            }
            if (discarder_.has_value()) {
                auto& discarder = main_stage().players()[*discarder_];
                for (const auto& poker : pokers_) {
                    discarder.hand_.Remove(poker);
                }
                discarder.coins_ += max_chip;
            }
            pokers_.clear();
            sender << "\n恭喜" << At(ret.second[0]) << "中标，现持有卡牌：\n" << winner.hand_;
            return StageErrCode::CHECKOUT;
        } else if ((++bid_count_) == option().GET_VALUE(投标轮数)) {
            sender << "\n投标轮数达到最大值，本商品流标";
            if (discarder_.has_value()) {
                pokers_.clear();
            }
            return StageErrCode::CHECKOUT;
        } else {
            auto sender = Boardcast();
            sender << "\n最大金额投标者有多名玩家，分别是：";
            for (const auto& winner : ret.second) {
                sender << At(winner);
            }
            sender << "\n开始新的一轮投标，这些玩家可在此轮中重新投标（投标额不得少于上一轮）";
            if (bid_count_ + 1 == option().GET_VALUE(投标轮数)) {
                sender << "\n注意：若本轮仍未能决出中标者，则商品流标";
            }
            StartTimer(option().GET_VALUE(投标时间));
            ClearReady();
            return StageErrCode::OK;
        }
    }

    AtomStageErrCode Bid_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t chip)
    {
        if (is_public) {
            reply() << "投标失败：请私信裁判进行投标";
            return StageErrCode::FAILED;
        }
        if (discarder_.has_value() && pid == *discarder_) {
            reply() << "投标失败：您是该商品的拍卖者，不可以进行投标";
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
        return bidding_manager_.Bid(pid, chip, reply()) ? StageErrCode::READY : StageErrCode::FAILED;
    }

    AtomStageErrCode Cancel_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "撤标失败：请私信裁判撤标";
            return StageErrCode::FAILED;
        }
        if (discarder_.has_value() && pid == *discarder_) {
            reply() << "撤标失败：您是该商品的拍卖者，不可以撤标";
            return StageErrCode::FAILED;
        }
        return bidding_manager_.Bid(pid, std::nullopt, reply()) ? StageErrCode::READY : StageErrCode::FAILED;
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
        : GameStage(main_stage, "投标阶段"), index_(0)
    {}

    virtual VariantSubStage OnStageBegin() override
    {
        static const auto cmp = [](const PokerItems::value_type& _1, const PokerItems::value_type& _2) {
            return _1.second.size() < _2.second.size() ||
                   (_1.second.size() == _2.second.size() && *_1.second.begin() < *_2.second.begin());
        };
        assert(!main_stage().poker_items().empty());
        std::sort(main_stage().poker_items().begin(), main_stage().poker_items().end(), cmp);
        auto sender = Boardcast();
        sender << name_ << "开始，请私信裁判进行投标，商品列表：";
        for (uint32_t i = 0; i < main_stage().poker_items().size(); ++i) {
            auto& poker_item = main_stage().poker_items()[i];
            sender << "\n" << (i + 1) << "号商品：";
            for (const auto& poker : poker_item.second) {
                sender << poker << " ";
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
    VariantSubStage CreateBidStage()
    {
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
                MakeStageCommand("弃牌", &DiscardStage::Discard_, VoidChecker("弃牌"), RepeatableChecker<AnyArg>("弃牌列表", "红桃A")),
                MakeStageCommand("不弃牌", &DiscardStage::Cancel_, VoidChecker("不弃牌")))
    {}

    void OnStageBegin()
    {
        Boardcast() << "弃牌阶段开始，请私信裁判进行弃牌，当到达时间限制，或所有玩家皆选择完毕后，回合结束。"
                       "\n回合结束前您可以随意更改您的选择。";
        StartTimer(option().GET_VALUE(弃牌时间));
    }

  private:
    AtomStageErrCode Discard_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::vector<std::string>& poker_strs)
    {
        if (is_public) {
            reply() << "弃牌失败：请私信裁判进行弃牌";
            return StageErrCode::FAILED;
        }
        if (poker_strs.empty()) {
            reply() << "弃牌失败：弃牌为空";
            return StageErrCode::FAILED;
        }
        std::set<poker::Poker> pokers_to_discard;
        for (const auto& poker_str : poker_strs) {
            std::stringstream ss;
            if (const auto poker = poker::Parse(poker_str, ss); !poker.has_value()) {
                reply() << "弃牌失败：非法的扑克名“" << poker_str << "”，" << ss.str();
                return StageErrCode::FAILED;
            } else if (!main_stage().players()[pid].hand_.Has(*poker)) {
                reply() << "弃牌失败：您未持有这张扑克牌";
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
                return StageErrCode::OK;
            }
        }
        main_stage().poker_items().emplace_back(pid, std::move(pokers_to_discard));
        return StageErrCode::READY;
    }

    AtomStageErrCode Cancel_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
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
        reply() << "成功，您当前选择不弃牌";
        return StageErrCode::READY;
    }

    AtomStageErrCode OnTimeout()
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
        return std::make_unique<MainBidStage>(main_stage());
    }

    virtual VariantSubStage NextSubStage(MainBidStage& sub_stage, const CheckoutReason reason) override
    {
        return {};
    }
};

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    const auto shuffled_pokers = poker::ShuffledPokers(option().GET_VALUE(种子));
    assert(shuffled_pokers.size() == 52);
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 0, shuffled_pokers.begin() + 5));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 5, shuffled_pokers.begin() + 10));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 10, shuffled_pokers.begin() + 15));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 15, shuffled_pokers.begin() + 20));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 20, shuffled_pokers.begin() + 25));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 25, shuffled_pokers.begin() + 30));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 30, shuffled_pokers.begin() + 35));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 35, shuffled_pokers.begin() + 40));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 40, shuffled_pokers.begin() + 46));
    poker_items_.emplace_back(std::nullopt, std::set<poker::Poker>(shuffled_pokers.begin() + 46, shuffled_pokers.begin() + 52));
    return std::make_unique<MainBidStage>(*this);
}

MainStage::VariantSubStage MainStage::NextSubStage(MainBidStage& sub_stage, const CheckoutReason reason)
{
    return std::make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if ((++round_) <= option().GET_VALUE(回合数)) {
        return std::make_unique<RoundStage>(*this, round_);
    }

    auto sender = Boardcast();
    sender << "== 游戏结束 ==\n";
    // calculcate rank
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
    if (best_decks.empty()) {
        sender << "无人凑到有效牌型，故直接根据剩余金币数计算排名";
        return {};
    }
    std::sort(best_decks.begin(), best_decks.end(), [](const DeckElement& _1, const DeckElement& _2) { return _1.second > _2.second; });
    sender << "【各玩家牌型】";
    for (const auto& [pid, deck] : best_decks) {
        sender << "\n" << At(pid) << "：" << deck;
    }
    for (const auto& pid : no_deck_players) {
        sender << "\n" << At(pid) << "未组成合法牌型";
    }
    static const auto half = [](auto& value, const uint32_t split)
    {
        const auto half_value = value - value / split;
        value = value - half_value;
        return half_value;
    };

    uint32_t bonus_coins = BonusCoins_();

    // decrease coins
    const auto decrese_coins = [&](const PlayerID pid)
    {
        sender << "\n" << At(pid) << "原持有金币" << players_[pid].coins_ << "枚，";
        const auto lost_coins = half(players_[pid].coins_, 2);
        bonus_coins += lost_coins;
        sender << "扣除末位惩罚金币" << lost_coins << "枚后，剩余" << players_[pid].coins_ << "枚，"
                << "奖池金币增加至" << bonus_coins << "枚";
    };
    sender << "\n\n【金币扣除情况】（扣除前，奖池金币共" << bonus_coins << "枚）";
    if (no_deck_players.empty()) {
        decrese_coins(best_decks.back().first);
    } else {
        for (const auto& loser_pid : no_deck_players) {
            decrese_coins(loser_pid);
        }
    }

    // increase coins
    sender << "\n\n【金币奖励情况】";
    const auto increase_coins = [&](const PlayerID pid, const uint32_t coins)
    {
        sender << "\n" << At(pid) << "原持有金币" << players_[pid].coins_ << "枚，";
        players_[pid].coins_ += coins;
        sender << "获得顺位奖励金币" << coins << "枚后，现持有金币" << players_[pid].coins_ << "枚";
    };
    for (uint64_t i = 0; i < best_decks.size() - 1; ++i) {
        increase_coins(best_decks[i].first, half(bonus_coins, 3));
    }
    increase_coins(best_decks.back().first, bonus_coins);
    return {};
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match)
{
    if (options.PlayerNum() < 2) {
        reply() << "该游戏至少2人参加，当前玩家数为" << options.PlayerNum();
        return nullptr;
    }
    return new MainStage(options, match);
}
