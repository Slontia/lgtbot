#define private public
#include "game_util/alchemist.h"
#undef private

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace alchemist;

class TestAlchemist : public testing::Test {};

static_assert(Board::k_size == 5, "board k_size mismatch");

TEST_F(TestAlchemist, already_set)
{
    Board board("");
    ASSERT_EQ(FAIL_ALREADY_SET, board.SetOrClearLine(2, 2, Card{Color::RED, Point::FIVE}));
}

TEST_F(TestAlchemist, make_one_row)
{
    Board board("");
    ASSERT_EQ(0, board.SetOrClearLine(2, 3, Card{Color::RED, Point::FIVE}));
    ASSERT_EQ(0, board.SetOrClearLine(2, 4, Card{Color::RED, Point::FIVE}));
    ASSERT_EQ(0, board.SetOrClearLine(2, 1, Card{Color::RED, Point::FIVE}));
    ASSERT_EQ(2, board.SetOrClearLine(2, 0, Card{Color::RED, Point::FIVE}));

    for (uint32_t i = 0; i < 5; ++i) {
        ASSERT_EQ(FAIL_NON_ADJ_CARDS, board.SetOrClearLine(2, i, Card{Color::RED, Point::FIVE}));
    }
}

TEST_F(TestAlchemist, make_one_col)
{
    Board board("");
    ASSERT_EQ(0, board.SetOrClearLine(3, 2, Card{Color::RED, Point::FIVE}));
    ASSERT_EQ(0, board.SetOrClearLine(4, 2, Card{Color::RED, Point::FIVE}));
    ASSERT_EQ(0, board.SetOrClearLine(1, 2, Card{Color::RED, Point::FIVE}));
    ASSERT_EQ(2, board.SetOrClearLine(0, 2, Card{Color::RED, Point::FIVE}));

    for (uint32_t i = 0; i < 5; ++i) {
        ASSERT_EQ(FAIL_NON_ADJ_CARDS, board.SetOrClearLine(i, 2, Card{Color::RED, Point::FIVE}));
    }
}

TEST_F(TestAlchemist, make_one_left_slash)
{
    Board board("");
    board.areas_[0][0] = Card{Color::RED, Point::FIVE};
    board.areas_[1][1] = Card{Color::RED, Point::FIVE};
    board.areas_[3][3] = Card{Color::RED, Point::FIVE};
    board.areas_[3][4] = Card{Color::RED, Point::FIVE};
    ASSERT_EQ(3, board.SetOrClearLine(4, 4, Card{Color::RED, Point::FIVE}));

    board.areas_[3][4].reset();
    for (uint32_t i = 0; i < 5; ++i) {
        ASSERT_EQ(FAIL_NON_ADJ_CARDS, board.SetOrClearLine(i, i, Card{Color::RED, Point::FIVE}));
    }
}

TEST_F(TestAlchemist, make_one_right_slash)
{
    Board board("");
    board.areas_[0][4] = Card{Color::RED, Point::FIVE};
    board.areas_[1][3] = Card{Color::RED, Point::FIVE};
    board.areas_[3][1] = Card{Color::RED, Point::FIVE};
    board.areas_[4][1] = Card{Color::RED, Point::FIVE};
    ASSERT_EQ(3, board.SetOrClearLine(4, 0, Card{Color::RED, Point::FIVE}));

    board.areas_[4][1].reset();
    for (uint32_t i = 0; i < 5; ++i) {
        ASSERT_EQ(FAIL_NON_ADJ_CARDS, board.SetOrClearLine(i, 5 - i, Card{Color::RED, Point::FIVE}));
    }
}

TEST_F(TestAlchemist, make_four_lines)
{
    Board board("");
    for (uint32_t i = 0; i < 5; ++i) {
        board.areas_[2][i] = Card{Color::RED, Point::FIVE};
        board.areas_[i][2] = Card{Color::RED, Point::FIVE};
        board.areas_[i][i] = Card{Color::RED, Point::FIVE};
        board.areas_[i][4 - i] = Card{Color::RED, Point::FIVE};
    }
    board.areas_[2][2].reset();
    ASSERT_EQ(10, board.SetOrClearLine(2, 2, Card{Color::RED, Point::FIVE}));

    for (uint32_t i = 0; i < 5; ++i) {
        ASSERT_EQ(FAIL_NON_ADJ_CARDS, board.SetOrClearLine(2, i, Card{Color::RED, Point::FIVE}));
        ASSERT_EQ(FAIL_NON_ADJ_CARDS, board.SetOrClearLine(i, 2, Card{Color::RED, Point::FIVE}));
        ASSERT_EQ(FAIL_NON_ADJ_CARDS, board.SetOrClearLine(i, i, Card{Color::RED, Point::FIVE}));
        ASSERT_EQ(FAIL_NON_ADJ_CARDS, board.SetOrClearLine(i, 4 - i, Card{Color::RED, Point::FIVE}));
    }
}

TEST_F(TestAlchemist, fail_non_adj)
{
    Board board("");
    for (uint32_t i = 0; i < 5; ++i) {
        for (uint32_t j = 0; j < 5; ++j) {
            if (i == 2 && j == 1 || i == 2 && j == 3 || i == 1 && j == 2 || i == 3 && j == 2 || i == 2 && j == 2) {
                continue;
            }
            ASSERT_EQ(FAIL_NON_ADJ_CARDS, board.SetOrClearLine(i, j, Card{Color::RED, Point::FIVE}));
        }
    }
}

TEST_F(TestAlchemist, adj_mismatch)
{
    Board board("");
    board.areas_[2][4] = Card{Color::RED, Point::FIVE};
    ASSERT_EQ(FAIL_ADJ_CARDS_MISMATCH, board.SetOrClearLine(2, 3, Card{Color::BLUE, Point::FOUR}));
}

TEST_F(TestAlchemist, adj_mismatch_2)
{
    Board board("");
    board.areas_[2][4] = Card{Color::RED, Point::FIVE};
    board.areas_[3][3] = Card{Color::BLUE, Point::FIVE};
    ASSERT_EQ(FAIL_ADJ_CARDS_MISMATCH, board.SetOrClearLine(2, 3, Card{Color::BLUE, Point::FOUR}));
}

TEST_F(TestAlchemist, test_clear)
{
    Board board("");
    board.areas_[2][4] = Card{Color::RED, Point::FIVE};
    ASSERT_TRUE(board.Unset(2, 4));
    ASSERT_TRUE(board.Unset(2, 2));
}
