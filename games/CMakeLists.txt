cmake_minimum_required(VERSION 3.0)

project(Games LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugins)
file(GLOB GAME_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/*)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

foreach (GAME_DIR ${GAME_DIRS})
  if (IS_DIRECTORY ${GAME_DIR})

    string(REGEX REPLACE ".*/\(.*\)" "\\1" GAME ${GAME_DIR})

    message("Found game ${GAME} in ${GAME_DIR}")

    set(SOURCE_FILES
      ${GAME_DIR}/mygame.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/game_main.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/resource_loader.cpp
    )

    set(RULE_BINARY ${CMAKE_CURRENT_BINARY_DIR}/${GAME}_rule.o)

    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
      add_custom_target(${GAME}_rule ALL
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND cd ${GAME_DIR} && objcopy --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 --rename-section .data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA rule.txt ${RULE_BINARY}
        COMMENT "Build rule"
      )
    endif()

    add_library(${GAME} SHARED ${SOURCE_FILES})

    target_include_directories(${GAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${GAME})

    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
      add_dependencies(${GAME} ${GAME}_rule)
      target_link_libraries(${GAME} ${RULE_BINARY})
    endif()

    if (WITH_TEST)
      enable_testing()
      find_package(GTest REQUIRED)

      add_executable(test_game_${GAME} ${SOURCE_FILES} ${GAME_DIR}/unittest.cc)
      target_link_libraries(test_game_${GAME} GTest::GTest GTest::Main)
      target_include_directories(test_game_${GAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${GAME})
      add_test(NAME test_game_${GAME} COMMAND test_game_${GAME})

      add_executable(run_game_${GAME} ${SOURCE_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/run_game.cpp)
      target_include_directories(run_game_${GAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${GAME})
      target_link_libraries(run_game_${GAME} gflags)

      if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        add_dependencies(test_game_${GAME} ${GAME}_rule)
        target_link_libraries(test_game_${GAME} ${RULE_BINARY})
        target_link_libraries(run_game_${GAME} ${RULE_BINARY})
      endif()
    endif()

  endif()
endforeach()
