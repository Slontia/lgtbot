cmake_minimum_required(VERSION 3.14)
project(MatchProcess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# protobuf: find_package in parent ../CMakeLists.txt (config and/or module).

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/match_ipc.proto)

set(CHILD_TRANSPORT
  ${CMAKE_CURRENT_SOURCE_DIR}/ipc_frame.cc
  ${PROTO_SRCS}
)

find_package(Threads REQUIRED)

add_executable(match_game_runner
  ${CMAKE_CURRENT_SOURCE_DIR}/game_runner_main.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/child_session.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/ipc_match_env.cc
  ${CHILD_TRANSPORT}
)
target_include_directories(match_game_runner PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_link_libraries(match_game_runner PRIVATE Threads::Threads protobuf::libprotobuf)
if (WITH_GLOG)
  target_link_libraries(match_game_runner PRIVATE glog::glog)
endif()
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(match_game_runner PRIVATE ${CMAKE_DL_LIBS})
endif()
# Plugins (libgame.so) use glog macros; symbols must be visible to the dynamic linker (same idea as test_bot).
set_target_properties(match_game_runner PROPERTIES ENABLE_EXPORTS ON)

add_executable(config_runner
  ${CMAKE_CURRENT_SOURCE_DIR}/config_runner_main.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/child_config_session.cc
  ${CHILD_TRANSPORT}
)
target_include_directories(config_runner PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_link_libraries(config_runner PRIVATE Threads::Threads protobuf::libprotobuf)
if (WITH_GLOG)
  target_link_libraries(config_runner PRIVATE glog::glog)
endif()
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(config_runner PRIVATE ${CMAKE_DL_LIBS})
endif()
set_target_properties(config_runner PROPERTIES ENABLE_EXPORTS ON)

set(MATCH_PROCESS_PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
set(MATCH_PROCESS_PROTO_SRCS "${PROTO_SRCS}" PARENT_SCOPE)
set(MATCH_PROCESS_PROTO_HDRS "${PROTO_HDRS}" PARENT_SCOPE)
