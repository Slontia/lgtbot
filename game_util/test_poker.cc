// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/poker.h"
#include <gtest/gtest.h>
#include <gflags/gflags.h>

class TestPoker : public testing::Test {};

TEST_F(TestPoker, no_pattern_1)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_2, poker::PokerSuit::GREEN);
    ASSERT_FALSE(hand.BestDeck().has_value());
}

TEST_F(TestPoker, no_pattern_2)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_1, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_2, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_4, poker::PokerSuit::GREEN);
    ASSERT_FALSE(hand.BestDeck().has_value());
}

TEST_F(TestPoker, no_pattern_3)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::BLUE);
    ASSERT_FALSE(hand.BestDeck().has_value());
}

TEST_F(TestPoker, straight_flush)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_1, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_2, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_4, poker::PokerSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::STRAIGHT_FLUSH) << "best_deck: " << best_deck->type_;
}

TEST_F(TestPoker, straight_flush_mix_1)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_1, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_2, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_4, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::STRAIGHT_FLUSH) << "best_deck: " << best_deck->type_;
}

TEST_F(TestPoker, four_of_a_kind)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FOUR_OF_A_KIND) << "best_deck: " << best_deck->type_;
}

TEST_F(TestPoker, four_of_a_kind_1)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::RED);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FOUR_OF_A_KIND) << "best_deck: " << best_deck->type_;
}

TEST_F(TestPoker, four_of_a_kind_2)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_8, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_8, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_8, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::BLUE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(*best_deck == (poker::Deck{poker::PatternType::FOUR_OF_A_KIND, std::array<poker::Poker, 5> {
                poker::Poker{poker::PokerNumber::_6, poker::PokerSuit::GREEN},
                poker::Poker{poker::PokerNumber::_6, poker::PokerSuit::RED},
                poker::Poker{poker::PokerNumber::_6, poker::PokerSuit::BLUE},
                poker::Poker{poker::PokerNumber::_6, poker::PokerSuit::PURPLE},
                poker::Poker{poker::PokerNumber::_0, poker::PokerSuit::GREEN},
            }})) << "Actual: " << *best_deck;
}

TEST_F(TestPoker, full_house)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FULL_HOUSE) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, flush)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_1, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_2, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_4, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::GREEN);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FLUSH) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, flush_mix_1)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_2, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_4, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::PURPLE);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FLUSH) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, straight)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_1, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_2, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_4, poker::PokerSuit::GREEN);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::STRAIGHT) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, straight_mix_1)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_1, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_2, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_3, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_4, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_4, poker::PokerSuit::RED);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::STRAIGHT) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, three_of_a_kind)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::THREE_OF_A_KIND) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, two_pairs)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::TWO_PAIRS) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, one_pair)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_5, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_7, poker::PokerSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::ONE_PAIR) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, high_card)
{
    poker::Hand hand;
    hand.Add(poker::PokerNumber::_0, poker::PokerSuit::GREEN);
    hand.Add(poker::PokerNumber::_4, poker::PokerSuit::BLUE);
    hand.Add(poker::PokerNumber::_6, poker::PokerSuit::PURPLE);
    hand.Add(poker::PokerNumber::_7, poker::PokerSuit::RED);
    hand.Add(poker::PokerNumber::_8, poker::PokerSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::HIGH_CARD) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, compare_flushes)
{
    poker::Hand hand_1;
    hand_1.Add(poker::PokerNumber::_0, poker::PokerSuit::RED);
    hand_1.Add(poker::PokerNumber::_7, poker::PokerSuit::RED);
    hand_1.Add(poker::PokerNumber::_3, poker::PokerSuit::RED);
    hand_1.Add(poker::PokerNumber::_2, poker::PokerSuit::RED);
    hand_1.Add(poker::PokerNumber::_1, poker::PokerSuit::RED);
    const auto best_deck_1 = hand_1.BestDeck();
    poker::Hand hand_2;
    hand_2.Add(poker::PokerNumber::_0, poker::PokerSuit::BLUE);
    hand_2.Add(poker::PokerNumber::_9, poker::PokerSuit::BLUE);
    hand_2.Add(poker::PokerNumber::_8, poker::PokerSuit::BLUE);
    hand_2.Add(poker::PokerNumber::_7, poker::PokerSuit::BLUE);
    hand_2.Add(poker::PokerNumber::_2, poker::PokerSuit::BLUE);
    const auto best_deck_2 = hand_2.BestDeck();
    ASSERT_TRUE(best_deck_1.has_value());
    ASSERT_TRUE(best_deck_2.has_value());
    ASSERT_TRUE(*best_deck_2 < *best_deck_1); // the greatest poker RED 0 is greater than BLUE 0, so deck 1 is greater
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
