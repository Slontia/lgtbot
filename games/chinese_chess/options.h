GAME_OPTION("每回合超时时间", 时限, (ArithChecker<uint32_t>(10, 3600, "时限（秒）")), 180)
GAME_OPTION("[X] 回合后，连续 Y 回合未发生吃子或碰撞", 最小回合限制, (ArithChecker<uint32_t>(10, 100, "回合数")), 20)
GAME_OPTION("X 回合后，连续 [Y] 回合未发生吃子或碰撞", 和平回合限制, (ArithChecker<uint32_t>(1, 20, "回合数")), 1)
GAME_OPTION("每 [Z] 回合切换一次棋盘", 切换回合, (ArithChecker<uint32_t>(1, 20, "回合数")), 5)
GAME_OPTION("每名玩家初始控制阵营数", 阵营, (ArithChecker<uint32_t>(1, 3, "阵营数")), 2)

