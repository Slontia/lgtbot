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

function (make_test arg)
    list(GET ARGV 0 test_name)
    set(SRCS ${test_name}.cc)
    set(i 1)
    while(i LESS ${ARGC})
         list(GET ARGV ${i} file)
         set(SRCS ${SRCS} ${file})
         math(EXPR i "${i} + 1")
    endwhile()
    add_executable(${test_name} ${SRCS})
    target_compile_definitions(${test_name} PUBLIC TEST_BOT)
    target_link_libraries(${test_name} ${THIRD_PARTIES})
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction ()

make_test(test_poker)
make_test(test_numcomb ../utility/html.cc)
make_test(test_alchemist ../utility/html.cc)
make_test(test_quixo ../utility/html.cc)
make_test(test_mahjong_17_steps ../utility/html.cc)
make_test(test_renju ../utility/html.cc)
make_test(test_laser_chess ../utility/html.cc)
target_link_libraries(test_mahjong_17_steps Mahjong MahjongAlgorithm)
add_dependencies(test_mahjong_17_steps Mahjong MahjongAlgorithm)
make_test(test_bet_pool)
