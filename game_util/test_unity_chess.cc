// Copyright (c) 2023-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/unity_chess.h"

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace lgtbot::game_util::unity_chess;

const char* const k_image_path = "/root/lgtbot/src/games/sync_unity_chess/resource/";

#define SET_AND_SETTLEMENT(coordinate, player_id) \
do { \
    ASSERT_TRUE(board.Set((coordinate), (player_id))); \
    board.Settlement(); \
} while(0)

TEST(TestUnityChess, bonus_should_not_be_counted_as_available_count)
{
    Board board(8, {Coordinate(0, 0)}, k_image_path);
    ASSERT_EQ(63, board.Settlement().available_area_count_);
}

TEST(TestUnityChess, available_area_count_decrease_after_set)
{
    Board board(8, {}, k_image_path);
    ASSERT_TRUE(board.Set(Coordinate(0, 0), 0));
    ASSERT_TRUE(board.Set(Coordinate(0, 0), 1));
    ASSERT_TRUE(board.Set(Coordinate(0, 1), 2));
    ASSERT_EQ(62, board.Settlement().available_area_count_);
}

TEST(TestUnityChess, simple_consecutive_line)
{
    Board board(8, {}, k_image_path);

    ASSERT_TRUE(board.Set(Coordinate(0, 0), 0));
    auto result = board.Settlement();
    ASSERT_EQ(63, result.available_area_count_);
    ASSERT_EQ((std::array{0U, 0U, 0U}), result.color_counts_);

    ASSERT_TRUE(board.Set(Coordinate(0, 1), 0));
    result = board.Settlement();
    ASSERT_EQ(62, result.available_area_count_);
    ASSERT_EQ((std::array{0U, 0U, 0U}), result.color_counts_);

    ASSERT_TRUE(board.Set(Coordinate(0, 2), 0));
    result = board.Settlement();
    ASSERT_EQ(61, result.available_area_count_);
    ASSERT_EQ((std::array{12U, 0U, 0U}), result.color_counts_);
}

TEST(TestUnityChess, cannot_set_when_there_is_already_a_chess)
{
    Board board(8, {}, k_image_path);
    SET_AND_SETTLEMENT(Coordinate(0, 0), 0);
    ASSERT_FALSE(board.Set(Coordinate(0, 0), 0));
}

TEST(TestUnityChess, cannot_set_on_bonus_area)
{
    Board board(8, {Coordinate(0, 0)}, k_image_path);
    ASSERT_FALSE(board.Set(Coordinate(0, 0), 0));
}

TEST(TestUnityChess, color_cover)
{
    Board board(8, {}, k_image_path);

    SET_AND_SETTLEMENT(Coordinate(0, 1), 0);
    SET_AND_SETTLEMENT(Coordinate(1, 1), 0);
    SET_AND_SETTLEMENT(Coordinate(2, 1), 0);
    ASSERT_EQ((std::array{18U, 0U, 0U}), board.Settlement().color_counts_);

    SET_AND_SETTLEMENT(Coordinate(0, 2), 1);
    SET_AND_SETTLEMENT(Coordinate(1, 2), 1);
    SET_AND_SETTLEMENT(Coordinate(2, 2), 1);
    ASSERT_EQ((std::array{6U, 18U, 0U}), board.Settlement().color_counts_);
}

TEST(TestUnityChess, bonus_area_count_three)
{
    Board board(8, {Coordinate(0, 0)}, k_image_path);
    SET_AND_SETTLEMENT(Coordinate(0, 1), 0);
    SET_AND_SETTLEMENT(Coordinate(1, 1), 0);
    SET_AND_SETTLEMENT(Coordinate(2, 1), 0);
    ASSERT_EQ((std::array{22U, 0U, 0U}), board.Settlement().color_counts_);
}

TEST(TestUnityChess, color_big_area)
{
    Board board(8, {}, k_image_path);

    SET_AND_SETTLEMENT(Coordinate(1, 1), 0);
    SET_AND_SETTLEMENT(Coordinate(1, 3), 0);
    SET_AND_SETTLEMENT(Coordinate(1, 5), 0);
    SET_AND_SETTLEMENT(Coordinate(2, 2), 0);
    SET_AND_SETTLEMENT(Coordinate(2, 3), 0);
    SET_AND_SETTLEMENT(Coordinate(2, 4), 0);
    SET_AND_SETTLEMENT(Coordinate(3, 1), 0);
    SET_AND_SETTLEMENT(Coordinate(3, 2), 0);
    SET_AND_SETTLEMENT(Coordinate(3, 4), 0);
    SET_AND_SETTLEMENT(Coordinate(3, 5), 0);
    SET_AND_SETTLEMENT(Coordinate(4, 2), 0);
    SET_AND_SETTLEMENT(Coordinate(4, 3), 0);
    SET_AND_SETTLEMENT(Coordinate(4, 4), 0);
    SET_AND_SETTLEMENT(Coordinate(5, 1), 0);
    SET_AND_SETTLEMENT(Coordinate(5, 3), 0);
    SET_AND_SETTLEMENT(Coordinate(5, 5), 0);

    SET_AND_SETTLEMENT(Coordinate(0, 3), 1);
    SET_AND_SETTLEMENT(Coordinate(1, 2), 1);
    SET_AND_SETTLEMENT(Coordinate(1, 4), 1);
    SET_AND_SETTLEMENT(Coordinate(2, 1), 1);
    SET_AND_SETTLEMENT(Coordinate(2, 5), 1);
    SET_AND_SETTLEMENT(Coordinate(3, 0), 1);
    SET_AND_SETTLEMENT(Coordinate(3, 6), 1);
    SET_AND_SETTLEMENT(Coordinate(4, 1), 1);
    SET_AND_SETTLEMENT(Coordinate(4, 5), 1);
    SET_AND_SETTLEMENT(Coordinate(5, 2), 1);
    SET_AND_SETTLEMENT(Coordinate(5, 4), 1);
    SET_AND_SETTLEMENT(Coordinate(6, 3), 1);

    ASSERT_EQ(2, board.Settlement().color_counts_[0]); // (3, 3) is colored by player 1

    ASSERT_TRUE(board.Set(Coordinate(3, 3), 0));
    ASSERT_EQ(50, board.Settlement().color_counts_[0]);
}

TEST(TestUnityChess, cover_at_same_time)
{
    Board board(8, {}, k_image_path);

    ASSERT_TRUE(board.Set(Coordinate(1, 1), 0));
    ASSERT_TRUE(board.Set(Coordinate(2, 1), 1));
    ASSERT_TRUE(board.Set(Coordinate(3, 1), 2));
    board.Settlement();

    ASSERT_TRUE(board.Set(Coordinate(1, 2), 0));
    ASSERT_TRUE(board.Set(Coordinate(2, 2), 1));
    ASSERT_TRUE(board.Set(Coordinate(3, 2), 2));
    board.Settlement();

    ASSERT_TRUE(board.Set(Coordinate(1, 3), 0));
    ASSERT_TRUE(board.Set(Coordinate(2, 3), 1));
    ASSERT_TRUE(board.Set(Coordinate(3, 3), 2));

    ASSERT_EQ((std::array{9U, 6U, 9U}), board.Settlement().color_counts_);
}

TEST(TestUnityChess, set_at_same_place)
{
    Board board(8, {}, k_image_path);

    ASSERT_TRUE(board.Set(Coordinate(1, 1), 0));
    ASSERT_TRUE(board.Set(Coordinate(3, 1), 1));
    board.Settlement();

    ASSERT_TRUE(board.Set(Coordinate(2, 2), 0));
    ASSERT_TRUE(board.Set(Coordinate(2, 2), 1));
    board.Settlement();

    ASSERT_TRUE(board.Set(Coordinate(3, 3), 0));
    ASSERT_TRUE(board.Set(Coordinate(1, 3), 1));
    board.Settlement();

    ASSERT_EQ((std::array{9U, 9U, 0U}), board.Settlement().color_counts_);
}
