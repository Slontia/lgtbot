EXTEND_OPTION("每回合的时间限制", 时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 180)
EXTEND_OPTION("设置游戏地图边长", 边长, (ArithChecker<uint32_t>(5, 19, "边长")), 5)
EXTEND_OPTION("玩家移动步数上限", 步数, (ArithChecker<uint32_t>(1, 10, "步数")), 3)
