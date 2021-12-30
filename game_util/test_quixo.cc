// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/quixo.h"

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace quixo;

class TestQuixo : public testing::Test {};

TEST_F(TestQuixo, make_row_line)
{
    Board board("");
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(15, 5, Type::X2)) << i;
        const auto ret = board.LineCount();
        ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]) << i;
        ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]) << i;
    }
    ASSERT_EQ(ErrCode::OK, board.Push(15, 5, Type::X1));
    const auto ret = board.LineCount();
    ASSERT_EQ(1, ret[static_cast<uint32_t>(Symbol::X)]);
}

TEST_F(TestQuixo, make_col_line)
{
    Board board("");
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(1, 11, Type::X2)) << i;
        const auto ret = board.LineCount();
        ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]) << i;
        ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]) << i;
    }
    ASSERT_EQ(ErrCode::OK, board.Push(1, 11, Type::X1));
    const auto ret = board.LineCount();
    ASSERT_EQ(1, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);
}

TEST_F(TestQuixo, make_left_top_line)
{
    Board board("");

    ASSERT_EQ(ErrCode::OK, board.Push(4, 0, Type::X2));
    auto ret = board.LineCount();
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);

    ASSERT_EQ(ErrCode::OK, board.Push(5, 15, Type::X1));
    ASSERT_EQ(ErrCode::OK, board.Push(5, 15, Type::O1));
    ret = board.LineCount();
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);

    ASSERT_EQ(ErrCode::OK, board.Push(6, 14, Type::X1));
    ASSERT_EQ(ErrCode::OK, board.Push(6, 14, Type::O1));
    ASSERT_EQ(ErrCode::OK, board.Push(6, 14, Type::O1));
    ret = board.LineCount();
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);

    ASSERT_EQ(ErrCode::OK, board.Push(13, 7, Type::X1));
    ASSERT_EQ(ErrCode::OK, board.Push(13, 7, Type::O1));
    ret = board.LineCount();
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);

    ASSERT_EQ(ErrCode::OK, board.Push(12, 8, Type::X1));
    ret = board.LineCount();
    ASSERT_EQ(1, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);
}

TEST_F(TestQuixo, make_right_top_line)
{
    Board board("");

    ASSERT_EQ(ErrCode::OK, board.Push(0, 4, Type::X2));
    auto ret = board.LineCount();
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);

    ASSERT_EQ(ErrCode::OK, board.Push(15, 5, Type::X1));
    ASSERT_EQ(ErrCode::OK, board.Push(15, 5, Type::O1));
    ret = board.LineCount();
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);

    ASSERT_EQ(ErrCode::OK, board.Push(14, 6, Type::X1));
    ASSERT_EQ(ErrCode::OK, board.Push(14, 6, Type::O1));
    ASSERT_EQ(ErrCode::OK, board.Push(14, 6, Type::O1));
    ret = board.LineCount();
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);

    ASSERT_EQ(ErrCode::OK, board.Push(7, 13, Type::X1));
    ASSERT_EQ(ErrCode::OK, board.Push(7, 13, Type::O1));
    ret = board.LineCount();
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);

    ASSERT_EQ(ErrCode::OK, board.Push(8, 12, Type::X1));
    ret = board.LineCount();
    ASSERT_EQ(1, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);
}

TEST_F(TestQuixo, full_lines)
{
    Board board("");
    for (uint32_t i = 0; i < 5; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(0, 4, i % 2 ? Type::X1 : Type::X2));
        ASSERT_EQ(ErrCode::OK, board.Push(15, 5, i % 2 ? Type::X1 : Type::X2));
        ASSERT_EQ(ErrCode::OK, board.Push(14, 6, i % 2 ? Type::X1 : Type::X2));
        ASSERT_EQ(ErrCode::OK, board.Push(13, 7, i % 2 ? Type::X1 : Type::X2));
        ASSERT_EQ(ErrCode::OK, board.Push(12, 8, i % 2 ? Type::X1 : Type::X2));
    }
    auto ret = board.LineCount();
    ASSERT_EQ(12, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);
}

TEST_F(TestQuixo, double_symbol_line)
{
    Board board("");
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(0, 4, i % 2 ? Type::X1 : Type::X2));
        ASSERT_EQ(ErrCode::OK, board.Push(15, 5, i % 2 ? Type::O1 : Type::O2));
    }
    ASSERT_EQ(ErrCode::OK, board.Push(15, 5, Type::X2));
    ASSERT_EQ(ErrCode::OK, board.Push(0, 4, Type::O2));
    auto ret = board.LineCount();
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(0, ret[static_cast<uint32_t>(Symbol::O)]);
    ASSERT_EQ(ErrCode::OK, board.Push(5, 4, Type::X2));
    ret = board.LineCount();
    ASSERT_EQ(1, ret[static_cast<uint32_t>(Symbol::X)]);
    ASSERT_EQ(1, ret[static_cast<uint32_t>(Symbol::O)]);
}

TEST_F(TestQuixo, invalid_src)
{
    Board board("");
    ASSERT_EQ(ErrCode::OK, board.Push(14, 6, Type::X1));
    ASSERT_EQ(ErrCode::INVALID_SRC, board.Push(6, 14, Type::X2));
    ASSERT_EQ(ErrCode::INVALID_SRC, board.Push(6, 14, Type::O1));
    ASSERT_EQ(ErrCode::INVALID_SRC, board.Push(6, 14, Type::O2));
}

static const std::array<std::vector<uint32_t>, 16> k_valid_dsts{
    std::vector<uint32_t>{4, 12}, // 0
    std::vector<uint32_t>{0, 4, 11}, // 1
    std::vector<uint32_t>{0, 4, 10}, // 2
    std::vector<uint32_t>{0, 4, 9}, // 3
    std::vector<uint32_t>{0, 8}, // 4
    std::vector<uint32_t>{4, 8, 15}, // 5
    std::vector<uint32_t>{4, 8, 14}, // 6
    std::vector<uint32_t>{4, 8, 13}, // 7
    std::vector<uint32_t>{4, 12}, // 8
    std::vector<uint32_t>{3, 8, 12}, // 9
    std::vector<uint32_t>{2, 8, 12}, // 10
    std::vector<uint32_t>{1, 8, 12}, // 11
    std::vector<uint32_t>{0, 8}, // 12
    std::vector<uint32_t>{0, 7, 12}, // 13
    std::vector<uint32_t>{0, 6, 12}, // 14
    std::vector<uint32_t>{0, 5, 12}, // 15
};

TEST_F(TestQuixo, invalid_dst)
{
    Board board("");
    for (uint32_t src = 0; src < 16; ++src) {
        const auto valid_dsts = k_valid_dsts[src];
        for (uint32_t dst = 0; dst < 16; ++dst) {
            if (std::none_of(valid_dsts.begin(), valid_dsts.end(), [dst](const auto i) { return i == dst; })) {
                ASSERT_EQ(ErrCode::INVALID_DST, board.Push(src, dst, Type::X1));
            }
        }
    }
}

TEST_F(TestQuixo, valid_dst_hint)
{
    Board board("");
    for (uint32_t src = 0; src < 16; ++src) {
        auto valid_dsts = board.ValidDsts(src);
        std::sort(valid_dsts.begin(), valid_dsts.end());
        ASSERT_EQ(valid_dsts, k_valid_dsts[src]);
    }
}

TEST_F(TestQuixo, can_push)
{
    Board board("");
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(0, 4, Type::X1));
    }
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(12, 0, Type::X1));
    }
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(8, 12, Type::X1));
    }
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(ErrCode::OK, board.Push(8, 4, Type::X1));
    }
    ASSERT_TRUE(board.CanPush(Type::X1));
    ASSERT_FALSE(board.CanPush(Type::X2));
    ASSERT_FALSE(board.CanPush(Type::O1));
    ASSERT_FALSE(board.CanPush(Type::O2));
}
