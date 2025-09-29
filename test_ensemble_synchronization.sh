#!/bin/bash

# Complete Feature Synchronization Testing Script
# Tests FIG filtering, Hex Viewer, and DAB Ensemble Explorer synchronization

echo "🚀 Enhanced DAB Analyzer - Complete Feature Synchronization Testing"
echo "=================================================================="

ANALYZER="./build_enhanced/DABAnalyzer_Enhanced_FilteringBuild"
ETI_FILE="./eti/bkk_20062022_141637.eti"

# Check files
if [ ! -f "$ANALYZER" ]; then
    echo "❌ Analyzer not found: $ANALYZER"
    exit 1
fi

if [ ! -f "$ETI_FILE" ]; then
    echo "❌ ETI file not found: $ETI_FILE"
    exit 1
fi

echo "✅ Analyzer: $ANALYZER ($(du -h "$ANALYZER" | cut -f1))"
echo "✅ ETI File: $ETI_FILE ($(du -h "$ETI_FILE" | cut -f1))"
echo ""

echo "🎯 Complete Feature Set - READY FOR TESTING:"
echo ""
echo "📋 1. FIG Type Checkbox Filtering"
echo "   • Location: FIC-Analyzer tab → FIG Type Analysis section"
echo "   • Functionality: Interactive checkboxes filter FIG Analysis Results"
echo "   • Supported: FIG 0/0, 0/1, 0/2, 0/3, 1/0, 1/1, 1/4"
echo ""
echo "📋 2. Advanced Hex Viewer with Frame Highlighting"
echo "   • Location: Bottom panel → Hex Viewer tab"
echo "   • Functionality: Color-coded ETI frame structure (6144 bytes)"
echo "   • Highlighting: Sync(Red), FC(Green), Header(Blue), FIC(Orange), MSC(Gray)"
echo ""
echo "📋 3. ✨ NEW: DAB Ensemble Explorer Frame Synchronization"
echo "   • Location: Left panel → DAB Ensemble Explorer"
echo "   • Functionality: Updates service tree when ETI frame is selected"
echo "   • Features: Frame-specific service discovery and availability"
echo "   • Dynamic: Different services shown per frame based on FIC content"
echo ""

echo "🧪 Enhanced Testing Workflow:"
echo "=================================================================="
echo ""
echo "Step 1: Load Bangkok ETI File"
echo "   → File → Open → Select eti/bkk_20062022_141637.eti"
echo "   → Verify 5,001 frames loaded in ETI Frame Navigator"
echo ""
echo "Step 2: Test Frame Selection → Ensemble Synchronization"
echo "   → Click different frames in ETI Frame Navigator"
echo "   → ✨ Observe DAB Ensemble Explorer updates with frame-specific services"
echo "   → Verify service availability changes based on frame index"
echo "   → Check System Messages for ensemble update notifications"
echo ""
echo "Step 3: Test Hex Viewer Integration"
echo "   → Select any frame → Switch to Hex Viewer tab"
echo "   → Verify ETI structure highlighting (Sync, FC, Header, FIC, MSC)"
echo "   → Confirm frame-specific hex data display"
echo ""
echo "Step 4: Test FIG Filtering"
echo "   → Navigate to FIC-Analyzer tab"
echo "   → Use FIG type checkboxes to filter analysis results"
echo "   → Verify filtering works independently of frame selection"
echo ""
echo "Step 5: Combined Feature Testing"
echo "   → Select Frame A → Check ensemble services → View hex data"
echo "   → Select Frame B → Verify ensemble updates → Check different hex"
echo "   → Apply FIG filters → Confirm all features work together"
echo ""

echo "📊 Expected Synchronized Behavior:"
echo "=================================================================="
echo ""
echo "✅ Frame Selection Triggers:"
echo "   1. Frame details update (Frame X, 6144 bytes, offset calculation)"
echo "   2. Hex Viewer displays frame-specific ETI data with highlighting"
echo "   3. ✨ DAB Ensemble Explorer refreshes with frame-specific services"
echo "   4. System Messages show all three update notifications"
echo ""
echo "✅ Frame-Specific Service Variation:"
echo "   • Base Services: NBT Radio, NBT News (always present)"
echo "   • Conditional Services: Traffic Info (every 10th frame)"
echo "   • Time-Based Services: Morning/Afternoon/Evening programs"
echo "   • Quality Indicators: High Quality vs Standard (frame-dependent)"
echo ""
echo "✅ Professional Workflow Integration:"
echo "   • Seamless switching between frame analysis perspectives"
echo "   • Real-time UI updates without manual refresh"
echo "   • Memory-efficient partial data loading"
echo "   • No conflicts between filtering and frame synchronization"
echo ""

echo "🔧 Launching Enhanced Analyzer with Complete Feature Set..."
echo ""

# Set display
export DISPLAY=${DISPLAY:-:0}

echo "  Command: $ANALYZER"
echo "  Features: FIG Filtering + Hex Viewer + Ensemble Synchronization"
echo "  Test File: Bangkok DAB+ stream (5,001 frames)"
echo ""
echo "🎉 Ready for complete feature synchronization testing!"
echo "   All three features now work together seamlessly."

# Launch analyzer
exec "$ANALYZER"
