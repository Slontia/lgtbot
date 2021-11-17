#include "game_util/quixo.h"

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace quixo;

class TestQuixo : public testing::Test {};

TEST_F(TestQuixo, make_row_line)
{
    Board board("");
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(15, 5, Type::X)) << i;
        ASSERT_EQ(Type::_, board.CheckWin()) << i;
    }
    ASSERT_EQ(ErrCode::OK, board.Push(15, 5, Type::X));
    ASSERT_EQ(Type::X, board.CheckWin());
}

TEST_F(TestQuixo, make_col_line)
{
    Board board("");
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(1, 11, Type::X)) << i;
        ASSERT_EQ(Type::_, board.CheckWin()) << i;
    }
    ASSERT_EQ(ErrCode::OK, board.Push(1, 11, Type::X));
    ASSERT_EQ(Type::X, board.CheckWin());
}

TEST_F(TestQuixo, make_left_top_line)
{
    Board board("");

    ASSERT_EQ(ErrCode::OK, board.Push(4, 0, Type::X));
    ASSERT_EQ(Type::_, board.CheckWin());

    ASSERT_EQ(ErrCode::OK, board.Push(5, 15, Type::X));
    ASSERT_EQ(ErrCode::OK, board.Push(5, 15, Type::O));
    ASSERT_EQ(Type::_, board.CheckWin());

    ASSERT_EQ(ErrCode::OK, board.Push(6, 14, Type::X));
    ASSERT_EQ(ErrCode::OK, board.Push(6, 14, Type::O));
    ASSERT_EQ(ErrCode::OK, board.Push(6, 14, Type::O));
    ASSERT_EQ(Type::_, board.CheckWin());

    ASSERT_EQ(ErrCode::OK, board.Push(13, 7, Type::X));
    ASSERT_EQ(ErrCode::OK, board.Push(13, 7, Type::O));
    ASSERT_EQ(Type::_, board.CheckWin());

    ASSERT_EQ(ErrCode::OK, board.Push(12, 8, Type::X));
    ASSERT_EQ(Type::X, board.CheckWin());
}

TEST_F(TestQuixo, make_right_top_line)
{
    Board board("");

    ASSERT_EQ(ErrCode::OK, board.Push(0, 4, Type::X));
    ASSERT_EQ(Type::_, board.CheckWin());

    ASSERT_EQ(ErrCode::OK, board.Push(15, 5, Type::X));
    ASSERT_EQ(ErrCode::OK, board.Push(15, 5, Type::O));
    ASSERT_EQ(Type::_, board.CheckWin());

    ASSERT_EQ(ErrCode::OK, board.Push(14, 6, Type::X));
    ASSERT_EQ(ErrCode::OK, board.Push(14, 6, Type::O));
    ASSERT_EQ(ErrCode::OK, board.Push(14, 6, Type::O));
    ASSERT_EQ(Type::_, board.CheckWin());

    ASSERT_EQ(ErrCode::OK, board.Push(7, 13, Type::X));
    ASSERT_EQ(ErrCode::OK, board.Push(7, 13, Type::O));
    ASSERT_EQ(Type::_, board.CheckWin());

    ASSERT_EQ(ErrCode::OK, board.Push(8, 12, Type::X));
    ASSERT_EQ(Type::X, board.CheckWin());
}

