cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(tLBS VERSION 1.0)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 11)
get_filename_component(TLBS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(EXECUTABLE_OUTPUT_PATH ../bin)

find_package (gflags REQUIRED)
message("gflags_INCLUDE_DIR ${gflags_INCLUDE_DIR}")
include_directories (${gflags_INCLUDE_DIR})

set(SRC_SERVER_TMP
#        src/adlist.cc
#        src/ae.cc
#        src/anet.cc
#        src/childinfo.cc
        src/client.cc
#        src/command.cc
#        src/common.cc
#        src/config.cc
#        src/connection.cc
#        src/persistence.cc
        src/db.cc
#        src/debug.cc
#        src/dict.cc
#        src/endianconv.cc
#        src/geo.cc
#        src/localtime.cc
#        src/networking.cc
        src/object.cc
#        src/rax.cc
#        src/sds.cc
#        src/setproctitle.cc
        src/server.cc
#        src/siphash.cc
#        src/sha1.cc
#        src/sha256.cc
#        src/syncio.cc
#        src/t_point.cc
#        src/t_polygon.cc
#        src/t_string.cc
#        src/util.cc
#        src/zmalloc.cc
        )
set(SRC_SERVER src/server_main.cc ${SRC_SERVER_TMP})



#link_directories(
#        ${TLBS_ROOT}/deps/gflags
#        ${TLBS_ROOT}/deps/glog
#        ${TLBS_ROOT}/deps/s2geometry
#)
add_executable(tLBS-server ${SRC_SERVER})
#target_include_directories(tLBS-server
#        PRIVATE gflags
#        )

target_link_libraries(tLBS-server
         gflags
        )