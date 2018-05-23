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


#include "game_state.h"
#include "game.h"
#include "game_container.h"
#include "player.h"
#include "match.h"
#include "time_trigger.h"

using std::string;
using std::vector;
using std::map;
