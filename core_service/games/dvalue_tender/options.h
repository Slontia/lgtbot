EXTEND_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(3, 20, "回合数")), 9)
EXTEND_OPTION("金币", 金币, (ArithChecker<uint32_t>(10, 1000000, "金币")), 30)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 90)
