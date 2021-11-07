cmake_minimum_required(VERSION 3.11)
project(BotUnittest LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)

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

add_executable(test_poker test_poker.cc)
target_link_libraries(test_poker ${THIRD_PARTIES})
add_test(NAME test_poker COMMAND test_poker)

add_executable(test_numcomb test_numcomb.cc ../utility/html.cpp)
target_link_libraries(test_numcomb ${THIRD_PARTIES})
add_test(NAME test_numcomb COMMAND test_numcomb)

add_executable(test_alchemist test_alchemist.cc ../utility/html.cpp)
target_link_libraries(test_alchemist ${THIRD_PARTIES})
add_test(NAME test_alchemist COMMAND test_alchemist)
