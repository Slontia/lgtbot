EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 90)
EXTEND_OPTION("游戏需要完成的目标距离", 目标, (ArithChecker<uint32_t>(30, 300, "距离")), 100)
EXTEND_OPTION("每个玩家开局的初始上限", 上限, (ArithChecker<uint32_t>(5, 20, "初始上限")), 10)
