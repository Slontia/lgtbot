GAME_OPTION("设置胜利局数", 胜利局数, (std::make_unique<ArithChecker<uint32_t, 1, 9>>("胜利局数")), 3)
GAME_OPTION("每回合最长时间（秒）", 局时, (std::make_unique<ArithChecker<uint32_t, 10, 300>>("局时（秒）")), 60)
