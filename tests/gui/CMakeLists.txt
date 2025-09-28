# GUI Tests CMakeLists.txt
# TDD Test Suite for ETI UI Integration with Real Bangkok ETI File

# GUI test requires Qt Test framework and GUI application
find_package(Qt6 REQUIRED COMPONENTS Test Widgets Network)

# Enable Qt6 automoc for signal/slot testing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories for GUI components
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_BINARY_DIR}  # For generated files
)

# TDD ETI UI Integration Test
add_executable(test_eti_ui_integration
    test_eti_ui_integration.cpp
)

target_link_libraries(test_eti_ui_integration
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    streamdab_core
    streamdab_gui
    streamdab_utils
)

# Set test properties for GUI testing
set_target_properties(test_eti_ui_integration PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

# Add test to CTest framework
add_test(
    NAME ETI_UI_Integration_TDD_Test
    COMMAND test_eti_ui_integration
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Set test environment
set_tests_properties(ETI_UI_Integration_TDD_Test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"  # Headless GUI testing
    TIMEOUT 60  # 60 seconds timeout for UI tests
)

# TDD Phase Labels for test organization
set_tests_properties(ETI_UI_Integration_TDD_Test PROPERTIES
    LABELS "GUI;TDD;Integration;Bangkok_ETI"
)

# === UI-Backend Integration Test ===
add_executable(test_ui_backend_integration
    test_ui_backend_integration.cpp
)

target_link_libraries(test_ui_backend_integration
    Qt6::Test
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    streamdab_core
    streamdab_gui
    streamdab_utils
    streamdab_network
)

# Set test properties for UI-Backend integration testing
set_target_properties(test_ui_backend_integration PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

# Add test to CTest framework
add_test(
    NAME UI_Backend_Integration_Test
    COMMAND test_ui_backend_integration
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Set test environment for comprehensive integration
set_tests_properties(UI_Backend_Integration_Test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"  # Headless GUI testing
    TIMEOUT 120  # 120 seconds timeout for integration tests
)

# TDD Phase Labels for comprehensive testing
set_tests_properties(UI_Backend_Integration_Test PROPERTIES
    LABELS "GUI;Integration;TDD;Network;Performance;ETSI"
)

message(STATUS "GUI TDD Tests configured - ETI UI Integration Test Suite ready")
