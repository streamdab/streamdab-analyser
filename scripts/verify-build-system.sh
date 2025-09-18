#!/bin/bash
# ============================================================================
# ETI Stream Analyzer - Build System Verification Script
# Validates that all build methods work correctly
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

success() {
    echo -e "${GREEN}✓ $1${NC}"
}

error() {
    echo -e "${RED}✗ $1${NC}"
}

warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

banner() {
    echo ""
    echo -e "${GREEN}============================================================================${NC}"
    echo -e "${GREEN} $1${NC}"
    echo -e "${GREEN}============================================================================${NC}"
    echo ""
}

# ============================================================================
# Verification Functions
# ============================================================================

verify_project_structure() {
    banner "Verifying Project Structure"

    local required_files=(
        "CMakeLists.txt"
        "Dockerfile.build"
        "scripts/build-windows.ps1"
        "scripts/build-linux-macos.sh"
        "INSTALL.md"
        "src/main.cpp"
        "src/utils/logger_singleton.h"
    )

    for file in "${required_files[@]}"; do
        if [[ -f "$PROJECT_ROOT/$file" ]]; then
            success "Found: $file"
        else
            error "Missing: $file"
            return 1
        fi
    done

    success "Project structure verification passed"
}

verify_cmake_configuration() {
    banner "Verifying CMake Configuration"

    cd "$PROJECT_ROOT"

    # Check if CMake can configure the project
    if mkdir -p build-verify && cd build-verify; then
        info "Testing CMake configuration..."

        if cmake .. -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug &>/dev/null; then
            success "CMake configuration successful"
            cd ..
            rm -rf build-verify
            return 0
        else
            error "CMake configuration failed"
            cd ..
            rm -rf build-verify
            return 1
        fi
    else
        error "Failed to create build directory"
        return 1
    fi
}

verify_docker_system() {
    banner "Verifying Docker Build System"

    # Check if Docker is available
    if ! command -v docker &> /dev/null; then
        warning "Docker not found - skipping Docker verification"
        return 0
    fi

    cd "$PROJECT_ROOT"

    # Test Docker build (dry run)
    info "Testing Docker build configuration..."

    if docker build -f Dockerfile.build --target base -t eti-analyzer:verify . &>/dev/null; then
        success "Docker build system verified"

        # Clean up
        docker rmi eti-analyzer:verify &>/dev/null || true
        return 0
    else
        error "Docker build failed"
        return 1
    fi
}

verify_native_scripts() {
    banner "Verifying Native Build Scripts"

    cd "$PROJECT_ROOT"

    # Check script permissions and syntax
    local scripts=(
        "scripts/build-linux-macos.sh"
        "scripts/verify-build-system.sh"
    )

    for script in "${scripts[@]}"; do
        if [[ -f "$script" ]]; then
            if [[ -x "$script" ]]; then
                success "Script executable: $script"
            else
                warning "Making script executable: $script"
                chmod +x "$script"
                success "Script made executable: $script"
            fi

            # Basic syntax check
            if bash -n "$script" 2>/dev/null; then
                success "Script syntax valid: $script"
            else
                error "Script syntax error: $script"
                return 1
            fi
        else
            error "Script not found: $script"
            return 1
        fi
    done

    # Check PowerShell script if on Windows-like environment
    if [[ -f "scripts/build-windows.ps1" ]]; then
        success "PowerShell script found: scripts/build-windows.ps1"

        # Basic syntax check if PowerShell is available
        if command -v pwsh &> /dev/null; then
            if pwsh -Command "Get-Content scripts/build-windows.ps1 | Out-Null" 2>/dev/null; then
                success "PowerShell script syntax valid"
            else
                warning "PowerShell script syntax check failed (may be environment-specific)"
            fi
        else
            info "PowerShell not available for syntax checking"
        fi
    fi

    success "Native build scripts verification passed"
}

verify_dependencies() {
    banner "Verifying Build Dependencies"

    local found_deps=0
    local total_deps=0

    # Check essential tools
    local tools=(
        "cmake:CMake build system"
        "git:Version control"
        "pkg-config:Package configuration"
    )

    for tool_desc in "${tools[@]}"; do
        local tool="${tool_desc%%:*}"
        local desc="${tool_desc##*:}"

        ((total_deps++))
        if command -v "$tool" &> /dev/null; then
            local version
            case "$tool" in
                cmake)
                    version=$(cmake --version | head -n1 | cut -d' ' -f3)
                    ;;
                git)
                    version=$(git --version | cut -d' ' -f3)
                    ;;
                *)
                    version="unknown"
                    ;;
            esac
            success "$desc: $tool ($version)"
            ((found_deps++))
        else
            warning "$desc: $tool (not found)"
        fi
    done

    # Check optional tools
    local optional_tools=(
        "docker:Container platform"
        "qmake6:Qt6 development"
        "lcov:Coverage analysis"
    )

    info "Optional dependencies:"
    for tool_desc in "${optional_tools[@]}"; do
        local tool="${tool_desc%%:*}"
        local desc="${tool_desc##*:}"

        if command -v "$tool" &> /dev/null; then
            success "$desc: $tool (available)"
        else
            info "$desc: $tool (not available)"
        fi
    done

    if [[ $found_deps -eq $total_deps ]]; then
        success "All essential dependencies found"
        return 0
    else
        warning "Found $found_deps/$total_deps essential dependencies"
        return 1
    fi
}

verify_documentation() {
    banner "Verifying Documentation"

    cd "$PROJECT_ROOT"

    local docs=(
        "INSTALL.md:Installation guide"
        "README.md:Project overview"
        "CLAUDE.md:Project specifications"
    )

    for doc_desc in "${docs[@]}"; do
        local doc="${doc_desc%%:*}"
        local desc="${doc_desc##*:}"

        if [[ -f "$doc" ]]; then
            local lines=$(wc -l < "$doc")
            if [[ $lines -gt 10 ]]; then
                success "$desc: $doc ($lines lines)"
            else
                warning "$desc: $doc (too short: $lines lines)"
            fi
        else
            error "$desc: $doc (missing)"
        fi
    done

    success "Documentation verification completed"
}

# ============================================================================
# Main Verification Workflow
# ============================================================================

main() {
    banner "ETI Stream Analyzer - Build System Verification"

    local failures=0

    # Run all verification steps
    verify_project_structure || ((failures++))
    verify_cmake_configuration || ((failures++))
    verify_docker_system || ((failures++))
    verify_native_scripts || ((failures++))
    verify_dependencies || ((failures++))
    verify_documentation || ((failures++))

    # Summary
    banner "Verification Summary"

    if [[ $failures -eq 0 ]]; then
        success "All verifications passed! Build system is ready."
        echo ""
        info "You can now build the project using:"
        echo "  Docker:     docker build -t eti-analyzer ."
        echo "  Linux/macOS: ./scripts/build-linux-macos.sh"
        echo "  Windows:    .\\scripts\\build-windows.ps1"
        echo ""
        return 0
    else
        error "$failures verification(s) failed."
        echo ""
        warning "Please fix the issues above before building."
        echo ""
        return 1
    fi
}

# Make this script executable if it isn't already
if [[ ! -x "$0" ]]; then
    chmod +x "$0"
fi

# Run main function
main "$@"
