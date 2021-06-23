#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(PokerSuit)
ENUM_MEMBER(PokerSuit, CLUB)
ENUM_MEMBER(PokerSuit, DIANMOND)
ENUM_MEMBER(PokerSuit, HEART)
ENUM_MEMBER(PokerSuit, SPADE)
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
#include <regex>
#include <cassert>
#include <random>

namespace poker {

#define ENUM_FILE "../game_util/poker.h"
#include "../utility/extend_enum.h"

struct Poker
{
    auto operator<=>(const Poker&) const = default;
    const PokerNumber number_;
    const PokerSuit suit_;
};

void swap(Poker& _1, Poker& _2)
{
    std::swap(const_cast<PokerNumber&>(_1.number_), const_cast<PokerNumber&>(_2.number_));
    std::swap(const_cast<PokerSuit&>(_1.suit_), const_cast<PokerSuit&>(_2.suit_));
}

std::vector<Poker> ShuffledPokers(const std::string_view& sv = "")
{
    std::vector<Poker> pokers;
    for (const auto& number : Members<PokerNumber>()) {
        for (const auto& suit : Members<PokerSuit>()) {
            pokers.emplace_back(number, suit);
        }
    }
    if (sv.empty()) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(pokers.begin(), pokers.end(), g);
    } else {
        std::seed_seq seed(sv.begin(), sv.end());
        std::mt19937 g(seed);
        std::shuffle(pokers.begin(), pokers.end(), g);
    }
    return pokers;
}

template <typename Sender>
Sender& operator<<(Sender& sender, const Poker& poker)
{
    switch (poker.suit_) {
        case PokerSuit::SPADE: sender << "♠"; break;
        case PokerSuit::HEART: sender << "♡"; break;
        case PokerSuit::CLUB: sender << "♣"; break;
        case PokerSuit::DIANMOND: sender << "♢"; break;
    }
    switch (poker.number_) {
        case PokerNumber::_2: sender << "2"; break;
        case PokerNumber::_3: sender << "3"; break;
        case PokerNumber::_4: sender << "4"; break;
        case PokerNumber::_5: sender << "5"; break;
        case PokerNumber::_6: sender << "6"; break;
        case PokerNumber::_7: sender << "7"; break;
        case PokerNumber::_8: sender << "8"; break;
        case PokerNumber::_9: sender << "9"; break;
        case PokerNumber::_10: sender << "10"; break;
        case PokerNumber::_J: sender << "J"; break;
        case PokerNumber::_Q: sender << "Q"; break;
        case PokerNumber::_K: sender << "K"; break;
        case PokerNumber::_A: sender << "A"; break;
    }
    return sender;
}

template <typename String, typename Sender>
std::optional<PokerSuit> ParseSuit(const String& s, Sender&& sender)
{
    static const std::map<std::string, PokerSuit> str2suit = {
        {"黑桃", PokerSuit::SPADE},
        {"黑心", PokerSuit::SPADE},
        {"黑", PokerSuit::SPADE},
        {"♠", PokerSuit::SPADE},
        {"♤", PokerSuit::SPADE},
        {"红桃", PokerSuit::HEART},
        {"红心", PokerSuit::HEART},
        {"红", PokerSuit::HEART},
        {"♥", PokerSuit::HEART},
        {"♡", PokerSuit::HEART},
        {"梅花", PokerSuit::CLUB},
        {"梅", PokerSuit::CLUB},
        {"♣", PokerSuit::CLUB},
        {"♧", PokerSuit::CLUB},
        {"方板", PokerSuit::DIANMOND},
        {"方片", PokerSuit::DIANMOND},
        {"方块", PokerSuit::DIANMOND},
        {"♦", PokerSuit::DIANMOND},
        {"♢", PokerSuit::DIANMOND},
    };
    const auto it = str2suit.find(s);
    if (it == str2suit.end()) {
        sender << "非预期的花色\'" << s << "\'，期望为：";
        for (const auto& [str, _] : str2suit) {
            sender << "\'" << str << "\' ";
        }
        return std::nullopt;
    } else {
        return it->second;
    }
}

template <typename String, typename Sender>
std::optional<PokerNumber> ParseNumber(const String& s, Sender&& sender)
{
    static const std::map<std::string, PokerNumber> str2num = {
        {"2", PokerNumber::_2},
        {"3", PokerNumber::_3},
        {"4", PokerNumber::_4},
        {"5", PokerNumber::_5},
        {"6", PokerNumber::_6},
        {"7", PokerNumber::_7},
        {"8", PokerNumber::_8},
        {"9", PokerNumber::_9},
        {"10", PokerNumber::_10},
        {"J", PokerNumber::_J},
        {"j", PokerNumber::_J},
        {"11", PokerNumber::_J},
        {"Q", PokerNumber::_Q},
        {"q", PokerNumber::_Q},
        {"12", PokerNumber::_Q},
        {"K", PokerNumber::_K},
        {"k", PokerNumber::_K},
        {"13", PokerNumber::_K},
        {"A", PokerNumber::_A},
        {"a", PokerNumber::_A},
        {"1", PokerNumber::_A},
    };
    const auto it = str2num.find(s);
    if (it == str2num.end()) {
        sender << "非预期的点数\'" << s << "\'，期望为：";
        for (const auto& [str, _] : str2num) {
            sender << "\'" << str << "\' ";
        }
        return std::nullopt;
    } else {
        return it->second;
    }
}

template <typename String, typename Sender>
std::optional<Poker> Parse(const String& s, Sender&& sender)
{
    using namespace std::literals;
    std::smatch match_ret;
    PokerNumber number_;
    PokerSuit suit_;
    if (!std::regex_match(s, match_ret, std::regex("(.*)([AJQKajqk1-9]|10|11|12|13)"))) {
        sender << "非法的点数，需为2~10，或A、J、Q、K中一种";
        return std::nullopt;
    }
    assert(match_ret.size() == 3);
    if (const auto suit = ParseSuit(match_ret[1], sender); !suit.has_value()) {
        return std::nullopt;
    } else if (const auto number = ParseNumber(match_ret[2], sender); !suit.has_value()) {
        return std::nullopt;
    } else {
        return Poker(*number, *suit);
    }
}

struct Deck
{
    Deck& operator=(const Deck& deck)
    {
        this->~Deck();
        new(this) Deck(deck.type_, deck.pokers_);
        return *this;
    }
    auto operator<=>(const Deck&) const = default;
    int CompareIgnoreSuit(const Deck& d)
    {
        if (type_ < d.type_) {
            return -1;
        } else if (type_ > d.type_) {
            return 1;
        }
        static const auto cmp = [](const Poker& _1, const Poker& _2) { return _1.number_ < _2.number_; };
        if (std::lexicographical_compare(pokers_.begin(), pokers_.end(), d.pokers_.begin(), d.pokers_.end(), cmp)) {
            return -1;
        } else if (std::equal(pokers_.begin(), pokers_.end(), d.pokers_.begin(), d.pokers_.end(), cmp)) {
            return 0;
        } else {
            return 1;
        }
    }
    const PatternType type_;
    const std::array<Poker, 5> pokers_;
};

//void swap(Deck& _1, Deck& _2)
//{
//    std::swap(const_cast<PatternType&>(_1.type_), const_cast<PatternType&>(_2.type_));
//    std::swap(const_cast<std::array<Poker, 5>&>(_1.pokers_), const_cast<std::array<Poker, 5>&>(_2.pokers_));
//}

template <typename Sender>
Sender& operator<<(Sender& sender, const Deck& deck)
{
    sender << "[";
    switch (deck.type_) {
        case PatternType::HIGH_CARD: sender << "高牌"; break;
        case PatternType::ONE_PAIR: sender << "一对"; break;
        case PatternType::TWO_PAIRS: sender << "两对"; break;
        case PatternType::THREE_OF_A_KIND: sender << "三条"; break;
        case PatternType::STRAIGHT: sender << "顺子"; break;
        case PatternType::FLUSH: sender << "同花"; break;
        case PatternType::FULL_HOUSE: sender << "满堂红"; break;
        case PatternType::FOUR_OF_A_KIND: sender << "四条"; break;
        case PatternType::STRAIGHT_FLUSH:
            if (deck.pokers_.front().number_ == PokerNumber::_A) {
                sender << "皇家同花顺";
            } else {
                sender << "同花顺";
            }
            break;
    }
    sender << "]";
    for (const auto& poker : deck.pokers_) {
        sender << " " << poker;
    }
    return sender;
}

class Hand
{
   public:
    Hand() : pokers_{{false}}, need_refresh_(false) {}

    bool Add(const PokerNumber& number, const PokerSuit& suit)
    {
        const auto old_value = std::exchange(pokers_[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)], true);
        if (old_value == false) {
            need_refresh_ = true;
            return true;
        }
        return false;
    }

    bool Add(const Poker& poker) { return Add(poker.number_, poker.suit_); }

    bool Remove(const PokerNumber& number, const PokerSuit& suit)
    {
        const auto old_value = std::exchange(pokers_[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)], false);
        if (old_value == true) {
            need_refresh_ = true;
            return true;
        }
        return false;
    }

    bool Remove(const Poker& poker) { return Remove(poker.number_, poker.suit_); }

    bool Has(const PokerNumber& number, const PokerSuit& suit) const
    {
        return pokers_[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)];
    }

    bool Has(const Poker& poker) const { return Has(poker.number_, poker.suit_); }

    template <typename Sender>
    friend Sender& operator<<(Sender& sender, const Hand& hand)
    {
        for (const auto& number : Members<PokerNumber>()) {
            for (const auto& suit : Members<PokerSuit>()) {
                if (hand.Has(number, suit)) {
                    sender << Poker(number, suit) << " ";
                }
            }
        }
        const auto& best_deck = hand.BestDeck();
        if (best_deck.has_value()) {
            sender << "（" << *best_deck << "）";
        }
        return sender;
    }

    const std::optional<Deck>& BestDeck() const
    {
        if (!need_refresh_) {
            return best_deck_;
        }
        need_refresh_ = false;
        best_deck_ = std::nullopt;
        const auto update_deck = [this](const std::optional<Deck>& deck) {
            if (!deck.has_value()) {
                return;
            } else if (!best_deck_.has_value() || *best_deck_ < *deck) {
                best_deck_.emplace(*deck);
            }
        };

        for (auto suit_it = Members<PokerSuit>().rbegin(); suit_it != Members<PokerSuit>().rend(); ++suit_it) {
            update_deck(BestFlushPattern_<true>(*suit_it));
        }
        if (best_deck_.has_value()) {
            return best_deck_;
        }

        update_deck(BestPairPattern_());
        if (best_deck_.has_value() && best_deck_->type_ >= PatternType::FULL_HOUSE) {
            return best_deck_;
        }

        for (auto suit_it = Members<PokerSuit>().rbegin(); suit_it != Members<PokerSuit>().rend(); ++suit_it) {
            update_deck(BestFlushPattern_<false>(*suit_it));
        }
        if (best_deck_.has_value() && best_deck_->type_ >= PatternType::FLUSH) {
            return best_deck_;
        }

        update_deck(BestNonFlushNonPairPattern_());

        return best_deck_;
    }

   private:
    std::optional<Deck> BestNonFlushNonPairPattern_() const {
        const auto get_poker = [&pokers = pokers_](const PokerNumber number) -> std::optional<Poker> {
            for (auto suit_it = Members<PokerSuit>().rbegin(); suit_it != Members<PokerSuit>().rend(); ++suit_it) {
                if (pokers[static_cast<uint32_t>(number)][static_cast<uint32_t>(*suit_it)]) {
                    return Poker(number, *suit_it);
                }
            }
            return std::nullopt;
        };
        const auto deck = CollectNonPairDeck_<true>(get_poker);
        if (deck.has_value()) {
            return Deck(PatternType::STRAIGHT, *deck);
        } else {
            return std::nullopt;
        }
    }

    template <bool FIND_STRAIGHT>
    std::optional<Deck> BestFlushPattern_(const PokerSuit suit) const
    {
        const auto get_poker = [&suit, &pokers = pokers_](const PokerNumber number) -> std::optional<Poker> {
            if (pokers[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)]) {
                return Poker(number, suit);
            } else {
                return std::nullopt;
            }
        };
        const auto deck = CollectNonPairDeck_<FIND_STRAIGHT>(get_poker);
        if (deck.has_value()) {
            return Deck(FIND_STRAIGHT ? PatternType::STRAIGHT_FLUSH : PatternType::FLUSH, *deck);
        } else {
            return std::nullopt;
        }
    }

    template <bool FIND_STRAIGHT>
    static std::optional<std::array<Poker, 5>> CollectNonPairDeck_(const auto& get_poker)
    {
        std::vector<Poker> pokers;
        for (auto it = Members<PokerNumber>().rbegin(); it != Members<PokerNumber>().rend(); ++it) {
            const auto poker = get_poker(*it);
            if (poker.has_value()) {
                pokers.emplace_back(*poker);
                if (pokers.size() == 5) {
                    return std::array<Poker, 5>{pokers[0], pokers[1], pokers[2], pokers[3], pokers[4]};
                }
            } else if (FIND_STRAIGHT) {
                pokers.clear();
            }
        }
        if (const auto poker = get_poker(PokerNumber::_A); FIND_STRAIGHT && pokers.size() == 4 && poker.has_value()) {
            return std::array<Poker, 5>{pokers[0], pokers[1], pokers[2], pokers[3], *poker};
        } else {
            return std::nullopt;
        }
    }

    std::optional<Deck> BestPairPattern_() const
    {
        std::array<std::deque<PokerNumber>, Count<PokerSuit>() + 1> poker_numbers;
        for (const auto number : Members<PokerNumber>()) {
            const uint64_t count = std::count(pokers_[static_cast<uint32_t>(number)].begin(),
                                              pokers_[static_cast<uint32_t>(number)].end(), true);
            poker_numbers[count].emplace_front(number);
        }
        std::vector<Poker> pokers;
        // fill big pair poker first
        for (auto it = poker_numbers.rbegin(); it != poker_numbers.rend(); ++it) {
            // fill big number poker first
            for (auto number_it = it->rbegin(); number_it != it->rend(); ++number_it) {
                for (const auto suit : Members<PokerSuit>()) {
                    if (pokers_[static_cast<uint32_t>(*number_it)][static_cast<uint32_t>(suit)]) {
                        pokers.emplace_back(*number_it, suit);
                        if (pokers.size() == 5) {
                            return Deck(PairPatternType_(poker_numbers),
                                        std::array<Poker, 5>{pokers[0], pokers[1], pokers[2], pokers[3], pokers[4]});
                        }
                    }
                }
            }
        }
        return std::nullopt;
    }

    static PatternType PairPatternType_(
            const std::array<std::deque<PokerNumber>, Count<PokerSuit>() + 1>& poker_numbers)
    {
        if (!poker_numbers[4].empty()) {
            return PatternType::FOUR_OF_A_KIND;
        } else if (poker_numbers[3].size() >= 2 || (!poker_numbers[3].empty() && !poker_numbers[2].empty())) {
            return PatternType::FULL_HOUSE;
        } else if (!poker_numbers[3].empty()) {
            return PatternType::THREE_OF_A_KIND;
        } else if (poker_numbers[2].size() >= 2) {
            return PatternType::TWO_PAIRS;
        } else if (!poker_numbers[2].empty()) {
            return PatternType::ONE_PAIR;
        } else {
            return PatternType::HIGH_CARD;
        }
    }

    std::array<std::array<bool, Count<PokerSuit>()>, Count<PokerNumber>()> pokers_;
    mutable std::optional<Deck> best_deck_;
    mutable bool need_refresh_;
};

};  // namespace poker

#endif
