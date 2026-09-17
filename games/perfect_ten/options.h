// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

EXTEND_OPTION("每个步骤的思考时间（秒）", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("单个任务要求的最少张数", 任务张数下限, (ArithChecker<uint32_t>(2, 6, "张数")), 2)
EXTEND_OPTION("单个任务要求的最多张数", 任务张数上限, (ArithChecker<uint32_t>(2, 6, "张数")), 4)
EXTEND_OPTION("两个任务张数要求之和的下限", 任务总张数下限, (ArithChecker<uint32_t>(4, 11, "张数")), 5)
EXTEND_OPTION("两个任务张数要求之和的上限", 任务总张数上限, (ArithChecker<uint32_t>(4, 11, "张数")), 7)
EXTEND_OPTION("随机种子", 种子, (AnyArg("种子", "我是随便输入的一个字符串")), "")
