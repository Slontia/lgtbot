EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(1, 15, "回合数")), 10)
EXTEND_OPTION("玩家初始金币数", 金币, (ArithChecker<uint32_t>(0, 20, "金币")), 10)
EXTEND_OPTION("开启特殊规则", 特殊规则, (AlterChecker<uint32_t>({{"无", 0}, {"狂热", 1}, {"毒雾", 2}, {"撒币", 3}})), 0)
