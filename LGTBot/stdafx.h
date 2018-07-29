// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>



// TODO: reference additional headers your program requires here
#include "stdint.h"
#include "string"
#include <atomic>
#include <vector>
#include <map>
#include <functional>
#include <Windows.h>
#include <time.h>

/* Mysql Connector */
#include <mysql_connection.h>
#include <mysql_driver.h>

#include "log.h"
#include "game_state.h"
#include "game.h"
#include "game_container.h"
#include "player.h"
#include "match.h"
#include "time_trigger.h"
#include "db_manager.h"

using std::string;
using std::vector;
using std::map;


/*
接下来要考虑的事情：
1. 对异常的处理，游戏中的异常应当导致游戏的中止，且主程序不发生崩溃
2. Time Up的处理，目前AtomState没有TimerHandle，且TimerHandle的行为应当包括中止子状态
3. 获取父状态信息的时候无法类型转换，可以考虑使用boost
*/