# UI Redesign — Evaluation & Implementation Plan (Tab 1 regroup / toolbar / strip)

**Date:** 2026-10-02
**Branch:** `feature/gui-qtads`
**Sources:** [StreamDAB-Analyser UI Redesign.html](StreamDAB-Analyser%20UI%20Redesign.html) (mockup) ·
[streamdabanalyser_ui_redesign.xlsx](streamdabanalyser_ui_redesign.xlsx) (grouping rationale + bottom-strip matrix + Qt-ADS button reference)
**Current implementation:** Qt-ADS migration `263bcab` + decode fixes `6b0b20f`, `4858126`, settings `fe1c431`
**Constraint (user):** ฟังก์ชันหลักเหมือนเดิม — แก้ / group / ขยับ layout เป็นหลัก (functions stay identical; layout regrouping/relocation only)

---

## 1. Evaluation — mockup vs current layout

### 1.1 Tab 1 "DAB Ensemble ETI Analysis"

| Mockup (doc) | Panels inside | Change type (xlsx) | Current (263bcab) | Action |
|---|---|---|---|---|
| **Transport** (tabbed) | Player, Decoder | Grouped (new) | 2 separate left-column docks | Tabify into one dock area (Player \| Decoder) |
| **System** (standalone) | System (FIC Overview / Control internal tabs) | Unchanged | 1 left dock w/ internal QTabWidget | Keep as-is |
| **Signal Analysis** (tabbed) | DAB Ensemble Explorer, Subchannel Organisation, **CU Usage** | Grouped (new); CU **moved from right column** | 2 centre docks + CU right dock | Tabify 3; CU relocated to centre group |
| **Frame Analysis** (tabbed) | ETI Frame Navigator, Hex Viewer, ETSI Compliance | Grouped (new) | Navigator dock + one Hex\|Compliance dock (internal tabs) | Combine into one tabbed group (3 tabs) |
| **Signal Info** (tabbed) | ETI Overview, Status, Timing | Grouped (new) | 3 separate right docks | Tabify into one area |
| **Error & Statistics** (tabbed) | DAB Error Counter, DAB Stream Statistics | Grouped (new) | 2 separate right docks | Tabify |
| **Now Playing (DLS+)** | Now Playing (DLS+) | Unchanged | Centre bottom dock | Keep (compact, bottom of centre) |
| **Input Source** → **toolbar** | File/Network + path + Browse + Load | Moved (toolbar bar above workspace) | Full centre dock | Remove dock; build compact bar in tab-1 page header |
| **Messages** → **bottom strip** | Messages (log) | Moved (strip) | Left-column dock | Remove dock; add as 6th tab in shared bottom strip |

**Column widths (mockup):** left 230 px, right 272 px, centre flexible — vs current ADS default split; set initial splitter sizes ≈ {230, flex, 272}.
**Dock heights (mockup):** Transport 178 · Signal Analysis 190 · Now Playing 82 (fixed, bottom of centre) · Signal Info 210 · Error & Stats + Frame Analysis + System grow.

### 1.2 Tabs 2 & 3 + bottom strip

- Tab 2 "FIC-Analyser" and Tab 3 "FIC-Extractor": **structure unchanged** (3 / 2 column docks).
- **Bottom strip** (shared across ALL 3 tabs): gains a 6th tab **"Messages"** (moved) — per xlsx sheet "Bottom Strip": `Messages ✔ (moved here) ✔ ✔`; all 6 tabs always shared.
  - Adjustment to current: today the strip is only on the *window level* (below the QTabWidget) — 5 tabbed docks. This matches "shared across all 3 tabs" already; just add the Messages dock.

### 1.3 Behavioural notes per xlsx "Qt-ADS Dock Buttons"

The mockup's per-dock header buttons (⊣ auto-hide, ⧉ float, ✕ close, tab-label ✕ = close-one-tab) are **exactly Qt-ADS default features** already enabled (auto-hide config set in the migration; tabified docks close individually via tab ✕). **No new buttons/API needed** — grouping via tabify preserves per-panel float/auto-hide/close, matching the sheet.

### 1.4 Titles

- Mockup shows "**ETI Navigator**" for what we call "ETI Frame Navigator" (xlsx keeps "ETI Frame Navigator" — mockup HTML shortens). **Decision: keep "ETI Frame Navigator"** (matches xlsx + current docs); note in follow-up if the HTML short title is wanted.

---

## 2. Implementation plan (Qt-ADS, incremental; each step verifiable)

### Step A — Input Source toolbar (Tab 1)
- Remove the `tab1_InputSource` dock registration from tab-1 centre column.
- Build a compact bar widget `m_inputToolbar` (QWidget + QHBoxLayout: File/Network radios · path QLineEdit · Browse… · Load File) placed at the TOP of the tab-1 page (above the CDockManager layout, in the page's QVBoxLayout).
- **Network flow preserved:** when "Network Stream" radio is selected, the bar expands with the URL box + Connect/Disconnect/Record + quick-examples row (same member widgets `m_streamUrlEdit`, `m_connectBtn`, `m_disconnectBtn`, `m_recordBtn`, `m_quickExampleCombo`, stream stats labels → could stay in a collapsible second row inside the bar). All existing slots (`onSourceTypeChanged`, `onBrowseFileClicked`, `onLoadFileClicked`, `onConnectClicked`, …) rewire to the bar's widgets — **no function change**.
- Verification: open-file flow works from toolbar; network row appears/hides with radio.

### Step B — Grouped tabbed docks (Tab 1)
Tabify via `addDockWidgetTabToArea(dock, area)` (each panel remains its own `CDockWidget` → float/auto-hide/close-one-tab intact):
- **Transport**: Player + Decoder tabified (left column).
- **Signal Analysis**: Ensemble Explorer + Subchannel Organisation + **CU Usage** (moved from right) tabified (centre).
- **Frame Analysis**: ETI Frame Navigator + (existing Hex Viewer/ETSI Compliance dock) tabified (centre).
- **Signal Info**: ETI Overview + Status + Timing tabified (right).
- **Error & Statistics**: DAB Error Counter + DAB Stream Statistics tabified (right).
- Default active tab per group: Player, Ensemble Explorer, ETI Frame Navigator, ETI Overview, DAB Error Counter (first of each group).
- Verify: each group shows as one dock with N tabs; dragging one tab out floats just that panel; ✕ closes one tab only.

### Step C — Messages → bottom strip
- Remove `tab1_Messages` dock from left column; add `bottom_Messages` CDockWidget (wrapping the same `createLoggingPanel()` widget/members) into `m_bottomDockManager` via `addDockWidgetTabToArea` (6th tab).
- Strip still window-level/shared ⇒ tabs 2/3 automatically get Messages too (per xlsx matrix ✔ ✔ ✔).

### Step D — Default sizes (first launch = mockup)
- Tab-1 column splitter sizes ≈ {230, centre, 272}; per-group dock heights via `setMinimumSizeHintMode`/initial splitter sizes as close as feasible to 178/190/82/210 (Qt-ADS approximates; document residual differences).

### Step E — Layout-version migration
- Keep **objectNames stable** (26 + new `bottom_Messages`); add `docking/layout_version = 2` in QSettings: if saved version ≠ 2, clear saved dock states → first run after upgrade shows the new default layout exactly; afterwards user rearrangements persist as before.
- On save: write version 2 alongside states.

### Step F — Tabs 2/3
- No column changes; only the shared strip gains Messages (automatic via Step C). Verify both tabs show 6 strip tabs.

### Step G — Docs + settings sanity
- Update `docs/gui/QT_ADS_DOCKING.md` (registry: -InputSource dock, +bottom_Messages, new grouped dock areas, version key) & `docs/TASK_LIST.md` (T10).
- Any code touching `m_inputSourcePanel`/`m_messagesPanel` ambiguity: keep member widgets; adjust only registration/placement.

---

## 3. Risks & mitigations

| Risk | Mitigation |
|---|---|
| Saved layouts (QSettings) break after regrouping | Step E version guard resets once; objectNames stable |
| Toolbar loses network/record/quick-example surface | Step A keeps all controls in the expandable bar row; the Input menu was later removed (T23-A) with UDP settings moved to a ⚙ button |
| GUI tests assert old dock structure | Update `tests/gui/test_gui_dock_layout.cpp` expectations (dock count 26→26: −InputSource +Messages move; areas/groups change) |
| Grouped tabs hide panels users expect visible | Groups default to first tab; all panels reachable in 1 click; float persists per panel |

## 4. Acceptance criteria

1. Functions unchanged: open file (toolbar + auto-load CLI arg), play/decoder state, ensemble tree, hex/ETS, error counters, DLS+, strip tabs, network connect flow — all behave as before.
2. Default first launch = mockup layout (groups tabified, toolbar top, Messages in strip, widths/heights ≈ mockup).
3. Per-panel float / auto-hide / close-one-tab still work inside groups (Qt-ADS features untouched).
4. Layout persists across restarts (version 2); old settings reset once.
5. Build 0 errors; ctest 45/46 (only known v1.4 perf failure) incl. updated dock-layout test; GUI smoke exit 124; CLI parity unchanged (decode untouched).
6. Docs updated (`QT_ADS_DOCKING.md`, `TASK_LIST.md`).

## 5. Commit breakdown (per "commit แต่ละส่วน")

1. `docs: UI redesign evaluation + plan` (this file) — docs-only commit.
2. `feat(gui): apply UI redesign layout (toolbar/grouping/strip/v2)` — implementation commit incl. doc updates + test updates.