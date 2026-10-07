# ==============================================================================
# Network Component TDD Test Suite CMake Configuration
# ==============================================================================
#
# Comprehensive test-driven development framework for network components
# implementing professional broadcast industry standards and requirements.
#
# Author: Network/Stream Agent - TDD Lead
# Date: 2025-09-22
# Copyright (c) 2025 ETI Stream Analyser Team
# ==============================================================================

cmake_minimum_required(VERSION 3.20)

# Network TDD Test Suite Configuration
set(NETWORK_TDD_TEST_SUITE_VERSION "1.0.0")
message(STATUS "Configuring Network Component TDD Test Suite v${NETWORK_TDD_TEST_SUITE_VERSION}")

# ==============================================================================
# Test Suite Components
# ==============================================================================

# EDI Packet Parser TDD Tests (NEW)
set(EDI_PACKET_PARSER_TDD_SOURCES
    test_edi_packet_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/network/edi_packet_parser.cpp
)

# ETI-over-IP Receiver TDD Tests
set(ETI_OVER_IP_RECEIVER_TDD_SOURCES
    test_eti_over_ip_receiver_tdd.cpp
)

# Streaming Processor TDD Tests
set(STREAMING_PROCESSOR_TDD_SOURCES
    test_streaming_processor_tdd.cpp
)

# Broadcast Equipment Integration TDD Tests
set(BROADCAST_EQUIPMENT_TDD_SOURCES
    test_broadcast_equipment_integration_tdd.cpp
)

# Performance Validation Framework TDD Tests
set(PERFORMANCE_VALIDATION_TDD_SOURCES
    test_performance_validation_framework_tdd.cpp
)

# Professional Workflow Integration TDD Tests
set(WORKFLOW_INTEGRATION_TDD_SOURCES
    test_professional_workflow_integration_tdd.cpp
)

# Test Fixtures and Mock Objects
set(NETWORK_TEST_FIXTURES_SOURCES
    ../fixtures/network_test_data.cpp
    ../fixtures/broadcast_equipment_data.cpp
    ../fixtures/performance_test_data.cpp
    ../fixtures/professional_workflow_data.cpp
)

set(NETWORK_MOCK_SOURCES
    ../mocks/mock_rtp_stream_generator.cpp
    ../mocks/mock_streaming_source.cpp
    ../mocks/mock_broadcast_equipment.cpp
    ../mocks/mock_performance_metrics_collector.cpp
    ../mocks/mock_broadcast_automation_system.cpp
)

# ==============================================================================
# Test Target Definitions
# ==============================================================================

# EDI Packet Parser TDD Test Executable (NEW)
add_executable(test_edi_packet_parser
    test_edi_packet_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/network/edi_packet_parser.cpp
)

target_link_libraries(test_edi_packet_parser
    PRIVATE
        Qt6::Core
        Qt6::Test
)

target_compile_features(test_edi_packet_parser PRIVATE cxx_std_20)

target_include_directories(test_edi_packet_parser PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(test_edi_packet_parser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network
    FOLDER "Tests/Network"
    AUTOMOC ON
)

# Register EDI Packet Parser test with CTest
add_test(
    NAME NetworkTDD_EDIPacketParser
    COMMAND test_edi_packet_parser
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network
)

set_tests_properties(
    NetworkTDD_EDIPacketParser
    PROPERTIES
        LABELS "Network;TDD;Unit;EDI"
        TIMEOUT 60  # 1 minute for unit tests
)

# ETI-over-IP Receiver TDD Test Executable
add_executable(test_eti_over_ip_receiver_tdd
    ${ETI_OVER_IP_RECEIVER_TDD_SOURCES}
    ${NETWORK_TEST_FIXTURES_SOURCES}
    ${NETWORK_MOCK_SOURCES}
)

target_link_libraries(test_eti_over_ip_receiver_tdd
    PRIVATE
        # Main project libraries
        ETIStreamAnalyser_core
        ETIStreamAnalyser_network
        ETIStreamAnalyser_utils

        # Qt6 components
        Qt6::Core
        Qt6::Network
        Qt6::Test

        # Google Test framework
        ${GTEST_LIBS}

        # Platform-specific networking
        $<$<PLATFORM_ID:Windows>:ws2_32>
        $<$<PLATFORM_ID:Linux>:pthread>
)

target_compile_definitions(test_eti_over_ip_receiver_tdd
    PRIVATE
        # Enable network testing features
        ETI_NETWORK_TESTING_ENABLED
        RTP_PROTOCOL_TESTING_ENABLED
        MULTICAST_TESTING_ENABLED

        # Performance testing targets
        TARGET_FPS=900
        TARGET_LATENCY_MS=17
        TARGET_MEMORY_MB=100

        # Test environment configuration
        TEST_MULTICAST_ADDRESS="239.192.0.1"
        TEST_PORT=9200
        TEST_DURATION_MULTIPLIER=1
)

# Streaming Processor TDD Test Executable
add_executable(test_streaming_processor_tdd
    ${STREAMING_PROCESSOR_TDD_SOURCES}
    ${NETWORK_TEST_FIXTURES_SOURCES}
    ${NETWORK_MOCK_SOURCES}
)

target_link_libraries(test_streaming_processor_tdd
    PRIVATE
        ETIStreamAnalyser_core
        ETIStreamAnalyser_network
        ETIStreamAnalyser_utils
        Qt6::Core
        Qt6::Network
        Qt6::Test
        ${GTEST_LIBS}
        $<$<PLATFORM_ID:Linux>:pthread>
)

target_compile_definitions(test_streaming_processor_tdd
    PRIVATE
        STREAMING_PROCESSOR_TESTING_ENABLED
        LOCK_FREE_QUEUE_TESTING_ENABLED
        ADAPTIVE_QUALITY_TESTING_ENABLED
        ULTRA_LOW_LATENCY_TESTING_ENABLED
        MULTI_THREADED_TESTING_ENABLED
)

# Broadcast Equipment Integration TDD Test Executable
add_executable(test_broadcast_equipment_integration_tdd
    ${BROADCAST_EQUIPMENT_TDD_SOURCES}
    ${NETWORK_TEST_FIXTURES_SOURCES}
    ${NETWORK_MOCK_SOURCES}
)

target_link_libraries(test_broadcast_equipment_integration_tdd
    PRIVATE
        ETIStreamAnalyser_core
        ETIStreamAnalyser_network
        ETIStreamAnalyser_utils
        Qt6::Core
        Qt6::Network
        Qt6::Test
        ${GTEST_LIBS}
)

target_compile_definitions(test_broadcast_equipment_integration_tdd
    PRIVATE
        BROADCAST_EQUIPMENT_TESTING_ENABLED
        SAP_SDP_PROTOCOL_TESTING_ENABLED
        UPNP_DISCOVERY_TESTING_ENABLED
        MDNS_DISCOVERY_TESTING_ENABLED
        PROFESSIONAL_EQUIPMENT_SIMULATION_ENABLED
        ELECARD_INTEGRATION_TESTING_ENABLED
        DEKTEC_INTEGRATION_TESTING_ENABLED
        ROHDE_SCHWARZ_INTEGRATION_TESTING_ENABLED
)

# Performance Validation Framework TDD Test Executable
add_executable(test_performance_validation_framework_tdd
    ${PERFORMANCE_VALIDATION_TDD_SOURCES}
    ${NETWORK_TEST_FIXTURES_SOURCES}
    ${NETWORK_MOCK_SOURCES}
)

target_link_libraries(test_performance_validation_framework_tdd
    PRIVATE
        ETIStreamAnalyser_core
        ETIStreamAnalyser_network
        ETIStreamAnalyser_utils
        Qt6::Core
        Qt6::Network
        Qt6::Test
        ${GTEST_LIBS}
)

target_compile_definitions(test_performance_validation_framework_tdd
    PRIVATE
        PERFORMANCE_VALIDATION_TESTING_ENABLED
        MEMORY_LEAK_DETECTION_ENABLED
        CPU_OPTIMIZATION_TESTING_ENABLED
        BROADCAST_QUALITY_TESTING_ENABLED
        RELIABILITY_TESTING_ENABLED
        EXTENDED_OPERATION_TESTING_ENABLED
)

# Professional Workflow Integration TDD Test Executable
add_executable(test_professional_workflow_integration_tdd
    ${WORKFLOW_INTEGRATION_TDD_SOURCES}
    ${NETWORK_TEST_FIXTURES_SOURCES}
    ${NETWORK_MOCK_SOURCES}
)

target_link_libraries(test_professional_workflow_integration_tdd
    PRIVATE
        ETIStreamAnalyser_core
        ETIStreamAnalyser_network
        ETIStreamAnalyser_utils
        Qt6::Core
        Qt6::Network
        Qt6::Test
        ${GTEST_LIBS}
)

target_compile_definitions(test_professional_workflow_integration_tdd
    PRIVATE
        WORKFLOW_INTEGRATION_TESTING_ENABLED
        BROADCAST_AUTOMATION_TESTING_ENABLED
        EMERGENCY_BROADCAST_TESTING_ENABLED
        MULTI_REGIONAL_TESTING_ENABLED
        PRODUCTION_ENVIRONMENT_TESTING_ENABLED
        AUDIT_TRAIL_TESTING_ENABLED
)

# ==============================================================================
# Test Configuration and Properties
# ==============================================================================

# Set test properties for all network TDD tests
set(NETWORK_TDD_TESTS
    test_edi_packet_parser
    test_eti_over_ip_receiver_tdd
    test_streaming_processor_tdd
    test_broadcast_equipment_integration_tdd
    test_performance_validation_framework_tdd
    test_professional_workflow_integration_tdd
)

foreach(test_target ${NETWORK_TDD_TESTS})
    # Set C++ standard and compiler features (use C++20 for newer tests)
    if(test_target STREQUAL "test_edi_packet_parser")
        target_compile_features(${test_target} PRIVATE cxx_std_20)
    else()
        target_compile_features(${test_target} PRIVATE cxx_std_17)
    endif()

    # Compiler-specific optimizations
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${test_target} PRIVATE
            -Wall -Wextra -Wpedantic
            -O2  # Optimized for performance testing
            -march=native  # Use native CPU features
            $<$<CONFIG:Debug>:-g3 -O0>
            $<$<CONFIG:Release>:-O3 -DNDEBUG>
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${test_target} PRIVATE
            /W4
            /O2
            $<$<CONFIG:Debug>:/Od /Zi>
            $<$<CONFIG:Release>:/O2 /DNDEBUG>
        )
    endif()

    # Include directories (skip for standalone test_edi_packet_parser which sets its own)
    if(NOT test_target STREQUAL "test_edi_packet_parser")
        target_include_directories(${test_target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures
            ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
            ${CMAKE_SOURCE_DIR}/src
        )
    endif()

    # Test-specific properties
    set_target_properties(${test_target} PROPERTIES
        # Output directory
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network

        # Test categorization
        FOLDER "Tests/Network"

        # Enable testing features
        AUTOMOC ON

        # Version information
        VERSION ${NETWORK_TDD_TEST_SUITE_VERSION}
        SOVERSION 1
    )
endforeach()

# ==============================================================================
# Advanced Test Configuration
# ==============================================================================

# Performance Testing Configuration
if(ENABLE_PERFORMANCE_TESTING)
    message(STATUS "Enabling performance testing optimizations")

    foreach(test_target ${NETWORK_TDD_TESTS})
        target_compile_definitions(${test_target} PRIVATE
            PERFORMANCE_TESTING_ENABLED=1
            HIGH_PRECISION_TIMING_ENABLED=1
            EXTENDED_TEST_DURATION_ENABLED=1
        )

        # Enable high-resolution timing
        if(WIN32)
            target_link_libraries(${test_target} PRIVATE winmm)
        endif()
    endforeach()
endif()

# Network Testing Configuration
if(ENABLE_NETWORK_TESTING)
    message(STATUS "Enabling network testing capabilities")

    target_compile_definitions(test_eti_over_ip_receiver_tdd PRIVATE
        REAL_NETWORK_TESTING_ENABLED=1
        MULTICAST_CAPABILITY_TESTING_ENABLED=1
    )

    target_compile_definitions(test_broadcast_equipment_integration_tdd PRIVATE
        EQUIPMENT_DISCOVERY_TESTING_ENABLED=1
        PROTOCOL_VALIDATION_TESTING_ENABLED=1
    )
endif()

# Memory Testing Configuration
if(ENABLE_MEMORY_TESTING)
    message(STATUS "Enabling memory testing and leak detection")

    foreach(test_target ${NETWORK_TDD_TESTS})
        target_compile_definitions(${test_target} PRIVATE
            MEMORY_TESTING_ENABLED=1
            LEAK_DETECTION_ENABLED=1
        )

        # Add memory testing libraries
        if(UNIX AND NOT APPLE)
            find_library(TCMALLOC_LIB tcmalloc)
            if(TCMALLOC_LIB)
                target_link_libraries(${test_target} PRIVATE ${TCMALLOC_LIB})
            endif()
        endif()
    endforeach()
endif()

# Thread Safety Testing Configuration
if(ENABLE_THREAD_SAFETY_TESTING)
    message(STATUS "Enabling thread safety testing")

    target_compile_definitions(test_streaming_processor_tdd PRIVATE
        THREAD_SAFETY_TESTING_ENABLED=1
        LOCK_FREE_VALIDATION_ENABLED=1
        RACE_CONDITION_DETECTION_ENABLED=1
    )

    # Add thread sanitizer in debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(test_streaming_processor_tdd PRIVATE -fsanitize=thread)
            target_link_options(test_streaming_processor_tdd PRIVATE -fsanitize=thread)
        endif()
    endif()
endif()

# ==============================================================================
# Test Registration with CTest
# ==============================================================================

# Enable testing
enable_testing()

# Register individual test executables
add_test(
    NAME NetworkTDD_EtiOverIpReceiver
    COMMAND test_eti_over_ip_receiver_tdd
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network
)

add_test(
    NAME NetworkTDD_StreamingProcessor
    COMMAND test_streaming_processor_tdd
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network
)

add_test(
    NAME NetworkTDD_BroadcastEquipmentIntegration
    COMMAND test_broadcast_equipment_integration_tdd
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network
)

add_test(
    NAME NetworkTDD_PerformanceValidationFramework
    COMMAND test_performance_validation_framework_tdd
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network
)

add_test(
    NAME NetworkTDD_ProfessionalWorkflowIntegration
    COMMAND test_professional_workflow_integration_tdd
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network
)

# Test properties and labels
set_tests_properties(
    NetworkTDD_EtiOverIpReceiver
    NetworkTDD_StreamingProcessor
    NetworkTDD_BroadcastEquipmentIntegration
    NetworkTDD_PerformanceValidationFramework
    NetworkTDD_ProfessionalWorkflowIntegration
    PROPERTIES
        LABELS "Network;TDD;Integration"
        TIMEOUT 300  # 5 minutes per test suite
)

# Performance test specific properties
set_tests_properties(
    NetworkTDD_PerformanceValidationFramework
    PROPERTIES
        LABELS "Network;TDD;Performance;Extended"
        TIMEOUT 600  # 10 minutes for performance tests
)

# Network integration test properties
set_tests_properties(
    NetworkTDD_EtiOverIpReceiver
    NetworkTDD_BroadcastEquipmentIntegration
    PROPERTIES
        LABELS "Network;TDD;Integration;Equipment"
        TIMEOUT 420  # 7 minutes for network tests
)

# ==============================================================================
# Custom Test Targets
# ==============================================================================

# Network TDD Test Suite Target
add_custom_target(network_tdd_tests
    DEPENDS ${NETWORK_TDD_TESTS}
    COMMENT "Building Network Component TDD Test Suite"
)

# Run all network TDD tests
add_custom_target(run_network_tdd_tests
    COMMAND ${CMAKE_CTEST_COMMAND}
        --output-on-failure
        --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
        -L "Network.*TDD"
    DEPENDS network_tdd_tests
    COMMENT "Running Network Component TDD Test Suite"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Run performance tests only
add_custom_target(run_network_performance_tests
    COMMAND ${CMAKE_CTEST_COMMAND}
        --output-on-failure
        -L "Performance"
    DEPENDS test_performance_validation_framework_tdd
    COMMENT "Running Network Performance Validation Tests"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Run integration tests only
add_custom_target(run_network_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND}
        --output-on-failure
        -L "Integration"
    DEPENDS
        test_broadcast_equipment_integration_tdd
        test_professional_workflow_integration_tdd
    COMMENT "Running Network Integration Tests"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Network test coverage target (if coverage tools available)
if(ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
        add_custom_target(network_tdd_coverage
            # Cleanup previous coverage data
            COMMAND ${LCOV_PATH} --directory . --zerocounters

            # Run tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "Network.*TDD"

            # Capture coverage data
            COMMAND ${LCOV_PATH} --directory . --capture --output-file network_tdd_coverage.info

            # Filter out system headers and test files
            COMMAND ${LCOV_PATH} --remove network_tdd_coverage.info
                '/usr/*' '*/tests/*' '*/mocks/*' '*/fixtures/*'
                --output-file network_tdd_coverage_filtered.info

            # Generate HTML report
            COMMAND ${GENHTML_PATH} -o network_tdd_coverage_html network_tdd_coverage_filtered.info

            DEPENDS network_tdd_tests
            COMMENT "Generating Network TDD Test Coverage Report"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()
endif()

# ==============================================================================
# Installation and Packaging
# ==============================================================================

# Install test executables (optional, for distribution)
if(INSTALL_TESTS)
    install(TARGETS ${NETWORK_TDD_TESTS}
        RUNTIME DESTINATION bin/tests/network
        COMPONENT Tests
    )

    # Install test data and fixtures
    install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/
        DESTINATION share/ETIStreamAnalyser/test_data/network
        COMPONENT Tests
        PATTERN "*.cpp" EXCLUDE
    )
endif()

# ==============================================================================
# Summary and Validation
# ==============================================================================

message(STATUS "Network Component TDD Test Suite Configuration Summary:")
message(STATUS "  - EDI Packet Parser TDD Tests: test_edi_packet_parser.cpp")
message(STATUS "  - ETI-over-IP Receiver TDD Tests: ${ETI_OVER_IP_RECEIVER_TDD_SOURCES}")
message(STATUS "  - Streaming Processor TDD Tests: ${STREAMING_PROCESSOR_TDD_SOURCES}")
message(STATUS "  - Broadcast Equipment Integration Tests: ${BROADCAST_EQUIPMENT_TDD_SOURCES}")
message(STATUS "  - Performance Validation Framework Tests: ${PERFORMANCE_VALIDATION_TDD_SOURCES}")
message(STATUS "  - Professional Workflow Integration Tests: ${WORKFLOW_INTEGRATION_TDD_SOURCES}")
message(STATUS "  - Performance Testing: ${ENABLE_PERFORMANCE_TESTING}")
message(STATUS "  - Network Testing: ${ENABLE_NETWORK_TESTING}")
message(STATUS "  - Memory Testing: ${ENABLE_MEMORY_TESTING}")
message(STATUS "  - Thread Safety Testing: ${ENABLE_THREAD_SAFETY_TESTING}")
message(STATUS "Network Component TDD Test Suite configured successfully!")

# Validation checks
if(NOT GTEST_LIBS)
    message(WARNING "Google Test not found - network TDD tests may not build correctly")
endif()

if(NOT TARGET Qt6::Network)
    message(WARNING "Qt6 Network module not found - network tests may not build correctly")
endif()

# Performance testing validation
if(ENABLE_PERFORMANCE_TESTING AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Performance testing enabled in Debug mode - results may not be representative")
endif()

message(STATUS "Network Component TDD Test Suite ready for execution")
message(STATUS "Run 'make run_network_tdd_tests' to execute all network TDD tests")
message(STATUS "Run 'make run_network_performance_tests' to execute performance validation tests")

# PFT Reassembler Unit Test Executable (NEW)
add_executable(test_pft_reassembler
    test_pft_reassembler.cpp
    ${CMAKE_SOURCE_DIR}/src/network/pft_reassembler.cpp
)

target_link_libraries(test_pft_reassembler
    PRIVATE
        ${GTEST_LIBS}
        $<$<PLATFORM_ID:Linux>:pthread>
)

target_compile_features(test_pft_reassembler PRIVATE cxx_std_17)

target_include_directories(test_pft_reassembler PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(test_pft_reassembler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network
    FOLDER "Tests/Network"
)

# Register PFT Reassembler test with CTest
add_test(
    NAME NetworkTDD_PFTReassembler
    COMMAND test_pft_reassembler
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/network
)

set_tests_properties(
    NetworkTDD_PFTReassembler
    PROPERTIES
        LABELS "Network;Unit;PFT;EDI"
        TIMEOUT 60  # 1 minute for unit tests
)
