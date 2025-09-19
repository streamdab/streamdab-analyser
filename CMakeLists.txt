cmake_minimum_required(VERSION 3.20)

# Project configuration
project(ETIStreamAnalyzer
    VERSION 1.0.0
    DESCRIPTION "Professional ETI Stream Analyzer for DAB/DAB+ Broadcasting"
    LANGUAGES CXX
)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Enable testing from project start (TDD requirement)
enable_testing()

message(STATUS "=== ETI Stream Analyzer Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")

# Common dependencies (cross-platform)
find_package(PkgConfig REQUIRED)

# Qt6 framework (cross-platform)
find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Network
    Test
    Multimedia
)

# Enable Qt automation
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Platform detection and configuration
if(WIN32)
    include(cmake/WindowsConfig.cmake)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    include(cmake/MacOSConfig.cmake)
    set(PLATFORM_NAME "macOS")
elseif(UNIX)
    include(cmake/LinuxConfig.cmake)
    set(PLATFORM_NAME "Linux")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

message(STATUS "Platform configuration: ${PLATFORM_NAME}")
pkg_check_modules(FAAD2 REQUIRED faad2)
pkg_check_modules(FFTW3 REQUIRED fftw3)

# Optional YAML configuration support
find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
    message(STATUS "YAML-CPP found: Configuration features enabled")
    set(HAVE_YAML_CPP ON)
else()
    message(WARNING "YAML-CPP not found: Some configuration features disabled")
    set(HAVE_YAML_CPP OFF)
endif()

# Optional nlohmann/json for StreamDAB integration
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    message(STATUS "nlohmann/json found: Enhanced ETISnoop integration enabled")
    set(HAVE_NLOHMANN_JSON ON)
else()
    message(WARNING "nlohmann/json not found: Some ETISnoop features disabled")
    set(HAVE_NLOHMANN_JSON OFF)
endif()

# ETISnoop Integration - Add submodule
message(STATUS "Adding ETISnoop backend integration...")
add_subdirectory(third_party/etisnoop)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/etisnoop/src
)

# Minimal core source files for basic functionality
set(CORE_SOURCE_FILES
    src/main.cpp
    src/core/eti_processor.cpp
    src/core/eti_processor.h
    src/core/etisnoop_wrapper.cpp
    src/core/etisnoop_wrapper.h
    src/core/eti_types.cpp
    src/core/eti_types.h
    src/core/dab_decoder.cpp
    src/core/dab_decoder.h
    src/core/fic_decoder.cpp
    src/core/fic_decoder.h
    src/core/ensemble_manager.cpp
    src/core/ensemble_manager.h
    src/core/service_manager.cpp
    src/core/service_manager.h
    src/gui/main_window.cpp
    src/gui/main_window.h
    src/gui/analyzer_widget.cpp
    src/gui/analyzer_widget.h
    src/gui/service_browser.cpp
    src/gui/service_browser.h
    src/gui/constellation_widget.cpp
    src/gui/constellation_widget.h
    src/gui/settings_dialog.cpp
    src/gui/settings_dialog.h
    src/gui/about_dialog.cpp
    src/gui/about_dialog.h
    src/utils/logger.cpp
    src/utils/logger.h
    src/utils/config_manager.cpp
    src/utils/config_manager.h
)

# Use only core files for minimal build
set(SOURCE_FILES ${CORE_SOURCE_FILES})

# Resource files
set(RESOURCE_FILES
    resources/application.qrc
)

# Debug: Print remaining source files
message(STATUS "Remaining SOURCE_FILES after filtering:")
foreach(file ${SOURCE_FILES})
    message(STATUS "  ${file}")
endforeach()

# Create main executable
add_executable(${PROJECT_NAME}
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
)

# Common libraries (cross-platform)
set(COMMON_LIBS
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    Qt6::Multimedia
    ${pkgcfg_lib_FAAD2_faad}
    ${pkgcfg_lib_FFTW3_fftw3}
    etisnoop::etisnoop
)

# Add optional YAML support
if(yaml-cpp_FOUND)
    list(APPEND COMMON_LIBS yaml-cpp::yaml-cpp)
endif()

# Add optional nlohmann/json support
if(nlohmann_json_FOUND)
    list(APPEND COMMON_LIBS nlohmann_json::nlohmann_json)
endif()

# Link common libraries
target_link_libraries(${PROJECT_NAME} ${COMMON_LIBS})

# Add platform-specific libraries
if(WIN32)
    add_windows_libraries(${PROJECT_NAME})
elseif(APPLE)
    add_macos_libraries(${PROJECT_NAME})
elseif(UNIX)
    add_linux_libraries(${PROJECT_NAME})
endif()

# Include directories for dependencies
target_include_directories(${PROJECT_NAME} PRIVATE
    ${FAAD2_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_NO_DEPRECATED_WARNINGS
    ${FAAD2_CFLAGS_OTHER}
    ${FFTW3_CFLAGS_OTHER}
)

if(HAVE_YAML_CPP)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_YAML_CPP)
endif()

if(HAVE_NLOHMANN_JSON)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_NLOHMANN_JSON)
endif()

# Testing configuration
add_subdirectory(tests)

# Packaging configuration (platform-specific settings loaded from platform configs)
include(CPack)

message(STATUS "=== Configuration Complete ===")
message(STATUS "Target platform: ${PLATFORM_NAME}")
message(STATUS "Qt version: ${Qt6_VERSION}")
message(STATUS "YAML support: ${HAVE_YAML_CPP}")
message(STATUS "Testing: Enabled")
message(STATUS "==========================================")
