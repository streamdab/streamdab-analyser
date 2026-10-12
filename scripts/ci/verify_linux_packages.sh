#!/usr/bin/env bash
# Verify the Linux packages before anyone downloads them.
#
#   verify_linux_packages.sh deb      <file.deb>
#   verify_linux_packages.sh appimage <file.AppImage>
#
# deb      : the file list (app, desktop entry, every icon size, no leaked
#            development files), the control file, and - with docker - a clean
#            ubuntu:24.04 container that `apt install`s it and starts the CLI.
# appimage : extracts it and checks the icon files, the desktop entry, the Qt
#            SVG/multimedia plugins, unresolved libraries, and starts it.
set -euo pipefail

die() { echo "error: $*" >&2; exit 1; }
need() { grep -qE "$2" <<<"$1" || die "missing in $3: $2"; }
forbid() { if grep -qE "$2" <<<"$1"; then die "must not be in $3: $2"; fi; }

ICON=eti-stream-analyser
SIZES="16 24 32 48 64 128 256 512"

check_deb() {
    local deb="$1" files
    [ -f "${deb}" ] || die "no such file: ${deb}"
    echo "=== ${deb}"; dpkg-deb -I "${deb}" | sed -n '1,25p'
    files="$(dpkg-deb -c "${deb}" | awk '{print $6}')"

    need "${files}" '^\./usr/bin/StreamDABAnalyser$' "the DEB"
    need "${files}" '^\./usr/bin/streamdab-cli$' "the DEB"
    need "${files}" "^\./usr/share/applications/${ICON}\.desktop$" "the DEB"
    need "${files}" "^\./usr/share/icons/hicolor/scalable/apps/${ICON}\.svg$" "the DEB"
    for s in ${SIZES}; do
        need "${files}" "^\./usr/share/icons/hicolor/${s}x${s}/apps/${ICON}\.png$" "the DEB"
    done
    need "${files}" '^\./usr/lib/libqtadvanceddocking-qt6\.so' "the DEB"
    forbid "${files}" '^\./usr/include/' "the DEB"
    forbid "${files}" '^\./usr/lib/cmake/' "the DEB"
    ! dpkg-deb -f "${deb}" Depends | grep -q 'libqt6test' || die "Depends lists libqt6test"
    echo "DEB file list OK"

    if command -v docker >/dev/null 2>&1; then
        echo "--- installing in a clean ubuntu:24.04 container"
        docker run --rm -v "$(cd "$(dirname "${deb}")" && pwd):/pkg:ro" ubuntu:24.04 bash -euxo pipefail -c '
            export DEBIAN_FRONTEND=noninteractive
            apt-get update -qq
            apt-get install -y /pkg/'"$(basename "${deb}")"' >/dev/null
            streamdab-cli --version >/tmp/ver.txt; head -3 /tmp/ver.txt
            if ldd /usr/bin/StreamDABAnalyser | grep "not found"; then echo "unresolved libraries"; exit 1; fi
            if ldd /usr/bin/streamdab-cli | grep "not found"; then echo "unresolved libraries"; exit 1; fi
            apt-get install -y -qq desktop-file-utils >/dev/null
            desktop-file-validate /usr/share/applications/'"${ICON}"'.desktop
            apt-get remove -y -qq streamdab-analyser >/dev/null
            test ! -e /usr/bin/StreamDABAnalyser
        '
    else
        echo "note: docker not available - skipped the clean-install check"
    fi
}

check_appimage() {
    local img="$1" dir
    [ -f "${img}" ] || die "no such file: ${img}"
    img="$(cd "$(dirname "${img}")" && pwd)/$(basename "${img}")"   # absolute: we cd below
    chmod +x "${img}"
    export APPIMAGE_EXTRACT_AND_RUN=1
    dir="$(mktemp -d)"
    ( cd "${dir}" && "${img}" --appimage-extract >/dev/null )
    local root="${dir}/squashfs-root"
    [ -d "${root}" ] || die "could not extract the AppImage"

    # icon at the AppDir root (what file managers show) + inside the hicolor tree
    [ -e "${root}/.DirIcon" ] || die "no .DirIcon"
    ls "${root}"/${ICON}.png >/dev/null 2>&1 || ls "${root}"/${ICON}.svg >/dev/null 2>&1 \
        || die "no ${ICON}.png/.svg at the AppDir root"
    [ -f "${root}/usr/share/icons/hicolor/256x256/apps/${ICON}.png" ] || die "no 256x256 icon in usr/share/icons"
    grep -q "^Icon=${ICON}$" "${root}"/*.desktop || die "desktop entry does not say Icon=${ICON}"
    command -v desktop-file-validate >/dev/null 2>&1 && desktop-file-validate "${root}"/*.desktop
    python3 - "${root}/.DirIcon" <<'PY'
import sys
from PIL import Image
im = Image.open(sys.argv[1]); im.load()
assert im.size[0] >= 128, f".DirIcon too small: {im.size}"
print("icon:", im.size, im.mode)
PY

    # Qt plugins the app needs at run time
    ls "${root}"/usr/plugins/imageformats/libqsvg.so >/dev/null || die "libqsvg.so not bundled"
    ls "${root}"/usr/plugins/iconengines/libqsvgicon.so >/dev/null || die "libqsvgicon.so not bundled"
    ls "${root}"/usr/plugins/platforms/libqxcb.so >/dev/null || die "libqxcb.so not bundled"
    ls "${root}"/usr/plugins/multimedia/*.so >/dev/null 2>&1 || echo "note: no multimedia plugin bundled"

    # libraries: nothing may stay unresolved (system libs such as libc are fine)
    local missing
    missing="$(LD_LIBRARY_PATH="${root}/usr/lib" ldd "${root}/usr/bin/StreamDABAnalyser" | grep 'not found' || true)"
    [ -z "${missing}" ] || die "unresolved libraries:\n${missing}"

    echo "--- starting the AppImage"
    QT_QPA_PLATFORM=offscreen "${img}" --version >"${dir}/version.txt"
    head -3 "${dir}/version.txt"
    rm -rf "${dir}"
    echo "AppImage OK"
}

case "${1:-}" in
    deb)      check_deb "${2:?usage: $0 deb <file.deb>}" ;;
    appimage) check_appimage "${2:?usage: $0 appimage <file.AppImage>}" ;;
    *) die "usage: $0 deb|appimage <file>" ;;
esac
