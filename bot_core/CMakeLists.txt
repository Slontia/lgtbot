cmake_minimum_required(VERSION 3.0)
project(BotCore LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -latomic")

list(APPEND SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/bot_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/db_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/match.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/match_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/message_handlers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/load_game_modules.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/msg_sender.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/msg_sender.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../utility/html.cpp
)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
  list(APPEND SOURCE_FLIES ${CMAKE_CURRENT_SOURCE_DIR}/dllmain.cpp)
endif()

find_package(Threads REQUIRED)
list(APPEND THIRD_PARTIES Threads::Threads)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND THIRD_PARTIES
    dl      # load shared libraries dynamicly on linux
    atomic
  )
endif()

find_package(gflags REQUIRED)
list(APPEND THIRD_PARTIES gflags)

add_library(bot_core_static STATIC ${SOURCE_FILES})
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_library(bot_core SHARED ${SOURCE_FILES})
endif()

target_link_libraries(bot_core_static ${THIRD_PARTIES})

if (WITH_TEST)
  enable_testing()
  find_package(GTest REQUIRED)
  list(APPEND THIRD_PARTIES GTest::GTest GTest::Main)

  add_executable(test_bot test_bot.cc ${SOURCE_FILES})
  target_link_libraries(test_bot ${THIRD_PARTIES})
  target_compile_definitions(test_bot PUBLIC TEST_BOT)
  target_include_directories(test_bot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # to include empty options.h
  add_test(NAME test_bot COMMAND test_bot)

  add_executable(test_db test_db.cc db_manager.cpp)
  target_link_libraries(test_db ${THIRD_PARTIES})
  add_test(NAME test_db COMMAND test_db)

  add_executable(test_match_score test_match_score.cc match.cpp msg_sender.cpp)
  target_link_libraries(test_match_score ${THIRD_PARTIES})
  add_test(NAME test_match_score COMMAND test_match_score)
endif()

