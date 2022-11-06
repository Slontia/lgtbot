EXTEND_OPTION("等式长度", 等式长度, (ArithChecker<uint32_t>(5, 9, "长度")), 7)
EXTEND_OPTION("游戏模式", 游戏模式, (BoolChecker("标准", "狂野")), true)
EXTEND_OPTION("时限", 时限, (ArithChecker<uint32_t>(20, 300, "时间限制（秒）")), 180)