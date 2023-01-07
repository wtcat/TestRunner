cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
project(app)
enable_language(C CXX)

set(ZEPHYR_BASE ${CMAKE_CURRENT_SOURCE_DIR})

# Import user function 
include(cmake/extension.cmake)
include(cmake/extensions.cmake)

if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()

option(CONFIG_CPLUSPLUS "Enable c++ launguage" ON)

# Select operation system
message(STATUS "OS Platform: ${CMAKE_SYSTEM_NAME}")
if (CMAKE_HOST_WIN32)
    #enable_language(RC)
    set(WINDOWS TRUE)
    set(TargetName ${PROJECT_NAME}.exe)
  #  add_compile_options(/wd4819)
elseif (CMAKE_HOST_APPLE)
    set(MACOS TRUE)
    set(TargetName ${PROJECT_NAME}.elf)
elseif (CMAKE_HOST_UNIX)
    set(LINUX TRUE)
    set(TargetName ${PROJECT_NAME}.elf)
else ()
  message(FATAL_ERROR "Unknown operation system")
endif ()

include_directories(.)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(
    -Wall
    -O0
    -g
)

#
# Project configurations
#
set(CONFIG_GTEST True)
set(CONFIG_BASEWORK_TEST True)

add_executable(${TargetName}
    main.cc
    # thread/thread_test.cc
)

# add_subdirectory(googletest)
add_subdirectory(basework)

collect_link_libraries(libs ${TargetName})
target_link_libraries(${TargetName}
    -Wl,--whole-archive
    ${libs}
    -Wl,--no-whole-archive
    pthread
    rt
)
