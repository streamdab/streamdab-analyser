#!/usr/bin/env bash
# Check that a bundled macOS .app is self-contained and starts on its own.
#
#   verify_macos_bundle.sh <StreamDAB-Analyser.app>
#
# 1. every Mach-O inside the bundle may only depend on system libraries or on
#    something that is itself inside the bundle (@executable_path/@loader_path,
#    or an @rpath name present in Contents/Frameworks); Homebrew / build-tree
#    absolute paths mean the app only works on the machine that built it;
# 2. the app starts (`--version`) with an empty environment: no DYLD_*, and a
#    PATH without Homebrew.
set -euo pipefail

APP="${1:?usage: $0 <App.app>}"
[ -d "$APP/Contents/MacOS" ] || { echo "not an app bundle: $APP" >&2; exit 2; }
EXE="$(find "$APP/Contents/MacOS" -type f -perm -u+x | head -1)"
FRAMEWORKS="$APP/Contents/Frameworks"
bad=0

while IFS= read -r -d '' file; do
    case "$(file -b "$file")" in *Mach-O*) ;; *) continue ;; esac
    # otool -L also lists the library's own install name; that is not a
    # dependency, so leave it out.
    self_id="$(otool -D "$file" 2>/dev/null | sed -n 2p)"
    while IFS= read -r dep; do
        [ -n "$self_id" ] && [ "$dep" = "$self_id" ] && continue
        case "$dep" in
            /usr/lib/*|/System/*|@executable_path/*|@loader_path/*) ;;
            @rpath/*)
                name="${dep#@rpath/}"
                if [ ! -e "$FRAMEWORKS/$name" ] && [ ! -e "$(dirname "$file")/$name" ]; then
                    echo "UNRESOLVED @rpath dependency: ${file#"$APP"/} -> $dep"; bad=1
                fi ;;
            *)
                echo "NON-RELOCATABLE dependency: ${file#"$APP"/} -> $dep"; bad=1 ;;
        esac
    done < <(otool -L "$file" 2>/dev/null | tail -n +2 | awk '{print $1}')
done < <(find "$APP/Contents" -type f -print0)

# 3. the plugins and frameworks the application really uses must be there: the
#    Cocoa platform plugin (the GUI cannot start without it), the SVG image and
#    icon-engine plugins (the docking library's buttons and the window icon are
#    SVG), and a multimedia backend for audio playback.
PLUGINS="$APP/Contents/PlugIns"
need=("$PLUGINS/platforms/libqcocoa.dylib"
      "$PLUGINS/iconengines/libqsvgicon.dylib"
      "$PLUGINS/imageformats/libqsvg.dylib"
      "$FRAMEWORKS/QtSvg.framework")
for item in "${need[@]}"; do
    [ -e "$item" ] || { echo "MISSING required component: ${item#"$APP"/}"; bad=1; }
done
if [ -d "$FRAMEWORKS/QtMultimedia.framework" ] && \
   ! ls "$PLUGINS"/multimedia/*.dylib >/dev/null 2>&1; then
    echo "MISSING required component: PlugIns/multimedia backend (QtMultimedia is linked)"; bad=1
fi
echo "--- bundled frameworks and libraries:"; ls "$FRAMEWORKS" | sed 's/^/    /'
echo "--- bundled plugins:"; (cd "$PLUGINS" && find . -name '*.dylib' | sort | sed 's/^/    /')

echo "--- starting $(basename "$EXE") --version with an empty environment"
if ! env -i HOME="${HOME:-/tmp}" PATH=/usr/bin:/bin "$EXE" --version; then
    echo "app failed to start (exit $?)"; bad=1
fi

# 4. the GUI itself must come up (the --version run above never loads the
#    Cocoa platform plugin, the SVG plugins or the docking library's UI).
echo "--- starting the GUI for 10 s with an empty environment"
GUI_LOG="$(mktemp)"
env -i HOME="${HOME:-/tmp}" PATH=/usr/bin:/bin "$EXE" >"$GUI_LOG" 2>&1 &
GUI_PID=$!
sleep 10
if kill -0 "$GUI_PID" 2>/dev/null; then
    echo "GUI is running after 10 s - start-up OK"
    kill "$GUI_PID" 2>/dev/null || true
    wait "$GUI_PID" 2>/dev/null || true
else
    wait "$GUI_PID" 2>/dev/null; echo "GUI exited early (exit $?); output:"
    sed 's/^/    /' "$GUI_LOG"; bad=1
fi
grep -i "could not load\|plugin\|dyld" "$GUI_LOG" | head -5 | sed 's/^/    log: /' || true
rm -f "$GUI_LOG"

if [ "$bad" -ne 0 ]; then
    echo "macOS BUNDLE CHECK FAILED"; exit 1
fi
echo "macOS bundle check passed: self-contained and starts on its own."
