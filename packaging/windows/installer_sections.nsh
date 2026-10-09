# StreamDAB Analyser — NSIS installer component sections
# ---------------------------------------------------------------------------
# Included by installer.nsi via "${__FILEDIR__}\installer_sections.nsh".
# Uses ${SOURCE_DIR} / ${BUILD_DIR} defined in nsis_config.nsh (repo-resolved).

# Core application section (required)
Section "StreamDAB Analyser" SEC_CORE
    SectionIn RO  ; Read-only, cannot be deselected

    SetOutPath "$INSTDIR\bin"
    File "${BUILD_DIR}\StreamDABAnalyser.exe"
    File "${BUILD_DIR}\*.dll"  ; Qt and other required DLLs

    SetOutPath "$INSTDIR"
    File "${SOURCE_DIR}\README.md"
    File "${SOURCE_DIR}\LICENSE"

    ; Application data directory
    CreateDirectory "$APPDATA\StreamDAB Analyser"
    CreateDirectory "$APPDATA\StreamDAB Analyser\Settings"
    CreateDirectory "$APPDATA\StreamDAB Analyser\Logs"
    CreateDirectory "$APPDATA\StreamDAB Analyser\Reports"

    ; Registry entries
    WriteRegStr HKLM "Software\StreamDAB Analyser" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\StreamDAB Analyser" "Version" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "Software\StreamDAB Analyser" "Publisher" "${PRODUCT_PUBLISHER}"

    ; Uninstaller + uninstall registry
    WriteUninstaller "$INSTDIR\uninstall.exe"

    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\bin\StreamDABAnalyser.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLUpdateInfo" "${PRODUCT_UPDATES_URL}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "HelpLink" "${PRODUCT_SUPPORT_URL}"
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1

    ; Shortcuts
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\bin\StreamDABAnalyser.exe"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Readme.lnk" "$INSTDIR\README.md"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\bin\StreamDABAnalyser.exe"

    ; File associations
    Call RegisterFileAssociations
SectionEnd

# Documentation section
Section "Documentation" SEC_DOCS
    SetOutPath "$INSTDIR\docs"
    !if /FileExists "${SOURCE_DIR}\docs"
    File /r "${SOURCE_DIR}\docs\*.*"
    !endif

    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    !if /FileExists "$INSTDIR\docs\user_manual.pdf"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\User Manual.lnk" "$INSTDIR\docs\user_manual.pdf"
    !else
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Read Me.lnk" "$INSTDIR\README.md"
    !endif
SectionEnd

# Sample files section (guarded: the repo has no samples/ tree by default)
Section "Sample ETI Files" SEC_SAMPLES
    !if /FileExists "${SOURCE_DIR}\samples"
    SetOutPath "$INSTDIR\samples"
    File /r "${SOURCE_DIR}\samples\*.*"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Sample ETI Files.lnk" "$INSTDIR\samples\"
    !endif
SectionEnd

# File associations
Function RegisterFileAssociations
    ; ETI files
    WriteRegStr HKCR ".eti" "" "ETIStreamFile"
    WriteRegStr HKCR ".eti" "Content Type" "application/x-eti-stream"
    WriteRegStr HKCR "ETIStreamFile" "" "ETI Stream File"
    WriteRegStr HKCR "ETIStreamFile\DefaultIcon" "" "$INSTDIR\bin\StreamDABAnalyser.exe,0"
    WriteRegStr HKCR "ETIStreamFile\shell" "" "open"
    WriteRegStr HKCR "ETIStreamFile\shell\open" "" "Analyze with StreamDAB Analyser"
    WriteRegStr HKCR "ETIStreamFile\shell\open\command" "" '"$INSTDIR\bin\StreamDABAnalyser.exe" "%1"'
    WriteRegStr HKCR "ETIStreamFile\shell\analyze" "" "Quick Analysis"
    WriteRegStr HKCR "ETIStreamFile\shell\analyze\command" "" '"$INSTDIR\bin\StreamDABAnalyser.exe" "--quick-analyze" "%1"'

    ; ETI-NI / ETI-LI extensions
    WriteRegStr HKCR ".etini" "" "ETINIStreamFile"
    WriteRegStr HKCR "ETINIStreamFile" "" "ETI-NI Stream File"
    WriteRegStr HKCR "ETINIStreamFile\DefaultIcon" "" "$INSTDIR\bin\StreamDABAnalyser.exe,0"
    WriteRegStr HKCR "ETINIStreamFile\shell\open\command" "" '"$INSTDIR\bin\StreamDABAnalyser.exe" "%1"'
    WriteRegStr HKCR ".etili" "" "ETILIStreamFile"
    WriteRegStr HKCR "ETILIStreamFile" "" "ETI-LI Stream File"
    WriteRegStr HKCR "ETILIStreamFile\DefaultIcon" "" "$INSTDIR\bin\StreamDABAnalyser.exe,0"
    WriteRegStr HKCR "ETILIStreamFile\shell\open\command" "" '"$INSTDIR\bin\StreamDABAnalyser.exe" "%1"'

    ; Application registration
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\StreamDABAnalyser.exe\SupportedTypes" ".eti" ""
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\StreamDABAnalyser.exe\SupportedTypes" ".etini" ""
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\StreamDABAnalyser.exe\SupportedTypes" ".etili" ""
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\StreamDABAnalyser.exe\shell\open\command" "" '"$INSTDIR\bin\StreamDABAnalyser.exe" "%1"'

    ; Add to Windows Programs list
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\StreamDABAnalyser.exe" "" "$INSTDIR\bin\StreamDABAnalyser.exe"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\StreamDABAnalyser.exe" "Path" "$INSTDIR\bin"

    ; Notify the system of file association changes
    System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
FunctionEnd

# Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_CORE} "Core StreamDAB Analyser application and required runtime libraries. This component is required."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DOCS} "User documentation and engineering guides."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_SAMPLES} "Example ETI stream files for testing and demonstration."
!insertmacro MUI_FUNCTION_DESCRIPTION_END