EXTEND_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(1, 25, "回合数")), 7)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("测试题目", 测试, (ArithChecker<uint32_t>(0, 999, "测试")), 0)
EXTEND_OPTION("增加特殊规则", 特殊规则, (ArithChecker<uint32_t>(0, 7, "特殊规则")), 0)
