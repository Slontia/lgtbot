EXTEND_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(1, 20, "回合数")), 8)
EXTEND_OPTION("初始 Chips 数量", Chips, (ArithChecker<uint32_t>(1, 20, "数量")), 8)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 180)
