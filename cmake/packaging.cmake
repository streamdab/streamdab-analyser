# =============================================================================
# PROFESSIONAL CROSS-PLATFORM PACKAGING CONFIGURATION
# ETI Stream Analyser Professional Desktop Application
# =============================================================================

# Include system runtime libraries and CPack
include(InstallRequiredSystemLibraries)

# Professional packaging metadata
set(CPACK_PACKAGE_NAME "StreamDAB Stream Analyser")
set(CPACK_PACKAGE_VENDOR "Professional Broadcast Tools")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional ETI/DAB Stream Analysis Tool for Broadcast Engineers")
set(CPACK_PACKAGE_DESCRIPTION "Professional desktop application for analyzing ETI (Ensemble Transport Interface) and DAB/DAB+ streams. Provides real-time monitoring, offline analysis, ETSI compliance validation, and comprehensive reporting for broadcast industry professionals.")

# Professional contact and support information
set(CPACK_PACKAGE_CONTACT "ETI Analyser Professional Support <support@eti-analyser.org>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/eti-analyser/desktop")

# Professional branding and resources
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
endif()

# =============================================================================
# WINDOWS PROFESSIONAL NSIS INSTALLER CONFIGURATION
# =============================================================================
if(WIN32)
    set(CPACK_GENERATOR "NSIS")

    # Professional Windows installer configuration
    set(CPACK_NSIS_PACKAGE_NAME "StreamDAB Stream Analyser Professional")
    set(CPACK_NSIS_DISPLAY_NAME "StreamDAB Stream Analyser v${PROJECT_VERSION}")
    set(CPACK_NSIS_HELP_LINK "https://eti-analyser.org/support")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://eti-analyser.org")
    set(CPACK_NSIS_CONTACT "support@eti-analyser.org")

    # Professional installation options
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_INSTALL_ROOT "C:\\\\Program Files")

    # Desktop and Start Menu shortcuts
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "
        CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\StreamDAB Stream Analyser.lnk' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe'
        CreateShortCut '$DESKTOP\\\\StreamDAB Stream Analyser.lnk' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe'
    ")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA "
        Delete '$SMPROGRAMS\\\\$START_MENU\\\\StreamDAB Stream Analyser.lnk'
        Delete '$DESKTOP\\\\StreamDAB Stream Analyser.lnk'
    ")

    # Professional file associations for .eti files
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        ; Register ETI file association
        WriteRegStr HKCR '.eti' '' 'ETIStreamFile'
        WriteRegStr HKCR '.eti' 'Content Type' 'application/x-eti-stream'
        WriteRegStr HKCR 'ETIStreamFile' '' 'ETI Stream File'
        WriteRegStr HKCR 'ETIStreamFile\\\\DefaultIcon' '' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe,0'
        WriteRegStr HKCR 'ETIStreamFile\\\\shell\\\\open\\\\command' '' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe \\\"%1\\\"'
        WriteRegStr HKCR 'ETIStreamFile\\\\shell\\\\analyze\\\\command' '' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe --analyze \\\"%1\\\"'

        ; Register application for broadcast file types
        WriteRegStr HKLM 'SOFTWARE\\\\Classes\\\\Applications\\\\${PROJECT_NAME}.exe\\\\SupportedTypes' '.eti' ''
        WriteRegStr HKLM 'SOFTWARE\\\\Classes\\\\Applications\\\\${PROJECT_NAME}.exe\\\\shell\\\\open\\\\command' '' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe \\\"%1\\\"'

        ; Notify system of changes
        System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'

        ; Add to Windows Programs list
        WriteRegStr HKLM 'SOFTWARE\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\App Paths\\\\${PROJECT_NAME}.exe' '' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe'
        WriteRegStr HKLM 'SOFTWARE\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\App Paths\\\\${PROJECT_NAME}.exe' 'Path' '$INSTDIR\\\\bin'
    ")

    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        ; Remove ETI file associations
        DeleteRegKey HKCR '.eti'
        DeleteRegKey HKCR 'ETIStreamFile'
        DeleteRegKey HKLM 'SOFTWARE\\\\Classes\\\\Applications\\\\${PROJECT_NAME}.exe'
        DeleteRegKey HKLM 'SOFTWARE\\\\Microsoft\\\\Windows\\\\CurrentVersion\\\\App Paths\\\\${PROJECT_NAME}.exe'

        ; Notify system of changes
        System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
    ")

    # Professional branding customization
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/installer.ico")
        set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/installer.ico")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/uninstaller.ico")
        set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/uninstaller.ico")
    endif()

    # Component installation for Windows
    set(CPACK_NSIS_COMPONENT_INSTALL ON)

    # =============================================================================
    # MACOS PROFESSIONAL DMG CONFIGURATION
    # =============================================================================
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")

    # Professional macOS DMG configuration
    set(CPACK_DMG_VOLUME_NAME "StreamDAB Stream Analyser v${PROJECT_VERSION}")
    set(CPACK_DMG_FORMAT "UDZO")  # Compressed DMG for smaller distribution
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE ON)

    # Professional DMG styling and layout
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/dmg/background.png")
        set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/resources/dmg/background.png")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/DMGSetup.scpt")
        set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/DMGSetup.scpt")
    endif()

    # Bundle configuration for professional appearance
    set(CPACK_BUNDLE_NAME "StreamDAB Stream Analyser")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/eti-analyser.icns")
        set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/eti-analyser.icns")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in")
        set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in")
    endif()

    # Code signing preparation (requires Apple Developer certificate)
    if(DEFINED ENV{APPLE_CODESIGN_IDENTITY})
        set(CPACK_BUNDLE_APPLE_CERT_APP "$ENV{APPLE_CODESIGN_IDENTITY}")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/entitlements.plist")
        set(CPACK_BUNDLE_APPLE_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/entitlements.plist")
    endif()

    # =============================================================================
    # LINUX PROFESSIONAL PACKAGE CONFIGURATION
    # =============================================================================
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")

    # Debian/Ubuntu DEB package configuration
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ETI Analyser Team <team@eti-analyser.org>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "multimedia")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "qtbase5-dev | qt6-base-dev, libfaad-dev, libfftw3-dev, libyaml-cpp-dev, libc6 (>= 2.17)")
    set(CPACK_DEBIAN_PACKAGE_SUGGESTS "alsa-utils, pulseaudio")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Professional StreamDAB Stream Analyser
 The StreamDAB Stream Analyser is a professional desktop application designed for
 broadcast engineers to analyze ETI (Ensemble Transport Interface) and DAB/DAB+
 streams. It provides real-time monitoring, offline analysis, ETSI compliance
 validation, and comprehensive reporting capabilities.
 .
 Features include:
  * Real-time ETI stream capture and analysis
  * Professional Qt-based GUI interface
  * ETSI EN 300 799 compliance validation
  * Multi-stream support with audio extraction
  * Report generation (PDF/HTML) for broadcast documentation
  * Stream recording with time-based segmentation")

    # Desktop integration scripts
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/postinst")
        set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/prerm")
    endif()

    # RPM package configuration for Fedora/RHEL/SUSE
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
    set(CPACK_RPM_PACKAGE_VENDOR "Professional Broadcast Tools")
    set(CPACK_RPM_PACKAGE_URL "https://eti-analyser.org")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase-devel, faad2-devel, fftw-devel, yaml-cpp-devel")
    set(CPACK_RPM_PACKAGE_SUGGESTS "alsa-lib, pulseaudio")
    set(CPACK_RPM_PACKAGE_DESCRIPTION "Professional ETI/DAB Stream Analyser for broadcast engineers. Provides real-time monitoring, offline analysis, and ETSI compliance validation for ETI and DAB/DAB+ streams.")

    # RPM scripts for desktop integration
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm_postinst.sh")
        set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm_postinst.sh")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm_preun.sh")
        set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm_preun.sh")
    endif()
endif()

# =============================================================================
# PROFESSIONAL COMPONENT CONFIGURATION
# =============================================================================

# Define components for selective installation
set(CPACK_COMPONENTS_ALL Runtime Documentation Samples Development)

# Runtime component (main application) - REQUIRED
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "StreamDAB Stream Analyser Application")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main application executable and required runtime libraries for ETI stream analysis")
set(CPACK_COMPONENT_RUNTIME_REQUIRED ON)
set(CPACK_COMPONENT_RUNTIME_GROUP "Core")

# Documentation component
set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "User Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "Complete user manual, API documentation, and broadcast engineering guides")
set(CPACK_COMPONENT_DOCUMENTATION_DEPENDS Runtime)
set(CPACK_COMPONENT_DOCUMENTATION_GROUP "Documentation")

# Sample files component
set(CPACK_COMPONENT_SAMPLES_DISPLAY_NAME "Sample ETI Files")
set(CPACK_COMPONENT_SAMPLES_DESCRIPTION "Example ETI stream files for testing and demonstration purposes")
set(CPACK_COMPONENT_SAMPLES_DEPENDS Runtime)
set(CPACK_COMPONENT_SAMPLES_GROUP "Examples")

# Development component (optional)
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Headers")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Header files and libraries for ETI processing development and integration")
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS Runtime)
set(CPACK_COMPONENT_DEVELOPMENT_GROUP "Development")

# Component groups
set(CPACK_COMPONENT_GROUP_CORE_DISPLAY_NAME "Core Application")
set(CPACK_COMPONENT_GROUP_CORE_DESCRIPTION "Essential application files")
set(CPACK_COMPONENT_GROUP_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_GROUP_DOCUMENTATION_DESCRIPTION "User and developer documentation")
set(CPACK_COMPONENT_GROUP_EXAMPLES_DISPLAY_NAME "Examples and Samples")
set(CPACK_COMPONENT_GROUP_EXAMPLES_DESCRIPTION "Sample files and examples")
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DISPLAY_NAME "Development Tools")
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION "Development headers and tools")

# =============================================================================
# PROFESSIONAL SOURCE PACKAGE CONFIGURATION
# =============================================================================

# Source package for open source distribution
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-source")
set(CPACK_SOURCE_IGNORE_FILES
    # Version control
    "/\\.git/"
    "/\\.gitignore"
    "/\\.gitmodules"

    # Build directories
    "/build/"
    "/Build/"
    "/BUILD/"

    # IDE files
    "/\\.vs/"
    "/\\.vscode/"
    "/\\.idea/"
    "\\.user$"
    "\\.vcxproj\\.user$"

    # CMake generated files
    "/CMakeFiles/"
    "/CMakeCache\\.txt"
    "/Makefile"
    "/cmake_install\\.cmake"
    "/CPackConfig\\.cmake"
    "/CPackSourceConfig\\.cmake"
    "/install_manifest\\.txt"

    # System files
    "/\\.DS_Store"
    "/Thumbs\\.db"

    # Temporary files
    "\\.tmp$"
    "\\.temp$"
    "\\.log$"
    "\\.bak$"
    "\\.swp$"
    "\\.swo$"
    "~$"

    # Package output
    "/packages/"
    "\\.deb$"
    "\\.rpm$"
    "\\.dmg$"
    "\\.exe$"
    "\\.msi$"
)

# =============================================================================
# CUSTOM PACKAGING TARGETS FOR PROFESSIONAL DISTRIBUTION
# =============================================================================

# Individual platform packaging targets
add_custom_target(package-windows
    COMMAND ${CMAKE_CPACK_COMMAND} -G NSIS
    COMMENT "Creating Windows NSIS installer package"
    VERBATIM
)

add_custom_target(package-linux-deb
    COMMAND ${CMAKE_CPACK_COMMAND} -G DEB
    COMMENT "Creating Linux DEB package"
    VERBATIM
)

add_custom_target(package-linux-rpm
    COMMAND ${CMAKE_CPACK_COMMAND} -G RPM
    COMMENT "Creating Linux RPM package"
    VERBATIM
)

add_custom_target(package-linux-appimage
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/create_appimage.sh
    COMMENT "Creating Linux AppImage package"
    VERBATIM
)

add_custom_target(package-macos
    COMMAND ${CMAKE_CPACK_COMMAND} -G DragNDrop
    COMMENT "Creating macOS DMG package"
    VERBATIM
)

add_custom_target(package-source
    COMMAND ${CMAKE_CPACK_COMMAND} --config CPackSourceConfig.cmake
    COMMENT "Creating source distribution package"
    VERBATIM
)

# Comprehensive packaging target
add_custom_target(package-all
    COMMENT "Creating all applicable distribution packages"
)

# Platform-specific package-all dependencies
if(WIN32)
    add_dependencies(package-all package-windows package-source)
elseif(APPLE)
    add_dependencies(package-all package-macos package-source)
elseif(UNIX)
    add_dependencies(package-all package-linux-deb package-linux-rpm package-source)
endif()

# =============================================================================
# VALIDATION AND TESTING TARGETS
# =============================================================================

# Package validation target
add_custom_target(validate-packages
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/packaging/scripts/validate_packages.sh
    COMMENT "Validating created packages"
    VERBATIM
)

# Professional deployment preparation
add_custom_target(prepare-deployment
    DEPENDS package-all validate-packages
    COMMENT "Preparing packages for professional deployment"
)

# Include CPack after all configuration
include(CPack)

message(STATUS "=== Professional Packaging Configuration Complete ===")
message(STATUS "Supported generators: ${CPACK_GENERATOR}")
message(STATUS "Package name: ${CPACK_PACKAGE_NAME}")
message(STATUS "Package version: ${CPACK_PACKAGE_VERSION}")
message(STATUS "Target platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "======================================================")
