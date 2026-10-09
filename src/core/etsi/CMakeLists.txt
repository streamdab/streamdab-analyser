# CMakeLists.txt for ETSI Compliance Module
# Professional ETSI Standards Framework with comprehensive compliance validation

cmake_minimum_required(VERSION 3.16)

# ETSI Compliance Module Configuration
set(ETSI_MODULE_NAME "etsi_compliance")
set(ETSI_VERSION_MAJOR 1)
set(ETSI_VERSION_MINOR 0)
set(ETSI_VERSION_PATCH 0)
set(ETSI_VERSION "${ETSI_VERSION_MAJOR}.${ETSI_VERSION_MINOR}.${ETSI_VERSION_PATCH}")

message(STATUS "Configuring ETSI Compliance Module v${ETSI_VERSION}")

# ETSI Module Source Files
set(ETSI_HEADERS
    compliance_engine.h
    standard_validators.h
    alert_system.h
    compliance_logger.h
    compliance_reporter.h
    report_generators.h
    realtime_monitor.h
    broadcast_standards.h
    etsi_integration_manager.h
)

set(ETSI_SOURCES
    compliance_engine.cpp
    standard_validators.cpp
    alert_system.cpp
    compliance_logger.cpp
    realtime_monitor.cpp
)

# ETSI Data Types (shared with main core)
set(ETSI_DATA_TYPES
    ../eti_types.h
    ../eti_types.cpp
)

# Create ETSI Compliance Library
add_library(${ETSI_MODULE_NAME} STATIC
    ${ETSI_SOURCES}
    ${ETSI_DATA_TYPES}
)

# Set target properties
set_target_properties(${ETSI_MODULE_NAME} PROPERTIES
    VERSION ${ETSI_VERSION}
    SOVERSION ${ETSI_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# ETSI Module Include Directories
target_include_directories(${ETSI_MODULE_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/src/core
)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Network)

# Find optional audio processing libraries
find_package(PkgConfig QUIET)

# Find FFTW3 for spectrum analysis
if(PKG_CONFIG_FOUND)
    pkg_check_modules(FFTW3 fftw3f)
    if(FFTW3_FOUND)
        message(STATUS "Found FFTW3: ${FFTW3_VERSION}")
        target_compile_definitions(${ETSI_MODULE_NAME} PRIVATE HAVE_FFTW3)
    else()
        message(WARNING "FFTW3 not found - spectrum analysis features disabled")
    endif()
else()
    # Try to find FFTW3 manually
    find_path(FFTW3_INCLUDE_DIR fftw3.h)
    find_library(FFTW3_LIBRARY fftw3f)

    if(FFTW3_INCLUDE_DIR AND FFTW3_LIBRARY)
        set(FFTW3_FOUND TRUE)
        set(FFTW3_LIBRARIES ${FFTW3_LIBRARY})
        set(FFTW3_INCLUDE_DIRS ${FFTW3_INCLUDE_DIR})
        message(STATUS "Found FFTW3: ${FFTW3_LIBRARY}")
        target_compile_definitions(${ETSI_MODULE_NAME} PRIVATE HAVE_FFTW3)
    endif()
endif()

# Find audio libraries (optional)
find_library(AUDIO_LIBRARY
    NAMES
        portaudio
        alsa
        pulse
        coreaudio
    HINTS
        /usr/local/lib
        /opt/local/lib
)

if(AUDIO_LIBRARY)
    message(STATUS "Found audio library: ${AUDIO_LIBRARY}")
    set(AUDIO_LIBRARIES ${AUDIO_LIBRARY})
else()
    message(STATUS "No audio library found - audio monitoring features may be limited")
    set(AUDIO_LIBRARIES "")
endif()

# Math library (required for complex calculations)
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    set(MATH_LIBRARIES ${MATH_LIBRARY})
else()
    set(MATH_LIBRARIES "")
endif()

# Platform-specific libraries
if(WIN32)
    set(PLATFORM_LIBRARIES ws2_32 iphlpapi psapi)
elseif(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
    set(PLATFORM_LIBRARIES
        ${COREFOUNDATION_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
    )
elseif(UNIX)
    set(PLATFORM_LIBRARIES pthread rt dl)
endif()

# Link libraries
target_link_libraries(${ETSI_MODULE_NAME}
    PUBLIC
        Qt6::Core
        Qt6::Network
    PRIVATE
        ${MATH_LIBRARIES}
        ${PLATFORM_LIBRARIES}
)

# Link optional libraries if found
if(FFTW3_FOUND)
    target_link_libraries(${ETSI_MODULE_NAME} PRIVATE ${FFTW3_LIBRARIES})
    target_include_directories(${ETSI_MODULE_NAME} PRIVATE ${FFTW3_INCLUDE_DIRS})
endif()

if(AUDIO_LIBRARIES)
    target_link_libraries(${ETSI_MODULE_NAME} PRIVATE ${AUDIO_LIBRARIES})
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${ETSI_MODULE_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-variable
        -ffast-math
        -march=native
    )

    # Enable optimization for release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${ETSI_MODULE_NAME} PRIVATE -O3 -DNDEBUG)
    endif()

    # Enable debug symbols for debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${ETSI_MODULE_NAME} PRIVATE -g -O0)
    endif()

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${ETSI_MODULE_NAME} PRIVATE
        /W3
        /wd4996  # Disable deprecated warnings
        /fp:fast
    )

    # Enable optimization for release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${ETSI_MODULE_NAME} PRIVATE /O2 /DNDEBUG)
    endif()
endif()

# Preprocessor definitions
target_compile_definitions(${ETSI_MODULE_NAME} PRIVATE
    ETSI_VERSION_MAJOR=${ETSI_VERSION_MAJOR}
    ETSI_VERSION_MINOR=${ETSI_VERSION_MINOR}
    ETSI_VERSION_PATCH=${ETSI_VERSION_PATCH}
    ETSI_VERSION_STRING="${ETSI_VERSION}"
)

# Build configuration specific definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${ETSI_MODULE_NAME} PRIVATE
        ETSI_DEBUG
        ETSI_ENABLE_DETAILED_LOGGING
        ETSI_ENABLE_PERFORMANCE_PROFILING
    )
endif()

# Feature flags based on available libraries
if(FFTW3_FOUND)
    target_compile_definitions(${ETSI_MODULE_NAME} PRIVATE ETSI_ENABLE_SPECTRUM_ANALYSIS)
endif()

if(AUDIO_LIBRARIES)
    target_compile_definitions(${ETSI_MODULE_NAME} PRIVATE ETSI_ENABLE_AUDIO_MONITORING)
endif()

# Professional broadcast features
target_compile_definitions(${ETSI_MODULE_NAME} PRIVATE
    ETSI_ENABLE_PROFESSIONAL_FEATURES
    ETSI_ENABLE_BROADCAST_COMPLIANCE
    ETSI_ENABLE_REAL_TIME_MONITORING
    ETSI_ENABLE_EMERGENCY_ALERTS
)

# Thread support
find_package(Threads REQUIRED)
target_link_libraries(${ETSI_MODULE_NAME} PRIVATE Threads::Threads)

# Create alias for easier linking
add_library(ETI::ETSI ALIAS ${ETSI_MODULE_NAME})

# Export ETSI module for other targets
set(ETSI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(ETSI_LIBRARIES ${ETSI_MODULE_NAME} PARENT_SCOPE)

# Installation configuration
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(GNUInstallDirs)

    # Install headers
    install(FILES ${ETSI_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/eti_analyser/etsi
        COMPONENT Development
    )

    # Install library
    install(TARGETS ${ETSI_MODULE_NAME}
        EXPORT ETSITargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install CMake config files
    install(EXPORT ETSITargets
        FILE ETSITargets.cmake
        NAMESPACE ETI::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ETSI
    )

    # Create version file
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/ETSIConfigVersion.cmake"
        VERSION ${ETSI_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Install config version file
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ETSIConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ETSI
    )
endif()

# Build summary
message(STATUS "ETSI Compliance Module Configuration Summary:")
message(STATUS "  Version: ${ETSI_VERSION}")
message(STATUS "  Qt6 Core: ${Qt6Core_VERSION}")
message(STATUS "  Qt6 Network: ${Qt6Network_VERSION}")
message(STATUS "  FFTW3 Support: ${FFTW3_FOUND}")
message(STATUS "  Audio Support: ${AUDIO_LIBRARIES}")
message(STATUS "  Math Library: ${MATH_LIBRARIES}")
message(STATUS "  Platform Libraries: ${PLATFORM_LIBRARIES}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")

# Validation warnings
if(NOT FFTW3_FOUND)
    message(STATUS "")
    message(STATUS "WARNING: FFTW3 not found")
    message(STATUS "  Spectrum analysis and frequency domain features will be disabled")
    message(STATUS "  Install FFTW3 development package for full functionality:")
    if(UNIX AND NOT APPLE)
        message(STATUS "    Ubuntu/Debian: sudo apt-get install libfftw3-dev")
        message(STATUS "    CentOS/RHEL: sudo yum install fftw-devel")
    elseif(APPLE)
        message(STATUS "    macOS: brew install fftw")
    elseif(WIN32)
        message(STATUS "    Windows: Install FFTW3 from http://www.fftw.org/install/windows.html")
    endif()
    message(STATUS "")
endif()

if(NOT AUDIO_LIBRARIES)
    message(STATUS "")
    message(STATUS "WARNING: No audio library found")
    message(STATUS "  Real-time audio monitoring may have limited functionality")
    message(STATUS "  Consider installing audio development libraries:")
    if(UNIX AND NOT APPLE)
        message(STATUS "    Ubuntu/Debian: sudo apt-get install libasound2-dev libpulse-dev")
        message(STATUS "    CentOS/RHEL: sudo yum install alsa-lib-devel pulseaudio-libs-devel")
    elseif(APPLE)
        message(STATUS "    macOS: Audio libraries should be available by default")
    elseif(WIN32)
        message(STATUS "    Windows: Consider installing PortAudio or DirectSound")
    endif()
    message(STATUS "")
endif()

# Success message
message(STATUS "ETSI Compliance Module configured successfully!")
message(STATUS "  Library target: ${ETSI_MODULE_NAME}")
message(STATUS "  Alias target: ETI::ETSI")
message(STATUS "")
