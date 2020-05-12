add_subdirectory(gflags)
add_subdirectory(glog)

add_definitions(-DGTEST_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest)
set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
add_subdirectory(s2geometry)

