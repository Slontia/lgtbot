cmake_minimum_required(VERSION 3.11)
project(BotUnittest LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND THIRD_PARTIES
    dl      # load shared libraries dynamicly on linux
    atomic
  )
endif()

find_package(gflags REQUIRED)
list(APPEND THIRD_PARTIES gflags)

enable_testing()
find_package(GTest REQUIRED)
list(APPEND THIRD_PARTIES GTest::GTest GTest::Main)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
add_executable(poker_unittest poker_unittest.cc)
target_link_libraries(poker_unittest ${THIRD_PARTIES} bot_core_static)
