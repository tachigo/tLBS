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

set(EXECUTABLE_OUTPUT_PATH bin)

add_executable(test-primary-type src/primary_type.cc)
add_executable(test-string src/string_test.cc)