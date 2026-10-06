# Dock Interaction QA — T4

**Suite:** `tests/gui/test_dock_interactions.cpp` (target `test_dock_interactions`, offscreen).
**Companion:** `tests/gui/test_gui_dock_layout.cpp` validates the *default* layout, persistence,
version reset and real-fixture panel wiring; this suite validates *interactive* dock operations.
**Design reference:** `docs/gui/QT_ADS_DOCKING.md`.

Run:

```bash
export QT_QPA_PLATFORM=offscreen
cd build && ctest -R "test_dock_interactions|test_gui_dock_layout" --output-on-failure
```

---

## 1. What the automated suite covers

Each test drives the real `DABAnalyzerWindow` (27 docks, 4 `CDockManager`s) through the Qt-ADS API
and asserts the resulting state. No fixture load; headless (offscreen) only.

| # | Test | Interaction driven | Asserted |
|---|------|--------------------|----------|
| 1 | `testFloatAndRedockTab1` | `setFloating()` on `tab1_CUUsage` (tabbed group), then re-dock into the **original** area | floating container → docked again, visible, area tab count restored, still in `dockWidgetsMap` |
| 2 | `testFloatAndRedockBottom` | same on `bottom_Performance` (7-tab strip) | idem for the bottom manager |
| 3 | `testAutoHideToggleTab1` | `setAutoHide(true)` then `setAutoHide(false)` on `tab1_NowPlaying` | auto-hide container + side-bar rail tab exist; after un-pin: container/rail gone, docked, visible, no lost dock |
| 4 | `testAutoHideToggleBottom` | idem on `bottom_Logging` | idem for the bottom manager |
| 5 | `testMoveAcrossAreasTab1` | `addDockWidget(Right/Left, tab1_System)` | leaves its original column, new area, then leaves the moved area on the way back; never closed |
| 6 | `testMoveAcrossAreasBottom` | `addDockWidget(Center/Bottom, bottom_Constellation)` | new single-dock area, then re-docked at the bottom; never closed |
| 7 | `testTabifyAndUntabify` | `addDockWidgetTabToArea(NowPlaying → System area)` then move out | one area with 2 tabs → two areas with 1 tab each |
| 8 | `testPersistenceAfterInteractions` | float `bottom_Logging` + move `bottom_Constellation` out + tabify `bottom_ErrorDetection` into it, `close()`, reopen | floating restored; tab grouping (2 tabs) restored; shared strip retains its remaining 4 tabs; separate areas |
| 9 | `testAllDocksReachableAfterInteractionsAndRestart` | float + auto-hide + move + tabify, close, reopen | all 27 docks findable by objectName; every dock `toggleView(false/true)` works; 27 docks still registered (no lost dock) |

Settings isolation: `QStandardPaths::setTestModeEnabled(true)` + per-test clearing of
`window/geometry`, `docking/layout_version`, `docking/tab1|tab2|tab3|bottom`.

---

## 2. Manual checklist (headless CI cannot exercise these)

Run the app with a real display: `./build/StreamDABAnalyzer`. Tick every box on the shipping layout
(27 docks, `DOCKING_LAYOUT_VERSION = 8`).

**Drag & drop**
- [ ] Drag a dock by its tab/title bar within the same area → it reorders / becomes current.
- [ ] Drag a dock onto another area's **center** → tabifies into a group with it.
- [ ] Drag a dock to a window **edge** (left/right/top/bottom) → drop indicator appears and it splits there.
- [ ] Drag a whole tabbed group (multi-tab) → all tabs move together, no tab lost.
- [ ] Drag a dock to the **window outside** → it floats; drag it back → re-docks cleanly.
- [ ] Drag a floating window onto an edge of the main window → re-docks, no ghost floating window remains.

**Float / restore**
- [ ] Double-click title bar / use the float button → dock floats as a resizable top-level window.
- [ ] Float each of the 4 managers' docks at least once; close the floating window via its X → dock returns (or is restorable via the View menu), never disappears permanently.
- [ ] Restart the app → floating docks reopen as floating windows at sane positions.

**Auto-hide rail**
- [ ] Pin a dock (auto-hide button) → it collapses to a labelled rail tab on the nearest edge.
- [ ] Hover the rail tab → the panel slides out over the content; move the mouse away → it collapses.
- [ ] Pin/unpin from the title bar and from the rail context menu → no duplicated rail tab, no lost dock.
- [ ] Restart → pinned docks come back pinned (rail tab present).

**Resize / splitters**
- [ ] Drag the vertical/horizontal splitters between columns and between stacked areas → both sides resize, no minimum-size thrash.
- [ ] Drag the bottom strip's top splitter → strip resizes freely (≥60 px, no 120 px cap); System Messages stays scrollable. (The strip's size is not persisted across restarts — only the workspace/dock layout is.)
- [ ] Drag the Now Playing inner splitter → DLS+ text vs 4:3 slideshow ratio changes.

**Multi-monitor / DPI**
- [ ] Float a dock onto a second monitor at a different DPI → no clipped tabs, no unreadable text.
- [ ] Move the main window between monitors with the layout loaded → docks stay inside the window.
- [ ] Change the system scale (e.g. 100 % → 200 %) and restart → tabs/titles remain readable.

**Visual sanity while floating**
- [ ] Float the **Now Playing / Slideshow** dock → the MOT slideshow stays 4:3 letterboxed with no stretching/cropping, and DLS+ text wraps.
- [ ] Float the **Audio** dock (Transport group) → L/R peak meters and volume/mute still update during playback and are not clipped.
- [ ] Float the **Real-Time Chart** / **Constellation** / **Performance** docks → repaint continues, no frozen plot.
- [ ] Thai UTF-8 labels (ensemble/service/DLS+) render correctly in a floated dock (no tofu).

---

## 3. Gates (T4)

- Build: 0 errors / 0 warnings.
- `ctest -R "test_dock_interactions|test_gui_dock_layout"` → pass.
- Full idle `ctest` → 54/54.
- Fixture parity unchanged (DLS+ 1114, MOT 97/97, 0 MOT warnings) — asserted by `test_gui_dock_layout`.
- GUI smoke `timeout 10 ./build/StreamDABAnalyzer eti/bkk_20062022_141637.eti` → exit **124** (still running, no crash).
- CLI: exit 0 on the fixture; YAML decode output unchanged.
