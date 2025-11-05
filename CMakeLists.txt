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
    src/core/epg_decoder.cpp
    src/core/journaline_decoder.cpp
    src/core/dls_plus_decoder.cpp
    src/core/phase3a_integration.cpp
    src/core/network_stream_receiver.cpp
    src/core/eti_reader_thread.cpp
    src/core/shared_eti_data.cpp
    src/core/headless_eti_processor.cpp
    src/core/eti_processor.cpp
    src/core/modern_eti_frame_parser.cpp
    src/core/enhanced_fig_analyzer.cpp
    src/core/comprehensive_etsi_validator.cpp
    src/core/ensemble_manager.cpp
    src/core/dab_service_discovery_engine.cpp
    src/core/fig_parser.cpp
    src/utils/logger.cpp
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

# Create executable
add_executable(DABAnalyzer_Enhanced_FilteringBuild ${SOURCES} ${EDI_SOURCES} ${CLI_SOURCES})

# Link Qt6 and ZeroMQ libraries
target_link_libraries(DABAnalyzer_Enhanced_FilteringBuild
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
    ${ZMQ_LIBRARIES}
    pthread
)

# Add ZeroMQ library directories
target_link_directories(DABAnalyzer_Enhanced_FilteringBuild PRIVATE ${ZMQ_LIBRARY_DIRS})

# Enable testing
enable_testing()

# Add tests subdirectory
# Re-enabled after Phase 1 fixes complete with A+ grade
add_subdirectory(tests)
