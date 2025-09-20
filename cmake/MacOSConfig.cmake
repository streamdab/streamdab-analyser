# macOS-specific build configuration
# For ETI Stream Analyzer on macOS platforms

message(STATUS "Configuring for macOS platform")

# macOS-specific dependencies
find_library(COREAUDIO_FRAMEWORK CoreAudio)
find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
find_library(FOUNDATION_FRAMEWORK Foundation)
find_library(COCOA_FRAMEWORK Cocoa)

# macOS-specific audio decoder handling with Homebrew support
if(APPLE)
    # Try to find fdk-aac first (preferred for professional DAB+ decoding)
    find_path(FDK_AAC_INCLUDE_DIR NAMES fdk-aac/aacdecoder_lib.h
        PATHS
            /usr/local/include
            /opt/homebrew/include
            /usr/include
            "${CMAKE_PREFIX_PATH}/include"
    )
    find_library(FDK_AAC_LIBRARY NAMES fdk-aac
        PATHS
            /usr/local/lib
            /opt/homebrew/lib
            /usr/lib
            "${CMAKE_PREFIX_PATH}/lib"
    )

    # Fallback to FAAD2 if fdk-aac not found
    if(NOT FDK_AAC_LIBRARY)
        find_path(FAAD2_INCLUDE_DIR NAMES neaacdec.h
            PATHS
                /usr/local/include
                /opt/homebrew/include
                /usr/include
                "${CMAKE_PREFIX_PATH}/include"
        )
        find_library(FAAD2_LIBRARY NAMES faad2
            PATHS
                /usr/local/lib
                /opt/homebrew/lib
                /usr/lib
                "${CMAKE_PREFIX_PATH}/lib"
        )
    endif()
endif()

# Verify macOS frameworks are found
if(NOT COREAUDIO_FRAMEWORK)
    message(FATAL_ERROR "CoreAudio framework not found")
endif()

if(NOT AUDIOTOOLBOX_FRAMEWORK)
    message(FATAL_ERROR "AudioToolbox framework not found")
endif()

if(NOT AUDIOUNIT_FRAMEWORK)
    message(FATAL_ERROR "AudioUnit framework not found")
endif()

# macOS version requirements
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")

# macOS-specific compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# macOS-specific frameworks to link
function(add_macos_libraries target)
    target_link_libraries(${target}
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
        ${AUDIOUNIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${COCOA_FRAMEWORK}
    )
endfunction()

# macOS application bundle configuration
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in
    MACOSX_BUNDLE_BUNDLE_NAME "ETI Stream Analyzer"
    MACOSX_BUNDLE_DISPLAY_NAME "ETI Stream Analyzer"
    MACOSX_BUNDLE_IDENTIFIER "com.eti-analyzer.desktop"
    MACOSX_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_COPYRIGHT "Copyright © 2024 ETI Stream Analyzer Team"
)

# macOS packaging configuration
set(CPACK_GENERATOR "DragNDrop;TGZ")
set(CPACK_DMG_VOLUME_NAME "ETI Stream Analyzer")
set(CPACK_DMG_FORMAT "UDZO")
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/resources/macos_dmg_background.png")

# macOS-specific install configuration
set(CMAKE_INSTALL_BINDIR ".")
set(CMAKE_INSTALL_LIBDIR "Frameworks")
set(CMAKE_INSTALL_INCLUDEDIR "Headers")
