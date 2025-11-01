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

# Test 5: MOT Protocol Parser
add_executable(test_mot_protocol
    test_mot_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/mot_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_mot_protocol
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_mot_protocol COMMAND test_mot_protocol)


# Test: FIG 0/18 Announcement Support (PDCA Week 2 Agent 1)
add_executable(test_fig018_announcement
    test_fig018_announcement.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig018_announcement
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)
add_test(NAME test_fig018_announcement COMMAND test_fig018_announcement)

# Test 6: EPG Decoder (Phase 3A Wave 2.1)
add_executable(test_epg_decoder
    test_epg_decoder.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_epg_decoder
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_epg_decoder COMMAND test_epg_decoder)

# Test 7: EPG Pipeline Integration (Phase 3A Wave 2.1)
add_executable(test_epg_pipeline
    integration/test_epg_pipeline.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_epg_pipeline
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_epg_pipeline COMMAND test_epg_pipeline)

# Test 8: Journaline Decoder (Phase 3A Wave 2.2)
add_executable(test_journaline_decoder
    test_journaline_decoder.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_journaline_decoder
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_journaline_decoder COMMAND test_journaline_decoder)

# Test 9: Journaline Pipeline Integration (Phase 3A Wave 2.2)
add_executable(test_journaline_pipeline
    integration/test_journaline_pipeline.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_journaline_pipeline
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_journaline_pipeline COMMAND test_journaline_pipeline)

# Test 10: DLS+ Decoder Unit Tests (Phase 3B Week 2)
# Note: Currently stubbed - waiting for Agent 1 to complete dls_plus_decoder.hpp/cpp
add_executable(test_dls_plus_decoder
    test_dls_plus_decoder.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_dls_plus_decoder
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_dls_plus_decoder COMMAND test_dls_plus_decoder)

# ============================================================================
# PHASE A TESTS - Worker Thread Architecture (Pattern 3 + Pattern 5)
# ============================================================================

# Test 11: ETI Reader Thread (Pattern 3: Worker Thread + Event Loop)
add_executable(test_eti_reader_thread
    test_eti_reader_thread.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_eti_reader_thread
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
    gtest
    gtest_main
)
add_test(NAME test_eti_reader_thread COMMAND test_eti_reader_thread)
set_tests_properties(test_eti_reader_thread PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)

# Test 12: Shared ETI Data (Pattern 5: Shared Data Model with Mutex)
add_executable(test_shared_eti_data
    test_shared_eti_data.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_shared_eti_data
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
    gtest
    gtest_main
)
add_test(NAME test_shared_eti_data COMMAND test_shared_eti_data)
set_tests_properties(test_shared_eti_data PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)


# ============================================================================
# ETSI TS 101 756 REGISTERED TABLES TESTS (v1.1 - October 28, 2025)
# ============================================================================

message(STATUS "")
message(STATUS "=== ETSI TS 101 756 Registered Tables Tests ===")

# Test: ETSI Registered Tables (20+ tests)
add_executable(test_etsi_registered_tables
    test_etsi_registered_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_registered_tables.cpp
)
target_link_libraries(test_etsi_registered_tables
    Qt6::Test
    Qt6::Core
    pthread
)
add_test(NAME test_etsi_registered_tables COMMAND test_etsi_registered_tables)
set_tests_properties(test_etsi_registered_tables PROPERTIES
    TIMEOUT 60
)

message(STATUS "  - test_etsi_registered_tables: 20+ tests for ECC, Language, and PTy tables")
message(STATUS "  - Coverage: ETSI TS 101 756 v2.4.1 complete table implementation")
# Add FIG 1/0 test

# Test: FIG 1/0 Ensemble Label Parser (Week 1 v1.1 ETSI Compliance)
add_executable(test_fig10_ensemble_label
    test_fig10_ensemble_label.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig10_ensemble_label
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_fig10_ensemble_label COMMAND test_fig10_ensemble_label)

# Test 13: Week 1 Comprehensive Test Suite (FIG 1/0 + FIG 0/18)
# PDCA Methodology - TDD for Agent 1, Agent 2, Agent 4 deliverables
add_executable(test_fig_week1_comprehensive
    core/test_fig_week1_comprehensive.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig_week1_comprehensive
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_fig_week1_comprehensive COMMAND test_fig_week1_comprehensive)
set_tests_properties(test_fig_week1_comprehensive PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

# ============================================================================
# PDCA WEEK 2 TESTS - Integration Test Suite Expansion (Agent 4)
# ============================================================================

message(STATUS "")
message(STATUS "=== PDCA Week 2 Integration Test Suite ===")

# Test 14: Week 2 Comprehensive Test Suite (40+ tests)
# Expands Week 1 (26 tests) to Week 2 (40+ tests)
# Coverage: FIG 0/18, FIG 0/19, FIG 0/10 integration
add_executable(test_fig_week2_comprehensive
    core/test_fig_week2_comprehensive.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig_week2_comprehensive
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_fig_week2_comprehensive COMMAND test_fig_week2_comprehensive)
set_tests_properties(test_fig_week2_comprehensive PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

message(STATUS "  ✓ test_fig_week2_comprehensive (40+ integration tests)")
message(STATUS "    - Week 1 tests: 26 (FIG 1/0, FIG 0/18 basic)")
message(STATUS "    - Week 2 tests: 14+ (FIG 0/18, 0/19, 0/10 integration)")
message(STATUS "    - Cross-FIG validation: 4 tests")
message(STATUS "    - Emergency Warning System: end-to-end")

# ============================================================================
# INTEGRATION TESTS - Phase 3A Wave 3.1
# ============================================================================

# Include integration test subdirectory
add_subdirectory(integration)

# ============================================================================
# GUI TESTS - QTest Automated Widget Testing (October 25, 2025)
# ============================================================================

message(STATUS "")
message(STATUS "=== GUI Test Configuration ===")

# GUI tests require X11/Wayland or offscreen platform
# Detect if we're in a headless environment
if(DEFINED ENV{DISPLAY} OR DEFINED ENV{WAYLAND_DISPLAY} OR "$ENV{QT_QPA_PLATFORM}" STREQUAL "offscreen")
    set(CAN_RUN_GUI_TESTS ON)
    message(STATUS "  ✓ GUI test environment detected")
    message(STATUS "    Display: $ENV{DISPLAY}")
    message(STATUS "    Platform: $ENV{QT_QPA_PLATFORM}")
else()
    set(CAN_RUN_GUI_TESTS OFF)
    message(STATUS "  ✗ No display detected - GUI tests will be skipped in CI")
    message(STATUS "    Set QT_QPA_PLATFORM=offscreen to enable")
endif()

if(CAN_RUN_GUI_TESTS)
    message(STATUS "  Configuring 4 GUI test suites...")

    # Test 1: Stream Info Panel Widget (October 25, 2025)
    add_executable(test_gui_stream_info_panel
        gui/test_gui_stream_info_panel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/stream_info_panel.cpp
        ${CORE_SOURCES}
    )
    target_link_libraries(test_gui_stream_info_panel
        Qt6::Test Qt6::Core Qt6::Widgets Qt6::Network pthread
    )
    target_include_directories(test_gui_stream_info_panel PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui
    )
    add_test(NAME test_gui_stream_info_panel
             COMMAND test_gui_stream_info_panel)
    set_tests_properties(test_gui_stream_info_panel PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )

    # Test 2: Service Discovery Panel Widget (October 25, 2025)
    add_executable(test_gui_service_discovery_panel
        gui/test_gui_service_discovery_panel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/service_discovery_panel.cpp
        ${CORE_SOURCES}
    )
    target_link_libraries(test_gui_service_discovery_panel
        Qt6::Test Qt6::Core Qt6::Widgets Qt6::Network pthread
    )
    target_include_directories(test_gui_service_discovery_panel PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui
    )
    add_test(NAME test_gui_service_discovery_panel
             COMMAND test_gui_service_discovery_panel)
    set_tests_properties(test_gui_service_discovery_panel PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )

    # Test 3: FIG Type Display Panel Widget (October 25, 2025)
    add_executable(test_gui_fig_type_display_panel
        gui/test_gui_fig_type_display_panel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/fig_type_display_panel.cpp
        ${CORE_SOURCES}
    )
    target_link_libraries(test_gui_fig_type_display_panel
        Qt6::Test Qt6::Core Qt6::Widgets Qt6::Network pthread
    )
    target_include_directories(test_gui_fig_type_display_panel PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui
    )
    add_test(NAME test_gui_fig_type_display_panel
             COMMAND test_gui_fig_type_display_panel)
    set_tests_properties(test_gui_fig_type_display_panel PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )

    # Test 4: Main Window Integration (October 25, 2025)
    add_executable(test_gui_main_window
        gui/test_gui_main_window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/main_window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/stream_info_panel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/service_discovery_panel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui/fig_type_display_panel.cpp
        ${CORE_SOURCES}
    )
    target_link_libraries(test_gui_main_window
        Qt6::Test Qt6::Core Qt6::Widgets Qt6::Network pthread
    )
    target_include_directories(test_gui_main_window PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui
    )
    add_test(NAME test_gui_main_window
             COMMAND test_gui_main_window)
    set_tests_properties(test_gui_main_window PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
    )

    message(STATUS "  ✓ test_gui_stream_info_panel (10 widget tests)")
    message(STATUS "  ✓ test_gui_service_discovery_panel (10 widget tests)")
    message(STATUS "  ✓ test_gui_fig_type_display_panel (10 widget tests)")
    message(STATUS "  ✓ test_gui_main_window (10+ integration tests)")

else()
    message(STATUS "  Skipping GUI tests (no display environment)")
endif()

# ============================================================================
# E2E TESTS - Phase 3A Wave 3.2 (October 24, 2025)
# ============================================================================

message(STATUS "")
message(STATUS "=== E2E Test Configuration ===")

# E2E tests enabled (real ETI data processing)
if(TRUE)
    message(STATUS "  E2E tests ENABLED (real ETI data)")

    # Include Phase 3A E2E test configuration
    set(PHASE3A_CORE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/enhanced_eti_processor_qt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/advanced_fig_analyzer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dabplus_stream_validator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/mot_protocol.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/epg_decoder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/journaline_decoder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/phase3a_integration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/dls_plus_decoder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/etsi_compliance_engine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/fig_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/charset_converter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
    )

    # Phase 3A Complete Pipeline E2E Test
    add_executable(test_phase3a_complete_pipeline
        e2e/test_phase3a_complete_pipeline.cpp
        ${PHASE3A_CORE_SOURCES}
    )
    target_link_libraries(test_phase3a_complete_pipeline
        Qt6::Test Qt6::Core Qt6::Widgets pthread
    )
    target_include_directories(test_phase3a_complete_pipeline PRIVATE
    Qt6::Network
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/integration
    )
    add_test(NAME test_phase3a_complete_pipeline
             COMMAND test_phase3a_complete_pipeline)
    set_tests_properties(test_phase3a_complete_pipeline PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false"
        TIMEOUT 120
    )

    # Phase 3A Performance Benchmark (standalone tool, not a test)
    add_executable(performance_benchmark
        e2e/performance_benchmark.cpp
        ${PHASE3A_CORE_SOURCES}
    )
    target_link_libraries(performance_benchmark
        Qt6::Core Qt6::Widgets pthread
    )
    target_include_directories(performance_benchmark PRIVATE
    Qt6::Network
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )

    message(STATUS "  ✓ test_phase3a_complete_pipeline (26 E2E tests)")
    message(STATUS "  ✓ performance_benchmark (standalone tool)")
endif()

# ============================================================================
# REGRESSION TESTS - Segfault Fixes (October 24, 2025)
# ============================================================================

# Test: Segmentation Fault Fixes Regression Suite
add_executable(test_segfault_fixes
    test_segfault_fixes.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_segfault_fixes
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_segfault_fixes COMMAND test_segfault_fixes)
set_tests_properties(test_segfault_fixes PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)

message(STATUS "")
message(STATUS "===========================================")
message(STATUS "Configured 17 validation tests:")
message(STATUS "  - 5 core tests (ETI, FIG 0/1, LI CRC)")
message(STATUS "  - 5 Phase 3A tests (MOT, EPG, Journaline)")
message(STATUS "  - 1 Phase 3B test (DLS+ decoder - STUBBED)")
message(STATUS "  - 1 Regression test (Segfault fixes)")
message(STATUS "  - 4 GUI tests (QTest widget verification)")
message(STATUS "  - 1 PDCA Week 2 test (40+ integration tests)")
message(STATUS "Integration tests: 4 (audio + EPG + Journaline + DLS+)")
message(STATUS "E2E tests: 1 (Phase 3A complete pipeline - 26 tests)")
message(STATUS "GUI tests: 4 (40+ widget test cases)")
message(STATUS "Week 2 tests: 1 (40+ FIG integration tests)")
message(STATUS "Benchmark tools: 1 (performance_benchmark)")
message(STATUS "")
message(STATUS "NOTE: DLS+ tests are STUBBED pending Agent 1 completion")
message(STATUS "      All DLS+ tests will SKIP until dls_plus_decoder ready")
message(STATUS "")
message(STATUS "Run tests: cd build && ctest --output-on-failure")
message(STATUS "===========================================")

# ============================================================================
# NETWORK TESTS - EDI Protocol Support (October 23, 2025)
# ============================================================================

# EDI Packet Parser Test
add_executable(test_edi_packet_parser
    test_edi_packet_parser.cpp
    ${NETWORK_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_edi_packet_parser
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_edi_packet_parser COMMAND test_edi_packet_parser)

# PFT Reassembler Test
add_executable(test_pft_reassembler
    test_pft_reassembler.cpp
    ${NETWORK_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/logger.cpp
)
target_link_libraries(test_pft_reassembler
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_pft_reassembler COMMAND test_pft_reassembler)

# ============================================================================
# ETSI COMPLIANCE PROOF TEST SUITE (October 28, 2025)
# ============================================================================

# Test: ETSI Compliance Proof (50+ tests)
add_executable(test_etsi_compliance_proof
    test_etsi_compliance_proof.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_etsi_compliance_proof
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_etsi_compliance_proof COMMAND test_etsi_compliance_proof)

message(STATUS "=== ETSI Compliance Proof Test Added ===")
message(STATUS "  - test_etsi_compliance_proof: 50+ tests proving ETSI standards compliance")


# FIG 0/10 Date and Time Test (PDCA Week 2, Agent 3)
# FIG 0/10 Date and Time Test (PDCA Week 2, Agent 3)
add_executable(test_fig010_date_time
    test_fig010_date_time.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig010_date_time
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME TestFig010DateTime COMMAND test_fig010_date_time)
set_tests_properties(TestFig010DateTime PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

# FIG 0/19 Announcement Switching Test (PDCA Week 2, Agent 2)
add_executable(test_fig019_announcement_switch
    test_fig019_announcement_switch.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig019_announcement_switch
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME test_fig019_announcement_switch COMMAND test_fig019_announcement_switch)

# FIG 0/5 Service Component Language Test (PDCA Week 3, Agent 8)
add_executable(test_fig005_language
    test_fig005_language.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig005_language
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME Test_FIG005_Language COMMAND test_fig005_language)

# FIG 0/17 Programme Type Test (PDCA Week 3, Agent 9)
add_executable(test_fig017_programme_type
    test_fig017_programme_type.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig017_programme_type
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME Test_FIG017_ProgrammeType COMMAND test_fig017_programme_type)

# ============================================================================
# PDCA WEEK 4 TESTS - Multi-Agent Parallel Development (Agent 11-14)
# ============================================================================

message(STATUS "")
message(STATUS "=== PDCA Week 4 Test Suite ===")

# Test: FIG 0/3 Service Component in Packet Mode (PDCA Week 4 - Agent 11)
add_executable(test_fig003_packet_mode
    test_fig003_packet_mode.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig003_packet_mode
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME Test_FIG003_PacketMode COMMAND test_fig003_packet_mode)
set_tests_properties(Test_FIG003_PacketMode PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

# Test: FIG 0/6 Service Linking (PDCA Week 4 - Agent 12)
add_executable(test_fig006_service_linking
    test_fig006_service_linking.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig006_service_linking
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME Test_FIG006_ServiceLinking COMMAND test_fig006_service_linking)
set_tests_properties(Test_FIG006_ServiceLinking PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

# Test: FIG 0/13 User Application Information (PDCA Week 4 - Agent 13)
add_executable(test_fig013_user_application
    test_fig013_user_application.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_fig013_user_application
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    pthread
)
add_test(NAME Test_FIG013_UserApplication COMMAND test_fig013_user_application)
set_tests_properties(Test_FIG013_UserApplication PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

message(STATUS "  ✓ test_fig003_packet_mode (20 tests - ETSI EN 300 401 Section 8.1.3)")
message(STATUS "  ✓ test_fig006_service_linking (20 tests - ETSI EN 300 401 Section 8.1.15)")
message(STATUS "  ✓ test_fig013_user_application (20 tests - ETSI EN 300 401 Section 8.1.14)")
message(STATUS "  Total Week 4 Tests: 60 comprehensive tests")

