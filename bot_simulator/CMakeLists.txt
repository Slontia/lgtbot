cmake_minimum_required(VERSION 3.0)
project(Simulator LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/simulator.cc)
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(SOURCE_FILES ${SOURCE_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/linenoise/linenoise.c)
endif()

find_package(gflags REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

add_executable(simulator ${SOURCE_FILES})
target_link_libraries(simulator bot_core_static gflags)

