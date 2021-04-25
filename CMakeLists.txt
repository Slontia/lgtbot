cmake_minimum_required(VERSION 2.8)
project(BotCore LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR})
add_subdirectory(bot_core)
add_subdirectory(bot_simulator)
add_subdirectory(bot_unittest)
add_subdirectory(game_util)

set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/plugins)
file(GLOB GAMES ${CMAKE_CURRENT_SOURCE_DIR}/games/*)
foreach (GAME ${GAMES})
  message("Found game ${GAME}")
  if (IS_DIRECTORY ${GAME})
    add_subdirectory(${GAME})
  endif()
endforeach()
