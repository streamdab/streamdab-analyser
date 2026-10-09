cmake_minimum_required(VERSION 3.16)
# PROJECT_VERSION is the fallback used when no git metadata is available (e.g. a
# source tarball). Tagged checkouts build with the tag they are on.
project(DABAnalyser_Enhanced VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Product version -- single source of truth for the GUI, the CLI banner, the
# About dialog and the YAML header. Resolution order:
#   1. -DDABX_VERSION=x.y.z on the command line (release/CI override)
#   2. PROJECT_VERSION from the project() line above (public release = 1.0.0)
#   3. the most recent reachable git tag (leading "v" stripped) -- ONLY if
#      explicitly enabled via -DDABX_USE_GIT_VERSION=ON (not for public release)
# The value is exported to every target as the DABX_VERSION compile definition
# (see src/core/product_version.hpp for the include-time fallback).
# ============================================================================
# Default to PROJECT_VERSION for the public release (1.0.0).
# Git-tag resolution is opt-in via -DDABX_USE_GIT_VERSION=ON.
if(NOT DABX_VERSION)
    set(DABX_VERSION "${PROJECT_VERSION}")
endif()
if(DABX_USE_GIT_VERSION AND NOT DABX_VERSION)
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
#   - ALSA: Linux playback backend (src/core/audio_output_sink.cpp).
#   - Qt Multimedia (QAudioSink): Windows/macOS playback backend
#     (src/core/qt_audio_output_sink.cpp), auto-detected when the Qt kit ships
#     the module (aqt / brew qt@6 do; qtbase-only installs do not).
# All are OPTIONAL and independent: the code degrades gracefully when one of
# them is missing (HAVE_ALSA / HAVE_QMULTIMEDIA simply stay undefined).
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

option(DABX_ENABLE_QMULTIMEDIA
    "Enable the Qt Multimedia audio output backend (Windows/macOS)" ON)
# Same stale-cache discipline as ALSA: a previous configure that found the
# module must not keep HAVE_QMULTIMEDIA defined after -DDABX_ENABLE_QMULTIMEDIA=OFF.
unset(Qt6Multimedia_FOUND CACHE)
set(Qt6Multimedia_FOUND FALSE)
if(DABX_ENABLE_QMULTIMEDIA)
    # QUIET by design: this dev machine has no qt6-multimedia-dev, which is the
    # expected compiled-out configuration here (the guarded code is exercised
    # by the aqt/brew Qt Multimedia builds on Windows/macOS).
    find_package(Qt6 COMPONENTS Multimedia QUIET)
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
    message(STATUS "T34 audio: ALSA found -> Linux audio output enabled")
else()
    message(STATUS "T34 audio: ALSA NOT found -> ALSA output disabled "
                   "(Windows/macOS use the Qt Multimedia backend below)")
endif()
if(Qt6Multimedia_FOUND)
    list(APPEND T34_AUDIO_LIBS Qt6::Multimedia)
    list(APPEND T34_AUDIO_DEFINES HAVE_QMULTIMEDIA)
    message(STATUS "T34 audio: Qt Multimedia found -> Windows/macOS audio output enabled")
else()
    message(STATUS "T34 audio: Qt Multimedia NOT found -> Qt audio output compiled out "
                   "(install qt6-multimedia-dev on Linux, or use the full aqt/brew Qt)")
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

# ============================================================================
# Application Icon Generation (cross-platform)
# ============================================================================
# Generate PNG fallbacks, .ico (Windows), .icns (macOS) from the source SVG
find_program(CONVERT_EXECUTABLE convert DOC "ImageMagick convert for icon generation")
if(NOT CONVERT_EXECUTABLE)
    message(WARNING "ImageMagick 'convert' not found - platform icons will not be generated")
endif()

# Platform-specific icon sizes
set(ICON_SIZES_PNG 16 24 32 48 64 128 256)
set(ICON_SIZES_ICO 16 24 32 48 64 128 256)
set(ICON_SIZES_ICNS 16 32 64 128 256 512 1024)

# Output paths for generated icons
set(ICON_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/icons")
set(ICON_SOURCE_SVG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/eti-stream-analyser.svg")

# Ensure output directories exist at configure time
file(MAKE_DIRECTORY "${ICON_OUTPUT_DIR}")
if(APPLE AND ICONUTIL_EXECUTABLE)
    set(iconset_dir "${ICON_OUTPUT_DIR}/eti-stream-analyser.iconset")
    file(MAKE_DIRECTORY "${iconset_dir}")
endif()

# Generate PNG fallbacks for all platforms (used as Qt resource fallbacks)
if(CONVERT_EXECUTABLE)
    foreach(size ${ICON_SIZES_PNG})
        set(png_output "${ICON_OUTPUT_DIR}/eti-stream-analyser_${size}.png")
        add_custom_command(
            OUTPUT "${png_output}"
            COMMAND ${CONVERT_EXECUTABLE} "${ICON_SOURCE_SVG}" -resize ${size}x${size} -background none "${png_output}"
            DEPENDS "${ICON_SOURCE_SVG}"
            COMMENT "Generating ${size}x${size} PNG icon"
            VERBATIM
        )
        list(APPEND GENERATED_PNG_ICONS "${png_output}")
    endforeach()

    # Generate Windows .ico (multi-resolution)
    if(WIN32)
        set(ico_output "${ICON_OUTPUT_DIR}/eti-stream-analyser.ico")
        set(ico_inputs "")
        foreach(size ${ICON_SIZES_ICO})
            list(APPEND ico_inputs "${ICON_OUTPUT_DIR}/eti-stream-analyser_${size}.png")
        endforeach()
        add_custom_command(
            OUTPUT "${ico_output}"
            COMMAND ${CONVERT_EXECUTABLE} ${ico_inputs} "${ico_output}"
            DEPENDS ${ico_inputs}
            COMMENT "Generating Windows .ico icon"
            VERBATIM
        )
        set(GENERATED_ICO_ICON "${ico_output}")
    endif()

    # Generate macOS .icns (using iconutil on macOS, ImageMagick fallback)
    if(APPLE)
        # On macOS, prefer iconutil which creates proper .icns with all required sizes
        find_program(ICONUTIL_EXECUTABLE iconutil)
        if(ICONUTIL_EXECUTABLE)
            set(iconset_dir "${ICON_OUTPUT_DIR}/eti-stream-analyser.iconset")
            # Generate iconset PNGs
            foreach(size ${ICON_SIZES_ICNS})
                set(iconset_png "${iconset_dir}/icon_${size}x${size}.png")
                add_custom_command(
                    OUTPUT "${iconset_png}"
                    COMMAND ${CONVERT_EXECUTABLE} "${ICON_SOURCE_SVG}" -resize ${size}x${size} -background none "${iconset_png}"
                    DEPENDS "${ICON_SOURCE_SVG}"
                    COMMENT "Generating ${size}x${size} PNG for iconset"
                    VERBATIM
                )
                # @2x variants for Retina
                if(size LESS 512)
                    set(iconset_png_2x "${iconset_dir}/icon_${size}x${size}@2x.png")
                    set(double_size ${size})
                    math(EXPR double_size "${size} * 2")
                    add_custom_command(
                        OUTPUT "${iconset_png_2x}"
                        COMMAND ${CONVERT_EXECUTABLE} "${ICON_SOURCE_SVG}" -resize ${double_size}x${double_size} -background none "${iconset_png_2x}"
                        DEPENDS "${ICON_SOURCE_SVG}"
                        COMMENT "Generating ${size}x${size}@2x PNG for iconset"
                        VERBATIM
                    )
                    list(APPEND ICONSET_PNGS "${iconset_png_2x}")
                endif()
                list(APPEND ICONSET_PNGS "${iconset_png}")
            endforeach()
            set(icns_output "${ICON_OUTPUT_DIR}/eti-stream-analyser.icns")
            add_custom_command(
                OUTPUT "${icns_output}"
                COMMAND ${ICONUTIL_EXECUTABLE} -c icns "${iconset_dir}" -o "${icns_output}"
                DEPENDS ${ICONSET_PNGS}
                COMMENT "Generating macOS .icns icon via iconutil"
                VERBATIM
            )
            set(GENERATED_ICNS_ICON "${icns_output}")
        else()
            # Fallback: ImageMagick can create basic .icns (limited support)
            message(WARNING "iconutil not found, using ImageMagick fallback for .icns (limited)")
            set(icns_output "${ICON_OUTPUT_DIR}/eti-stream-analyser.icns")
            set(icns_inputs "")
            foreach(size ${ICON_SIZES_ICNS})
                list(APPEND icns_inputs "${ICON_OUTPUT_DIR}/eti-stream-analyser_${size}.png")
            endforeach()
            add_custom_command(
                OUTPUT "${icns_output}"
                COMMAND ${CONVERT_EXECUTABLE} ${icns_inputs} "${icns_output}"
                DEPENDS ${icns_inputs}
                COMMENT "Generating macOS .icns icon (ImageMagick fallback)"
                VERBATIM
            )
            set(GENERATED_ICNS_ICON "${icns_output}")
        endif()
    endif()
endif()

# Custom target to build all icons
if(CONVERT_EXECUTABLE)
    set(GENERATED_ICON_OUTPUTS ${GENERATED_PNG_ICONS})
    if(WIN32 AND GENERATED_ICO_ICON)
        list(APPEND GENERATED_ICON_OUTPUTS "${GENERATED_ICO_ICON}")
    endif()
    if(APPLE AND GENERATED_ICNS_ICON)
        list(APPEND GENERATED_ICON_OUTPUTS "${GENERATED_ICNS_ICON}")
    endif()
    add_custom_target(generate_icons DEPENDS ${GENERATED_ICON_OUTPUTS})
    # Make main target depend on icon generation (added after target creation)
endif()

# Copy generated PNGs to source icons directory for Qt resource system
# The .qrc references them as ../icons/eti-stream-analyser_XX.png relative to resources/
if(CONVERT_EXECUTABLE)
    foreach(size ${ICON_SIZES_PNG})
        set(src_png "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/eti-stream-analyser_${size}.png")
        set(build_png "${ICON_OUTPUT_DIR}/eti-stream-analyser_${size}.png")
        add_custom_command(
            OUTPUT "${src_png}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${build_png}" "${src_png}"
            DEPENDS "${build_png}"
            COMMENT "Copying ${size}x${size} PNG icon to source for resources"
            VERBATIM
        )
        list(APPEND COPIED_PNG_ICONS "${src_png}")
    endforeach()
    # Add a target that depends on all copied PNGs
    add_custom_target(copy_icons_to_source DEPENDS ${COPIED_PNG_ICONS})
    # Make copy_icons_to_source depend on generate_icons to ensure proper build order
    if(TARGET generate_icons AND TARGET copy_icons_to_source)
        add_dependencies(copy_icons_to_source generate_icons)
    endif()
endif()

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
    # v1.4 data-path wave: TPEG/TEPG base/partial decoder, per-service data
    # model, and the synchronous ETI scanner used by capture-backed tests and
    # the upcoming GUI data wave.
    src/core/tpeg_decoder.cpp
    src/core/data_service_store.cpp
    src/core/eti_file_scanner.cpp
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

# T34 Qt Multimedia backend: the Qt Multimedia (QAudioSink) sink is only
# compiled when the module was found at configure time. The tests subdirectory
# mirrors this (see tests/CMakeLists.txt: it guards on the inherited
# Qt6Multimedia_FOUND var).
if(Qt6Multimedia_FOUND)
    list(APPEND SOURCES src/core/qt_audio_output_sink.cpp)
    message(STATUS "T34 audio: compiled src/core/qt_audio_output_sink.cpp into the GUI")
endif()

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

# Depend on icon generation (must be after target creation)
if(CONVERT_EXECUTABLE AND TARGET generate_icons)
    add_dependencies(DABAnalyser_Enhanced_FilteringBuild generate_icons)
endif()
if(CONVERT_EXECUTABLE AND TARGET copy_icons_to_source)
    add_dependencies(DABAnalyser_Enhanced_FilteringBuild copy_icons_to_source)
endif()

# Platform-specific icon properties
if(WIN32 AND GENERATED_ICO_ICON)
    set_target_properties(DABAnalyser_Enhanced_FilteringBuild PROPERTIES RC_ICON "${GENERATED_ICO_ICON}")
elseif(APPLE AND GENERATED_ICNS_ICON)
    set_target_properties(DABAnalyser_Enhanced_FilteringBuild PROPERTIES MACOSX_BUNDLE_ICON_FILE "${GENERATED_ICNS_ICON}")
endif()

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

# ============================================================================
# Installation + packaging (production distribution)
#   * install() rules: binaries, Linux desktop/mime/icon, docs
#   * macOS .app bundle properties (MACOSX_BUNDLE_*)
#   * CPack — wired via cmake/packaging.cmake (single branding + per-OS
#     generators: TGZ/DEB on Linux, NSIS/ZIP on Windows, BUNDLE/DragNDrop
#     .app/.dmg on macOS). Must be included AFTER the install() rules so CPack
#     sees the full install manifest.
# Test data (eti/) is deliberately NOT installed.
# ============================================================================

if(APPLE)
    # --- macOS .app bundle ---------------------------------------------------
    # The installed artifact on macOS is StreamDAB-Analyser.app (OUTPUT_NAME
    # applies to the bundle directory); Linux/Windows keep bin/StreamDABAnalyser.
    set_target_properties(DABAnalyser_Enhanced_FilteringBuild PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "StreamDAB-Analyser"
        MACOSX_BUNDLE_BUNDLE_NAME "StreamDAB Analyser"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.streamdab-analyser.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "${DABX_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${DABX_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
    )
    # MACOSX_BUNDLE_ICON_FILE is deliberately OMITTED: the project ships an SVG
    # only, and macOS Finder icons must be .icns (Qt does not accept SVG here).
    # Generating a .icns is a documented follow-up.
    # Keep @rpath-based Qt links working for the installed .app (brew qt@6 uses
    # @rpath); scripts/bundle-macos.sh runs macdeployqt afterwards, which
    # rewrites them to @executable_path/../Frameworks.
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# GUI + CLI binaries. On macOS the GUI installs as the .app bundle
# (BUNDLE DESTINATION .); on Linux/Windows as bin/StreamDABAnalyser.
install(TARGETS DABAnalyser_Enhanced_FilteringBuild
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
install(TARGETS streamdab-cli RUNTIME DESTINATION bin)

if(UNIX AND NOT APPLE)
    # Qt-ADS is a shared library that the GUI links dynamically, and its own
    # install() rules place it in lib/. Embed an $ORIGIN-relative rpath so the
    # installed binary finds it in a stage prefix, a TGZ/AppImage layout and
    # /usr|/usr/local lib dirs alike (the build-tree rpath is not carried over).
    set_target_properties(DABAnalyser_Enhanced_FilteringBuild PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib")
    # .desktop prerequisite check: the desktop file must reference the installed
    # binary (Exec=StreamDABAnalyser) and the icon name, otherwise DEB/MIME
    # desktop integration silently points at a binary that does not exist.
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/eti-stream-analyser.desktop"
        _dabx_desktop_content)
    foreach(_dabx_needle IN ITEMS "Exec=StreamDABAnalyser" "Icon=eti-stream-analyser")
        string(FIND "${_dabx_desktop_content}" "${_dabx_needle}" _dabx_needle_pos)
        if(_dabx_needle_pos EQUAL -1)
            message(FATAL_ERROR
                "resources/linux/eti-stream-analyser.desktop must contain "
                "'${_dabx_needle}' (install()/CPack prerequisite for desktop "
                "integration); fix the file.")
        endif()
    endforeach()
    install(FILES resources/linux/eti-stream-analyser.desktop
        DESTINATION share/applications)
    install(FILES resources/linux/eti-stream-analyser.xml
        DESTINATION share/mime/packages)
    install(FILES resources/icons/eti-stream-analyser.svg
        DESTINATION share/icons/hicolor/scalable/apps)
endif()

# Docs ship with every package (also used as the CPack license/readme
# resources on DEB/NSIS/DMG).
install(FILES LICENSE README.md DESTINATION share/doc/streamdab-analyser)

# CPack branding + per-OS generators (after the install() rules above).
include(cmake/packaging.cmake)

# Add tests subdirectory
# Re-enabled after Phase 1 fixes complete with A+ grade
add_subdirectory(tests)
