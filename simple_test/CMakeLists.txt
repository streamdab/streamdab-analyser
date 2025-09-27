# Simple CMakeLists.txt for testing GUI fixes only
cmake_minimum_required(VERSION 3.16)
project(SimpleGUITest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

# Set Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Core source files needed for MainWindow
set(CORE_SOURCES
    src/utils/logger.cpp
    src/core/eti_processor.cpp
    src/core/headless_eti_processor.cpp
    src/core/modern_eti_frame_parser.cpp
    src/core/enhanced_fig_analyzer.cpp
    src/core/eti_processing_engine.cpp
    src/core/eti_file_worker.cpp
    src/core/comprehensive_etsi_validator.cpp
    src/core/fig_parser.cpp
    src/core/thai_nbtc_validator.cpp
    src/gui/main_window.cpp
    src/gui/service_explorer_panel.cpp
    src/gui/analyzer_widget.cpp
    src/gui/constellation_widget.cpp
    src/gui/service_browser.cpp
    src/gui/settings_dialog.cpp
    src/gui/about_dialog.cpp
    src/gui/alert_configuration_dialog.cpp
    src/gui/stream_recording_dialog.cpp
    src/gui/eti_service_tree_model.cpp
    src/gui/eti_frame_list_model.cpp
    src/gui/fig_analysis_widget.cpp
    src/gui/audio_monitoring_widget.cpp
    src/gui/performance_dashboard.cpp
    src/gui/etsi_compliance_monitor.cpp
    src/gui/eti_analysis_widget.cpp
    src/gui/professional_workflow_manager.cpp
    src/utils/config_manager.cpp
)

# Create simple test executable
add_executable(simple_gui_test
    simple_gui_test.cpp
    ${CORE_SOURCES}
)

# Link Qt libraries
target_link_libraries(simple_gui_test
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
)
