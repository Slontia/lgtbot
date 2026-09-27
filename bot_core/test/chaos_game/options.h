// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef EXTEND_OPTION
EXTEND_OPTION("时间限制（0 表示不设限制）", 时限, (ArithChecker<int>(0, 10)), 2)
EXTEND_OPTION("最大玩家数（0 表示无限制）", 最大玩家数, (ArithChecker<uint64_t>(0, 100)), 8)
EXTEND_OPTION("拒绝开始", 拒绝开始, (OptionalDefaultChecker<BoolChecker>(true, "开启", "关闭")), false)
EXTEND_OPTION("随机种子", 随机种子, (ArithChecker<uint64_t>(0, 4294967295)), 0)
EXTEND_OPTION("淘汰概率（百分比）", 淘汰概率, (ArithChecker<uint64_t>(0, 100)), 10)
EXTEND_OPTION("挂机概率（百分比）", 挂机概率, (ArithChecker<uint64_t>(0, 100)), 10)
EXTEND_OPTION("重计时概率（百分比）", 重计时概率, (ArithChecker<uint64_t>(0, 100)), 10)
EXTEND_OPTION("成就概率（百分比）", 成就概率, (ArithChecker<uint64_t>(0, 100)), 10)
EXTEND_OPTION("加分概率（百分比）", 加分概率, (ArithChecker<uint64_t>(0, 100)), 10)
EXTEND_OPTION("直接结束概率（百分比）", 直接结束概率, (ArithChecker<uint64_t>(0, 100)), 10)
EXTEND_OPTION("失败概率（百分比）", 失败概率, (ArithChecker<uint64_t>(0, 100)), 10)
EXTEND_OPTION("崩溃概率（百分比）", 崩溃概率, (ArithChecker<uint64_t>(0, 100)), 0)
#endif