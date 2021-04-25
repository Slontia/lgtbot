#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(PokerSuit)
ENUM_MEMBER(PokerSuit, DIANMOND)
ENUM_MEMBER(PokerSuit, CLUB)
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

namespace poker {

#define ENUM_FILE "../game_util/poker.h"
#include "../utility/extend_enum.h"

struct Poker {
    auto operator<=>(const Poker&) const = default;
    const PokerNumber number_;
    const PokerSuit suit_;
};

struct Deck {
    auto operator<=>(const Deck&) const = default;
    const PatternType type_;
    const std::array<Poker, 5> pokers_;
};

class Hand
{
   public:
    Hand() : pokers_{{false}} {}

    void AddPoker(const PokerNumber& number, const PokerSuit& suit)
    {
        std::scoped_lock l(m_);
        pokers_[static_cast<uint32_t>(number)][static_cast<uint32_t>(suit)] = true;
    }

    std::optional<Deck> BestDeck() const
    {
        std::scoped_lock l(m_);
        std::optional<Deck> best_deck;
        const auto update_deck = [&best_deck](const std::optional<Deck>& deck) {
            if (!deck.has_value()) {
                return;
            } else if (!best_deck.has_value() || *best_deck < *deck) {
                best_deck.emplace(*deck);
            }
        };

        for (auto suit_it = Members<PokerSuit>().rbegin(); suit_it != Members<PokerSuit>().rend(); ++suit_it) {
            update_deck(BestFlushPattern_<true>(*suit_it));
        }
        if (best_deck.has_value()) {
            return best_deck;
        }

        update_deck(BestPairPattern_());
        if (best_deck.has_value() && best_deck->type_ >= PatternType::FULL_HOUSE) {
            return best_deck;
        }

        for (auto suit_it = Members<PokerSuit>().rbegin(); suit_it != Members<PokerSuit>().rend(); ++suit_it) {
            update_deck(BestFlushPattern_<false>(*suit_it));
        }
        if (best_deck.has_value() && best_deck->type_ >= PatternType::FLUSH) {
            return best_deck;
        }

        update_deck(BestNonFlushNonPairPattern_());

        return best_deck;
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

    mutable std::mutex m_;
    std::array<std::array<bool, Count<PokerSuit>()>, Count<PokerNumber>()> pokers_;
};

};  // namespace poker

#endif
