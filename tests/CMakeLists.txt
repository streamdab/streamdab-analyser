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
# INTEGRATION TESTS - Phase 3A Wave 3.1
# ============================================================================

# Include integration test subdirectory
add_subdirectory(integration)

# ============================================================================
# GUI TESTS - QTest Automated Widget Testing (October 25, 2025)
# ============================================================================

message(STATUS "")
message(STATUS "=== GUI Test Configuration ===")

# GUI tests require Qt6 Widgets
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

# GUI Test 1: ETI Explorer Table Tests
add_executable(test_eti_explorer
    gui/test_eti_explorer.cpp
)
target_link_libraries(test_eti_explorer
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    Qt6::Network
)
target_include_directories(test_eti_explorer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui
)
add_test(NAME ETIExplorerTests COMMAND test_eti_explorer)
set_tests_properties(ETIExplorerTests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

# GUI Test 2: Service List (Ensemble Tree) Tests
add_executable(test_service_list
    gui/test_service_list.cpp
)
target_link_libraries(test_service_list
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    Qt6::Network
)
target_include_directories(test_service_list PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui
)
add_test(NAME ServiceListTests COMMAND test_service_list)
set_tests_properties(ServiceListTests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

# GUI Test 3: Frame Viewer Tests
add_executable(test_frame_viewer
    gui/test_frame_viewer.cpp
)
target_link_libraries(test_frame_viewer
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    Qt6::Network
)
target_include_directories(test_frame_viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui
)
add_test(NAME FrameViewerTests COMMAND test_frame_viewer)
set_tests_properties(FrameViewerTests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

# GUI Test 4: FIG Extractor Tests
add_executable(test_fig_extractor
    gui/test_fig_extractor.cpp
)
target_link_libraries(test_fig_extractor
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    Qt6::Network
)
target_include_directories(test_fig_extractor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/gui
)
add_test(NAME FIGExtractorTests COMMAND test_fig_extractor)
set_tests_properties(FIGExtractorTests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 10
)

# Custom target to build all GUI tests
add_custom_target(build_gui_tests
    DEPENDS test_eti_explorer test_service_list test_frame_viewer test_fig_extractor
    COMMENT "Building all QTest GUI automated tests..."
)

# Custom target to run all GUI tests
add_custom_target(run_gui_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R ".*Tests"
    DEPENDS test_eti_explorer test_service_list test_frame_viewer test_fig_extractor
    COMMENT "Running all QTest GUI automated tests..."
)

message(STATUS "  ✓ GUI tests enabled (4 QTest suites, 40+ test cases)")
message(STATUS "    - test_eti_explorer (8 tests)")
message(STATUS "    - test_service_list (11 tests)")
message(STATUS "    - test_frame_viewer (10 tests)")
message(STATUS "    - test_fig_extractor (10 tests)")
message(STATUS "  Run: make build_gui_tests && make run_gui_tests")

# ============================================================================
# PHASE 3A E2E TESTS - Wave 3.2 End-to-End Testing
# ============================================================================

# Check if Phase 3A E2E tests exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/e2e/test_phase3a_complete_pipeline.cpp")
    message(STATUS "Phase 3A E2E tests found - configuring...")

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
message(STATUS "Configured 16 validation tests:")
message(STATUS "  - 5 core tests (ETI, FIG 0/1, LI CRC)")
message(STATUS "  - 5 Phase 3A tests (MOT, EPG, Journaline)")
message(STATUS "  - 1 Phase 3B test (DLS+ decoder - STUBBED)")
message(STATUS "  - 1 Regression test (Segfault fixes)")
message(STATUS "  - 4 GUI tests (QTest widget verification)")
message(STATUS "Integration tests: 4 (audio + EPG + Journaline + DLS+)")
message(STATUS "E2E tests: 1 (Phase 3A complete pipeline - 26 tests)")
message(STATUS "GUI tests: 4 (40+ widget test cases)")
message(STATUS "Benchmark tools: 1 (performance_benchmark)")
message(STATUS "")
message(STATUS "NOTE: DLS+ tests are STUBBED pending Agent 1 completion")
message(STATUS "      All DLS+ tests will SKIP until dls_plus_decoder ready")
message(STATUS "")
message(STATUS "Regression tests verify all fixes for:")
message(STATUS "  ✓ ProcessedFrame deep copy (QByteArray detach)")
message(STATUS "  ✓ Qt::QueuedConnection safety")
message(STATUS "  ✓ Pass-by-value slots")
message(STATUS "  ✓ MSC bounds error handling")
message(STATUS "")
message(STATUS "GUI tests verify (October 25, 2025):")
message(STATUS "  ✓ ETI Explorer table population and structure")
message(STATUS "  ✓ Service list display and Thai UTF-8 support")
message(STATUS "  ✓ Frame viewer safe click handling (no crashes)")
message(STATUS "  ✓ FIG Extractor statistics display")
message(STATUS "")
message(STATUS "Test execution: make && ctest")
message(STATUS "GUI tests only: make build_gui_tests && make run_gui_tests")
message(STATUS "Benchmark: make && ./tests/performance_benchmark")
message(STATUS "===========================================")
