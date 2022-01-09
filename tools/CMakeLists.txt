cmake_minimum_required(VERSION 3.0)
project(Simulator LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)

find_package(gflags REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

# score updater
add_executable(score_updater ${CMAKE_CURRENT_SOURCE_DIR}/score_updater.cc ${CMAKE_CURRENT_SOURCE_DIR}/../bot_core/score_calculation.cc)
target_link_libraries(score_updater gflags SQLite::SQLite3)

# simulator
set(SIMULATOR_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/simulator.cc)
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(SIMULATOR_SOURCE_FILES ${SIMULATOR_SOURCE_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/linenoise/linenoise.c)
endif()
add_executable(simulator ${SIMULATOR_SOURCE_FILES})
target_link_libraries(simulator bot_core_static gflags)

