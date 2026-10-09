# Integration Tests - Phase 3A Wave 3.1
# Complete pipeline testing: ETI → FIG → Audio/MOT validation

cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Integration Tests Configuration ===")

# Find Qt6 Test module
find_package(Qt6 REQUIRED COMPONENTS Test Core Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include project source directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Core source files needed for integration tests
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/enhanced_eti_processor_qt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/advanced_fig_analyser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/dabplus_stream_validator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/mot_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/epg_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/phase3a_integration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/dls_plus_decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core/etsi_compliance_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/utils/logger.cpp
)

# ============================================================================
# Integration Test 1: Audio Pipeline (ETI → FIG → Audio Validation)
# ============================================================================
add_executable(test_audio_pipeline
    test_audio_pipeline.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_audio_pipeline
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Threads::Threads
)
add_test(NAME test_audio_pipeline COMMAND test_audio_pipeline)

# ============================================================================
# Integration Test 2: MOT Pipeline (ETI → FIG → MOT Object Extraction)
# ============================================================================
add_executable(test_mot_pipeline
    test_mot_pipeline.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_mot_pipeline
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Threads::Threads
)
add_test(NAME test_mot_pipeline COMMAND test_mot_pipeline)

# ============================================================================
# Integration Test 3: Phase 3A Complete Integration
# ============================================================================
add_executable(test_phase3a_integration
    test_phase3a_integration.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_phase3a_integration
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Threads::Threads
)
add_test(NAME test_phase3a_integration COMMAND test_phase3a_integration)

# ============================================================================
# Integration Test 4: DLS+ Pipeline (Phase 3B Week 2)
# Note: Removed - will be recreated with proper implementation in Week 2
# ============================================================================
# TODO Week 2: Add test_dls_plus_pipeline.cpp with real PAD data testing

message(STATUS "Integration tests configured:")
message(STATUS "  - test_audio_pipeline: ETI → Audio validation")
message(STATUS "  - test_mot_pipeline: ETI → MOT extraction")
message(STATUS "  - test_phase3a_integration: Complete Phase 3A pipeline")
message(STATUS "")
message(STATUS "NOTE: DLS+ pipeline integration test will be added in Week 2")
message(STATUS "==========================================")
