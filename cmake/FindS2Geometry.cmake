# - Try to find S2Geometry
#
# The following variables are optionally searched for defaults
#  GFLAGS_ROOT_DIR:            Base directory where all GFLAGS components are found
#
# The following are set after configuration is done:
#  S2GEOMETRY_FOUND
#  S2GEOMETRY_INCLUDE_DIRS
#  S2GEOMETRY_LIBRARIES
#  S2GEOMETRY_LIBRARYRARY_DIRS

include(FindPackageHandleStandardArgs)

set(S2GEOMETRY_ROOT_DIR "" CACHE PATH "Folder contains S2Geometry")

# We are testing only a couple of files in the include directories
if(WIN32)
    find_path(S2GEOMETRY_INCLUDE_DIR s2/s2earth.h
            PATHS ${GFLAGS_ROOT_DIR}/src/windows)
else()
    find_path(S2GEOMETRY_INCLUDE_DIR s2/s2earth.h
            PATHS ${GFLAGS_ROOT_DIR})
endif()

if(MSVC)
    find_library(S2GEOMETRY_LIBRARY_RELEASE
            NAMES libs2
            PATHS ${S2GEOMETRY_ROOT_DIR}
            PATH_SUFFIXES Release)

    find_library(S2GEOMETRY_LIBRARY_DEBUG
            NAMES libs2-debug
            PATHS ${S2GEOMETRY_ROOT_DIR}
            PATH_SUFFIXES Debug)

    set(S2GEOMETRY_LIBRARY optimized ${S2GEOMETRY_LIBRARY_RELEASE} debug ${S2GEOMETRY_LIBRARY_DEBUG})
else()
    find_library(S2GEOMETRY_LIBRARY s2)
endif()

find_package_handle_standard_args(S2GEOMETRY DEFAULT_MSG
        S2GEOMETRY_INCLUDE_DIR S2GEOMETRY_LIBRARY)


if(S2GEOMETRY_FOUND)
    set(S2GEOMETRY_INCLUDE_DIRS ${S2GEOMETRY_INCLUDE_DIR})
    set(S2GEOMETRY_LIBRARIES ${S2GEOMETRY_LIBRARY})
endif()
