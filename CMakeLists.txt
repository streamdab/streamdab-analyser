cmake_minimum_required(VERSION 3.16)
project(DABAnalyzer_Enhanced)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 setup with automatic MOC
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Test)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ZeroMQ library (libzmq3-dev installed)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
include_directories(${ZMQ_INCLUDE_DIRS})

# ============================================================================
# T34: audio decode/output dependencies.
#   - libfaad2: DAB+ HE-AAC decode (no faad.pc is installed on this host, so
#     locate the header/library directly).
#   - ALSA: playback sink (the project deliberately avoids Qt Multimedia).
# Both are OPTIONAL: the code degrades gracefully when either is missing.
# ============================================================================
find_path(FAAD2_INCLUDE_DIR neaacdec.h)
find_library(FAAD2_LIBRARY NAMES faad)
pkg_check_modules(ALSA alsa)

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
    message(STATUS "T34 audio: ALSA NOT found -> audio output disabled")
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

# Enhanced build flags (LTO disabled to avoid compiler internal error)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -ffast-math -funroll-loops -finline-functions -DRELEASE_BUILD")
set(CMAKE_BUILD_TYPE Release)

# Source files
set(SOURCES
    src/main.cpp
    src/core/enhanced_eti_processor_qt.cpp
    src/core/advanced_fig_analyzer.cpp
    src/core/charset_converter.cpp
    src/core/etsi_compliance_engine.cpp
    src/core/etsi_registered_tables.cpp
    src/core/multi_stream_processor.cpp
    src/core/advanced_error_detection.cpp
    src/core/professional_logging_system.cpp
    src/core/hex_viewer_formatter.cpp
    src/core/dabplus_stream_validator.cpp
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
    src/core/analyzer_settings.cpp
    src/core/eti_types.cpp
    src/core/performance_profiler.cpp
    src/core/enhanced_fig_analyzer.cpp
    src/core/comprehensive_etsi_validator.cpp
    src/core/ensemble_manager.cpp
    src/core/dab_service_discovery_engine.cpp
    src/core/fig_parser.cpp
    src/utils/logger.cpp
    # Adopted src/gui widgets (Wave B). Each was a stale pre-v1.3 generation
    # file, fixed minimally to compile against the current src/core and driven
    # by DABAnalyzerWindow adapters (no fake data).
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
add_executable(DABAnalyzer_Enhanced_FilteringBuild ${SOURCES} ${EDI_SOURCES} ${CLI_SOURCES})

# Runnable name (tests keep referencing the CMake target name above)
set_target_properties(DABAnalyzer_Enhanced_FilteringBuild PROPERTIES OUTPUT_NAME StreamDABAnalyzer)

# Link Qt6, ZeroMQ, and Qt-ADS libraries
# Qt-ADS 4.5.0 exports its include dir via $<BUILD_INTERFACE>, so no manual
# include-directory hack is needed. Target name: qtadvanceddocking-qt6.
target_link_libraries(DABAnalyzer_Enhanced_FilteringBuild
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
    qtadvanceddocking-qt6
    ${ZMQ_LIBRARIES}
    ${T34_AUDIO_LIBS}
    pthread
)

# T34: audio decode/output includes + feature defines (HAVE_FAAD2 / HAVE_ALSA).
target_include_directories(DABAnalyzer_Enhanced_FilteringBuild PRIVATE ${T34_AUDIO_INCLUDES})
target_compile_definitions(DABAnalyzer_Enhanced_FilteringBuild PRIVATE ${T34_AUDIO_DEFINES})

# Add ZeroMQ library directories
target_link_directories(DABAnalyzer_Enhanced_FilteringBuild PRIVATE ${ZMQ_LIBRARY_DIRS})

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
    src/core/analyzer_settings.cpp
    src/core/enhanced_fig_analyzer.cpp
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
