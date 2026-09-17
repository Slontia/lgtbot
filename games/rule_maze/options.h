EXTEND_OPTION("准备与情报阶段的时间限制", 准备时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 180)
EXTEND_OPTION("每一步行动的时间限制", 行动时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("游戏最大回合数限制", 回合数, (ArithChecker<uint32_t>(5, 50, "回合数")), 20)
