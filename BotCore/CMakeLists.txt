cmake_minimum_required(VERSION 2.8)
project(BotCore LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

list(APPEND SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/bot_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/db_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/match.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/message_handlers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/load_game_modules.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

find_package(gflags REQUIRED)
list(APPEND THIRD_PARTIES gflags)
find_package(glog REQUIRED)
list(APPEND THIRD_PARTIES glog)

# find mysql-connector-c++
option(WITH_MYSQL "build with mysql connection" TRUE)
if (WITH_MYSQL)
  option(MYSQL_CONNECTOR_PATH "path of mysql-connector-c++ headers")
  option(MYSQL_CONNECTOR_LIBRARIES "path of mysql-connector-c++ libraries")
  if (NOT MYSQL_CONNECTOR_PATH OR NOT MYSQL_CONNECTOR_LIBRARIES)
    message(FATAL_ERROR "MYSQL_CONNECTOR_PATH and MYSQL_CONNECTOR_LIBRARIES is required")
  else()
    include_directories(${MYSQL_CONNECTOR_PATH})
    list(APPEND THIRD_PARTIES ${MYSQL_CONNECTOR_LIBRARIES})
  endif()
endif()

add_library(game_core_static STATIC ${SOURCE_FILES})
add_library(game_core_shared SHARED ${SOURCE_FILES})
target_link_libraries(game_core_static ${THIRD_PARTIES})
