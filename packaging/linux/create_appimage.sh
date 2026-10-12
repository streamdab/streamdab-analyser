#!/usr/bin/env bash
# =============================================================================
# StreamDAB Analyser — Linux AppImage builder (linuxdeploy + plugin-qt)
# -----------------------------------------------------------------------------
# Builds a self-contained AppImage from the `cmake --install` prefix using the
# modern linuxdeploy toolchain. Requires a configured + built tree:
#
#   export QT_QPA_PLATFORM=offscreen
#   cmake -B build -S .
#   JOBS="$(bash scripts/build_jobs.sh)"        # bounded (never a bare -j)
#   cmake --build build --parallel "$JOBS"
#   ./packaging/linux/create_appimage.sh
#
# Output: build/StreamDAB-Analyser-<DABX_VERSION>-x86_64.AppImage
# (the staging AppDir is build/appimage-stage/).
#
# Prerequisites (tool binaries are NOT downloaded by this script):
#   - linuxdeploy            -> PATH, or $LINUXDEPLOY
#   - linuxdeploy-plugin-qt  -> PATH, or $LINUXDEPLOY_PLUGIN_QT
#   - (optional) appimagetool on PATH, or linuxdeploy-plugin-appimage, to produce
#     the final .AppImage; without one of them the script stops after leaving a
#     valid AppDir (build/appimage-stage/) and prints the manual command.
#
# Example install of the tools (one-time):
#   mkdir -p ~/bin
#   wget -qO ~/bin/linuxdeploy \
#     https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
#   wget -qO ~/bin/linuxdeploy-plugin-qt \
#     https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
#   chmod +x ~/bin/linuxdeploy ~/bin/linuxdeploy-plugin-qt
#   export PATH="$HOME/bin:$PATH"
#
# On headless CI, set the AppImage container environment first:
#   export APPIMAGE_EXTRACT_AND_RUN=1
#
# Env overrides:
#   BUILD_DIR   build tree (default: <repo>/build)
#   STAGE_DIR   AppDir staging prefix (default: <repo>/build/appimage-stage)
#   LINUXDEPLOY / LINUXDEPLOY_PLUGIN_QT  explicit tool paths
#   OUTPUT      AppImage file name (default: StreamDAB-Analyser-<ver>-x86_64.AppImage)
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
STAGE_DIR="${STAGE_DIR:-${BUILD_DIR}/appimage-stage}"
ARCH="$(uname -m)"                         # x86_64 / aarch64

die() { echo "error: $*" >&2; exit 1; }

# --- version (single source: DABX_VERSION -> CPACK_PACKAGE_VERSION in cache) --
resolve_version() {
    local v
    v="$(sed -n 's/^CPACK_PACKAGE_VERSION:STRING=//p' \
        "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | head -1)"
    if [ -z "${v}" ]; then
        v="$(sed -n 's/^set(CPACK_PACKAGE_VERSION "\(.*\)")$/\1/p' \
            "${BUILD_DIR}/CPackConfig.cmake" 2>/dev/null | head -1)"
    fi
    if [ -z "${v}" ]; then
        v="$(git -C "${ROOT}" describe --tags --abbrev=0 2>/dev/null | sed 's/^v//' || true)"
    fi
    [ -n "${v}" ] || die "cannot resolve the version — configure build/ first (cmake -B ${BUILD_DIR} -S ${ROOT})"
    echo "${v}"
}

# --- tool discovery ----------------------------------------------------------
LINUXDEPLOY="${LINUXDEPLOY:-$(command -v linuxdeploy || true)}"
LINUXDEPLOY_PLUGIN_QT="${LINUXDEPLOY_PLUGIN_QT:-$(command -v linuxdeploy-plugin-qt || true)}"
[ -n "${LINUXDEPLOY}" ] || die "linuxdeploy not found — set LINUXDEPLOY or add it to PATH (see header)"
[ -n "${LINUXDEPLOY_PLUGIN_QT}" ] || die "linuxdeploy-plugin-qt not found — set LINUXDEPLOY_PLUGIN_QT or add it to PATH (see header)"

VERSION="$(resolve_version)"
OUTPUT="${OUTPUT:-StreamDAB-Analyser-${VERSION}-${ARCH}.AppImage}"

echo "=== StreamDAB Analyser AppImage (v${VERSION}, ${ARCH}) ==="
echo "Staging : ${STAGE_DIR}"
echo "linuxdeploy: ${LINUXDEPLOY}"
echo "plugin-qt : ${LINUXDEPLOY_PLUGIN_QT}"

# --- 1) fresh AppDir staged from cmake --install -------------------------------
# The AppDir root holds usr/, so the install prefix is <stage>/usr: that gives
# usr/bin, usr/lib (Qt-ADS), usr/share/applications, usr/share/icons/hicolor/*,
# usr/share/mime/packages and usr/share/doc.
rm -rf "${STAGE_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${STAGE_DIR}/usr"

[ -x "${STAGE_DIR}/usr/bin/StreamDABAnalyser" ] || die \
    "staged AppDir has no usr/bin/StreamDABAnalyser — did the build succeed?"
[ -f "${STAGE_DIR}/usr/share/applications/eti-stream-analyser.desktop" ] || die \
    "staged AppDir has no desktop file — check install() rules"
ICON_PNG="${STAGE_DIR}/usr/share/icons/hicolor/256x256/apps/eti-stream-analyser.png"
[ -f "${ICON_PNG}" ] || die \
    "staged AppDir has no 256x256 PNG icon — check the icon install() rules"

# --- 2) Qt plugins linuxdeploy-plugin-qt must bundle ---------------------------
# svg + iconengines: the Qt-ADS button icons and the window icon are SVG;
# multimedia: QAudioSink backend; tls/imageformats: network and image loading.
export QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake || true)}"
[ -n "${QMAKE}" ] || die "qmake6/qmake not found — install qt6-base-dev (the Qt plugin needs it)"
if [ -z "${EXTRA_QT_PLUGINS:-}" ]; then
    QT_PLUGIN_DIR="$("${QMAKE}" -query QT_INSTALL_PLUGINS)"
    # svg needs the Qt SVG module (apt: libqt6svg6) - a hard requirement
    [ -f "${QT_PLUGIN_DIR}/iconengines/libqsvgicon.so" ] || \
        die "Qt SVG plugins not found in ${QT_PLUGIN_DIR} (install libqt6svg6)"
    plugins="svg;iconengines;imageformats"
    [ -d "${QT_PLUGIN_DIR}/tls" ] && plugins="${plugins};tls"
    export EXTRA_QT_PLUGINS="${plugins}"
fi
echo "Qt plugins to deploy: ${EXTRA_QT_PLUGINS}"

# --- 3) bundle Qt + libs into the AppDir with linuxdeploy ----------------------
"${LINUXDEPLOY}" \
    --appdir "${STAGE_DIR}" \
    --executable "${STAGE_DIR}/usr/bin/StreamDABAnalyser" \
    --desktop-file "${STAGE_DIR}/usr/share/applications/eti-stream-analyser.desktop" \
    --icon-file "${ICON_PNG}" \
    --plugin qt

# --- 3b) PNG icon at the AppDir root -------------------------------------------
# linuxdeploy picks the scalable SVG for the root icon and for .DirIcon. File
# managers and AppImage launchers read .DirIcon as a PNG (the AppImage spec),
# so replace both with the 256x256 PNG.
rm -f "${STAGE_DIR}/.DirIcon" "${STAGE_DIR}/eti-stream-analyser.svg" \
      "${STAGE_DIR}/eti-stream-analyser.png"
cp "${ICON_PNG}" "${STAGE_DIR}/eti-stream-analyser.png"
cp "${ICON_PNG}" "${STAGE_DIR}/.DirIcon"

# --- 4) produce the AppImage ---------------------------------------------------
if command -v appimagetool >/dev/null 2>&1; then
    echo "Creating ${OUTPUT} with appimagetool..."
    appimagetool "${STAGE_DIR}" "${OUTPUT}"
elif command -v linuxdeploy-plugin-appimage >/dev/null 2>&1; then
    echo "Creating ${OUTPUT} with linuxdeploy-plugin-appimage (OUTPUT=${OUTPUT})..."
    OUTPUT="${OUTPUT}" "${LINUXDEPLOY}" \
        --appdir "${STAGE_DIR}" \
        --output appimage
else
    echo "note: neither appimagetool nor linuxdeploy-plugin-appimage found."
    echo "A valid AppDir is ready at: ${STAGE_DIR}"
    echo "Create the AppImage manually, e.g.:"
    echo "  appimagetool ${STAGE_DIR} ${OUTPUT}"
    exit 0
fi

if [ -f "${OUTPUT}" ]; then
    echo "=== AppImage created successfully ==="
    echo "Artifact: ${OUTPUT}"
    echo "Size    : $(du -h "${OUTPUT}" | cut -f1)"
    echo "SHA256  : $(sha256sum "${OUTPUT}" | cut -d' ' -f1)"
else
    die "AppImage was not created (see tool output above)"
fi