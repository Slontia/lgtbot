#include "game_util/numcomb.h"
#include <gtest/gtest.h>
#include <gflags/gflags.h>

class TestComb : public testing::Test {};

TEST_F(TestComb, coordinate_to_index)
{
    ASSERT_EQ(1, comb::Comb::ToIndex(comb::Coordinate{-2, -2}));
    ASSERT_EQ(2, comb::Comb::ToIndex(comb::Coordinate{-2, 0}));
    ASSERT_EQ(3, comb::Comb::ToIndex(comb::Coordinate{-2, 2}));
    ASSERT_EQ(4, comb::Comb::ToIndex(comb::Coordinate{-1, -3}));
    ASSERT_EQ(5, comb::Comb::ToIndex(comb::Coordinate{-1, -1}));
    ASSERT_EQ(6, comb::Comb::ToIndex(comb::Coordinate{-1, 1}));
    ASSERT_EQ(7, comb::Comb::ToIndex(comb::Coordinate{-1, 3}));
    ASSERT_EQ(8, comb::Comb::ToIndex(comb::Coordinate{0, -4}));
    ASSERT_EQ(9, comb::Comb::ToIndex(comb::Coordinate{0, -2}));
    ASSERT_EQ(10, comb::Comb::ToIndex(comb::Coordinate{0, 0}));
    ASSERT_EQ(11, comb::Comb::ToIndex(comb::Coordinate{0, 2}));
    ASSERT_EQ(12, comb::Comb::ToIndex(comb::Coordinate{0, 4}));
    ASSERT_EQ(13, comb::Comb::ToIndex(comb::Coordinate{1, -3}));
    ASSERT_EQ(14, comb::Comb::ToIndex(comb::Coordinate{1, -1}));
    ASSERT_EQ(15, comb::Comb::ToIndex(comb::Coordinate{1, 1}));
    ASSERT_EQ(16, comb::Comb::ToIndex(comb::Coordinate{1, 3}));
    ASSERT_EQ(17, comb::Comb::ToIndex(comb::Coordinate{2, -2}));
    ASSERT_EQ(18, comb::Comb::ToIndex(comb::Coordinate{2, 0}));
    ASSERT_EQ(19, comb::Comb::ToIndex(comb::Coordinate{2, 2}));
}

TEST_F(TestComb, fill_line_mismatch)
{
    comb::Comb cb("");
    ASSERT_EQ(0, cb.Fill(4, comb::AreaCard(4, 9, 6)));
    ASSERT_EQ(0, cb.Fill(5, comb::AreaCard(3, 5, 7)));
    ASSERT_EQ(0, cb.Fill(6, comb::AreaCard(3, 5, 7)));
    ASSERT_EQ(0, cb.Fill(7, comb::AreaCard(3, 5, 7)));
}

TEST_F(TestComb, get_score)
{
    comb::Comb cb("");
    ASSERT_EQ(0, cb.Fill(8, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(9, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(10, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(11, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(45, cb.Fill(12, comb::AreaCard(8, 9, 6)));

    ASSERT_EQ(0, cb.Fill(3, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(24, cb.Fill(7, comb::AreaCard(8, 9, 6)));

    ASSERT_EQ(0, cb.Fill(18, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(24, cb.Fill(15, comb::AreaCard(8, 9, 6)));
}

TEST_F(TestComb, get_score_three_direct)
{
    comb::Comb cb("");
    ASSERT_EQ(0, cb.Fill(8, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(9, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(10, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(12, comb::AreaCard(8, 9, 6)));

    ASSERT_EQ(0, cb.Fill(2, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(6, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(16, comb::AreaCard(8, 9, 6)));

    ASSERT_EQ(0, cb.Fill(7, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(18, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(15, comb::AreaCard(8, 9, 6)));

    ASSERT_EQ(8 * 4 + 9 * 5 + 6 * 4, cb.Fill(11, comb::AreaCard(8, 9, 6)));
}

TEST_F(TestComb, get_score_wild)
{
    comb::Comb cb("");
    ASSERT_EQ(0, cb.Fill(8, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(9, comb::AreaCard()));
    ASSERT_EQ(0, cb.Fill(10, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(11, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(45, cb.Fill(12, comb::AreaCard(8, 9, 6)));

    ASSERT_EQ(0, cb.Fill(3, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(24, cb.Fill(7, comb::AreaCard()));
}

TEST_F(TestComb, get_score_three_direct_wild)
{
    comb::Comb cb("");
    ASSERT_EQ(0, cb.Fill(8, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(9, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(10, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(12, comb::AreaCard(8, 9, 6)));

    ASSERT_EQ(0, cb.Fill(2, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(6, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(16, comb::AreaCard(8, 9, 6)));

    ASSERT_EQ(0, cb.Fill(7, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(18, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(0, cb.Fill(15, comb::AreaCard(8, 9, 6)));

    ASSERT_EQ(8 * 4 + 9 * 5 + 6 * 4, cb.Fill(11, comb::AreaCard()));
}

TEST_F(TestComb, all_set)
{
    comb::Comb cb("");
    int32_t score = 0;
    for (int32_t i = 1; i <= 19; ++i) {
        score += cb.Fill(i, comb::AreaCard(8, 9, 6));
    }
    ASSERT_EQ(19 * (8 + 9 + 6), score);
}

TEST_F(TestComb, fill_zero)
{
    ASSERT_EQ(8 + 9 + 6, comb::Comb("").Fill(0, comb::AreaCard(8, 9, 6)));
    ASSERT_EQ(30, comb::Comb("").Fill(0, comb::AreaCard()));
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
