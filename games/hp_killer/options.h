#include "occupation.h"
EXTEND_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(1, 100, "回合数")), 20)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 300)
EXTEND_OPTION("初始血量", 血量, (ArithChecker<int32_t>(1, 100, "HP 值")), 100)
EXTEND_OPTION("初始治愈次数", 治愈次数, (ArithChecker<int32_t>(0, 10, "次数")), 3)
EXTEND_OPTION("【仅测试用】依次设置玩家身份", 身份列表, (RepeatableChecker<EnumChecker<Occupation>>()), std::vector<Occupation>{})
