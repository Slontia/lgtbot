EXTEND_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(1, 100, "回合数")), 30)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 90)
EXTEND_OPTION("棋盘尺寸", 尺寸, (ArithChecker<uint32_t>(6, 10, "边长")), 8)
EXTEND_OPTION("奖励格所占百分比", 奖励格百分比, (ArithChecker<uint32_t>(0, 50, "比例")), 5)
