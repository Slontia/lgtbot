# Shared protobuf / gRPC codegen for core_service (C++) and all Go platform_services.
# Added from core_service/CMakeLists.txt as:
#   add_subdirectory(${CMAKE_SOURCE_DIR}/proto ${CMAKE_CURRENT_BINARY_DIR}/lgtbot_proto)
#
# All *.proto files in this directory are compiled (no per-file lists here).
#
# Inputs (set by parent before add_subdirectory):
#   LGTBOT_PROTO_GENERATE_GRPC — ON when gRPC C++ stubs should be generated.

cmake_minimum_required(VERSION 3.14)

set(LGTBOT_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LGTBOT_PROTO_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")
file(MAKE_DIRECTORY "${LGTBOT_PROTO_GEN}")

file(GLOB LGTBOT_PROTO_FILES CONFIGURE_DEPENDS "${LGTBOT_PROTO_ROOT}/*.proto")
list(SORT LGTBOT_PROTO_FILES)
if (NOT LGTBOT_PROTO_FILES)
  message(FATAL_ERROR "No .proto files under ${LGTBOT_PROTO_ROOT}")
endif()

set(LGTBOT_PROTO_FILENAMES "")
set(LGTBOT_PB_CC_SOURCES "")
set(LGTBOT_PB_OUTPUTS "")
set(LGTBOT_GRPC_CC_SOURCES "")
set(LGTBOT_GRPC_OUTPUTS "")
foreach(_proto IN LISTS LGTBOT_PROTO_FILES)
  get_filename_component(_base "${_proto}" NAME_WE)
  get_filename_component(_fname "${_proto}" NAME)
  list(APPEND LGTBOT_PROTO_FILENAMES "${_fname}")
  list(APPEND LGTBOT_PB_CC_SOURCES "${LGTBOT_PROTO_GEN}/${_base}.pb.cc")
  list(APPEND LGTBOT_PB_OUTPUTS
    "${LGTBOT_PROTO_GEN}/${_base}.pb.cc"
    "${LGTBOT_PROTO_GEN}/${_base}.pb.h")
  list(APPEND LGTBOT_GRPC_CC_SOURCES "${LGTBOT_PROTO_GEN}/${_base}.grpc.pb.cc")
  list(APPEND LGTBOT_GRPC_OUTPUTS
    "${LGTBOT_PROTO_GEN}/${_base}.grpc.pb.cc"
    "${LGTBOT_PROTO_GEN}/${_base}.grpc.pb.h")
endforeach()

cmake_policy(SET CMP0077 NEW)
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(protobuf CONFIG QUIET)
if (NOT TARGET protobuf::libprotobuf)
  find_package(Protobuf MODULE REQUIRED)
endif()
if (NOT TARGET protobuf::libprotobuf)
  message(FATAL_ERROR "Could not find protobuf (need protobuf::libprotobuf).")
endif()

# Legacy nested output trees from older CMake rules (flat gen/ is canonical now).

if (LGTBOT_PROTO_GENERATE_GRPC)
  if (NOT TARGET gRPC::grpc_cpp_plugin)
    message(FATAL_ERROR "LGTBOT_PROTO_GENERATE_GRPC is ON but gRPC::grpc_cpp_plugin is missing.")
  endif()
  add_custom_command(
    OUTPUT ${LGTBOT_PB_OUTPUTS} ${LGTBOT_GRPC_OUTPUTS}
    WORKING_DIRECTORY "${LGTBOT_PROTO_ROOT}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf
      "${LGTBOT_PROTO_GEN}/server"
      "${LGTBOT_PROTO_GEN}/bot_core"
    COMMAND protobuf::protoc
    ARGS --proto_path=${LGTBOT_PROTO_ROOT}
         --cpp_out=${LGTBOT_PROTO_GEN}
         --grpc_out=${LGTBOT_PROTO_GEN}
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         ${LGTBOT_PROTO_FILENAMES}
    DEPENDS ${LGTBOT_PROTO_FILES}
    VERBATIM)
else()
  add_custom_command(
    OUTPUT ${LGTBOT_PB_OUTPUTS}
    WORKING_DIRECTORY "${LGTBOT_PROTO_ROOT}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf
      "${LGTBOT_PROTO_GEN}/server"
      "${LGTBOT_PROTO_GEN}/bot_core"
    COMMAND protobuf::protoc
    ARGS --proto_path=${LGTBOT_PROTO_ROOT}
         --cpp_out=${LGTBOT_PROTO_GEN}
         ${LGTBOT_PROTO_FILENAMES}
    DEPENDS ${LGTBOT_PROTO_FILES}
    VERBATIM)
endif()

add_library(lgtbot_command_args SHARED ${LGTBOT_PB_CC_SOURCES})
set_target_properties(lgtbot_command_args PROPERTIES CXX_STANDARD 20)
set_source_files_properties(${LGTBOT_PB_OUTPUTS} PROPERTIES GENERATED TRUE)
target_include_directories(lgtbot_command_args PUBLIC ${LGTBOT_PROTO_GEN})
target_link_libraries(lgtbot_command_args PUBLIC protobuf::libprotobuf)

if (LGTBOT_PROTO_GENERATE_GRPC)
  set_source_files_properties(${LGTBOT_GRPC_OUTPUTS} PROPERTIES GENERATED TRUE)
  add_library(lgtbot_grpc_generated STATIC ${LGTBOT_GRPC_CC_SOURCES})
  set_target_properties(lgtbot_grpc_generated PROPERTIES CXX_STANDARD 20)
  target_include_directories(lgtbot_grpc_generated PUBLIC ${LGTBOT_PROTO_GEN})
  target_link_libraries(lgtbot_grpc_generated PUBLIC
    lgtbot_command_args
    protobuf::libprotobuf
    gRPC::grpc++)
endif()

# ---------------------------------------------------------------------------
# Go: shared module lgtbot/proto (optional; requires protoc-gen-go and
# protoc-gen-go-grpc). Output: proto/go/pb — any Go platform can require this module.
# ---------------------------------------------------------------------------
set(LGTBOT_GO_MODULE_ROOT "${LGTBOT_PROTO_ROOT}/go")
find_program(LGTBOT_PROTOC_GEN_GO NAMES protoc-gen-go)
find_program(LGTBOT_PROTOC_GEN_GO_GRPC NAMES protoc-gen-go-grpc)
if (LGTBOT_PROTOC_GEN_GO AND LGTBOT_PROTOC_GEN_GO_GRPC)
  set(LGTBOT_GO_MODULE "lgtbot/proto")
  add_custom_target(lgtbot_go_protos
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LGTBOT_GO_MODULE_ROOT}"
    COMMAND protobuf::protoc
    ARGS -I=${LGTBOT_PROTO_ROOT}
         --go_out=${LGTBOT_GO_MODULE_ROOT}
         --go_opt=module=${LGTBOT_GO_MODULE}
         --go-grpc_out=${LGTBOT_GO_MODULE_ROOT}
         --go-grpc_opt=module=${LGTBOT_GO_MODULE}
         ${LGTBOT_PROTO_FILENAMES}
    WORKING_DIRECTORY "${LGTBOT_PROTO_ROOT}"
    DEPENDS ${LGTBOT_PROTO_FILES}
    COMMENT "Generating Go protobuf (+grpc) into proto/go/"
    VERBATIM)
else()
  message(STATUS "protoc-gen-go / protoc-gen-go-grpc not found — target lgtbot_go_protos disabled")
endif()
