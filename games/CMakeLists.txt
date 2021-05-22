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

foreach (GAME_DIR ${GAME_DIRS})
  if (IS_DIRECTORY ${GAME_DIR})

    string(REGEX REPLACE ".*/\(.*\)" "\\1" GAME ${GAME_DIR})

    message("Found game ${GAME} in ${GAME_DIR}")

    set(SOURCE_FILES
      ${GAME_DIR}/mygame.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/game.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/game_main.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/resource_loader.cpp
    )
    
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
      add_custom_target(${GAME}_rule ALL
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND objcopy --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 --rename-section .data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA rule.txt ${CMAKE_CURRENT_SOURCE_DIR}/rule.o
        COMMENT "Build rule"
      )
    endif()
    
    add_library(${GAME} SHARED ${SOURCE_FILES})

    set_property(TARGET ${GAME} PROPERTY INCLUDE_DIRECTORIES ${GAME_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../)
    #include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

    #set(GAME_OPTION_FILE ${GAME_DIR}/options.h)

    #configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/game_options.h.in ${CMAKE_CURRENT_SOURCE_DIR}/../game_framework/game_options.h)
    
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
      add_dependencies(${GAME} ${GAME}_rule)
      target_link_libraries(${GAME} ${CMAKE_CURRENT_SOURCE_DIR}/rule.o)
    endif()
    
    if (WITH_TEST)
      enable_testing()
      find_package(GTest REQUIRED)
      add_executable(test_game_${GAME} ${SOURCE_FILES} ${GAME_DIR}/unittest.cc)
      target_link_libraries(test_game_${GAME} GTest::GTest GTest::Main ${CMAKE_CURRENT_SOURCE_DIR}/rule.o)
      if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        add_dependencies(test_game_${GAME} ${GAME}_rule)
        target_link_libraries(test_game_${GAME} ${CMAKE_CURRENT_SOURCE_DIR}/rule.o)
      endif()
    endif()

  endif()
endforeach() 
