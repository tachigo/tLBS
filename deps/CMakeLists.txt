#add_subdirectory(gflags)
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/gflags)

#add_subdirectory(glog)
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/glog)

set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
add_subdirectory(s2geometry)

