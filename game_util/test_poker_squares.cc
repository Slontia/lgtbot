// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/poker_squares.h"
#include <gtest/gtest.h>
#include <gflags/gflags.h>

namespace poker = lgtbot::game_util::poker;
namespace ps = lgtbot::game_util::poker_squares;

#define ASSERT_RESULT_EQ(score, exp) \
do { \
    const auto old_score = ps::GetScore(square.GetStatistic()); \
    ASSERT_TRUE(exp); \
    ASSERT_EQ((score), ps::GetScore(square.GetStatistic()) - old_score); \
} while (false)

TEST(TestPokerSquares, simple_score)
{
    std::array<ps::GridType, ps::k_grid_num> grid_types;
    grid_types.fill(ps::GridType::empty);

    ps::PokerSquare square{"", grid_types};

    ASSERT_RESULT_EQ(0, square.Fill(10, ps::Card{poker::PokerNumber::_A, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(11, ps::Card{poker::PokerNumber::_10, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(12, ps::Card{poker::PokerNumber::_Q, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(13, ps::Card{poker::PokerNumber::_K, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(16 * 12 * 1, square.Fill(14, ps::Card{poker::PokerNumber::_J, poker::PokerSuit::HEARTS}));
}

TEST(TestPokerSquares, skip_next)
{
    std::array<ps::GridType, ps::k_grid_num> grid_types;
    grid_types.fill(ps::GridType::empty);
    grid_types[23] = ps::GridType::skip_next;

    ps::PokerSquare square{"", grid_types};

    ASSERT_TRUE(square.Fill(23, ps::Card{poker::PokerNumber::_A, poker::PokerSuit::SPADES}));
    ASSERT_TRUE(square.GetStatistic().skip_next_);
}

TEST(TestPokerSquares, red_bonus)
{
    std::array<ps::GridType, ps::k_grid_num> grid_types;
    grid_types.fill(ps::GridType::empty);
    grid_types[20] = ps::GridType::red_bonus;
    grid_types[21] = ps::GridType::red_bonus;
    grid_types[22] = ps::GridType::red_bonus;
    grid_types[23] = ps::GridType::red_bonus;

    ps::PokerSquare square{"", grid_types};

    ASSERT_RESULT_EQ(0, square.Fill(20, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(20, square.Fill(21, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(0, square.Fill(22, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::CLUBS}));
    ASSERT_RESULT_EQ(20, square.Fill(23, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::DIAMONDS}));
}

TEST(TestPokerSquares, black_bonus)
{
    std::array<ps::GridType, ps::k_grid_num> grid_types;
    grid_types.fill(ps::GridType::empty);
    grid_types[20] = ps::GridType::black_bonus;
    grid_types[21] = ps::GridType::black_bonus;
    grid_types[22] = ps::GridType::black_bonus;
    grid_types[23] = ps::GridType::black_bonus;

    ps::PokerSquare square{"", grid_types};

    ASSERT_RESULT_EQ(20, square.Fill(20, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(21, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(20, square.Fill(22, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::CLUBS}));
    ASSERT_RESULT_EQ(0, square.Fill(23, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::DIAMONDS}));
}

TEST(TestPokerSquares, set_card_on_double_score_success)
{
    std::array<ps::GridType, ps::k_grid_num> grid_types;
    grid_types.fill(ps::GridType::empty);
    grid_types[14] = ps::GridType::double_score;

    ps::PokerSquare square{"", grid_types};

    ASSERT_RESULT_EQ(0, square.Fill(10, ps::Card{poker::PokerNumber::_A, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(11, ps::Card{poker::PokerNumber::_10, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(12, ps::Card{poker::PokerNumber::_Q, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(13, ps::Card{poker::PokerNumber::_K, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(16 * 12 * 2, square.Fill(14, ps::Card{poker::PokerNumber::_J, poker::PokerSuit::HEARTS}));
}

TEST(TestPokerSquares, set_card_on_double_score_failure)
{
    std::array<ps::GridType, ps::k_grid_num> grid_types;
    grid_types.fill(ps::GridType::empty);
    grid_types[13] = ps::GridType::double_score;

    ps::PokerSquare square{"", grid_types};

    ASSERT_RESULT_EQ(0, square.Fill(10, ps::Card{poker::PokerNumber::_A, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(11, ps::Card{poker::PokerNumber::_10, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(12, ps::Card{poker::PokerNumber::_Q, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(13, ps::Card{poker::PokerNumber::_K, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(16 * 12 * 1, square.Fill(14, ps::Card{poker::PokerNumber::_J, poker::PokerSuit::HEARTS}));
}

TEST(TestPokerSquares, make_four_lines)
{
    std::array<ps::GridType, ps::k_grid_num> grid_types;
    grid_types.fill(ps::GridType::empty);
    grid_types[13] = ps::GridType::double_score;

    ps::PokerSquare square{"", grid_types};

    ASSERT_RESULT_EQ(0, square.Fill(10, ps::Card{poker::PokerNumber::_A, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(11, ps::Card{poker::PokerNumber::_10, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(13, ps::Card{poker::PokerNumber::_Q, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(14, ps::Card{poker::PokerNumber::_K, poker::PokerSuit::HEARTS}));

    ASSERT_RESULT_EQ(0, square.Fill(2, ps::Card{poker::PokerNumber::_A, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(0, square.Fill(7, ps::Card{poker::PokerNumber::_10, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(0, square.Fill(17, ps::Card{poker::PokerNumber::_Q, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(0, square.Fill(22, ps::Card{poker::PokerNumber::_K, poker::PokerSuit::CLUBS}));

    ASSERT_RESULT_EQ(0, square.Fill(0, ps::Card{poker::PokerNumber::_A, poker::PokerSuit::CLUBS}));
    ASSERT_RESULT_EQ(0, square.Fill(6, ps::Card{poker::PokerNumber::_10, poker::PokerSuit::CLUBS}));
    ASSERT_RESULT_EQ(0, square.Fill(18, ps::Card{poker::PokerNumber::_Q, poker::PokerSuit::CLUBS}));
    ASSERT_RESULT_EQ(0, square.Fill(24, ps::Card{poker::PokerNumber::_K, poker::PokerSuit::DIAMONDS}));

    ASSERT_RESULT_EQ(0, square.Fill(4, ps::Card{poker::PokerNumber::_A, poker::PokerSuit::DIAMONDS}));
    ASSERT_RESULT_EQ(0, square.Fill(8, ps::Card{poker::PokerNumber::_10, poker::PokerSuit::DIAMONDS}));
    ASSERT_RESULT_EQ(0, square.Fill(16, ps::Card{poker::PokerNumber::_Q, poker::PokerSuit::DIAMONDS}));
    ASSERT_RESULT_EQ(0, square.Fill(20, ps::Card{poker::PokerNumber::_K, poker::PokerSuit::SPADES}));

    ASSERT_RESULT_EQ(16 * 12 * 1 * 6, square.Fill(12, ps::Card{poker::PokerNumber::_J, poker::PokerSuit::HEARTS}));
}

TEST(TestPokerSquares, pattern_type_num_bonus)
{
    std::array<ps::GridType, ps::k_grid_num> grid_types;
    grid_types.fill(ps::GridType::empty);

    ps::PokerSquare square{"/root/lgtbot/src/games/poker_squares/resource/", grid_types};

    // s2 s3 s4 s5 s6
    // h2 h3 h4 h5 sQ
    // c7 c3 c4 c5 c8
    // d7 dX d8 d5 sK
    // c9 dQ s8 sJ h6

    ASSERT_RESULT_EQ(0, square.Fill(0, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(1, ps::Card{poker::PokerNumber::_3, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(2, ps::Card{poker::PokerNumber::_4, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(0, square.Fill(3, ps::Card{poker::PokerNumber::_5, poker::PokerSuit::SPADES}));
    ASSERT_RESULT_EQ(5 * 30 * 1, square.Fill(4, ps::Card{poker::PokerNumber::_6, poker::PokerSuit::SPADES})); // STRAIGHT_FLUSH

    ASSERT_RESULT_EQ(0, square.Fill(5, ps::Card{poker::PokerNumber::_2, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(0, square.Fill(6, ps::Card{poker::PokerNumber::_3, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(0, square.Fill(7, ps::Card{poker::PokerNumber::_4, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(0, square.Fill(8, ps::Card{poker::PokerNumber::_5, poker::PokerSuit::HEARTS}));
    ASSERT_RESULT_EQ(0, square.Fill(9, ps::Card{poker::PokerNumber::_Q, poker::PokerSuit::SPADES}));

    ASSERT_RESULT_EQ(0, square.Fill(10, ps::Card{poker::PokerNumber::_7, poker::PokerSuit::CLUBS}));
    ASSERT_RESULT_EQ(0, square.Fill(11, ps::Card{poker::PokerNumber::_3, poker::PokerSuit::CLUBS}));
    ASSERT_RESULT_EQ(0, square.Fill(12, ps::Card{poker::PokerNumber::_4, poker::PokerSuit::CLUBS}));
    ASSERT_RESULT_EQ(0, square.Fill(13, ps::Card{poker::PokerNumber::_5, poker::PokerSuit::CLUBS}));
    ASSERT_RESULT_EQ(7 * 5 * 1, square.Fill(14, ps::Card{poker::PokerNumber::_8, poker::PokerSuit::CLUBS})); // FLUSH

    ASSERT_RESULT_EQ(0, square.Fill(15, ps::Card{poker::PokerNumber::_7, poker::PokerSuit::DIAMONDS}));
    ASSERT_RESULT_EQ(0, square.Fill(16, ps::Card{poker::PokerNumber::_10, poker::PokerSuit::DIAMONDS}));
    ASSERT_RESULT_EQ(0, square.Fill(17, ps::Card{poker::PokerNumber::_8, poker::PokerSuit::DIAMONDS}));
    ASSERT_RESULT_EQ(0, square.Fill(18, ps::Card{poker::PokerNumber::_5, poker::PokerSuit::DIAMONDS}));
    ASSERT_RESULT_EQ(0, square.Fill(19, ps::Card{poker::PokerNumber::_K, poker::PokerSuit::SPADES}));

    ASSERT_RESULT_EQ(8 * 3 * 1, square.Fill(20, ps::Card{poker::PokerNumber::_9, poker::PokerSuit::CLUBS})); // TWO_PAIRS
    ASSERT_RESULT_EQ(8 * 6 * 1, square.Fill(21, ps::Card{poker::PokerNumber::_Q, poker::PokerSuit::DIAMONDS})); // THREE_OF_A_KIND
    ASSERT_RESULT_EQ(7 * 10 * 1 + 100, square.Fill(22, ps::Card{poker::PokerNumber::_8, poker::PokerSuit::SPADES})); // FULL_HOUSE
    ASSERT_RESULT_EQ(7 * 16 * 1 + 100, square.Fill(23, ps::Card{poker::PokerNumber::_J, poker::PokerSuit::SPADES})); // FOUR_OF_A_KIND
    ASSERT_RESULT_EQ(10 * 1 * 1 + 5 * 12 * 2 + 200 + 400, square.Fill(24, ps::Card{poker::PokerNumber::_6, poker::PokerSuit::HEARTS})); // ONE_PAIR + STRAIGHT
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
