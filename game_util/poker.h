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
#include <span>
#include <map>
#include <set>
#include <regex>
#include <cassert>
#include <random>
#include <sstream>
#include <utility> // g++12 has a bug which will cause 'exchange' is not a member of 'std'
#include <algorithm>
#include <bitset>
#include <ranges>

#include "utility/html.h"

namespace lgtbot {

namespace game_util {

namespace poker {

#define ENUM_FILE "../game_util/poker.h"
#include "../utility/extend_enum.h"

constexpr inline const char* GetPatternTypeName(const PatternType type)
{
    constexpr const char* k_pattern_type_names[] = {
        [PatternType{PatternType::HIGH_CARD}.ToUInt()] = "高牌",
        [PatternType{PatternType::ONE_PAIR}.ToUInt()] = "一对",
        [PatternType{PatternType::TWO_PAIRS}.ToUInt()] = "两对",
        [PatternType{PatternType::THREE_OF_A_KIND}.ToUInt()] = "三条",
        [PatternType{PatternType::STRAIGHT}.ToUInt()] = "顺子",
        [PatternType{PatternType::FLUSH}.ToUInt()] = "同花",
        [PatternType{PatternType::FULL_HOUSE}.ToUInt()] = "满堂红",
        [PatternType{PatternType::FOUR_OF_A_KIND}.ToUInt()] = "四条",
        [PatternType{PatternType::STRAIGHT_FLUSH}.ToUInt()] = "同花顺",
    };
    static_assert(sizeof(k_pattern_type_names) / sizeof(k_pattern_type_names[0]) == PatternType::Count());
    return k_pattern_type_names[type.ToUInt()];
}

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
        [BokaaSuit(BokaaSuit::PURPLE).ToUInt()] = "○",
        [BokaaSuit(BokaaSuit::BLUE).ToUInt()]   = "△",
        [BokaaSuit(BokaaSuit::RED).ToUInt()]    = "□",
        [BokaaSuit(BokaaSuit::GREEN).ToUInt()]  = "☆",
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
constexpr uint32_t k_card_num = Types<k_type>::NumberType::Count() * Types<k_type>::SuitType::Count();

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
std::array<Card<k_type>, k_card_num<k_type>> UnshuffledPokers()
{
    std::array<Card<k_type>, k_card_num<k_type>> cards;
    auto it = cards.begin();
    for (const auto& number : Types<k_type>::NumberType::Members()) {
        for (const auto& suit : Types<k_type>::SuitType::Members()) {
            *(it++) = Card<k_type>{number, suit};
        }
    }
    return cards;
}

template <CardType k_type>
std::array<Card<k_type>, k_card_num<k_type>> ShuffledPokers(const std::string_view& sv = "")
{
    auto cards = UnshuffledPokers<k_type>();
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

template <CardType k_type>
std::string Card<k_type>::ToString() const
{
    return std::string(Types<k_type>::k_suit_str[suit_.ToUInt()]) + (number_.ToString() + 1); // skip the '_' character
}

template <typename Sender, CardType k_type>
Sender& operator<<(Sender& sender, const Card<k_type>& poker)
{
    sender << poker.ToString();
    return sender;
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
    using Pokers = std::array<Card<k_type>, 5>;

    Deck& operator=(const Deck& deck)
    {
        this->~Deck();
        new(this) Deck(deck.type_, deck.pokers_);
        return *this;
    }

    auto operator<=>(const Deck& d) const { return Compare(d); }

    auto operator==(const Deck& d) const { return Compare(d) == std::strong_ordering::equal; }

    std::strong_ordering Compare(const Deck& d, const bool ignore_suit = false) const
    {
        if (type_ < d.type_) {
            return std::strong_ordering::less;
        } else if (type_ > d.type_) {
            return std::strong_ordering::greater;
        }
        static const auto get_number = std::views::transform([](const poker::Card<k_type>& card) { return card.number_; });
        static const auto get_suit = std::views::transform([](const poker::Card<k_type>& card) { return card.suit_; });
        const auto ret = ComparePokers_(pokers_ | get_number, d.pokers_ | get_number);
        if (ret != std::strong_ordering::equal || ignore_suit) {
            return ret;
        }
        return ComparePokers_(pokers_ | get_suit, d.pokers_ | get_suit);
    }

    std::strong_ordering CompareIgnoreSuit(const Deck& d) const { return Compare(d, true); }

    const char* TypeName() const;

    std::string ToString() const
    {
        std::string s = std::string("[") + TypeName() + "]";
        for (const auto& poker : pokers_) {
            s += " " + poker.ToString();
        }
        return s;
    }

    const PatternType type_;
    const Pokers pokers_;

  private:
    template <typename Range>
    static std::strong_ordering ComparePokers_(const Range& _1, const Range& _2)
    {
        if (std::ranges::lexicographical_compare(_1, _2)) {
            return std::strong_ordering::less;
        } else if (std::ranges::equal(_1, _2)) {
            return std::strong_ordering::equal;
        } else {
            return std::strong_ordering::greater;
        }
    }
};

template <CardType k_type>
struct OptionalDeck : public std::optional<Deck<k_type>>
{
    using std::optional<Deck<k_type>>::optional;

    OptionalDeck(const OptionalDeck&) = default;

    OptionalDeck(OptionalDeck&&) = default;

    OptionalDeck& operator=(const OptionalDeck&) = default;

    OptionalDeck& operator=(OptionalDeck&&) = default;

    auto operator<=>(const OptionalDeck& o) const
    {
        return Compare_(o, [](const Deck<k_type>& _1, const Deck<k_type>& _2) { return _1 <=> _2; });
    }

    std::strong_ordering Compare(const OptionalDeck& o, const bool ignore_suit = false) const
    {
        return Compare_(o, [ignore_suit](const Deck<k_type>& _1, const Deck<k_type>& _2) { return _1.Compare(_2, ignore_suit); });
    }

    std::strong_ordering CompareIgnoreSuit(const OptionalDeck& o) const
    {
        return Compare(o, true);
    }

  private:
    template <typename Compare>
    std::strong_ordering Compare_(const OptionalDeck& o, const Compare& cmp) const
    {
        return  this->has_value() &&  o.has_value() ? cmp(**this, *o)         :
                this->has_value() && !o.has_value() ? std::strong_ordering::greater :
               !this->has_value() &&  o.has_value() ? std::strong_ordering::less    : std::strong_ordering::equal;
    }
};

template <CardType k_type>
bool IsRoralStraightFlush(const Deck<k_type>& deck)
{
    return deck.type_ == PatternType::STRAIGHT_FLUSH && deck.pokers_.front().number_ == Types<k_type>::k_max_number_;
}

template <CardType k_type>
const char* Deck<k_type>::TypeName() const
{
    if (IsRoralStraightFlush(*this)) {
        return "皇家同花顺";
    }
    return GetPatternTypeName(type_);
}

template <typename Sender, CardType k_type>
Sender& operator<<(Sender& sender, const Deck<k_type>& deck)
{
    sender << deck.ToString();
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
        sender << hand.ToString();
        const auto& best_deck = hand.BestDeck();
        if (best_deck.has_value()) {
            sender << "（" << *best_deck << "）";
        }
        return sender;
    }

    std::string ToHtml() const { return ToString_<true>(); }

    std::string ToString() const { return ToString_<false>(); }

    const OptionalDeck<k_type>& BestDeck() const
    {
        if (!need_refresh_) {
            return best_deck_;
        }
        need_refresh_ = false;
        best_deck_ = std::nullopt;
        const auto update_deck = [this](const OptionalDeck<k_type>& deck) {
            if (deck > best_deck_) {
                best_deck_ = deck;
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
    template <bool TO_HTML>
    std::string ToString_() const
    {
        std::string s;
        for (const auto& number : NumberType::Members()) {
            for (const auto& suit : SuitType::Members()) {
                if (!Has(number, suit)) {
                    continue;
                }
                if constexpr (TO_HTML) {
                    s += Card<k_type>(number, suit).ToHtml() + " ";
                } else {
                    s += Card<k_type>(number, suit).ToString() + " ";
                }
            }
        }
        return s;
    }

    OptionalDeck<k_type> BestNonFlushNonPairPattern_() const {
        const auto get_poker = [&pokers = pokers_](const NumberType number) -> std::optional<Card<k_type>> {
            for (auto suit_it = SuitType::Members().rbegin(); suit_it != SuitType::Members().rend(); ++suit_it) {
                if (pokers[static_cast<uint32_t>(number)][static_cast<uint32_t>(*suit_it)]) {
                    return Card<k_type>(number, *suit_it);
                }
            }
            return std::nullopt;
        };
        const auto cards = CollectNonPairDeck_<true>(get_poker);
        if (cards.has_value()) {
            return Deck<k_type>(PatternType::STRAIGHT, *cards);
        } else {
            return std::nullopt;
        }
    }

    template <bool FIND_STRAIGHT>
    OptionalDeck<k_type> BestFlushPattern_(const SuitType suit) const
    {
        const auto get_poker = [&suit, &pokers = pokers_](const NumberType number) -> std::optional<Card<k_type>> {
            if (pokers[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)]) {
                return Card<k_type>(number, suit);
            } else {
                return std::nullopt;
            }
        };
        const auto cards = CollectNonPairDeck_<FIND_STRAIGHT>(get_poker);
        if (cards.has_value()) {
            return Deck<k_type>(PatternType::Condition(FIND_STRAIGHT, PatternType::STRAIGHT_FLUSH, PatternType::FLUSH), *cards);
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

    OptionalDeck<k_type> BestPairPattern_() const
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
    mutable OptionalDeck<k_type> best_deck_;
    mutable bool need_refresh_;
};

template <CardType k_type>
void UpdatePossibility(const std::vector<Hand<k_type>>& hands, const bool ignore_suit, std::vector<double>& points) {
    assert(hands.size() == points.size());
    assert(!hands.empty());
    const auto decks = hands | std::views::transform([](const Hand<k_type>& hand) { return hand.BestDeck(); });
    const auto best_deck = std::ranges::max_element(decks,
            [ignore_suit](const auto& _1, const auto& _2) { return _1.Compare(_2, ignore_suit) < 0; });
    const auto best_num = std::ranges::count_if(decks.begin(), decks.end(),
            [ignore_suit, &best_deck](const auto& deck) { return deck.Compare(*best_deck, ignore_suit) == 0; });
    assert(best_num > 0);
    // share 1 point for each winner deck
    for (uint32_t i = 0; i < decks.size(); ++i) {
        if (decks[i].Compare(*best_deck, ignore_suit) == 0) {
            points[i] += double(1) / double(best_num);
        }
    }
}

template <CardType k_type>
void UpdatePossibility(std::vector<Hand<k_type>>& hands, const std::span<Card<k_type>>& possible_hid_cards,
        const uint32_t hid_card_num, const bool ignore_suit, std::vector<double>& points)
{
    assert(possible_hid_cards.size() >= hid_card_num);
    assert(!hands.empty());
    if (hid_card_num == 0) {
        UpdatePossibility(hands, ignore_suit, points);
        return;
    }
    const auto limit = possible_hid_cards.size() - hid_card_num + 1;
    for (size_t i = 0; i < limit; ++i) {
        const auto card = possible_hid_cards[i];
        for (auto& hand : hands) {
            assert(!hand.Has(card));
            hand.Add(card);
        }
        UpdatePossibility(hands, possible_hid_cards.subspan(i + 1), hid_card_num - 1, ignore_suit, points);
        for (auto& hand : hands) {
            hand.Remove(card);
        }
    }
}

template <CardType k_type>
std::vector<double> WinPossibility(const std::vector<Hand<k_type>>& hands, const std::span<Card<k_type>>& possible_hid_cards,
        const uint32_t hid_card_num, const bool ignore_suit = false)
{
    if (hands.empty()) {
        return {};
    }
    std::vector<double> points(hands.size(), 0);
    auto hands_cpy = hands;
    UpdatePossibility(hands_cpy, possible_hid_cards, hid_card_num, ignore_suit, points);
    double total_point = 1;
    for (uint32_t i = 0; i < hid_card_num; ++i) {
        total_point *= possible_hid_cards.size() - i;
    }
    total_point /= std::tgamma(hid_card_num + 1);
    // normalize each point to [0~1] as the real possibiliy
    for (double& point : points) {
        point /= total_point;
    }
    return points;
}

} // namespace poker

} // namespace game_util

} // namespace lgtbot

#endif
