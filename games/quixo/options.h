GAME_OPTION("每每手棋x秒超时", 局时, (ArithChecker<uint32_t>(10, 3600, "局时（秒）")), 120)
GAME_OPTION("最大回合数（两名玩家各下一次算一回合）", 回合数, (ArithChecker<uint32_t>(10, 100, "回合数")), 30)
