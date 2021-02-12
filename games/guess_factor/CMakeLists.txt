cmake_minimum_required(VERSION 2.8)
string(REGEX REPLACE ".*/\(.*\)" "\\1" GAME ${CMAKE_CURRENT_SOURCE_DIR})

project(${GAME} LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

list(APPEND SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/mygame.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../game_framework/game.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../game_framework/game_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../game_framework/resource_loader.cpp
)

include_directories(./ ${CMAKE_CURRENT_SOURCE_DIR}/../../)

add_custom_target(${GAME}_rule ALL
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND objcopy --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 --rename-section .data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA rule.txt ${CMAKE_CURRENT_SOURCE_DIR}/rule.o
  COMMENT "Build rule"
)

add_library(${GAME} SHARED ${SOURCE_FILES})
add_dependencies(${GAME} ${GAME}_rule)
target_link_libraries(${GAME} ${CMAKE_CURRENT_SOURCE_DIR}/rule.o)

enable_testing()
find_package(GTest REQUIRED)
add_executable(unittest_${GAME} ${SOURCE_FILES} unittest.cc)
target_link_libraries(unittest_${GAME} GTest::GTest GTest::Main ${CMAKE_CURRENT_SOURCE_DIR}/rule.o)

