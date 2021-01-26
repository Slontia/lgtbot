cmake_minimum_required(VERSION 2.8)
project(BotCore LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR})
add_subdirectory(BotCore)
add_subdirectory(Simulator)

set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/games)
file(GLOB GAMES plugins/*)
foreach (GAME ${GAMES})
  if (IS_DIRECTORY ${GAME})
    add_subdirectory(${GAME})
  endif()
endforeach()
