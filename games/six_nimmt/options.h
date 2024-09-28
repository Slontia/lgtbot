EXTEND_OPTION("出牌阶段时间限制（放置阶段固定为90秒）", 时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("游戏模式（可选：单局模式、22/33/66分模式，大胃王）", 模式, (AlterChecker<uint32_t>({{"单局", 0}, {"22分", 1}, {"33分", 2}, {"66分", 3}, {"大胃王", 4}})), 0)
EXTEND_OPTION("使用牌库的数字范围 1-?（不够时则拓展牌堆至本局需要的牌数）", 卡牌, (ArithChecker<uint32_t>(50, 999, "上限")), 104)
EXTEND_OPTION("每个玩家开局的手牌数", 手牌, (ArithChecker<uint32_t>(5, 20, "牌数")), 10)
EXTEND_OPTION("桌面上的牌阵行数", 行数, (ArithChecker<uint32_t>(2, 8, "行数")), 4)
EXTEND_OPTION("每行的最大容量上限（吃牌位置）", 上限, (ArithChecker<uint32_t>(3, 10, "牌数")), 6)
EXTEND_OPTION("自定义倍数对应的牛头数量（分别对应2、3、5、7、10）", 倍数, (RepeatableChecker<ArithChecker<int32_t>>(1, 1000, "倍数")), (std::vector<int32_t>{5, 10, 11, 55, 110}))
