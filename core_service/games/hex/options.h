EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 180)
EXTEND_OPTION("棋盘边长", 边长, (ArithChecker<uint32_t>(7, 19, "长度")), 11)
