# Network Stream — Failure Analysis & Input-Menu Evaluation

**Date:** 2026-10-05 · **Branch:** `main` · **Status:** root cause identified; fix pending (queued after T20)
**Trigger:** manual test — Connect with a `localhost` URL → `[STREAM ERROR] Failed to start reception: Invalid multicast address: localhost`, and pressing Connect **loops** instead of reporting the failure.

---

## 1. Root cause (verified in code)

| # | Layer | Evidence | Effect |
|---|---|---|---|
| 1 | Quick examples advertise unsupported schemes | `src/main.cpp:4969-4971` — `udp://239.192.0.1:9200`, `tcp://192.168.1.100:9200`, **`zmq+tcp://localhost:9201`** | Selecting the 3rd example puts `localhost` in the URL; the receiver is **multicast-only** |
| 2 | Receiver rejects non-multicast by **throwing** | `src/core/network_stream_receiver.cpp:112-115` `NetworkStreamWorker::join_multicast_group()` → `throw std::runtime_error("Invalid multicast address: …")` (also throws on bind failure :95-99 and bad interface :120-125) | Exception from the worker path surfaces as the `[STREAM ERROR]` line; there is **no pre-validation** in the UI |
| 3 | **Retry loop** | `src/main.cpp:1907-1909` sets `config.auto_reconnect = true; max_reconnect_attempts = 10;` + `network_stream_receiver.cpp:298-301` schedules automatic reconnection (+ `setup_reconnection_timer()` :400-411) | Each attempt fails → `error_occurred` → the UI lambda (`main.cpp:1945-1956`) appends `[STREAM ERROR] …` and **re-enables Connect** → pressing Connect repeats → the observed loop |
| 4 | No user-facing popup | failure path only writes to System Messages + sets the status dot red (`main.cpp:1945-1956`, `1961-1971`) | User sees a loop of log lines, no clear "cannot connect" dialog |

## 2. Fix plan (network stream robustness)

1. **Pre-validate before touching the core** (UI): accept only a syntax the receiver supports — multicast IPv4 `224.0.0.0/4` in `udp://A.B.C.D:port` or `A.B.C.D:port` form; reject hostnames/unicast/TCP/ZMQ with a single clear `QMessageBox::warning` naming the supported format (no connect attempt, no state change).
2. **Break the loop**: for the *initial* connect attempt set `auto_reconnect = false` (or attempts = 1); enable auto-reconnect only **after** a successful connection is established. Ensure the error path cannot re-arm the button while a retry is pending.
3. **Popup on failure**: show one non-modal `QMessageBox` (or a clear modal warning) per connect attempt with the receiver's `get_last_error()`, plus the System Messages line; keep the status dot red and Connect re-enabled.
4. **Harden the receiver**: never let a worker-thread `throw` escape (wrap join/bind in try/catch → emit `error_occurred` + return false); validate the address in `connect_to_stream()`/`start_reception()` and return a bool + `get_last_error()` instead of throwing.
5. **Fix the examples**: replace unsupported entries with real, working targets (`udp://239.192.0.1:9200`, `239.192.0.1:9200`, and a documented note about ZMQ/TCP being unsupported) and validate on selection; optionally show a hint label. If TCP/ZMQ are desired later, that is a separate feature (receiver currently UDP-multicast only).
6. **Tests**: unit test for the URL validator (accept multicast/plain, reject localhost/TCP/ZMQ/hostnames, bad ports); receiver test asserting a non-multicast config produces `error_occurred` **without** throwing and **without** scheduling retries; GUI test asserting a bad URL click shows the guard (message path) with no receiver created.

## 3. Input menu — is it still needed?

**Current state (after T23 option A, 2026-10-05): the Input menu is REMOVED.** The former contents were `File Input` (radio), `UDP Input` (radio) — both redundant with the toolbar File/Network radios — and **`UDP Streaming Settings…`**, which now lives on a ⚙ button (`objectName=udpSettingsButton`) at the end of the network toolbar row. The options below are kept for the historical record.

| # | Option | Pros | Cons |
|---|---|---|---|
| **A** | **Remove the Input menu entirely**; move "UDP Streaming Settings…" to a ⚙ button in the network toolbar row; keep the File/Network radio in the toolbar | Cleanest menu bar (`File / View / Help`); single source of truth for input mode; matches the redesign (input lives in the toolbar) | The settings dialog becomes slightly less discoverable (toolbar icon + tooltip) |
| **B** | **Slim the menu**: keep only `UDP Streaming Settings…` (drop the two radio actions, redundant with the toolbar radios) | Keeps a familiar menu path; removes duplication | Menu still exists with one item — mild redundancy |
| **C** | **Keep as-is** | No work | Duplicate input switching (menu + toolbar); the mode can get out of sync |
| **D** | **Keep the menu but re-purpose**: File Input / UDP Input toggle **and** Settings, with the toolbar radios bound to the same `QActionGroup` (single source of truth via shared actions) | No duplication (actions shared), menu + toolbar stay in sync; a11y/keyboard | More wiring |

**Recommendation: A** (with B as the conservative fallback) — the redesigned toolbar already owns File/Network + Connect/Disconnect/Record, so the menu duplicates it; keep only the unique thing (UDP settings) next to the controls it configures.

**Decision (2026-10-05): Option A — implemented (T23).** The Input menu is removed (menu bar is **File / View / Help**); the File/Network radios + Connect/Disconnect/Record remain the single source of truth for input mode, and "UDP Streaming Settings…" now opens from the ⚙ button (`objectName=udpSettingsButton`, tooltip "UDP streaming settings") appended to the network row after Record/status (`src/main.cpp`). The old radio `QActionGroup` is deleted. Covered by `test_gui_dock_layout` (`testInputToolbar` asserts no Input menu + the gear, `testUdpSettingsGearOpensDialog` exercises the dialog path offscreen).

## 4. Sequencing

The currently running T20 worker owns `src/main.cpp`; the network fix (and any Input-menu change) must land **after** T20 to avoid collisions. Planned order:
1. T20 close-out (findings + observations) → commit.
2. **T22 network robustness** (this doc §2) + **T23 Input-menu decision** (§3, pending user's option choice) → one commit.
3. **T21 MOT slideshow (option A)** → separate commit.