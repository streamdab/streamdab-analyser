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
    while IFS= read -r dep; do
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

echo "--- starting $(basename "$EXE") --version with an empty environment"
if ! env -i HOME="${HOME:-/tmp}" PATH=/usr/bin:/bin "$EXE" --version; then
    echo "app failed to start (exit $?)"; bad=1
fi

if [ "$bad" -ne 0 ]; then
    echo "macOS BUNDLE CHECK FAILED"; exit 1
fi
echo "macOS bundle check passed: self-contained and starts on its own."
