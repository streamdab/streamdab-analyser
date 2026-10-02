# Qt-ADS Docking — v1.3 GUI Migration Design

**@brief** Design + usage notes for the v1.3 migration of `DAB-XPlorer` ("StreamDAB Analyzer", Qt6/C++20) to
the Qt Advanced Docking System 4.5.0. The approach is **layout-preserving**: the original
`DABAnalyzerWindow` panel layout from `src/main.cpp` is restored and each existing named panel is wrapped
in an `ads::CDockWidget` — no placeholder panels, no layout redesign. Code-style conventions live in
`docs/Qt6_CPP20_Style_Guide.md`; this document is the design counterpart.

**Branch:** `feature/gui-qtads` · **Runnable:** `./build/StreamDABAnalyzer` (CMake target
`DABAnalyzer_Enhanced_FilteringBuild` keeps its name for tests, `OUTPUT_NAME = StreamDABAnalyzer`).

**Revision:** 2026-10-02 — earlier placeholder-panel experiment rejected (it changed the layout too much);
superseded by this layout-preserving plan.

---

## 1. Architecture

Four independent `ads::CDockManager` instances:

| # | Manager | Host | Docks | QSettings key |
|---|---------|------|-------|---------------|
| 1 | tab1 manager | Tab "📡 DAB Ensemble & ETI Analysis" | 16 | `docking/tab1` |
| 2 | tab2 manager | Tab "🔬 FIC-Analyzer" | 3 | `docking/tab2` |
| 3 | tab3 manager | Tab "📊 FIC-Extractor" | 2 | `docking/tab3` |
| 4 | bottom manager | Window-level bottom strip (shared across tabs) | 5 | `docking/bottom` |

The top `QTabWidget` keeps its 3 original tabs. Each **tab page owns its own `CDockManager`**, so every tab
can be floated/rearranged independently and its state persists separately. The bottom strip is a
**window-level** dock area pinned below the tabs — shared by all tabs, exactly like the original
`bottomTabs` widget.

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
|  |  Each tab page hosts ONE ads::CDockManager:                 |  |
|  |   Tab 1 → CDockManager #1: 3-column layout, 16 docks        |  |
|  |   Tab 2 → CDockManager #2: left/center/right, 3 docks       |  |
|  |   Tab 3 → CDockManager #3: left/right, 2 docks              |  |
|  +--------------------------------------------------------------+  |
|  +--------------------------------------------------------------+  |
|  | Bottom strip — window-level CDockManager #4   (~30% height) |  |
|  | 5 tabbed CDockWidgets, SHARED across all tabs:              |  |
|  |  System Messages | Performance | Error Detection |          |  |
|  |  Logging | Constellation                                   |  |
|  +--------------------------------------------------------------+  |
+--------------------------------------------------------------------+
```

### Tab 1 default dock arrangement (CDockManager #1)

```
+---------------------+---------------------------+----------------------+
| LEFT  (4 docks)     | CENTER (6 docks)          | RIGHT (6 docks)      |
|                     |                           |                      |
| Player              | Input Source              | ETI Overview         |
| Decoder             | DAB Ensemble Explorer     | DAB Error Counter    |
| System              | ETI Frame Navigator       | Status               |
| Messages            | Hex Viewer / ETSI Comp.   | Timing               |
|                     | Subchannel Organization   | CU Usage             |
|                     | Now Playing (DLS+)        | DAB Stream Statistics|
+---------------------+---------------------------+----------------------+
```

The default is built programmatically per manager and reproduces the original screenshot layout exactly
(see registry below for objectName → visible title → area mapping).

## 2. Panel registry (all 26 docks)

Dock `objectName` is used for QSettings persistence and auto-hide identity; the visible title reproduces the
original panel label where one existed. For tab 2/3 panels (which bundled several `QGroupBox`es), the visible
title is the original column name (`Left`/`Center`/`Right`) exactly as passed to `ads::CDockWidget` in
`src/main.cpp` — not a content label.

### Tab 1 — 📡 DAB Ensemble & ETI Analysis (16)

| Dock objectName | Visible title | Column / area |
|-----------------|---------------|---------------|
| `tab1_Player` | Player | Left |
| `tab1_Decoder` | Decoder | Left |
| `tab1_System` | System | Left |
| `tab1_Messages` | Messages | Left |
| `tab1_InputSource` | Input Source | Center |
| `tab1_EnsembleExplorer` | DAB Ensemble Explorer | Center |
| `tab1_FrameNavigator` | ETI Frame Navigator | Center |
| `tab1_HexCompliance` | Hex Viewer / ETSI Compliance | Center |
| `tab1_SubchannelOrg` | Subchannel Organization | Center |
| `tab1_NowPlaying` | Now Playing (DLS+) | Center |
| `tab1_ETIOverview` | ETI Overview | Right |
| `tab1_ErrorCounter` | DAB Error Counter | Right |
| `tab1_Status` | Status | Right |
| `tab1_Timing` | Timing | Right |
| `tab1_CUUsage` | CU Usage | Right |
| `tab1_StreamStats` | DAB Stream Statistics | Right |

### Tab 2 — 🔬 FIC-Analyzer (3)

| Dock objectName | Visible title | Column / area |
|-----------------|---------------|---------------|
| `tab2_Left` | Left | Left |
| `tab2_Center` | Center | Center |
| `tab2_Right` | Right | Right |

### Tab 3 — 📊 FIC-Extractor (2)

| Dock objectName | Visible title | Column / area |
|-----------------|---------------|---------------|
| `tab3_Left` | Left | Left |
| `tab3_Right` | Right | Right |

### Bottom strip — shared across tabs (5)

| Dock objectName | Visible title | Area |
|-----------------|---------------|------|
| `bottom_SystemMessages` | System Messages | Bottom strip |
| `bottom_Performance` | Performance | Bottom strip |
| `bottom_ErrorDetection` | Error Detection | Bottom strip |
| `bottom_Logging` | Logging | Bottom strip |
| `bottom_Constellation` | Constellation | Bottom strip |

## 3. User-facing behavior

- **Float** — double-click a dock title bar (or drag it out of the area) to detach into a floating window.
- **Close** — title-bar ✕ hides the dock for the session. There is **no in-app "re-show" action in v1.3**
  (View-menu toggles are out of scope) — a closed dock returns on next launch under the saved layout, or in
  the default layout if no state was saved.
- **Rearrange** — drag dock title bars to re-tab, split, or move between columns/areas (ADS
  `CDockAreaWidget` behavior). Tab 1/2/3 rearrangements are independent per tab; the bottom strip is shared.
- **Auto-hide / collapse** — ADS auto-hide is enabled on all docks: the pin button collapses a dock into an
  auto-hide side pane and re-opens it on hover.
- **Layout persistence** — on exit, each `CDockManager::saveState()` is stored alongside the window geometry
  in QSettings (`org = "StreamDAB-Analyzer"`, `app = "DABAnalyzer"`); on start, `restoreState()` restores it.
  Keys: `window/geometry`, `docking/tab1`, `docking/tab2`, `docking/tab3`, `docking/bottom`.
- **First launch** — no saved state ⇒ each manager builds the default layout (section 1), identical to the
  original pre-migration screenshot.

### First-launch approximations (v1.3)

- Horizontal column proportions are reproduced exactly: Tab 1 `root->setSizes({300, 600, 300})`
  (left | center | right), Tab 2 `{300, 600, 300}`, Tab 3 `{400, 600}` (40% | 60%), and the window-level
  tabs/bottom 700/300 vertical split.
- Per-panel **vertical** proportions inside each column are **approximated** as equal-height stacks for v1.3
  (Qt-ADS 4.5.0 has no public API to set per-dock vertical stretch on first launch). Accepted trade-off for
  this release; panel proportions remain visible in the original panel widgets' internal `QSplitter`s.

### Manual QA needed (v1.3.x)

- Drag / float / auto-hide interaction on the post-surgery layout (Tab 1's 3-column splitter surgery and
  Tab 2/3 root splits) **must be exercised manually** — headless CI covers visibility + floating round-trip
  only. Tracked as a follow-up in `docs/TASK_LIST.md` (T4).

## 4. Implementation notes

- One wrapped dock per original panel; **zero panel content changes** — the existing widgets/tables/trees are
  reused verbatim, only the outer `CDockWidget` wrapper is added (plus auto-hide feature flags).
- Tab page margins match the original: **Tab 1 keeps its 4 px contents margins** (`setContentsMargins(4,4,4,4)`
  on the tab page layout), while Tab 2/3 pages use 0 px margins around their dock managers.
- Each tab page's widget is the `CDockManager` itself; tab pages are added to the top `QTabWidget` unchanged.
- The bottom `CDockManager` lives at window level inside `mainVerticalSplitter` (tabs ~70% / bottom ~30%),
  pinned as the splitter's second pane.
- Persistence helpers centralize the QSettings org/app + key naming so tab1/tab2/tab3/bottom states round-trip
  independently.

## 5. Out of scope for v1.3 (documented decisions)

- **Per-tab bottom strips** — the bottom dock area stays window-level and shared, as in the original design.
- **View-menu dock toggle actions** — no in-app menu to re-show a closed dock; closing persists via the saved
  layout only.
- **Panel content changes** — panels are preserved from the original `DABAnalyzerWindow`; "rich panel content
  porting" is not needed and not attempted.

## References

- `docs/TASK_LIST.md` — v1.3 task checklist (T2 = this migration, revised 2026-10-02).
- `docs/PHASE_E_PLAN.md` — main plan (v1.3 section + progress log).
- `docs/Qt6_CPP20_Style_Guide.md` — code style for the implementation.
- Qt Advanced Docking System 4.5.0 — `ads::CDockManager`, `ads::CDockWidget` (vendored via CMake FetchContent).