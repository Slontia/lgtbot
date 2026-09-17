// EXTEND_OPTION 展开时依赖 cloudcomb.h 中的 MakeSpecialEventOptionMap / ScoreMode / SelectOrder
#ifdef INIT_OPTION_DEPEND
#include "cloudcomb.h"
#endif

EXTEND_OPTION("每回合最长时间x秒", 局时, (ArithChecker<uint32_t>(10, 3600, "局时（秒）")), 120)
EXTEND_OPTION("初始血量", 血量, (ArithChecker<uint32_t>(50, 1000, "血量")), 150)
EXTEND_OPTION("终局计分方式（排名：按淘汰名次结算；分数：直接按最终总分结算）", 计分,
        (AlterChecker<ScoreMode>(MakeScoreModeOptionMap())), ScoreMode::排名)
EXTEND_OPTION("决定选牌先后顺序（顺位：按开局玩家顺序，且首轮按数字和升序发放）", 选牌顺序,
        (AlterChecker<SelectOrder>(MakeSelectOrderOptionMap())), SelectOrder::顺位)
EXTEND_OPTION("特殊事件", 事件, AlterChecker<int>(MakeSpecialEventOptionMap()), 0)
EXTEND_OPTION("随机种子", 种子, (OptionalDefaultChecker<AnyArg>("", "种子", "我是随便输入的一个字符串")), "")
