# Network Streaming Tests CMakeLists.txt
# TDD test suite for professional streaming components

cmake_minimum_required(VERSION 3.21)

# Network streaming test executable
add_executable(test_network_streaming
    test_stream_recording_manager.cpp
    test_audio_extraction_pipeline.cpp
    test_multi_stream_processor.cpp
    test_realtime_buffer_manager.cpp
    test_stream_quality_monitor.cpp
)

# Link with main project libraries and test frameworks
target_link_libraries(test_network_streaming
    PRIVATE
    gtest gtest_main gmock gmock_main  # Use the fetched GTest targets
    Qt6::Core
    Qt6::Network
    Qt6::Test
    ${FFTW3_LIBRARIES}
)

# Include directories
target_include_directories(test_network_streaming
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
    ${FAAD2_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)

# Compile definitions
target_compile_definitions(test_network_streaming
    PRIVATE
    ${FAAD2_CFLAGS_OTHER}
    ${FFTW3_CFLAGS_OTHER}
    TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

# Add test to CTest
add_test(
    NAME NetworkStreamingTests
    COMMAND test_network_streaming
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Set test properties
set_tests_properties(NetworkStreamingTests PROPERTIES
    TIMEOUT 120
    LABELS "network;streaming;professional"
)

# TDD-specific test targets
add_custom_target(test-network-streaming
    COMMAND ${CMAKE_CTEST_COMMAND} -L "network" --output-on-failure
    DEPENDS test_network_streaming
    COMMENT "Run network streaming TDD tests"
)

# Performance test target
add_custom_target(test-streaming-performance
    COMMAND test_network_streaming --gtest_filter="*Performance*" --gtest_verbose
    DEPENDS test_network_streaming
    COMMENT "Run streaming performance tests"
)

# Integration test target
add_custom_target(test-streaming-integration
    COMMAND test_network_streaming --gtest_filter="*Integration*:*EndToEnd*" --gtest_verbose
    DEPENDS test_network_streaming
    COMMENT "Run streaming integration tests"
)
