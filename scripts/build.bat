@echo off
REM Windows build script for ETI Stream Analyzer
REM Requires Visual Studio Build Tools and Qt6

setlocal enabledelayedexpansion

REM Script configuration
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "BUILD_TYPE=Release"
set "CLEAN_BUILD=false"
set "INSTALL=false"
set "PACKAGE=false"
set "JOBS=%NUMBER_OF_PROCESSORS%"

REM Parse command line arguments
:parse_args
if "%~1"=="" goto end_parse
if "%~1"=="-h" goto show_help
if "%~1"=="--help" goto show_help
if "%~1"=="-d" set "BUILD_TYPE=Debug" & shift & goto parse_args
if "%~1"=="--debug" set "BUILD_TYPE=Debug" & shift & goto parse_args
if "%~1"=="-c" set "CLEAN_BUILD=true" & shift & goto parse_args
if "%~1"=="--clean" set "CLEAN_BUILD=true" & shift & goto parse_args
if "%~1"=="-i" set "INSTALL=true" & shift & goto parse_args
if "%~1"=="--install" set "INSTALL=true" & shift & goto parse_args
if "%~1"=="-p" set "PACKAGE=true" & shift & goto parse_args
if "%~1"=="--package" set "PACKAGE=true" & shift & goto parse_args
if "%~1"=="-j" set "JOBS=%~2" & shift & shift & goto parse_args
if "%~1"=="--jobs" set "JOBS=%~2" & shift & shift & goto parse_args
if "%~1"=="--qt-dir" set "QT_DIR=%~2" & shift & shift & goto parse_args

echo [ERROR] Unknown option: %~1
goto show_help

:end_parse

echo [INFO] ETI Stream Analyzer Windows Build Script
echo [INFO] ==========================================

REM Check for Visual Studio
echo [INFO] Checking for Visual Studio Build Tools...
call :find_vs_tools
if errorlevel 1 (
    echo [ERROR] Visual Studio Build Tools not found
    echo [ERROR] Please install Visual Studio 2019 or later with C++ tools
    exit /b 1
)

REM Check CMake
echo [INFO] Checking for CMake...
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake is not installed or not in PATH
    echo [ERROR] Please install CMake and add it to your PATH
    exit /b 1
)

REM Set up Qt6 if specified
if not "%QT_DIR%"=="" (
    echo [INFO] Using Qt6 from: %QT_DIR%
    set "CMAKE_PREFIX_PATH=%QT_DIR%;%CMAKE_PREFIX_PATH%"
)

REM Clean build if requested
if "%CLEAN_BUILD%"=="true" (
    echo [INFO] Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Configure with CMake
echo [INFO] Configuring build ^(%BUILD_TYPE%^)...
cd /d "%BUILD_DIR%"

cmake -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
    "%PROJECT_ROOT%"

if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    exit /b 1
)

REM Build the project
echo [INFO] Building project with %JOBS% parallel jobs...
cmake --build . --config %BUILD_TYPE% --parallel %JOBS%

if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

echo [SUCCESS] Build complete

REM Install if requested
if "%INSTALL%"=="true" (
    echo [INFO] Installing project...
    cmake --install . --config %BUILD_TYPE%
    if errorlevel 1 (
        echo [ERROR] Installation failed
        exit /b 1
    )
    echo [SUCCESS] Installation complete
)

REM Create package if requested
if "%PACKAGE%"=="true" (
    echo [INFO] Creating package...
    cpack -C %BUILD_TYPE%
    if errorlevel 1 (
        echo [ERROR] Package creation failed
        exit /b 1
    )
    echo [SUCCESS] Package creation complete
)

echo [SUCCESS] Build process completed successfully!
echo [INFO] Build artifacts are in: %BUILD_DIR%
echo [INFO] To run the application: %BUILD_DIR%\%BUILD_TYPE%\ETIStreamAnalyzer.exe

exit /b 0

:show_help
echo ETI Stream Analyzer Windows Build Script
echo.
echo Usage: %~nx0 [OPTIONS]
echo.
echo OPTIONS:
echo     -h, --help          Show this help message
echo     -d, --debug         Build in Debug mode ^(default: Release^)
echo     -c, --clean         Clean build directory before building
echo     -i, --install       Install after building
echo     -p, --package       Create package after building
echo     -j, --jobs N        Number of parallel jobs ^(default: CPU count^)
echo     --qt-dir PATH       Qt installation directory
echo.
echo EXAMPLES:
echo     %~nx0                  # Basic release build
echo     %~nx0 --debug          # Debug build
echo     %~nx0 --clean --install # Clean build and install
echo     %~nx0 --package        # Build and create package
exit /b 0

:find_vs_tools
REM Try to find Visual Studio installation
set "VS_YEARS=2022 2019 2017"
for %%y in (%VS_YEARS%) do (
    if exist "C:\Program Files\Microsoft Visual Studio\%%y\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\%%y\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
        exit /b 0
    )
    if exist "C:\Program Files\Microsoft Visual Studio\%%y\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\%%y\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
        exit /b 0
    )
    if exist "C:\Program Files\Microsoft Visual Studio\%%y\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\%%y\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
        exit /b 0
    )
    if exist "C:\Program Files\Microsoft Visual Studio\%%y\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\%%y\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
        exit /b 0
    )
)

REM Try Build Tools locations
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    exit /b 0
)

exit /b 1
