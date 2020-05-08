cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(tLBS VERSION 1.0)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 11)
get_filename_component(TLBS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)


add_subdirectory(deps)

set(SRC_SERVER_TMP
        src/dict.cc
        src/t_point.cc
        )
set(SRC_SERVER src/server.cc ${SRC_SERVER_TMP})

set(EXECUTABLE_OUTPUT_PATH src)
add_executable(tLBS-server ${SRC_SERVER})