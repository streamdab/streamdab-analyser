#!/bin/bash
# StreamDAB Analyser - Phase 3 Qt6 GUI Launcher
# Build: Phase 3 Complete (Commit 5d8fa92)
# Date: 2025-10-18
# Status: All simulation code removed, real ETI integration complete

echo "=========================================="
echo "StreamDAB Analyser - Phase 3 Qt6 GUI"
echo "=========================================="
echo "Build: Phase 3 Complete (5d8fa92)"
echo "Features:"
echo "  ✅ Real ETI file processing"
echo "  ✅ Real FIG data service tree"
echo "  ✅ Real frame hex viewer"
echo "  ✅ LRU frame cache (100 frames)"
echo "  ✅ Thread-safe frame access"
echo ""
echo "Test Files Available:"
echo "  - eti/bkk_20062022_141637.eti (30 MB Thai DAB)"
echo "  - eti/BayerischerRundfunk-Warntag2024-2024-09-12/*.dab (59 MB German DAB)"
echo ""
echo "Launching application..."
echo "=========================================="
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_ROOT}/build"
./StreamDABAnalyser

echo ""
echo "Application closed."
