EXTEND_OPTION("局数", 局数, (ArithChecker<uint32_t>(1, 8, "局数")), 4)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 90)
EXTEND_OPTION("是否包含赤宝牌", 赤宝牌, (BoolChecker("开启", "关闭")), true)
EXTEND_OPTION("透明牌数量", 透明牌, (ArithChecker<uint32_t>(0, 3, "每种数量")), 0)
EXTEND_OPTION("随机种子", 种子, (AnyArg("种子", "我是随便输入的一个字符串")), "")
#ifdef TEST_BOT
EXTEND_OPTION("配牌（玩家 1 手牌 - 玩家 1 牌山 - 玩家 2 手牌 - 玩家 2 牌山 - ... - 宝牌 - 里宝牌）", 配牌, (AnyArg("136 张牌", "3m5s7p1z...")), "")
#endif
