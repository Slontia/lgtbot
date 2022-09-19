GAME_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(1, 20, "回合数")), 20)
GAME_OPTION("长度", 长度, (ArithChecker<uint32_t>(5, 8, "长度")), 0)
GAME_OPTION("高难", 高难, (ArithChecker<uint32_t>(0, 1, "高难")), 0)
GAME_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 180)
