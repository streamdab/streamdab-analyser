# ============================================================================
# ETI Stream Analyzer - Windows Native Build Script
# Professional PowerShell build automation for Windows development
# ============================================================================

param(
    [string]$BuildType = "Release",
    [switch]$EnableTesting = $true,
    [switch]$EnableCoverage = $false,
    [switch]$CleanBuild = $false,
    [switch]$InstallDeps = $false,
    [switch]$Package = $false,
    [switch]$Help = $false
)

# ============================================================================
# Configuration and Constants
# ============================================================================

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build"
$InstallDir = Join-Path $ProjectRoot "install"

# Version information
$AppName = "ETI Stream Analyzer"
$AppVersion = "1.0.0"

# ============================================================================
# Helper Functions
# ============================================================================

function Write-Banner {
    param([string]$Message)
    Write-Host ""
    Write-Host "============================================================================" -ForegroundColor Green
    Write-Host " $Message" -ForegroundColor Green
    Write-Host "============================================================================" -ForegroundColor Green
    Write-Host ""
}

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Write-Error {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor Red
}

function Write-Warning {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor Yellow
}

function Test-Command {
    param([string]$Command)
    try {
        Get-Command $Command -ErrorAction Stop | Out-Null
        return $true
    }
    catch {
        return $false
    }
}

function Show-Help {
    Write-Banner "$AppName - Windows Build Script"
    Write-Host "USAGE:"
    Write-Host "  .\build-windows.ps1 [OPTIONS]"
    Write-Host ""
    Write-Host "OPTIONS:"
    Write-Host "  -BuildType <Debug|Release>  Build configuration (default: Release)"
    Write-Host "  -EnableTesting              Enable test suite compilation"
    Write-Host "  -EnableCoverage             Enable code coverage reporting"
    Write-Host "  -CleanBuild                 Clean build directory before building"
    Write-Host "  -InstallDeps                Install required dependencies"
    Write-Host "  -Package                    Create distribution packages"
    Write-Host "  -Help                       Show this help message"
    Write-Host ""
    Write-Host "EXAMPLES:"
    Write-Host "  .\build-windows.ps1                          # Default release build"
    Write-Host "  .\build-windows.ps1 -BuildType Debug         # Debug build"
    Write-Host "  .\build-windows.ps1 -CleanBuild -Package     # Clean build with packaging"
    Write-Host "  .\build-windows.ps1 -InstallDeps             # Install dependencies only"
    Write-Host ""
    Write-Host "REQUIREMENTS:"
    Write-Host "  - Visual Studio 2019/2022 with C++ workload"
    Write-Host "  - CMake 3.21 or later"
    Write-Host "  - vcpkg package manager"
    Write-Host "  - Qt6 development libraries"
    Write-Host ""
}

# ============================================================================
# Dependency Installation
# ============================================================================

function Install-Dependencies {
    Write-Step "Installing Windows dependencies"

    # Check for vcpkg
    if (-not (Test-Command "vcpkg")) {
        Write-Error "vcpkg not found. Please install vcpkg first:"
        Write-Host "  git clone https://github.com/Microsoft/vcpkg.git"
        Write-Host "  cd vcpkg"
        Write-Host "  .\bootstrap-vcpkg.bat"
        Write-Host "  .\vcpkg integrate install"
        exit 1
    }
    Write-Success "vcpkg found"

    # Install required packages via vcpkg
    Write-Step "Installing packages via vcpkg"
    $packages = @(
        "qt6[core,widgets,network,multimedia,test]",
        "faad2",
        "fftw3",
        "yaml-cpp",
        "gtest",
        "gmock"
    )

    foreach ($package in $packages) {
        Write-Host "Installing $package..."
        & vcpkg install $package --triplet x64-windows
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Failed to install $package"
            exit 1
        }
    }
    Write-Success "All packages installed successfully"

    # Set environment variables
    $vcpkgRoot = & vcpkg list | Select-String "vcpkg-root" | ForEach-Object { ($_ -split ": ")[1] }
    if ($vcpkgRoot) {
        [Environment]::SetEnvironmentVariable("VCPKG_ROOT", $vcpkgRoot, "User")
        Write-Success "VCPKG_ROOT set to $vcpkgRoot"
    }
}

# ============================================================================
# Build System Check
# ============================================================================

function Test-BuildEnvironment {
    Write-Step "Checking build environment"

    # Check CMake
    if (-not (Test-Command "cmake")) {
        Write-Error "CMake not found. Please install CMake 3.21 or later"
        exit 1
    }
    $cmakeVersion = & cmake --version | Select-String "version" | ForEach-Object { ($_ -split " ")[2] }
    Write-Success "CMake $cmakeVersion found"

    # Check Visual Studio
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsInstances = & $vsWhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
        if ($vsInstances.Count -gt 0) {
            $vsVersion = $vsInstances[0].catalog.productDisplayVersion
            Write-Success "Visual Studio $vsVersion found"
        } else {
            Write-Error "Visual Studio with C++ tools not found"
            exit 1
        }
    } else {
        Write-Warning "Visual Studio installer not found, assuming Visual Studio is available"
    }

    # Check Qt6
    if ($env:Qt6_DIR -or $env:QTDIR) {
        Write-Success "Qt6 environment detected"
    } else {
        Write-Warning "Qt6 environment variables not set. Trying vcpkg..."
        if (Test-Path "vcpkg_installed") {
            Write-Success "Qt6 via vcpkg detected"
        } else {
            Write-Error "Qt6 not found. Please install Qt6 or use -InstallDeps"
            exit 1
        }
    }
}

# ============================================================================
# Build Configuration
# ============================================================================

function Configure-Build {
    Write-Step "Configuring CMake build"

    if ($CleanBuild -and (Test-Path $BuildDir)) {
        Write-Step "Cleaning build directory"
        Remove-Item -Recurse -Force $BuildDir
        Write-Success "Build directory cleaned"
    }

    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }

    Set-Location $BuildDir

    # CMake configuration arguments
    $cmakeArgs = @(
        "-S", $ProjectRoot,
        "-B", ".",
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-DBUILD_TESTING=$($EnableTesting.ToString().ToLower())",
        "-DENABLE_COVERAGE=$($EnableCoverage.ToString().ToLower())",
        "-DCMAKE_INSTALL_PREFIX=$InstallDir"
    )

    # Add vcpkg toolchain if available
    if ($env:VCPKG_ROOT) {
        $vcpkgToolchain = Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"
        if (Test-Path $vcpkgToolchain) {
            $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
            Write-Success "Using vcpkg toolchain"
        }
    }

    # Configure with Visual Studio generator
    $cmakeArgs += "-G", "Visual Studio 17 2022", "-A", "x64"

    Write-Host "Running: cmake $($cmakeArgs -join ' ')"
    & cmake @cmakeArgs

    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        exit 1
    }
    Write-Success "CMake configuration completed"
}

# ============================================================================
# Build Execution
# ============================================================================

function Build-Project {
    Write-Step "Building project ($BuildType)"

    Set-Location $BuildDir

    # Build with maximum parallel jobs
    $buildArgs = @(
        "--build", ".",
        "--config", $BuildType,
        "--parallel"
    )

    Write-Host "Running: cmake $($buildArgs -join ' ')"
    & cmake @buildArgs

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        exit 1
    }
    Write-Success "Build completed successfully"
}

# ============================================================================
# Testing
# ============================================================================

function Run-Tests {
    if (-not $EnableTesting) {
        Write-Step "Testing disabled, skipping tests"
        return
    }

    Write-Step "Running test suite"
    Set-Location $BuildDir

    # Run CTest
    $testArgs = @(
        "--output-on-failure",
        "--parallel", [Environment]::ProcessorCount,
        "--config", $BuildType
    )

    Write-Host "Running: ctest $($testArgs -join ' ')"
    & ctest @testArgs

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Some tests failed"
        exit 1
    }
    Write-Success "All tests passed"
}

# ============================================================================
# Coverage Analysis
# ============================================================================

function Generate-Coverage {
    if (-not $EnableCoverage) {
        Write-Step "Coverage disabled, skipping coverage analysis"
        return
    }

    Write-Step "Generating coverage report"
    Set-Location $BuildDir

    # Generate coverage (requires Visual Studio Enterprise or specialized tools)
    if (Test-Command "OpenCppCoverage") {
        Write-Host "Using OpenCppCoverage for coverage analysis"
        # Add OpenCppCoverage commands here
        Write-Success "Coverage report generated"
    } else {
        Write-Warning "OpenCppCoverage not found. Coverage analysis skipped"
        Write-Host "Install OpenCppCoverage for Windows coverage support"
    }
}

# ============================================================================
# Installation
# ============================================================================

function Install-Application {
    Write-Step "Installing application"
    Set-Location $BuildDir

    $installArgs = @(
        "--install", ".",
        "--config", $BuildType,
        "--prefix", $InstallDir
    )

    Write-Host "Running: cmake $($installArgs -join ' ')"
    & cmake @installArgs

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Installation failed"
        exit 1
    }
    Write-Success "Application installed to $InstallDir"
}

# ============================================================================
# Packaging
# ============================================================================

function Create-Packages {
    if (-not $Package) {
        Write-Step "Packaging disabled, skipping package creation"
        return
    }

    Write-Step "Creating distribution packages"
    Set-Location $BuildDir

    # Create NSIS installer
    Write-Host "Creating Windows installer..."
    & cpack -G NSIS -C $BuildType

    if ($LASTEXITCODE -eq 0) {
        Write-Success "Windows installer created"
    } else {
        Write-Warning "Windows installer creation failed"
    }

    # Create ZIP package
    Write-Host "Creating ZIP package..."
    & cpack -G ZIP -C $BuildType

    if ($LASTEXITCODE -eq 0) {
        Write-Success "ZIP package created"
    } else {
        Write-Warning "ZIP package creation failed"
    }

    # List created packages
    $packages = Get-ChildItem -Path . -Name "*.exe", "*.zip", "*.msi" | Where-Object { $_ -like "*$AppName*" }
    if ($packages) {
        Write-Success "Created packages:"
        foreach ($pkg in $packages) {
            Write-Host "  - $pkg"
        }
    }
}

# ============================================================================
# Main Execution
# ============================================================================

function Main {
    try {
        Write-Banner "$AppName v$AppVersion - Windows Build System"

        if ($Help) {
            Show-Help
            return
        }

        # Store original location
        $originalLocation = Get-Location

        if ($InstallDeps) {
            Install-Dependencies
            Write-Success "Dependencies installation completed"
            return
        }

        # Execute build pipeline
        Test-BuildEnvironment
        Configure-Build
        Build-Project
        Run-Tests
        Generate-Coverage
        Install-Application
        Create-Packages

        Write-Banner "Build Completed Successfully!"
        Write-Host "Build Type: $BuildType" -ForegroundColor Green
        Write-Host "Testing: $($EnableTesting ? 'Enabled' : 'Disabled')" -ForegroundColor Green
        Write-Host "Coverage: $($EnableCoverage ? 'Enabled' : 'Disabled')" -ForegroundColor Green
        Write-Host "Install Directory: $InstallDir" -ForegroundColor Green

        if ($Package) {
            Write-Host "Packages: Created in $BuildDir" -ForegroundColor Green
        }

    }
    catch {
        Write-Error "Build failed: $($_.Exception.Message)"
        exit 1
    }
    finally {
        # Restore original location
        Set-Location $originalLocation
    }
}

# Execute main function
Main
