#!/bin/bash

echo "Testing ETI file loading functionality..."

cd /home/seksan/workspace/streamdab-analyzer/build

# Check if test file exists
if [ ! -f "../test_stream.eti" ]; then
    echo "❌ Test file test_stream.eti not found"
    exit 1
fi

echo "✅ Test file found: test_stream.eti"
echo "📊 File size: $(stat -c%s ../test_stream.eti) bytes"
echo "📊 Expected frames: $(($(stat -c%s ../test_stream.eti) / 6144))"

echo ""
echo "🚀 Starting application..."
echo "📝 Instructions for manual testing:"
echo "1. Application should start without freezing"
echo "2. Go to File -> Open"
echo "3. Select test_stream.eti"
echo "4. Check if frames appear in the frame list"
echo "5. Check if system messages show 'File loaded successfully!'"
echo ""
echo "Press Ctrl+C to exit when testing is complete"

./DABAnalyzer_Enhanced_FilteringBuild
