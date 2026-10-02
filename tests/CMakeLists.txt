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

# Test 4.5: ETI Format Auto-Detection (Agent 43 - November 7, 2025)
add_executable(test_eti_format_detection
    test_eti_format_detection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_types.cpp
)
target_link_libraries(test_eti_format_detection
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_eti_format_detection COMMAND test_eti_format_detection)

# Test 4.6: ETI-NI Parser Unit Tests (Agent 46 - November 8, 2025)
add_executable(test_eti_ni_parser
    test_eti_ni_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/modern_eti_frame_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyzer_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/enhanced_fig_analyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/performance_profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_eti_ni_parser
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_eti_ni_parser COMMAND test_eti_ni_parser)

# Test 5: Service Discovery
#add_executable(test_service_discovery
#    test_service_discovery.cpp
#    ${CORE_SOURCES}
#)
#target_link_libraries(test_service_discovery
#    Qt6::Test
#    Qt6::Core
#    Qt6::Widgets
#    Qt6::Network
#    pthread
#)
#add_test(NAME test_service_discovery COMMAND test_service_discovery)

# Test 6: Charset Converter (Thai UTF-8 + EBU Latin, F4)
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
#add_executable(test_dabplus_stream_validator
#    test_dabplus_stream_validator.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dabplus_stream_validator.cpp
#)
#target_link_libraries(test_dabplus_stream_validator
#    Qt6::Test
#    Qt6::Core
#)
#add_test(NAME test_dabplus_stream_validator COMMAND test_dabplus_stream_validator)

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
#add_executable(test_network_stream_receiver
#    test_network_stream_receiver.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/../src/network/network_stream_receiver.cpp
#)
#target_link_libraries(test_network_stream_receiver
#    Qt6::Test
#    Qt6::Core
#    Qt6::Network
#    pthread
#)
#add_test(NAME test_network_stream_receiver COMMAND test_network_stream_receiver)

# Test 2: Live ETI Frame Processing
#add_executable(test_live_eti_processing
#    test_live_eti_processing.cpp
#    ${CORE_SOURCES}
#)
#target_link_libraries(test_live_eti_processing
#    Qt6::Test
#    Qt6::Core
#    Qt6::Widgets
#    Qt6::Network
#    pthread
#)
#add_test(NAME test_live_eti_processing COMMAND test_live_eti_processing)

# Test 3: Stream Recording
# DISABLED - test file not yet created
#add_executable(test_stream_recording
#    test_stream_recording.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/../src/network/network_stream_receiver.cpp
#)
#target_link_libraries(test_stream_recording
#    Qt6::Test
#    Qt6::Core
#    Qt6::Network
#    pthread
#)
#add_test(NAME test_stream_recording COMMAND test_stream_recording)

# ============================================================================
# CODE REVIEW FIX TESTS - Security & Validation (October 25, 2025)
# ============================================================================

# DISABLED - test file not yet created
#add_executable(test_security_fixes
#    test_security_fixes.cpp
#    ${CORE_SOURCES}
#)
#target_link_libraries(test_security_fixes
#    Qt6::Test
#    Qt6::Core
#    Qt6::Widgets
#    Qt6::Network
#    pthread
#)
#add_test(NAME test_security_fixes COMMAND test_security_fixes)

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

# FIG 0/7 Service Component Stream Mode Test
add_executable(test_fig007_stream_mode
    test_fig007_stream_mode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig007_stream_mode
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig007_stream_mode COMMAND test_fig007_stream_mode)

# FIG 0/16 Programme Type International Test (Python Pro Agent 29 - November 4, 2025)
add_executable(test_fig016_programme_type_intl
    test_fig016_programme_type_intl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig016_programme_type_intl
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig016_programme_type_intl COMMAND test_fig016_programme_type_intl)

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

# FIG 0/18 Announcement Support Test (Python Pro Agent - November 3, 2025)
add_executable(test_fig018_announcement
    test_fig018_announcement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig018_announcement
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig018_announcement COMMAND test_fig018_announcement)

# FIG 0/8 Service Component Global Definition Test (PDCA Week 4 - Agent 19)
add_executable(test_fig008_service_component_global
    test_fig008_service_component_global.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig008_service_component_global
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig008_service_component_global COMMAND test_fig008_service_component_global)

# FIG 0/9 Country/LTO/International Table Test (PDCA Week 6 - Agent 20)
add_executable(test_fig009_country_lto_table
    test_fig009_country_lto_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig009_country_lto_table
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig009_country_lto_table COMMAND test_fig009_country_lto_table)

# FIG 0/10 Date and Time Test (PDCA Week 2 - Python Pro)
add_executable(test_fig010_date_time
    test_fig010_date_time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig010_date_time
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig010_date_time COMMAND test_fig010_date_time)

# FIG 0/14 FEC Sub-channel Organization Test (PDCA Week 6 - Agent 21)
add_executable(test_fig014_fec_subchannel
    test_fig014_fec_subchannel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig014_fec_subchannel
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig014_fec_subchannel COMMAND test_fig014_fec_subchannel)

# FIG 0/15 Programme Number Test (PDCA Week 7 - Batch 4 / Agent 28)
add_executable(test_fig015_programme_number
    test_fig015_programme_number.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig015_programme_number
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig015_programme_number COMMAND test_fig015_programme_number)

# FIG 0/19 Announcement Switching Test (PDCA Week 7 - Batch 2 / Agent 22)
add_executable(test_fig019_announcement_switching
    test_fig019_announcement_switching.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig019_announcement_switching
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig019_announcement_switching COMMAND test_fig019_announcement_switching)

# FIG 0/21 Frequency Information Test (PDCA Week 7 - Batch 3 / Agent 25)
add_executable(test_fig021_frequency_information
    test_fig021_frequency_information.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig021_frequency_information
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig021_frequency_information COMMAND test_fig021_frequency_information)

# FIG 0/12 MPEG-2 TS Streaming Test (PDCA Week 7 - Batch 4 / Agent 27)
add_executable(test_fig012_mpeg2_ts
    test_fig012_mpeg2_ts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig012_mpeg2_ts
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig012_mpeg2_ts COMMAND test_fig012_mpeg2_ts)

# FIG 0/11 Region Definition Test (PDCA Week 7 - Batch 3 / Agent 24)
add_executable(test_fig011_region_definition
    test_fig011_region_definition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig011_region_definition
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig011_region_definition COMMAND test_fig011_region_definition)

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

    # Test 5: Qt-ADS Docking Layout (v1.3 GUI migration, October 2026)
    # Compiles the REAL application sources so the actual DABAnalyzerWindow
    # (src/main.cpp via GUI_TEST_MODE, main() excluded) can be validated:
    #   4 CDockManagers, 26 docks, 3-column Tab 1, persistence round-trip.
    set(DOCK_TEST_SOURCES
        gui/test_gui_dock_layout.cpp
    )
    # Everything the app links, minus src/main.cpp (included by the test file).
    # ${SOURCES} etc. hold paths relative to the top-level source dir.
    foreach(src ${SOURCES} ${EDI_SOURCES} ${CLI_SOURCES})
        if(NOT src STREQUAL "src/main.cpp")
            list(APPEND DOCK_TEST_SOURCES ${CMAKE_SOURCE_DIR}/${src})
        endif()
    endforeach()
    add_executable(test_gui_dock_layout ${DOCK_TEST_SOURCES})
    target_link_libraries(test_gui_dock_layout
        Qt6::Test
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        qtadvanceddocking-qt6
        ${ZMQ_LIBRARIES}
        pthread
    )
    target_compile_definitions(test_gui_dock_layout PRIVATE GUI_TEST_MODE)
    # test_gui_dock_layout.cpp includes ../src/main.cpp, whose trailing
    # `#include "main.moc"` needs the moc output that AUTOMOC generates for the
    # app target. Ensure the app target (and its autogen dir) builds first, then
    # expose its autogen include directory.
    add_dependencies(test_gui_dock_layout DABAnalyzer_Enhanced_FilteringBuild)
    target_include_directories(test_gui_dock_layout PRIVATE
        ${CMAKE_BINARY_DIR}/DABAnalyzer_Enhanced_FilteringBuild_autogen/include)
    add_test(NAME test_gui_dock_layout
             COMMAND test_gui_dock_layout)
    set_tests_properties(test_gui_dock_layout PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )

    message(STATUS "  ✓ test_gui_stream_info_panel (10 widget tests)")
    message(STATUS "  ✓ test_gui_service_discovery_panel (13 widget tests)")
    message(STATUS "  ✓ test_gui_fig_type_display_panel (15 widget tests)")
    message(STATUS "  ✓ test_gui_main_window (16 integration tests)")
    message(STATUS "  ✓ test_gui_dock_layout (7 dock layout tests)")

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
    GTest::GTest
    GTest::Main
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
message(STATUS "  Core Tests: 7 (ETI processing, FIG parsing, ETI-NI parser)")
message(STATUS "  Phase 3A Tests: 4 (DAB+, MOT, EPG, Journaline)")
message(STATUS "  Phase 3B Tests: 1 (DLS+)")
message(STATUS "  Phase 4 Tests: 3 (Live streaming, recording)")
message(STATUS "  Security Tests: 1 (Code review fixes)")
message(STATUS "  PDCA Week 1 Tests: 3 (FIG 1/0, TS 101 756, integration)")
message(STATUS "  PDCA Week 2 Tests: 4 (FIG 0/5, 0/17, 0/18, integration)")
if(CAN_RUN_GUI_TESTS)
    message(STATUS "  GUI Tests: 4 (Widgets and main window)")
else()
    message(STATUS "  GUI Tests: 0 (disabled - no display)")
endif()
message(STATUS "  E2E Tests: 3 (Full pipeline validation)")
message(STATUS "  Network/EDI Tests: 2 (EDI, PFT)")
message(STATUS "")
if(CAN_RUN_GUI_TESTS)
    message(STATUS "  Total Test Executables: 32")
else()
    message(STATUS "  Total Test Executables: 28")
endif()
message(STATUS "==============================================")

# FIG 0/20 Service Component Information Test (PDCA Week 7 - Batch 4 / Agent 30)
add_executable(test_fig020_service_component_info
    test_fig020_service_component_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig020_service_component_info
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig020_service_component_info COMMAND test_fig020_service_component_info)
# FIG 0/24 Other Ensemble Services Test (PDCA Week 7 - Final / Agent 33)
add_executable(test_fig024_oe_services
    test_fig024_oe_services.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_fig024_oe_services
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_fig024_oe_services COMMAND test_fig024_oe_services)

# ============================================================================
# PDCA Week 8 - CLI Mode Tests (Agent 36: CLI Main Integration)
# ============================================================================

# Agent 36: CLI Main Integration Test
add_executable(test_cli_main_integration
    test_cli_main_integration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli/cli_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli/cli_argument_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli/yaml_output_generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/headless_eti_processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/enhanced_eti_processor_qt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/advanced_fig_analyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/modern_eti_frame_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyzer_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/ensemble_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dab_service_discovery_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/enhanced_fig_analyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/performance_profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_cli_main_integration
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_cli_main_integration COMMAND test_cli_main_integration)

# Agent 34: CLI Argument Parser Test
add_executable(test_cli_argument_parser
    test_cli_argument_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli/cli_argument_parser.cpp
)
target_link_libraries(test_cli_argument_parser
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_cli_argument_parser COMMAND test_cli_argument_parser)

# Agent 35: YAML Output Generator Test
add_executable(test_yaml_output_generator
    test_yaml_output_generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli/yaml_output_generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/ensemble_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dab_service_discovery_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_yaml_output_generator
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_yaml_output_generator COMMAND test_yaml_output_generator)

# Agent 37: CLI Functional Tests (E2E)
add_executable(test_cli_mode_functional
    test_cli_mode_functional.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/modern_eti_frame_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyzer_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/performance_profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_cli_mode_functional
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_cli_mode_functional COMMAND test_cli_mode_functional)

# ============================================================================
# Analyzer Settings Tests (option-variant matrix rows 1-14, settings feature)
# ============================================================================

# Test: AnalyzerSettings defaults / QSettings round-trip / CLI mapping +
#       behavior changes on synthetic data (charset override, NST-1, FIB CRC
#       ignore, SId display, short labels, ECC override).
add_executable(test_analyzer_settings
    test_analyzer_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyzer_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/modern_eti_frame_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/performance_profiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/ensemble_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dab_service_discovery_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli/cli_argument_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli/yaml_output_generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_analyzer_settings
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_analyzer_settings COMMAND test_analyzer_settings)
set_tests_properties(test_analyzer_settings PROPERTIES TIMEOUT 30)
