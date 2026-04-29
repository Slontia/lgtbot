// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/laser_chess.h"

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace lgtbot::game_util::laser_chess;

class TestLaserChess : public testing::Test {};

TEST_F(TestLaserChess, single_chess_reflect)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{1, 1}, SingleMirrorChess<1>(RIGHT));
    b.SetChess(Coor{0, 1}, KingChess<0>());
    b.SetChess(Coor{1, 2}, KingChess<0>());
    b.SetChess(Coor{2, 1}, KingChess<1>());
    const auto ret = b.Settle();
    ASSERT_EQ(2, ret.king_alive_num_[0]);
    ASSERT_EQ(0, ret.king_alive_num_[1]);
    ASSERT_EQ(0, ret.chess_dead_num_[0]);
    ASSERT_EQ(1, ret.chess_dead_num_[1]);
    ASSERT_EQ(3, b.ChessCount(0));
    ASSERT_EQ(1, b.ChessCount(1));
}

TEST_F(TestLaserChess, single_chess_dead)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{1, 1}, SingleMirrorChess<1>(UP));
    b.SetChess(Coor{0, 1}, KingChess<0>());
    b.SetChess(Coor{1, 2}, KingChess<0>());
    b.SetChess(Coor{2, 1}, KingChess<1>());
    const auto ret = b.Settle();
    ASSERT_EQ(2, ret.king_alive_num_[0]);
    ASSERT_EQ(1, ret.king_alive_num_[1]);
    ASSERT_EQ(0, ret.chess_dead_num_[0]);
    ASSERT_EQ(1, ret.chess_dead_num_[1]);
    ASSERT_EQ(3, b.ChessCount(0));
    ASSERT_EQ(1, b.ChessCount(1));
}

TEST_F(TestLaserChess, double_chess_reflect)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{1, 1}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{2, 1}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{2, 2}, DoubleMirrorChess<1>(true));
    b.SetChess(Coor{0, 2}, DoubleMirrorChess<1>(false));
    b.SetChess(Coor{0, 1}, KingChess<0>());
    const auto ret = b.Settle();
    ASSERT_EQ(0, ret.king_alive_num_[0]);
    ASSERT_EQ(0, ret.king_alive_num_[1]);
    ASSERT_EQ(1, ret.chess_dead_num_[0]);
    ASSERT_EQ(0, ret.chess_dead_num_[1]);
    ASSERT_EQ(1, b.ChessCount(0));
    ASSERT_EQ(4, b.ChessCount(1));
}

TEST_F(TestLaserChess, lensed_chess_reflect)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{1, 1}, LensedMirrorChess<1>(false));
    b.SetChess(Coor{1, 2}, ShieldChess<0>(LEFT));
    b.SetChess(Coor{0, 1}, KingChess<0>());
    const auto ret = b.Settle();
    ASSERT_EQ(0, ret.king_alive_num_[0]);
    ASSERT_EQ(0, ret.king_alive_num_[1]);
    ASSERT_EQ(1, ret.chess_dead_num_[0]);
    ASSERT_EQ(0, ret.chess_dead_num_[1]);
    ASSERT_EQ(2, b.ChessCount(0));
    ASSERT_EQ(1, b.ChessCount(1));
}

#define ASSERT_SUCC(ret) \
do { \
    const auto errmsg = (ret); \
    ASSERT_TRUE(errmsg.empty()) << errmsg; \
} while (0)

#define ASSERT_FAIL(ret) ASSERT_FALSE((ret).empty())

TEST_F(TestLaserChess, move_crash)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 1}, LensedMirrorChess<1>(false));
    b.SetChess(Coor{1, 2}, ShieldChess<0>(LEFT));
    ASSERT_SUCC(b.Move(Coor{1, 1}, Coor{2, 1}, 1));
    ASSERT_SUCC(b.Move(Coor{1, 2}, Coor{2, 1}, 0));
    ASSERT_TRUE(b.IsEmpty(Coor{1, 1}));
    ASSERT_TRUE(b.IsEmpty(Coor{2, 1}));
    ASSERT_TRUE(b.IsEmpty(Coor{1, 2}));
    const auto ret = b.Settle();
    ASSERT_EQ(0, b.ChessCount(0));
    ASSERT_EQ(0, b.ChessCount(1));
}

TEST_F(TestLaserChess, move_back)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 1}, LensedMirrorChess<1>(false));
    ASSERT_SUCC(b.Move(Coor{1, 1}, Coor{2, 1}, 1));
    b.Settle();
    ASSERT_SUCC(b.Move(Coor{2, 1}, Coor{1, 1}, 1));
    const auto ret = b.Settle();
    ASSERT_EQ(0, b.ChessCount(0));
    ASSERT_EQ(1, b.ChessCount(1));
}

TEST_F(TestLaserChess, move_to_other_src)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 1}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{1, 2}, LensedMirrorChess<1>(false));
    ASSERT_SUCC(b.Move(Coor{1, 1}, Coor{2, 1}, 0));
    ASSERT_FAIL(b.Move(Coor{1, 2}, Coor{1, 1}, 1));
    const auto ret = b.Settle();
    ASSERT_EQ(1, b.ChessCount(0));
    ASSERT_EQ(1, b.ChessCount(1));
}

TEST_F(TestLaserChess, rotate_self)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 1}, LensedMirrorChess<0>(false));
    ASSERT_SUCC(b.Rotate(Coor{1, 1}, true, 0));
    const auto ret = b.Settle();
    ASSERT_EQ(1, b.ChessCount(0));
    ASSERT_EQ(0, b.ChessCount(1));
}

TEST_F(TestLaserChess, rotate_other)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 1}, LensedMirrorChess<0>(false));
    ASSERT_FAIL(b.Rotate(Coor{1, 1}, true, 1));
    const auto ret = b.Settle();
    ASSERT_EQ(1, b.ChessCount(0));
    ASSERT_EQ(0, b.ChessCount(1));
}

TEST_F(TestLaserChess, cannot_rotate_chess_nearby_king)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 1}, KingChess<0>());
    b.SetChess(Coor{0, 0}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{0, 1}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{0, 2}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{1, 2}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{2, 2}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{2, 1}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{2, 0}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{1, 0}, LensedMirrorChess<0>(false));
    ASSERT_FAIL(b.Rotate(Coor{0, 0}, true, 0));
    ASSERT_FAIL(b.Rotate(Coor{0, 1}, true, 0));
    ASSERT_FAIL(b.Rotate(Coor{0, 2}, true, 0));
    ASSERT_FAIL(b.Rotate(Coor{1, 2}, true, 0));
    ASSERT_FAIL(b.Rotate(Coor{2, 2}, true, 0));
    ASSERT_FAIL(b.Rotate(Coor{2, 1}, true, 0));
    ASSERT_FAIL(b.Rotate(Coor{2, 0}, true, 0));
    ASSERT_FAIL(b.Rotate(Coor{1, 0}, true, 0));
}

TEST_F(TestLaserChess, cannot_move_chess_nearby_king)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 1}, KingChess<0>());
    b.SetChess(Coor{2, 2}, LensedMirrorChess<0>(false));
    ASSERT_FAIL(b.Move(Coor{2, 2}, Coor{2, 1}, 0));
}

TEST_F(TestLaserChess, cannot_swap_chess_nearby_king)
{
    Board b(8, 8, "");
    b.SetChess(Coor{1, 1}, KingChess<0>());
    b.SetChess(Coor{2, 2}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{1, 2}, LensedMirrorChess<1>(false));
    b.SetChess(Coor{1, 3}, LensedMirrorChess<0>(false));
    ASSERT_FAIL(b.Move(Coor{1, 3}, Coor{1, 2}, 0));
    ASSERT_FAIL(b.Move(Coor{1, 3}, Coor{2, 2}, 0));
}

TEST_F(TestLaserChess, can_rotate_nearby_dst_king)
{
    Board b(8, 8, "");
    b.SetChess(Coor{0, 0}, KingChess<1>());
    b.SetChess(Coor{2, 2}, LensedMirrorChess<0>(false));
    ASSERT_SUCC(b.Move(Coor{0, 0}, Coor{1, 1}, 1));
    ASSERT_SUCC(b.Rotate(Coor{2, 2}, true, 0));
}

TEST_F(TestLaserChess, kill_two_chess)
{
    Board b(8, 8, "");
    b.SetChess(Coor{0, 0}, ShooterChess<0>(RIGHT, std::bitset<4>().set(RIGHT).set(DOWN)));
    b.SetChess(Coor{0, 1}, LensedMirrorChess<0>(false));
    b.SetChess(Coor{0, 2}, ShieldChess<0>(DOWN));
    b.SetChess(Coor{1, 1}, ShieldChess<0>(DOWN));
    const auto ret = b.Settle();
    ASSERT_EQ(2, b.ChessCount(0));
    ASSERT_EQ(0, b.ChessCount(1));
}

