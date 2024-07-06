EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("随机种子", 种子, (AnyArg("种子", "我是随便输入的一个字符串")), "")
#ifdef TEST_BOT
EXTEND_OPTION("洗牌", 洗牌, (BoolChecker("开启", "关闭")), true)
#endif
