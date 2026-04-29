#include "occupation.h"
EXTEND_OPTION("回合数", 回合数, (ArithChecker<uint32_t>(1, 100, "回合数")), 20)
EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 300)
EXTEND_OPTION("初始血量", 血量, (ArithChecker<int32_t>(1, 100, "HP 值")), 100)
EXTEND_OPTION("初始治愈次数", 治愈次数, (ArithChecker<int32_t>(0, 10, "次数")), 3)
EXTEND_OPTION("杀手阵营知道全部平民阵营代号，但不知道杀手", 身份互通, (BoolChecker("开启", "关闭")), false)
EXTEND_OPTION("多数派平民需要声明「晚安」才可取得胜利", 晚安模式, (BoolChecker("开启", "关闭")), false)
EXTEND_OPTION("五人场身份列表（身份数量需为 5，否则实际采用默认身份列表）", 五人身份, (RepeatableChecker<EnumChecker<Occupation>>()), std::vector<Occupation>{})
EXTEND_OPTION("六人场身份列表（身份数量需为 6，否则实际采用默认身份列表）", 六人身份, (RepeatableChecker<EnumChecker<Occupation>>()), std::vector<Occupation>{})
EXTEND_OPTION("七人场身份列表（身份数量需为 7，否则实际采用默认身份列表）", 七人身份, (RepeatableChecker<EnumChecker<Occupation>>()), std::vector<Occupation>{})
EXTEND_OPTION("八人场身份列表（身份数量需为 8，否则实际采用默认身份列表）", 八人身份, (RepeatableChecker<EnumChecker<Occupation>>()), std::vector<Occupation>{})
EXTEND_OPTION("九人场身份列表（身份数量需为 9，否则实际采用默认身份列表）", 九人身份, (RepeatableChecker<EnumChecker<Occupation>>()), std::vector<Occupation>{})
#ifdef TEST_BOT
EXTEND_OPTION("【仅测试用】依次设置玩家身份", 身份列表, (RepeatableChecker<EnumChecker<Occupation>>()), std::vector<Occupation>{})
#endif
