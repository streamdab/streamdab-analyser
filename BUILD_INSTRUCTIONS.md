# ETI Stream Analyzer - Build Instructions

## Phase 1 Week 1 - Complete CMake Build System Setup

This document provides comprehensive build instructions for the ETI Stream Analyzer cross-platform desktop application.

## Prerequisites

### System Requirements
- **Operating System**: Linux (Ubuntu 20.04+), macOS (10.15+), or Windows (10/11)
- **RAM**: Minimum 4GB, recommended 8GB+
- **Storage**: 2GB free space for dependencies and build artifacts
- **Network**: Internet connection for dependency installation

### Required Tools

#### 1. CMake (Version 3.21+)
**macOS (Homebrew):**
```bash
brew install cmake
```

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install cmake
```

**Windows:**
- Download from [cmake.org](https://cmake.org/download/)
- Or use Chocolatey: `choco install cmake`

#### 2. Qt6 Framework
**macOS (Homebrew):**
```bash
brew install qt@6
export Qt6_DIR=$(brew --prefix qt@6)/lib/cmake/Qt6
```

**Ubuntu/Debian:**
```bash
sudo apt-get install qt6-base-dev qt6-tools-dev qt6-tools-dev-tools
```

**Windows:**
- Download Qt6 from [qt.io](https://www.qt.io/download)
- Install Qt Creator with Qt 6.x components
- Add Qt6 bin directory to PATH

#### 3. System Dependencies

**macOS (Homebrew):**
```bash
brew install faad2 fftw yaml-cpp
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libfaad-dev libfftw3-dev libyaml-cpp-dev
```

**Windows:**
Use vcpkg or compile manually:
```cmd
vcpkg install faad2 fftw3 yaml-cpp
```

### Development Tools

#### Compilers
- **Linux/macOS**: GCC 9+ or Clang 10+
- **Windows**: Visual Studio 2019+ with C++ tools

#### Optional Tools
- **Git**: For version control
- **Ninja**: Fast build system (optional alternative to Make)
- **Ccache**: Compiler cache for faster rebuilds

## Project Structure

```
streamdab-analyzer/
├── src/                    # Source code
│   ├── core/              # ETI processing engine
│   ├── gui/               # Qt GUI components  
│   ├── utils/             # Utilities and helpers
│   └── main.cpp           # Application entry point
├── resources/             # Icons, UI files, assets
├── third_party/           # External dependencies
├── tests/                 # Unit and integration tests
├── cmake/                 # CMake modules and scripts
│   └── modules/           # Custom find modules
├── scripts/               # Build automation scripts
├── build/                 # Build output directory
├── CMakeLists.txt         # Root build configuration
└── BUILD_INSTRUCTIONS.md  # This file
```

## Build Process

### Quick Start (Unix/Linux/macOS)

1. **Clone and setup:**
```bash
git clone <repository-url>
cd streamdab-analyzer
```

2. **Install dependencies (macOS example):**
```bash
brew install cmake qt@6 faad2 fftw yaml-cpp
export Qt6_DIR=$(brew --prefix qt@6)/lib/cmake/Qt6
```

3. **Build:**
```bash
./scripts/build.sh
```

4. **Run:**
```bash
./build/ETIStreamAnalyzer
```

### Manual Build Process

1. **Configure:**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

2. **Build:**
```bash
cmake --build . --parallel 4
```

3. **Install (optional):**
```bash
cmake --install .
```

4. **Test:**
```bash
ctest --output-on-failure
```

### Windows Build (Visual Studio)

1. **Setup environment:**
```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
```

2. **Configure:**
```cmd
mkdir build && cd build
cmake -A x64 -DCMAKE_BUILD_TYPE=Release ..
```

3. **Build:**
```cmd
cmake --build . --config Release --parallel 4
```

4. **Run:**
```cmd
.\Release\ETIStreamAnalyzer.exe
```

### Alternative: Use Build Scripts

**Unix/Linux/macOS:**
```bash
# Basic release build
./scripts/build.sh

# Debug build with install
./scripts/build.sh --debug --install

# Clean build with packaging
./scripts/build.sh --clean --package
```

**Windows:**
```cmd
# Basic release build
scripts\build.bat

# Debug build with install  
scripts\build.bat --debug --install

# Clean build with packaging
scripts\build.bat --clean --package
```

## Build Configuration Options

### CMake Options
- `CMAKE_BUILD_TYPE`: Release, Debug, RelWithDebInfo, MinSizeRel
- `CMAKE_INSTALL_PREFIX`: Installation directory
- `CMAKE_PREFIX_PATH`: Additional search paths for dependencies
- `ENABLE_COVERAGE`: Enable code coverage (Debug builds)
- `ENABLE_BENCHMARKS`: Enable benchmark tests

### Advanced Configuration
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_INSTALL_PREFIX=/opt/eti-analyzer \
      -DQt6_DIR=/custom/qt6/path \
      -DENABLE_COVERAGE=ON \
      ..
```

## Packaging

### Linux AppImage
```bash
./scripts/build.sh --package
# Creates: ETI_Stream_Analyzer-1.0.0-x86_64.AppImage
```

### macOS DMG
```bash
./scripts/build.sh --package
# Creates: ETI_Stream_Analyzer-1.0.0-Darwin.dmg
```

### Windows Installer
```cmd
scripts\build.bat --package
REM Creates: ETI_Stream_Analyzer-1.0.0-win64.exe
```

## Testing

### Unit Tests
```bash
# Run all tests
cd build && ctest

# Run tests with output
ctest --output-on-failure --verbose

# Run specific test
ctest -R "eti_processor"
```

### Manual Testing
```bash
# Run test executable directly
./build/eti_analyzer_tests

# Run with Qt test options
./build/eti_analyzer_tests -v2 -o results.xml,xunitxml
```

### Coverage Analysis (Linux/macOS)
```bash
# Configure with coverage
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..

# Build and test
make && make test && make coverage

# View results
open coverage_html/index.html
```

## Troubleshooting

### Common Issues

#### Qt6 Not Found
```
Error: Could not find a package configuration file provided by "Qt6"
```
**Solution:**
```bash
export Qt6_DIR=/path/to/qt6/lib/cmake/Qt6
# Or add to cmake command:
cmake -DQt6_DIR=/path/to/qt6/lib/cmake/Qt6 ..
```

#### Missing Dependencies
```
Error: Could not find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)
```
**Solution:**
```bash
# macOS
brew install pkg-config

# Ubuntu
sudo apt-get install pkg-config
```

#### FAAD2/FFTW3 Not Found
**Solution:**
```bash
# Check installation
pkg-config --list-all | grep -E "(faad|fftw)"

# Install if missing
# macOS: brew install faad2 fftw
# Ubuntu: sudo apt-get install libfaad-dev libfftw3-dev
```

#### Compilation Errors
1. **Check compiler version:**
```bash
g++ --version  # Should be 9.0+
clang++ --version  # Should be 10.0+
```

2. **Clean build:**
```bash
rm -rf build && mkdir build && cd build
cmake ..
```

3. **Verbose build:**
```bash
cmake --build . --verbose
```

### Platform-Specific Notes

#### macOS
- Use Homebrew for dependency management
- Ensure Xcode Command Line Tools are installed: `xcode-select --install`
- For Apple Silicon Macs, use native ARM64 Qt6 build

#### Ubuntu/Debian
- Install build-essential: `sudo apt-get install build-essential`
- Some packages may need universe repository enabled

#### Windows
- Ensure Visual Studio C++ tools are installed
- Use Developer Command Prompt for building
- Consider using vcpkg for dependency management

## Performance Optimization

### Compiler Optimizations
```bash
# Maximum optimization
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native" ..

# Link-time optimization
cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..
```

### Parallel Builds
```bash
# Use all CPU cores
cmake --build . --parallel $(nproc)

# Specific number of jobs
cmake --build . --parallel 8
```

## Development Workflow

### Debug Builds
```bash
./scripts/build.sh --debug
# Enables debug symbols, assertions, verbose logging
```

### IDE Integration
- **Qt Creator**: Open CMakeLists.txt directly
- **Visual Studio**: Use "Open Folder" with CMake support
- **CLion**: Import CMake project
- **VS Code**: Use CMake Tools extension

### Continuous Integration
The build system supports CI/CD pipelines:
- GitHub Actions (Linux, macOS, Windows)
- GitLab CI (Docker-based builds)
- Jenkins (enterprise builds)

## Phase 1 Status: COMPLETE ✅

### Deliverables Completed:
1. ✅ Complete project structure created
2. ✅ CMakeLists.txt with Qt6 integration configured  
3. ✅ Cross-platform dependency management implemented
4. ✅ Build scripts for all platforms created
5. ✅ Testing framework integrated
6. ✅ Build verification system implemented

### Success Criteria Met:
- ✅ CMake generates successfully on target platform
- ✅ Basic Qt application structure implemented
- ✅ All dependencies properly configured
- ✅ Build system ready for ETISnoop integration (Phase 2)

### Next Phase: ETISnoop Integration
Ready to integrate ETISnoop source code and implement ETI processing functionality.

## Support

For build issues or questions:
1. Check this documentation first
2. Review error messages carefully
3. Verify all dependencies are installed
4. Try a clean build
5. Consult platform-specific notes above

The build system has been thoroughly tested and designed for reliability across all supported platforms.