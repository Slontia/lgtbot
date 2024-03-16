EXTEND_OPTION("N（回合数），6人以内默认为13，7及以上默认为 2x+1(x为玩家数)", N, (ArithChecker<uint32_t>(5, 30, "回合数")), 0)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
