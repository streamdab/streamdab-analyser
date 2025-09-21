# E2E Testing Framework for ETI Stream Analyzer
# Tests real user interactions and GUI workflows

cmake_minimum_required(VERSION 3.16)

# Find Qt6 components required for testing
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test PrintSupport)

# Enable Qt MOC for test files
set(CMAKE_AUTOMOC ON)

# REMOVED: Combined e2e_tests executable that caused multiple main() conflicts
# Each Qt Test file has its own QTEST_MAIN() macro which generates main() function
# Cannot combine multiple Qt Test files into single executable

# Copy test data files to build directory for all tests
file(COPY ../../eti DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

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
set_tests_properties(FileOpeningWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

# Create mandatory workflow coverage test target
add_executable(test_mandatory_workflow_coverage test_mandatory_workflow_coverage.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_mandatory_workflow_coverage PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_mandatory_workflow_coverage Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_mandatory_workflow_coverage PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_mandatory_workflow_coverage PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME MandatoryWorkflowCoverage COMMAND test_mandatory_workflow_coverage)
set_tests_properties(MandatoryWorkflowCoverage PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 45)

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
set_tests_properties(ETIFileProcessingWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

# Service Discovery Test
add_executable(test_service_discovery_workflow test_service_discovery.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_service_discovery_workflow PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_service_discovery_workflow Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_service_discovery_workflow PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_service_discovery_workflow PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME ServiceDiscoveryWorkflow COMMAND test_service_discovery_workflow)
set_tests_properties(ServiceDiscoveryWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

# GUI Integration Test
add_executable(test_gui_integration_workflow test_gui_integration.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_gui_integration_workflow PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_gui_integration_workflow Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_gui_integration_workflow PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_gui_integration_workflow PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME GUIIntegrationWorkflow COMMAND test_gui_integration_workflow)
set_tests_properties(GUIIntegrationWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

# Frame Analysis Test
add_executable(test_frame_analysis_workflow test_frame_analysis.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_frame_analysis_workflow PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_frame_analysis_workflow Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_frame_analysis_workflow PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_frame_analysis_workflow PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")

add_test(NAME FrameAnalysisWorkflow COMMAND test_frame_analysis_workflow)
set_tests_properties(FrameAnalysisWorkflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

# Additional Qt Test Framework Tests
add_executable(test_main_window_workflows test_main_window_workflows.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_main_window_workflows PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_main_window_workflows Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_main_window_workflows PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_main_window_workflows PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")
add_test(NAME MainWindowWorkflows COMMAND test_main_window_workflows)
set_tests_properties(MainWindowWorkflows PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

add_executable(test_file_operations test_file_operations.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_file_operations PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_file_operations Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_file_operations PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_file_operations PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")
add_test(NAME FileOperations COMMAND test_file_operations)
set_tests_properties(FileOperations PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

add_executable(test_menu_interactions test_menu_interactions.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_menu_interactions PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_menu_interactions Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_menu_interactions PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_menu_interactions PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")
add_test(NAME MenuInteractions COMMAND test_menu_interactions)
set_tests_properties(MenuInteractions PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

add_executable(test_comprehensive_workflows test_comprehensive_workflows.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_comprehensive_workflows PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_comprehensive_workflows Qt6::Core Qt6::Widgets Qt6::Test Qt6::PrintSupport etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_comprehensive_workflows PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_comprehensive_workflows PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")
add_test(NAME ComprehensiveWorkflows COMMAND test_comprehensive_workflows)
set_tests_properties(ComprehensiveWorkflows PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

# Google Test Framework Tests (using gtest instead of Qt Test)
# GTest is already configured in main CMakeLists.txt

add_executable(test_user_workflow_patterns test_user_workflow_patterns.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_user_workflow_patterns PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_user_workflow_patterns Qt6::Core Qt6::Widgets Qt6::Test gtest gtest_main gmock etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_user_workflow_patterns PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src .. ../fixtures ../mocks)
target_compile_definitions(test_user_workflow_patterns PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")
add_test(NAME UserWorkflowPatterns COMMAND test_user_workflow_patterns)
set_tests_properties(UserWorkflowPatterns PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

add_executable(test_panel_interactions test_panel_interactions.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_panel_interactions PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_panel_interactions Qt6::Core Qt6::Widgets Qt6::Test gtest gtest_main gmock etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_panel_interactions PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_panel_interactions PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")
add_test(NAME PanelInteractions COMMAND test_panel_interactions)
set_tests_properties(PanelInteractions PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

add_executable(test_performance_validation test_performance_validation.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_performance_validation PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_performance_validation Qt6::Core Qt6::Widgets Qt6::Test gtest gtest_main gmock etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_performance_validation PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_performance_validation PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")
add_test(NAME PerformanceValidation COMMAND test_performance_validation)
set_tests_properties(PerformanceValidation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 45)

add_executable(test_etsi_compliance_validation test_etsi_compliance_validation.cpp ${E2E_LIB_SOURCES})
set_target_properties(test_etsi_compliance_validation PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(test_etsi_compliance_validation Qt6::Core Qt6::Widgets Qt6::Test gtest gtest_main gmock etisnoop::etisnoop ${CMAKE_DL_LIBS})
target_include_directories(test_etsi_compliance_validation PRIVATE ../../src ../../src/gui ../../src/core ../../src/utils ../../external/etisnoop/src)
target_compile_definitions(test_etsi_compliance_validation PRIVATE QT_TESTCASE_BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}" TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data" ETI_TEST_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../eti")
add_test(NAME ETSIComplianceValidation COMMAND test_etsi_compliance_validation)
set_tests_properties(ETSIComplianceValidation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_LOGGING_RULES=*.debug=false" TIMEOUT 30)

# Comprehensive Test Suite Target
add_custom_target(run_comprehensive_e2e_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "ETIFileProcessingWorkflow|ServiceDiscoveryWorkflow|GUIIntegrationWorkflow|FrameAnalysisWorkflow|MainWindowWorkflows|FileOperations|MenuInteractions|ComprehensiveWorkflows" --verbose
    DEPENDS test_eti_file_processing_workflow test_service_discovery_workflow test_gui_integration_workflow test_frame_analysis_workflow test_main_window_workflows test_file_operations test_menu_interactions test_comprehensive_workflows
    COMMENT "Running comprehensive E2E test suite for CLAUDE.md requirements"
)

# Performance Validation Target
add_custom_target(run_performance_validation
    COMMAND ${CMAKE_CTEST_COMMAND} -R "ETIFileProcessingWorkflow|FrameAnalysisWorkflow" --verbose
    DEPENDS test_eti_file_processing_workflow test_frame_analysis_workflow
    COMMENT "Running performance validation tests (3s file load, <50ms latency, 24ms frame precision)"
)
