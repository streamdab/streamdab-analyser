cmake_minimum_required(VERSION 3.21)
project(StreamDAB_Analyzer VERSION 1.1.0 LANGUAGES CXX)

# ========================================
# StreamDAB Analyzer - Professional Build Configuration
# ========================================
message(STATUS "========================================")
message(STATUS "StreamDAB Analyzer - Professional Build Configuration")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Professional build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# Qt6 components - Complete Professional Suite
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Concurrent
    Network
    Gui
    PrintSupport
    Test
    Multimedia
)
qt6_standard_project_setup()

# Configure Qt Designer UI file search paths
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)

message(STATUS "Qt6 Version: ${Qt6_VERSION}")

# Platform detection
if(WIN32)
    message(STATUS "Platform: Windows")
elseif(APPLE)
    message(STATUS "Platform: macOS")
else()
    message(STATUS "Platform: Linux")
endif()

# Professional dependency management
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Required libraries for ETI processing
pkg_check_modules(FAAD2 REQUIRED faad2)
pkg_check_modules(FFTW3 REQUIRED fftw3)

# ZeroMQ for ODR-DabMux connectivity
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
if(ZMQ_FOUND)
    message(STATUS "ZeroMQ: Found version ${ZMQ_VERSION}")
else()
    message(FATAL_ERROR "ZeroMQ: Required library not found")
endif()

# Optional libraries
find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
    message(STATUS "yaml-cpp: Found")
else()
    message(STATUS "yaml-cpp: Not found (optional)")
endif()

# ========================================
# CORE APPLICATION SOURCES
# ========================================

# Core ETI processing engine
set(CORE_SOURCES
    src/core/eti_processor.cpp
    src/core/eti_file_worker.cpp
    src/core/eti_processing_engine.cpp
    src/core/eti_types.cpp
    src/core/modern_eti_frame_parser.cpp
    src/core/headless_eti_processor.cpp
    src/core/optimized_eti_processor.cpp
    src/core/ultra_performance_eti_processor.cpp
    src/core/eti_ni_processor.cpp
    src/core/reed_solomon_codec.cpp
    src/core/modern_eti_controller.cpp

    # DAB processing
    src/core/dab_decoder.cpp
    src/core/dab_service_discovery_engine.cpp
    src/core/dab_audio_decoder.cpp
    src/core/audio_decoder.cpp
    src/core/audio_extraction_pipeline.cpp

    # FIC and FIG processing
    src/core/fic_decoder.cpp
    src/core/fig_parser.cpp
    src/core/enhanced_fig_analyzer.cpp

    # Service management
    src/core/service_manager.cpp
    src/core/ensemble_manager.cpp

    # Network streaming
    src/core/network_stream_receiver.cpp
    src/core/multi_stream_processor.cpp
    src/core/stream_recording_manager.cpp
    src/core/zeromq_eti_client.cpp

    # Phase 2: Streaming Engine and Resilience
    src/core/streaming_engine.cpp
    src/core/stream_resilience.cpp

    # ETSI compliance
    src/core/comprehensive_etsi_validator.cpp
    src/core/thai_nbtc_validator.cpp
    src/core/standards_compliance_perfection.cpp

    # Performance and monitoring
    src/core/performance_profiler.cpp
    src/core/eti_memory_pool.cpp
    src/core/eti_processing_cache.cpp
    src/core/ai_signal_intelligence_framework.cpp
)

# ETSI Standards compliance subsystem
set(ETSI_SOURCES
    src/core/etsi/compliance_engine.cpp
    src/core/etsi/compliance_logger.cpp
    src/core/etsi/standard_validators.cpp
    src/core/etsi/alert_system.cpp
    src/core/etsi/realtime_monitor.cpp
)

# GUI Application components
set(GUI_SOURCES
    src/gui/main_window.cpp
    src/gui/analyzer_widget.cpp
    src/gui/constellation_widget.cpp
    src/gui/service_browser.cpp
    src/gui/service_explorer_panel.cpp
    src/gui/settings_dialog.cpp
    src/gui/about_dialog.cpp
    src/gui/alert_configuration_dialog.cpp
    src/gui/stream_recording_dialog.cpp

    # Data models
    src/gui/eti_service_tree_model.cpp
    src/gui/eti_frame_list_model.cpp

    # Analysis widgets
    src/gui/eti_analysis_widget.cpp
    src/gui/fig_analysis_widget.cpp
    src/gui/audio_monitoring_widget.cpp
    src/gui/performance_dashboard.cpp

    # Professional workflow
    src/gui/professional_workflow_manager.cpp
    src/gui/etsi_compliance_monitor.cpp
    src/gui/integration_validator.cpp

    # UI framework
    src/gui/progress_manager.cpp
    src/gui/status_manager.cpp
    src/gui/error_indicator_system.cpp
    src/gui/context_menu_factory.cpp
    src/gui/real_time_controller.cpp
    src/gui/signal_router.cpp

    # Themes and rendering
    src/gui/broadcast_theme.cpp
    src/gui/enhanced_broadcast_theme.cpp
    src/gui/real_time_chart_widget.cpp
    src/gui/professional_export_manager.cpp

    # Integration components
    src/gui/main_window_alert_integration.cpp
    src/gui/audio_decoder_integration.cpp
    src/gui/test_gui_integration.cpp
)

# Network processing components
set(NETWORK_SOURCES
    src/network/eti_over_ip_receiver.cpp
    src/network/eti_network_integration.cpp
    src/network/network_discovery.cpp
    src/network/streaming_processor.cpp
    src/network/stream_playback.cpp
    src/network/circular_buffer_manager.cpp
)

# Utility components
set(UTILS_SOURCES
    src/utils/logger.cpp
    src/utils/config_manager.cpp
    src/utils/file_utils.cpp
    src/utils/signal_processing.cpp
    src/utils/report_generator.cpp
)

# Performance benchmarking
set(PERFORMANCE_SOURCES
    src/performance/ultra_performance_benchmark_suite.cpp
)

# ========================================
# MAIN APPLICATION EXECUTABLE
# ========================================

add_executable(StreamDAB_Analyzer
    src/main.cpp
    ${CORE_SOURCES}
    ${ETSI_SOURCES}
    ${GUI_SOURCES}
    ${NETWORK_SOURCES}
    ${UTILS_SOURCES}
    ${PERFORMANCE_SOURCES}
)

# Include directories
target_include_directories(StreamDAB_Analyzer PRIVATE
    src
    src/core
    src/gui
    src/network
    src/utils
    src/performance
    ${FAAD2_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
)

# Link Qt6 libraries - Professional suite
target_link_libraries(StreamDAB_Analyzer
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Gui
    Qt6::PrintSupport
    Qt6::Multimedia
    Threads::Threads
    ${FAAD2_LIBRARIES}
    ${FFTW3_LIBRARIES}
    ${ZMQ_LIBRARIES}
)

# Link optional libraries
if(yaml-cpp_FOUND)
    target_link_libraries(StreamDAB_Analyzer yaml-cpp::yaml-cpp)
endif()

# Professional build settings
target_compile_definitions(StreamDAB_Analyzer PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x060400
    STREAMDAB_VERSION="${PROJECT_VERSION}"
    ${FAAD2_CFLAGS_OTHER}
    ${FFTW3_CFLAGS_OTHER}
    ${ZMQ_CFLAGS_OTHER}
)

# Compiler-specific settings
if(MSVC)
    target_compile_options(StreamDAB_Analyzer PRIVATE /W4)
    target_compile_definitions(StreamDAB_Analyzer PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(StreamDAB_Analyzer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ========================================
# TDD FRAMEWORK INTEGRATION
# ========================================

# Enable testing framework
enable_testing()
option(ENABLE_TESTING "Enable comprehensive TDD testing framework" ON)
option(ENABLE_COVERAGE "Enable TDD code coverage reporting" ON)
option(ENABLE_TDD_BENCHMARKS "Enable TDD performance benchmark tests" ON)

if(ENABLE_TESTING)
    message(STATUS "TDD Framework: ENABLED")
    add_subdirectory(tests)
else()
    message(STATUS "TDD Framework: DISABLED")
endif()

# ========================================
# PROFESSIONAL INSTALLATION
# ========================================

include(GNUInstallDirs)

# Install the executable
install(TARGETS StreamDAB_Analyzer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install documentation
if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
    install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()

# Install additional documentation files
file(GLOB DOC_FILES "docs/*.md")
if(DOC_FILES)
    install(FILES ${DOC_FILES} DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()

# ========================================
# PROFESSIONAL PACKAGING
# ========================================

# Basic package information
set(CPACK_PACKAGE_NAME "StreamDAB-Analyzer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional ETI Stream Analyzer for Broadcast Industry")
set(CPACK_PACKAGE_DESCRIPTION "Advanced ETSI-compliant ETI stream analysis tool for professional broadcast monitoring and DAB+ service discovery with real-time processing capabilities.")
set(CPACK_PACKAGE_VENDOR "StreamDAB Technologies")
set(CPACK_PACKAGE_CONTACT "support@streamdab.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://streamdab.com")
set(CPACK_PACKAGE_MAINTAINER "StreamDAB Technologies <support@streamdab.com>")

# Platform-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "WIX;NSIS")
    set(CPACK_WIX_UPGRADE_GUID "12345678-1234-1234-1234-123456789012")
    set(CPACK_WIX_PRODUCT_GUID "87654321-4321-4321-4321-210987654321")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "StreamDAB Analyzer")
elseif(APPLE)
    set(CPACK_GENERATOR "Bundle;DragNDrop")
    set(CPACK_BUNDLE_NAME "StreamDAB Analyzer")
    set(CPACK_BUNDLE_PLIST "MacOSX/Info.plist")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6network6, libfaad2, libfftw3-3")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, faad2-libs, fftw")
endif()

# Available generators
get_property(CPACK_AVAILABLE_GENERATORS GLOBAL PROPERTY CPACK_GENERATORS)
message(STATUS "Available Generators: ${CPACK_AVAILABLE_GENERATORS}")

include(CPack)

# ========================================
# BUILD STATUS SUMMARY
# ========================================

message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Available Generators: ${CPACK_AVAILABLE_GENERATORS}")
message(STATUS "========================================")
