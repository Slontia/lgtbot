EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("目标分数", 目标分数, (ArithChecker<uint32_t>(10, 100, "目标分数")), 25)
