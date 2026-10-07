#!/bin/bash

# TDD Development Workflow Script for ETI Stream Analyser
# Provides easy-to-use commands for TDD Red-Green-Refactor cycle

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${SCRIPT_DIR}")"
BUILD_DIR="${PROJECT_ROOT}/build"

# Build parallelism: bounded by CPU *and* RAM (see scripts/build_jobs.sh).
# Override with JOBS=<n> or CMAKE_BUILD_PARALLEL_LEVEL=<n>.
. "${SCRIPT_DIR}/build_jobs.sh"
JOBS=$(streamdab_build_jobs)
export CMAKE_BUILD_PARALLEL_LEVEL="${JOBS}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# TDD Status indicators
TDD_RED="🔴"
TDD_GREEN="🟢"
TDD_BLUE="🔵"

# Print functions
print_info() {
    echo -e "${BLUE}[TDD INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[TDD SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[TDD WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[TDD ERROR]${NC} $1"
}

# Help function
show_help() {
    cat << EOF
ETI Stream Analyser TDD Development Workflow

Usage: $0 <command> [options]

COMMANDS:
    setup               Initialize TDD environment
    red                 🔴 TDD RED: Run failing tests
    green               🟢 TDD GREEN: Run passing tests
    refactor           🔵 TDD REFACTOR: Code optimization
    cycle              🔄 Complete Red-Green-Refactor cycle
    test               Run all tests
    coverage           Generate coverage report
    quality-gate       Run complete quality validation
    clean              Clean build and start fresh
    watch              Watch for file changes and auto-test

TDD WORKFLOW:
    $0 setup           # First time setup
    $0 red             # Write failing test
    $0 green           # Make test pass
    $0 refactor        # Optimize code
    $0 quality-gate    # Final validation

EXAMPLES:
    $0 setup           # Initialize TDD environment
    $0 cycle           # Run complete TDD workflow
    $0 watch           # Continuous testing mode

EOF
}

# Check if build directory exists
check_build_environment() {
    if [[ ! -d "${BUILD_DIR}" ]]; then
        print_error "Build directory not found: ${BUILD_DIR}"
        print_info "Run './scripts/build.sh --tdd' first to setup TDD environment"
        exit 1
    fi

    if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
        print_error "CMake not configured in build directory"
        print_info "Run './scripts/build.sh --tdd' first to setup TDD environment"
        exit 1
    fi
}

# Setup TDD environment
setup_tdd() {
    print_info "${TDD_BLUE} Setting up TDD development environment..."

    # Run TDD build to ensure everything is configured
    "${SCRIPT_DIR}/build.sh" --tdd --clean

    print_success "${TDD_GREEN} TDD environment setup complete!"
    print_info "You can now use: $0 red|green|refactor|cycle"
}

# TDD RED phase
tdd_red() {
    print_info "${TDD_RED} TDD RED Phase: Running tests expecting failures..."
    check_build_environment

    cd "${BUILD_DIR}"
    if make tdd_red_phase 2>/dev/null || cmake --build . --target tdd_red_phase; then
        print_info "${TDD_RED} RED phase complete - some tests failed (this is expected)"
        print_info "Now write minimal code to make tests pass and run: $0 green"
    else
        print_error "${TDD_RED} Failed to run RED phase"
        exit 1
    fi
}

# TDD GREEN phase
tdd_green() {
    print_info "${TDD_GREEN} TDD GREEN Phase: Running tests expecting success..."
    check_build_environment

    cd "${BUILD_DIR}"
    if make tdd_green_phase 2>/dev/null || cmake --build . --target tdd_green_phase; then
        print_success "${TDD_GREEN} GREEN phase complete - all tests passed!"
        print_info "Now optimize your code and run: $0 refactor"
    else
        print_error "${TDD_RED} GREEN phase failed - tests are not passing"
        print_info "Fix your implementation and try again"
        exit 1
    fi
}

# TDD REFACTOR phase
tdd_refactor() {
    print_info "${TDD_BLUE} TDD REFACTOR Phase: Code optimization while maintaining tests..."
    check_build_environment

    cd "${BUILD_DIR}"

    # First rebuild to ensure changes are compiled
    if cmake --build . --config Debug --parallel "${JOBS}"; then
        print_info "Code rebuilt successfully"
    else
        print_error "Build failed during refactor phase"
        exit 1
    fi

    # Run tests to ensure refactoring didn't break anything
    if make tdd_green_phase 2>/dev/null || cmake --build . --target tdd_green_phase; then
        print_success "${TDD_BLUE} REFACTOR phase complete - tests still passing after optimization!"
        print_info "TDD cycle complete. Run: $0 quality-gate for final validation"
    else
        print_error "${TDD_RED} REFACTOR phase failed - refactoring broke tests"
        print_info "Revert changes and try a smaller refactoring step"
        exit 1
    fi
}

# Complete TDD cycle
tdd_cycle() {
    print_info "${TDD_RED}${TDD_GREEN}${TDD_BLUE} Running complete TDD Red-Green-Refactor cycle..."

    tdd_red
    echo ""
    read -p "Press Enter after writing code to make tests pass..."

    tdd_green
    echo ""
    read -p "Press Enter after optimizing/refactoring your code..."

    tdd_refactor

    print_success "${TDD_RED}${TDD_GREEN}${TDD_BLUE} Complete TDD cycle finished!"
}

# Run all tests
run_tests() {
    print_info "Running all TDD tests..."
    check_build_environment

    cd "${BUILD_DIR}"
    if make run_tests 2>/dev/null || ctest --output-on-failure --verbose; then
        print_success "All tests passed!"
    else
        print_error "Some tests failed!"
        exit 1
    fi
}

# Generate coverage report
generate_coverage() {
    print_info "Generating code coverage report..."
    check_build_environment

    cd "${BUILD_DIR}"
    if make tdd_coverage_generate 2>/dev/null || cmake --build . --target coverage; then
        print_success "Coverage report generated"
        print_info "Open ${BUILD_DIR}/coverage_html/index.html to view coverage"

        # Check coverage requirements
        if make tdd_coverage_check 2>/dev/null || cmake --build . --target coverage-check; then
            print_success "${TDD_GREEN} Coverage requirement met!"
        else
            print_error "${TDD_RED} Coverage requirement not met!"
            exit 1
        fi
    else
        print_error "Failed to generate coverage report"
        exit 1
    fi
}

# Quality gate validation
quality_gate() {
    print_info "${TDD_BLUE} Running TDD quality gate validation..."
    check_build_environment

    cd "${BUILD_DIR}"
    if make tdd_quality_gate 2>/dev/null || cmake --build . --target quality-gate; then
        print_success "${TDD_GREEN} Quality gate passed!"
        print_info "✅ All tests pass"
        print_info "✅ Coverage requirements met"
        print_info "✅ Ready for commit/merge"
    else
        print_error "${TDD_RED} Quality gate failed!"
        print_info "Fix issues before committing code"
        exit 1
    fi
}

# Clean build
clean_build() {
    print_info "Cleaning TDD build environment..."
    rm -rf "${BUILD_DIR}"
    print_success "Build directory cleaned"
    print_info "Run '$0 setup' to reinitialize TDD environment"
}

# Watch mode for continuous testing
watch_mode() {
    print_info "Starting TDD watch mode (continuous testing)..."
    check_build_environment

    if ! command -v inotifywait &> /dev/null && ! command -v fswatch &> /dev/null; then
        print_warning "File watching tools not found"
        print_info "Install inotify-tools (Linux) or fswatch (macOS) for watch mode"
        exit 1
    fi

    print_info "Watching for changes in src/ and tests/ directories..."
    print_info "Press Ctrl+C to stop watching"

    # Function to run tests
    run_watch_tests() {
        echo ""
        print_info "${TDD_BLUE} File changed, running tests..."
        cd "${BUILD_DIR}"
        cmake --build . --config Debug --parallel "${JOBS}"
        if ctest --output-on-failure; then
            print_success "${TDD_GREEN} Tests passed!"
        else
            print_error "${TDD_RED} Tests failed!"
        fi
        echo ""
    }

    # Watch for file changes
    if command -v inotifywait &> /dev/null; then
        # Linux
        while inotifywait -r -e modify,create,delete "${PROJECT_ROOT}/src" "${PROJECT_ROOT}/tests" 2>/dev/null; do
            run_watch_tests
        done
    elif command -v fswatch &> /dev/null; then
        # macOS
        fswatch -o "${PROJECT_ROOT}/src" "${PROJECT_ROOT}/tests" | while read f; do
            run_watch_tests
        done
    fi
}

# Main execution
main() {
    case "${1:-}" in
        setup)
            setup_tdd
            ;;
        red)
            tdd_red
            ;;
        green)
            tdd_green
            ;;
        refactor)
            tdd_refactor
            ;;
        cycle)
            tdd_cycle
            ;;
        test)
            run_tests
            ;;
        coverage)
            generate_coverage
            ;;
        quality-gate)
            quality_gate
            ;;
        clean)
            clean_build
            ;;
        watch)
            watch_mode
            ;;
        -h|--help|help)
            show_help
            ;;
        "")
            print_error "No command specified"
            show_help
            exit 1
            ;;
        *)
            print_error "Unknown command: $1"
            show_help
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
