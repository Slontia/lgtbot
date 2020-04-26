GAME_OPTION("设置胜利局数", win_count, (std::make_unique<ArithChecker<uint32_t, 1, 9>>("胜利局数")), 3)
