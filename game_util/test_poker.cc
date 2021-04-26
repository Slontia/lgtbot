#include "game_util/poker.h"
#include <gtest/gtest.h>
#include <gflags/gflags.h>

class TestPoker : public testing::Test {};

TEST_F(TestPoker, no_pattern_1)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_2, poker::PokerSuit::SPADE);
    ASSERT_FALSE(hand.BestDeck().has_value());
}

TEST_F(TestPoker, no_pattern_2)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_2, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_4, poker::PokerSuit::SPADE);
    ASSERT_FALSE(hand.BestDeck().has_value());
}

TEST_F(TestPoker, no_pattern_3)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::CLUB);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::DIANMOND);
    ASSERT_FALSE(hand.BestDeck().has_value());
}

TEST_F(TestPoker, straight_flush)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_2, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_4, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::SPADE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::STRAIGHT_FLUSH);
}

TEST_F(TestPoker, straight_flush_mix_1)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_2, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::CLUB);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_4, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::SPADE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::STRAIGHT_FLUSH);
}

TEST_F(TestPoker, four_of_a_kind)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::CLUB);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::SPADE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::FOUR_OF_A_KIND);
}

TEST_F(TestPoker, four_of_a_kind_mix_1)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::CLUB);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::HEART);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::FOUR_OF_A_KIND);
}

TEST_F(TestPoker, full_house)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::CLUB);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::SPADE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::FULL_HOUSE);
}

TEST_F(TestPoker, flush)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_2, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_4, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_6, poker::PokerSuit::SPADE);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::FLUSH);
}

TEST_F(TestPoker, flush_mix_1)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_2, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_4, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_6, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_6, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_6, poker::PokerSuit::CLUB);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::FLUSH);
}

TEST_F(TestPoker, straight)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_2, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_4, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::SPADE);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::STRAIGHT);
}

TEST_F(TestPoker, straight_mix_1)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_2, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_3, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_4, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::HEART);
    ASSERT_TRUE(hand.BestDeck().has_value());
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::STRAIGHT);
}

TEST_F(TestPoker, three_of_a_kind)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::CLUB);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_6, poker::PokerSuit::SPADE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::THREE_OF_A_KIND);
}

TEST_F(TestPoker, two_pairs)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::CLUB);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_6, poker::PokerSuit::SPADE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::TWO_PAIRS);
}

TEST_F(TestPoker, one_pair)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_5, poker::PokerSuit::CLUB);
    hand.AddPoker(poker::PokerNumber::_6, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_7, poker::PokerSuit::SPADE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::ONE_PAIR);
}

TEST_F(TestPoker, high_card)
{
    poker::Hand hand;
    hand.AddPoker(poker::PokerNumber::_A, poker::PokerSuit::SPADE);
    hand.AddPoker(poker::PokerNumber::_4, poker::PokerSuit::DIANMOND);
    hand.AddPoker(poker::PokerNumber::_6, poker::PokerSuit::CLUB);
    hand.AddPoker(poker::PokerNumber::_7, poker::PokerSuit::HEART);
    hand.AddPoker(poker::PokerNumber::_8, poker::PokerSuit::SPADE);
    const auto best_deck = hand.BestDeck();
    ASSERT_TRUE(best_deck.has_value());
    ASSERT_EQ(best_deck->type_, poker::PatternType::HIGH_CARD);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
