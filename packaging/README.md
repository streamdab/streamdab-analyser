# ETI Stream Analyser - Professional Cross-Platform Packaging

This directory contains the professional packaging infrastructure for the ETI Stream Analyser desktop application, designed to create production-ready distributions for Windows, Linux, and macOS platforms.

## 🚀 Overview

The packaging system provides comprehensive cross-platform distribution support for broadcast engineering professionals:

- **Windows**: Professional NSIS installer with file associations and broadcast branding
- **Linux**: DEB, RPM, and AppImage packages for universal compatibility
- **macOS**: Professional DMG with native bundle and code signing preparation
- **Source**: Complete source distributions for open source compliance

## 📁 Directory Structure

```
packaging/
├── windows/               # Windows NSIS installer configuration
│   ├── nsis_config.nsh   # Professional installer settings
│   └── installer_sections.nsh  # Component installation sections
├── linux/                # Linux package configurations
│   ├── postinst          # Debian/Ubuntu post-installation script
│   ├── prerm             # Debian/Ubuntu pre-removal script
│   ├── rpm_postinst.sh   # RPM post-installation script
│   ├── rpm_preun.sh      # RPM pre-uninstallation script
│   └── create_appimage.sh # AppImage creation script
├── macos/                # macOS bundle configuration
│   ├── Info.plist.in     # Professional app bundle metadata
│   ├── entitlements.plist # Security entitlements
│   └── DMGSetup.scpt     # DMG layout and styling script
└── scripts/              # Cross-platform build and validation scripts
    ├── build_all_packages.sh    # Comprehensive build automation
    └── validate_packages.sh     # Package integrity validation
```

## 🛠️ Professional Build System

### Automated Build Pipeline

The packaging system integrates with GitHub Actions for professional CI/CD:

```yaml
# .github/workflows/professional-packaging.yml
# Automated cross-platform builds with professional validation
```

### Manual Build Process

For local development and testing:

```bash
# Build all packages for current platform
./packaging/scripts/build_all_packages.sh

# Validate created packages
./packaging/scripts/validate_packages.sh

# Create specific package types via CMake
cd build
cpack -G NSIS     # Windows NSIS installer
cpack -G DEB      # Linux DEB package
cpack -G RPM      # Linux RPM package
cpack -G DragNDrop # macOS DMG
```

## 🎯 Platform-Specific Features

### Windows Professional Installer

- **NSIS-based installer** with professional broadcast branding
- **File associations** for .eti, .etini, .etili files
- **Registry integration** for professional tool registration
- **Component selection** (Runtime, Documentation, Samples, Development)
- **Desktop and Start Menu** shortcuts
- **Professional uninstaller** with complete cleanup

### Linux Professional Packages

- **DEB packages** for Debian/Ubuntu systems with professional metadata
- **RPM packages** for RedHat/Fedora/SUSE with broadcast categorization
- **AppImage** for universal Linux distribution
- **Desktop integration** with MIME type registration
- **Professional documentation** and sample file installation

### macOS Professional Bundle

- **Native .app bundle** with professional Info.plist configuration
- **DMG disk image** with professional styling and layout
- **File type associations** for ETI broadcast files
- **Code signing preparation** for Apple Developer Program
- **Gatekeeper compatibility** for secure distribution

## 🔧 Configuration Options

### Build Configuration

The packaging system supports multiple build configurations:

```bash
# Environment variables for customization
export BUILD_TYPE=Release          # Release, Debug, RelWithDebInfo
export PARALLEL_JOBS=8             # Parallel build jobs
export CMAKE_BUILD_TYPE=Release    # CMake build type
export QT_VERSION=6.5.0           # Qt framework version
```

### Platform-Specific Settings

#### Windows Configuration
- Visual Studio 2022 toolchain
- Qt6 framework with professional styling
- NSIS installer with broadcast branding
- Code signing preparation for distribution

#### Linux Configuration
- GCC/Clang compiler support
- Qt6 with native Linux integration
- Desktop file and MIME type registration
- Professional package metadata

#### macOS Configuration
- Xcode toolchain with universal binary support
- Qt6 framework with native macOS integration
- Professional bundle configuration
- Apple Developer Program preparation

## 🧪 Quality Assurance

### Professional Validation

The packaging system includes comprehensive validation:

- **Package integrity** verification
- **Dependency checking** for all platforms
- **Functional testing** of installed applications
- **Metadata validation** for professional standards
- **Cross-platform compatibility** verification

### Testing Matrix

| Platform | Package Type | Validation |
|----------|--------------|------------|
| Windows 10/11 | NSIS .exe | ✅ File structure, registry entries, shortcuts |
| Ubuntu 20.04+ | .deb | ✅ Package metadata, dependencies, desktop integration |
| Fedora 35+ | .rpm | ✅ Package structure, SELinux compatibility |
| Universal Linux | AppImage | ✅ Executable permissions, library bundling |
| macOS 10.15+ | .dmg | ✅ Bundle structure, code signing, file associations |

## 📦 Distribution

### Release Workflow

1. **Development builds** created on every commit
2. **Release candidates** for tagged versions
3. **Professional validation** before public release
4. **Multi-platform distribution** via GitHub Releases

### Package Naming Convention

```
ETI_Stream_Analyser-{version}-{platform}-{arch}.{extension}

Examples:
- ETI_Stream_Analyser-1.0.0-windows-x64.exe
- ETI_Stream_Analyser-1.0.0-linux-x86_64.deb
- ETI_Stream_Analyser-1.0.0-linux-x86_64.AppImage
- ETI_Stream_Analyser-1.0.0-macos-universal.dmg
```

## 🔐 Security and Code Signing

### Windows Code Signing
- **Authenticode** signature preparation
- **Certificate validation** for professional distribution
- **SmartScreen** compatibility

### macOS Code Signing
- **Apple Developer ID** preparation
- **Notarization** workflow setup
- **Gatekeeper** compatibility validation

### Linux Security
- **GPG signature** preparation for repository distribution
- **AppImage** integrity verification
- **SELinux** compatibility for enterprise environments

## 📋 Professional Requirements

### Broadcast Industry Standards
- **ETSI compliance** documentation
- **Professional metadata** in all packages
- **Broadcast engineering** workflow integration
- **Industry-standard** file associations

### Enterprise Deployment
- **Silent installation** options for Windows
- **Package manager** integration for Linux
- **Enterprise distribution** preparation for macOS
- **Professional support** documentation

## 🚀 Getting Started

### Prerequisites

#### Windows
- Visual Studio 2022 with C++ workload
- Qt6 framework
- NSIS installer creation system
- Windows SDK

#### Linux
- GCC/Clang compiler
- Qt6 development packages
- CMake and build tools
- Package creation tools (dpkg-dev, rpm-build)

#### macOS
- Xcode command line tools
- Qt6 framework (via Homebrew)
- CMake and build tools
- Apple Developer account (for signing)

### Quick Start

```bash
# Clone the repository
git clone https://github.com/eti-analyser/desktop.git
cd desktop

# Build all packages
./packaging/scripts/build_all_packages.sh

# Validate packages
./packaging/scripts/validate_packages.sh

# Find created packages
find build -name "*.exe" -o -name "*.deb" -o -name "*.rpm" -o -name "*.dmg" -o -name "*.AppImage"
```

## 📞 Professional Support

For broadcast engineering support and professional deployment assistance:

- **Email**: support@eti-analyser.org
- **Documentation**: https://github.com/eti-analyser/desktop/wiki
- **Issues**: https://github.com/eti-analyser/desktop/issues

---

**ETI Stream Analyser Professional Packaging System**  
*Designed for broadcast engineering excellence*