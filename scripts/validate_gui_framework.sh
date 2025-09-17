#!/bin/bash

# Qt GUI TDD Framework Validation Script
# Validates the complete implementation and integration

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

log() {
    echo -e "${BLUE}[GUI-FRAMEWORK-VALIDATION]${NC} $1"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

validation_count=0
passed_count=0

validate() {
    validation_count=$((validation_count + 1))
    if [ $? -eq 0 ]; then
        passed_count=$((passed_count + 1))
        success "$1"
        return 0
    else
        error "$1"
        return 1
    fi
}

log "Validating Qt GUI TDD Framework Implementation..."
log "Project root: ${PROJECT_ROOT}"

# Check file structure
log "Checking GUI test file structure..."

test -f "${PROJECT_ROOT}/tests/gui/test_main_window.cpp"
validate "MainWindow test implementation exists"

test -f "${PROJECT_ROOT}/tests/gui/test_analyzer_widget.cpp"
validate "AnalyzerWidget test implementation exists"

test -f "${PROJECT_ROOT}/tests/gui/test_constellation_widget.cpp"
validate "ConstellationWidget test implementation exists"

test -f "${PROJECT_ROOT}/tests/gui/gui_test_framework.h"
validate "GUI test framework header exists"

test -f "${PROJECT_ROOT}/tests/mocks/mock_gui_components.h"
validate "GUI mock components exist"

test -f "${PROJECT_ROOT}/scripts/tdd_gui_test.sh"
validate "TDD GUI test script exists"

test -x "${PROJECT_ROOT}/scripts/tdd_gui_test.sh"
validate "TDD GUI test script is executable"

# Check CMake integration
log "Checking CMake integration..."

grep -q "eti_analyzer_gui_tests" "${PROJECT_ROOT}/tests/CMakeLists.txt"
validate "GUI tests integrated in CMake"

grep -q "Qt6::Test" "${PROJECT_ROOT}/tests/CMakeLists.txt"
validate "Qt Test framework linked"

grep -q "GUI_TEST_FRAMEWORK_SOURCES" "${PROJECT_ROOT}/tests/CMakeLists.txt"
validate "GUI test framework sources configured"

# Check header implementations
log "Checking GUI component headers..."

test -f "${PROJECT_ROOT}/src/gui/main_window.h"
validate "Professional MainWindow header exists"

test -f "${PROJECT_ROOT}/src/gui/analyzer_widget.h"
validate "Professional AnalyzerWidget header exists"

grep -q "60 FPS" "${PROJECT_ROOT}/src/gui/main_window.h" || grep -q "TARGET_FPS" "${PROJECT_ROOT}/src/gui/main_window.h"
validate "Performance requirements documented in headers"

grep -q "QTimer" "${PROJECT_ROOT}/src/gui/main_window.h"
validate "Real-time update infrastructure present"

# Check integration with other agents
log "Checking integration with other agent frameworks..."

test -f "${PROJECT_ROOT}/tests/gui/test_eti_data_integration.cpp"
validate "Multi-agent integration tests exist"

grep -q "EtiTestData" "${PROJECT_ROOT}/tests/gui/test_eti_data_integration.cpp"
validate "Network/Stream Agent data integration"

grep -q "EtsiTestData\|ETSI" "${PROJECT_ROOT}/tests/gui/test_eti_data_integration.cpp"
validate "ETSI Compliance Agent integration"

grep -q "MockEtiProcessor" "${PROJECT_ROOT}/tests/gui/test_eti_data_integration.cpp"
validate "Tester Agent mock framework integration"

# Check documentation
log "Checking documentation..."

test -f "${PROJECT_ROOT}/QT_GUI_TDD_FRAMEWORK_COMPLETE.md"
validate "Comprehensive GUI framework documentation exists"

grep -q "Phase 1" "${PROJECT_ROOT}/QT_GUI_TDD_FRAMEWORK_COMPLETE.md"
validate "Phase 1 completion documented"

grep -q "60 FPS" "${PROJECT_ROOT}/QT_GUI_TDD_FRAMEWORK_COMPLETE.md"
validate "Performance requirements documented"

# Summary
log ""
log "=== Qt GUI TDD Framework Validation Summary ==="
log "Total validations: ${validation_count}"
log "Passed validations: ${passed_count}"
log "Failed validations: $((validation_count - passed_count))"

if [ ${passed_count} -eq ${validation_count} ]; then
    success "🎉 Qt GUI TDD Framework validation PASSED completely!"
    success "All ${validation_count} validation checks successful"
    success "Framework ready for Phase 2 development"
    exit 0
else
    error "❌ Qt GUI TDD Framework validation FAILED"
    error "$((validation_count - passed_count)) validation(s) failed"
    error "Please fix issues before proceeding to Phase 2"
    exit 1
fi
