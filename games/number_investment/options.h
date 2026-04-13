EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("最大数字（同时决定回合数）", 范围, (ArithChecker<uint32_t>(5, 30, "最大数字")), 10)
EXTEND_OPTION("连续得分奖励分数", 奖励分数, (ArithChecker<uint32_t>(0, 5, "奖励分数")), 2)
