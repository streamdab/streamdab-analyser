cmake_minimum_required(VERSION 3.16)
# PROJECT_VERSION is the fallback used when no git metadata is available (e.g. a
# source tarball). Tagged checkouts build with the tag they are on.
project(DABAnalyser_Enhanced VERSION 1.6.0 LANGUAGES CXX)

# ============================================================================
# Product version -- single source of truth for the GUI, the CLI banner, the
# About dialog and the YAML header. Resolution order:
#   1. -DDABX_VERSION=x.y.z on the command line (release/CI override)
#   2. the most recent reachable git tag (leading "v" stripped)
#   3. PROJECT_VERSION from the project() line above
# The value is exported to every target as the DABX_VERSION compile definition
# (see src/core/product_version.hpp for the include-time fallback).
# ============================================================================
if(NOT DABX_VERSION)
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE _dabx_git_tag
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE _dabx_git_rc
        )
        if(_dabx_git_rc EQUAL 0 AND _dabx_git_tag MATCHES "^v?[0-9]+\\.[0-9]+\\.[0-9]+")
            string(REGEX REPLACE "^v" "" DABX_VERSION "${_dabx_git_tag}")
        endif()
    endif()
endif()
if(NOT DABX_VERSION)
    set(DABX_VERSION "${PROJECT_VERSION}")
endif()
message(STATUS "Product version (DABX_VERSION): ${DABX_VERSION}")
add_compile_definitions(DABX_VERSION="${DABX_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 setup with automatic MOC
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Test)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Threads (portable replacement for a bare `pthread`: empty on MSVC, -pthread
# on GCC/Clang). Never link `pthread` directly -- it does not exist on Windows.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# ============================================================================
# pkg-config (OPTIONAL). Windows has no pkg-config by default and vcpkg's .pc
# files are not guaranteed, so it must NEVER be REQUIRED. Every dependency
# below has a non-pkg-config fallback.
# ============================================================================
find_package(PkgConfig QUIET)

# ============================================================================
# ZeroMQ (OPTIONAL; T48 zmq+tcp:// ETI transport).
# Portable detection ladder, with no hard pkg-config dependency:
#   1. CMake config packages -- vcpkg ships `zeromq` (libzmq) and `cppzmq`
#      (the cppzmq::cppzmq interface target carrying <zmq.hpp>).
#   2. pkg-config `libzmq` (Linux/macOS), only when PkgConfig was found.
#   3. find_path/find_library fallback with vcpkg-style + CMAKE_PREFIX_PATH
#      hints.
# HAVE_ZMQ (and the include dirs/link target) is defined ONLY when both the
# C++ header <zmq.hpp> and libzmq are available. Otherwise the transport is
# compiled out (network_stream_receiver.cpp) and the validator rejects
# zmq+tcp:// with a clear "not built in" message. Force off with
# -DDABX_ENABLE_ZMQ=OFF.
# ============================================================================
option(DABX_ENABLE_ZMQ "Enable the ZeroMQ (zmq+tcp://) ETI stream transport" ON)

# Defensively drop detection results cached by a PREVIOUS configure of the same
# build directory. pkg_check_modules() stores <prefix>_FOUND as an INTERNAL
# cache variable, so a stale ZMQ_FOUND=1 would otherwise survive with the
# option OFF (the guard skips the lookup but the cached TRUE is still read by
# the ladder's `if(ZMQ_FOUND)`). ZMQ_HAVE_ZMQ is a normal variable recomputed
# every configure, but the cache vars must be cleared for ON->OFF->ON to flip.
unset(ZMQ_FOUND CACHE)
unset(ZMQ_FOUND)
set(ZMQ_HAVE_ZMQ FALSE)
set(ZMQ_LIBRARIES "")
set(ZMQ_INCLUDE_DIRS "")
set(ZMQ_LIBRARY_DIRS "")
set(ZMQ_DEFINES "")

if(DABX_ENABLE_ZMQ)
    # --- 1) CMake config packages (vcpkg: zeromq + cppzmq) ----------------
    find_package(ZeroMQ CONFIG QUIET)
    find_package(cppzmq CONFIG QUIET)

    if(TARGET cppzmq::cppzmq)
        set(ZMQ_HAVE_ZMQ TRUE)
        set(ZMQ_LIBRARIES cppzmq::cppzmq)
    elseif(TARGET cppzmq)
        set(ZMQ_HAVE_ZMQ TRUE)
        set(ZMQ_LIBRARIES cppzmq)
    endif()

    # --- 2) pkg-config libzmq (Linux/macOS) ------------------------------
    if(NOT ZMQ_HAVE_ZMQ AND PkgConfig_FOUND)
        pkg_check_modules(ZMQ QUIET libzmq)
        if(ZMQ_FOUND)
            # libzmq ships zmq.h; the C++ bindings header zmq.hpp (cppzmq) must
            # also be present for <zmq.hpp>. On Debian/Ubuntu libzmq3-dev
            # provides both.
            find_path(ZMQ_CPP_HEADER zmq.hpp HINTS ${ZMQ_INCLUDE_DIRS})
            if(ZMQ_CPP_HEADER)
                set(ZMQ_HAVE_ZMQ TRUE)
                set(ZMQ_LIBRARIES ${ZMQ_LINK_LIBRARIES})
                set(ZMQ_LIBRARY_DIRS ${ZMQ_LIBRARY_DIRS})
                list(APPEND ZMQ_INCLUDE_DIRS ${ZMQ_CPP_HEADER})
            endif()
        endif()
    endif()

    # --- 3) find_path / find_library fallback ----------------------------
    if(NOT ZMQ_HAVE_ZMQ)
        set(_zmq_hints "")
        if(DEFINED ENV{VCPKG_ROOT})
            list(APPEND _zmq_hints
                "$ENV{VCPKG_ROOT}/installed/x64-windows"
                "$ENV{VCPKG_ROOT}/installed/x64-linux"
                "$ENV{VCPKG_ROOT}/installed/x64-osx")
        endif()
        find_path(ZMQ_INCLUDE_DIR NAMES zmq.hpp
                  HINTS ${_zmq_hints} PATH_SUFFIXES include)
        find_library(ZMQ_LIBRARY NAMES libzmq libzmq-mt zeromq zmq
                     HINTS ${_zmq_hints} PATH_SUFFIXES lib)
        if(ZMQ_INCLUDE_DIR AND ZMQ_LIBRARY)
            set(ZMQ_HAVE_ZMQ TRUE)
            set(ZMQ_LIBRARIES ${ZMQ_LIBRARY})
            list(APPEND ZMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIR})
        endif()
    endif()

    if(ZMQ_HAVE_ZMQ)
        list(APPEND ZMQ_DEFINES HAVE_ZMQ)
        message(STATUS "ZeroMQ: found -> zmq+tcp:// ETI transport enabled")
    else()
        message(STATUS "ZeroMQ: NOT found -> zmq+tcp:// transport disabled "
                       "(install zeromq + cppzmq; or -DDABX_ENABLE_ZMQ=OFF)")
    endif()
else()
    message(STATUS "ZeroMQ: disabled (-DDABX_ENABLE_ZMQ=OFF) -> zmq+tcp:// transport disabled")
endif()

if(ZMQ_INCLUDE_DIRS)
    include_directories(${ZMQ_INCLUDE_DIRS})
endif()

# ============================================================================
# T34: audio decode/output dependencies.
#   - libfaad2: DAB+ HE-AAC decode (no faad.pc is installed on this host, so
#     locate the header/library directly).
#   - ALSA: playback sink (the project deliberately avoids Qt Multimedia).
# Both are OPTIONAL: the code degrades gracefully when either is missing.
# ============================================================================
find_path(FAAD2_INCLUDE_DIR neaacdec.h)
find_library(FAAD2_LIBRARY NAMES faad)
option(DABX_ENABLE_ALSA "Enable the ALSA audio output backend (Linux only)" ON)
# Clear the cached ALSA detection result first: pkg_check_modules() stores
# ALSA_FOUND as an INTERNAL cache variable, so on an incremental reconfigure
# with -DDABX_ENABLE_ALSA=OFF the guard would skip the lookup while the stale
# ALSA_FOUND=1 kept HAVE_ALSA defined. Reset both the cache and the normal var.
unset(ALSA_FOUND CACHE)
set(ALSA_FOUND FALSE)
if(DABX_ENABLE_ALSA AND PkgConfig_FOUND)
    pkg_check_modules(ALSA alsa)
endif()

set(T34_AUDIO_LIBS "")
set(T34_AUDIO_INCLUDES "")
set(T34_AUDIO_DEFINES "")
if(FAAD2_INCLUDE_DIR AND FAAD2_LIBRARY)
    list(APPEND T34_AUDIO_LIBS ${FAAD2_LIBRARY})
    list(APPEND T34_AUDIO_INCLUDES ${FAAD2_INCLUDE_DIR})
    list(APPEND T34_AUDIO_DEFINES HAVE_FAAD2)
    message(STATUS "T34 audio: libfaad2 found -> DAB+ HE-AAC decode enabled")
else()
    message(STATUS "T34 audio: libfaad2 NOT found -> audio decode disabled")
endif()
if(ALSA_FOUND)
    # ALSA_LINK_LIBRARIES holds full paths (robust across distros).
    list(APPEND T34_AUDIO_LIBS ${ALSA_LINK_LIBRARIES})
    list(APPEND T34_AUDIO_INCLUDES ${ALSA_INCLUDE_DIRS})
    list(APPEND T34_AUDIO_DEFINES HAVE_ALSA)
    message(STATUS "T34 audio: ALSA found -> audio output enabled")
else()
    message(STATUS "T34 audio: ALSA NOT found -> audio output disabled "
                   "(no playback backend on Windows/macOS yet; decode still works)")
endif()

# Qt-Advanced-Docking-System (Qt-ADS) via FetchContent
include(FetchContent)
FetchContent_Declare(
    QtAdvancedDockingSystem
    GIT_REPOSITORY https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git
    GIT_TAG 87cffe5d40307c61141c88b88c10c5e7d181d880 # 4.5.0
)
# Qt-ADS ships examples + a Qt6Quick demo app; we only need the library and
# Qt6Quick is not installed. Force-disable them before MakeAvailable.
set(BUILD_EXAMPLES OFF CACHE BOOL "Build Qt-ADS examples/demo" FORCE)
FetchContent_MakeAvailable(QtAdvancedDockingSystem)

# Enhanced build flags (LTO disabled to avoid compiler internal error).
# GCC/Clang keep the project's tuned -O3 flags; MSVC cannot parse them and uses
# its own equivalents instead (a bare `-march=native` breaks every MSVC build).
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /DRELEASE_BUILD")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -funroll-loops -finline-functions -DRELEASE_BUILD")
    # -march/-mtune=native are x86_64-only: Apple Clang on arm64 rejects both
    # (docs/CROSS_PLATFORM_BUILD_PLAN.md §3.1).
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
        string(APPEND CMAKE_CXX_FLAGS_RELEASE " -march=native -mtune=native")
    endif()
endif()
set(CMAKE_BUILD_TYPE Release)

# Source files
set(SOURCES
    src/main.cpp
    src/core/enhanced_eti_processor_qt.cpp
    src/core/advanced_fig_analyser.cpp
    src/core/charset_converter.cpp
    src/core/etsi_compliance_engine.cpp
    src/core/etsi_registered_tables.cpp
    src/core/multi_stream_processor.cpp
    src/core/advanced_error_detection.cpp
    # T41/F11: professional_logging_system.cpp is no longer referenced by any
    # translation unit (the legacy Messages UI was removed). List-ed only here
    # before; nothing in src/ or tests/ includes professional_logging_system.hpp.
    # Files kept for reference/history; not compiled.
    src/core/hex_viewer_formatter.cpp
    src/core/dabplus_stream_validator.cpp
    # T38: shared DAB+ superframe/AU/PAD transport (main.cpp + audio decoder).
    src/core/dabplus_superframe.cpp
    src/core/mot_protocol.cpp
    src/core/mot_pad_adapter.cpp
    src/core/epg_decoder.cpp
    src/core/journaline_decoder.cpp
    src/core/dls_plus_decoder.cpp
    # T34: DAB+ audio decode (libfaad2) + ALSA output + selected-service playback.
    src/core/dabplus_audio_decoder.cpp
    src/core/audio_output_sink.cpp
    src/core/audio_playback_controller.cpp
    src/core/phase3a_integration.cpp
    src/core/network_stream_receiver.cpp
    src/core/eti_reader_thread.cpp
    src/core/shared_eti_data.cpp
    src/core/headless_eti_processor.cpp
    src/core/eti_processor.cpp
    src/core/modern_eti_frame_parser.cpp
    src/core/analyser_settings.cpp
    src/core/eti_types.cpp
    src/core/performance_profiler.cpp
    src/core/enhanced_fig_analyser.cpp
    src/core/comprehensive_etsi_validator.cpp
    src/core/ensemble_manager.cpp
    src/core/dab_service_discovery_engine.cpp
    src/core/fig_parser.cpp
    src/utils/logger.cpp
    # Adopted src/gui widgets (Wave B). Each was a stale pre-v1.3 generation
    # file, fixed minimally to compile against the current src/core and driven
    # by DABAnalyserWindow adapters (no fake data).
    src/gui/constellation_widget.cpp
    src/gui/about_dialog.cpp
    src/gui/performance_dashboard.cpp
    src/gui/real_time_chart_widget.cpp
    src/gui/settings_dialog.cpp
    src/gui/etsi_compliance_monitor.cpp
    src/gui/professional_export_manager.cpp
)

# EDI Protocol sources (ETSI TS 102 693)
set(EDI_SOURCES
    src/network/edi_packet_parser.cpp
    src/network/pft_reassembler.cpp
)

# CLI Mode sources (PDCA Week 8)
set(CLI_SOURCES
    src/cli/cli_argument_parser.cpp
    src/cli/yaml_output_generator.cpp
    src/cli/cli_main.cpp
)

# Include directories
include_directories(src include)

# === Main GUI Application ===
# Create executable
add_executable(DABAnalyser_Enhanced_FilteringBuild ${SOURCES} ${EDI_SOURCES} ${CLI_SOURCES})

# Runnable name (tests keep referencing the CMake target name above)
set_target_properties(DABAnalyser_Enhanced_FilteringBuild PROPERTIES OUTPUT_NAME StreamDABAnalyser)

# Link Qt6, ZeroMQ, and Qt-ADS libraries
# Qt-ADS 4.5.0 exports its include dir via $<BUILD_INTERFACE>, so no manual
# include-directory hack is needed. Target name: qtadvanceddocking-qt6.
target_link_libraries(DABAnalyser_Enhanced_FilteringBuild
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
    qtadvanceddocking-qt6
    ${ZMQ_LIBRARIES}
    ${T34_AUDIO_LIBS}
    Threads::Threads
)

# T34: audio decode/output includes + feature defines (HAVE_FAAD2 / HAVE_ALSA).
# ZMQ_DEFINES carries HAVE_ZMQ only when the ZeroMQ transport was found.
target_include_directories(DABAnalyser_Enhanced_FilteringBuild PRIVATE ${T34_AUDIO_INCLUDES})
target_compile_definitions(DABAnalyser_Enhanced_FilteringBuild PRIVATE ${T34_AUDIO_DEFINES} ${ZMQ_DEFINES})

# Add ZeroMQ library directories
target_link_directories(DABAnalyser_Enhanced_FilteringBuild PRIVATE ${ZMQ_LIBRARY_DIRS})

# === CLI-Only Executable ===
# All dependencies needed for CLI mode (no GUI, no network)
# NOTE: comprehensive_etsi_validator.cpp removed - all methods commented out due to MOC errors
# NOTE: professional_logging_system.cpp removed - depends on QApplication (GUI only)
set(CLI_ONLY_SOURCES
    src/streamdab-cli.cpp
    src/cli/cli_argument_parser.cpp
    src/cli/yaml_output_generator.cpp
    src/cli/cli_main.cpp
    src/core/headless_eti_processor.cpp
    src/core/eti_processor.cpp
    src/core/eti_types.cpp
    src/core/modern_eti_frame_parser.cpp
    src/core/analyser_settings.cpp
    src/core/enhanced_fig_analyser.cpp
    src/core/performance_profiler.cpp
    # src/core/comprehensive_etsi_validator.cpp  # REMOVED: All methods commented out
    src/core/fig_parser.cpp
    src/core/ensemble_manager.cpp
    src/core/dab_service_discovery_engine.cpp
    src/core/etsi_registered_tables.cpp
    src/core/charset_converter.cpp
    src/core/etsi_compliance_engine.cpp
    # src/core/professional_logging_system.cpp  # REMOVED: Requires QApplication (GUI only)
    src/utils/logger.cpp
)

add_executable(streamdab-cli ${CLI_ONLY_SOURCES})
target_link_libraries(streamdab-cli Qt6::Core)
target_include_directories(streamdab-cli PRIVATE src include)

# Find GTest (libgtest-dev installed)
find_package(GTest REQUIRED)

# Enable testing
enable_testing()

# Add tests subdirectory
# Re-enabled after Phase 1 fixes complete with A+ grade
add_subdirectory(tests)
