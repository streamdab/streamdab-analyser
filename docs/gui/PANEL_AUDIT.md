# GUI Panel Audit — Post-Load Behavior (ELG: every panel shows real decoded data)

**Date:** 2026-10-02
**Branch:** `feature/gui-qtads`
**Base:** CLI decode now etisnoop-verified parity (6b0b20f/4858126) · analyzer settings (fe1c431) ·
UI redesign v2 (1a2f9fd) · AdvancedFIGAnalyzer GUI-path fix (uncommitted worker round — 25-dock registry active)
**Reference (design):** `ref/man-dabxp_start-en.pdf` (Ingenieurbüro Mulka DA B-XPlorer manual — FIC-XPlorer = FIG database +
MCI/SI tree+lists; FIC-XTractor = bit-level FIC parse, chronological or by-FIG-type views, FIG element tree,
readable value translation: text/date/frequency/coordinates, FIB-hex with marked FIG, error icons
[erroneous FIG / FIB CRC / MCI reconfiguration])
**Ground truth data:** CLI YAML on `eti/bkk_20062022_141637.eti` — ensemble 0x2000 "Bangkok DAB+", ECC 0xF3,
15 services (14 labelled, e.g. 0x2C61 "RROne FM 101", 0x250F "LTM FM 95", 0x2500 "NBTCtest"...),
16 subchannels EEP 3-A (24/12/3 CU), FIB CRCs 100 %, tail TIST, frame CRC counters.

## Audit matrix — Tab 1 (DAB Ensemble ETI Analysis)

Legend: ✅ real data wired & verified · ⚠️ real data wired, needs live verify · ❌ stub/placeholder → fix · 🚫 removed in v2

| Dock / Panel | Populate slot (main.cpp) | Data source | Expected post-load (5001-frame Bangkok capture) | Status |
|---|---|---|---|---|
| **Transport: Player** | `updatePlayerPanelStatus` :2385 | processor state | Status Playing/Stopped, file name, Time, Frame x/5001, progress | ⚠️ verify values after load |
| **Transport: Decoder** | `updateDecoderPanelStatus` :2410 | processor | Status, Type (ETI-NI), Time, Frames 5001 | ⚠️ verify |
| **System** (FIC Overview/Control) | `buildFICOverviewTree` :2516 | m_figAnalyzer | FIG/FIB counts, ensemble info rows | ⚠️ verify populates (used to be tree of "Property/Value/Data/Status") |
| **Signal Analysis: Ensemble Explorer** | `buildEnsembleTree` :2105 + `updateEnsembleTree` :5332 (real) **vs** `updateEnsembleExplorerForFrame` :518 (**SIMULATES** — clears+writes fake "Frame N FIC Data") | m_figAnalyzer (getCurrentEnsemble/getDiscoveredServices) | Tree: ensemble 0x2000 "Bangkok DAB+" + 15 services w/ labels | ❌ **dual writer: real + simulator; frame-selection path overwrites with fake data → FIX** |
| **Signal Analysis: Subchannel Org** | `updateSubchannelTable` :2204 | m_figAnalyzer getAllSubChannels | 16 rows EEP 3-A, size/bitrate/SAD | ⚠️ verify |
| **Signal Analysis: CU Usage** | `updateCUUsage` :2265 | m_figAnalyzer | bars per subchannel, used/total CU, uses-percent | ⚠️ verify |
| **Frame Analysis: ETI Frame Navigator** | nav list build (frame list populating slot — see :489 loop) | frame metadata | 5001 entries, sync/err markers | ⚠️ verify |
| **Frame Analysis: Hex Viewer** | `displayFrameHexData` :1792 | m_hexFormatter + frame bytes | hex dump of selected frame (FIC region marked per FIC-XTractor ideal) | ⚠️ verify |
| **Frame Analysis: ETSI Compliance** | `performETSIComplianceValidation` :610 → `displayComplianceResult` :787 | m_etsiComplianceEngine | validation rows per frame | ⚠️ verify |
| **Now Playing (DLS+)** | `updateNowPlaying` :460 | DLS+ decoder | Track/Artist on DLS frequency (may be empty for this capture) | ✅ passthrough verified in v1.2 |
| **Signal Info: ETI Overview** | `updateETIOverviewTable` :2049 | frame + format | ETI type, ERR, FICF, MID, FCT, NST 16, FL 743 | ⚠️ verify |
| **Signal Info: Status** | status labels | processor | Health/state | ⚠️ verify |
| **Signal Info: Timing** | `updateTimingTable` :2786 | fig/mode | FCT step µs (Mode I 12288 µs? per frame step), ticks | ⚠️ verify values |
| **Error & Stats: DAB Error Counter** | `updateErrorCounterTable` :2438 | error detector + FIB CRC failures | FIC/MSC CRC 0, FIB CRC 0 (clean capture), invalid FIG 0 | ⚠️ verify; wire AdvancedFIGAnalyzer fib_crc_failures |
| **Error & Stats: Stream Statistics** | `updateStreamStatistics` :2301 | m_etiProcessor + m_figAnalyzer | FIC blocks/FIBs processed, FIG totals, frame rate | ⚠️ verify |
| **Input toolbar** | ctor + onSourceTypeChanged | QSettings/CLI arg | file path shown; Network row hidden | ✅ (v2 test) |
| **Bottom strip** (System Messages / Performance / Error Detection / Logging / Constellation / **Messages**) | `refreshLogDisplay` :1723, `updatePerformanceMetrics` :1913, strip builders | logging + perf profiler + error detector | OSD messages during load/complete; perf line; log entries; constellation plot | ⚠️ verify strip tabs show content (Performance/Error Detection/Logging/Constellation real widgets from original app) |

## Audit matrix — Tabs 2 & 3 (FIC-Analyzer / FIC-Extractor)

| Panel | Slot | Status |
|---|---|---|
| Tab2 Left (Overview + Service Tree + Digital Org) | `updateTab2OverviewLabels` :2551, `buildTab2ServiceTree` :2564, `updateTab2ServiceTable` :2639 | ❌ **overview = literal "Ready/Available" strings; service tree built from what? → wire real ensemble/service data** |
| Tab2 Center (Ensemble/Sub-Channel/Service tables) | `updateTab2EnsembleTable` :2605, `updateTab2SubchannelTable` :2623 | ❌ likely mirror of tab1 data → wire from m_figAnalyzer |
| Tab2 Right (FIG Content) | `updateTab2FIGContentTable` :2655 | ❌ verify; should list FIG types observed with counts (0/0,0/1,0/2,0/9,1/0,1/1 on this capture) |
| Tab3 Left (FIG Instance List) | `buildFIGInstanceTree` :2672 | ❌ **explicit placeholder** — "TODO: Connect to actual FIG instance tracker" → wire real per-frame FIG instances (type/ext/frame/CIF/FIB/len) — FIC-XTractor chronological view |
| Tab3 Right (FIG Item Details) | `buildFIGItemDetailsTree` :2729 | ❌ build real element tree per selected FIG (per EN 300 401 field tables) — FIC-XTractor element view |

## Required fixes (from this audit)

1. **Kill the simulator**: `updateEnsembleExplorerForFrame` must stop writing fake content to `m_ensembleTree`; frame-selection should only re-scroll/select, not rewrite with simulated FIC. Keep the real writers (`buildEnsembleTree`/`updateEnsembleTree` via m_figAnalyzer).
2. **Tab 2 real data**: overview labels show real ensemble name/label, real FIC FIG count, real subchannel count; service/subchannel/ensemble tables from m_figAnalyzer (labels incl. short labels); FIG content table from a FIG-type counter fed by the analyzer.
3. **Tab 3 real FIG tracer**: add a per-frame FIG instance collector on the GUI path (reuse the modern parser's fig blocks / AdvancedFIGAnalyzer walk): rows = FIG type/ext/frame/CIF/FIB/length/items (chronological); select row → FIG item details tree built from real element decoders (per EN 300 401); mark FIB CRC failures + erroneous FIGs + MCI reconfig flags (per manual icons).
4. **FIB-hex**: hex viewer should mark the analyzed FIG bytes within FIB (per manual FIC-XTractor) — v1.3.x scope candidate if not cheap.
5. **Error counter**: feed FIB CRC failure count from the analyzer (`fib_crc_failures`); verify error-detector counts.
6. **Dock-layout GUI test** extension: offscreen load of the real fixture, then assert per-panel contents (ensemble tree items, subchannel table rows, tab2 labels ≠ "Ready", tab3 FIG list non-empty, hex viewer non-empty, overview/timing values, error counter row counts) — deterministic, no per-frame ticking.

Status notes: panels fed by the now-fixed AdvancedFIGAnalyzer (ensemble tree real path, subchannels, CU, FIC overview) should already show correct values; the above ❌ items are the concrete gaps found by this audit.

## Commit plan
1. docs-only: this audit (this file) + `CLI_FIG_COVERAGE.md` (parallel evaluation).
2. GUI fix wave: simulator removal, tab2/tab3 real wiring, error counter, extended offscreen GUI assertions — one commit w/ doc updates.
3. CLI detail wave (see CLI_FIG_COVERAGE.md) — separate commit.