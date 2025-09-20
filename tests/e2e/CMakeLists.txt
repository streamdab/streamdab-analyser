# E2E Testing Framework for ETI Stream Analyzer
# Tests real user interactions and GUI workflows

cmake_minimum_required(VERSION 3.16)

# Find Qt6 components required for testing
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test PrintSupport)

# Enable Qt MOC for test files
set(CMAKE_AUTOMOC ON)

# Create E2E test executable
set(E2E_TEST_SOURCES
    test_main_window_workflows.cpp
    test_file_operations.cpp
    test_menu_interactions.cpp
    test_eti_file_opening.cpp
    test_comprehensive_workflows.cpp
    test_mandatory_workflow_coverage.cpp
    # New comprehensive E2E test cases from CLAUDE.md requirements
    test_eti_file_processing.cpp
    test_service_discovery.cpp
    test_gui_integration.cpp
    test_frame_analysis.cpp
    # Include main application sources for testing (EXCLUDE main.cpp to avoid multiple main() definitions)
    ../../src/gui/main_window.cpp
    ../../src/gui/analyzer_widget.cpp
    ../../src/gui/service_browser.cpp
    ../../src/gui/constellation_widget.cpp
    ../../src/gui/settings_dialog.cpp
    ../../src/gui/about_dialog.cpp
    # ../../src/gui/alert_config_dialog.cpp  # Not yet implemented
    ../../src/gui/audio_monitoring_widget.cpp
    ../../src/gui/service_explorer_panel.cpp
    ../../src/gui/eti_analysis_widget.cpp
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

# Set C++20 standard for test targets
set_target_properties(e2e_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Link Qt libraries for E2E testing
target_link_libraries(e2e_tests
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    Qt6::PrintSupport
    etisnoop::etisnoop
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
set(E2E_LIB_SOURCES
    ../../src/gui/main_window.cpp
    ../../src/gui/analyzer_widget.cpp
    ../../src/gui/service_browser.cpp
    ../../src/gui/constellation_widget.cpp
    ../../src/gui/settings_dialog.cpp
    ../../src/gui/about_dialog.cpp
    ../../src/gui/audio_monitoring_widget.cpp
    ../../src/gui/service_explorer_panel.cpp
    ../../src/gui/eti_analysis_widget.cpp
    ../../src/gui/eti_service_tree_model.cpp
    ../../src/gui/eti_frame_list_model.cpp
    ../../src/gui/fig_analysis_widget.cpp
    ../../src/core/eti_processor.cpp
    ../../src/core/etisnoop_wrapper.cpp
    ../../src/core/eti_file_worker.cpp
    ../../src/core/eti_types.cpp
    ../../src/core/dab_decoder.cpp
    ../../src/core/fic_decoder.cpp
    ../../src/core/ensemble_manager.cpp
    ../../src/core/service_manager.cpp
    ../../src/core/dab_audio_decoder.cpp
    ../../src/utils/logger.cpp
    ../../src/utils/config_manager.cpp
    ../../src/utils/report_generator.cpp
)

add_executable(test_file_opening_workflow test_eti_file_opening.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_file_opening_workflow PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_file_opening_workflow Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_file_opening_workflow PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_file_opening_workflow PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME FileOpeningWorkflow COMMAND test_file_opening_workflow)
set_tests_properties(FileOpeningWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)

# Create mandatory workflow coverage test target
add_executable(test_mandatory_workflow_coverage test_mandatory_workflow_coverage.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_mandatory_workflow_coverage PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_mandatory_workflow_coverage Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_mandatory_workflow_coverage PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_mandatory_workflow_coverage PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME MandatoryWorkflowCoverage COMMAND test_mandatory_workflow_coverage)
set_tests_properties(MandatoryWorkflowCoverage PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 300)

# Enable verbose test output for debugging
set(CMAKE_CTEST_ARGUMENTS "--output-on-failure")

# Create individual test targets for comprehensive E2E test cases
# ETI File Processing Test
add_executable(test_eti_file_processing_workflow test_eti_file_processing.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_eti_file_processing_workflow PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_eti_file_processing_workflow Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_eti_file_processing_workflow PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_eti_file_processing_workflow PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME ETIFileProcessingWorkflow COMMAND test_eti_file_processing_workflow)
set_tests_properties(ETIFileProcessingWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 120)

# Service Discovery Test
add_executable(test_service_discovery_workflow test_service_discovery.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_service_discovery_workflow PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_service_discovery_workflow Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_service_discovery_workflow PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_service_discovery_workflow PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME ServiceDiscoveryWorkflow COMMAND test_service_discovery_workflow)
set_tests_properties(ServiceDiscoveryWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 120)

# GUI Integration Test
add_executable(test_gui_integration_workflow test_gui_integration.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_gui_integration_workflow PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_gui_integration_workflow Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_gui_integration_workflow PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_gui_integration_workflow PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME GUIIntegrationWorkflow COMMAND test_gui_integration_workflow)
set_tests_properties(GUIIntegrationWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 120)

# Frame Analysis Test
add_executable(test_frame_analysis_workflow test_frame_analysis.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_frame_analysis_workflow PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_frame_analysis_workflow Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_frame_analysis_workflow PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_frame_analysis_workflow PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME FrameAnalysisWorkflow COMMAND test_frame_analysis_workflow)
set_tests_properties(FrameAnalysisWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 120)

# Comprehensive Test Suite Target
add_custom_target(run_comprehensive_e2e_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "ETIFileProcessingWorkflow|ServiceDiscoveryWorkflow|GUIIntegrationWorkflow|FrameAnalysisWorkflow" --verbose
    DEPENDS test_eti_file_processing_workflow test_service_discovery_workflow test_gui_integration_workflow test_frame_analysis_workflow
    COMMENT "Running comprehensive E2E test suite for CLAUDE.md requirements"
)

# Performance Validation Target
add_custom_target(run_performance_validation
    COMMAND ${CMAKE_CTEST_COMMAND} -R "ETIFileProcessingWorkflow|FrameAnalysisWorkflow" --verbose
    DEPENDS test_eti_file_processing_workflow test_frame_analysis_workflow
    COMMENT "Running performance validation tests (3s file load, <50ms latency, 24ms frame precision)"
)
