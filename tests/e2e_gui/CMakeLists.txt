# E2E GUI Testing Framework
# Designed to catch critical GUI stability issues like segmentation faults

cmake_minimum_required(VERSION 3.16)

# Find Qt6 Test module
find_package(Qt6 REQUIRED COMPONENTS Test Widgets Core Gui)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/build)

# Create E2E GUI test executable (renamed to avoid conflicts)
add_executable(eti_analyser_e2e_gui_tests
    test_main_workflow.cpp
    test_file_operations.cpp
    test_gui_stability.cpp
    # Add needed source files directly
    ${CMAKE_SOURCE_DIR}/src/core/eti_processor.cpp
    ${CMAKE_SOURCE_DIR}/src/core/modern_eti_frame_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/core/enhanced_fig_analyser.cpp
    ${CMAKE_SOURCE_DIR}/src/core/performance_profiler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/eti_types.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/logger.cpp
)

# Link Qt6 and project libraries
target_link_libraries(eti_analyser_e2e_gui_tests
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    # Link against the main project executable's sources
    ${COMMON_LIBS}
    ${CMAKE_DL_LIBS}
)

# Enable Qt automoc
set_target_properties(eti_analyser_e2e_gui_tests PROPERTIES
    AUTOMOC ON
)

# Add test to CTest
add_test(NAME E2EGUITests COMMAND eti_analyser_e2e_gui_tests)
