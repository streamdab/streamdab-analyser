# `src/gui/` Audit — Unused GUI Sources & Placement Proposal

**Date:** 2026-10-06 · **Branch:** `main` · **Method:** read-only audit (CMake targets × instantiation greps × header/cpp consistency)
**Headline:** **no file under `src/gui/` is compiled by any configured CMake target, and `src/main.cpp` includes none of its headers.** The whole directory is a stale pre-v1.3 generation ("Modern ETI Core Engine" / "Phase 4 streaming"); the running app builds its 25-dock GUI inline in `src/main.cpp` on `src/core` only.

## Evidence
- `CMakeLists.txt:35-68` (`SOURCES`) contains **zero** `src/gui` files; `add_executable` at `:88`.
- `src/main.cpp:1-84` includes only Qt + `src/core/*` + Qt-ADS.
- The one CMake file listing `src/gui` sources is **orphaned**: `tests/e2e/CMakeLists.txt:20-33` (`E2E_LIB_SOURCES`), and `tests/CMakeLists.txt` never `add_subdirectory(tests/e2e)`.
- The compiled GUI test pulls the real app: `tests/gui/test_gui_dock_layout.cpp:64` → `#include "../src/main.cpp"`.
- Several files cannot even compile against their own headers (legacy `eti_processor` vs `EtiProcessor`): `etsi_compliance_monitor.{h:123,cpp:341}`, `service_explorer_panel.{h:103,cpp:523}`, `performance_dashboard.{h:93,cpp:559}`, `context_menu_factory.{h:83,cpp:9}`, `integration_validator.cpp:20` (missing `core/eti_processor.h`), `test_gui_integration.cpp:41` (missing header).

## Placement proposal — shippable but unshown widgets

| Widget | Recommended location | Data source | Effort | Overlap note |
|---|---|---|---|---|
| `constellation_widget` | **replace the bottom-strip "Constellation" tab** (currently a dashed placeholder label) | `m_etiProcessor` perf signals — ETI is post-demod, so only the synthetic/`generatePerformanceBasedConstellation()` path is honest | S | supersedes the placeholder |
| `performance_dashboard` | **replace the thin "Performance" strip tab** (3 labels + progress bar today) | `m_etiProcessor` / `m_figAnalyser` FPS, memory, ETSI counters | M (adapt legacy types) | clear upgrade of the existing thin tab |
| `about_dialog` | **Help → About** (replace `QMessageBox::about`) | static version/system info | S | direct replacement |
| `real_time_chart_widget` | new bottom-strip tab (next to Performance) | `m_networkReceiver->get_statistics()`, processor FPS/memory | S/M | no current equivalent |
| `settings_dialog` | new **File → Settings…** | `analyser_settings.hpp` (already compiled) | M | complements the existing analyser settings |
| `service_explorer_panel` (or `service_browser`, pick one) | Tab-1 **DAB Ensemble Explorer** dock / Tab-2 **Service Tree** | `m_figAnalyser->getDABServices()` | M | duplicates the existing Ensemble Explorer + Tab-2 Service Tree — pick one, do not adopt both |
| `fig_analysis_widget` | Tab-2 **FIG Content** or a Tab-3 tab | `m_figAnalyser` | M/L (expects legacy analyser type) | duplicates Tab-2/3 FIG views |
| `etsi_compliance_monitor` | inner tab of "Hex Viewer / ETSI Compliance" or the Error-Detection strip tab | `ETSIComplianceEngine` + `AdvancedErrorDetector` | M | duplicates existing compliance viewer |
| `professional_export_manager` | new **File → Export…** | `m_figAnalyser` services/FIGs, frame store | M | net-new capability |
| `audio_monitoring_widget` + `audio_decoder_integration` | Tab-1 Transport group (or strip) | blocked on T34 audio-output route | L | needs libfaad output first |
| infrastructure (`status_manager`, `progress_manager`, `error_indicator_system`, `signal_router`, `configuration_manager`, `real_time_controller`) | behind existing Status/Timing docks | processor signals | L | replaces ad-hoc label updates; low user payoff, high churn |
| `eti_frame_list_model`, `eti_service_tree_model` | future Tab-3 model/view refactor | `m_figAnalyser` / parser | L | currently widget-based; models bind to legacy types |

## Dead / obsolete → delete (with reason)

| Files | Reason |
|---|---|
| `stream_recording_dialog.*`, `alert_configuration_dialog.*` | pure stubs (every method "placeholder"), depend on uncompiled core |
| `test_gui_integration.cpp` | stale demo with its own `main()`, includes a nonexistent header |
| `context_menu_factory.*` | header/cpp API divergence — cannot compile |
| `optimized_gui_renderer.h`, `ui/main_window.ui` | never implemented / never referenced |
| `integration_validator.*`, `professional_workflow_manager.*` | QA/orchestrator against a non-existent `MainWindow` API |
| `streaming_dashboard.*`, `connection_config_dialog.*`, `ui_integration_bridge.*` | built for the uncompiled "StreamingEngine" stack; the app already has its own network toolbar + UDP dialog |
| `analyser_widget.*`, `eti_analysis_widget.*` | superseded by the inline Tab-1 layout; still contains TODO stubs |
| `main_window.h` | duplicate/stale `DABAnalyserWindow` declaration (real class is in `src/main.cpp`) |
| `tests/e2e/CMakeLists.txt:20-33` | orphan list that falsely implies `src/gui` is in the build — clean in the same pass |

## Recommended disposition
1. **Low-risk wins:** Constellation widget → strip tab; About dialog; real-time chart tab; Settings dialog.
2. **Upgrades with known data sources:** Performance dashboard; ETSI monitor; FIG analysis; one service browser (pick one); export manager.
3. **Defer:** audio monitoring (T34).
4. **Delete/archive:** the list above (+ the orphan CMake list).