cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(tLBS VERSION 1.0)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 11)
get_filename_component(TLBS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)


add_subdirectory(deps)

set(SRC_SERVER_TMP
        src/adlist.cc
        src/ae.cc
#        src/ae_epoll.cc
#        src/ae_select.cc
#        src/ae_kqueue.cc
#        src/ae_evport.cc
        src/anet.cc
        src/client.cc
        src/command.cc
        src/common.cc
        src/config.cc
        src/connection.cc
        src/db.cc
        src/debug.cc
        src/dict.cc
        src/endianconv.cc
        src/localtime.cc
        src/networking.cc
        src/object.cc
        src/rax.cc
        src/sds.cc
        src/setproctitle.cc
        src/siphash.cc
        src/sha1.cc
        src/sha256.cc
        src/syncio.cc
        src/t_point.cc
        src/util.cc
        src/zmalloc.cc
        )
set(SRC_SERVER src/server.cc ${SRC_SERVER_TMP})

set(EXECUTABLE_OUTPUT_PATH src)
link_directories(
        ${TLBS_ROOT}/deps/glog
        ${TLBS_ROOT}/deps/gflags
#        ${TLBS_ROOT}/deps/googletest/googletest
        ${TLBS_ROOT}/deps/s2geometry
)

add_executable(tLBS-server ${SRC_SERVER})
target_include_directories(tLBS-server
        PRIVATE ${TLBS_ROOT}/deps/glog/src
        PRIVATE ${TLBS_ROOT}/deps/gflags/src
#        PRIVATE ${TLBS_ROOT}/deps/googletest/googletest/src
        PRIVATE ${TLBS_ROOT}/deps/s2geometry/src
        )


target_link_libraries(tLBS-server
        PRIVATE pthread
        PRIVATE m
        PRIVATE glog
        PRIVATE gflags
#        PRIVATE googletest
        PRIVATE s2
        )