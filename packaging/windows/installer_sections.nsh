# ETI Stream Analyser - Professional Windows Installer Sections

# =============================================================================
# PROFESSIONAL INSTALLER SECTIONS
# =============================================================================

# Core application section (required)
Section "ETI Stream Analyser" SEC_CORE
    SectionIn RO  ; Read-only, cannot be deselected

    SetOutPath "$INSTDIR\bin"
    File "${BUILD_DIR}\ETIStreamAnalyser.exe"
    File "${BUILD_DIR}\*.dll"  ; Qt and other required DLLs

    SetOutPath "$INSTDIR"
    File "${SOURCE_DIR}\README.md"
    File "${SOURCE_DIR}\LICENSE"

    # Create professional application data directory
    CreateDirectory "$APPDATA\ETI Stream Analyser"
    CreateDirectory "$APPDATA\ETI Stream Analyser\Settings"
    CreateDirectory "$APPDATA\ETI Stream Analyser\Logs"
    CreateDirectory "$APPDATA\ETI Stream Analyser\Reports"

    # Professional registry entries
    WriteRegStr HKLM "Software\ETI Stream Analyser" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\ETI Stream Analyser" "Version" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "Software\ETI Stream Analyser" "Publisher" "${PRODUCT_PUBLISHER}"

    # Professional uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"

    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\bin\ETIStreamAnalyser.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLUpdateInfo" "${PRODUCT_UPDATES_URL}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "HelpLink" "${PRODUCT_SUPPORT_URL}"
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
SectionEnd

# Documentation section
Section "Documentation" SEC_DOCS
    SetOutPath "$INSTDIR\docs"
    File /r "${SOURCE_DIR}\docs\*.*"

    # Professional documentation shortcuts
    CreateDirectory "$SMPROGRAMS\ETI Stream Analyser"
    CreateShortCut "$SMPROGRAMS\ETI Stream Analyser\User Manual.lnk" "$INSTDIR\docs\user_manual.pdf"
    CreateShortCut "$SMPROGRAMS\ETI Stream Analyser\API Documentation.lnk" "$INSTDIR\docs\api\index.html"
SectionEnd

# Sample files section
Section "Sample ETI Files" SEC_SAMPLES
    SetOutPath "$INSTDIR\samples"
    File /r "${SOURCE_DIR}\samples\*.*"

    # Professional samples organization
    CreateDirectory "$INSTDIR\samples\eti_streams"
    CreateDirectory "$INSTDIR\samples\reports"
    CreateDirectory "$INSTDIR\samples\configurations"
SectionEnd

# Development headers section (optional)
Section /o "Development Headers" SEC_DEV
    SetOutPath "$INSTDIR\include"
    File /r "${SOURCE_DIR}\src\*.h"

    SetOutPath "$INSTDIR\lib"
    File "${BUILD_DIR}\*.lib"  ; Static libraries for development

    # Professional development environment setup
    WriteRegStr HKLM "Software\ETI Stream Analyser\SDK" "IncludeDir" "$INSTDIR\include"
    WriteRegStr HKLM "Software\ETI Stream Analyser\SDK" "LibDir" "$INSTDIR\lib"
SectionEnd

# Professional file associations section
Section "File Associations" SEC_ASSOC
    # Register .eti file extension
    WriteRegStr HKCR ".eti" "" "ETIStreamFile"
    WriteRegStr HKCR ".eti" "Content Type" "application/x-eti-stream"
    WriteRegStr HKCR ".eti" "PerceivedType" "audio"

    # Register ETI file type
    WriteRegStr HKCR "ETIStreamFile" "" "ETI Stream File"
    WriteRegStr HKCR "ETIStreamFile\DefaultIcon" "" "$INSTDIR\bin\ETIStreamAnalyser.exe,0"
    WriteRegStr HKCR "ETIStreamFile\shell" "" "open"
    WriteRegStr HKCR "ETIStreamFile\shell\open" "" "Analyze with ETI Stream Analyser"
    WriteRegStr HKCR "ETIStreamFile\shell\open\command" "" '"$INSTDIR\bin\ETIStreamAnalyser.exe" "%1"'

    # Professional context menu integration
    WriteRegStr HKCR "ETIStreamFile\shell\analyze" "" "Quick Analysis"
    WriteRegStr HKCR "ETIStreamFile\shell\analyze\command" "" '"$INSTDIR\bin\ETIStreamAnalyser.exe" "--quick-analyze" "%1"'

    # Register with Windows for broadcast file handling
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\ETIStreamAnalyser.exe\SupportedTypes" ".eti" ""
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\ETIStreamAnalyser.exe\shell\open\command" "" '"$INSTDIR\bin\ETIStreamAnalyser.exe" "%1"'

    # Notify system of file association changes
    System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
SectionEnd

# Desktop integration section
Section "Desktop Integration" SEC_DESKTOP
    # Professional desktop shortcut
    CreateShortCut "$DESKTOP\ETI Stream Analyser.lnk" "$INSTDIR\bin\ETIStreamAnalyser.exe" "" "$INSTDIR\bin\ETIStreamAnalyser.exe" 0

    # Professional Start Menu integration
    CreateDirectory "$SMPROGRAMS\ETI Stream Analyser"
    CreateShortCut "$SMPROGRAMS\ETI Stream Analyser\ETI Stream Analyser.lnk" "$INSTDIR\bin\ETIStreamAnalyser.exe"
    CreateShortCut "$SMPROGRAMS\ETI Stream Analyser\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortCut "$SMPROGRAMS\ETI Stream Analyser\Website.lnk" "${PRODUCT_WEB_SITE}"

    # Professional Quick Launch (if available)
    IfFileExists "$QUICKLAUNCH" 0 +2
    CreateShortCut "$QUICKLAUNCH\ETI Stream Analyser.lnk" "$INSTDIR\bin\ETIStreamAnalyser.exe"
SectionEnd

# =============================================================================
# SECTION DESCRIPTIONS FOR PROFESSIONAL APPEARANCE
# =============================================================================

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_CORE} "Core ETI Stream Analyser application and required runtime libraries. This component is required."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DOCS} "Complete user documentation, API reference, and broadcast engineering guides."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_SAMPLES} "Example ETI stream files and sample configurations for testing and demonstration."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DEV} "Development headers and libraries for custom ETI processing applications."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_ASSOC} "Register ETI Stream Analyser as the default application for .eti files."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "Create desktop shortcuts and Start Menu entries for easy access."
!insertmacro MUI_FUNCTION_DESCRIPTION_END
