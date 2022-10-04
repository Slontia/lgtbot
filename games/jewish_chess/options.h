GAME_OPTION("棋盘大小", 棋盘大小, (ArithChecker<uint32_t>(3, 9, "边长")), 6)
GAME_OPTION("时限", 时限, (ArithChecker<uint32_t>(20, 300, "时间限制（秒）")), 120)