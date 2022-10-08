GAME_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(1, 20, "回合数")), 7)
GAME_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
GAME_OPTION("测试", 测试, (ArithChecker<uint32_t>(0, 999, "测试")), 0)
