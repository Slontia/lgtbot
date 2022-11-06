EXTEND_OPTION("每回合落子时间", 时限, (ArithChecker<uint32_t>(10, 300, "时间（秒）")), 90)
EXTEND_OPTION("多少次碰撞发生后，游戏平局（0 视为碰撞不会造成平局）", 碰撞上限, (ArithChecker<uint32_t>(0, 100, "碰撞次数")), 10)
EXTEND_OPTION("多少回合后，游戏平局（0 视为棋盘下满才会平局）", 回合上限, (ArithChecker<uint32_t>(0, 120, "回合数")), 50)
