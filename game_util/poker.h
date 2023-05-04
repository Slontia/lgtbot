// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(BokaaSuit)
ENUM_MEMBER(BokaaSuit, PURPLE)
ENUM_MEMBER(BokaaSuit, BLUE)
ENUM_MEMBER(BokaaSuit, RED)
ENUM_MEMBER(BokaaSuit, GREEN)
ENUM_END(BokaaSuit)

ENUM_BEGIN(BokaaNumber)
ENUM_MEMBER(BokaaNumber, _1)
ENUM_MEMBER(BokaaNumber, _2)
ENUM_MEMBER(BokaaNumber, _3)
ENUM_MEMBER(BokaaNumber, _4)
ENUM_MEMBER(BokaaNumber, _5)
ENUM_MEMBER(BokaaNumber, _6)
ENUM_MEMBER(BokaaNumber, _7)
ENUM_MEMBER(BokaaNumber, _8)
ENUM_MEMBER(BokaaNumber, _9)
ENUM_MEMBER(BokaaNumber, _X)
ENUM_END(BokaaNumber)

ENUM_BEGIN(PokerSuit)
ENUM_MEMBER(PokerSuit, CLUBS)
ENUM_MEMBER(PokerSuit, DIAMONDS)
ENUM_MEMBER(PokerSuit, HEARTS)
ENUM_MEMBER(PokerSuit, SPADES)
ENUM_END(PokerSuit)

ENUM_BEGIN(PokerNumber)
ENUM_MEMBER(PokerNumber, _2)
ENUM_MEMBER(PokerNumber, _3)
ENUM_MEMBER(PokerNumber, _4)
ENUM_MEMBER(PokerNumber, _5)
ENUM_MEMBER(PokerNumber, _6)
ENUM_MEMBER(PokerNumber, _7)
ENUM_MEMBER(PokerNumber, _8)
ENUM_MEMBER(PokerNumber, _9)
ENUM_MEMBER(PokerNumber, _10)
ENUM_MEMBER(PokerNumber, _J)
ENUM_MEMBER(PokerNumber, _Q)
ENUM_MEMBER(PokerNumber, _K)
ENUM_MEMBER(PokerNumber, _A)
ENUM_END(PokerNumber)


ENUM_BEGIN(PatternType)
ENUM_MEMBER(PatternType, HIGH_CARD)
ENUM_MEMBER(PatternType, ONE_PAIR)
ENUM_MEMBER(PatternType, TWO_PAIRS)
ENUM_MEMBER(PatternType, THREE_OF_A_KIND)
ENUM_MEMBER(PatternType, STRAIGHT)
ENUM_MEMBER(PatternType, FLUSH)
ENUM_MEMBER(PatternType, FULL_HOUSE)
ENUM_MEMBER(PatternType, FOUR_OF_A_KIND)
ENUM_MEMBER(PatternType, STRAIGHT_FLUSH)
ENUM_END(PatternType)

#endif
#endif
#endif

#ifndef POKER_H_
#define POKER_H_

#include <array>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <cassert>
#include <random>
#include <sstream>
#include <utility> // g++12 has a bug which will cause 'exchange' is not a member of 'std'
#include <algorithm>
#include <bitset>


#include "utility/html.h"

namespace lgtbot {

namespace game_util {

namespace poker {

#define ENUM_FILE "../game_util/poker.h"
#include "../utility/extend_enum.h"

enum class CardType { POKER, BOKAA };

template <CardType k_type> struct Types;

template <>
struct Types<CardType::BOKAA>
{
    using NumberType = BokaaNumber;
    using SuitType = BokaaSuit;
    static constexpr BokaaNumber k_max_number_ = BokaaNumber::_X;
    static constexpr const char* const k_regex = "(.*)([Xx1-9])";
    static inline const std::map<std::string, BokaaSuit> k_parse_suit_map{
        {"绿", BokaaSuit::GREEN},
        {"星", BokaaSuit::GREEN},
        {"★",  BokaaSuit::GREEN},
        {"☆",  BokaaSuit::GREEN},
        {"红", BokaaSuit::RED},
        {"方", BokaaSuit::RED},
        {"■",  BokaaSuit::RED},
        {"□",  BokaaSuit::RED},
        {"蓝", BokaaSuit::BLUE},
        {"角", BokaaSuit::BLUE},
        {"▲",  BokaaSuit::BLUE},
        {"△",  BokaaSuit::BLUE},
        {"紫", BokaaSuit::PURPLE},
        {"圆", BokaaSuit::PURPLE},
        {"●",  BokaaSuit::PURPLE},
        {"○",  BokaaSuit::PURPLE},
    };
    static constexpr const char* const k_colored_suit_str[BokaaNumber::Count()] = {
        [BokaaSuit(BokaaSuit::PURPLE).ToUInt()] = HTML_COLOR_FONT_HEADER(purple) "●",
        [BokaaSuit(BokaaSuit::BLUE).ToUInt()]   = HTML_COLOR_FONT_HEADER(blue) "▲",
        [BokaaSuit(BokaaSuit::RED).ToUInt()]    = HTML_COLOR_FONT_HEADER(red) "■",
        [BokaaSuit(BokaaSuit::GREEN).ToUInt()]  = HTML_COLOR_FONT_HEADER(green) "★",
    };
    static constexpr const char* const k_suit_str[BokaaNumber::Count()] = {
        [BokaaSuit(BokaaSuit::PURPLE).ToUInt()] = HTML_COLOR_FONT_HEADER(purple) "○",
        [BokaaSuit(BokaaSuit::BLUE).ToUInt()]   = HTML_COLOR_FONT_HEADER(blue) "△",
        [BokaaSuit(BokaaSuit::RED).ToUInt()]    = HTML_COLOR_FONT_HEADER(red) "□",
        [BokaaSuit(BokaaSuit::GREEN).ToUInt()]  = HTML_COLOR_FONT_HEADER(green) "☆",
    };
};

template <>
struct Types<CardType::POKER>
{
    using NumberType = PokerNumber;
    using SuitType = PokerSuit;
    static constexpr PokerNumber k_max_number_ = PokerNumber::_A;
    static constexpr const char* const k_regex = "(.*)([AaJjQqKk1-9]|10)";
    static inline const std::map<std::string, PokerSuit> k_parse_suit_map{
        {"黑桃", PokerSuit::SPADES},
        {"♠",    PokerSuit::SPADES},
        {"红桃", PokerSuit::HEARTS},
        {"红心", PokerSuit::HEARTS},
        {"♥",    PokerSuit::HEARTS},
        {"方片", PokerSuit::DIAMONDS},
        {"方板", PokerSuit::DIAMONDS},
        {"♦",    PokerSuit::DIAMONDS},
        {"梅花", PokerSuit::CLUBS},
        {"♣",    PokerSuit::CLUBS},
    };
    static constexpr const char* const k_colored_suit_str[PokerNumber::Count()] = {
        [PokerSuit(PokerSuit::CLUBS).ToUInt()]    = HTML_COLOR_FONT_HEADER(black) "♣",
        [PokerSuit(PokerSuit::DIAMONDS).ToUInt()] = HTML_COLOR_FONT_HEADER(red) "♦",
        [PokerSuit(PokerSuit::HEARTS).ToUInt()]   = HTML_COLOR_FONT_HEADER(red) "♥",
        [PokerSuit(PokerSuit::SPADES).ToUInt()]   = HTML_COLOR_FONT_HEADER(black) "♠",
    };
    static constexpr const char* const k_suit_str[PokerNumber::Count()] = {
        [PokerSuit(PokerSuit::CLUBS).ToUInt()]    = "♧",
        [PokerSuit(PokerSuit::DIAMONDS).ToUInt()] = "♢",
        [PokerSuit(PokerSuit::HEARTS).ToUInt()]   = "♡",
        [PokerSuit(PokerSuit::SPADES).ToUInt()]   = "♤",
    };
};

template <CardType k_type>
struct Card
{
    auto operator<=>(const Card&) const = default;
    std::string ToString() const;
    std::string ToHtml() const;
    Types<k_type>::NumberType number_;
    Types<k_type>::SuitType suit_;
};

template <CardType k_type>
void swap(Card<k_type>& _1, Card<k_type>& _2)
{
    std::swap(const_cast<Types<k_type>::NumberType&>(_1.number_), const_cast<Types<k_type>::NumberType&>(_2.number_));
    std::swap(const_cast<Types<k_type>::SuitType&>(_1.suit_), const_cast<Types<k_type>::SuitType&>(_2.suit_));
}

template <CardType k_type>
std::vector<Card<k_type>> ShuffledPokers(const std::string_view& sv = "")
{
    std::vector<Card<k_type>> cards;
    for (const auto& number : Types<k_type>::NumberType::Members()) {
        for (const auto& suit : Types<k_type>::SuitType::Members()) {
            cards.emplace_back(number, suit);
        }
    }
    if (sv.empty()) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(cards.begin(), cards.end(), g);
    } else {
        std::seed_seq seed(sv.begin(), sv.end());
        std::mt19937 g(seed);
        std::shuffle(cards.begin(), cards.end(), g);
    }
    return cards;
}

template <CardType k_type>
std::string Card<k_type>::ToHtml() const
{
    return std::string(Types<k_type>::k_colored_suit_str[suit_.ToUInt()]) + (number_.ToString() + 1) + HTML_FONT_TAIL; // skip the '_' character
}

template <typename Sender, CardType k_type>
Sender& operator<<(Sender& sender, const Card<k_type>& poker)
{
    sender << Types<k_type>::k_suit_str[poker.suit_.ToUInt()] << (poker.number_.ToString() + 1); // skip the '_' character
    return sender;
}

template <CardType k_type>
std::string Card<k_type>::ToString() const
{
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

template <typename String, typename Sender, CardType k_type>
std::optional<typename Types<k_type>::SuitType> ParseSuit(const String& s, Sender&& sender)
{
    static const auto& k_parse_suit_map = Types<k_type>::k_parse_suit_map;
    const auto it = k_parse_suit_map.find(s);
    if (it == k_parse_suit_map.end()) {
        sender << "非预期的花色\'" << s << "\'，期望为：";
        for (const auto& [str, _] : k_parse_suit_map) {
            sender << "\'" << str << "\' ";
        }
        return std::nullopt;
    } else {
        return it->second;
    }
}

template <typename Sender, CardType k_type>
void ShowExpectedNumberStrs_(Sender&& sender)
{
    for (const auto& number : Types<k_type>::NumberType::Members()) {
        sender << "\'" << (number.ToString() + 1) << "\' "; // skip the '_' character
    }
}

template <typename String, typename Sender, CardType k_type>
std::optional<typename Types<k_type>::NumberType> ParseNumber(const String& s, Sender&& sender)
{
    std::string number_str = "_" + std::string(s);
    number_str[1] = std::toupper(number_str[1]); // lower case is expected
    const auto parse_ret = Types<k_type>::NumberType::Parse(number_str);
    if (parse_ret.has_value()) {
        sender << "非预期的点数\'" << s << "\'，期望为：";
        ShowExpectedNumberStrs_<Sender, k_type>(sender);
    }
    return parse_ret;
}

template <typename String, typename Sender, CardType k_type>
std::optional<Card<k_type>> Parse(const String& s, Sender&& sender)
{
    using namespace std::literals;
    std::smatch match_ret;
    typename Types<k_type>::NumberType number_;
    typename Types<k_type>::SuitType suit_;
    if (!std::regex_match(s, match_ret, std::regex(Types<k_type>::k_regex))) {
        sender << "非预期的点数，期望为：";
        ShowExpectedNumberStrs_<Sender, k_type>(sender);
        return std::nullopt;
    }
    assert(match_ret.size() == 3);
    if (const auto suit = ParseSuit<std::smatch::value_type, Sender, k_type>(match_ret[1], sender); !suit.has_value()) {
        return std::nullopt;
    } else if (const auto number = ParseNumber<std::smatch::value_type, Sender, k_type>(match_ret[2], sender); !suit.has_value()) {
        return std::nullopt;
    } else {
        return Card<k_type>(*number, *suit);
    }
}

template <CardType k_type>
struct Deck
{
    Deck& operator=(const Deck& deck)
    {
        this->~Deck();
        new(this) Deck(deck.type_, deck.pokers_);
        return *this;
    }
    auto operator<=>(const Deck&) const = default;
    int CompareIgnoreSuit(const Deck& d) const
    {
        if (type_ < d.type_) {
            return -1;
        } else if (type_ > d.type_) {
            return 1;
        }
        static const auto cmp = [](const Card<k_type>& _1, const Card<k_type>& _2) { return _1.number_ < _2.number_; };
        if (std::lexicographical_compare(pokers_.begin(), pokers_.end(), d.pokers_.begin(), d.pokers_.end(), cmp)) {
            return -1;
        } else if (std::equal(pokers_.begin(), pokers_.end(), d.pokers_.begin(), d.pokers_.end(), cmp)) {
            return 0;
        } else {
            return 1;
        }
    }
    const char* TypeName() const;
    const PatternType type_;
    const std::array<Card<k_type>, 5> pokers_;
};

template <CardType k_type>
const char* Deck<k_type>::TypeName() const
{
    switch (type_) {
        case PatternType::HIGH_CARD: return "高牌";
        case PatternType::ONE_PAIR: return "一对";
        case PatternType::TWO_PAIRS: return "两对";
        case PatternType::THREE_OF_A_KIND: return "三条";
        case PatternType::STRAIGHT: return "顺子";
        case PatternType::FLUSH: return "同花";
        case PatternType::FULL_HOUSE: return "满堂红";
        case PatternType::FOUR_OF_A_KIND: return "四条";
        case PatternType::STRAIGHT_FLUSH:
            if (pokers_.front().number_ == Types<k_type>::k_max_number_) {
                return "皇家同花顺";
            } else {
                return "同花顺";
            }
    }
    return "【错误：未知的牌型】";
}

template <typename Sender, CardType k_type>
Sender& operator<<(Sender& sender, const Deck<k_type>& deck)
{
    sender << "[" << deck.TypeName() << "]";
    for (const auto& poker : deck.pokers_) {
        sender << " " << poker;
    }
    return sender;
}

template <CardType k_type>
class Hand
{
    using NumberType = Types<k_type>::NumberType;
    using SuitType = Types<k_type>::SuitType;

   public:
    Hand() : pokers_{{false}}, need_refresh_(false) {}

    bool Add(const NumberType& number, const SuitType& suit)
    {
        const auto old_value = std::exchange(pokers_[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)], true);
        if (old_value == false) {
            need_refresh_ = true;
            return true;
        }
        return false;
    }

    bool Add(const Card<k_type>& poker) { return Add(poker.number_, poker.suit_); }

    bool Remove(const NumberType& number, const SuitType& suit)
    {
        const auto old_value = std::exchange(pokers_[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)], false);
        if (old_value == true) {
            need_refresh_ = true;
            return true;
        }
        return false;
    }

    bool Remove(const Card<k_type>& poker) { return Remove(poker.number_, poker.suit_); }

    bool Has(const NumberType& number, const SuitType& suit) const
    {
        return pokers_[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)];
    }

    bool Has(const Card<k_type>& poker) const { return Has(poker.number_, poker.suit_); }

    bool Empty() const
    {
        return std::all_of(pokers_.begin(), pokers_.end(),
                [](const auto& array) { return std::all_of(array.begin(), array.end(),
                    [](const bool has) { return !has; }); });
    }

    template <typename Sender>
    friend Sender& operator<<(Sender& sender, const Hand& hand)
    {
        for (const auto& number : NumberType::Members()) {
            for (const auto& suit : SuitType::Members()) {
                if (hand.Has(number, suit)) {
                    sender << Card<k_type>(number, suit) << " ";
                }
            }
        }
        const auto& best_deck = hand.BestDeck();
        if (best_deck.has_value()) {
            sender << "（" << *best_deck << "）";
        }
        return sender;
    }

    std::string ToHtml() const
    {
        std::string s;
        for (const auto& number : NumberType::Members()) {
            for (const auto& suit : SuitType::Members()) {
                if (Has(number, suit)) {
                    s += Card<k_type>(number, suit).ToHtml() + " ";
                }
            }
        }
        return s;
    }

    const std::optional<Deck<k_type>>& BestDeck() const
    {
        if (!need_refresh_) {
            return best_deck_;
        }
        need_refresh_ = false;
        best_deck_ = std::nullopt;
        const auto update_deck = [this](const std::optional<Deck<k_type>>& deck) {
            if (!deck.has_value()) {
                return;
            } else if (!best_deck_.has_value() || *best_deck_ < *deck) {
                best_deck_.emplace(*deck);
            }
        };

        for (auto suit_it = SuitType::Members().rbegin(); suit_it != SuitType::Members().rend(); ++suit_it) {
            update_deck(BestFlushPattern_<true>(*suit_it));
        }
        if (best_deck_.has_value()) {
            return best_deck_;
        }

        update_deck(BestPairPattern_());
        if (best_deck_.has_value() && best_deck_->type_ >= PatternType::FULL_HOUSE) {
            return best_deck_;
        }

        for (auto suit_it = SuitType::Members().rbegin(); suit_it != SuitType::Members().rend(); ++suit_it) {
            update_deck(BestFlushPattern_<false>(*suit_it));
        }
        if (best_deck_.has_value() && best_deck_->type_ >= PatternType::FLUSH) {
            return best_deck_;
        }

        update_deck(BestNonFlushNonPairPattern_());

        return best_deck_;
    }

   private:
    std::optional<Deck<k_type>> BestNonFlushNonPairPattern_() const {
        const auto get_poker = [&pokers = pokers_](const NumberType number) -> std::optional<Card<k_type>> {
            for (auto suit_it = SuitType::Members().rbegin(); suit_it != SuitType::Members().rend(); ++suit_it) {
                if (pokers[static_cast<uint32_t>(number)][static_cast<uint32_t>(*suit_it)]) {
                    return Card<k_type>(number, *suit_it);
                }
            }
            return std::nullopt;
        };
        const auto deck = CollectNonPairDeck_<true>(get_poker);
        if (deck.has_value()) {
            return Deck<k_type>(PatternType::STRAIGHT, *deck);
        } else {
            return std::nullopt;
        }
    }

    template <bool FIND_STRAIGHT>
    std::optional<Deck<k_type>> BestFlushPattern_(const SuitType suit) const
    {
        const auto get_poker = [&suit, &pokers = pokers_](const NumberType number) -> std::optional<Card<k_type>> {
            if (pokers[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)]) {
                return Card<k_type>(number, suit);
            } else {
                return std::nullopt;
            }
        };
        const auto deck = CollectNonPairDeck_<FIND_STRAIGHT>(get_poker);
        if (deck.has_value()) {
            return Deck<k_type>(PatternType::Condition(FIND_STRAIGHT, PatternType::STRAIGHT_FLUSH, PatternType::FLUSH), *deck);
        } else {
            return std::nullopt;
        }
    }

    template <bool FIND_STRAIGHT>
    static std::optional<std::array<Card<k_type>, 5>> CollectNonPairDeck_(const auto& get_poker)
    {
        std::vector<Card<k_type>> pokers;
        for (auto it = NumberType::Members().rbegin(); it != NumberType::Members().rend(); ++it) {
            const auto poker = get_poker(*it);
            if (poker.has_value()) {
                pokers.emplace_back(*poker);
                if (pokers.size() == 5) {
                    return std::array<Card<k_type>, 5>{pokers[0], pokers[1], pokers[2], pokers[3], pokers[4]};
                }
            } else if (FIND_STRAIGHT) {
                pokers.clear();
            }
        }
        if (const auto poker = get_poker(Types<k_type>::k_max_number_); FIND_STRAIGHT && pokers.size() == 4 && poker.has_value()) {
            return std::array<Card<k_type>, 5>{pokers[0], pokers[1], pokers[2], pokers[3], *poker};
        } else {
            return std::nullopt;
        }
    }

    std::optional<Deck<k_type>> BestPairPattern_() const
    {
        // If poker_ is AA22233334, the same_number_poker_counts will be:
        // [0]: A 4 3 2 (at least has one)
        // [1]: A 3 2 (at least has two)
        // [2]: 3 2 (at least has three)
        // [3]: 3 (at least has four)
        // Then we go through from the back of poker_number to fill the deck.
        // When at [3], the deck become 3333?
        // When at [2], the deck become 3333A, which is the result deck.
        std::array<std::deque<NumberType>, SuitType::Count()> same_number_poker_counts_accurate;
        std::array<std::deque<NumberType>, SuitType::Count()> same_number_poker_counts;
        for (const auto number : NumberType::Members()) {
            const uint64_t count = std::count(pokers_[static_cast<uint32_t>(number)].begin(),
                                              pokers_[static_cast<uint32_t>(number)].end(), true);
            if (count > 0) {
                same_number_poker_counts_accurate[count - 1].emplace_back(number);
                for (uint64_t i = 0; i < count; ++i) {
                    same_number_poker_counts[i].emplace_back(number);
                }
            }
        }
        std::set<NumberType> already_used_numbers;
        std::vector<Card<k_type>> pokers;

        const auto fill_pair_to_deck = [&](const NumberType& number)
        {
            for (auto suit_it = SuitType::Members().rbegin();  suit_it != SuitType::Members().rend(); ++suit_it) {
                if (pokers_[static_cast<uint32_t>(number)][static_cast<uint32_t>(*suit_it)]) {
                    pokers.emplace_back(number, *suit_it);
                    if (pokers.size() == 5) {
                        return;
                    }
                }
            }
        };

        const auto fill_best_pair_to_deck = [&]()
        {
            // fill big pair poker first
            for (int64_t i = std::min(SuitType::Count(), 5 - pokers.size()) - 1; i >= 0; --i) {
                const auto& owned_numbers = same_number_poker_counts[i];
                // fill big number poker first
                for (auto number_it = owned_numbers.rbegin(); number_it != owned_numbers.rend(); ++number_it) {
                    if (already_used_numbers.emplace(*number_it).second) {
                        fill_pair_to_deck(*number_it);
                        return true;
                    }
                }
            }
            return false;
        };

        while (fill_best_pair_to_deck() && pokers.size() < 5)
            ;
        if (pokers.size() < 5) {
            return std::nullopt;
        }
        return Deck<k_type>(PairPatternType_(same_number_poker_counts_accurate),
                    std::array<Card<k_type>, 5>{pokers[0], pokers[1], pokers[2], pokers[3], pokers[4]});
    }

    static PatternType PairPatternType_(
            const std::array<std::deque<NumberType>, SuitType::Count()>& same_number_poker_counts)
    {
        if (!same_number_poker_counts[4 - 1].empty()) {
            return PatternType::FOUR_OF_A_KIND;
        } else if (same_number_poker_counts[3 - 1].size() >= 2 ||
                   (!same_number_poker_counts[3 - 1].empty() && !same_number_poker_counts[2 - 1].empty())) {
            return PatternType::FULL_HOUSE;
        } else if (!same_number_poker_counts[3 - 1].empty()) {
            return PatternType::THREE_OF_A_KIND;
        } else if (same_number_poker_counts[2 - 1].size() >= 2) {
            return PatternType::TWO_PAIRS;
        } else if (!same_number_poker_counts[2 - 1].empty()) {
            return PatternType::ONE_PAIR;
        } else {
            return PatternType::HIGH_CARD;
        }
    }

    std::array<std::array<bool, SuitType::Count()>, NumberType::Count()> pokers_;
    mutable std::optional<Deck<k_type>> best_deck_;
    mutable bool need_refresh_;
};

} // namespace poker

} // namespace game_util

} // namespace lgtbot

#endif
