# FindFAAD2.cmake
# Find the FAAD2 library for AAC decoding
#
# This module defines:
#  FAAD2_FOUND - True if FAAD2 is found
#  FAAD2_INCLUDE_DIRS - Include directories for FAAD2
#  FAAD2_LIBRARIES - Libraries to link for FAAD2
#  FAAD2_VERSION - Version of FAAD2 found

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_FAAD2 QUIET faad2)
endif()

# Find the header file
find_path(FAAD2_INCLUDE_DIR
    NAMES faad.h
    PATHS
        ${PC_FAAD2_INCLUDE_DIRS}
        /usr/include
        /usr/local/include
        /opt/local/include
        ${CMAKE_PREFIX_PATH}/include
    PATH_SUFFIXES
        faad2
        faad
)

# Find the library
find_library(FAAD2_LIBRARY
    NAMES faad faad2
    PATHS
        ${PC_FAAD2_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
)

# Get version from pkg-config if available
if(PC_FAAD2_VERSION)
    set(FAAD2_VERSION ${PC_FAAD2_VERSION})
endif()

# Handle standard arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FAAD2
    REQUIRED_VARS FAAD2_LIBRARY FAAD2_INCLUDE_DIR
    VERSION_VAR FAAD2_VERSION
)

if(FAAD2_FOUND)
    set(FAAD2_LIBRARIES ${FAAD2_LIBRARY})
    set(FAAD2_INCLUDE_DIRS ${FAAD2_INCLUDE_DIR})

    # Create imported target
    if(NOT TARGET FAAD2::FAAD2)
        add_library(FAAD2::FAAD2 UNKNOWN IMPORTED)
        set_target_properties(FAAD2::FAAD2 PROPERTIES
            IMPORTED_LOCATION "${FAAD2_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FAAD2_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(FAAD2_INCLUDE_DIR FAAD2_LIBRARY)
