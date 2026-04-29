EXTEND_OPTION("游戏棋盘边长", 边长, (ArithChecker<uint32_t>(3, 5, "长度")), 3)
EXTEND_OPTION("获胜所需的分数", 分数, (ArithChecker<uint32_t>(30, 600, "分数")), 60)
EXTEND_OPTION("游戏局数上限", 局数, (ArithChecker<uint32_t>(1, 20, "局数")), 20)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 90)
