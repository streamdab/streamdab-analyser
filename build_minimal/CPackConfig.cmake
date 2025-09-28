# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


set(CPACK_BUILD_SOURCE_DIRS "/home/seksan/workspace/streamdab-analyzer;/home/seksan/workspace/streamdab-analyzer/build_minimal")
set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_COMPONENTS_ALL "Runtime;Documentation;Samples;Development")
set(CPACK_COMPONENTS_ALL_SET_BY_USER "TRUE")
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS "Runtime")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Header files and libraries for ETI processing development and integration")
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Headers")
set(CPACK_COMPONENT_DEVELOPMENT_GROUP "Development")
set(CPACK_COMPONENT_DOCUMENTATION_DEPENDS "Runtime")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "Complete user manual, API documentation, and broadcast engineering guides")
set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "User Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_GROUP "Documentation")
set(CPACK_COMPONENT_GROUP_CORE_DESCRIPTION "Essential application files")
set(CPACK_COMPONENT_GROUP_CORE_DISPLAY_NAME "Core Application")
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION "Development headers and tools")
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DISPLAY_NAME "Development Tools")
set(CPACK_COMPONENT_GROUP_DOCUMENTATION_DESCRIPTION "User and developer documentation")
set(CPACK_COMPONENT_GROUP_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_GROUP_EXAMPLES_DESCRIPTION "Sample files and examples")
set(CPACK_COMPONENT_GROUP_EXAMPLES_DISPLAY_NAME "Examples and Samples")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main application executable and required runtime libraries for ETI stream analysis")
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "StreamDAB Stream Analyzer Application")
set(CPACK_COMPONENT_RUNTIME_GROUP "Core")
set(CPACK_COMPONENT_RUNTIME_REQUIRED "ON")
set(CPACK_COMPONENT_SAMPLES_DEPENDS "Runtime")
set(CPACK_COMPONENT_SAMPLES_DESCRIPTION "Example ETI stream files for testing and demonstration purposes")
set(CPACK_COMPONENT_SAMPLES_DISPLAY_NAME "Sample ETI Files")
set(CPACK_COMPONENT_SAMPLES_GROUP "Examples")
set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "/home/seksan/workspace/streamdab-analyzer/packaging/linux/postinst;/home/seksan/workspace/streamdab-analyzer/packaging/linux/prerm")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qtbase5-dev | qt6-base-dev, libfaad-dev, libfftw3-dev, libyaml-cpp-dev, libc6 (>= 2.17)")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Professional StreamDAB Stream Analyzer
 The StreamDAB Stream Analyzer is a professional desktop application designed for
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
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ETI Analyzer Team <team@eti-analyzer.org>")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SECTION "multimedia")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "alsa-utils, pulseaudio")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_FILE "/usr/share/cmake-3.28/Templates/CPack.GenericDescription.txt")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_SUMMARY "ETIStreamAnalyzer built using CMake")
set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE "ON")
set(CPACK_GENERATOR "DEB;RPM;TGZ")
set(CPACK_INNOSETUP_ARCHITECTURE "x64")
set(CPACK_INSTALL_CMAKE_PROJECTS "/home/seksan/workspace/streamdab-analyzer/build_minimal;ETIStreamAnalyzer;ALL;/")
set(CPACK_INSTALL_PREFIX "/usr/local")
set(CPACK_MODULE_PATH "/usr/lib/x86_64-linux-gnu/cmake/Qt6;/usr/lib/x86_64-linux-gnu/cmake/Qt6/3rdparty/extra-cmake-modules/find-modules;/usr/lib/x86_64-linux-gnu/cmake/Qt6/3rdparty/kwin")
set(CPACK_NSIS_DISPLAY_NAME "StreamDAB Stream Analyzer 1.0.0")
set(CPACK_NSIS_INSTALLER_ICON_CODE "")
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_PACKAGE_NAME "StreamDAB Stream Analyzer 1.0.0")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
set(CPACK_OBJCOPY_EXECUTABLE "/usr/bin/objcopy")
set(CPACK_OBJDUMP_EXECUTABLE "/usr/bin/objdump")
set(CPACK_OUTPUT_CONFIG_FILE "/home/seksan/workspace/streamdab-analyzer/build_minimal/CPackConfig.cmake")
set(CPACK_PACKAGE_CONTACT "ETI Analyzer Professional Support <support@eti-analyzer.org>")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/")
set(CPACK_PACKAGE_DESCRIPTION "Professional desktop application for analyzing ETI (Ensemble Transport Interface) and DAB/DAB+ streams. Provides real-time monitoring, offline analysis, ETSI compliance validation, and comprehensive reporting for broadcast industry professionals.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "/usr/share/cmake-3.28/Templates/CPack.GenericDescription.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional ETI/DAB Stream Analysis Tool for Broadcast Engineers")
set(CPACK_PACKAGE_FILE_NAME "StreamDAB Stream Analyzer-1.0.0-Linux")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/eti-analyzer/desktop")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "StreamDAB Stream Analyzer 1.0.0")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "StreamDAB Stream Analyzer 1.0.0")
set(CPACK_PACKAGE_NAME "StreamDAB Stream Analyzer")
set(CPACK_PACKAGE_RELOCATABLE "true")
set(CPACK_PACKAGE_VENDOR "Professional Broadcast Tools")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_READELF_EXECUTABLE "/usr/bin/readelf")
set(CPACK_RESOURCE_FILE_LICENSE "/home/seksan/workspace/streamdab-analyzer/LICENSE")
set(CPACK_RESOURCE_FILE_README "/home/seksan/workspace/streamdab-analyzer/README.md")
set(CPACK_RESOURCE_FILE_WELCOME "/usr/share/cmake-3.28/Templates/CPack.GenericWelcome.txt")
set(CPACK_RPM_PACKAGE_DESCRIPTION "Professional ETI/DAB Stream Analyzer for broadcast engineers. Provides real-time monitoring, offline analysis, and ETSI compliance validation for ETI and DAB/DAB+ streams.")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase-devel, faad2-devel, fftw-devel, yaml-cpp-devel")
set(CPACK_RPM_PACKAGE_SUGGESTS "alsa-lib, pulseaudio")
set(CPACK_RPM_PACKAGE_URL "https://eti-analyzer.org")
set(CPACK_RPM_PACKAGE_VENDOR "Professional Broadcast Tools")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "/home/seksan/workspace/streamdab-analyzer/packaging/linux/rpm_postinst.sh")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "/home/seksan/workspace/streamdab-analyzer/packaging/linux/rpm_preun.sh")
set(CPACK_SET_DESTDIR "OFF")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES "/\.git/;/\.gitignore;/\.gitmodules;/build/;/Build/;/BUILD/;/\.vs/;/\.vscode/;/\.idea/;\.user$;\.vcxproj\.user$;/CMakeFiles/;/CMakeCache\.txt;/Makefile;/cmake_install\.cmake;/CPackConfig\.cmake;/CPackSourceConfig\.cmake;/install_manifest\.txt;/\.DS_Store;/Thumbs\.db;\.tmp$;\.temp$;\.log$;\.bak$;\.swp$;\.swo$;~$;/packages/;\.deb$;\.rpm$;\.dmg$;\.exe$;\.msi$")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/home/seksan/workspace/streamdab-analyzer/build_minimal/CPackSourceConfig.cmake")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "ETIStreamAnalyzer-1.0.0-source")
set(CPACK_SYSTEM_NAME "Linux")
set(CPACK_THREADS "1")
set(CPACK_TOPLEVEL_TAG "Linux")
set(CPACK_WIX_SIZEOF_VOID_P "8")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "/home/seksan/workspace/streamdab-analyzer/build_minimal/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
