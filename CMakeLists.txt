cmake_minimum_required(VERSION 3.5)
project(tLBS)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)
include(FeatureSummary)
include(FindPackageHandleStandardArgs)

get_filename_component(TLBS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Avoid "Policy CMP0042 is not set" warning on macOS.
if (APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# No compiler-specific extensions, i.e. -std=c++11, not -std=gnu++11.
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(EXECUTABLE_OUTPUT_PATH ../bin)

find_package(GFlags REQUIRED)
if (NOT ${GFLAGS_FOUND})
    message(FATAL_ERROR "GFlags requested but not found")
endif()
message("GFLAGS_INCLUDE_DIRS ${GFLAGS_INCLUDE_DIRS}")
message("GFLAGS_LIBRARIES ${GFLAGS_LIBRARIES}")

find_package(Glog REQUIRED)
if (NOT ${GLOG_FOUND})
    message(FATAL_ERROR "Glog requested but not found")
endif()
message("GLOG_INCLUDE_DIRS ${GLOG_INCLUDE_DIRS}")
message("GLOG_LIBRARIES ${GLOG_LIBRARIES}")

find_package(RapidJSON)
find_package(S2Geometry REQUIRED)

message("S2GEOMETRY_INCLUDE_DIRS ${S2GEOMETRY_INCLUDE_DIRS}")
message("S2GEOMETRY_LIBRARIES ${S2GEOMETRY_LIBRARIES}")

add_definitions(-DS2_USE_GLOG)
add_definitions(-DS2_USE_GFLAGS)

include_directories (${GFLAGS_INCLUDE_DIRS} ${GLOG_INCLUDE_DIRS} ${S2GEOMETRY_INCLUDE_DIRS})


set(SRC_SERVER_TMP
#        src/childinfo.cc
        src/client.cc
        src/command.cc
        src/common.cc
        src/config.cc
        src/connection.cc
        src/db.cc
        src/el.cc
#        src/endianconv.cc
#        src/geo.cc
        src/http.cc
        src/json.cc
        src/log.cc
#        src/localtime.cc
        src/net_tcp.cc
        src/object.cc
#        src/rax.cc
#        src/setproctitle.cc
        src/server.cc
#        src/siphash.cc
#        src/sha1.cc
#        src/sha256.cc
#        src/syncio.cc
        src/table.cc
        src/t_s2geometry.cc
#        src/t_point.cc
#        src/t_polygon.cc
#        src/t_string.cc
        src/threadpool_c.cc
        )
set(SRC_SERVER src/server_main.cc ${SRC_SERVER_TMP})



add_executable(tLBS-server ${SRC_SERVER})

target_link_libraries(tLBS-server
        ${S2GEOMETRY_LIBRARIES}
        )

# 测试
add_subdirectory(test)