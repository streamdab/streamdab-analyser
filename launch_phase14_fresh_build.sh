#!/bin/bash

echo "=============================================="
echo "🆕 PHASE 1.4 DAB-XPLORER - FRESH BUILD"
echo "=============================================="
echo "📊 Status: NEW BUILD READY FOR TESTING"
echo "🔧 Fixes Applied: All 4 Critical UI Issues"
echo "📁 Bangkok ETI: 5,001 frames processing"
echo "⚡ Build Time: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

cd /home/seksan/workspace/streamdab-analyzer

# Ensure ETI file is available
if [ ! -f "build/eti/bkk_20062022_141637.eti" ]; then
    echo "📋 Copying Bangkok ETI file to build directory..."
    mkdir -p build/eti
    cp eti/bkk_20062022_141637.eti build/eti/
    echo "✅ ETI file ready"
fi

echo ""
echo "🔧 CRITICAL FIXES INCLUDED IN THIS BUILD:"
echo "   ✅ Fix 1: White theme for all lists (no more black backgrounds)"
echo "   ✅ Fix 2: Frame Navigator shows ALL 5,001 frames (not just 10)"
echo "   ✅ Fix 3: FIG Analysis controls now functional"
echo "   ✅ Fix 4: Status bar shows correct Frame X/5001 format"
echo ""
echo "🚀 Launching FRESH Phase 1.4 DAB-Analyzer..."
echo ""
echo "🎛️  Expected Improvements:"
echo "   • Perfect readability with white backgrounds"
echo "   • Complete frame access (all 5,001 frames)"
echo "   • Working FIG analysis controls with real-time effects"
echo "   • Accurate frame counting in status bar"
echo ""
echo "📖 Test Instructions:"
echo "   1. Click File → Open ETI File"
echo "   2. Select eti/bkk_20062022_141637.eti"
echo "   3. Verify all 5,001 frames are accessible"
echo "   4. Check lists have white backgrounds (readable text)"
echo "   5. Test FIG analysis controls for real-time updates"
echo "   6. Confirm status bar shows 'Frame X/5001'"
echo ""
echo "✅ Launching fresh build..."

cd build
export DISPLAY=:0
./Phase14_DAB_XPlorer_Complete
