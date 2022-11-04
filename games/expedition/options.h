GAME_OPTION("时限", 时限, (ArithChecker<uint32_t>(20, 300, "时间限制（秒）")), 128)
GAME_OPTION("地图", 地图,
            AlterChecker<int>(
                {{"随机", 0}, {"王国边境", 1}, {"蜂巢之血", 2}, {"水晶洞穴", 3}, {"富饶之地", 4}}),
            0)