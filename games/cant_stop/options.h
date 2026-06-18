EXTEND_OPTION("配置移动变体规则（跳跃 / 强制运动）", 变体, (AlterChecker<uint32_t>({{"无", 0}, {"跳跃", 1}, {"强制运动", 2}})), 0)
EXTEND_OPTION("每回合行动的决策时限", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 60)
EXTEND_OPTION("获得游戏胜利需要夺得的列数", 胜利列数, (ArithChecker<uint32_t>(1, 5, "列数")), 3)
EXTEND_OPTION("回合数上限（达到上限则按夺列数与进度结算名次）", 回合数, (ArithChecker<uint32_t>(20, 500, "回合数")), 100)
