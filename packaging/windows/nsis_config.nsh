; =============================================================================
; StreamDAB Analyser — NSIS installer configuration
; -----------------------------------------------------------------------------
; Included by installer.nsi via "${__FILEDIR__}/..." so every include resolves
; relative to THIS file (the repo), regardless of the directory makensis is
; run from. No absolute paths are used.
;
; PRODUCT_VERSION is NOT hardcoded here: it is the single source of truth from
; the CMake build (DABX_VERSION -> build/CMakeCache.txt). Pass it to makensis:
;   makensis -DPRODUCT_VERSION=1.5.0 packaging\windows\installer.nsi
; =============================================================================

; ---- branding ---------------------------------------------------------------
!ifndef PRODUCT_NAME
!define PRODUCT_NAME "StreamDAB Analyser"
!endif

!ifndef PRODUCT_VERSION
!define PRODUCT_VERSION "0.0.0"
!warning "PRODUCT_VERSION not defined — pass -DPRODUCT_VERSION=<ver> (read from build/CMakeCache.txt DABX_VERSION)"
!endif

!define PRODUCT_PUBLISHER "StreamDAB Analyser Team"
!define PRODUCT_WEB_SITE "https://git.morange.co.th/morange/streamdab-analyser"
!define PRODUCT_SUPPORT_URL "https://git.morange.co.th/morange/streamdab-analyser"
!define PRODUCT_UPDATES_URL "https://git.morange.co.th/morange/streamdab-analyser"

!define MUI_PRODUCT "StreamDAB Analyser"
!define MUI_BRANDINGTEXT "StreamDAB Analyser"

; ---- repo-resolvable paths --------------------------------------------------
; __FILEDIR__ is the directory of THIS file (packaging/windows).
!ifndef SOURCE_DIR
!define SOURCE_DIR "${__FILEDIR__}/../.."
!endif
!ifndef BUILD_DIR
; Default for the standard MSVC multi-config layout; override with
; -DBUILD_DIR=C:\path\to\binaries when packaging outside the repo layout.
!define BUILD_DIR "${SOURCE_DIR}/build/Release"
!endif

; ---- installer icons and graphics -------------------------------------------
; The repo has no custom .ico assets, so the NSIS stock graphics are used.
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"

; ---- welcome / finish pages --------------------------------------------------
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange-uninstall.bmp"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\orange.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "${NSISDIR}\Contrib\Graphics\Header\orange-uninstall.bmp"

; ---- installer behavior ------------------------------------------------------
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

; ---- finish page options ------------------------------------------------------
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\StreamDABAnalyser.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch StreamDAB Analyser"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.md"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Show Readme"

; ---- components page ----------------------------------------------------------
!define MUI_COMPONENTSPAGE_CHECKBITMAP "${NSISDIR}\Contrib\Graphics\Checks\colorful.bmp"

; ---- registry keys for the uninstaller ----------------------------------------
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; ---- file associations ---------------------------------------------------------
!define ETI_FILE_ASSOCIATION_KEY "Software\Classes\.eti"
!define ETI_FILE_TYPE_KEY "Software\Classes\ETIStreamFile"

; ---- start menu registry ---------------------------------------------------------
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"