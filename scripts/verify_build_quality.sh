#!/bin/bash
# Build Quality Verification Script
# PM-Agent Policy Enforcement: ZERO ERROR, ZERO WARNING BUILDS
# Usage: ./scripts/verify_build_quality.sh

set -e

echo "🏗️  ETI Stream Analyser - Build Quality Verification"
echo "📋 Policy: ZERO ERROR, ZERO WARNING BUILDS"
echo "=================================================="

# Navigate to project root
cd "$(dirname "$0")/.."

# Ensure build directory exists
if [ ! -d "build" ]; then
    echo "❌ ERROR: Build directory not found. Run cmake first."
    exit 1
fi

cd build

echo "🧹 Step 1: Clean build (removing cached artifacts)..."
make clean > /dev/null 2>&1

echo "🔨 Step 2: Full build with warning detection..."
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "${SCRIPT_DIR}/build_jobs.sh"
VQ_JOBS=$(streamdab_build_jobs)
export CMAKE_BUILD_PARALLEL_LEVEL="${VQ_JOBS}"
BUILD_OUTPUT=$(make -j"${VQ_JOBS}" 2>&1)
BUILD_EXIT_CODE=$?

echo "📊 Step 3: Analyzing build results..."

# Count warnings
WARNING_COUNT=$(echo "$BUILD_OUTPUT" | grep -i warning | wc -l)

# Count errors
ERROR_COUNT=$(echo "$BUILD_OUTPUT" | grep -i error | wc -l)

# Display results
echo ""
echo "📈 BUILD QUALITY REPORT"
echo "======================="
echo "Exit Code: $BUILD_EXIT_CODE"
echo "Errors: $ERROR_COUNT"
echo "Warnings: $WARNING_COUNT"
echo ""

# Policy enforcement
if [ $BUILD_EXIT_CODE -ne 0 ]; then
    echo "❌ POLICY VIOLATION: Build failed with exit code $BUILD_EXIT_CODE"
    echo "🚨 REQUIRED ACTION: Fix all compilation errors immediately"
    exit 1
fi

if [ $ERROR_COUNT -gt 0 ]; then
    echo "❌ POLICY VIOLATION: Found $ERROR_COUNT compilation errors"
    echo "🚨 REQUIRED ACTION: Fix all errors before committing"
    exit 1
fi

if [ $WARNING_COUNT -gt 0 ]; then
    echo "❌ POLICY VIOLATION: Found $WARNING_COUNT compilation warnings"
    echo "🚨 REQUIRED ACTION: Fix all warnings before committing"
    echo ""
    echo "📝 Warning Details:"
    echo "$BUILD_OUTPUT" | grep -i warning | head -10
    if [ $WARNING_COUNT -gt 10 ]; then
        echo "... and $((WARNING_COUNT - 10)) more warnings"
    fi
    exit 1
fi

# Success
echo "✅ BUILD QUALITY: EXCELLENT"
echo "🏆 ZERO ERRORS, ZERO WARNINGS achieved"
echo "💯 Professional build standards maintained"
echo ""
echo "📋 Verified Targets:"
echo "$BUILD_OUTPUT" | grep "Built target" | tail -5
echo ""
echo "🎉 Ready for commit/deployment"
