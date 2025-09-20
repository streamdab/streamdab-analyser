# E2E Testing Framework for ETI Stream Analyzer
# Tests real user interactions and GUI workflows

cmake_minimum_required(VERSION 3.16)

# Find Qt6 components required for testing
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

# Enable Qt MOC for test files
set(CMAKE_AUTOMOC ON)

# Create E2E test executable
set(E2E_TEST_SOURCES
    test_main_window_workflows.cpp
    test_file_operations.cpp
    test_menu_interactions.cpp
    test_eti_file_opening.cpp
    test_comprehensive_workflows.cpp
    # Include main application sources for testing
    ../../src/main.cpp
    ../../src/gui/main_window.cpp
    ../../src/gui/analyzer_widget.cpp
    ../../src/gui/service_browser.cpp
    ../../src/gui/constellation_widget.cpp
    ../../src/gui/settings_dialog.cpp
    ../../src/gui/about_dialog.cpp
    # ../../src/gui/alert_config_dialog.cpp  # Not yet implemented
    ../../src/gui/audio_monitoring_widget.cpp
    ../../src/core/eti_processor.cpp
    ../../src/core/etisnoop_wrapper.cpp
    # ../../src/core/alert_system.cpp  # Not yet implemented
    # ../../src/core/network_stream.cpp  # Not yet implemented
    # ../../src/core/stream_recording.cpp  # Not yet implemented
    ../../src/core/dab_audio_decoder.cpp
    ../../src/utils/logger.cpp
    ../../src/utils/report_generator.cpp
)

add_executable(e2e_tests ${E2E_TEST_SOURCES})

# Link Qt libraries for E2E testing
target_link_libraries(e2e_tests
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    yaml-cpp
    ${CMAKE_DL_LIBS}
)

# Include directories for E2E tests
target_include_directories(e2e_tests PRIVATE
    ../../src
    ../../src/gui
    ../../src/core
    ../../src/utils
    ../../external/etisnoop/src
)

# Compiler definitions for testing
target_compile_definitions(e2e_tests PRIVATE
    QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}"
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data"
    ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti"
)

# Add test discovery for CTest integration
add_test(NAME E2ETests COMMAND e2e_tests)

# Copy test data files to build directory
file(COPY ../../eti DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Set test properties for proper execution
set_tests_properties(E2ETests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 300
)

# Create individual test targets for specific workflows
add_executable(test_file_opening_workflow test_eti_file_opening.cpp ${E2E_TEST_SOURCES})
target_link_libraries(test_file_opening_workflow Qt6::Core Qt6::Widgets Qt6::Test yaml-cpp ${CMAKE_DL_LIBS})
target_include_directories(test_file_opening_workflow PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_file_opening_workflow PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME FileOpeningWorkflow COMMAND test_file_opening_workflow)
set_tests_properties(FileOpeningWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)

# Enable verbose test output for debugging
set(CMAKE_CTEST_ARGUMENTS "--output-on-failure")
