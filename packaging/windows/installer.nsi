# ETI Stream Analyzer Professional Windows Installer
# NSIS Script for Professional Broadcast Tools Distribution

# Modern UI and Professional Setup
!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "WinVer.nsh"

# Include configuration
!include "nsis_config.nsh"
!include "installer_sections.nsh"

# Professional installer settings
SetCompressor /SOLID lzma
SetCompressorDictSize 64
Unicode True
RequestExecutionLevel admin

# Professional version information
VIProductVersion "${PRODUCT_VERSION}.0.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "LegalCopyright" "© 2024 Professional Broadcast Tools"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Professional Installer"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}.0.0"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "InternalName" "ETIStreamAnalyzerInstaller"

# Professional branding
Name "${MUI_PRODUCT}"
OutFile "ETI_Stream_Analyzer_Professional_${PRODUCT_VERSION}_Setup.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
BrandingText "${MUI_BRANDINGTEXT}"

# Professional UI Configuration
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

# Professional welcome page
!define MUI_WELCOMEPAGE_TITLE "Welcome to ${PRODUCT_NAME} Professional Setup"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of ${PRODUCT_NAME}.$\r$\n$\r$\n${PRODUCT_NAME} is a professional desktop application for analyzing ETI (Ensemble Transport Interface) and DAB/DAB+ streams, designed for broadcast engineering professionals.$\r$\n$\r$\nClick Next to continue."

# Professional license page
!define MUI_LICENSEPAGE_TEXT_TOP "Please review the license terms before installing ${PRODUCT_NAME} Professional."
!define MUI_LICENSEPAGE_TEXT_BOTTOM "If you accept the terms of the agreement, click I Agree to continue. You must accept the agreement to install ${PRODUCT_NAME} Professional."

# Professional components page
!define MUI_COMPONENTSPAGE_TEXT_TOP "Select the components you wish to install."
!define MUI_COMPONENTSPAGE_TEXT_COMPLIST "Select components to install:"
!define MUI_COMPONENTSPAGE_TEXT_INSTTYPE "Select installation type:"
!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_TITLE "Component Description"
!define MUI_COMPONENTSPAGE_TEXT_DESCRIPTION_INFO "Position your mouse over a component to see its description."

# Professional directory page
!define MUI_DIRECTORYPAGE_TEXT_TOP "Setup will install ${PRODUCT_NAME} Professional in the following folder."
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION "Destination Folder"

# Professional installation page
!define MUI_INSTFILESPAGE_FINISHHEADER_TEXT "Completing the ${PRODUCT_NAME} Professional Setup Wizard"
!define MUI_INSTFILESPAGE_FINISHHEADER_SUBTEXT "${PRODUCT_NAME} Professional has been installed on your computer."
!define MUI_INSTFILESPAGE_ABORTHEADER_TEXT "${PRODUCT_NAME} Professional Setup Wizard was interrupted"
!define MUI_INSTFILESPAGE_ABORTHEADER_SUBTEXT "Setup was not completed successfully."

# Professional finish page
!define MUI_FINISHPAGE_TITLE "Completing the ${PRODUCT_NAME} Professional Setup Wizard"
!define MUI_FINISHPAGE_TEXT "${PRODUCT_NAME} Professional has been installed on your computer.$\r$\n$\r$\nClick Finish to close this wizard."
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${PRODUCT_NAME} Professional"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Show User Manual"

# Professional UI pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

# Professional uninstaller pages
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

# Professional languages
!insertmacro MUI_LANGUAGE "English"

# Professional installer initialization
Function .onInit
    # Professional system requirements check
    ${IfNot} ${AtLeastWin10}
        MessageBox MB_OK|MB_ICONSTOP "This application requires Windows 10 or later for professional broadcast engineering features."
        Abort
    ${EndIf}

    # Professional architecture check
    ${If} ${RunningX64}
        StrCpy $INSTDIR "$PROGRAMFILES64\${PRODUCT_NAME}"
    ${Else}
        MessageBox MB_OK|MB_ICONSTOP "This professional application requires 64-bit Windows for optimal performance."
        Abort
    ${EndIf}

    # Professional prerequisites check
    Call CheckDotNetFramework
    Call CheckVCRedist
FunctionEnd

# Professional prerequisites verification
Function CheckDotNetFramework
    # Check for required .NET Framework version
    ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full" "Release"
    ${If} $0 < 461808  # .NET Framework 4.7.2
        MessageBox MB_YESNO|MB_ICONQUESTION "Professional features require .NET Framework 4.7.2 or later.$\r$\nWould you like to download it now?" IDNO +2
        ExecShell "open" "https://dotnet.microsoft.com/download/dotnet-framework"
    ${EndIf}
FunctionEnd

Function CheckVCRedist
    # Check for Visual C++ Redistributable
    ReadRegStr $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Version"
    ${If} $0 == ""
        MessageBox MB_YESNO|MB_ICONQUESTION "Professional features require Visual C++ Redistributable.$\r$\nWould you like to download it now?" IDNO +2
        ExecShell "open" "https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist"
    ${EndIf}
FunctionEnd

# Professional installation sections
Section "Core Application" SecCore
    SectionIn RO  # Required section

    SetOutPath "$INSTDIR\bin"
    File "ETIStreamAnalyzer.exe"
    File "*.dll"

    SetOutPath "$INSTDIR"
    File "README.md"
    File "LICENSE"

    # Professional shortcuts
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\bin\ETIStreamAnalyzer.exe"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\User Manual.lnk" "$INSTDIR\README.md"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"

    # Professional desktop shortcut
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\bin\ETIStreamAnalyzer.exe"

    # Professional file associations
    Call RegisterFileAssociations

    # Professional registry entries
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME} Professional"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLUpdateInfo" "${PRODUCT_UPDATES_URL}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "HelpLink" "${PRODUCT_SUPPORT_URL}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "QuietUninstallString" "$INSTDIR\uninstall.exe /S"
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1

    # Professional uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Documentation" SecDocs
    SetOutPath "$INSTDIR\docs"
    File /r "docs\*.*"

    # Documentation shortcuts
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Professional User Guide.lnk" "$INSTDIR\docs\user-guide.pdf"
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\API Documentation.lnk" "$INSTDIR\docs\api-reference.html"
SectionEnd

Section "Sample Files" SecSamples
    SetOutPath "$INSTDIR\samples"
    File /r "samples\*.*"

    # Sample files shortcuts
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Sample ETI Files.lnk" "$INSTDIR\samples\"
SectionEnd

Section "Development Tools" SecDev
    SetOutPath "$INSTDIR\dev"
    File /r "include\*.*"
    File /r "lib\*.*"

    # Development shortcuts
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Development Headers.lnk" "$INSTDIR\dev\include\"
SectionEnd

# Professional file associations
Function RegisterFileAssociations
    # ETI files
    WriteRegStr HKCR ".eti" "" "ETIStreamFile"
    WriteRegStr HKCR ".eti" "Content Type" "application/x-eti-stream"
    WriteRegStr HKCR "ETIStreamFile" "" "ETI Stream File"
    WriteRegStr HKCR "ETIStreamFile\DefaultIcon" "" "$INSTDIR\bin\ETIStreamAnalyzer.exe,0"
    WriteRegStr HKCR "ETIStreamFile\shell\open\command" "" '"$INSTDIR\bin\ETIStreamAnalyzer.exe" "%1"'
    WriteRegStr HKCR "ETIStreamFile\shell\analyze\command" "" '"$INSTDIR\bin\ETIStreamAnalyzer.exe" --analyze "%1"'

    # ETI-NI files
    WriteRegStr HKCR ".etini" "" "ETINIStreamFile"
    WriteRegStr HKCR "ETINIStreamFile" "" "ETI-NI Stream File"
    WriteRegStr HKCR "ETINIStreamFile\DefaultIcon" "" "$INSTDIR\bin\ETIStreamAnalyzer.exe,0"
    WriteRegStr HKCR "ETINIStreamFile\shell\open\command" "" '"$INSTDIR\bin\ETIStreamAnalyzer.exe" "%1"'

    # ETI-LI files
    WriteRegStr HKCR ".etili" "" "ETILIStreamFile"
    WriteRegStr HKCR "ETILIStreamFile" "" "ETI-LI Stream File"
    WriteRegStr HKCR "ETILIStreamFile\DefaultIcon" "" "$INSTDIR\bin\ETIStreamAnalyzer.exe,0"
    WriteRegStr HKCR "ETILIStreamFile\shell\open\command" "" '"$INSTDIR\bin\ETIStreamAnalyzer.exe" "%1"'

    # Application registration
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\ETIStreamAnalyzer.exe\SupportedTypes" ".eti" ""
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\ETIStreamAnalyzer.exe\SupportedTypes" ".etini" ""
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\ETIStreamAnalyzer.exe\SupportedTypes" ".etili" ""
    WriteRegStr HKLM "SOFTWARE\Classes\Applications\ETIStreamAnalyzer.exe\shell\open\command" "" '"$INSTDIR\bin\ETIStreamAnalyzer.exe" "%1"'

    # Add to Windows Programs list
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ETIStreamAnalyzer.exe" "" "$INSTDIR\bin\ETIStreamAnalyzer.exe"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ETIStreamAnalyzer.exe" "Path" "$INSTDIR\bin"

    # Notify Windows of file association changes
    System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
FunctionEnd

# Professional uninstaller
Section "Uninstall"
    # Remove files
    Delete "$INSTDIR\bin\ETIStreamAnalyzer.exe"
    Delete "$INSTDIR\bin\*.dll"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\uninstall.exe"

    # Remove directories
    RMDir /r "$INSTDIR\docs"
    RMDir /r "$INSTDIR\samples"
    RMDir /r "$INSTDIR\dev"
    RMDir "$INSTDIR\bin"
    RMDir "$INSTDIR"

    # Remove shortcuts
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\User Manual.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Professional User Guide.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\API Documentation.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Sample ETI Files.lnk"
    Delete "$SMPROGRAMS\${PRODUCT_NAME}\Development Headers.lnk"
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
    DeleteRegKey HKLM "SOFTWARE\Classes\Applications\ETIStreamAnalyzer.exe"
    DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ETIStreamAnalyzer.exe"

    # Remove registry entries
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"

    # Notify Windows of changes
    System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
SectionEnd

# Professional section descriptions
LangString DESC_SecCore ${LANG_ENGLISH} "Core application files and runtime libraries. This component is required."
LangString DESC_SecDocs ${LANG_ENGLISH} "Complete user manual, API documentation, and professional broadcast engineering guides."
LangString DESC_SecSamples ${LANG_ENGLISH} "Example ETI stream files for testing and demonstration purposes."
LangString DESC_SecDev ${LANG_ENGLISH} "Development headers and libraries for ETI processing integration."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs} $(DESC_SecDocs)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSamples} $(DESC_SecSamples)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDev} $(DESC_SecDev)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
