// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef EXTEND_OPTION

EXTEND_OPTION("计时器提示方式，私信提醒，或者群里公开 at 提醒", 计时公开提示, (BoolChecker("开启", "关闭")), false)

#elif !defined(BOT_CORE_OPTIONS_H)
#define BOT_CORE_OPTIONS_H

#define OPTION_CLASSNAME MutableBotOption
#define OPTION_FILENAME "bot_core/options.h"
#include "utility/extend_option.h"
#undef OPTION_CLASSNAME
#undef OPTION_FILENAME

#endif
