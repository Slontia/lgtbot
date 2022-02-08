#ifdef INIT_OPTION_DEPEND
enum class GameMap { RANDOM, ACE, CURIOSITY, GRAIL, MERCURY, SOPHIE, GENIUS };
#else
GAME_OPTION("每每手棋x秒超时", 局时, (ArithChecker<uint32_t>(10, 3600, "局时（秒）")), 120)
GAME_OPTION("最大回合数（两名玩家各下一次算一回合）", 回合数, (ArithChecker<uint32_t>(10, 100, "回合数")), 30)
GAME_OPTION("游戏地图", 地图, (AlterChecker<GameMap>(std::map<std::string, GameMap>{{"随机", GameMap::RANDOM}, {"王牌", GameMap::ACE}, {"珍奇", GameMap::CURIOSITY}, {"法宝", GameMap::GRAIL}, {"活力", GameMap::MERCURY}, {"苏菲", GameMap::SOPHIE}, {"天才", GameMap::GENIUS}})), GameMap::RANDOM)
#endif
