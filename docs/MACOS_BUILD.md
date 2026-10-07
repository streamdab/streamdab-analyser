# Building StreamDAB Analyser on macOS

Guide for Apple Silicon (arm64) and Intel (x86_64) Macs, macOS 12+. The build
steps are the same on both architectures; Homebrew/CMake resolve the toolchain
transparently. The project is a C++20 / Qt 6 desktop application with an
optional ZeroMQ ETI input transport; nothing here requires Xcode GUI — only the
Command Line Tools.

> The legacy `cmake/MacOSConfig.cmake` is **not** included by `CMakeLists.txt`
> (dead config, tracked for the wire-or-delete decision in
> `docs/CROSS_PLATFORM_BUILD_PLAN.md` B8). Everything below is handled by the
> portable detection ladder in the top-level `CMakeLists.txt`.

## 1. Prerequisites

```bash
xcode-select --install                  # Command Line Tools (clang, make)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install cmake qt@6 faad2 fftw libzmq googletest
```

Notes:

- **`qt@6` is keg-only** — its binaries are not on `PATH`, so you must point
  CMake at it explicitly (the preset does this via `QT_DIR`).
- `libzmq` (and the bundled cppzmq header, `zmq.hpp`) are optional: if brew
  cannot provide them, pass `-DDABX_ENABLE_ZMQ=OFF` to compile the ZeroMQ
  transport out. The rest of the analyser is unaffected.
- `faad2` provides the AAC decoder for DAB+ audio decode; `fftw` is used by the
  spectrum/FIC analysis. Both have plain `find_path`/`find_library` fallbacks,
  so Homebrew's default prefixes (`/opt/homebrew` on Apple Silicon,
  `/usr/local` on Intel) are found automatically.

## 2. Configure, build and test (presets)

```bash
export QT_QPA_PLATFORM=offscreen    # MUST be set before configure: registers the GUI test targets
export QT_DIR="$(brew --prefix qt@6)"
cmake --preset macos-brew           # configure into build/ (Release)
cmake --build --preset build-macos  # 8 jobs, RAM-safe (never a bare -j!)
ctest --preset test-macos           # full suite, serial, output-on-failure
```

Equivalent without presets:

```bash
export QT_QPA_PLATFORM=offscreen
cmake -B build -S . -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
JOBS="$(bash scripts/build_jobs.sh)"          # bounded: min(nproc, RAM_GB/2, 8)
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"     # nested cmake/make honour this
cmake --build build --parallel "$JOBS"        # NEVER a bare -j (unbounded)
cd build && ctest --output-on-failure
```

`macOSConfig.cmake` is **not** wired into the build (see note above); the
detection ladder in `CMakeLists.txt` is what finds Qt/FAAD2/FFTW/libzmq.

### What the suite expects

- `ctest` runs the same 55-target gate as Linux. Run the test binaries from
  `build/tests/` (fixture paths are relative to that directory), or just use
  `ctest` which sets the correct working directory automatically.
- Thai UTF-8 (DLS+/EPG/MOT labels) is a first-class feature and is validated by
  the fixtures on macOS too.

## 3. Run

```bash
./build/StreamDABAnalyser                 # GUI (native macOS window)
./build/streamdab-cli --input eti/bkk_20062022_141637.eti --output out.yaml
```

CLI mode is available both as the standalone `streamdab-cli` and via
`./build/StreamDABAnalyser --cli --input ... --output ...`.

## 4. Known gaps on macOS

| Area | Status |
|---|---|
| Audio output backend | **Not yet** — the ALSA backend is Linux-only and disabled outside Linux; DAB+ **decode** works, live playback does not. A cross-platform Qt Multimedia backend is planned (see `docs/CROSS_PLATFORM_BUILD_PLAN.md` B-blocks). |
| `.app` bundle / `.dmg` | **Not yet** — `install()` rules + CPack (`BUNDLE`/`DragNDrop`) are on the roadmap; until then run the binary from `build/`. |
| ZeroMQ transport | Optional; compiled in when `zmq.hpp` + libzmq are found (brew provides both), or disabled with `-DDABX_ENABLE_ZMQ=OFF`. The validator then rejects `zmq+tcp://` with a clear "not built in" message. |
| Notarisation / signing | Not configured; a `release.yml` GitHub Actions job is planned (unsigned for now). |

## 5. Troubleshooting

- **`Could not find Qt6`** — `QT_DIR` is unset or points at a non-Qt path.
  Re-check `brew --prefix qt@6`; Qt 6.9+ ships `lib/cmake/Qt6/Qt6Config.cmake`.
- **Undefined `zmq.hpp`** — brew's `libzmq` does not install the C++ header
  everywhere; `brew install cppzmq`, or disable with `-DDABX_ENABLE_ZMQ=OFF`.
- **Slow tests on Apple Silicon** — the 55-target suite is single-threaded
  (`ctest` runs serially on purpose: several GUI/dock tests share QSettings
  stores). Expected ~2–4 min.
- **`cmake --preset macos-brew` fails with an `$env{QT_DIR}` error** — the
  preset macro requires the variable to be set; export it first (step 2).
- **GUI tests fail when run from the repo root** — fixture paths are relative
  to `build/tests`; always run via `ctest` or `cd build/tests` first.

See also: `docs/WINDOWS_BUILD.md` (Windows), `docs/INSTALL`-equivalent steps in
the main `INSTALL.md`, and the cross-platform issues ledger in
`docs/CROSS_PLATFORM_BUILD_PLAN.md`.