GAME_OPTION("每回合最长时间x秒", 局时, (ArithChecker<uint32_t>(10, 3600, "局时（秒）")), 120)
GAME_OPTION("随机种子", 种子, (AnyArg("种子", "我是随便输入的一个字符串")), "")
GAME_OPTION("回合数，即放置数字的数量", 回合数, (ArithChecker<uint32_t>(10, 20, "回合数")), 20)
GAME_OPTION("对于洗好的56个砖块，若前X个均不是癞子，则跳过这X个砖块（用于提升游戏中癞子出现的概率）", 跳过非癞子, (ArithChecker<uint32_t>(0, 46, "数量")), 20)
