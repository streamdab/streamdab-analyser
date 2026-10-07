# Configuration

Where the StreamDAB Analyzer stores settings, which key groups are in use, and
how to reset or export them. Written after the v1.3 settings audit: **every
control in the Settings dialog either drives real behaviour or was removed —
there are no silent no-ops.**

## Store location (per platform)

All settings — the Settings dialog, the Analyzer decode options and the
window/docking state — use ONE explicit scope created with
`QSettings("StreamDAB-Analyzer", "DABAnalyzer")` (see `src/utils/app_settings.hpp`):

| Platform | Native store |
|---|---|
| Linux   | `~/.config/StreamDAB-Analyzer/DABAnalyzer.conf` (INI) |
| Windows | Registry `HKEY_CURRENT_USER\Software\StreamDAB-Analyzer\DABAnalyzer` |
| macOS   | `~/Library/Preferences/...plist` (via CFPreferences) |

The Settings dialog shows the resolved path on the **General** tab
("Configuration → Store"). The application and the dialog are guaranteed to
agree because both construct `QSettings` with the same explicit org/app.
Two legacy configuration classes are **not compiled** and use their own
scopes, so they cannot affect the store: `src/utils/config_manager.*`
(org/app "ETI Stream Analyzer", incl. a `files/` key that nothing reads) and
`src/utils/configuration_manager.*` if present. If they are ever revived they
must be migrated to this helper before use.

### Reset / export

- **Reset to defaults** inside the app: Settings → *Restore Defaults* (resets
  the *widgets*; the values are persisted when you press OK/Apply).
- **Full reset** (manual): delete the file / registry key / plist listed above
  while the app is closed. Window layout additionally resets when the
  `docking/layout_version` in the store does not match the current layout (the
  app discards stale dock states once after an upgrade).
- **Export**: the store is plain INI (Linux) — copy the file. On Windows/macOS
  use the registry/plist export tools. There is no separate export button.

## Settings dialog tabs

What each tab controls, and when a change takes effect:

| Tab | Controls | Effect / when it applies |
|---|---|---|
| **General** | Confirm before closing; read-only configuration-store path | Exit confirmation applies immediately (window X and File→Exit; headless runs never prompt). The path is display-only. |
| **Processing** | Dashboard update rate (`Hz`); network receive buffer (`frames`) | Update rate changes the dashboard/Real-Time-Chart sampler immediately. The buffer is applied to a new ETI-over-IP connection and re-initialises an active one; file playback is unaffected. |
| **Audio** | Output device, mute, volume | Applies immediately to the T34 audio controller; the Transport Audio tab shows the same state (no second copy). |
| **Advanced** | Log level (Error/Warning/Info/Debug/Critical) | Applies immediately to the singleton `Logger` (console + file). Also editable from the bottom-strip **Logging** tab (same key). |
| **Analyzer** | Decode options (charset, ETI mode, FIC/FIB handling, labels, timestamp, SId) | Persisted on OK/Apply but take full effect on the **next file load**; the CLI builds on the same defaults. |

## Key groups in use

| Group | Keys | Wired to |
|---|---|---|
| `general/` | `confirmExit` (bool) | close confirmation in `DABAnalyzerWindow::closeEvent` (window X and File→Exit only; headless runs never prompt) |
| `analysis/` | `updateRate` (Hz 1..10), `bufferSize` (frames 10..10000) | dashboard sampler interval (`m_perfSampleTimer`, 1 Hz default == historical 1 s); ETI-over-IP receiver frame buffer (applied to a live receiver and to every new connection) |
| `audio/` | `device` (string), `muted` (bool), `volume` (0..100) | the T34 `AudioPlaybackController` **and** the Transport group's Audio tab — one shared state (both the tab and the Settings dialog edit/persist it) |
| `advanced/` | `logLevel` (0=Error, 1=Warning, 2=Info, 3=Debug, 4=Critical) | `Logger::setLogLevel` (immediate). Written/read by BOTH Settings→Advanced and the bottom-strip Logging tab (single source of truth: the `Logger`). |
| `analyzer/` | rows 1-14 of the option-variant matrix (force charset, ETI mode, NST-1, FIC mode, FIB CRC, ECC, labels, timestamp, frame CRC, SId) | `eti::AnalyzerSettings` (honored by the FIG analyzer on the next file load; CLI flags build on the same defaults) |
| `window/`, `docking/` | geometry, dock states, layout version | window layout persistence (Qt-ADS), not shown in the dialog |

### Apply semantics

- **Apply immediately**: confirm-exit, update rate, network buffer, log level,
  audio device/mute/volume (both the dialog and the Audio tab reflect it right
  away).
- **Applied on the next load**: `analyzer/*` decode options. The GUI re-applies
  them at the next file load; the Settings dialog's status bar says so.
- Every remaining control therefore has an observable effect; nothing is
  written-then-ignored.

## Live stream input (ETI over IP)

The Network Stream input accepts exactly these URL forms (validated by
`eti::validate_stream_url()`, shared by the UI pre-check and the receiver):

| Transport | Grammar | Notes |
|---|---|---|
| UDP multicast | `udp://A.B.C.D[:port]` or `A.B.C.D[:port]` | `A.B.C.D` must be in `224.0.0.0/4`; `iface` auto-selected |
| TCP raw ETI | `tcp://host[:port]` | `host` is a DNS name or IPv4 literal; the byte stream is re-framed into 6144-byte ETI frames (sync-word resync on loss) |
| ZeroMQ SUB | `zmq+tcp://host[:port]` | Subscribes (all frames) to an **ODR-DabMux ETI publisher** |

- The port is optional for every transport and defaults to **9200**; when given
  it must be 1..65535 (digits only).
- Rejected with a descriptive error: empty input, `http://`/any other scheme,
  a host name **without** a transport scheme (ambiguous), unicast UDP, IPv6,
  paths and malformed addresses/ports.
- **ZMQ publisher expectation:** ODR-DabMux configured with a ZeroMQ ETI output,
  e.g. `outputs.zmq { endpoint "tcp://*:9201"; }`. The analyzer connects a
  `ZMQ_SUB` socket, subscribes to everything and expects each message to be a
  raw 6144-byte ETI frame. EDI/PFT (ETSI TS 102 693) is **not** supported yet —
  it is the v2.0 Phase E-2 work.
- **ZMQ slow-joiner:** ZeroMQ PUB/SUB has no subscription handshake. The
  analyzer's SUB socket therefore **loses the first frames** the publisher sends
  until the subscription has propagated (and again after any reconnect). This is
  inherent `ZMQ_SUB` behaviour, not a bug; the stream stabilises after a few
  frames.
- Both new transports keep the T22 connection lifecycle: a failed attempt
  produces exactly one error (no retry loop), automatic reconnection is armed
  only after a confirmed connection, and **intentional** teardown is
  loss-silent (it can never retrigger the reconnect policy). Loss-driven
  retries honour `reconnect_delay` (only the first retry is immediate).
- Teardown is **bounded**: TCP sockets are aborted, the ZMQ socket has
  `linger=0` and is destroyed on the worker thread, and the receiver waits at
  most a few seconds for the worker to shut down before terminating the thread.
  The worker's event loop never blocks indefinitely (the ZMQ poll uses a 1 ms
  timeout), so this wait is not expected to be reached.

## Removed options (with reason)

These controls existed before the audit but had **no backend**: nothing in the
application read their keys, and no feature implemented them. They were
deleted (widget + member + QSettings read/write), and their keys are no longer
read or written:

| Removed | Key(s) | Reason |
|---|---|---|
| Theme (Dark/Light/System) | `general/theme` | custom theming deliberately disabled in the app ("ALL CUSTOM THEMING DISABLED FOR TEXT VISIBILITY"); no palette/style backend |
| Auto-save + interval | `general/autoSave`, `general/autoSaveInterval` | no periodic autosave exists; window state is saved on close (docking keys) |
| Display colours + font | `display/background*`, `grid*`, `text*`, `font` | nobody consumed them; theming disabled |
| Real-time processing | `analysis/realTime` | no such processing toggle exists (decode is load-time + live-stream) |
| Hardware acceleration | `analysis/hardwareAccel` | no such code path |
| Proxy server | `network/useProxy`, `proxyHost`, `proxyPort` | the network path opens raw UDP/TCP/ZMQ sockets and does not use Qt's proxy-aware high-level networking — no proxy support |
| Memory limit | `performance/memoryLimit` | no allocation-cap enforcement |
| Process priority | `performance/priority` | no platform priority-setting code |
| Debug mode (checkbox) | `advanced/debugMode` | redundant: the log-level combo covers verbosity (Debug level == debug mode) |