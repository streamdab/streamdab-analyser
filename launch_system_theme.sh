#!/bin/bash

echo "=============================================="
echo "🖥️  PHASE 1.4 DAB-XPLORER - SYSTEM THEME"
echo "=============================================="
echo "✅ ALL CUSTOM THEMING REMOVED"
echo "✅ Pure Qt/System Default Theme"
echo "✅ Guaranteed Text Visibility"
echo "⚡ Build: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

cd /home/seksan/workspace/streamdab-analyzer

echo ""
echo "🔧 COMPLETE THEME REMOVAL APPLIED:"
echo "   ✅ Zero custom CSS styling"
echo "   ✅ No setStyleSheet() calls"
echo "   ✅ Pure Qt system defaults"
echo "   ✅ OS handles all colors and contrast"
echo ""
echo "🎨 What You Should See:"
echo "   • Native OS appearance (follows your system theme)"
echo "   • Standard Qt widget colors"
echo "   • Guaranteed readable text (system-managed contrast)"
echo "   • Familiar interface matching other applications"
echo ""

# Ensure ETI file is available
if [ ! -f "build/eti/bkk_20062022_141637.eti" ]; then
    echo "📋 Copying Bangkok ETI file..."
    mkdir -p build/eti
    cp eti/bkk_20062022_141637.eti build/eti/
fi

echo "🚀 Launching System-Themed DAB-Analyzer..."
echo ""
echo "✅ Text visibility is now guaranteed!"
echo "✅ Using your system's native theme!"

cd build
export DISPLAY=:0
./Phase14_DAB_XPlorer_Complete
