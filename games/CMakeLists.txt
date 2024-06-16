cmake_minimum_required(VERSION 3.0)

project(Games LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)

file(GLOB GAME_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/*)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

if (WITH_GLOG)
  find_package(glog REQUIRED)
  add_definitions(-DWITH_GLOG)
  if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    list(APPEND GAME_THIRD_PARTIES glog) # bot_core has already link glog
  endif()
endif()

foreach (GAME_DIR ${GAME_DIRS})
  if (IS_DIRECTORY ${GAME_DIR})

    string(REGEX REPLACE ".*/\(.*\)" "\\1" GAME ${GAME_DIR})

    message("Found game ${GAME} in ${GAME_DIR}")

    set(GAME_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugins/${GAME})
    set(GAME_RESOURCE_PATH ${GAME_OUTPUT_PATH}/resource)

    add_custom_target(make_output_dir_${GAME} ALL
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${GAME_OUTPUT_PATH})

    add_custom_target(resource_dir_${GAME} ALL
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${GAME_DIR}/resource
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${GAME_DIR}/resource ${GAME_RESOURCE_PATH}
      COMMAND ${CMAKE_COMMAND} -E copy ${GAME_DIR}/icon.png ${GAME_OUTPUT_PATH}/icon.png)

    set(SOURCE_FILES
      ${GAME_DIR}/mygame.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/game_main.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/resource_loader.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/stage.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/stage_utility.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/player_ready_masker.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/../utility/html.cc
    )

    set(RULE_BINARY ${CMAKE_CURRENT_BINARY_DIR}/${GAME}_rule.o)

    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
      execute_process(
        COMMAND bash -c "cd ${GAME_DIR} && printf 0x%x `stat -c %s rule.md | awk '{print $1 + 16}'`"
        OUTPUT_VARIABLE RULE_SIZE)
      message(STATUS "game_dir=${GAME_DIR}")
      message(STATUS "rule_size=${RULE_SIZE} ")
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
        add_custom_target(${GAME}_rule_binary ALL
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
          COMMAND cd ${GAME_DIR} && aarch64-linux-gnu-objcopy --input binary --output elf64-littleaarch64 --binary-architecture aarch64 --rename-section .data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA --pad-to ${RULE_SIZE} rule.md ${RULE_BINARY}
          COMMENT "Build rule binary"
        )
      else()
        add_custom_target(${GAME}_rule_binary ALL
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
          COMMAND cd ${GAME_DIR} && objcopy --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 --rename-section .data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA --pad-to ${RULE_SIZE} rule.md ${RULE_BINARY}
          COMMENT "Build rule binary"
        )
      endif()
    endif()

    # Make game library
    add_library(${GAME} SHARED ${SOURCE_FILES})
    target_compile_definitions(${GAME} PUBLIC
      GAME_ACHIEVEMENT_FILENAME="achievements.h"
      GAME_OPTION_FILENAME="options.h"
      GAME_MODULE_NAME=${GAME})
    target_include_directories(${GAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${GAME})
    target_link_libraries(${GAME} ${GAME_THIRD_PARTIES})
    set_target_properties(${GAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${GAME_OUTPUT_PATH}
      OUTPUT_NAME "libgame"
      PREFIX "")

    if (CMAKE_SYSTEM_NAME MATCHES "Windows")
      configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/resource.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/resource_${GAME}.rc")
      target_sources(${GAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/resource_${GAME}.rc")
    endif()

    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
      add_dependencies(${GAME} ${GAME}_rule_binary)
      target_link_libraries(${GAME} ${RULE_BINARY})
    endif()

    if (WITH_TEST)
      enable_testing()
      find_package(GTest REQUIRED)

      # Make library for tests
      add_library(${GAME}_for_test_lib OBJECT ${SOURCE_FILES})
      target_compile_definitions(${GAME}_for_test_lib PUBLIC
        GAME_ACHIEVEMENT_FILENAME="achievements.h"
        GAME_OPTION_FILENAME="options.h"
        GAME_MODULE_NAME=${GAME}
        TEST_BOT)
      target_include_directories(${GAME}_for_test_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${GAME})
      target_link_libraries(${GAME}_for_test_lib ${GAME_THIRD_PARTIES})

      # Make unit test
      add_executable(test_game_${GAME} ${GAME_DIR}/unittest.cc $<TARGET_OBJECTS:${GAME}_for_test_lib>)
      target_compile_definitions(test_game_${GAME} PUBLIC
        GAME_ACHIEVEMENT_FILENAME="achievements.h"
        GAME_OPTION_FILENAME="options.h"
        GAME_MODULE_NAME=${GAME}
        TEST_BOT)
      target_include_directories(test_game_${GAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${GAME})
      target_link_libraries(test_game_${GAME} glog gflags GTest::GTest GTest::Main ${GAME_THIRD_PARTIES})
      target_compile_definitions(test_game_${GAME} PUBLIC TEST_BOT)
      add_test(NAME test_game_${GAME} COMMAND test_game_${GAME} --resource_dir "${GAME_RESOURCE_PATH}")

      # Make run-game test
      add_executable(run_game_${GAME} ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/run_game.cc $<TARGET_OBJECTS:${GAME}_for_test_lib>)
      target_compile_definitions(run_game_${GAME} PUBLIC
        GAME_ACHIEVEMENT_FILENAME="achievements.h"
        GAME_OPTION_FILENAME="options.h"
        GAME_MODULE_NAME=${GAME}
        TEST_BOT)
      target_include_directories(run_game_${GAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${GAME})
      target_link_libraries(run_game_${GAME} glog gflags ${GAME_THIRD_PARTIES})
      target_compile_definitions(run_game_${GAME} PUBLIC TEST_BOT)
      add_test(NAME run_game_${GAME} COMMAND run_game_${GAME} --resource_dir "${GAME_RESOURCE_PATH}" --repeat=100)

      if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        add_dependencies(test_game_${GAME} ${GAME}_rule_binary)
        add_dependencies(run_game_${GAME} ${GAME}_rule_binary)
        target_link_libraries(test_game_${GAME} ${RULE_BINARY})
        target_link_libraries(run_game_${GAME} ${RULE_BINARY})
      endif()
    endif()

    include(${GAME_DIR}/option.cmake)

  endif()
endforeach()
