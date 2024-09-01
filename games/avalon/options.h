#ifdef INIT_OPTION_DEPEND

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

enum class LancelotMode { disable, implicit_three_rounds, explicit_five_rounds, recognition };

}

}

}

#else

EXTEND_OPTION("每回合投票时间限制", 投票时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 60)
EXTEND_OPTION("每回合队长组队时间限制", 组队时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 300)
EXTEND_OPTION("兰斯洛特扩展的游戏模式（具体含义请参考规则）", 兰斯洛特模式, (AlterChecker<LancelotMode>(std::map<std::string, LancelotMode>{
                { "禁用", LancelotMode::disable },
                { "五选三", LancelotMode::implicit_three_rounds },
                { "七选五", LancelotMode::explicit_five_rounds },
                { "互通身份", LancelotMode::recognition },
            })), LancelotMode::explicit_five_rounds)
EXTEND_OPTION("启用湖中仙女的最小玩家数（配置为 11 代表任何情况都不启用湖中仙女）", 湖中仙女人数, (ArithChecker<uint32_t>(5, 11, "玩家数")), 7)
EXTEND_OPTION("是否启用王者之剑扩展", 王者之剑, (BoolChecker("开启", "关闭")), true)

#ifdef TEST_BOT
EXTEND_OPTION("是否关闭一切随机", 测试模式, (BoolChecker("开启", "关闭")), true)
#endif

#endif
