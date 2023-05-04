// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/poker.h"
#include <gtest/gtest.h>
#include <gflags/gflags.h>

namespace poker = lgtbot::game_util::poker;

class TestPoker : public testing::Test {};

TEST_F(TestPoker, no_pattern_1)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_2, poker::BokaaSuit::GREEN);
    ASSERT_FALSE(hand.BestDeck().has_value());
}

TEST_F(TestPoker, no_pattern_2)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_1, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_2, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_4, poker::BokaaSuit::GREEN);
    ASSERT_FALSE(hand.BestDeck().has_value());
}

TEST_F(TestPoker, no_pattern_3)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::BLUE);
    ASSERT_FALSE(hand.BestDeck().has_value());
}

TEST_F(TestPoker, straight_flush)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_1, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_2, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_4, poker::BokaaSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::STRAIGHT_FLUSH) << "best_deck: " << best_deck->type_;
}

TEST_F(TestPoker, straight_flush_mix_1)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_1, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_2, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_4, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::STRAIGHT_FLUSH) << "best_deck: " << best_deck->type_;
}

TEST_F(TestPoker, four_of_a_kind)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FOUR_OF_A_KIND) << "best_deck: " << best_deck->type_;
}

TEST_F(TestPoker, four_of_a_kind_1)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::RED);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FOUR_OF_A_KIND) << "best_deck: " << best_deck->type_;
}

TEST_F(TestPoker, four_of_a_kind_2)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_8, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_8, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_8, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::BLUE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(*best_deck == (poker::Deck<poker::CardType::BOKAA>{poker::PatternType::FOUR_OF_A_KIND, std::array<poker::Card<poker::CardType::BOKAA>, 5> {
                poker::Card<poker::CardType::BOKAA>{poker::BokaaNumber::_6, poker::BokaaSuit::GREEN},
                poker::Card<poker::CardType::BOKAA>{poker::BokaaNumber::_6, poker::BokaaSuit::RED},
                poker::Card<poker::CardType::BOKAA>{poker::BokaaNumber::_6, poker::BokaaSuit::BLUE},
                poker::Card<poker::CardType::BOKAA>{poker::BokaaNumber::_6, poker::BokaaSuit::PURPLE},
                poker::Card<poker::CardType::BOKAA>{poker::BokaaNumber::_X, poker::BokaaSuit::GREEN},
            }})) << "Actual: " << *best_deck;
}

TEST_F(TestPoker, full_house)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FULL_HOUSE) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, flush)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_1, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_2, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_4, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::GREEN);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FLUSH) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, flush_mix_1)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_2, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_4, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::PURPLE);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::FLUSH) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, straight)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_1, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_2, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_4, poker::BokaaSuit::GREEN);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::STRAIGHT) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, straight_mix_1)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_1, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_2, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_3, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_4, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_4, poker::BokaaSuit::RED);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::STRAIGHT) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, three_of_a_kind)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::THREE_OF_A_KIND) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, two_pairs)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::TWO_PAIRS) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, one_pair)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_5, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_7, poker::BokaaSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::ONE_PAIR) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, high_card)
{
    poker::Hand<poker::CardType::BOKAA> hand;
    hand.Add(poker::BokaaNumber::_X, poker::BokaaSuit::GREEN);
    hand.Add(poker::BokaaNumber::_4, poker::BokaaSuit::BLUE);
    hand.Add(poker::BokaaNumber::_6, poker::BokaaSuit::PURPLE);
    hand.Add(poker::BokaaNumber::_7, poker::BokaaSuit::RED);
    hand.Add(poker::BokaaNumber::_8, poker::BokaaSuit::GREEN);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_TRUE(best_deck->type_ == poker::PatternType::HIGH_CARD) << "best_deck: " << best_deck->type_;;
}

TEST_F(TestPoker, compare_flushes)
{
    poker::Hand<poker::CardType::BOKAA> hand_1;
    hand_1.Add(poker::BokaaNumber::_X, poker::BokaaSuit::RED);
    hand_1.Add(poker::BokaaNumber::_7, poker::BokaaSuit::RED);
    hand_1.Add(poker::BokaaNumber::_3, poker::BokaaSuit::RED);
    hand_1.Add(poker::BokaaNumber::_2, poker::BokaaSuit::RED);
    hand_1.Add(poker::BokaaNumber::_1, poker::BokaaSuit::RED);
    const auto best_deck_1 = hand_1.BestDeck();
    poker::Hand<poker::CardType::BOKAA> hand_2;
    hand_2.Add(poker::BokaaNumber::_X, poker::BokaaSuit::BLUE);
    hand_2.Add(poker::BokaaNumber::_9, poker::BokaaSuit::BLUE);
    hand_2.Add(poker::BokaaNumber::_8, poker::BokaaSuit::BLUE);
    hand_2.Add(poker::BokaaNumber::_7, poker::BokaaSuit::BLUE);
    hand_2.Add(poker::BokaaNumber::_2, poker::BokaaSuit::BLUE);
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
