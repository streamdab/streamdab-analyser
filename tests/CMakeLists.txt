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
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/advanced_fig_analyser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_compliance_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dabplus_stream_validator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dabplus_superframe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/mot_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/mot_pad_adapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/epg_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/journaline_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/tpeg_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/data_service_store.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_file_scanner.cpp
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
    Threads::Threads
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
    Threads::Threads
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
    Threads::Threads
)
add_test(NAME test_fig1_processor COMMAND test_fig1_processor)

# GUI decode path: AdvancedFIGAnalyser FIC/FIB-walk + charset-aware labels
# (real frame-5 FIBs of eti/bkk_20062022_141637.eti, FIB CRC modes, TIS-620 /
# EBU-Latin charset conversion, analyser settings wiring).
add_executable(test_advanced_fig_analyser
    test_advanced_fig_analyser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/advanced_fig_analyser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyser_settings.cpp
)
target_link_libraries(test_advanced_fig_analyser
    Qt6::Test
    Qt6::Core
    Threads::Threads
)
add_test(NAME test_advanced_fig_analyser COMMAND test_advanced_fig_analyser)

# P1 CLI FIG detail wave (docs/fixes/CLI_FIG_COVERAGE.md): per-frame FIG
# instance trace (--fig-trace) + `figs:`/`fic_health:` YAML sections.
# Launches the real streamdab-cli (QProcess) on eti/bkk_20062022_141637.eti,
# frame-limited via --max-frames 200 for a < 60 s budget.
add_executable(test_cli_fig_trace
    test_cli_fig_trace.cpp
)
target_link_libraries(test_cli_fig_trace
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_cli_fig_trace COMMAND test_cli_fig_trace)
set_tests_properties(test_cli_fig_trace PROPERTIES TIMEOUT 60)

# P2/P3 CLI FIG-detail wave (docs/fixes/CLI_FIG_COVERAGE.md backlog): FIG 1/2 +
# 1/3 Thai labels, FIG 0/21 + 0/22 readable decodes, FIG type 2, 32-bit SId
# FIG 1/1 (GUI/CLI P/D alignment), EEP/UEP protection tables, --figs-by-type
# + --fib-hex. Also launches the real streamdab-cli on synthetic streams and
# the fixture (frame-limited).
add_executable(test_cli_fig_extended
    test_cli_fig_extended.cpp
)
target_link_libraries(test_cli_fig_extended
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_cli_fig_extended COMMAND test_cli_fig_extended)
set_tests_properties(test_cli_fig_extended PROPERTIES TIMEOUT 60)

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
    Threads::Threads
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyser_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/enhanced_fig_analyser.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/mot_pad_adapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
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
# v1.4 DATA-PATH WAVE — EPG / Journaline / TPEG(-TEPG)
# ============================================================================

# TPEG framing unit tests (self-contained, no capture): marker-phase carousel
# detection and TS 102 894-1 sync-pair scanning on synthetic streams. PUBLIC.
add_executable(test_tpeg_framing
    test_tpeg_framing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/tpeg_decoder.cpp
)
target_link_libraries(test_tpeg_framing
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_tpeg_framing COMMAND test_tpeg_framing)

# TPEG/TEPG capture-backed tests (real bytes from eti/bkk_20062022_141637.eti
# SCId 22): framing report, marker distribution, text inventory. PRIVATE.
add_executable(test_tpeg_decoder
    test_tpeg_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/tpeg_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_file_scanner.cpp
)
target_link_libraries(test_tpeg_decoder
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_tpeg_decoder COMMAND test_tpeg_decoder)
set_tests_properties(test_tpeg_decoder PROPERTIES TIMEOUT 60)

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

# Test 1: Network Stream Receiver — dead legacy suite removed (T22 review F5).
# The disabled test called declared-but-undefined receiver methods
# (get_error_messages/clear_error_messages/has_errors); deleted rather than
# ported. Live coverage now lives in tests/test_network_stream_validation.cpp.

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

    # Test 4.5: Settings dialog wiring (v1.3 settings audit)
    add_executable(test_settings_dialog
        gui/test_settings_dialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/settings_dialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyser_settings.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
    )
    target_link_libraries(test_settings_dialog
        Qt6::Core Qt6::Widgets
        GTest::GTest GTest::Main
        Threads::Threads
    )
    add_test(NAME test_settings_dialog COMMAND test_settings_dialog)
    set_tests_properties(test_settings_dialog PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
        # Shares the test-mode QSettings store with the window suites; serialise
        # them so a parallel ctest -j cannot wipe a store another test is using.
        RESOURCE_LOCK settings_store
    )

    # Test 5: Qt-ADS Docking Layout (v1.3 GUI migration, October 2026)
    # Compiles the REAL application sources so the actual DABAnalyserWindow
    # (src/main.cpp via GUI_TEST_MODE, main() excluded) can be validated:
    #   4 CDockManagers, 26 docks, 3-column Tab 1, persistence round-trip.
    # Everything the app links, minus src/main.cpp (included by each test file).
    # ${SOURCES} etc. hold paths relative to the top-level source dir.
    set(APP_UNDER_TEST_SOURCES)
    foreach(src ${SOURCES} ${EDI_SOURCES} ${CLI_SOURCES})
        if(NOT src STREQUAL "src/main.cpp")
            list(APPEND APP_UNDER_TEST_SOURCES ${CMAKE_SOURCE_DIR}/${src})
        endif()
    endforeach()
    add_executable(test_gui_dock_layout
        gui/test_gui_dock_layout.cpp
        ${APP_UNDER_TEST_SOURCES})
    target_link_libraries(test_gui_dock_layout
        Qt6::Test
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        qtadvanceddocking-qt6
        ${ZMQ_LIBRARIES}
        ${T34_AUDIO_LIBS}
        Threads::Threads
    )
    target_compile_definitions(test_gui_dock_layout PRIVATE GUI_TEST_MODE ${T34_AUDIO_DEFINES} ${ZMQ_DEFINES} ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../eti")
    target_include_directories(test_gui_dock_layout PRIVATE ${T34_AUDIO_INCLUDES})
    # test_gui_dock_layout.cpp includes ../src/main.cpp, whose trailing
    # `#include "main.moc"` needs the moc output that AUTOMOC generates for the
    # app target. Ensure the app target (and its autogen dir) builds first, then
    # expose its autogen include directory.
    add_dependencies(test_gui_dock_layout DABAnalyser_Enhanced_FilteringBuild)
    target_include_directories(test_gui_dock_layout PRIVATE
        ${CMAKE_BINARY_DIR}/DABAnalyser_Enhanced_FilteringBuild_autogen/include)
    add_test(NAME test_gui_dock_layout
             COMMAND test_gui_dock_layout)
    set_tests_properties(test_gui_dock_layout PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        # Several full-fixture loads (real-data assertions, the reload/reset
        # test and the MOT Slideshow real-fixture test) need extra headroom on
        # slow CI hosts. B-LOW5: measured 229 s on this host AFTER the Wave
        # A/B additions (baseline was 191-217 s), so the old 240 s left under
        # 5% margin — raised to 480 s (≈2× measured) for slow CI runners.
        TIMEOUT 480
        # Shares the test-mode QSettings store with test_settings_dialog /
        # test_dock_interactions; serialise (see test_settings_dialog).
        RESOURCE_LOCK settings_store
    )

    # Test 6: Qt-ADS Dock Interaction QA (v1.3 GUI migration, T4)
    # Separate target (not an extension of test_gui_dock_layout) so the slow
    # fixture-driven dock suite does not grow: this suite drives float/re-dock,
    # auto-hide toggle, cross-area moves, tabify/untabify, persistence after a
    # non-default arrangement and 27-dock reachability, all offscreen and
    # fixture-free.
    add_executable(test_dock_interactions
        gui/test_dock_interactions.cpp
        ${APP_UNDER_TEST_SOURCES})
    target_link_libraries(test_dock_interactions
        Qt6::Test
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        qtadvanceddocking-qt6
        ${ZMQ_LIBRARIES}
        ${T34_AUDIO_LIBS}
        Threads::Threads
    )
    target_compile_definitions(test_dock_interactions PRIVATE GUI_TEST_MODE ${T34_AUDIO_DEFINES} ${ZMQ_DEFINES} ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../eti")
    target_include_directories(test_dock_interactions PRIVATE ${T34_AUDIO_INCLUDES})
    # Same main.moc contract as test_gui_dock_layout (see above).
    add_dependencies(test_dock_interactions DABAnalyser_Enhanced_FilteringBuild)
    target_include_directories(test_dock_interactions PRIVATE
        ${CMAKE_BINARY_DIR}/DABAnalyser_Enhanced_FilteringBuild_autogen/include)
    add_test(NAME test_dock_interactions
             COMMAND test_dock_interactions)
    set_tests_properties(test_dock_interactions PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 240
        # Shares the test-mode QSettings store; serialise (see above).
        RESOURCE_LOCK settings_store
    )

    message(STATUS "  ✓ test_gui_stream_info_panel (10 widget tests)")
    message(STATUS "  ✓ test_gui_service_discovery_panel (13 widget tests)")
    message(STATUS "  ✓ test_gui_fig_type_display_panel (15 widget tests)")
    message(STATUS "  ✓ test_gui_main_window (16 integration tests)")
    message(STATUS "  ✓ test_gui_dock_layout (7 dock layout tests)")
    message(STATUS "  ✓ test_dock_interactions (9 dock interaction tests)")

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
    Threads::Threads
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
    Threads::Threads
)
add_test(NAME test_e2e_mot_slideshow COMMAND test_e2e_mot_slideshow)
set_tests_properties(test_e2e_mot_slideshow PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

add_executable(test_e2e_epg_journaline
    e2e/test_e2e_epg_journaline.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_e2e_epg_journaline
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Threads::Threads
)
add_test(NAME test_e2e_epg_journaline COMMAND test_e2e_epg_journaline)
set_tests_properties(test_e2e_epg_journaline PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

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
    Threads::Threads
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
    Threads::Threads
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/advanced_fig_analyser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/modern_eti_frame_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyser_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/ensemble_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dab_service_discovery_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/eti_processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/enhanced_fig_analyser.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyser_settings.cpp
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
# Analyser Settings Tests (option-variant matrix rows 1-14, settings feature)
# ============================================================================

# Test: AnalyserSettings defaults / QSettings round-trip / CLI mapping +
#       behavior changes on synthetic data (charset override, NST-1, FIB CRC
#       ignore, SId display, short labels, ECC override).
add_executable(test_analyser_settings
    test_analyser_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/analyser_settings.cpp
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
target_link_libraries(test_analyser_settings
    Qt6::Test
    Qt6::Core
)
add_test(NAME test_analyser_settings COMMAND test_analyser_settings)
set_tests_properties(test_analyser_settings PROPERTIES TIMEOUT 30)

# ============================================================================
# T22 Network Stream Validation (URL validator + receiver hardening)
# ============================================================================
# The receiver is multicast-only: `validate_stream_url()` is the shared
# pre-check (UI + receiver), and a non-multicast target must fail fast with
# exactly one `error_occurred` and no retry loop (and no worker-thread throw).
add_executable(test_network_stream_validation
    test_network_stream_validation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/network_stream_receiver.cpp
)
target_link_libraries(test_network_stream_validation
    Qt6::Test
    Qt6::Core
    Qt6::Network
    ${ZMQ_LIBRARIES}
    Threads::Threads
)
# item 4: the TCP/ZMQ loopback tests link libzmq directly (the GUI target
# already does; the CLI target deliberately does not). HAVE_ZMQ is only defined
# when the transport was found; without it the ZMQ cases compile to a QSKIP.
target_link_directories(test_network_stream_validation PRIVATE ${ZMQ_LIBRARY_DIRS})
target_compile_definitions(test_network_stream_validation PRIVATE ${ZMQ_DEFINES})
add_test(NAME test_network_stream_validation COMMAND test_network_stream_validation)
set_tests_properties(test_network_stream_validation PROPERTIES TIMEOUT 30)

# ============================================================================
# T24 ProcessedFrame hand-off / segfault regression suite
# ============================================================================
# Resurrected orphan: covers the ProcessedFrame ownership model after T24
# (shared_ptr<const vector<SubChannelSlice>> slices, msc_size byte count,
# cheap COW-safe copy), the deep-copy/queued-connection crash regression, and
# Qt meta-type registration. Links CORE_SOURCES because it drives
# EnhancedETIProcessorQt::frameProcessed / qRegisterMetaType<ProcessedFrame>.
add_executable(test_segfault_fixes
    test_segfault_fixes.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_segfault_fixes
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Threads::Threads
)
add_test(NAME test_segfault_fixes COMMAND test_segfault_fixes)
set_tests_properties(test_segfault_fixes PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

# ============================================================================
# T34 DAB+ audio decode (libfaad2) — headless, no sound device required
# ============================================================================
# Drives the real Bangkok fixture through the production ETI frame parser +
# MSC slice extraction, assembles DAB+ superframes, decodes the AUs with
# libfaad2 and asserts non-silent PCM + plausible output format. Also asserts
# that two different DAB+ sub-channels decode to different PCM (service switch
# changes the decoded stream identity). QSKIPs when libfaad2 is not built in.
add_executable(test_dabplus_audio_decoder
    test_dabplus_audio_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dabplus_audio_decoder.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_dabplus_audio_decoder
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ${T34_AUDIO_LIBS}
    Threads::Threads
)
target_include_directories(test_dabplus_audio_decoder PRIVATE ${T34_AUDIO_INCLUDES})
target_compile_definitions(test_dabplus_audio_decoder PRIVATE ${T34_AUDIO_DEFINES})
add_test(NAME test_dabplus_audio_decoder COMMAND test_dabplus_audio_decoder)
set_tests_properties(test_dabplus_audio_decoder PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 120
)

# ============================================================================
# T34 audio output sink lifecycle (review gate 5) — headless, simulated backend
# ============================================================================
# Proves the sink's close()/destructor cannot hang on a stalled device and that
# a bad device name fails gracefully. No real playback is required. The Qt
# Multimedia sink source is also compiled here (with its define/link when the
# module was found) so an enabled build compiles both sink backends.
add_executable(test_audio_output_sink
    test_audio_output_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/audio_output_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/qt_audio_output_sink.cpp
)
target_link_libraries(test_audio_output_sink
    Qt6::Test
    Qt6::Core
    ${T34_AUDIO_LIBS}
    Threads::Threads
)
target_include_directories(test_audio_output_sink PRIVATE ${T34_AUDIO_INCLUDES})
target_compile_definitions(test_audio_output_sink PRIVATE ${T34_AUDIO_DEFINES})
add_test(NAME test_audio_output_sink COMMAND test_audio_output_sink)
set_tests_properties(test_audio_output_sink PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)

# ============================================================================
# T34-ext Qt Multimedia (QAudioSink) output sink — graceful degradation, the
# backend factory and (when the module is present) format mapping + the same
# no-hang stall proof as test_audio_output_sink, on the Qt backend.
# ============================================================================
# On this host Qt Multimedia is absent (find_package fails, HAVE_QMULTIMEDIA
# undefined) so every QAudio*/QMedia* symbol is compiled out and the suite
# asserts the graceful-degradation contract; the controller's backend preference
# is checked on every platform via #ifdef, and the module-only cases QSKIP.
# When Qt6Multimedia_FOUND the ${T34_AUDIO_DEFINES}/${T34_AUDIO_LIBS} variables
# carry HAVE_QMULTIMEDIA and Qt6::Multimedia above.
add_executable(test_qt_audio_output_sink
    test_qt_audio_output_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/qt_audio_output_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/audio_output_sink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/audio_playback_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dabplus_audio_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dabplus_superframe.cpp
)
target_link_libraries(test_qt_audio_output_sink
    Qt6::Test
    Qt6::Core
    ${T34_AUDIO_LIBS}
    Threads::Threads
)
target_include_directories(test_qt_audio_output_sink PRIVATE ${T34_AUDIO_INCLUDES})
target_compile_definitions(test_qt_audio_output_sink PRIVATE ${T34_AUDIO_DEFINES})
add_test(NAME test_qt_audio_output_sink COMMAND test_qt_audio_output_sink)
set_tests_properties(test_qt_audio_output_sink PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)

# ============================================================================
# Suite split: PUBLIC vs PRIVATE (see docs/TEST_SUITE_PUBLIC_PRIVATE_SPLIT.md)
#
# The eti/ captures are World DAB licensed material and never reach the
# public repository, so tests that load them are labelled PRIVATE and run
# only where the captures exist (locally). Every other registered test is
# PUBLIC — the only set GitHub CI can run (`ctest -L PUBLIC`).
# ============================================================================
set(DABX_PRIVATE_TESTS
    test_advanced_fig_analyser
    test_cli_fig_extended
    test_cli_fig_trace
    test_dabplus_audio_decoder
    test_e2e_epg_journaline
    test_eti_format_detection
    test_eti_frame_parser
    test_eti_li_crc_validation
    test_eti_ni_parser
    test_gui_dock_layout
    test_network_stream_validation
    test_segfault_fixes
    test_tpeg_decoder
)

# Label every registered test: PRIVATE for the capture-dependent set, PUBLIC
# otherwise. (A test target not listed here is left unlabelled.)
foreach(_dabx_test_target IN ITEMS
        test_advanced_fig_analyser test_analyser_settings test_audio_output_sink
        test_charset_converter test_cli_argument_parser test_cli_fig_extended
        test_cli_fig_trace test_cli_main_integration test_cli_mode_functional
        test_dabplus_audio_decoder test_dabplus_stream_validator
        test_dls_plus_decoder test_dock_interactions test_e2e_epg_journaline
        test_e2e_eti_to_audio test_e2e_mot_slideshow test_edi_packet_parser
        test_epg_decoder test_eti_format_detection test_eti_frame_parser
        test_eti_li_crc_validation test_eti_ni_parser
        test_etsi_registered_tables test_fig005_language test_fig007_stream_mode
        test_fig008_service_component_global test_fig009_country_lto_table
        test_fig010_date_time test_fig011_region_definition
        test_fig012_mpeg2_ts test_fig014_fec_subchannel
        test_fig015_programme_number test_fig016_programme_type_intl
        test_fig017_programme_type test_fig018_announcement
        test_fig019_announcement_switching test_fig020_service_component_info
        test_fig021_frequency_information test_fig024_oe_services
        test_fig0_processor test_fig10_ensemble_label test_fig1_processor
        test_fig_week1_comprehensive test_fig_week2_comprehensive
        test_gui_dock_layout test_gui_fig_type_display_panel
        test_gui_main_window test_gui_service_discovery_panel
        test_gui_stream_info_panel test_journaline_decoder
        test_live_eti_processing test_mot_protocol test_network_stream_validation
        test_pft_reassembler test_qt_audio_output_sink test_security_fixes
        test_segfault_fixes test_service_discovery test_settings_dialog
        test_stream_recording test_yaml_output_generator test_tpeg_framing
        test_tpeg_decoder)
    if(TEST "${_dabx_test_target}")   # guard: some add_test() are conditional/commented
        if(_dabx_test_target IN_LIST DABX_PRIVATE_TESTS)
            set_tests_properties("${_dabx_test_target}" PROPERTIES LABELS "PRIVATE")
        else()
            set_tests_properties("${_dabx_test_target}" PROPERTIES LABELS "PUBLIC")
        endif()
    endif()
endforeach()
