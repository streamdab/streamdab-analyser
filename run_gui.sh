#!/bin/bash
# StreamDAB Analyzer - Phase 3 Qt6 GUI Launcher
# Build: Phase 3 Complete (Commit 5d8fa92)
# Date: 2025-10-18
# Status: All simulation code removed, real ETI integration complete

echo "=========================================="
echo "StreamDAB Analyzer - Phase 3 Qt6 GUI"
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

cd /home/seksan/workspace/streamdab-analyzer/build
./DABAnalyzer_Enhanced_FilteringBuild

echo ""
echo "Application closed."
