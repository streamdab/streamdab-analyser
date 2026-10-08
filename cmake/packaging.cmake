# =============================================================================
# StreamDAB Analyser — CPack packaging configuration
# -----------------------------------------------------------------------------
# Single branding source for every installer/package. Included from the
# top-level CMakeLists.txt AFTER the install() rules so CPack sees the full
# install manifest. Version is taken from DABX_VERSION (the single source of
# truth resolved in CMakeLists.txt from the git tag / -DDABX_VERSION /
# PROJECT_VERSION) — never hardcode it here.
#
# Generators per OS:
#   Linux   : TGZ;DEB           (AppImage is NOT a CPack generator — it is built
#                                by packaging/linux/create_appimage.sh with
#                                linuxdeploy + linuxdeploy-plugin-qt)
#   Windows : NSIS;ZIP          (NSIS branded via the CPACK_NSIS_* variables;
#                                the standalone advanced template lives in
#                                packaging/windows/installer.nsi)
#   macOS   : BUNDLE;DragNDrop  (.app via the install tree; .dmg via DragNDrop).
#                                Run scripts/bundle-macos.sh (macdeployqt) AFTER
#                                `cmake --install build --prefix dist` and
#                                BEFORE `cpack -G DragNDrop` so the .dmg carries
#                                the Qt frameworks.
# =============================================================================

include(InstallRequiredSystemLibraries)

# -----------------------------------------------------------------------------
# Branding (single source)
# -----------------------------------------------------------------------------
if(NOT DEFINED DABX_VERSION)
    set(DABX_VERSION "${PROJECT_VERSION}")
endif()

# Cached (FORCE) so scripts and CI can read the resolved version from
# build/CMakeCache.txt (e.g. packaging/linux/create_appimage.sh,
# scripts/bundle-macos.sh) — still driven by DABX_VERSION every configure.
set(CPACK_PACKAGE_NAME "streamdab-analyser"
    CACHE STRING "CPack package name (package-name-safe)" FORCE)
set(CPACK_PACKAGE_VENDOR "StreamDAB Analyser Team")
set(CPACK_PACKAGE_VERSION "${DABX_VERSION}"
    CACHE STRING "CPack package version (single source: DABX_VERSION)" FORCE)
# Version components from the SAME single source (not PROJECT_VERSION) so the
# CPack _VERSION_MAJOR/MINOR/PATCH triplet can never drift from the package
# version (review LOW: CPACK_PACKAGE_VERSION was 1.5.0 but the triplet 1.6.0).
string(REPLACE "." ";" _dabx_version_parts "${DABX_VERSION}")
list(GET _dabx_version_parts 0 _dabx_major)
list(GET _dabx_version_parts 1 _dabx_minor)
list(GET _dabx_version_parts 2 _dabx_patch)
set(CPACK_PACKAGE_VERSION_MAJOR "${_dabx_major}" CACHE STRING "major" FORCE)
set(CPACK_PACKAGE_VERSION_MINOR "${_dabx_minor}" CACHE STRING "minor" FORCE)
set(CPACK_PACKAGE_VERSION_PATCH "${_dabx_patch}" CACHE STRING "patch" FORCE)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "StreamDAB Analyser — ETI/DAB stream analyser")
set(CPACK_PACKAGE_DESCRIPTION
    "StreamDAB Analyser is a Qt6/C++20 desktop application for analysing ETI \
(Ensemble Transport Interface), DAB/DAB+ and EDI streams. It provides real-time \
and offline analysis, ETSI EN 300 799 / EN 300 401 compliance validation, DAB+ \
audio decode, and comprehensive reporting for broadcast engineers.")
set(CPACK_PACKAGE_CONTACT "StreamDAB Analyser Team <maintainers@streamdab-analyser>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/streamdab/streamdab-analyser")

# CPack artifacts always land in the build tree, no matter which directory
# `cpack --config build/CPackConfig.cmake` is invoked from (keeps the repo
# root clean). Encoded into CPackConfig.cmake by include(CPack) below.
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# LICENSE + README as CPack resources (shown by DEB/NSIS/DMG wizards and the
# TGZ install-tree). Also installed as share/doc via install().
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
endif()

# CPACK_PACKAGE_ICON is intentionally NOT set: it needs a PNG/ICO asset and the
# project currently ships only resources/icons/eti-stream-analyser.svg. A
# raster icon (and the NSIS .ico) is a documented follow-up.

# -----------------------------------------------------------------------------
# Linux — TGZ + DEB
# -----------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")

    # Debian/Ubuntu DEB. Ubuntu 24.04 (noble) renamed the Qt6/time_t runtime
    # packages to the *t64 variants; the alternates keep the same metadata
    # installable on older (22.04) and newer releases. The DEB dependencies are
    # guarded by a cache variable so off-Debian systems can override or clear it
    # without touching this file:
    #   cmake -DDABX_DEBIAN_DEPENDS="" ...
    set(DABX_DEBIAN_DEPENDS
        "libqt6core6t64 | libqt6core6, libqt6gui6t64 | libqt6gui6, libqt6widgets6t64 | libqt6widgets6, libqt6network6t64 | libqt6network6, libqt6multimedia6, libqt6test6t64 | libqt6test6, libasound2 | libasound2t64, libfaad2, libfftw3-3"
        CACHE STRING "CPack DEB Depends (Ubuntu 24.04 defaults; override for other distros)")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "StreamDAB Analyser Team <maintainers@streamdab-analyser>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "multimedia")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${DABX_DEBIAN_DEPENDS}")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postinst")
        set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/prerm")
    endif()

    # Convenience target for the AppImage (separate from CPack on purpose).
    add_custom_target(package-linux-appimage
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/create_appimage.sh"
        COMMENT "Build the StreamDAB Analyser AppImage (packaging/linux/create_appimage.sh)"
        VERBATIM)

# -----------------------------------------------------------------------------
# Windows — NSIS + ZIP
# -----------------------------------------------------------------------------
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")

    # The CPack NSIS generator uses its built-in template, branded through the
    # CPACK_NSIS_* variables below. The standalone advanced template
    # (packaging/windows/installer.nsi + installer_sections.nsh +
    # nsis_config.nsh) keeps its own PRODUCT_* branding — keep the two in sync.
    set(CPACK_NSIS_PACKAGE_NAME "StreamDAB Analyser")
    set(CPACK_NSIS_DISPLAY_NAME "StreamDAB Analyser v${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_CONTACT "StreamDAB Analyser Team <maintainers@streamdab-analyser>")
    set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_INSTALL_ROOT "C:\\\\Program Files")
    # Installer icons are skipped: no .ico asset exists in the repo.

# -----------------------------------------------------------------------------
# macOS — .app (BUNDLE) + .dmg (DragNDrop)
# -----------------------------------------------------------------------------
elseif(APPLE)
    set(CPACK_GENERATOR "BUNDLE;DragNDrop")

    # The .app itself is produced by the install() tree (MACOSX_BUNDLE TRUE in
    # CMakeLists.txt); CPack BUNDLE mirrors it and DragNDrop wraps it into the
    # .dmg. Qt frameworks are added by macdeployqt (scripts/bundle-macos.sh)
    # BEFORE cpack -G DragNDrop — a raw cpack -G BUNDLE/DragNDrop .app has no
    # Qt libraries (see docs/MACOS_BUILD.md for the documented order).
    set(CPACK_BUNDLE_NAME "StreamDAB-Analyser")
    set(CPACK_DMG_VOLUME_NAME "StreamDAB Analyser")
    set(CPACK_DMG_FORMAT "UDZO")
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE ON)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/DMGSetup.scpt")
        set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/DMGSetup.scpt")
    endif()
    if(DEFINED ENV{APPLE_CODESIGN_IDENTITY})
        set(CPACK_BUNDLE_APPLE_CERT_APP "$ENV{APPLE_CODESIGN_IDENTITY}")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/entitlements.plist")
        set(CPACK_BUNDLE_APPLE_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/entitlements.plist")
    endif()
endif()

set(_dabx_cpack_generators "${CPACK_GENERATOR}")

include(CPack)

message(STATUS "Packaging: ${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION} "
               "generators=[${_dabx_cpack_generators}]")