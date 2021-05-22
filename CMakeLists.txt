cmake_minimum_required(VERSION 3.0)
project(BotCore LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR})

option(WITH_CORE "build with bot core" TRUE)
if (WITH_CORE)
  add_subdirectory(bot_core)
endif()

option(WITH_SIMULATOR "build with bot simulator" TRUE)
if (WITH_SIMULATOR)
  add_subdirectory(bot_simulator)
endif()

if (WITH_TEST)
  add_subdirectory(game_util) # only build for unittest
endif()

option(WITH_GAMES "build with games" TRUE)
if (WITH_GAMES)
  add_subdirectory(games)
endif()
