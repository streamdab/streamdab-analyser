#!/bin/bash
# ETI Stream Analyser - Professional Linux AppImage Creation Script
# Universal Linux distribution for all major distros

set -e

# Professional AppImage configuration
APP_NAME="ETI Stream Analyser"
APP_VERSION="${PROJECT_VERSION:-1.0.0}"
APP_DIR="ETIStreamAnalyser.AppDir"
APPIMAGE_NAME="ETI_Stream_Analyser-${APP_VERSION}-x86_64.AppImage"

echo "=== Creating Professional ETI Stream Analyser AppImage ==="
echo "Version: ${APP_VERSION}"
echo "Target: Universal Linux Distribution"

# Clean previous builds
rm -rf "${APP_DIR}"
rm -f "${APPIMAGE_NAME}"

# Create professional AppDir structure
mkdir -p "${APP_DIR}/usr/bin"
mkdir -p "${APP_DIR}/usr/lib"
mkdir -p "${APP_DIR}/usr/share/applications"
mkdir -p "${APP_DIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${APP_DIR}/usr/share/mime/packages"
mkdir -p "${APP_DIR}/usr/share/doc/eti-stream-analyser"

# Copy main application
echo "Installing application executable..."
cp "${CMAKE_BINARY_DIR}/ETIStreamAnalyser" "${APP_DIR}/usr/bin/"
chmod +x "${APP_DIR}/usr/bin/ETIStreamAnalyser"

# Professional library bundling with linuxdeploy
echo "Bundling required libraries..."
if command -v linuxdeploy >/dev/null 2>&1; then
    linuxdeploy --appdir "${APP_DIR}" --executable "${APP_DIR}/usr/bin/ETIStreamAnalyser"
else
    echo "Warning: linuxdeploy not found. Manual library copying required."
    # Manual library detection and copying
    ldd "${APP_DIR}/usr/bin/ETIStreamAnalyser" | grep "=>" | awk '{print $3}' | while read lib; do
        if [[ "$lib" == /usr/lib* ]] || [[ "$lib" == /lib* ]]; then
            continue  # Skip system libraries
        fi
        cp "$lib" "${APP_DIR}/usr/lib/" 2>/dev/null || true
    done
fi

# Professional desktop integration
echo "Creating professional desktop entry..."
cat > "${APP_DIR}/usr/share/applications/eti-stream-analyser.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=ETI Stream Analyser
GenericName=Professional ETI/DAB Stream Analysis
Comment=Professional ETI and DAB/DAB+ stream analyser for broadcast engineers
Keywords=ETI;DAB;DAB+;Broadcast;Audio;Stream;Analysis;ETSI;Professional;
Icon=eti-stream-analyser
Exec=ETIStreamAnalyser %F
Terminal=false
MimeType=application/x-eti-stream;application/x-eti-ni;application/x-eti-li;
Categories=AudioVideo;Audio;Professional;Engineering;
StartupNotify=true
StartupWMClass=ETIStreamAnalyser
X-AppImage-Version=${APP_VERSION}
X-AppImage-Comment=Professional broadcast engineering tool
EOF

# Create AppImage desktop file (required by AppImage spec)
cp "${APP_DIR}/usr/share/applications/eti-stream-analyser.desktop" "${APP_DIR}/"

# Professional MIME type support
echo "Registering professional broadcast file types..."
cat > "${APP_DIR}/usr/share/mime/packages/eti-stream-analyser.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
    <mime-type type="application/x-eti-stream">
        <comment>ETI Stream File</comment>
        <comment xml:lang="en">ETI (Ensemble Transport Interface) Stream File</comment>
        <icon name="eti-stream-analyser"/>
        <glob pattern="*.eti"/>
        <magic priority="50">
            <match value="ETI" type="string" offset="0"/>
        </magic>
    </mime-type>
    <mime-type type="application/x-eti-ni">
        <comment>ETI-NI Stream File</comment>
        <icon name="eti-stream-analyser"/>
        <glob pattern="*.eti"/>
        <glob pattern="*.etini"/>
    </mime-type>
    <mime-type type="application/x-eti-li">
        <comment>ETI-LI Stream File</comment>
        <icon name="eti-stream-analyser"/>
        <glob pattern="*.eti"/>
        <glob pattern="*.etili"/>
    </mime-type>
</mime-info>
EOF

# Professional application icon
echo "Installing professional application icon..."
if [ -f "${CMAKE_SOURCE_DIR}/resources/icons/eti-analyser-256.png" ]; then
    cp "${CMAKE_SOURCE_DIR}/resources/icons/eti-analyser-256.png" "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/eti-stream-analyser.png"
    cp "${CMAKE_SOURCE_DIR}/resources/icons/eti-analyser-256.png" "${APP_DIR}/eti-stream-analyser.png"
else
    echo "Warning: Application icon not found. Using placeholder."
    # Create a simple placeholder icon
    convert -size 256x256 xc:lightblue -pointsize 24 -fill black -gravity center -annotate +0+0 "ETI\nAnalyser" "${APP_DIR}/eti-stream-analyser.png" 2>/dev/null || true
fi

# Professional AppRun script
echo "Creating professional AppRun launcher..."
cat > "${APP_DIR}/AppRun" << 'EOF'
#!/bin/bash
# ETI Stream Analyser Professional AppImage Launcher

HERE="$(dirname "$(readlink -f "${0}")")"

# Professional environment setup
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export PATH="${HERE}/usr/bin:${PATH}"
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS}"

# Professional Qt application settings
export QT_PLUGIN_PATH="${HERE}/usr/lib/qt6/plugins:${QT_PLUGIN_PATH}"
export QML2_IMPORT_PATH="${HERE}/usr/lib/qt6/qml:${QML2_IMPORT_PATH}"

# Professional broadcast engineering environment
export ETI_ANALYSER_PORTABLE=1
export ETI_ANALYSER_CONFIG_DIR="${HOME}/.config/eti-stream-analyser"

# Create professional config directory
mkdir -p "${ETI_ANALYSER_CONFIG_DIR}"

# Launch professional application
exec "${HERE}/usr/bin/ETIStreamAnalyser" "$@"
EOF

chmod +x "${APP_DIR}/AppRun"

# Professional documentation
echo "Installing professional documentation..."
if [ -f "${CMAKE_SOURCE_DIR}/README.md" ]; then
    cp "${CMAKE_SOURCE_DIR}/README.md" "${APP_DIR}/usr/share/doc/eti-stream-analyser/"
fi
if [ -f "${CMAKE_SOURCE_DIR}/LICENSE" ]; then
    cp "${CMAKE_SOURCE_DIR}/LICENSE" "${APP_DIR}/usr/share/doc/eti-stream-analyser/"
fi

# Create professional AppImage
echo "Creating professional AppImage package..."
if command -v appimagetool >/dev/null 2>&1; then
    # Use official AppImage tool
    appimagetool "${APP_DIR}" "${APPIMAGE_NAME}"
else
    echo "Warning: appimagetool not found. Please install AppImageKit."
    echo "AppDir created at: ${APP_DIR}"
    echo "Manual AppImage creation required."
    exit 1
fi

# Professional package validation
if [ -f "${APPIMAGE_NAME}" ]; then
    echo "=== Professional AppImage Created Successfully ==="
    echo "File: ${APPIMAGE_NAME}"
    echo "Size: $(du -h "${APPIMAGE_NAME}" | cut -f1)"
    echo "Executable: chmod +x ${APPIMAGE_NAME} && ./${APPIMAGE_NAME}"

    # Professional integrity check
    if command -v sha256sum >/dev/null 2>&1; then
        echo "SHA256: $(sha256sum "${APPIMAGE_NAME}" | cut -d' ' -f1)"
    fi

    echo "Ready for professional broadcast engineering deployment."
else
    echo "Error: AppImage creation failed."
    exit 1
fi
