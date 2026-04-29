// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/chinese_chess.h"

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace lgtbot::game_util::chinese_chess;

class TestChineseChess_ChessRule : public testing::Test
{
  public:
    TestChineseChess_ChessRule() : hb_(KingdomId(0)) {}

  protected:
    HalfBoard hb_;
};

TEST_F(TestChineseChess_ChessRule, can_move_ju_in_straight)
{
    const auto& chess = JuChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{0, 0}, Coor{2, 0}));
    ASSERT_TRUE(chess.CanMove(hb_, Coor{1, 0}, Coor{1, 8}));
    ASSERT_TRUE(chess.CanMove(hb_, Coor{1, 8}, Coor{1, 0}));
    ASSERT_TRUE(chess.CanMove(hb_, Coor{8, 3}, Coor{1, 3}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_ju_not_in_straight)
{
    const auto& chess = JuChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 0}, Coor{1, 1}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_ju_obstruct_by_other_chess)
{
    const auto& chess = JuChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 0}, Coor{8, 0}));
}

TEST_F(TestChineseChess_ChessRule, can_move_ma_without_obstruct)
{
    const auto& chess = MaChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{0, 1}, Coor{2, 0}));
    ASSERT_TRUE(chess.CanMove(hb_, Coor{0, 1}, Coor{2, 2}));
    ASSERT_TRUE(chess.CanMove(hb_, Coor{1, 2}, Coor{0, 0}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_ma_to_incorrect_pos)
{
    const auto& chess = MaChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 1}, Coor{1, 2}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_ma_with_obstruct)
{
    const auto& chess = MaChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 1}, Coor{1, 3}));
}

TEST_F(TestChineseChess_ChessRule, can_move_xiang_to_correct_pos)
{
    const auto& chess = XiangChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{0, 2}, Coor{2, 0}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_xiang_to_incorrect_pos)
{
    const auto& chess = XiangChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 2}, Coor{1, 2}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_xiang_cross_river)
{
    const auto& chess = XiangChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{3, 3}, Coor{5, 5}));
}

TEST_F(TestChineseChess_ChessRule, can_move_shi_to_correct_pos)
{
    const auto& chess = ShiChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{0, 3}, Coor{1, 4}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_shi_to_incorrect_pos)
{
    const auto& chess = ShiChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 3}, Coor{1, 3}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_shi_outside_house)
{
    const auto& chess = ShiChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 3}, Coor{1, 2}));
}

TEST_F(TestChineseChess_ChessRule, can_move_jiang_to_correct_pos)
{
    const auto& chess = JiangChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{0, 4}, Coor{1, 4}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_jiang_to_incorrect_pos)
{
    const auto& chess = JiangChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 4}, Coor{1, 3}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_jiang_outside_house)
{
    const auto& chess = JiangChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 3}, Coor{1, 2}));
}

TEST_F(TestChineseChess_ChessRule, can_move_jiang_to_another_jiang_without_obstruct)
{
    const auto& chess = JiangChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{7, 4}, Coor{9, 4}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_jiang_to_another_jiang_with_obstruct)
{
    const auto& chess = JiangChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 4}, Coor{9, 4}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_jiang_to_another_non_jiang_chess_without_obstruct)
{
    const auto& chess = JiangChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{0, 3}, Coor{9, 3}));
}

TEST_F(TestChineseChess_ChessRule, can_move_pao_without_jump_and_eat)
{
    const auto& chess = PaoChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{2, 1}, Coor{5, 1}));
}

TEST_F(TestChineseChess_ChessRule, can_move_pao_with_jump_and_eat)
{
    const auto& chess = PaoChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{2, 1}, Coor{9, 1}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_pao_with_jump_but_without_eat)
{
    const auto& chess = PaoChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{2, 1}, Coor{8, 1}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_pao_without_jump_but_with_eat)
{
    const auto& chess = PaoChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{2, 1}, Coor{7, 1}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_pao_jump_two_chesses)
{
    const auto& chess = PaoChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{2, 4}, Coor{9, 4}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_pao_to_incorrect_pos)
{
    const auto& chess = PaoChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{2, 1}, Coor{3, 2}));
}

TEST_F(TestChineseChess_ChessRule, can_move_zu_forward)
{
    const auto& chess = ZuChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{3, 0}, Coor{4, 0}));
    ASSERT_TRUE(chess.CanMove(hb_, Coor{6, 0}, Coor{5, 0}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_zu_otherward)
{
    const auto& chess = ZuChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{3, 0}, Coor{2, 0}));
    ASSERT_FALSE(chess.CanMove(hb_, Coor{3, 0}, Coor{3, 1}));
    ASSERT_FALSE(chess.CanMove(hb_, Coor{6, 0}, Coor{7, 0}));
    ASSERT_FALSE(chess.CanMove(hb_, Coor{6, 0}, Coor{6, 1}));
}

TEST_F(TestChineseChess_ChessRule, can_move_promoted_zu_non_backward)
{
    const auto& chess = PromotedZuChessRule::Singleton();
    ASSERT_TRUE(chess.CanMove(hb_, Coor{3, 0}, Coor{2, 0}));
    ASSERT_TRUE(chess.CanMove(hb_, Coor{3, 0}, Coor{3, 1}));
    ASSERT_TRUE(chess.CanMove(hb_, Coor{6, 0}, Coor{7, 0}));
    ASSERT_TRUE(chess.CanMove(hb_, Coor{6, 0}, Coor{6, 1}));
}

TEST_F(TestChineseChess_ChessRule, cannot_move_promoted_zu_backward)
{
    const auto& chess = PromotedZuChessRule::Singleton();
    ASSERT_FALSE(chess.CanMove(hb_, Coor{3, 0}, Coor{4, 0}));
    ASSERT_FALSE(chess.CanMove(hb_, Coor{6, 0}, Coor{5, 0}));
}

#define ASSERT_FAIL(expr) ASSERT_FALSE((expr).empty())
#define ASSERT_SUCC(expr) \
    do { \
        const auto errstr = (expr); \
        ASSERT_TRUE(errstr.empty()) << errstr; \
    } while (0)

TEST(TestChineseChess, move_chess_not_eat)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{0, 0}, Coor{1, 0}));
}

TEST(TestChineseChess, cannot_move_other_player_chess)
{
    BoardMgr board(2, 1);
    ASSERT_FAIL(board.Move(1, 0, Coor{0, 0}, Coor{1, 0}));
}

TEST(TestChineseChess, cannot_eat_self_chess)
{
    BoardMgr board(2, 1);
    ASSERT_FAIL(board.Move(0, 0, Coor{0, 0}, Coor{0, 1}));
}

TEST(TestChineseChess, eat_other_chess)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{2, 1}, Coor{9, 1}));
    board.Settle();
    ASSERT_EQ(17, board.GetScore(0));
    ASSERT_EQ(15, board.GetScore(1));
}

TEST(TestChineseChess, can_continuously_move_same_chess_if_not_eat)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{0, 0}, Coor{1, 0}));
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{1, 0}, Coor{2, 0}));
}

TEST(TestChineseChess, cannot_continuously_move_same_chess_if_eat)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{2, 1}, Coor{9, 1}));
    board.Settle();
    ASSERT_FAIL(board.Move(0, 0, Coor{9, 1}, Coor{8, 1}));
}

TEST(TestChineseChess, can_move_same_chess_skip_one_round_if_eat)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{2, 1}, Coor{9, 1}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{9, 1}, Coor{8, 1}));
}

TEST(TestChineseChess, just_moved_chess_cannot_eat)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{2, 1}, Coor{1, 1}));
    board.Settle();
    ASSERT_FAIL(board.Move(0, 0, Coor{1, 1}, Coor{9, 1}));
}

TEST(TestChineseChess, just_moved_chess_can_eat_skip_one_round)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{2, 1}, Coor{1, 1}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{1, 1}, Coor{9, 1}));
}

TEST(TestChineseChess, eat_moved_chess_means_eat_failed)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{2, 1}, Coor{9, 1}));
    ASSERT_SUCC(board.Move(1, 0, Coor{9, 1}, Coor{7, 0}));
    board.Settle();
    ASSERT_EQ(16, board.GetScore(0));
    ASSERT_EQ(16, board.GetScore(1));
    ASSERT_EQ(16, board.GetChessCount(KingdomId(0)));
    ASSERT_EQ(16, board.GetChessCount(KingdomId(1)));
}

TEST(TestChineseChess, promote_zu)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{3, 0}, Coor{4, 0}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{4, 0}, Coor{5, 0}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{5, 0}, Coor{6, 0}));
}

TEST(TestChineseChess, promote_zu_cannot_move_at_immediately)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{3, 0}, Coor{4, 0}));
    ASSERT_SUCC(board.Move(1, 0, Coor{6, 0}, Coor{5, 0}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{4, 0}, Coor{5, 0}));
    board.Settle();
    ASSERT_FAIL(board.Move(0, 0, Coor{5, 0}, Coor{6, 0}));
}

TEST(TestChineseChess, chess_crash)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{3, 0}, Coor{4, 0}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{4, 0}, Coor{5, 0}));
    ASSERT_SUCC(board.Move(1, 0, Coor{6, 0}, Coor{5, 0}));
    board.Settle();
    board.Settle();
    ASSERT_EQ(15, board.GetScore(0));
    ASSERT_EQ(15, board.GetScore(1));
    ASSERT_EQ(15, board.GetChessCount(KingdomId(0)));
    ASSERT_EQ(15, board.GetChessCount(KingdomId(1)));
    ASSERT_FAIL(board.Move(0, 0, Coor{5, 0}, Coor{6, 0}));
    ASSERT_FAIL(board.Move(1, 0, Coor{5, 0}, Coor{4, 0}));
}

TEST(TestChineseChess, chess_eat_jiang_will_occupy)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{2, 1}, Coor{9, 1}));
    ASSERT_SUCC(board.Move(1, 0, Coor{9, 3}, Coor{8, 4}));
    board.Settle(); // p1's ma is ate
    ASSERT_EQ(17, board.GetScore(0));
    ASSERT_EQ(15, board.GetScore(1));
    ASSERT_EQ(16, board.GetChessCount(KingdomId(0)));
    ASSERT_EQ(15, board.GetChessCount(KingdomId(1)));
    ASSERT_TRUE((std::vector<KingdomId>{KingdomId(0)}) == board.GetUnreadyKingdomIds(0));
    ASSERT_TRUE((std::vector<KingdomId>{KingdomId(1)}) == board.GetUnreadyKingdomIds(1));
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{9, 1}, Coor{9, 4}));
    board.Settle();
    board.Settle();
    ASSERT_EQ(32, board.GetScore(0));
    ASSERT_EQ(0, board.GetScore(1));
    ASSERT_EQ(30, board.GetChessCount(KingdomId(0)));
    ASSERT_EQ(0, board.GetChessCount(KingdomId(1)));
    ASSERT_TRUE((std::vector<KingdomId>{KingdomId(0)}) == board.GetUnreadyKingdomIds(0));
    ASSERT_TRUE((std::vector<KingdomId>{}) == board.GetUnreadyKingdomIds(1));
    ASSERT_SUCC(board.Move(0, 0, Coor{9, 0}, Coor{8, 0})); // move occupied ju
}

TEST(TestChineseChess, jiang_eat_jiang_will_occupy)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{0, 4}, Coor{1, 4}));
    ASSERT_SUCC(board.Move(1, 0, Coor{9, 4}, Coor{8, 4}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{1, 4}, Coor{1, 3}));
    ASSERT_SUCC(board.Move(1, 0, Coor{8, 4}, Coor{8, 3}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{1, 3}, Coor{8, 3})); // k0 jiang eat k1 jiang
    board.Settle();
    ASSERT_EQ(32, board.GetScore(0));
    ASSERT_EQ(0, board.GetScore(1));
    ASSERT_EQ(31, board.GetChessCount(KingdomId(0)));
    ASSERT_EQ(0, board.GetChessCount(KingdomId(1)));
    ASSERT_TRUE((std::vector<KingdomId>{KingdomId(0)}) == board.GetUnreadyKingdomIds(0));
    ASSERT_TRUE((std::vector<KingdomId>{}) == board.GetUnreadyKingdomIds(1));
    ASSERT_SUCC(board.Move(0, 0, Coor{9, 0}, Coor{8, 0})); // move occupied ju
}

TEST(TestChineseChess, jiang_crash_will_destroy)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{0, 4}, Coor{1, 4}));
    ASSERT_SUCC(board.Move(1, 0, Coor{9, 4}, Coor{8, 4}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{1, 4}, Coor{1, 3}));
    ASSERT_SUCC(board.Move(1, 0, Coor{8, 4}, Coor{8, 3}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{1, 3}, Coor{8, 3}));
    ASSERT_SUCC(board.Move(1, 0, Coor{8, 3}, Coor{8, 4}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(1, 0, Coor{8, 4}, Coor{8, 5}));
    board.Settle();
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{8, 3}, Coor{8, 4})); // move k0 jiang
    ASSERT_SUCC(board.Move(1, 0, Coor{8, 5}, Coor{8, 4})); // move k1 jiang
    board.Settle(); // crashed
    ASSERT_EQ(0, board.GetScore(0));
    ASSERT_EQ(0, board.GetScore(1));
    ASSERT_EQ(0, board.GetChessCount(KingdomId(0)));
    ASSERT_EQ(0, board.GetChessCount(KingdomId(1)));
    ASSERT_TRUE((std::vector<KingdomId>{}) == board.GetUnreadyKingdomIds(0));
    ASSERT_TRUE((std::vector<KingdomId>{}) == board.GetUnreadyKingdomIds(1));
    ASSERT_FAIL(board.Move(0, 0, Coor{0, 0}, Coor{1, 0})); // ju becomes yong, cannot be moved
}

TEST(TestChineseChess, switch_board)
{
    BoardMgr board(2, 2);
    // 0 - 2
    // 1 - 3
    ASSERT_SUCC(board.Move(1, 0, Coor{9, 0}, Coor{8, 0}));
    ASSERT_TRUE((std::vector<KingdomId>{KingdomId(3)}) == board.GetUnreadyKingdomIds(1));
    board.Settle();
    board.Switch();
    // 0 - 3
    // 2 - 1
    ASSERT_SUCC(board.Move(1, 0, Coor{9, 0}, Coor{8, 0}));
    ASSERT_TRUE((std::vector<KingdomId>{KingdomId(2)}) == board.GetUnreadyKingdomIds(1));
    board.Settle();
    board.Switch();
    // 0 - 1
    // 3 - 2
    ASSERT_SUCC(board.Move(0, 0, Coor{9, 0}, Coor{8, 0}));
    ASSERT_TRUE((std::vector<KingdomId>{KingdomId(0)}) == board.GetUnreadyKingdomIds(0));
}

TEST(TestChineseChess, cannot_move_one_kingdom_chess_twice)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{0, 0}, Coor{1, 0}));
    ASSERT_FAIL(board.Move(0, 0, Coor{0, 8}, Coor{1, 8}));
}

TEST(TestChineseChess, eat_each_jiang_will_destroy)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Move(0, 0, Coor{2, 1}, Coor{9, 1})); // k0 pao eat k1 ma
    ASSERT_SUCC(board.Move(1, 0, Coor{7, 1}, Coor{0, 1})); // k1 pao eat k0 ma
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{0, 3}, Coor{1, 4})); // move shi
    ASSERT_SUCC(board.Move(1, 0, Coor{9, 3}, Coor{8, 4})); // move shi
    board.Settle();
    ASSERT_SUCC(board.Move(0, 0, Coor{9, 1}, Coor{9, 4})); // k0 pao eat k1 jiang
    ASSERT_SUCC(board.Move(1, 0, Coor{0, 1}, Coor{0, 4})); // k1 pao eat k2 jiang
    board.Settle();
    ASSERT_EQ(2, board.GetScore(0)); // eat ma and jiang
    ASSERT_EQ(2, board.GetScore(1)); // eat ma and jiang
    ASSERT_EQ(0, board.GetChessCount(KingdomId(0)));
    ASSERT_EQ(0, board.GetChessCount(KingdomId(1)));
    ASSERT_TRUE((std::vector<KingdomId>{}) == board.GetUnreadyKingdomIds(0));
    ASSERT_TRUE((std::vector<KingdomId>{}) == board.GetUnreadyKingdomIds(1));
    ASSERT_FAIL(board.Move(0, 0, Coor{0, 0}, Coor{1, 0})); // ju becomes yong, cannot be moved
}

TEST(TestChineseChess, pass_kingdom)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Pass(0, KingdomId(0)));
}

TEST(TestChineseChess, cannot_pass_other_player_kingdom)
{
    BoardMgr board(2, 1);
    ASSERT_FAIL(board.Pass(0, KingdomId(1)));
}

TEST(TestChineseChess, cannot_pass_not_exist_kingdom)
{
    BoardMgr board(2, 1);
    ASSERT_FAIL(board.Pass(0, KingdomId(2)));
}

TEST(TestChineseChess, cannot_move_after_pass)
{
    BoardMgr board(2, 1);
    ASSERT_SUCC(board.Pass(0, KingdomId(0)));
    ASSERT_FAIL(board.Move(0, 0, Coor{0, 0}, Coor{1, 0}));
}

