# FindFFTW3.cmake
# Find the FFTW3 library for FFT operations
#
# This module defines:
#  FFTW3_FOUND - True if FFTW3 is found
#  FFTW3_INCLUDE_DIRS - Include directories for FFTW3
#  FFTW3_LIBRARIES - Libraries to link for FFTW3
#  FFTW3_VERSION - Version of FFTW3 found

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_FFTW3 QUIET fftw3)
endif()

# Find the header file
find_path(FFTW3_INCLUDE_DIR
    NAMES fftw3.h
    PATHS
        ${PC_FFTW3_INCLUDE_DIRS}
        /usr/include
        /usr/local/include
        /opt/local/include
        ${CMAKE_PREFIX_PATH}/include
    PATH_SUFFIXES
        fftw3
        fftw
)

# Find the main library
find_library(FFTW3_LIBRARY
    NAMES fftw3
    PATHS
        ${PC_FFTW3_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
)

# Find optional libraries
find_library(FFTW3F_LIBRARY
    NAMES fftw3f
    PATHS
        ${PC_FFTW3_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
)

find_library(FFTW3L_LIBRARY
    NAMES fftw3l
    PATHS
        ${PC_FFTW3_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
)

# Get version from pkg-config if available
if(PC_FFTW3_VERSION)
    set(FFTW3_VERSION ${PC_FFTW3_VERSION})
endif()

# Handle standard arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFTW3
    REQUIRED_VARS FFTW3_LIBRARY FFTW3_INCLUDE_DIR
    VERSION_VAR FFTW3_VERSION
)

if(FFTW3_FOUND)
    set(FFTW3_LIBRARIES ${FFTW3_LIBRARY})
    set(FFTW3_INCLUDE_DIRS ${FFTW3_INCLUDE_DIR})

    # Add optional libraries if found
    if(FFTW3F_LIBRARY)
        list(APPEND FFTW3_LIBRARIES ${FFTW3F_LIBRARY})
    endif()
    if(FFTW3L_LIBRARY)
        list(APPEND FFTW3_LIBRARIES ${FFTW3L_LIBRARY})
    endif()

    # Create imported target
    if(NOT TARGET FFTW3::FFTW3)
        add_library(FFTW3::FFTW3 UNKNOWN IMPORTED)
        set_target_properties(FFTW3::FFTW3 PROPERTIES
            IMPORTED_LOCATION "${FFTW3_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FFTW3_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(FFTW3_INCLUDE_DIR FFTW3_LIBRARY FFTW3F_LIBRARY FFTW3L_LIBRARY)
