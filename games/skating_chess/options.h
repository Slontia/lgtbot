EXTEND_OPTION("每回合行动时限，超时判负", 时限, (ArithChecker<uint32_t>(30, 3600, "时限（秒）")), 120)
EXTEND_OPTION("最大回合数（双方各行动一次算一回合），达到上限判和", 回合数, (ArithChecker<uint32_t>(20, 200, "回合数")), 60)
