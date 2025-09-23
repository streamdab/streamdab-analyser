# StreamDAB Stream Analyzer - Production Deployment Guide

## Overview

This guide provides comprehensive instructions for building, packaging, and deploying the StreamDAB Stream Analyzer across different platforms with professional broadcast industry standards.

## Build System Architecture

### Core Components
- **CMake 3.20+**: Cross-platform build system with Qt6 integration
- **Qt6 Framework**: Professional GUI with LGPL-3.0/GPL-3.0 licensing
- **Professional Packaging**: Platform-specific installers and packages
- **Dependency Management**: Automated dependency resolution and bundling

### Build Configurations
- **Debug**: Development build with debugging symbols and verbose output
- **Release**: Optimized production build with performance optimizations
- **RelWithDebInfo**: Release build with debugging information retained

## Production Build Process

### 1. Prerequisites

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install \
    build-essential cmake ninja-build \
    qt6-base-dev qt6-multimedia-dev qt6-tools-dev \
    libfdk-aac-dev libfftw3-dev libasound2-dev \
    pkg-config git
```

#### Linux (Fedora/RHEL)
```bash
sudo dnf install \
    gcc-c++ cmake ninja-build \
    qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qttools-devel \
    fdk-aac-devel fftw-devel alsa-lib-devel \
    pkgconf git
```

#### macOS
```bash
# Install Homebrew if not present
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake qt6 fdk-aac fftw
```

#### Windows
```powershell
# Install Qt6 from official installer
# Download and install Visual Studio 2022 Community
# Install vcpkg for dependency management
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install qt6 fdk-aac fftw3
```

### 2. Build Commands

#### Quick Production Build
```bash
# Clone and build
git clone <repository-url>
cd streamdab-analyzer
./scripts/build_production_release.sh
```

#### Manual Production Build
```bash
# Create release build directory
mkdir build-release && cd build-release

# Configure for production
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -march=native -mtune=native" \
    -DENABLE_CLANG_TIDY=OFF \
    -DBUILD_TESTING=ON

# Build with all CPU cores
make -j$(nproc)

# Run tests
make test

# Create packages
make package
```

## Platform-Specific Packaging

### Linux Packaging

#### DEB Package (Ubuntu/Debian)
```bash
# Build DEB package
make package-linux-deb

# Install locally for testing
sudo dpkg -i streamdab-stream-analyzer_*.deb
sudo apt-get install -f  # Fix dependencies
```

#### RPM Package (Fedora/RHEL/SUSE)
```bash
# Build RPM package
make package-linux-rpm

# Install locally for testing
sudo rpm -i streamdab-stream-analyzer-*.rpm
# or
sudo dnf install streamdab-stream-analyzer-*.rpm
```

#### AppImage (Universal Linux)
```bash
# Create AppImage package
make package-linux-appimage

# Test AppImage
chmod +x StreamDAB_Stream_Analyzer-*.AppImage
./StreamDAB_Stream_Analyzer-*.AppImage
```

### Windows Packaging

#### NSIS Installer
```bash
# Configure with NSIS generator
cmake .. -DCPACK_GENERATOR="NSIS"

# Build installer
make package-windows

# Result: StreamDAB_Stream_Analyzer_Setup.exe
```

### macOS Packaging

#### DMG Distribution
```bash
# Configure for macOS bundle
cmake .. \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"

# Build DMG
make package-macos

# Result: StreamDAB_Stream_Analyzer.dmg
```

## Quality Assurance

### Automated Validation
```bash
# Run comprehensive package validation
./packaging/scripts/validate_packages.sh

# Check dependency licensing
./scripts/check_dependencies_license.sh

# Generate compliance report
# Creates: LICENSE_COMPLIANCE_REPORT.md
```

### Manual Testing Checklist

#### Basic Functionality
- [ ] Application launches without errors
- [ ] File → Open successfully loads ETI files
- [ ] GUI elements render correctly
- [ ] Real-time mode initializes properly
- [ ] Export functionality works

#### Platform Integration
- [ ] Desktop shortcuts created
- [ ] File associations work (.eti files)
- [ ] Application menu entries correct
- [ ] Uninstaller functions properly
- [ ] System tray integration (if enabled)

#### Performance Validation
- [ ] Memory usage < 100MB during normal operation
- [ ] ETI processing maintains >900 FPS
- [ ] GUI remains responsive during intensive processing
- [ ] No memory leaks during extended operation

## Distribution Strategy

### Open Source Distribution
- **License**: GPL-3.0 with LGPL linking exception
- **Audio Decoder**: FAAD2 (GPL-2.0 compatible)
- **Target**: Broadcast engineers, researchers, open source community
- **Channels**: GitHub Releases, Linux distribution repositories

### Commercial Distribution
- **License**: Commercial licensing available
- **Audio Decoder**: FDK-AAC (with patent considerations)
- **Target**: Professional broadcast facilities
- **Channels**: Direct sales, professional integrators

## License Compliance

### Required Attributions
```
Third-party licenses must be included in distribution:
- Qt6: LGPL-3.0/GPL-3.0 attribution
- FFTW3: GPL-2.0+ acknowledgment
- Audio Decoder: Appropriate license text
- ALSA: LGPL-2.1 attribution (Linux)
```

### Patent Considerations
- DAB/DAB+ technology patents may apply
- FDK-AAC may require patent licensing for commercial use
- Consult patent attorney for commercial deployments
- Open source/research use typically has different requirements

## Deployment Environments

### Broadcast Facilities
- **Requirements**: High reliability, 24/7 operation capability
- **Recommended**: DEB/RPM packages for automated deployment
- **Configuration**: Centralized configuration management
- **Support**: Professional support channels

### Research Institutions
- **Requirements**: Flexibility, source code access
- **Recommended**: Source packages or AppImage
- **Configuration**: Local customization support
- **Support**: Community forums, documentation

### Individual Users
- **Requirements**: Easy installation, minimal configuration
- **Recommended**: Platform-specific installers (NSIS, DMG, DEB)
- **Configuration**: GUI-based setup wizards
- **Support**: User guides, video tutorials

## Troubleshooting

### Common Build Issues

#### Missing Qt6
```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev qt6-tools-dev

# Fedora/RHEL
sudo dnf install qt6-qtbase-devel qt6-qttools-devel
```

#### Audio Decoder Problems
```bash
# Check available decoders
pkg-config --list-all | grep -E "(fdk-aac|faad2)"

# Install FDK-AAC
sudo apt install libfdk-aac-dev  # Ubuntu
sudo dnf install fdk-aac-devel   # Fedora
```

#### CMake Configuration Issues
```bash
# Clear CMake cache
rm -rf build/CMakeCache.txt build/CMakeFiles/

# Reconfigure with verbose output
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
```

### Runtime Issues

#### Missing Dependencies
```bash
# Check dynamic library dependencies
ldd ETIStreamAnalyzer

# Install missing libraries
sudo apt install <missing-package>
```

#### Performance Problems
```bash
# Check CPU scaling governor
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Set performance mode
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

## Support and Maintenance

### Version Management
- **Semantic Versioning**: MAJOR.MINOR.PATCH format
- **Release Cycles**: Quarterly feature releases, monthly bug fixes
- **LTS Support**: Long-term support for major versions

### Update Strategy
- **Automatic Updates**: Optional for professional installations
- **Manual Updates**: Package manager integration
- **Rollback Support**: Previous version retention

### Professional Support
- **Commercial Licensing**: Professional support included
- **Integration Services**: Custom deployment assistance
- **Training Programs**: Broadcast engineer certification

## Security Considerations

### Code Signing
- **Windows**: Authenticode signing for NSIS installer
- **macOS**: Apple Developer certificate for DMG
- **Linux**: GPG signature for packages

### Network Security
- **ETI-over-IP**: Secure multicast configuration
- **Update Mechanism**: Encrypted update channels
- **Configuration**: Secure credential storage

---

**Last Updated**: September 2025  
**Version**: 1.0.0  
**Target Platforms**: Linux, Windows 10/11, macOS 10.15+