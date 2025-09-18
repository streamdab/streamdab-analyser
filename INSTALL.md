# ETI Stream Analyzer - Installation Guide

## 🚀 Quick Start

**Choose your preferred build method:**

### Option 1: Docker Build (Recommended)
```bash
# Clone repository
git clone https://github.com/eti-analyzer/streamdab-analyzer.git
cd streamdab-analyzer

# Build with Docker (cross-platform, reproducible)
docker build -t eti-analyzer .
docker run -it eti-analyzer
```

### Option 2: Native Build
```bash
# Linux/macOS
./scripts/build-linux-macos.sh --install-deps
./scripts/build-linux-macos.sh

# Windows (PowerShell)
.\scripts\build-windows.ps1 -InstallDeps
.\scripts\build-windows.ps1
```

---

## 📋 Table of Contents

- [System Requirements](#system-requirements)
- [Docker Build System](#docker-build-system)
- [Native Build Instructions](#native-build-instructions)
  - [Windows](#windows-native-build)
  - [Linux](#linux-native-build)
  - [macOS](#macos-native-build)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)
- [Development Setup](#development-setup)
- [Testing](#testing)
- [Packaging](#packaging)

---

## 🖥️ System Requirements

### Minimum Requirements
- **OS**: Windows 10 (1903+), Ubuntu 20.04+, macOS 10.15+
- **RAM**: 4 GB (8 GB recommended for development)
- **Storage**: 2 GB free space (5 GB for full development setup)
- **CPU**: x64 architecture, 2+ cores

### Build Requirements
- **CMake**: 3.21 or later
- **Qt6**: 6.2 or later
- **C++ Compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **Dependencies**: See platform-specific sections below

---

## 🐳 Docker Build System

The Docker build system provides **reproducible, cross-platform builds** with all dependencies pre-configured.

### Available Docker Targets

```bash
# Development environment (default)
docker build -t eti-analyzer:dev .
docker run -it -v $(pwd):/workspace eti-analyzer:dev

# Production build
docker build --target production -t eti-analyzer:prod .

# Testing environment
docker build --target testing -t eti-analyzer:test .

# Cross-platform packaging
docker build --target packager -t eti-analyzer:package .

# Windows cross-compilation
docker build --target windows-builder -t eti-analyzer:windows .

# Documentation generation
docker build --target documentation -t eti-analyzer:docs .

# AppImage creation
docker build --target appimage -t eti-analyzer:appimage .
```

### Docker Development Workflow

```bash
# Start development container
docker run -it -v $(pwd):/workspace eti-analyzer:dev

# Inside container - available commands:
build-debug      # Debug build with testing
build-release    # Release build
test-all         # Run all tests
coverage         # Generate coverage report

# Or run specific builds:
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

### Docker Compose (Advanced)

```yaml
# docker-compose.yml
version: '3.8'
services:
  eti-dev:
    build: .
    volumes:
      - .:/workspace
      - build-cache:/workspace/build
    environment:
      - BUILD_TYPE=Debug
      - ENABLE_TESTING=ON
    command: /bin/bash

volumes:
  build-cache:
```

```bash
# Use with docker-compose
docker-compose up -d eti-dev
docker-compose exec eti-dev /bin/bash
```

---

## 🏗️ Native Build Instructions

### Windows Native Build

#### Prerequisites

1. **Visual Studio 2019/2022** with C++ workload
   ```powershell
   # Download from: https://visualstudio.microsoft.com/downloads/
   # Required components:
   # - MSVC v143 compiler toolset
   # - Windows 10/11 SDK
   # - CMake tools for Visual Studio
   ```

2. **vcpkg** package manager
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

3. **CMake** (if not included with Visual Studio)
   ```powershell
   # Download from: https://cmake.org/download/
   # Or via chocolatey:
   choco install cmake
   ```

#### Automated Installation

```powershell
# Install all dependencies automatically
.\scripts\build-windows.ps1 -InstallDeps

# Build with default settings (Release)
.\scripts\build-windows.ps1

# Build with custom options
.\scripts\build-windows.ps1 -BuildType Debug -EnableTesting -Package
```

#### Manual Build Steps

```powershell
# 1. Install dependencies via vcpkg
vcpkg install qt6[core,widgets,network,multimedia,test]:x64-windows
vcpkg install faad2:x64-windows fftw3:x64-windows yaml-cpp:x64-windows
vcpkg install gtest:x64-windows gmock:x64-windows

# 2. Configure build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" `
         -DCMAKE_BUILD_TYPE=Release `
         -DBUILD_TESTING=ON

# 3. Build
cmake --build . --config Release --parallel

# 4. Test
ctest --output-on-failure --config Release

# 5. Install
cmake --install . --config Release --prefix ../install
```

#### Windows-Specific Notes

- **Qt6 Installation**: Can use official installer or vcpkg
- **NSIS**: Required for installer creation (`choco install nsis`)
- **OpenCppCoverage**: For coverage analysis (`choco install opencppcoverage`)
- **Path Length**: Enable long path support in Windows for deep CMake builds

---

### Linux Native Build

#### Ubuntu/Debian

```bash
# Install dependencies
sudo apt update
sudo apt install -y \
    build-essential cmake pkg-config git \
    qt6-base-dev qt6-tools-dev qt6-multimedia-dev \
    libfaad-dev libfftw3-dev libyaml-cpp-dev \
    libasound2-dev libpulse-dev \
    lcov gcovr

# Build
./scripts/build-linux-macos.sh --install-deps  # Optional: auto-install
./scripts/build-linux-macos.sh --build-type Release --enable-testing
```

#### Fedora/CentOS/RHEL

```bash
# Install dependencies
sudo dnf install -y \
    gcc-c++ cmake pkgconfig git \
    qt6-qtbase-devel qt6-qttools-devel qt6-qtmultimedia-devel \
    faad2-devel fftw-devel yaml-cpp-devel \
    alsa-lib-devel pulseaudio-libs-devel \
    lcov

# Build
./scripts/build-linux-macos.sh
```

#### Arch Linux

```bash
# Install dependencies
sudo pacman -S --needed \
    base-devel cmake pkgconf git \
    qt6-base qt6-tools qt6-multimedia \
    faad2 fftw yaml-cpp \
    alsa-lib libpulse lcov

# Build
./scripts/build-linux-macos.sh
```

#### Manual Build Steps

```bash
# 1. Verify Qt6 installation
qmake6 -query QT_VERSION  # Should show 6.x.x

# 2. Configure
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DBUILD_TESTING=ON \
         -DENABLE_COVERAGE=ON

# 3. Build
cmake --build . -j$(nproc)

# 4. Test
ctest --output-on-failure -j$(nproc)

# 5. Install
cmake --install . --prefix ../install
```

---

### macOS Native Build

#### Prerequisites

1. **Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **Homebrew** package manager
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

3. **Dependencies via Homebrew**
   ```bash
   brew install cmake pkg-config qt6 faad2 fftw yaml-cpp portaudio git lcov
   ```

#### Automated Build

```bash
# Install dependencies and build
./scripts/build-linux-macos.sh --install-deps
./scripts/build-linux-macos.sh --build-type Release --package
```

#### Manual Build Steps

```bash
# 1. Set Qt6 environment
export Qt6_DIR="$(brew --prefix qt6)/lib/cmake/Qt6"
export PATH="$(brew --prefix qt6)/bin:$PATH"

# 2. Configure
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DBUILD_TESTING=ON \
         -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
         -DCMAKE_MACOSX_BUNDLE=ON

# 3. Build
cmake --build . -j$(sysctl -n hw.ncpu)

# 4. Test
ctest --output-on-failure

# 5. Create DMG
cpack -G DragNDrop
```

#### macOS-Specific Notes

- **Notarization**: For distribution, apps need to be notarized
- **Code Signing**: Set up Apple Developer certificates
- **Homebrew Qt6**: Recommended over official installer for development

---

## ⚙️ Build Options

### CMake Configuration Options

```bash
# Build types
-DCMAKE_BUILD_TYPE=Debug|Release|RelWithDebInfo|MinSizeRel

# Feature toggles
-DBUILD_TESTING=ON|OFF              # Enable/disable test suite
-DENABLE_COVERAGE=ON|OFF            # Enable code coverage
-DENABLE_SANITIZERS=ON|OFF          # Enable address/memory sanitizers
-DBUILD_DOCUMENTATION=ON|OFF        # Generate documentation

# Performance options
-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON  # Link-time optimization
-DCMAKE_UNITY_BUILD=ON                   # Unity/jumbo builds

# Installation
-DCMAKE_INSTALL_PREFIX=/path/to/install  # Installation directory
```

### Script Options

#### Linux/macOS Script
```bash
./scripts/build-linux-macos.sh \
    --build-type Release \
    --enable-testing \
    --enable-coverage \
    --clean \
    --package \
    --jobs 8
```

#### Windows Script
```powershell
.\scripts\build-windows.ps1 `
    -BuildType Release `
    -EnableTesting `
    -EnableCoverage `
    -CleanBuild `
    -Package
```

### Environment Variables

```bash
# Qt6 location (if not in standard paths)
export Qt6_DIR=/path/to/qt6/lib/cmake/Qt6

# vcpkg toolchain (Windows)
export VCPKG_ROOT=/path/to/vcpkg

# Compiler selection
export CC=gcc-11
export CXX=g++-11

# Build parallelism
export CMAKE_BUILD_PARALLEL_LEVEL=8
```

---

## 🐛 Troubleshooting

### Common Issues

#### Qt6 Not Found
```bash
# Linux: Install Qt6 packages
sudo apt install qt6-base-dev  # Ubuntu/Debian
sudo dnf install qt6-qtbase-devel  # Fedora

# macOS: Use Homebrew
brew install qt6

# Windows: Use vcpkg or official installer
vcpkg install qt6:x64-windows
```

#### CMake Version Too Old
```bash
# Linux: Use Kitware APT repository
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update && sudo apt install cmake

# macOS: Use Homebrew
brew install cmake

# Windows: Download from cmake.org
```

#### Compilation Errors

1. **Missing dependencies**
   ```bash
   # Check pkg-config can find libraries
   pkg-config --exists --print-errors faad2 fftw3 yaml-cpp
   
   # Install missing packages
   ```

2. **Linker errors**
   ```bash
   # Clear CMake cache and reconfigure
   rm -rf build
   mkdir build && cd build
   cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
   ```

3. **Qt MOC issues**
   ```bash
   # Ensure Qt6 tools are in PATH
   which qmake6
   which moc
   
   # Clean autogen files
   rm -rf build/*_autogen
   ```

### Debug Information

```bash
# Verbose build output
cmake --build build --verbose

# CMake debug information
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON --debug-output

# Check system information
cmake .. -DCMAKE_FIND_DEBUG_MODE=ON

# Test specific test
cd build
ctest -R test_name --verbose
```

### Platform-Specific Issues

#### Windows
- **Long paths**: Enable in Windows settings or Group Policy
- **vcpkg integration**: Ensure `vcpkg integrate install` was run
- **Visual Studio**: Use "x64 Native Tools Command Prompt"

#### Linux
- **AppImage**: Requires FUSE: `sudo apt install fuse`
- **Qt6 PPA**: For newer Qt6 on older Ubuntu
- **pkg-config**: Ensure PKG_CONFIG_PATH includes Qt6

#### macOS
- **Xcode**: Update to latest Xcode for Apple Silicon support
- **Homebrew**: Use `brew doctor` to diagnose issues
- **Code signing**: Temporary disable for development builds

---

## 🔧 Development Setup

### IDE Configuration

#### Visual Studio Code
```bash
# Install recommended extensions
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools
code --install-extension twxs.cmake

# Open project
code .
```

#### Qt Creator
```bash
# Open CMakeLists.txt as project
# Qt Creator will auto-configure for Qt6 development
```

#### CLion
```bash
# Import CMake project
# CLion provides excellent Qt6 and CMake support
```

### Pre-commit Hooks

```bash
# Install development tools
pip install pre-commit

# Install hooks
pre-commit install

# Run manually
pre-commit run --all-files
```

### Development Workflow

```bash
# 1. Create feature branch
git checkout -b feature/my-feature

# 2. Configure for development
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON

# 3. Build and test continuously
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure

# 4. Check code coverage
cmake --build build --target coverage
open build/coverage/index.html  # View coverage report

# 5. Run static analysis
cmake --build build --target clang-tidy
cmake --build build --target cppcheck
```

---

## 🧪 Testing

### Test Categories

```bash
# Unit tests only
ctest -R "^test_.*" --output-on-failure

# GUI tests only  
ctest -R "gui_test_.*" --output-on-failure

# Integration tests
ctest -R "integration_.*" --output-on-failure

# Performance tests
ctest -R "perf_.*" --output-on-failure
```

### Coverage Analysis

```bash
# Generate coverage report
cmake --build build --target coverage

# Check coverage percentage
cmake --build build --target coverage-check

# View HTML report
open build/coverage/index.html
```

### Memory Testing

```bash
# Linux: Valgrind
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cd build
ctest -T memcheck

# Windows: Application Verifier
# Enable in Windows SDK tools

# macOS: Address Sanitizer
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
```

---

## 📦 Packaging

### Distribution Packages

```bash
# Linux packages
cd build
cpack -G DEB      # Ubuntu/Debian .deb
cpack -G RPM      # Fedora/RHEL .rpm
cpack -G TGZ      # Portable .tar.gz

# macOS packages
cpack -G DragNDrop    # .dmg installer
cpack -G Bundle       # .app bundle

# Windows packages
cpack -G NSIS         # .exe installer
cpack -G ZIP          # Portable .zip
```

### AppImage (Linux)

```bash
# Build AppImage
docker build --target appimage -t eti-analyzer:appimage .
docker run --rm -v $(pwd):/output eti-analyzer:appimage \
    cp /workspace/build-appimage/*.AppImage /output/
```

### Cross-platform CI

```yaml
# .github/workflows/build.yml
name: Build
on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-22.04, windows-2022, macos-12]
        build_type: [Debug, Release]
    
    runs-on: ${{ matrix.os }}
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Build with Docker
      if: matrix.os == 'ubuntu-22.04'
      run: docker build --target testing .
    
    - name: Build Windows
      if: matrix.os == 'windows-2022'
      run: .\scripts\build-windows.ps1 -BuildType ${{ matrix.build_type }}
    
    - name: Build macOS  
      if: matrix.os == 'macos-12'
      run: ./scripts/build-linux-macos.sh --build-type ${{ matrix.build_type }}
```

---

## 📚 Additional Resources

- **Project Repository**: https://github.com/eti-analyzer/streamdab-analyzer
- **Documentation**: https://eti-analyzer.org/docs
- **Issue Tracker**: https://github.com/eti-analyzer/streamdab-analyzer/issues
- **Qt6 Documentation**: https://doc.qt.io/qt-6/
- **CMake Documentation**: https://cmake.org/documentation/
- **ETSI EN 300 799**: ETI specification standard

---

## 📄 License

ETI Stream Analyzer is licensed under the GPL-3.0-or-later license.
See [LICENSE](LICENSE) file for details.

---

**Build System Version**: 1.0.0  
**Last Updated**: 2024  
**Supported Platforms**: Windows 10+, Ubuntu 20.04+, macOS 10.15+