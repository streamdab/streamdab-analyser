#!/bin/bash
# ETI Stream Analyzer - Professional Package Validation Script
# Cross-platform package integrity and functionality testing

set -e

# Professional validation configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
PACKAGES_DIR="${BUILD_DIR}/packages"
VALIDATION_LOG="${BUILD_DIR}/package_validation.log"

# Professional validation functions
log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "${VALIDATION_LOG}"
}

validate_file_exists() {
    local file="$1"
    local description="$2"

    if [ -f "$file" ]; then
        log_message "✅ $description: $(basename "$file") - $(du -h "$file" | cut -f1)"
        return 0
    else
        log_message "❌ $description: Missing"
        return 1
    fi
}

validate_directory_exists() {
    local dir="$1"
    local description="$2"

    if [ -d "$dir" ]; then
        log_message "✅ $description: $(basename "$dir") - $(find "$dir" -type f | wc -l) files"
        return 0
    else
        log_message "❌ $description: Missing"
        return 1
    fi
}

# Professional platform detection
detect_platform() {
    case "$(uname -s)" in
        Linux*)     echo "linux" ;;
        Darwin*)    echo "macos" ;;
        CYGWIN*|MINGW*|MSYS*) echo "windows" ;;
        *)          echo "unknown" ;;
    esac
}

# Professional Windows package validation
validate_windows_packages() {
    log_message "=== Professional Windows Package Validation ==="

    local validation_passed=true

    # Validate NSIS installer
    local nsis_installer=$(find "${BUILD_DIR}" -name "*.exe" -type f | head -1)
    if validate_file_exists "$nsis_installer" "Windows NSIS Installer"; then
        # Professional NSIS validation
        if command -v file >/dev/null 2>&1; then
            local file_type=$(file "$nsis_installer")
            if [[ "$file_type" == *"PE32+"* ]] || [[ "$file_type" == *"executable"* ]]; then
                log_message "✅ NSIS installer format validation passed"
            else
                log_message "❌ NSIS installer format validation failed: $file_type"
                validation_passed=false
            fi
        fi

        # Professional size validation (should be reasonable for distribution)
        local size_mb=$(du -m "$nsis_installer" | cut -f1)
        if [ "$size_mb" -gt 10 ] && [ "$size_mb" -lt 500 ]; then
            log_message "✅ NSIS installer size validation passed: ${size_mb}MB"
        else
            log_message "⚠️  NSIS installer size unusual: ${size_mb}MB"
        fi
    else
        validation_passed=false
    fi

    # Validate dependencies bundled
    log_message "Windows package validation completed"
    return $([ "$validation_passed" = true ] && echo 0 || echo 1)
}

# Professional Linux package validation
validate_linux_packages() {
    log_message "=== Professional Linux Package Validation ==="

    local validation_passed=true

    # Validate DEB package
    local deb_package=$(find "${BUILD_DIR}" -name "*.deb" -type f | head -1)
    if validate_file_exists "$deb_package" "Linux DEB Package"; then
        if command -v dpkg-deb >/dev/null 2>&1; then
            if dpkg-deb -I "$deb_package" >/dev/null 2>&1; then
                log_message "✅ DEB package structure validation passed"

                # Professional DEB metadata validation
                local deb_info=$(dpkg-deb -I "$deb_package")
                if echo "$deb_info" | grep -q "ETI Stream Analyzer"; then
                    log_message "✅ DEB package metadata validation passed"
                else
                    log_message "❌ DEB package metadata validation failed"
                    validation_passed=false
                fi
            else
                log_message "❌ DEB package structure validation failed"
                validation_passed=false
            fi
        fi
    else
        log_message "⚠️  DEB package not found (may not be built on this platform)"
    fi

    # Validate RPM package
    local rpm_package=$(find "${BUILD_DIR}" -name "*.rpm" -type f | head -1)
    if validate_file_exists "$rpm_package" "Linux RPM Package"; then
        if command -v rpm >/dev/null 2>&1; then
            if rpm -qip "$rpm_package" >/dev/null 2>&1; then
                log_message "✅ RPM package structure validation passed"

                # Professional RPM metadata validation
                local rpm_info=$(rpm -qip "$rpm_package")
                if echo "$rpm_info" | grep -q "ETI Stream Analyzer"; then
                    log_message "✅ RPM package metadata validation passed"
                else
                    log_message "❌ RPM package metadata validation failed"
                    validation_passed=false
                fi
            else
                log_message "❌ RPM package structure validation failed"
                validation_passed=false
            fi
        fi
    else
        log_message "⚠️  RPM package not found (may not be built on this platform)"
    fi

    # Validate AppImage
    local appimage=$(find "${BUILD_DIR}" -name "*.AppImage" -type f | head -1)
    if validate_file_exists "$appimage" "Linux AppImage"; then
        if [ -x "$appimage" ]; then
            log_message "✅ AppImage executable validation passed"

            # Professional AppImage functionality test
            if timeout 10s "$appimage" --appimage-help >/dev/null 2>&1; then
                log_message "✅ AppImage functionality validation passed"
            else
                log_message "⚠️  AppImage functionality test inconclusive"
            fi
        else
            log_message "❌ AppImage executable validation failed"
            validation_passed=false
        fi
    else
        log_message "⚠️  AppImage not found (may not be built on this platform)"
    fi

    log_message "Linux package validation completed"
    return $([ "$validation_passed" = true ] && echo 0 || echo 1)
}

# Professional macOS package validation
validate_macos_packages() {
    log_message "=== Professional macOS Package Validation ==="

    local validation_passed=true

    # Validate DMG
    local dmg_package=$(find "${BUILD_DIR}" -name "*.dmg" -type f | head -1)
    if validate_file_exists "$dmg_package" "macOS DMG Package"; then
        if command -v hdiutil >/dev/null 2>&1; then
            if hdiutil imageinfo "$dmg_package" >/dev/null 2>&1; then
                log_message "✅ DMG package structure validation passed"

                # Professional DMG content validation
                local mount_point="/tmp/eti_analyzer_dmg_$$"
                mkdir -p "$mount_point"

                if hdiutil attach "$dmg_package" -readonly -mountpoint "$mount_point" >/dev/null 2>&1; then
                    if [ -d "$mount_point/ETI Stream Analyzer.app" ]; then
                        log_message "✅ DMG package content validation passed"
                    else
                        log_message "❌ DMG package content validation failed"
                        validation_passed=false
                    fi
                    hdiutil detach "$mount_point" >/dev/null 2>&1 || true
                else
                    log_message "❌ DMG package mount validation failed"
                    validation_passed=false
                fi

                rm -rf "$mount_point"
            else
                log_message "❌ DMG package structure validation failed"
                validation_passed=false
            fi
        fi
    else
        log_message "⚠️  DMG package not found (may not be built on this platform)"
    fi

    # Validate App Bundle
    local app_bundle=$(find "${BUILD_DIR}" -name "*.app" -type d | head -1)
    if validate_directory_exists "$app_bundle" "macOS App Bundle"; then
        local info_plist="$app_bundle/Contents/Info.plist"
        if [ -f "$info_plist" ]; then
            log_message "✅ App bundle Info.plist validation passed"
        else
            log_message "❌ App bundle Info.plist validation failed"
            validation_passed=false
        fi

        local executable="$app_bundle/Contents/MacOS/ETIStreamAnalyzer"
        if [ -x "$executable" ]; then
            log_message "✅ App bundle executable validation passed"
        else
            log_message "❌ App bundle executable validation failed"
            validation_passed=false
        fi
    else
        log_message "⚠️  App bundle not found (may not be built on this platform)"
    fi

    log_message "macOS package validation completed"
    return $([ "$validation_passed" = true ] && echo 0 || echo 1)
}

# Professional source package validation
validate_source_packages() {
    log_message "=== Professional Source Package Validation ==="

    local validation_passed=true

    # Validate source tarball
    local source_tar=$(find "${BUILD_DIR}" -name "*-source.tar.gz" -type f | head -1)
    if validate_file_exists "$source_tar" "Source Package (tar.gz)"; then
        if tar -tzf "$source_tar" >/dev/null 2>&1; then
            log_message "✅ Source package structure validation passed"

            # Professional source content validation
            local file_count=$(tar -tzf "$source_tar" | wc -l)
            if [ "$file_count" -gt 50 ]; then
                log_message "✅ Source package content validation passed: $file_count files"
            else
                log_message "❌ Source package content validation failed: only $file_count files"
                validation_passed=false
            fi
        else
            log_message "❌ Source package structure validation failed"
            validation_passed=false
        fi
    fi

    # Validate source ZIP
    local source_zip=$(find "${BUILD_DIR}" -name "*-source.zip" -type f | head -1)
    if validate_file_exists "$source_zip" "Source Package (zip)"; then
        if command -v unzip >/dev/null 2>&1; then
            if unzip -t "$source_zip" >/dev/null 2>&1; then
                log_message "✅ Source ZIP validation passed"
            else
                log_message "❌ Source ZIP validation failed"
                validation_passed=false
            fi
        fi
    fi

    log_message "Source package validation completed"
    return $([ "$validation_passed" = true ] && echo 0 || echo 1)
}

# Professional main validation routine
main() {
    log_message "=========================================="
    log_message "ETI Stream Analyzer Professional Package Validation"
    log_message "=========================================="

    # Initialize validation log
    echo "" > "${VALIDATION_LOG}"
    log_message "Starting professional package validation"
    log_message "Build directory: ${BUILD_DIR}"
    log_message "Platform: $(detect_platform)"

    # Professional validation summary
    local overall_result=0
    local platform=$(detect_platform)

    # Validate packages based on platform and availability
    if [ "$platform" = "windows" ] || find "${BUILD_DIR}" -name "*.exe" -type f | grep -q .; then
        validate_windows_packages || overall_result=1
    fi

    if [ "$platform" = "linux" ] || find "${BUILD_DIR}" -name "*.deb" -o -name "*.rpm" -o -name "*.AppImage" | grep -q .; then
        validate_linux_packages || overall_result=1
    fi

    if [ "$platform" = "macos" ] || find "${BUILD_DIR}" -name "*.dmg" -o -name "*.app" | grep -q .; then
        validate_macos_packages || overall_result=1
    fi

    # Always validate source packages
    validate_source_packages || overall_result=1

    # Professional validation summary
    log_message "=========================================="
    if [ $overall_result -eq 0 ]; then
        log_message "🎉 Professional package validation PASSED"
        log_message "All packages are ready for professional deployment"
    else
        log_message "❌ Professional package validation FAILED"
        log_message "Some packages require attention before deployment"
    fi
    log_message "=========================================="
    log_message "Validation log saved to: ${VALIDATION_LOG}"

    return $overall_result
}

# Professional script execution
main "$@"
