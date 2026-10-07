# StreamDAB Analyser — Windows NSIS installer (standalone advanced template)
# ---------------------------------------------------------------------------
# Full-featured NSIS 3 script used for hand-built installers; it shares the
# PRODUCT_* branding with CPack's NSIS generator (see cmake/packaging.cmake).
#
# Run from the repo with makensis 3.x, passing the version (single source is
# DABX_VERSION in build/CMakeCache.txt — do NOT hardcode it in nsis_config.nsh):
#   makensis -DPRODUCT_VERSION=1.5.0 packaging\windows\installer.nsi
#
# The includes resolve relative to this file via ${__FILEDIR__}, so the script
# works from any working directory. Built binaries (StreamDABAnalyser.exe +
# the Qt DLLs) must be present in ${BUILD_DIR} (default <repo>\build\Release).
# Component sections live in installer_sections.nsh (included below).

# Modern UI and utilities
!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "WinVer.nsh"

# Configuration (PRODUCT_* branding + repo-resolvable paths)
!include "${__FILEDIR__}\nsis_config.nsh"

# Installer settings
SetCompressor /SOLID lzma
SetCompressorDictSize 64
Unicode True
RequestExecutionLevel admin

# Version information — NSIS requires exactly X.X.X.X (PRODUCT_VERSION=1.5.0
# -> 1.5.0.0; do NOT append two components, makensis rejects 5 parts).
VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "LegalCopyright" "© 2026 ${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Installer"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}.0"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "InternalName" "StreamDABAnalyserInstaller"

# Branding
Name "${MUI_PRODUCT}"
OutFile "StreamDAB_Analyser_${PRODUCT_VERSION}_Setup.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
BrandingText "${MUI_BRANDINGTEXT}"

# UI configuration
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

# Welcome page
!define MUI_WELCOMEPAGE_TITLE "Welcome to ${PRODUCT_NAME} Setup"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of ${PRODUCT_NAME}.$\r$\n$\r$\n${PRODUCT_NAME} is a desktop application for analysing ETI (Ensemble Transport Interface) and DAB/DAB+ streams, with ETSI compliance validation and DAB+ audio decode.$\r$\n$\r$\nClick Next to continue."

# License page
!define MUI_LICENSEPAGE_TEXT_TOP "Please review the license terms before installing ${PRODUCT_NAME}."
!define MUI_LICENSEPAGE_TEXT_BOTTOM "If you accept the terms of the agreement, click I Agree to continue."

# Components page
!define MUI_COMPONENTSPAGE_TEXT_TOP "Select the components you wish to install."
!define MUI_COMPONENTSPAGE_TEXT_COMPLIST "Select components to install:"
!define MUI_COMPONENTSPAGE_TEXT_INSTTYPE "Select installation type:"
!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_TITLE "Component Description"
!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_INFO "Position your mouse over a component to see its description."

# Directory page
!define MUI_DIRECTORYPAGE_TEXT_TOP "Setup will install ${PRODUCT_NAME} in the following folder."
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "Destination Folder"

# Installation page
!define MUI_INSTFILESPAGE_FINISHHEADER_TEXT "Completing the ${PRODUCT_NAME} Setup Wizard"
!define MUI_INSTFILESPAGE_FINISHHEADER_SUBTEXT "${PRODUCT_NAME} has been installed on your computer."
!define MUI_INSTFILESPAGE_ABORTHEADER_TEXT "${PRODUCT_NAME} Setup Wizard was interrupted"
!define MUI_INSTFILESPAGE_ABORTHEADER_SUBTEXT "Setup was not completed successfully."

# Finish page
!define MUI_FINISHPAGE_TITLE "Completing the ${PRODUCT_NAME} Setup Wizard"
!define MUI_FINISHPAGE_TEXT "${PRODUCT_NAME} has been installed on your computer.$\r$\n$\r$\nClick Finish to close this wizard."
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${PRODUCT_NAME}"

# UI pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

# Uninstaller pages
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

# Language
!insertmacro MUI_LANGUAGE "English"

# Component sections (must come after MUI_LANGUAGE: they use ${LANG_ENGLISH})
!include "${__FILEDIR__}\installer_sections.nsh"

# Installer initialization
Function .onInit
    ${IfNot} ${AtLeastWin10}
        MessageBox MB_OK|MB_ICONSTOP "This application requires Windows 10 or later."
        Abort
    ${EndIf}

    ${If} ${RunningX64}
        StrCpy $INSTDIR "$PROGRAMFILES64\${PRODUCT_NAME}"
    ${Else}
        MessageBox MB_OK|MB_ICONSTOP "This application requires 64-bit Windows."
        Abort
    ${EndIf}

    Call CheckVCRedist
FunctionEnd

Function CheckVCRedist
    # Check for Visual C++ Redistributable
    ReadRegStr $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Version"
    ${If} $0 == ""
        MessageBox MB_YESNO|MB_ICONQUESTION "StreamDAB Analyser requires the Visual C++ Redistributable.$\r$\nWould you like to download it now?" IDNO +2
        ExecShell "open" "https://aka.ms/vs/17/release/vc_redist.x64.exe"
    ${EndIf}
FunctionEnd

# Uninstaller section (installs are defined in installer_sections.nsh)
Section "Uninstall"
    # Remove files
    Delete "$INSTDIR\bin\StreamDABAnalyser.exe"
    Delete "$INSTDIR\bin\*.dll"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\uninstall.exe"

    # Remove directories
    RMDir /r "$INSTDIR\docs"
    RMDir /r "$INSTDIR\samples"
    RMDir "$INSTDIR\bin"
    RMDir "$INSTDIR"

    # Remove shortcuts
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Readme.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Read Me.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\User Manual.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Sample ETI Files.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

    # Remove file associations
    DeleteRegKey HKCR ".eti"
    DeleteRegKey HKCR ".etini"
    DeleteRegKey HKCR ".etili"
    DeleteRegKey HKCR "ETIStreamFile"
    DeleteRegKey HKCR "ETINIStreamFile"
    DeleteRegKey HKCR "ETILIStreamFile"
    DeleteRegKey HKLM "SOFTWARE\Classes\Applications\StreamDABAnalyser.exe"
    DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\StreamDABAnalyser.exe"

    # Remove registry entries
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"

    # Notify Windows of changes
    System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
SectionEnd