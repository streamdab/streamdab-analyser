# Simplified Test Configuration - PDCA Methodology
# Phase 1: Core ETI Processing Tests Only

cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Simplified Test Framework - PDCA Workflow ===")

# Find Qt6 Test module
find_package(Qt6 REQUIRED COMPONENTS Test Core Widgets)
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
    pthread
)
add_test(NAME test_dls_plus_decoder COMMAND test_dls_plus_decoder)

# ============================================================================
# INTEGRATION TESTS - Phase 3A Wave 3.1
# ============================================================================

# Include integration test subdirectory
add_subdirectory(integration)

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
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )

    message(STATUS "  ✓ test_phase3a_complete_pipeline (26 E2E tests)")
    message(STATUS "  ✓ performance_benchmark (standalone tool)")
endif()

message(STATUS "")
message(STATUS "===========================================")
message(STATUS "Configured 11 validation tests:")
message(STATUS "  - 5 core tests (ETI, FIG 0/1, LI CRC)")
message(STATUS "  - 5 Phase 3A tests (MOT, EPG, Journaline)")
message(STATUS "  - 1 Phase 3B test (DLS+ decoder - STUBBED)")
message(STATUS "Integration tests: 4 (audio + EPG + Journaline + DLS+)")
message(STATUS "E2E tests: 1 (Phase 3A complete pipeline - 26 tests)")
message(STATUS "Benchmark tools: 1 (performance_benchmark)")
message(STATUS "")
message(STATUS "NOTE: DLS+ tests are STUBBED pending Agent 1 completion")
message(STATUS "      All DLS+ tests will SKIP until dls_plus_decoder ready")
message(STATUS "")
message(STATUS "Test execution: make && ctest")
message(STATUS "Benchmark: make && ./tests/performance_benchmark")
message(STATUS "===========================================")
