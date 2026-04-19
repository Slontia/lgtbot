cmake_minimum_required(VERSION 3.14)
project(BotCore LANGUAGES CXX C)

set(CMAKE_SYSTEM_VERSION 1)

# Default build with Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

option(WITH_GCOV "bulid with gcov" FALSE)
option(WITH_ASAN "build with address sanitizier" FALSE)
option(WITH_TEST "build with unittest" TRUE)
option(WITH_CORE "build with bot core" TRUE)
option(WITH_TOOLS "build with tools" TRUE)
option(WITH_GAMES "build with games" TRUE)
option(WITH_IMAGE "allow bot print image" TRUE)
option(WITH_GLOG "build with glog" TRUE)
option(WITH_SQLITE "build with sqlite" TRUE)

# 'char' type in arm machines is 'unsigned char', we should define it as 'signed char'
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char -g -Wno-invalid-utf8 -Wno-c99-designator -Winconsistent-missing-override -Werror=inconsistent-missing-override")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char -g -Wno-invalid-utf8")

include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include)

# On Apple Silicon, Homebrew installs under /opt/homebrew while legacy x86
# Homebrew remnants may still linger in /usr/local. Prepend /opt/homebrew so
# find_package() prefers the native ARM build, and exclude /usr/local from
# library search so that bare library names (e.g. the "gflags" string in
# glog's INTERFACE_LINK_LIBRARIES) resolve from the same prefix as the
# config-mode targets, avoiding rpath cycle warnings.
# On x86 Mac, /opt/homebrew does not exist, so this block is a no-op there.
if (APPLE AND EXISTS /opt/homebrew)
  list(PREPEND CMAKE_PREFIX_PATH /opt/homebrew)
  list(APPEND CMAKE_IGNORE_PATH /usr/local/lib /usr/local/include)
endif()

if (WITH_IMAGE)
  find_package(gflags REQUIRED)

  # markdown2image: default to Qt5 WebKit (e.g. RHEL 8); auto-fall back to Qt6 WebEngine when Qt5 WebKit is missing.
  option(MARKDOWN2IMAGE_USE_QT5WEBKIT
    "Prefer Qt5 WebKit for markdown2image; if unavailable, use Qt6 WebEngine when installed"
    ON)

  # Guard Qt5 find_package: Qt5CoreConfig.cmake derives its install prefix from
  # its own location (get_filename_component 3 levels up), then checks that
  # <prefix>/mkspecs/... exists. When Qt5 is partially installed (e.g. missing
  # mkspecs), find_package(Qt5) raises a hard CMake error even with QUIET.
  # Pre-flight: compute the same prefix and skip Qt5 if mkspecs is absent.
  find_file(_qt5_core_cfg_path Qt5CoreConfig.cmake
    PATHS
      /usr/local/lib/cmake/Qt5Core
      /usr/lib/cmake/Qt5Core
      /usr/lib64/cmake/Qt5Core)
  set(_qt5_usable FALSE)
  if (_qt5_core_cfg_path)
    # install prefix = Qt5CoreConfig.cmake dir / ../../.. (3 levels up)
    get_filename_component(_qt5_prefix "${_qt5_core_cfg_path}" DIRECTORY)
    get_filename_component(_qt5_prefix "${_qt5_prefix}" DIRECTORY)
    get_filename_component(_qt5_prefix "${_qt5_prefix}" DIRECTORY)
    get_filename_component(_qt5_prefix "${_qt5_prefix}" DIRECTORY)
    # Qt's CMake files derive prefix from .../lib(64)/cmake/Qt5Core; on RHEL/Fedora
    # that is /usr while mkspecs lives under lib64/qt5/mkspecs, not <prefix>/mkspecs.
    set(_qt5_mkspecs_ok FALSE)
    foreach (_qt5_mkspecs_dir
        "${_qt5_prefix}/mkspecs"
        "${_qt5_prefix}/lib64/qt5/mkspecs"
        "${_qt5_prefix}/lib/qt5/mkspecs"
        "${_qt5_prefix}/share/qt5/mkspecs")
      if (EXISTS "${_qt5_mkspecs_dir}")
        set(_qt5_mkspecs_ok TRUE)
        break()
      endif()
    endforeach()
    if (_qt5_mkspecs_ok)
      set(_qt5_usable TRUE)
    else()
      message(STATUS "markdown2image: Qt5 installation broken (no mkspecs next to ${_qt5_prefix}); skipping Qt5")
    endif()
  endif()
  if (_qt5_usable)
    find_package(Qt5 QUIET COMPONENTS Gui Widgets WebKit WebKitWidgets)
  endif()
  find_package(Qt6 QUIET COMPONENTS Gui Widgets WebEngineWidgets)

  set(_md2img_qt5_ok FALSE)
  if (TARGET Qt5::WebKitWidgets AND TARGET Qt5::WebKit AND TARGET Qt5::Widgets AND TARGET Qt5::Gui)
    set(_md2img_qt5_ok TRUE)
  endif()
  set(_md2img_qt6_ok FALSE)
  if (TARGET Qt6::WebEngineWidgets AND TARGET Qt6::Widgets AND TARGET Qt6::Gui)
    set(_md2img_qt6_ok TRUE)
  endif()

  if (MARKDOWN2IMAGE_USE_QT5WEBKIT)
    if (_md2img_qt5_ok)
      set(_md2img_use_qt5 TRUE)
    elseif (_md2img_qt6_ok)
      set(_md2img_use_qt5 FALSE)
      message(STATUS "markdown2image: Qt5 WebKit not found; using Qt6 WebEngine instead")
    else()
      message(WARNING "WITH_IMAGE=ON but neither Qt5 WebKit nor Qt6 WebEngine found; disabling image support.")
      set(WITH_IMAGE OFF)
    endif()
  else()
    if (_md2img_qt6_ok)
      set(_md2img_use_qt5 FALSE)
    elseif (_md2img_qt5_ok)
      set(_md2img_use_qt5 TRUE)
      message(STATUS "markdown2image: Qt6 WebEngine not found; using Qt5 WebKit")
    else()
      message(WARNING "WITH_IMAGE=ON but neither Qt6 WebEngine nor Qt5 WebKit found; disabling image support.")
      set(WITH_IMAGE OFF)
    endif()
  endif()

  if (WITH_IMAGE)
    set(USE_QT5WEBKIT "${_md2img_use_qt5}" CACHE BOOL "Forwarded to markdown2image: use Qt5 WebKit backend" FORCE)
    add_subdirectory(third_party/markdown2image)
  endif()
endif()

if (WITH_GCOV)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -fprofile-arcs -ftest-coverage -lgcov --coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -fprofile-arcs -ftest-coverage -lgcov --coverage")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
endif()

if (WITH_ASAN)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

if (WITH_SQLITE)
  add_definitions(-DWITH_SQLITE)
  find_package(SQLite3 REQUIRED)
  list(APPEND THIRD_PARTIES SQLite::SQLite3)
  include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite_modern_cpp/hdr)
endif()

if (WITH_GLOG)
  find_package(glog REQUIRED)
  list(APPEND THIRD_PARTIES glog::glog)
  add_definitions(-DWITH_GLOG)
  get_target_property(GLOG_INCLUDE_DIRS glog::glog INTERFACE_INCLUDE_DIRECTORIES)
  include_directories(${GLOG_INCLUDE_DIRS})
  get_target_property(GLOG_COMPILE_DEFS glog::glog INTERFACE_COMPILE_DEFINITIONS)
  add_definitions(-DGLOG_USE_GLOG_EXPORT)
endif()

if (WITH_TEST)
  enable_testing()
endif()

if (WITH_CORE)
  # Prefer protobuf's CMake package (config mode): protobuf >= 22 pulls abseil and
  # the bundled FindProtobuf module does not model that, which can break linking.
  # EL8 and similar only ship protobuf-devel + CMake's FindProtobuf (module mode),
  # so fall back when protobuf-config.cmake is not installed.
  # CMP0077 NEW: option() must not clear a normal variable already set by the
  # caller; without this, protobuf-options.cmake's option(protobuf_MODULE_COMPATIBLE)
  # would silently reset the value we set below.
  cmake_policy(SET CMP0077 NEW)
  set(protobuf_MODULE_COMPATIBLE TRUE)
  find_package(protobuf CONFIG QUIET)
  if (NOT TARGET protobuf::libprotobuf)
    find_package(Protobuf MODULE REQUIRED)
  endif()
  if (NOT TARGET protobuf::libprotobuf)
    message(FATAL_ERROR
      "Could not find protobuf (no protobuf::libprotobuf). Install a build with "
      "protobuf-config.cmake, or system protobuf-devel for CMake FindProtobuf.")
  endif()
  add_subdirectory(match_process)
  add_subdirectory(bot_core)
endif()

if (WITH_TOOLS)
  add_subdirectory(tools)
endif()

if (WITH_GAMES)
  add_subdirectory(third_party/necessary-and-unnecessary-tiles)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/necessary-and-unnecessary-tiles)
  find_package(pybind11 QUIET)
  if(pybind11_FOUND)
    include_directories(${pybind11_INCLUDE_DIRS})
    find_package(Python3 COMPONENTS Development QUIET)
    if(NOT Python3_FOUND)
      set(pybind11_FOUND FALSE)
      message(STATUS "Python3 development libraries not found, skipping MahjongPy")
    else()
      set(PYTHON_LIBRARIES ${Python3_LIBRARIES})
      set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
      if(APPLE)
        # Python framework on macOS does not export all symbols; resolve at runtime
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
      endif()
    endif()
  endif()
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  add_subdirectory(third_party/mahjong)
  unset(CMAKE_CXX_STANDARD)
  unset(CMAKE_CXX_STANDARD_REQUIRED)
  unset(CMAKE_CXX_EXTENSIONS)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/mahjong)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/tiny_expr)
  add_library(tinyexpr SHARED ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tiny_expr/tinyexpr.c)
  add_subdirectory(games)
endif()

if (WITH_TEST)
  add_subdirectory(game_util) # only build for unittest
  add_subdirectory(utility) # only build for unittest
endif()
