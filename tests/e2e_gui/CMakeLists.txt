# E2E GUI Testing Framework
# Designed to catch critical GUI stability issues like segmentation faults

cmake_minimum_required(VERSION 3.16)

# Find Qt6 Test module
find_package(Qt6 REQUIRED COMPONENTS Test Widgets Core Gui)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/build)

# Create E2E GUI test executable
add_executable(eti_analyzer_e2e_tests
    test_main_workflow.cpp
    test_file_operations.cpp
    test_gui_stability.cpp
)

# Link Qt6 and project libraries
target_link_libraries(eti_analyzer_e2e_tests
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    # Link against internal project components only
    ${CMAKE_SOURCE_DIR}/build/CMakeFiles/ETIStreamAnalyzer.dir/src/core/eti_processor.cpp.o
    ${CMAKE_SOURCE_DIR}/build/CMakeFiles/ETIStreamAnalyzer.dir/src/core/eti_file_worker.cpp.o
    ${CMAKE_SOURCE_DIR}/build/CMakeFiles/ETIStreamAnalyzer.dir/src/utils/logger.cpp.o
)

# Enable Qt automoc
set_target_properties(eti_analyzer_e2e_tests PROPERTIES
    AUTOMOC ON
)

# Add test to CTest
add_test(NAME E2EGUITests COMMAND eti_analyzer_e2e_tests)
