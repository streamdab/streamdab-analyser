# Simplified Test Configuration - PDCA Methodology
# Phase 1: Core ETI Processing Tests Only

cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Simplified Test Framework - PDCA Workflow ===")

# Find Qt6 Test module
find_package(Qt6 REQUIRED COMPONENTS Test Core Widgets Network)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include project source directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Source files from main project needed for tests
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/enhanced_eti_processor_qt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/advanced_fig_analyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_compliance_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dabplus_stream_validator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/mot_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/epg_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/journaline_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dls_plus_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_reader_thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/shared_eti_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)

# Network sources for EDI tests
set(NETWORK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/network/edi_packet_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/network/pft_reassembler.cpp
)

# ============================================================================
# CORE TESTS - Keep essential validation tests only
# ============================================================================

# Test 1: ETI Frame Parser
add_executable(test_eti_frame_parser
    test_eti_frame_parser.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_eti_frame_parser
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Network
    pthread
)
add_test(NAME test_eti_frame_parser COMMAND test_eti_frame_parser)

# Test 2: FIG Type 0 Processor
add_executable(test_fig0_processor
    test_fig0_processor.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig0_processor
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_fig0_processor COMMAND test_fig0_processor)

# Test 3: FIG Type 1 Processor
add_executable(test_fig1_processor
    test_fig1_processor.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig1_processor
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_fig1_processor COMMAND test_fig1_processor)

# Test 4: ETI LI CRC Validation
add_executable(test_eti_li_crc_validation
    test_eti_li_crc_validation.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_eti_li_crc_validation
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_eti_li_crc_validation COMMAND test_eti_li_crc_validation)

# Test 5: Service Discovery
add_executable(test_service_discovery
    test_service_discovery.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_service_discovery
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_service_discovery COMMAND test_service_discovery)

# Test 6: Charset Converter (Thai UTF-8)
add_executable(test_charset_converter
    test_charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
)
target_link_libraries(test_charset_converter
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_charset_converter COMMAND test_charset_converter)

# ============================================================================
# PHASE 3A TESTS - Audio & Data Services (October 24, 2025)
# ============================================================================

# Wave 1.1: DAB+ Stream Validator Tests
add_executable(test_dabplus_stream_validator
    test_dabplus_stream_validator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dabplus_stream_validator.cpp
)
target_link_libraries(test_dabplus_stream_validator
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_dabplus_stream_validator COMMAND test_dabplus_stream_validator)

# Wave 1.2: MOT Protocol Tests
add_executable(test_mot_protocol
    test_mot_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/mot_protocol.cpp
)
target_link_libraries(test_mot_protocol
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_mot_protocol COMMAND test_mot_protocol)

# Wave 2.1: EPG Decoder Tests
add_executable(test_epg_decoder
    test_epg_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/epg_decoder.cpp
)
target_link_libraries(test_epg_decoder
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_epg_decoder COMMAND test_epg_decoder)

# Wave 2.2: Journaline Decoder Tests
add_executable(test_journaline_decoder
    test_journaline_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/journaline_decoder.cpp
)
target_link_libraries(test_journaline_decoder
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_journaline_decoder COMMAND test_journaline_decoder)

# ============================================================================
# PHASE 3B TESTS - DLS+ Dynamic Label Plus (October 25, 2025)
# ============================================================================

add_executable(test_dls_plus_decoder
    test_dls_plus_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dls_plus_decoder.cpp
)
target_link_libraries(test_dls_plus_decoder
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_dls_plus_decoder COMMAND test_dls_plus_decoder)

# ============================================================================
# PHASE 4 TESTS - ETI-over-IP Live Streaming (October 25, 2025)
# ============================================================================

# Test 1: Network Stream Receiver (4,000 lines pre-existing backend)
add_executable(test_network_stream_receiver
    test_network_stream_receiver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/network/network_stream_receiver.cpp
)
target_link_libraries(test_network_stream_receiver
    Qt6::Test
    Qt6::Core
    Qt6::Network
    pthread
)
add_test(NAME test_network_stream_receiver COMMAND test_network_stream_receiver)

# Test 2: Live ETI Frame Processing
add_executable(test_live_eti_processing
    test_live_eti_processing.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_live_eti_processing
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_live_eti_processing COMMAND test_live_eti_processing)

# Test 3: Stream Recording
add_executable(test_stream_recording
    test_stream_recording.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/network/network_stream_receiver.cpp
)
target_link_libraries(test_stream_recording
    Qt6::Test
    Qt6::Core
    Qt6::Network
    pthread
)
add_test(NAME test_stream_recording COMMAND test_stream_recording)

# ============================================================================
# CODE REVIEW FIX TESTS - Security & Validation (October 25, 2025)
# ============================================================================

add_executable(test_security_fixes
    test_security_fixes.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_security_fixes
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_security_fixes COMMAND test_security_fixes)

# ============================================================================
# PDCA WEEK 1 TESTS - Multi-Agent ETSI Standards (October 28, 2025)
# ============================================================================

# Agent 1: FIG 1/0 Ensemble Label Parser
add_executable(test_fig10_ensemble_label
    test_fig10_ensemble_label.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig10_ensemble_label
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig10_ensemble_label COMMAND test_fig10_ensemble_label)

# Agent 4: ETSI TS 101 756 Registered Tables
add_executable(test_etsi_registered_tables
    test_etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
)
target_link_libraries(test_etsi_registered_tables
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_etsi_registered_tables COMMAND test_etsi_registered_tables)

# Agent 3: Comprehensive Integration Test Suite
add_executable(test_fig_week1_comprehensive
    core/test_fig_week1_comprehensive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig_week1_comprehensive
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig_week1_comprehensive COMMAND test_fig_week1_comprehensive)

# ============================================================================
# PDCA WEEK 2 TESTS - FIG 0/10, 0/17, 0/18, 0/19 (November 2, 2025)
# ============================================================================

# FIG 0/5 Language Test
add_executable(test_fig005_language
    test_fig005_language.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig005_language
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig005_language COMMAND test_fig005_language)

# FIG 0/17 Programme Type Test
add_executable(test_fig017_programme_type
    test_fig017_programme_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig017_programme_type
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig017_programme_type COMMAND test_fig017_programme_type)

# FIG Week 2 Comprehensive Integration Test
add_executable(test_fig_week2_comprehensive
    core/test_fig_week2_comprehensive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig_week2_comprehensive
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig_week2_comprehensive COMMAND test_fig_week2_comprehensive)

# ============================================================================
# GUI TESTS - Phase 5 (October 25, 2025)
# ============================================================================

message(STATUS "")
message(STATUS "=== GUI Test Configuration ===")

# Check if GUI tests can be run (need X11 or offscreen platform)
if(DEFINED ENV{DISPLAY} OR DEFINED ENV{QT_QPA_PLATFORM})
    set(CAN_RUN_GUI_TESTS ON)
    message(STATUS "  GUI tests enabled (display environment detected)")
else()
    set(CAN_RUN_GUI_TESTS OFF)
    message(STATUS "  GUI tests disabled (no display environment)")
    message(STATUS "    Set QT_QPA_PLATFORM=offscreen to enable")
endif()

if(CAN_RUN_GUI_TESTS)
    message(STATUS "  Configuring 4 GUI test suites...")

    # Test 1: Stream Info Panel Widget (October 25, 2025) - MOCK TEST (no external dependencies)
    add_executable(test_gui_stream_info_panel
        gui/test_gui_stream_info_panel.cpp
    )
    target_link_libraries(test_gui_stream_info_panel
        Qt6::Test Qt6::Core Qt6::Widgets
    )
    add_test(NAME test_gui_stream_info_panel
             COMMAND test_gui_stream_info_panel)
    set_tests_properties(test_gui_stream_info_panel PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )

    # Test 2: Service Discovery Panel Widget (November 2, 2025) - MOCK TEST
    add_executable(test_gui_service_discovery_panel
        gui/test_gui_service_discovery_panel.cpp
    )
    target_link_libraries(test_gui_service_discovery_panel
        Qt6::Test Qt6::Core Qt6::Widgets
    )
    add_test(NAME test_gui_service_discovery_panel
             COMMAND test_gui_service_discovery_panel)
    set_tests_properties(test_gui_service_discovery_panel PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )

    # Test 3: FIG Type Display Panel Widget (November 2, 2025) - MOCK TEST
    add_executable(test_gui_fig_type_display_panel
        gui/test_gui_fig_type_display_panel.cpp
    )
    target_link_libraries(test_gui_fig_type_display_panel
        Qt6::Test Qt6::Core Qt6::Widgets
    )
    add_test(NAME test_gui_fig_type_display_panel
             COMMAND test_gui_fig_type_display_panel)
    set_tests_properties(test_gui_fig_type_display_panel PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )

    # Test 4: Main Window Integration (November 2, 2025) - MOCK TEST
    add_executable(test_gui_main_window
        gui/test_gui_main_window.cpp
    )
    target_link_libraries(test_gui_main_window
        Qt6::Test Qt6::Core Qt6::Widgets
    )
    add_test(NAME test_gui_main_window
             COMMAND test_gui_main_window)
    set_tests_properties(test_gui_main_window PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )

    message(STATUS "  ✓ test_gui_stream_info_panel (10 widget tests)")
    message(STATUS "  ✓ test_gui_service_discovery_panel (13 widget tests)")
    message(STATUS "  ✓ test_gui_fig_type_display_panel (15 widget tests)")
    message(STATUS "  ✓ test_gui_main_window (16 integration tests)")

else()
    message(STATUS "  Skipping GUI tests (no display environment)")
endif()

# ============================================================================
# E2E TESTS - Phase 3A Wave 3.2 (October 24, 2025)
# ============================================================================

message(STATUS "")
message(STATUS "=== E2E Test Configuration ===")

add_executable(test_e2e_eti_to_audio
    e2e/test_e2e_eti_to_audio.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_e2e_eti_to_audio
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_e2e_eti_to_audio COMMAND test_e2e_eti_to_audio)

add_executable(test_e2e_mot_slideshow
    e2e/test_e2e_mot_slideshow.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_e2e_mot_slideshow
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_e2e_mot_slideshow COMMAND test_e2e_mot_slideshow)

add_executable(test_e2e_epg_journaline
    e2e/test_e2e_epg_journaline.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_e2e_epg_journaline
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_e2e_epg_journaline COMMAND test_e2e_epg_journaline)

message(STATUS "  ✓ test_e2e_eti_to_audio (ETI → DAB+ validation)")
message(STATUS "  ✓ test_e2e_mot_slideshow (MOT SlideShow pipeline)")
message(STATUS "  ✓ test_e2e_epg_journaline (EPG + Journaline)")

# ============================================================================
# NETWORK/EDI TESTS - Advanced protocols
# ============================================================================

message(STATUS "")
message(STATUS "=== Network/EDI Test Configuration ===")

add_executable(test_edi_packet_parser
    network/test_edi_packet_parser.cpp
    ${NETWORK_SOURCES}
)
target_link_libraries(test_edi_packet_parser
    Qt6::Test
    Qt6::Core
    Qt6::Network
    pthread
)
add_test(NAME test_edi_packet_parser COMMAND test_edi_packet_parser)

add_executable(test_pft_reassembler
    network/test_pft_reassembler.cpp
    ${NETWORK_SOURCES}
)
target_link_libraries(test_pft_reassembler
    Qt6::Test
    Qt6::Core
    Qt6::Network
    pthread
)
add_test(NAME test_pft_reassembler COMMAND test_pft_reassembler)

message(STATUS "  ✓ test_edi_packet_parser (ETSI TS 102 693)")
message(STATUS "  ✓ test_pft_reassembler (Protocol FEC Transport)")

# ============================================================================
# TEST SUMMARY
# ============================================================================

message(STATUS "")
message(STATUS "=== Test Framework Summary ===")
message(STATUS "  Core Tests: 6 (ETI processing, FIG parsing, service discovery)")
message(STATUS "  Phase 3A Tests: 4 (DAB+, MOT, EPG, Journaline)")
message(STATUS "  Phase 3B Tests: 1 (DLS+)")
message(STATUS "  Phase 4 Tests: 3 (Live streaming, recording)")
message(STATUS "  Security Tests: 1 (Code review fixes)")
message(STATUS "  PDCA Week 1 Tests: 3 (FIG 1/0, TS 101 756, integration)")
message(STATUS "  PDCA Week 2 Tests: 3 (FIG 0/5, 0/17, integration)")
if(CAN_RUN_GUI_TESTS)
    message(STATUS "  GUI Tests: 4 (Widgets and main window)")
else()
    message(STATUS "  GUI Tests: 0 (disabled - no display)")
endif()
message(STATUS "  E2E Tests: 3 (Full pipeline validation)")
message(STATUS "  Network/EDI Tests: 2 (EDI, PFT)")
message(STATUS "")
if(CAN_RUN_GUI_TESTS)
    message(STATUS "  Total Test Executables: 30")
else()
    message(STATUS "  Total Test Executables: 26")
endif()
message(STATUS "==============================================")
