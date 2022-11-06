EXTEND_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(1, 50, "回合数")), 40)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 360, "超时时间（秒）")), 120)
