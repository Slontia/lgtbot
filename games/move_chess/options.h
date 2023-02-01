EXTEND_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(30, 200, "回合数")), 70)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 180)
EXTEND_OPTION("棋盘边长", 边长, (ArithChecker<uint32_t>(9, 15, "边长")), 11)
