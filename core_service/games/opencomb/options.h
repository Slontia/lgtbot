EXTEND_OPTION("每回合超时时间x秒", 局时, (ArithChecker<uint32_t>(10, 3600, "局时（秒）")), 120)
EXTEND_OPTION("随机种子（关联卡池&道具，配置需一致）", 种子, (OptionalDefaultChecker<AnyArg>("", "种子", "我是随便输入的一个字符串")), "")
EXTEND_OPTION("游戏卡池：决定卡池中砖块的种类", 卡池, AlterChecker<uint32_t>({{"经典", 0}, {"癞子", 1}, {"空气", 2}, {"混乱", 3}}), 0)
EXTEND_OPTION("游戏模式：游戏采用的基础放置和对战规则", 模式, AlterChecker<uint32_t>({{"传统", 0}, {"云顶", 1}}), 0)
EXTEND_OPTION("开启连线额外得分（长度3及以上时获得额外分数）", 连线奖励, (BoolChecker("开启", "关闭")), true)
EXTEND_OPTION("游戏道具：设置是否在卡池中添加特殊道具", 道具, (BoolChecker("开启", "关闭")), true)
EXTEND_OPTION("游戏地图", 地图, AlterChecker<uint32_t>(
    {{"随机", 0}, {"经典", 1}, {"环巢", 2}, {"漩涡", 3}, {"飞机", 4}, {"面具", 5}, {"塔楼", 6}, {"三叶草", 7}}
), 1)