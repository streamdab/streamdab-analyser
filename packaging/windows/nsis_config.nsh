# ETI Stream Analyzer - Professional Windows Installer Configuration
# NSIS (Nullsoft Scriptable Install System) Configuration

# Professional installer appearance
!define PRODUCT_NAME "ETI Stream Analyzer"
!define PRODUCT_VERSION "${PROJECT_VERSION}"
!define PRODUCT_PUBLISHER "Professional Broadcast Tools"
!define PRODUCT_WEB_SITE "https://eti-analyzer.org"
!define PRODUCT_SUPPORT_URL "https://eti-analyzer.org/support"
!define PRODUCT_UPDATES_URL "https://eti-analyzer.org/downloads"

# Professional branding
!define MUI_PRODUCT "ETI Stream Analyzer Professional"
!define MUI_BRANDINGTEXT "Professional Broadcast Tools"

# Professional installer icons and graphics
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"

# Professional welcome and finish pages
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange-uninstall.bmp"

# Professional header image
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\orange.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "${NSISDIR}\Contrib\Graphics\Header\orange-uninstall.bmp"

# Professional installer behavior
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

# Professional finish page options
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\ETIStreamAnalyzer.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ETI Stream Analyzer"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.txt"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Show User Manual"

# Professional components page
!define MUI_COMPONENTSPAGE_CHECKBITMAP "${NSISDIR}\Contrib\Graphics\Checks\colorful.bmp"

# Registry keys for professional uninstaller
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

# Professional file associations
!define ETI_FILE_ASSOCIATION_KEY "Software\Classes\.eti"
!define ETI_FILE_TYPE_KEY "Software\Classes\ETIStreamFile"

# Professional installation verification
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"
