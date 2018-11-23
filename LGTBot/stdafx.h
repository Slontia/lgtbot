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

using std::string;
using std::vector;
using std::map;


/*
接下来要考虑的事情：
1. 对异常的处理，游戏中的异常应当导致游戏的中止，且主程序不发生崩溃
2. Time Up的处理，目前AtomState没有TimerHandle（不管了），且TimerHandle的行为应当包括中止子状态（设置两个函数）
3. 获取父状态信息的时候无法类型转换，可以考虑使用boost（不管了）
*/


/*
1. 在Container中添加主阶段建立的函数
两种阶段：一种为模板，即需要操作父阶段中成员；一种为普通类。只有普通类可以成为主阶段，需要在Container中绑定ID。

加锁，考虑释放阶段时的线程安全性
运行游戏前检测游戏合法性：是否存在主阶段
*/