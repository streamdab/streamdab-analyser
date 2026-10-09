# ============================================================================
# StreamDAB Analyser - Windows native build helper (MSVC + Qt6 + vcpkg)
#
# Configures with the Visual Studio 2022 generator, builds with bounded
# parallelism, then (optionally) runs the test suite offscreen. See
# docs/WINDOWS_BUILD.md for the full, detailed guide.
#
# Deliberately simple and correct rather than feature-complete: it does not
# install dependencies, package, or measure coverage.
# ============================================================================
[CmdletBinding()]
param(
    [ValidateSet("Release", "Debug", "RelWithDebInfo")]
    [string]$Config = "Release",

    # Build directory, relative to the repository root.
    [string]$BuildDir = "build",

    # Qt MSVC prefix: the directory containing lib\cmake\Qt6 (e.g.
    # C:\Qt\6.11.0\msvc2022_64). Auto-detected when empty.
    [string]$QtDir = "",

    # vcpkg root: the directory containing scripts\buildsystems\vcpkg.cmake.
    # Falls back to the VCPKG_ROOT environment variable.
    [string]$VcpkgRoot = "",

    # Bounded parallelism. Never leave this unbounded on Windows (each cl.exe
    # holds 1-2 GB; recursive subprojects add more).
    [ValidateRange(1, 64)]
    [int]$Parallel = 8,

    [switch]$DisableZmq,
    [switch]$EnableAlsa,
    [switch]$SkipTests,
    [switch]$Clean,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

$RepoRoot  = Split-Path -Parent $PSScriptRoot
$BuildPath = Join-Path $RepoRoot $BuildDir
$DefaultQt = "C:\Qt\6.11.0\msvc2022_64"

function Write-Step([string]$Message) { Write-Host "`n==> $Message" -ForegroundColor Cyan }
function Write-Ok([string]$Message)   { Write-Host "  [ok] $Message" -ForegroundColor Green }
function Write-Warn2([string]$Message){ Write-Host "  [!!] $Message" -ForegroundColor Yellow }

function Fail([string]$Message) {
    Write-Host "`n[error] $Message" -ForegroundColor Red
    exit 1
}

function Show-Help {
    Write-Host @"
StreamDAB Analyser - Windows build (MSVC + Qt6 + vcpkg)

USAGE:
  .\scripts\build-windows.ps1 [options]

OPTIONS:
  -Config <Release|Debug|RelWithDebInfo>  Build configuration (default: Release)
  -BuildDir <dir>     Build directory relative to the repo (default: build)
  -QtDir <dir>        Qt MSVC prefix (dir containing lib\cmake\Qt6)
  -VcpkgRoot <dir>    vcpkg root (dir containing scripts\buildsystems\vcpkg.cmake)
  -Parallel <n>       Bounded parallel build jobs (default: 8)
  -DisableZmq         Configure with -DDABX_ENABLE_ZMQ=OFF
  -EnableAlsa         Configure with -DDABX_ENABLE_ALSA=ON (Linux only; no-op here)
  -SkipTests          Do not run ctest
  -Clean              Delete the build directory before configuring
  -Help               Show this help

PREREQUISITES (see docs/WINDOWS_BUILD.md):
  VS 2022 Build Tools (Desktop C++), Git, Python + aqtinstall, and vcpkg with
  faad2 fftw3 zeromq cppzmq gtest installed for the x64-windows triplet.

EXAMPLES:
  .\scripts\build-windows.ps1
  .\scripts\build-windows.ps1 -Clean -Parallel 6
  .\scripts\build-windows.ps1 -DisableZmq -SkipTests
"@
}

function Resolve-QtPath {
    if ($QtDir) {
        if (Test-Path (Join-Path $QtDir "lib\cmake\Qt6")) { return (Resolve-Path $QtDir).Path }
        Fail "QtDir '$QtDir' does not contain lib\cmake\Qt6."
    }
    if ($env:QTDIR -and (Test-Path (Join-Path $env:QTDIR "lib\cmake\Qt6"))) {
        return $env:QTDIR
    }
    if (Test-Path (Join-Path $DefaultQt "lib\cmake\Qt6")) { return $DefaultQt }

    # Last resort: find a msvc* desktop kit under C:\Qt. The standard layout is
    # C:\Qt\<version>\msvc2022_64, so scan two levels deep (-Depth 2).
    if (Test-Path "C:\Qt") {
        $candidate = Get-ChildItem "C:\Qt" -Directory -Recurse -Depth 2 -ErrorAction SilentlyContinue |
            Where-Object {
                $_.Name -like "msvc*" -and
                (Test-Path (Join-Path $_.FullName "lib\cmake\Qt6"))
            } |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($candidate) {
            return $candidate.FullName
        }
    }
    Fail ("Qt (MSVC) not found. Install it with aqtinstall and pass -QtDir, e.g. " +
          "-QtDir C:\Qt\6.11.0\msvc2022_64.")
}

function Resolve-VcpkgRoot {
    if ($VcpkgRoot) {
        if (Test-Path (Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake")) {
            return (Resolve-Path $VcpkgRoot).Path
        }
        Fail "VcpkgRoot '$VcpkgRoot' does not contain scripts\buildsystems\vcpkg.cmake."
    }
    if ($env:VCPKG_ROOT -and
        (Test-Path (Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"))) {
        return $env:VCPKG_ROOT
    }
    Fail "vcpkg not found. Set VCPKG_ROOT or pass -VcpkgRoot (see docs/WINDOWS_BUILD.md)."
}

# ============================================================================
# Main
# ============================================================================

if ($Help) {
    Show-Help
    exit 0
}

Write-Host "StreamDAB Analyser - Windows build" -ForegroundColor Green
Write-Host "  Config:   $Config"
Write-Host "  BuildDir: $BuildPath"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Fail "cmake was not found on PATH."
}

$qt        = Resolve-QtPath
$vcpkg     = Resolve-VcpkgRoot
$toolchain = Join-Path $vcpkg "scripts\buildsystems\vcpkg.cmake"
Write-Ok "Qt:    $qt"
Write-Ok "vcpkg: $vcpkg"

if ($Clean -and (Test-Path $BuildPath)) {
    Write-Step "Cleaning $BuildPath"
    Remove-Item -Recurse -Force $BuildPath
}

# Set BEFORE configure: tests/CMakeLists.txt latches CAN_RUN_GUI_TESTS at
# configure time, so without this a fresh run would silently register only the
# non-GUI suites. It stays set for the ctest step below.
$env:QT_QPA_PLATFORM = "offscreen"

Write-Step "Configuring (Visual Studio 17 2022, x64, $Config)"
$configureArgs = @(
    "-B", $BuildPath,
    "-S", $RepoRoot,
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_PREFIX_PATH=$qt",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    "-DVCPKG_TARGET_TRIPLET=x64-windows"
)
if ($DisableZmq) { $configureArgs += "-DDABX_ENABLE_ZMQ=OFF" }
if ($EnableAlsa) { $configureArgs += "-DDABX_ENABLE_ALSA=ON" }

Write-Host "  cmake $($configureArgs -join ' ')"
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) { Fail "CMake configuration failed." }
Write-Ok "Configuration complete"

Write-Step "Building ($Config, $Parallel jobs)"
& cmake --build $BuildPath --config $Config --parallel $Parallel
if ($LASTEXITCODE -ne 0) { Fail "Build failed." }
Write-Ok "Build complete"

if (-not $SkipTests) {
    Write-Step "Testing (QT_QPA_PLATFORM=offscreen, set before configure)"
    Write-Warn2 ("Tests that read eti\*.eti (gitignored) fail without the fixtures; " +
                 "obtain them or run with -SkipTests.")
    & ctest --test-dir $BuildPath -C $Config --output-on-failure
    if ($LASTEXITCODE -ne 0) { Fail "Some tests failed." }
    Write-Ok "All tests passed"
}

Write-Host "`nDone. Binaries are in $BuildPath\$Config" -ForegroundColor Green
exit 0
