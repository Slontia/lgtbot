// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef INIT_OPTION_DEPEND
#include "talent_comb.h"
#endif

EXTEND_OPTION("每回合最长时间x秒", 局时, (ArithChecker<uint32_t>(10, 3600, "局时（秒）")), 120)
EXTEND_OPTION("初始血量", 血量, (ArithChecker<uint32_t>(50, 500, "血量")), 150)
EXTEND_OPTION("随机种子", 种子, (OptionalDefaultChecker<AnyArg>("", "种子", "我是随便输入的一个字符串")), "")
EXTEND_OPTION("特殊事件", 事件, AlterChecker<int>(MakeSpecialEventOptionMap()), 0)
#ifdef TEST_BOT
EXTEND_OPTION("【仅测试用】开局拥有天赋", 天赋, (RepeatableChecker<AlterChecker<int>>(MakeTalentOptionMap())), std::vector<int>{})
#endif
