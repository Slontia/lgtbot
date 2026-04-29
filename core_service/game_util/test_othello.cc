// Copyright (c) 2023-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/othello.h"

#include <iostream>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace lgtbot::game_util::othello;

class TestOthello : public testing::Test {};

TEST_F(TestOthello, beginning_placable_positions)
{
    Board board("");
    auto placable_positions = board.PlacablePositions(ChessType::BLACK);
    std::ranges::sort(placable_positions);
    ASSERT_EQ((std::vector<Coor>{Coor{2, 4}, Coor{3, 5}, Coor{4, 2}, Coor{5, 3}}), placable_positions);
}

TEST_F(TestOthello, reverse_chesses)
{
    Board board("");
    EXPECT_TRUE(board.Place(Coor{3, 5}, ChessType::BLACK));
    Statistic expected_statistic;
    expected_statistic[static_cast<uint8_t>(ChessType::BLACK)] = 4;
    expected_statistic[static_cast<uint8_t>(ChessType::WHITE)] = 1;
    expected_statistic[static_cast<uint8_t>(ChessType::CRASH)] = 0;
    expected_statistic[static_cast<uint8_t>(ChessType::NONE)] = 59;
    ASSERT_EQ(expected_statistic, board.Settlement());
}

TEST_F(TestOthello, both_reverse_chesses)
{
    Board board("");
    EXPECT_TRUE(board.Place(Coor{3, 5}, ChessType::BLACK));
    EXPECT_TRUE(board.Place(Coor{2, 3}, ChessType::WHITE));
    Statistic expected_statistic;
    expected_statistic[static_cast<uint8_t>(ChessType::BLACK)] = 3;
    expected_statistic[static_cast<uint8_t>(ChessType::WHITE)] = 3;
    expected_statistic[static_cast<uint8_t>(ChessType::CRASH)] = 0;
    expected_statistic[static_cast<uint8_t>(ChessType::NONE)] = 58;
    ASSERT_EQ(expected_statistic, board.Settlement());
}

TEST_F(TestOthello, invalid_position)
{
    Board board("");
    EXPECT_FALSE(board.Place(Coor{2, 2}, ChessType::BLACK));
    EXPECT_FALSE(board.Place(Coor{2, 3}, ChessType::BLACK));
    EXPECT_FALSE(board.Place(Coor{0, 0}, ChessType::BLACK));
}

TEST_F(TestOthello, cannot_reverse_those_not_between_own_chesses)
{
    Board board("", std::array{
        std::pair{Coor{0, 0}, ChessType::BLACK},
        std::pair{Coor{0, 2}, ChessType::BLACK},
        std::pair{Coor{0, 3}, ChessType::WHITE},
    });
    EXPECT_TRUE(board.Place(Coor{0, 1}, ChessType::WHITE));
    Statistic expected_statistic;
    expected_statistic[static_cast<uint8_t>(ChessType::BLACK)] = 1;
    expected_statistic[static_cast<uint8_t>(ChessType::WHITE)] = 3;
    expected_statistic[static_cast<uint8_t>(ChessType::CRASH)] = 0;
    expected_statistic[static_cast<uint8_t>(ChessType::NONE)] = 60;
    ASSERT_EQ(expected_statistic, board.Settlement());
}

TEST_F(TestOthello, reverse_crashes_chess)
{
    Board board("", std::array{
        std::pair{Coor{1, 0}, ChessType::BLACK},
        std::pair{Coor{1, 1}, ChessType::WHITE},
        std::pair{Coor{1, 3}, ChessType::BLACK},
        std::pair{Coor{1, 4}, ChessType::WHITE},
        std::pair{Coor{0, 2}, ChessType::WHITE},
    });
    Statistic expected_statistic;

    EXPECT_TRUE(board.Place(Coor{1, 2}, ChessType::WHITE));
    EXPECT_TRUE(board.Place(Coor{1, 2}, ChessType::BLACK));
    expected_statistic[static_cast<uint8_t>(ChessType::BLACK)] = 2;
    expected_statistic[static_cast<uint8_t>(ChessType::WHITE)] = 3;
    expected_statistic[static_cast<uint8_t>(ChessType::CRASH)] = 1;
    expected_statistic[static_cast<uint8_t>(ChessType::NONE)] = 58;
    ASSERT_EQ(expected_statistic, board.Settlement()); // crash at <1, 2>

    EXPECT_TRUE(board.Place(Coor{2, 2}, ChessType::WHITE));
    expected_statistic[static_cast<uint8_t>(ChessType::BLACK)] = 2;
    expected_statistic[static_cast<uint8_t>(ChessType::WHITE)] = 5;
    expected_statistic[static_cast<uint8_t>(ChessType::CRASH)] = 0;
    expected_statistic[static_cast<uint8_t>(ChessType::NONE)] = 57;
    ASSERT_EQ(expected_statistic, board.Settlement()); // reverse crashed chess to white
}

TEST_F(TestOthello, both_reverse_crashed_chess)
{
    Board board("", std::array{
        std::pair{Coor{2, 0}, ChessType::BLACK},
        std::pair{Coor{2, 1}, ChessType::WHITE},
        std::pair{Coor{2, 3}, ChessType::BLACK},
        std::pair{Coor{2, 4}, ChessType::WHITE},
        std::pair{Coor{1, 2}, ChessType::WHITE},
        std::pair{Coor{3, 2}, ChessType::BLACK},
    });
    Statistic expected_statistic;

    EXPECT_TRUE(board.Place(Coor{2, 2}, ChessType::WHITE));
    EXPECT_TRUE(board.Place(Coor{2, 2}, ChessType::BLACK));
    expected_statistic[static_cast<uint8_t>(ChessType::BLACK)] = 3;
    expected_statistic[static_cast<uint8_t>(ChessType::WHITE)] = 3;
    expected_statistic[static_cast<uint8_t>(ChessType::CRASH)] = 1;
    expected_statistic[static_cast<uint8_t>(ChessType::NONE)] = 57;
    ASSERT_EQ(expected_statistic, board.Settlement()); // crash at <2, 2>

    EXPECT_TRUE(board.Place(Coor{0, 2}, ChessType::BLACK));
    EXPECT_TRUE(board.Place(Coor{4, 2}, ChessType::WHITE));
    expected_statistic[static_cast<uint8_t>(ChessType::BLACK)] = 4;
    expected_statistic[static_cast<uint8_t>(ChessType::WHITE)] = 4;
    expected_statistic[static_cast<uint8_t>(ChessType::CRASH)] = 1;
    expected_statistic[static_cast<uint8_t>(ChessType::NONE)] = 55;
    ASSERT_EQ(expected_statistic, board.Settlement());
}

