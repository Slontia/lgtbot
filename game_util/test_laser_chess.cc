// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/laser_chess.h"

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace laser;

class TestLaserChess : public testing::Test {};

TEST_F(TestLaserChess, single_chess_reflect)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{7, 7}, ShooterChess<1>(UP, std::bitset<4>().set(LEFT).set(UP)));
    b.SetChess(Coor{1, 1}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{0, 1}, KingChess<0>());
    b.SetChess(Coor{1, 2}, KingChess<0>());
    b.SetChess(Coor{2, 1}, KingChess<1>());
    b.Shoot(0);
    const auto ret = b.Settle();
    ASSERT_EQ(0, ret.king_dead_num_[0]);
    ASSERT_EQ(1, ret.king_dead_num_[1]);
    ASSERT_EQ(0, ret.chess_dead_num_[0]);
    ASSERT_EQ(1, ret.chess_dead_num_[1]);
}

TEST_F(TestLaserChess, single_chess_dead)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{7, 7}, ShooterChess<1>(UP, std::bitset<4>().set(LEFT).set(UP)));
    b.SetChess(Coor{1, 1}, SingleMirrorChess<1>(UP));
    b.SetChess(Coor{0, 1}, KingChess<0>());
    b.SetChess(Coor{1, 2}, KingChess<0>());
    b.SetChess(Coor{2, 1}, KingChess<1>());
    b.Shoot(0);
    const auto ret = b.Settle();
    ASSERT_EQ(0, ret.king_dead_num_[0]);
    ASSERT_EQ(0, ret.king_dead_num_[1]);
    ASSERT_EQ(0, ret.chess_dead_num_[0]);
    ASSERT_EQ(1, ret.chess_dead_num_[1]);
}

TEST_F(TestLaserChess, double_chess_reflect)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{7, 7}, ShooterChess<1>(UP, std::bitset<4>().set(LEFT).set(UP)));
    b.SetChess(Coor{1, 1}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{2, 1}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{2, 2}, DoubleMirrorChess<1>(true));
    b.SetChess(Coor{0, 2}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{0, 1}, KingChess<0>());
    b.Shoot(0);
    const auto ret = b.Settle();
    ASSERT_EQ(1, ret.king_dead_num_[0]);
    ASSERT_EQ(0, ret.king_dead_num_[1]);
    ASSERT_EQ(1, ret.chess_dead_num_[0]);
    ASSERT_EQ(0, ret.chess_dead_num_[1]);
}

TEST_F(TestLaserChess, lensed_chess_reflect)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{7, 7}, ShooterChess<1>(UP, std::bitset<4>().set(LEFT).set(UP)));
    b.SetChess(Coor{1, 1}, LensedMirrorChess<1>(false));
    b.SetChess(Coor{1, 2}, ShieldChess<0>(LEFT));
    b.SetChess(Coor{0, 1}, KingChess<0>());
    b.Shoot(0);
    const auto ret = b.Settle();
    ASSERT_EQ(1, ret.king_dead_num_[0]);
    ASSERT_EQ(0, ret.king_dead_num_[1]);
    ASSERT_EQ(1, ret.chess_dead_num_[0]);
    ASSERT_EQ(0, ret.chess_dead_num_[1]);
}
