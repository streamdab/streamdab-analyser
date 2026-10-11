#!/usr/bin/env bash
# =============================================================================
# StreamDAB Analyser — macOS .app bundling + .dmg creation
# -----------------------------------------------------------------------------
# Production packaging for macOS. Requires a configured + built tree
# (cmake -B build -S . && cmake --build build) which produces the
# StreamDAB-Analyser.app bundle via the MACOSX_BUNDLE install() rules.
#
# Steps:
#   1. resolve the Qt 6 prefix from Homebrew (`brew --prefix qt@6`)
#   2. `cmake --install build --prefix dist`   (unstaged .app + CLI + docs)
#   3. `macdeployqt dist/StreamDAB-Analyser.app` (bundle the Qt frameworks)
#   4. `hdiutil create -volname "StreamDAB Analyser" -srcfolder dist ...`
#      -> a compressed UDZO .dmg, printed at the end.
#
# Qt frameworks are bundled by macdeployqt BEFORE any `cpack -G DragNDrop`
# would run — the CPack .dmg of an undeployed .app would carry no Qt libraries.
# You can run `cpack -G DragNDrop` inside build/ afterwards if you prefer the
# CPack route; this script is the canonical one-stop flow.
#
# Env overrides:
#   QT_DIR        Qt prefix (default: `brew --prefix qt@6`)
#   BUILD_DIR     build tree  (default: <repo>/build)
#   DIST_DIR      install/staging prefix (default: <repo>/dist)
#   BUILD_CONFIG  CMake --config value (default: Release)
#   DMG_OUT_DIR   where the .dmg lands (default: <repo>/build — NOT dist/, so
#                 the image is not nested inside its own source folder)
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
DIST_DIR="${DIST_DIR:-${ROOT}/dist}"
BUILD_CONFIG="${BUILD_CONFIG:-Release}"
DMG_OUT_DIR="${DMG_OUT_DIR:-${BUILD_DIR}}"

die() { echo "error: $*" >&2; exit 1; }

# --- 1) Qt 6 prefix -----------------------------------------------------------
QT_DIR="${QT_DIR:-$(brew --prefix qt@6 2>/dev/null || true)}"
[ -n "${QT_DIR}" ] || die \
    "brew --prefix qt@6 failed — install qt@6 (brew install qt@6) or set QT_DIR"
export PATH="${QT_DIR}/bin:${PATH}"

# --- version (single source: DABX_VERSION -> CPACK_PACKAGE_VERSION in cache) --
VERSION="$(sed -n 's/^CPACK_PACKAGE_VERSION:STRING=//p' \
    "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | head -1)"
if [ -z "${VERSION}" ]; then
    VERSION="$(sed -n 's/^set(CPACK_PACKAGE_VERSION "\(.*\)")$/\1/p' \
        "${BUILD_DIR}/CPackConfig.cmake" 2>/dev/null | head -1)"
fi
[ -n "${VERSION}" ] || die \
    "cannot resolve the package version — configure first: cmake -B ${BUILD_DIR} -S ${ROOT}"

APP_NAME="StreamDAB-Analyser.app"
DMG_NAME="StreamDAB-Analyser-${VERSION}.dmg"

echo "=== StreamDAB Analyser macOS bundle (v${VERSION}) ==="
echo "Qt dir   : ${QT_DIR}"
echo "Build dir: ${BUILD_DIR}"

# --- 2) staged install tree ---------------------------------------------------
echo "[1/3] cmake --install ${BUILD_DIR} --prefix ${DIST_DIR} (--config ${BUILD_CONFIG})"
cmake --install "${BUILD_DIR}" --prefix "${DIST_DIR}" --config "${BUILD_CONFIG}"

[ -d "${DIST_DIR}/${APP_NAME}" ] || die "expected ${DIST_DIR}/${APP_NAME} — check the install step"

# --- 3) Qt framework bundling -------------------------------------------------
APP_DIR="${DIST_DIR}/${APP_NAME}"
APP_EXE="${APP_DIR}/Contents/MacOS/StreamDAB-Analyser"
FRAMEWORKS_DIR="${APP_DIR}/Contents/Frameworks"

# The GUI links the Qt-Advanced-Docking-System dylib through @rpath, but the
# install step puts it in <prefix>/lib (outside the .app) and leaves the
# executable without an LC_RPATH. macdeployqt then cannot resolve it and
# silently ships a bundle that dies at start-up ("no LC_RPATH's found").
# Put the library inside the bundle and give the executable the Frameworks
# rpath *before* macdeployqt runs, so it is deployed and re-linked with the Qt
# frameworks.
mkdir -p "${FRAMEWORKS_DIR}"
for lib in "${DIST_DIR}"/lib/libqtadvanceddocking-qt6*.dylib; do
    [ -e "${lib}" ] && cp -a "${lib}" "${FRAMEWORKS_DIR}/"
done
ADS_LIB="$(ls "${FRAMEWORKS_DIR}"/libqtadvanceddocking-qt6.*.*.dylib 2>/dev/null | head -1 || true)"
[ -n "${ADS_LIB}" ] || die "Qt-ADS dylib (libqtadvanceddocking-qt6.<major>.<minor>.<patch>.dylib) not found in ${FRAMEWORKS_DIR} after copying from ${DIST_DIR}/lib — check the install step"
if ! otool -l "${APP_EXE}" | grep -q "path @executable_path/../Frameworks"; then
    install_name_tool -add_rpath "@executable_path/../Frameworks" "${APP_EXE}"
fi

# Homebrew splits Qt into one formula per module (qtbase, qtsvg, qtdeclarative,
# ...) and the plugins reference sibling frameworks through @rpath. Hand every
# Homebrew library directory to macdeployqt so it can resolve all of them.
BREW_PREFIX="$(brew --prefix 2>/dev/null || echo /opt/homebrew)"
LIBPATH_ARGS=()
for d in "${BREW_PREFIX}"/opt/*/lib; do
    [ -d "${d}" ] && LIBPATH_ARGS+=("-libpath=${d}")
done
LIBPATH_ARGS+=("-libpath=${DIST_DIR}/lib")

echo "[2/3] macdeployqt ${APP_DIR}"
macdeployqt "${APP_DIR}" -always-overwrite -verbose=1 \
    "-executable=${ADS_LIB}" "${LIBPATH_ARGS[@]}"

# Homebrew's split Qt keeps QtSvg, QtPdf, ... in separate prefixes that
# macdeployqt does not search for plugin dependencies, so a plugin can still
# point at a framework that never reached the bundle (QtSvg is needed for the
# SVG icons). Drop plugins the application does not use, then copy any
# framework a bundled binary still needs from Homebrew; the executable's
# Frameworks rpath resolves them for every image it loads.
rm -f "${APP_DIR}/Contents/PlugIns/platforminputcontexts/libqtvirtualkeyboardplugin.dylib" \
      "${APP_DIR}/Contents/PlugIns/imageformats/libqpdf.dylib"

# macdeployqt deploys the SVG *image-format* plugin only when QtSvg is already
# part of the bundle, and QtSvg is added by this script afterwards. Copy the
# plugin ourselves (QPixmap/QImage and style-sheet url(...svg) need it; the
# icon-engine plugin alone covers QIcon only).
if [ ! -e "${APP_DIR}/Contents/PlugIns/imageformats/libqsvg.dylib" ]; then
    QSVG_PLUGIN="$(find -L "${BREW_PREFIX}/opt/qtsvg" "${BREW_PREFIX}/Cellar/qtsvg" \
        -name libqsvg.dylib -path '*imageformats*' 2>/dev/null | head -1 || true)"
    [ -n "${QSVG_PLUGIN}" ] || die "Qt SVG image-format plugin (libqsvg.dylib) not found under ${BREW_PREFIX} — brew install qtsvg"
    mkdir -p "${APP_DIR}/Contents/PlugIns/imageformats"
    echo "bundling the SVG image-format plugin (from ${QSVG_PLUGIN})"
    cp "${QSVG_PLUGIN}" "${APP_DIR}/Contents/PlugIns/imageformats/"
fi

copy_missing_frameworks() {
    local changed file dep name src real
    for _ in 1 2 3 4 5; do
        changed=0
        while IFS= read -r -d '' file; do
            case "$(file -b "${file}")" in *Mach-O*) ;; *) continue ;; esac
            while IFS= read -r dep; do
                case "${dep}" in
                    @rpath/*.framework/*)
                        name="${dep#@rpath/}"; name="${name%%/*}"
                        [ -e "${FRAMEWORKS_DIR}/${name}" ] && continue
                        src="$(ls -d "${BREW_PREFIX}"/opt/*/lib/"${name}" 2>/dev/null | head -1 || true)"
                        [ -n "${src}" ] || continue
                        # Homebrew's "qt" prefix links the module frameworks in
                        # with symlinks. Copy the real framework: a copied
                        # symlink is dangling inside the bundle and breaks
                        # codesign.
                        real="$(cd "${src}" && pwd -P)"
                        echo "bundling missing framework ${name} (from ${real})"
                        cp -R "${real}" "${FRAMEWORKS_DIR}/${name}"
                        [ -e "${FRAMEWORKS_DIR}/${name}" ] || die "could not bundle ${name} from ${real}"
                        changed=1 ;;
                esac
            done < <(otool -L "${file}" 2>/dev/null | tail -n +2 | awk '{print $1}')
        done < <(find "${APP_DIR}/Contents" -type f -print0)
        [ "${changed}" -eq 1 ] || break
    done
}
copy_missing_frameworks

# Frameworks copied from Homebrew (QtSvg, ...) reference their Qt siblings by
# absolute Homebrew path. Point every such reference, and every Homebrew
# install name, at the copy inside the bundle (@rpath resolves through the
# executable's Frameworks rpath).
to_bundle_rpath() {
    local rel
    case "$1" in
        */lib/*) rel="${1#*/lib/}" ;;
        *)       rel="${1##*/}" ;;
    esac
    echo "@rpath/${rel}"
}

relink_homebrew_deps() {
    local file dep self rel
    while IFS= read -r -d '' file; do
        case "$(file -b "${file}")" in *Mach-O*) ;; *) continue ;; esac
        self="$(otool -D "${file}" 2>/dev/null | sed -n 2p)"
        case "${self}" in
            "${BREW_PREFIX}"/*)
                install_name_tool -id "$(to_bundle_rpath "${self}")" "${file}" ;;
        esac
        while IFS= read -r dep; do
            [ "${dep}" = "${self}" ] && continue
            case "${dep}" in
                "${BREW_PREFIX}"/*)
                    rel="$(to_bundle_rpath "${dep}")"
                    if [ -e "${FRAMEWORKS_DIR}/${rel#@rpath/}" ]; then
                        install_name_tool -change "${dep}" "${rel}" "${file}"
                    else
                        echo "warning: ${file#"${APP_DIR}"/} needs ${dep}, which is not in the bundle" >&2
                    fi ;;
            esac
        done < <(otool -L "${file}" 2>/dev/null | tail -n +2 | awk '{print $1}')
    done < <(find "${APP_DIR}/Contents" -type f -print0)
}
relink_homebrew_deps

# A dangling symlink makes codesign fail with a bare "No such file or
# directory"; list them so the cause is visible, and stop.
DANGLING="$(find "${APP_DIR}" -type l ! -exec test -e {} \; -print)"
if [ -n "${DANGLING}" ]; then
    echo "dangling symlinks in the bundle:" >&2
    echo "${DANGLING}" >&2
    die "bundle contains dangling symlinks"
fi

# macdeployqt rewrites binaries; make sure the final bundle carries a valid
# (ad-hoc) signature, otherwise Apple-silicon macOS refuses to run it.
codesign --force --deep --sign - "${APP_DIR}"
codesign --verify --deep --strict "${APP_DIR}"

# --- 4) compressed .dmg -------------------------------------------------------
mkdir -p "${DMG_OUT_DIR}"
echo "[3/3] hdiutil create -volname 'StreamDAB Analyser' -srcfolder ${DIST_DIR}"
echo "      -ov -format UDZO ${DMG_OUT_DIR}/${DMG_NAME}"
hdiutil create \
    -volname "StreamDAB Analyser" \
    -srcfolder "${DIST_DIR}" \
    -ov \
    -format UDZO \
    "${DMG_OUT_DIR}/${DMG_NAME}"

echo "=== macOS bundle complete ==="
echo "Artifact: ${DMG_OUT_DIR}/${DMG_NAME}"