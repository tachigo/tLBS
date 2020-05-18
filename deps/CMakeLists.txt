add_subdirectory(gflags)
set(WITH_GFLAGS ON)
set(GFLAGS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/gflags/src)


add_subdirectory(glog)
set(WITH_GLOG ON)
set(GLOG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/glog/src)

set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
set(GTEST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest)
add_subdirectory(s2geometry)

