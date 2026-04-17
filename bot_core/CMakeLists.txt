cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)

# protobuf: find_package in parent ../CMakeLists.txt (config and/or module).

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -latomic")
endif()

EXECUTE_PROCESS(
    COMMAND git describe --tags --always
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE MAGE_VERSION_GIT_HEAD_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE GET_GIT_VERSION_FAILED
)
if (GET_GIT_VERSION_FAILED)
    message("-- Get Git Describe failed, will sign unpublished version: ${GET_GIT_VERSION_FAILED}")
    set(MAGE_VERSION_GIT_HEAD_VERSION "<unpublished version>")
else()
    message("-- Current Git Describe: ${MAGE_VERSION_GIT_HEAD_VERSION}")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.cc.in" "${CMAKE_CURRENT_BINARY_DIR}/version.cc")

# GENERATED property set by protobuf_generate_cpp is only visible within its
# own directory scope (CMake < 3.20). Mark the files explicitly here so that
# CMake does not try to verify their existence at configure time.
set_source_files_properties(${MATCH_PROCESS_PROTO_SRCS} PROPERTIES GENERATED TRUE)

list(APPEND SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/bot_core.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/bot_ctx.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/db_manager.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/match.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/match_manager.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/message_handlers.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/msg_sender.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/score_calculation.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../utility/html.cc
  ${CMAKE_CURRENT_BINARY_DIR}/version.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/match_child_client.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/game_config_client.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/subprocess.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../match_process/ipc_frame.cc
  ${MATCH_PROCESS_PROTO_SRCS}
)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
  list(APPEND SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/dllmain.cc)
endif()

find_package(Threads REQUIRED)
list(APPEND THIRD_PARTIES Threads::Threads protobuf::libprotobuf)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND THIRD_PARTIES
    dl
    atomic
  )
endif()

find_package(gflags REQUIRED)
list(APPEND THIRD_PARTIES gflags)

add_library(bot_core_static STATIC ${SOURCE_FILES})
target_link_libraries(bot_core_static ${THIRD_PARTIES})
target_include_directories(bot_core_static PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${MATCH_PROCESS_PROTO_GEN_DIR}/..)
add_dependencies(bot_core_static match_game_runner config_runner)
target_compile_definitions(bot_core_static PRIVATE
  MATCH_GAME_RUNNER_PATH="$<TARGET_FILE:match_game_runner>"
  CONFIG_RUNNER_PATH="$<TARGET_FILE:config_runner>"
)

add_library(bot_core SHARED ${SOURCE_FILES})
target_link_libraries(bot_core ${THIRD_PARTIES})
target_include_directories(bot_core PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${MATCH_PROCESS_PROTO_GEN_DIR}/..)
add_dependencies(bot_core match_game_runner config_runner)
target_compile_definitions(bot_core PRIVATE
  MATCH_GAME_RUNNER_PATH="$<TARGET_FILE:match_game_runner>"
  CONFIG_RUNNER_PATH="$<TARGET_FILE:config_runner>"
)

if (WITH_TEST)
  find_package(GTest REQUIRED)
  list(APPEND THIRD_PARTIES GTest::GTest GTest::Main)

  set(TEST_GAME_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_game/mygame.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/game_main.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/resource_loader.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/stage.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/stage_utility.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/player_ready_masker.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../utility/html.cc
  )
  set(TEST_GAME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test_game)
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    execute_process(
      COMMAND bash -c "cd ${TEST_GAME_DIR} && printf 0x%x `stat -c %s rule.md | awk '{print $1 + 16}'`"
      OUTPUT_VARIABLE TEST_GAME_RULE_SIZE)
    set(TEST_GAME_RULE_BINARY ${CMAKE_CURRENT_BINARY_DIR}/test_game_rule.o)
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
      add_custom_target(test_game_rule_binary ALL
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND cd ${TEST_GAME_DIR} && aarch64-linux-gnu-objcopy --input binary --output elf64-littleaarch64 --binary-architecture aarch64 --rename-section .data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA --pad-to ${TEST_GAME_RULE_SIZE} rule.md ${TEST_GAME_RULE_BINARY}
        BYPRODUCTS ${TEST_GAME_RULE_BINARY}
        COMMENT "Build test_game rule binary")
    else()
      add_custom_target(test_game_rule_binary ALL
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND cd ${TEST_GAME_DIR} && objcopy --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 --rename-section .data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA --pad-to ${TEST_GAME_RULE_SIZE} rule.md ${TEST_GAME_RULE_BINARY}
        BYPRODUCTS ${TEST_GAME_RULE_BINARY}
        COMMENT "Build test_game rule binary")
    endif()
  endif()
  set(TEST_GAME_PLUGIN_PARENT_DIR ${CMAKE_BINARY_DIR}/test_game_plugins)
  set(TEST_GAME_PLUGIN_DIR ${TEST_GAME_PLUGIN_PARENT_DIR}/test_game)
  add_custom_target(make_test_game_plugin_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_GAME_PLUGIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/test_game/rule.md ${TEST_GAME_PLUGIN_DIR}/rule.md)
  add_library(test_game_plugin SHARED ${TEST_GAME_SOURCE_FILES})
  add_dependencies(test_game_plugin make_test_game_plugin_dir)
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_dependencies(test_game_plugin test_game_rule_binary)
    target_link_libraries(test_game_plugin ${TEST_GAME_RULE_BINARY})
  endif()
  target_compile_definitions(test_game_plugin PUBLIC
    GAME_ACHIEVEMENT_FILENAME="${CMAKE_CURRENT_SOURCE_DIR}/test_game/achievements.h"
    GAME_OPTION_FILENAME="${CMAKE_CURRENT_SOURCE_DIR}/test_game/options.h"
    GAME_MODULE_NAME=test_game
    TEST_BOT)
  target_include_directories(test_game_plugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_game
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
  # glog is linked into test_bot only; plugin resolves LOG symbols from the executable (ENABLE_EXPORTS).
  set_target_properties(test_game_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TEST_GAME_PLUGIN_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${TEST_GAME_PLUGIN_DIR}
    OUTPUT_NAME "libgame"
    PREFIX "")
  if (APPLE)
    # macOS requires explicit permission for unresolved symbols in dylibs.
    target_link_options(test_game_plugin PRIVATE -undefined dynamic_lookup)
  endif()

  add_executable(test_bot
    test_bot.cc
    ${SOURCE_FILES})
  target_link_libraries(test_bot ${THIRD_PARTIES})
  target_include_directories(test_bot PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${MATCH_PROCESS_PROTO_GEN_DIR}/..)
  add_dependencies(test_bot match_game_runner config_runner test_game_plugin)
  target_compile_definitions(test_bot PRIVATE
    MATCH_GAME_RUNNER_PATH="$<TARGET_FILE:match_game_runner>"
    CONFIG_RUNNER_PATH="$<TARGET_FILE:config_runner>"
    TEST_GAME_PLUGIN_DIR="${TEST_GAME_PLUGIN_PARENT_DIR}"
    TEST_BOT)
  set_target_properties(test_bot PROPERTIES ENABLE_EXPORTS ON)
  add_test(NAME test_bot COMMAND test_bot)

  add_library(test_game_for_test_lib OBJECT ${TEST_GAME_SOURCE_FILES})
  target_compile_definitions(test_game_for_test_lib PUBLIC
    GAME_ACHIEVEMENT_FILENAME="${CMAKE_CURRENT_SOURCE_DIR}/test_game/achievements.h"
    GAME_OPTION_FILENAME="${CMAKE_CURRENT_SOURCE_DIR}/test_game/options.h"
    GAME_MODULE_NAME=test_game
    TEST_BOT)
  target_include_directories(test_game_for_test_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_game
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
  if (WITH_GLOG)
    target_link_libraries(test_game_for_test_lib glog::glog)
  endif()
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_dependencies(test_game_for_test_lib test_game_rule_binary)
  endif()

  add_executable(test_test_game
    ${CMAKE_CURRENT_SOURCE_DIR}/test_game/unittest.cc
    $<TARGET_OBJECTS:test_game_for_test_lib>)
  target_compile_definitions(test_test_game PUBLIC
    GAME_ACHIEVEMENT_FILENAME="${CMAKE_CURRENT_SOURCE_DIR}/test_game/achievements.h"
    GAME_OPTION_FILENAME="${CMAKE_CURRENT_SOURCE_DIR}/test_game/options.h"
    GAME_MODULE_NAME=test_game
    TEST_BOT)
  target_include_directories(test_test_game PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_game
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
  target_link_libraries(test_test_game GTest::GTest GTest::Main gflags)
  if (WITH_GLOG)
    target_link_libraries(test_test_game glog::glog)
  endif()
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_dependencies(test_test_game test_game_rule_binary)
    target_link_libraries(test_test_game ${TEST_GAME_RULE_BINARY})
  endif()
  add_test(NAME test_test_game COMMAND test_test_game --resource_dir "${CMAKE_CURRENT_SOURCE_DIR}/test_game")

  add_executable(test_db test_db.cc db_manager.cc score_calculation.cc)
  target_link_libraries(test_db ${THIRD_PARTIES})
  add_test(NAME test_db COMMAND test_db)

  add_executable(test_match_score test_match_score.cc score_calculation.cc)
  target_link_libraries(test_match_score ${THIRD_PARTIES})
  add_test(NAME test_match_score COMMAND test_match_score)
endif()
