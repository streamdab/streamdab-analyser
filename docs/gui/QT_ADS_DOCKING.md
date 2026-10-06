# Qt-ADS Docking — v1.3 GUI Migration + v2 UI Redesign

**@brief** Design + usage notes for the Qt-ADS (Qt Advanced Docking System 4.5.0) layout of `DAB-XPlorer`
("StreamDAB Analyzer", Qt6/C++20). The v1.3 migration (`263bcab`) wrapped every existing named panel of the
original `DABAnalyzerWindow` layout from `src/main.cpp` in an `ads::CDockWidget`. The **v2 UI redesign**
(`docs/gui/UI_REDESIGN_PLAN.md`) regrouped the Tab-1 docks into tabbed groups, moved the Input Source panel
into a page-top toolbar, moved Messages into the shared bottom strip, and added a layout-version guard.
Code-style conventions live in `docs/Qt6_CPP20_Style_Guide.md`; this document is the design counterpart.

**Branch:** `feature/gui-qtads` · **Runnable:** `./build/StreamDABAnalyzer` (CMake target
`DABAnalyzer_Enhanced_FilteringBuild` keeps its name for tests, `OUTPUT_NAME = StreamDABAnalyzer`).

**Revisions:** 2026-10-02 — v2 UI redesign (grouped Tab-1 docks, Input toolbar, Messages→strip,
`docking/layout_version = 2`); earlier placeholder-panel experiment rejected (layout-preserving plan kept).
2026-10-05 — v4 MOT Slideshow (option A): Now Playing grows to ≈200 px and hosts inner tabs
(DLS+ Text · Slideshow); `docking/layout_version = 4`.
2026-10-06 — T27: Now Playing inner tabs replaced by a draggable **left/right splitter** (DLS+ text |
4:3 letterboxed MOT Slideshow); `nowPlayingTabs` is gone.
2026-10-07 — T40/T41/T42: bottom strip no longer hard-capped (default ≈120 px, resizable); the redundant
legacy **Messages** tab removed (8 → 7 strip tabs); Tab-3 right dock gained a **Hex Viewer** inner tab;
`docking/layout_version = 8`.

---

## 1. Architecture

Four independent `ads::CDockManager` instances:

| # | Manager | Host | Docks | QSettings key |
|---|---------|------|-------|---------------|
| 1 | tab1 manager | Tab "📡 DAB Ensemble & ETI Analysis" | 15 | `docking/tab1` |
| 2 | tab2 manager | Tab "🔬 FIC-Analyzer" | 3 | `docking/tab2` |
| 3 | tab3 manager | Tab "📊 FIC-Extractor" | 2 | `docking/tab3` |
| 4 | bottom manager | Window-level bottom strip (shared across tabs) | 7 | `docking/bottom` |

The top `QTabWidget` keeps its 3 original tabs. Each **tab page owns its own `CDockManager`**, so every tab
can be floated/rearranged independently and its state persists separately. The bottom strip is a
**window-level** dock area pinned below the tabs — shared by all tabs (so the redesign's `bottom_Messages`
tab automatically appears on every tab). The Tab-1 page additionally hosts the compact **Input toolbar**
above its dock manager (see §1.1).

### Layout diagram

```
+--------------------------------------------------------------------+
| DABAnalyzerWindow (QMainWindow)                                    |
|                                                                    |
|  mainVerticalSplitter (QSplitter::Vertical)                        |
|  +--------------------------------------------------------------+  |
|  | QTabWidget — tabs area                      (~70% height)   |  |
|  |                                                              |  |
|  |  [ 📡 DAB Ensemble & ETI Analysis ] [ 🔬 FIC-Analyzer ]     |  |
|  |  [ 📊 FIC-Extractor ]                                       |  |
|  |                                                              |  |
|  |  Tab 1 page (QVBoxLayout):                                  |  |
|  |   ├─ inputToolbar (compact Input Source bar, top)           |  |
|  |   └─ CDockManager #1: 3 grouped columns, 15 docks           |  |
|  |  Tab 2 → CDockManager #2: left/center/right, 3 docks        |  |
|  |  Tab 3 → CDockManager #3: left/right, 2 docks               |  |
|  |    left inner tabs: FIG Instance List | Frame List           |  |
|  |    right inner tabs: FIG Item Details | Hex Viewer (T42)    |  |
|  +--------------------------------------------------------------+  |
|  +--------------------------------------------------------------+  |
|  | Bottom strip — window-level CDockManager #4 (~120 px start) |  |
|  | 7 tabbed CDockWidgets, SHARED across all tabs:              |  |
|  |  System Messages | Performance | Real-Time Chart |          |  |
|  |  Error Detection | ETSI Compliance | Logging | Constellation |  |
|  +--------------------------------------------------------------+  |
+--------------------------------------------------------------------+
```

### 1.1 Tab 1 default dock arrangement (CDockManager #1) — v2 groups

Panels keep their **own** `CDockWidget` (float / auto-hide / close-one-tab intact); `addDockWidgetTabToArea()`
only groups them into tabbed dock areas. Registration order defines the tab order; the first dock of each
group is the default active tab.

```
+-------------------------------+-------------------------------+-------------------------------+
| LEFT                          | CENTER                        | RIGHT                         |
|                               |                               |                               |
| [Transport]  Player | Decoder | [Signal Analysis]            | [Signal Info]                 |
|                               |   DAB Ensemble Explorer  *   |   ETI Overview            *  |
| [System]  (standalone)        |   Subchannel Organization    |   Status                      |
|                               |   CU Usage (moved from right)|   Timing                      |
|                               | [Frame Analysis]             | [Error & Statistics]          |
|                               |   ETI Frame Navigator     *  |   DAB Error Counter       *   |
|                               |   Hex Viewer / ETSI Compl.   |   DAB Stream Statistics       |
|                               | [Now Playing / Slideshow]    |                               |
|                               |   (left/right split: DLS+ |   |                               |
|                               |    Slideshow, 4:3 letterbox) |                               |
+-------------------------------+-------------------------------+-------------------------------+
|         ↑ inputToolbar (File/Network radios · path · Browse… · Load File) sits ABOVE this grid |
+-------------------------------+-------------------------------+-------------------------------+
```

* `*` = default active tab of its group.
* CU Usage moved from the right column into the center Signal Analysis group.
* Messages dock removed from the left column → bottom strip (T41 later removed the whole tab).
* Input Source dock removed → toolbar (`inputToolbar`) at the top of the Tab-1 page; its network row
  (stream URL / quick examples / Connect / Disconnect / Record / stream stats) expands only in
  "Network Stream" mode (slots `onSourceTypeChanged` etc. unchanged).

### 1.2 Bottom strip (shared across all tabs)

Window-level CDockManager with ONE tabbed dock area holding the 7 strip docks — all shared by every tab
(per the redesign xlsx matrix ✔ ✔ ✔). T41 removed the redundant legacy **Messages** tab (a static,
empty logging panel backed by the never-constructed `ProfessionalLoggingSystem`), so the strip is now 7.

## 2. Panel registry (all 27 docks)

Dock `objectName` is used for QSettings persistence and auto-hide identity; the visible title reproduces the
original panel label where one existed. For tab 2/3 panels (which bundled several `QGroupBox`es), the visible
title is the original column name (`Left`/`Center`/`Right`) exactly as passed to `ads::CDockWidget` in
`src/main.cpp` — not a content label. The Input Source is **not** a dock anymore (toolbar, §1.1).

### Tab 1 — 📡 DAB Ensemble & ETI Analysis (15, grouped)

| Dock objectName | Visible title | Group (dock area) | Column |
|-----------------|---------------|-------------------|--------|
| `tab1_Player` | Player | Transport (3 tabs) | Left |
| `tab1_Decoder` | Decoder | Transport (3 tabs) | Left |
| `tab1_Audio` | Audio | Transport (3 tabs) | Left |
| `tab1_System` | System | — (standalone) | Left |
| `tab1_EnsembleExplorer` | DAB Ensemble Explorer | Signal Analysis (3 tabs) | Center |
| `tab1_SubchannelOrg` | Subchannel Organization | Signal Analysis (3 tabs) | Center |
| `tab1_CUUsage` | CU Usage | Signal Analysis (3 tabs) | Center |
| `tab1_FrameNavigator` | ETI Frame Navigator | Frame Analysis (2 tabs) | Center |
| `tab1_HexCompliance` | Hex Viewer / ETSI Compliance | Frame Analysis (2 tabs) | Center |
| `tab1_NowPlaying` | Now Playing / Slideshow | — (standalone; left/right splitter DLS+ text | Slideshow 4:3 letterbox, ≈200 px) | Center |
| `tab1_ETIOverview` | ETI Overview | Signal Info (3 tabs) | Right |
| `tab1_Status` | Status | Signal Info (3 tabs) | Right |
| `tab1_Timing` | Timing | Signal Info (3 tabs) | Right |
| `tab1_ErrorCounter` | DAB Error Counter | Error & Statistics (2 tabs) | Right |
| `tab1_StreamStats` | DAB Stream Statistics | Error & Statistics (2 tabs) | Right |

### Tab 2 — 🔬 FIC-Analyzer (3)

| Dock objectName | Visible title | Column / area |
|-----------------|---------------|---------------|
| `tab2_Left` | Service Tree | Left |
| `tab2_Center` | Ensemble & Sub-Channel Tables | Center |
| `tab2_Right` | FIG Content | Right |

### Tab 3 — 📊 FIC-Extractor (2)

| Dock objectName | Visible title | Column / area |
|-----------------|---------------|---------------|
| `tab3_Left` | FIG Instance List | Left (inner tabs: FIG Instance List · **Frame List**) |
| `tab3_Right` | FIG Item Details | Right (inner tabs: FIG Item Details · **Hex Viewer** — T42) |

> **T28 — Tab-3 Frame List.** The `tab3_Left` dock hosts an inner `QTabWidget` (`tab3LeftTabs`) with the
> existing FIG Instance List and a new **Frame List** (frames 1..N of the loaded capture). Selecting a
> frame decodes that frame's FIGs into `tab3_Right` as FIG roots (type/ext/len/FIB/CRC markers) with the
> existing per-FIG element builders nested underneath. FIG groups in the instance list are **collapsed by
> default** and are no longer pre-selected; the details pane shows the first group's summary until the
> user picks an instance.
>
> **Shared detail pane (L4).** The Frame List and the FIG-Instance selection both render into the single
> `tab3_Right` tree (`m_tab3_figItemDetailsTree`), so they overwrite each other. During playback the
> frame-driven rebuild is therefore skipped while the **Frame List** tab is not the active tab; a
> user-driven Frame List selection always wins.
>
> **T42 Hex Viewer.** The `tab3_Right` dock now hosts inner tabs (`tab3RightTabs`): the existing FIG Item
> Details tree and a new **Hex Viewer** (`tab3HexViewer`). The Hex Viewer mirrors the current selection:
> a FIG Instance List child → the collector instance's `raw` bytes with a `FIG t/e · length · FIB · CRC`
> header; a Frame List row → that frame's retained bytes, preferring the **FIC/FIB region** (offset
> `12 + 4·NST`, 96 bytes / 3 FIBs) and falling back to the full frame. No-selection / no-file states show
> a guard message. It is a separate widget, so the shared-tree "two writers" limitation is unchanged.

### Tab 1 — Player transport (T29)

The `tab1_Player` panel hosts a playback controller over the retained frame cache: **Play ▶ / Pause ⏸ /
Stop ■ / Reset ⏮ / Skip ⏪ ⏩ ±10 s**, a **Speed** combo (0.25× · 0.5× · 1× · 2× · 4×), the
**Loop playback** checkbox (now wired) and a **time bar** (`playerTimeBar`, the former
`m_playerProgressBar` as a seek slider) with time + frame labels. The playhead is a `QTimer` at 24 ms per
frame for 1× (DAB Mode I), scaled by the speed multiplier; it drives the existing frame-selection path
(hex/ETI-compliance/overview + the Tab-3 frame details), throttling the expensive panels to ≈6.7 Hz
while the clock/slider/frame counter/navigator highlight update every tick. Drag/wheel/keyboard/trough
changes to the seek slider all seek (`valueChanged`); a drag uses the throttled path and one forced
rebuild runs on release. The System Messages log is capped (`setMaximumBlockCount(5000)`) and per-tick
hex/compliance log lines are suppressed while playing. The slideshow image rescales when the splitter or
a floating dock resizes it. After a load the window starts at frame 0, **paused**; before any file the
controls are disabled.

### Bottom strip — shared across tabs (7)

> **T40** removed the old hard `setMaximumHeight(120)` cap. The strip still *starts* at ≈120 px (now
> expressed as the splitter size `{880, 120}`) with a 60 px minimum, so the user can drag it larger.

| Dock objectName | Visible title | Area |
|-----------------|---------------|------|
| `bottom_SystemMessages` | System Messages | Bottom strip (live plain-text log) |
| `bottom_Performance` | Performance | Bottom strip |
| `bottom_RealTimeChart` | Real-Time Chart | Bottom strip |
| `bottom_ErrorDetection` | Error Detection | Bottom strip |
| `bottom_EtsiCompliance` | ETSI Compliance | Bottom strip |
| `bottom_Logging` | Logging | Bottom strip |
| `bottom_Constellation` | Constellation | Bottom strip |

> **T41** removed the redundant `bottom_Messages` tab (the legacy logging panel) — it was fully redundant
> with `bottom_SystemMessages`, whose `QTextEdit` is the live log. Its members and the already-unreachable
> `createLoggingTab()` were removed with it. `DOCKING_LAYOUT_VERSION` was bumped **7 → 8** so existing
> users reset once to the 7-tab default.

## 3. User-facing behavior

- **Float** — double-click a dock title bar (or drag it out of the area) to detach into a floating window;
  inside a tabbed group the whole group area floats (per-dock float keeps working as in v1.3).
- **Close** — title-bar ✕ hides the dock for the session. A closed dock returns on next launch under the
  saved layout, or in the default layout if no state was saved.
- **Rearrange** — drag dock title bars to re-tab, split, or move between columns/areas (ADS
  `CDockAreaWidget` behavior). Tab 1/2/3 rearrangements are independent per tab; the bottom strip is shared.
- **Auto-hide / collapse** — ADS auto-hide is enabled on all docks: the pin button collapses a dock into an
  auto-hide side pane and re-opens it on hover.
- **Input toolbar** — the File/Network radios + path + Browse… + Load File bar sits at the top of Tab 1;
  selecting "Network Stream" expands the network row (URL, quick examples, Connect/Disconnect/Record and the
  stream status/stats labels). All original slots remain wired (`onSourceTypeChanged`, `onBrowseFileClicked`,
  `onLoadFileClicked`, `onQuickExampleSelected`, `onConnectClicked`, `onDisconnectClicked`, `onRecordClicked`);
  the menu-bar **Input** menu was removed (T23 option A) — the menu bar is now File / View / Help, and the UDP settings live on a ⚙ button at the end of the network row.
- **Layout persistence + versioning** — on exit, each `CDockManager::saveState()` is stored alongside the
  window geometry in QSettings (`org = "StreamDAB-Analyzer"`, `app = "DABAnalyzer"`); on start,
  `restoreState()` restores it. Keys: `window/geometry`, `docking/layout_version`, `docking/tab1`,
  `docking/tab2`, `docking/tab3`, `docking/bottom`.
  - `docking/layout_version = 8` is written on save. On restore, any saved version **≠ 8** (including a
    missing key, i.e. first launch) causes the stored dock states to be **discarded once** — the redesigned
    default layout is shown and version 8 is written again at the next close. `window/geometry` is kept.
    This gives existing users exactly one reset after each layout upgrade (v3 → v4 grew the Now Playing /
    Slideshow dock from 82 px to ≈200 px; v4 → v5 replaced its inner tabs with the T27 left/right splitter,
    added the T28 Frame List inner tabs to the Tab-3 left dock, and added the T29 Player transport
    controls — v5 also discards the stale Transport group active tab so **Player** opens by default;
    v6 → v7 added the T34 Audio tab; v7 → v8 (T41/T42) removed the redundant Messages strip tab and added
    the Tab-3 right-dock Hex Viewer inner tab).
- **First launch** — no saved state ⇒ each manager builds the default layout (§1), identical to the mockup
  (`docs/gui/StreamDAB-Analyzer UI Redesign.html`).

### First-launch approximations (v2)

- Tab-1 horizontal proportions: `root->setSizes({230, 698, 272})` (left | center | right ≈ mockup 230/flex/272);
  Tab 2 `{300, 600, 300}`, Tab 3 `{400, 600}` (40% | 60%), window-level tabs/bottom 700/300 vertical split.
- Tab-1 vertical group heights (Qt-ADS 4.5.0 has no public API for per-dock vertical stretch; splitter
  sizes are **approximations** — residual differences are documented/expected):
  `leftV {178, 320}` (Transport ≈ 178, System grows), `rightV {210, 240}` (Signal Info ≈ 210,
  Error & Statistics grows), `centerV {200, 190, 150}` (Signal Analysis ≈ 190, Frame Analysis flex,
  Now Playing / Slideshow ≈ 200) with `tab1_NowPlaying` area `setMinimumHeight(200)` (bottom of center;
  T27 — the dock hosts a **left/right `QSplitter`**: left = DLS+ text block, right = MOT Slideshow image in
  a fixed **4:3 letterbox** frame; the splitter default is 45/55 and is user-draggable).
  The center column is rebuilt in our own vertical `CDockSplitter` (the Qt-ADS built one carries an empty
  bookkeeping "ghost" splitter; the clean column is swapped into the root).
- Per-panel **vertical** proportions inside each group are set by the group tabs (one tab visible at a time).

### Manual QA needed (v2.x)

- Drag / float / auto-hide interaction on the grouped layout (Tab 1) and the toolbar network row
  **must be exercised manually** — headless CI covers visibility + floating round-trip. Tracked in
  `docs/TASK_LIST.md` (T4).

## 4. Implementation notes

- One wrapped dock per original panel; **zero panel content changes** — the existing widgets/tables/trees are
  reused verbatim, only the outer `CDockWidget` wrapper and the group placement change.
- Groups are built with `CDockManager::addDockWidgetTabToArea(dock, area)` on the FIRST dock's area
  (registration order = tab order, first dock = default active tab).
- Tab-1 column structure: left/right/center areas are moved into our own vertical `CDockSplitter` columns
  (Qt-ADS has no API to stack multiple areas vertically in a Left/Right strip); tabified docks share one
  `CDockAreaWidget`, so each unique area is added to the column splitter once.
- The Input toolbar (`inputToolbar`) replaces the `tab1_InputSource` dock; its network row
  (`m_sourceInputStack`, objectName `inputToolbarNetworkRow`) is `setVisible()` from `onSourceTypeChanged()`.
- Tab page margins match the original: **Tab 1 keeps its 4 px contents margins**, Tab 2/3 use 0 px.
- The bottom `CDockManager` lives at window level inside `mainVerticalSplitter` (tabs ~70% / bottom ~30%).
- Persistence helpers centralize the QSettings org/app + key naming (incl. `docking/layout_version`).

## 5. Out of scope (documented decisions)

- **Per-tab bottom strips** — the bottom dock area stays window-level and shared, as in the original design.
- **View-menu dock toggle actions** — no in-app menu to re-show a closed dock; closing persists via the saved
  layout only.
- **Panel content changes** — panels are preserved from the original `DABAnalyzerWindow`; "rich panel content
  porting" is not needed and not attempted. The redesign is layout regrouping/relocation only (ฟังก์ชันหลักเหมือนเดิม).
- **Mockup short title** — the HTML mockup shortens "ETI Frame Navigator" to "ETI Navigator"; the xlsx and
  this redesign keep "ETI Frame Navigator" (see `docs/gui/UI_REDESIGN_PLAN.md` §1.4).

## References

- `docs/gui/UI_REDESIGN_PLAN.md` — v2 UI redesign evaluation + implementation plan (authoritative).
- `docs/TASK_LIST.md` — task checklist (T2 = v1.3 migration, T10 = v2 redesign).
- `docs/PHASE_E_PLAN.md` — main plan (v1.3/v1.4 sections + progress log).
- `docs/Qt6_CPP20_Style_Guide.md` — code style for the implementation.
- Qt Advanced Docking System 4.5.0 — `ads::CDockManager`, `ads::CDockWidget` (vendored via CMake FetchContent).