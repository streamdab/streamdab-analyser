# Core Component Tests - TDD Implementation
# This file configures all core component tests for the TDD workflow

# Add comprehensive TDD test for 100% compliance
add_executable(test_eti_processor_comprehensive
    test_eti_processor_comprehensive.cpp
)

# Link against required libraries
target_link_libraries(test_eti_processor_comprehensive
    PRIVATE
    Qt6::Core
    Qt6::Test
    ETIStreamAnalyser_lib
)

# Include directories
target_include_directories(test_eti_processor_comprehensive
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Set C++20 standard
set_property(TARGET test_eti_processor_comprehensive PROPERTY CXX_STANDARD 20)

# Add as a test
add_test(NAME ComprehensiveETIProcessorTDD COMMAND test_eti_processor_comprehensive)

# Add our new ETI frame parsing fix test
add_executable(test_eti_frame_parsing_fix
    test_eti_frame_parsing_fix.cpp
    ${CMAKE_SOURCE_DIR}/src/core/modern_eti_frame_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/core/eti_types.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
)

# Link against Qt6 and GTest
target_link_libraries(test_eti_frame_parsing_fix
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    gtest
    gtest_main
)

# Include directories
target_include_directories(test_eti_frame_parsing_fix
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Set C++20 standard
set_property(TARGET test_eti_frame_parsing_fix PROPERTY CXX_STANDARD 20)

# Add as a test
add_test(NAME ETIFrameParsingFix COMMAND test_eti_frame_parsing_fix)

# === Week 1 Comprehensive Test Suite (NEW) ===
# FIG 1/0 + FIG 0/18 TDD tests following PDCA methodology
add_executable(test_fig_week1_comprehensive
    test_fig_week1_comprehensive.cpp
)

# Link against required libraries
target_link_libraries(test_fig_week1_comprehensive
    PRIVATE
    Qt6::Core
    Qt6::Test
    ETIStreamAnalyser_lib
)

# Include directories
target_include_directories(test_fig_week1_comprehensive
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Set C++20 standard
set_property(TARGET test_fig_week1_comprehensive PROPERTY CXX_STANDARD 20)

# Add as a test
add_test(NAME FigWeek1ComprehensiveTDD COMMAND test_fig_week1_comprehensive)

# Also include existing core tests
set(CORE_TEST_SOURCES
    test_eti_frame_parser.cpp
    test_eti_processor.cpp
    test_etsi_compliance.cpp
    test_fig_analysis.cpp
    test_eti_frame_error_handling_tdd.cpp
    test_clang_tidy_configuration_fix.cpp
)

foreach(TEST_SOURCE ${CORE_TEST_SOURCES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE})
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SOURCE})
        target_link_libraries(${TEST_NAME} PRIVATE Qt6::Core gtest gtest_main)
        target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
        target_compile_definitions(${TEST_NAME} PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
        set_property(TARGET ${TEST_NAME} PROPERTY CXX_STANDARD 20)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endif()
endforeach()
