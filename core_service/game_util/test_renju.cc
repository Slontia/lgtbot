// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/renju.h"

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace lgtbot::game_util::renju;

class TestGoBoard : public testing::Test {};

TEST_F(TestGoBoard, invalid_point_center)
{
    GoBoard board;
    board.Set(7, 7, AreaType::BLACK);
    board.Set(9, 7, AreaType::BLACK);
    board.Set(8, 6, AreaType::BLACK);
    board.Set(8, 8, AreaType::BLACK);
    ASSERT_FALSE(board.CanBeSet(8, 7, AreaType::WHITE));
}

TEST_F(TestGoBoard, invalid_point_edge)
{
    GoBoard board;
    board.Set(0, 7, AreaType::BLACK);
    board.Set(0, 5, AreaType::BLACK);
    board.Set(1, 6, AreaType::BLACK);
    ASSERT_FALSE(board.CanBeSet(0, 6, AreaType::WHITE));
}

TEST_F(TestGoBoard, invalid_point_with_one_piece)
{
    GoBoard board;
    board.Set(7, 7, AreaType::BLACK);
    board.Set(8, 6, AreaType::BLACK);
    board.Set(9, 6, AreaType::BLACK);
    board.Set(8, 8, AreaType::BLACK);
    board.Set(9, 8, AreaType::BLACK);
    board.Set(10, 7, AreaType::BLACK);
    board.Set(9, 7, AreaType::WHITE);
    ASSERT_FALSE(board.CanBeSet(8, 7, AreaType::WHITE));
}
