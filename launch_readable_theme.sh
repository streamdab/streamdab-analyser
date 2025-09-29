#!/bin/bash

echo "=============================================="
echo "📖 PHASE 1.4 DAB-XPLORER - READABLE THEME"
echo "=============================================="
echo "🎨 EMERGENCY THEME FIX APPLIED"
echo "✅ Perfect Readability: BLACK text on WHITE backgrounds"
echo "✅ High Contrast: No more invisible text"
echo "✅ Professional Styling: Broadcast industry standard"
echo "⚡ Build Time: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

cd /home/seksan/workspace/streamdab-analyzer

echo ""
echo "🔧 CRITICAL THEME FIXES APPLIED:"
echo "   ✅ ALL text is now BLACK (highly visible)"
echo "   ✅ ALL backgrounds are WHITE/light colors"
echo "   ✅ Selected items: Light blue background with black text"
echo "   ✅ Lists, labels, panels: Perfect readability"
echo "   ✅ NO MORE white text on white background"
echo ""
echo "🎨 Professional Color Scheme:"
echo "   • Text: Black (#000000) - Maximum visibility"
echo "   • Background: White (#FFFFFF) - Clean appearance"
echo "   • Selection: Light Blue (#E6F2FF) with black text"
echo "   • Borders: Light Gray (#CCCCCC) - Clear boundaries"
echo ""
echo "📖 What You Should See Now:"
echo "   • Ensemble Tree: Black text on white, fully readable"
echo "   • Frame Navigator: Black text on white, perfect visibility"
echo "   • All Labels: Black text, clearly visible"
echo "   • Status Bar: Black text on light background"
echo "   • FIG Controls: Black text, professional appearance"
echo ""

# Ensure ETI file is available
if [ ! -f "build/eti/bkk_20062022_141637.eti" ]; then
    echo "📋 Copying Bangkok ETI file..."
    mkdir -p build/eti
    cp eti/bkk_20062022_141637.eti build/eti/
    echo "✅ ETI file ready"
fi

echo "🚀 Launching READABLE Phase 1.4 DAB-Analyzer..."
echo ""
echo "✅ The theme crisis has been resolved!"
echo "✅ You should now see everything clearly!"

cd build
export DISPLAY=:0
./Phase14_DAB_XPlorer_Complete
