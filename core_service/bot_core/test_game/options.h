// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef EXTEND_OPTION
EXTEND_OPTION("时间限制", 时限, (ArithChecker<int>(0, 10)), 1)
EXTEND_OPTION("直接结束游戏", 直接结束, (OptionalDefaultChecker<BoolChecker>(true, "开启", "关闭")), false)
EXTEND_OPTION("最大玩家数（0表示无限制）", 最大玩家数, (ArithChecker<uint64_t>(0, 100)), 4)
#endif
