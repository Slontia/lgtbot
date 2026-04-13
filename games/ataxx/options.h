EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 180)
EXTEND_OPTION("棋盘大小", 棋盘大小, (ArithChecker<uint32_t>(5, 10, "边长")), 7)
