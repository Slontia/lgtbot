cmake_minimum_required(VERSION 2.8)
project(Simulator LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

set(SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/simulator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/linenoise/linenoise.c
)

find_package(gflags REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

add_executable(simulator ${SOURCE_FILES})
target_link_libraries(simulator game_core_static gflags)

