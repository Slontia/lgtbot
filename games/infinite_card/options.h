// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

EXTEND_OPTION("每个阶段的思考时间（秒）", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("双方的初始金币数", 金币, (ArithChecker<uint32_t>(10, 500, "金币")), 30)
EXTEND_OPTION("回合数上限，达到上限时金币较多的一方获胜", 回合上限, (ArithChecker<uint32_t>(3, 100, "回合")), 30)
EXTEND_OPTION("逐张出牌：改为每小局单独选牌，而非回合开始时一次排好四张", 逐张, (BoolChecker("开启", "关闭")), false)
EXTEND_OPTION("随机种子", 种子, (AnyArg("种子", "我是随便输入的一个字符串")), "")
