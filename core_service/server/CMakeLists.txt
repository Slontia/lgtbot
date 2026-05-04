# gRPC server: codegen lives in src/proto (target lgtbot_grpc_generated).

find_package(gRPC CONFIG REQUIRED)

add_executable(lgtbot_grpc_server
  main.cc
  lgtbot_grpc_service.cc)

target_include_directories(lgtbot_grpc_server PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_BINARY_DIR}/../bot_core
  ${CMAKE_CURRENT_BINARY_DIR}/../match_process)

target_link_libraries(lgtbot_grpc_server PRIVATE
  lgtbot_grpc_generated
  bot_core_static)

target_compile_definitions(lgtbot_grpc_server PRIVATE
  MATCH_GAME_RUNNER_PATH="$<TARGET_FILE:match_game_runner>"
  CONFIG_RUNNER_PATH="$<TARGET_FILE:config_runner>")

add_dependencies(lgtbot_grpc_server match_game_runner config_runner)

set_target_properties(lgtbot_grpc_server PROPERTIES CXX_STANDARD 20)
