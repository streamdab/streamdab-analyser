# CMakeLists.txt for E2E Testing Suite - Phase 5.3 Professional UI/UX Validation
#
# Comprehensive End-to-End testing for professional broadcast deployment readiness
# Tests all user workflow patterns, panel interactions, performance targets, and ETSI compliance

cmake_minimum_required(VERSION 3.16)

# E2E Test Suite Configuration
set(E2E_TEST_SOURCES
    test_user_workflow_patterns.cpp
    test_panel_interactions.cpp
    test_performance_validation.cpp
    test_etsi_compliance_validation.cpp
)

# Find required Qt components for GUI testing
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

# Find testing frameworks
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# Create E2E test executable
add_executable(eti_analyzer_e2e_tests ${E2E_TEST_SOURCES})

# Link required libraries
target_link_libraries(eti_analyzer_e2e_tests
    # Core application libraries
    eti_analyzer_core
    eti_analyzer_gui
    eti_analyzer_utils

    # Qt libraries
    Qt6::Core
    Qt6::Widgets
    Qt6::Test

    # Testing frameworks
    GTest::GTest
    GTest::Main
    GMock::GMock

    # System libraries
    ${CMAKE_THREAD_LIBS_INIT}
)

# Include directories
target_include_directories(eti_analyzer_e2e_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
    ${CMAKE_SOURCE_DIR}/tests/fixtures
    ${CMAKE_SOURCE_DIR}/tests/mocks
    ${CMAKE_BINARY_DIR}
)

# Compiler flags for E2E testing
target_compile_features(eti_analyzer_e2e_tests PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(eti_analyzer_e2e_tests PRIVATE /W4)
else()
    target_compile_options(eti_analyzer_e2e_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set test properties
set_target_properties(eti_analyzer_e2e_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/e2e
)

# Copy test data files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/tests/fixtures/test_data
     DESTINATION ${CMAKE_BINARY_DIR}/tests/e2e/)

# Add test discovery for CTest
include(GoogleTest)
gtest_discover_tests(eti_analyzer_e2e_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/e2e
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests/e2e"
    DISCOVERY_TIMEOUT 30
)

# Define individual test targets for CI/CD pipeline
add_test(NAME E2E_UserWorkflowPatterns
         COMMAND eti_analyzer_e2e_tests --gtest_filter="UserWorkflowPatternsTest.*"
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/e2e)

add_test(NAME E2E_PanelInteractions
         COMMAND eti_analyzer_e2e_tests --gtest_filter="PanelInteractionTest.*"
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/e2e)

add_test(NAME E2E_PerformanceValidation
         COMMAND eti_analyzer_e2e_tests --gtest_filter="PerformanceValidationTest.*"
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/e2e)

add_test(NAME E2E_ETSICompliance
         COMMAND eti_analyzer_e2e_tests --gtest_filter="ETSIComplianceValidationTest.*"
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/e2e)

# Set test timeouts (E2E tests may take longer)
set_tests_properties(E2E_UserWorkflowPatterns PROPERTIES TIMEOUT 300)
set_tests_properties(E2E_PanelInteractions PROPERTIES TIMEOUT 180)
set_tests_properties(E2E_PerformanceValidation PROPERTIES TIMEOUT 600)
set_tests_properties(E2E_ETSICompliance PROPERTIES TIMEOUT 240)

# Test environment setup
set_tests_properties(E2E_UserWorkflowPatterns E2E_PanelInteractions
                     E2E_PerformanceValidation E2E_ETSICompliance
    PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=qt.qpa.xcb.debug=false"
)

# Custom test target for complete E2E validation
add_custom_target(run_e2e_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose --parallel 4
            --test-dir ${CMAKE_BINARY_DIR}
            -R "E2E_.*"
    DEPENDS eti_analyzer_e2e_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running comprehensive E2E test suite for Phase 5.3 validation"
)

# Performance test target with detailed reporting
add_custom_target(run_performance_tests
    COMMAND eti_analyzer_e2e_tests --gtest_filter="PerformanceValidationTest.*"
            --gtest_output=xml:e2e_performance_results.xml
    DEPENDS eti_analyzer_e2e_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/e2e
    COMMENT "Running performance validation tests with detailed reporting"
)

# ETSI compliance test target
add_custom_target(run_etsi_compliance_tests
    COMMAND eti_analyzer_e2e_tests --gtest_filter="ETSIComplianceValidationTest.*"
            --gtest_output=xml:e2e_etsi_compliance_results.xml
    DEPENDS eti_analyzer_e2e_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/e2e
    COMMENT "Running ETSI compliance validation tests"
)

# Generate test coverage for E2E tests (if coverage tools available)
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
        add_custom_target(e2e_coverage
            COMMAND ${LCOV_PATH} --directory . --capture --output-file e2e_coverage.info
            COMMAND ${LCOV_PATH} --remove e2e_coverage.info '/usr/*' --output-file e2e_coverage.info
            COMMAND ${LCOV_PATH} --remove e2e_coverage.info '*/tests/*' --output-file e2e_coverage.info
            COMMAND ${GENHTML_PATH} -o e2e_coverage e2e_coverage.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating E2E test coverage report"
        )
    endif()
endif()

# Install test executable for packaging
install(TARGETS eti_analyzer_e2e_tests
    RUNTIME DESTINATION bin/tests
    COMPONENT Testing
)

# Install test data
install(DIRECTORY ${CMAKE_SOURCE_DIR}/tests/fixtures/test_data
    DESTINATION share/eti-analyzer/test_data
    COMPONENT Testing
)

message(STATUS "E2E Testing Suite configured for Phase 5.3 Professional UI/UX Validation")
message(STATUS "Test targets: run_e2e_tests, run_performance_tests, run_etsi_compliance_tests")
