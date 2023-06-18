#ifdef INIT_OPTION_DEPEND

#include "game_util/poker.h"

#else

EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<int32_t>(10, 3600, "超时时间（秒）")), 60)
EXTEND_OPTION("各玩家初始筹码数", 筹码, (ArithChecker<int32_t>(10, 10000, "筹码数")), 1000)
EXTEND_OPTION("最大游戏局数", 局数, (ArithChecker<int32_t>(1, 20, "局数")), 15)
EXTEND_OPTION("幸存玩家数不多于几人时游戏结束", 幸存, (ArithChecker<int32_t>(1, 15, "玩家数")), 1)
EXTEND_OPTION("底注增长间隔局数", 底注变化局数, (ArithChecker<int32_t>(1, 10, "局数")), 1)
EXTEND_OPTION("底注变化筹码数", 底注变化, (RepeatableChecker<ArithChecker<int32_t>>(1, 10000, "筹码数")),
        (std::vector<int32_t>{5, 10, 20, 30, 40, 60, 80, 100, 150, 200, 300, 400, 600, 800, 1000}))
EXTEND_OPTION("随机种子", 种子, (AnyArg("种子", "我是随便输入的一个字符串")), "")
EXTEND_OPTION("使用的卡牌类型", 卡牌, (AlterChecker<game_util::poker::CardType>(std::map<std::string, game_util::poker::CardType>{
                { "波卡", game_util::poker::CardType::BOKAA },
                { "扑克", game_util::poker::CardType::POKER },
            })), game_util::poker::CardType::BOKAA)

#endif
