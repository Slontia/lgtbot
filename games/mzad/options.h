EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("目标分数", 目标分数, (ArithChecker<uint32_t>(10, 100, "目标分数")), 25)
EXTEND_OPTION("开启后刺杀指定刺杀3可淘汰反制，关闭则反制必定成功", 刺杀反制, (BoolChecker("开启", "关闭")), false)
