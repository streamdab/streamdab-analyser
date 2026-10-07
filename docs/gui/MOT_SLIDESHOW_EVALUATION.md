# MOT Slideshow (MOT Body) — Evaluation & UI Options

**Date:** 2026-10-05 · **Branch:** `main` · **Status:** ✅ **DECISION: option A** (2026-10-05) — **IMPLEMENTED** (2026-10-05)
**Question:** do we already show MOT Body (slideshow images) anywhere? If not, where should the widget go?

> **Decision (user, 2026-10-05): Option A** — inner tabs in the now-playing dock
> (`Now Playing (DLS+)` → `DLS+ Text` · `Slideshow`), dock height grown from 82 px to ≈200 px.
> No new dock; registry stays at 25. Implementation follows the DLS+ findings close-out.
>
> **Update 2026-10-06 — T27: inner tabs → left/right split.** The inner `QTabWidget` was removed; the
> Now Playing / Slideshow dock now hosts a draggable horizontal `QSplitter` (default 45/55) with the
> DLS+ text block on the left and the MOT Slideshow on the right. The slideshow image lives in a fixed
> **4:3 letterbox** frame, so the picture area stays 4:3 at any splitter size. All DLS+/MOT state and
> decoding logic is unchanged; the dock keeps its ≈200 px minimum height. Tests assert the splitter +
> its two child panels (`dlsPanel` / `slideshowPanel`) instead of the old inner tabs.
>
> **Implemented (2026-10-05):**
> - `src/core/mot_pad_adapter.{hpp,cpp}` — parses standard EN 300 401 MOT MSC data groups
>   (header entity + body entity, out-of-order/gap tolerant) and re-frames completed objects into
>   the layout consumed by `MOTProtocol` (recursive-mutex deadlock in `MOTProtocol::processMOTData`
>   fixed along the way). Bounded accumulation (running byte counter + 2 MiB cap, stale/LRU
>   assembly eviction), carousel new-version reset (changed header core), full 16-bit transport id
>   end to end (`MOTProtocol::validateTransportId` widened to 0..0xFFFF).
> - `src/core/mot_protocol.cpp` — `parseMOTHeader()` now bounds the extension-parameter walk by
>   `header_size` and reconciles the (re-framed) body size, so the fixture load is **warning-free**.
> - `src/main.cpp` DAB+ PAD parser extended with X-PAD CI 1 (DGLI) + CI 12/13 assembly (continuation
>   length = the MOT sub-field, not the whole X-PAD), routed to `MOTProtocol` via the adapter; Now
>   Playing dock got the inner `QTabWidget` + Slideshow tab and grew to ≈200 px;
>   `DOCKING_LAYOUT_VERSION` bumped 3 → 4.
> - Real capture `eti/bkk_20062022_141637.eti`: 2795 CI-1 / 2795 CI-12 / 31480 CI-13 groups →
>   2773 standard data groups, 112 header + 2661 body segments, **97 objects, 97 images decoded**,
>   0 CRC errors, **0 MOT-path warnings**, max body 55703 B, 0 stale evictions; DLS+ unchanged
>   (1114 labels).
> - Tests: `test_mot_protocol` (+9 adapter tests: header core, object, multi-segment, CRC reject,
>   re-framed layout + validity, oversize, stale eviction, carousel reset), `test_gui_dock_layout`
>   (`testMotPadAdapterXpadPipeline`, `testMotXpadCiFlagZeroContinuation`, `testMotSlideshowPanelAndTabs`,
>   `testMotSlideshowRealFixture`), `test_e2e_mot_slideshow` (stub replaced). Full `ctest`: 50/50.
>
> **Update 2026-10-05 — T26 retired the re-framing.** The header/body re-framing
> described above (`MotPadAdapter::buildReframedDataGroup` + the synthesized
> legacy 7-byte core) is **deleted**. `MOTProtocol::processMOTData()` now parses
> the standard EN 300 401 §5.3.3 data group end to end (session header §5.3.3.1,
> segmentation header §5.1.1, header entity type 3 / body entity type 4, standard
> 28-bit/13-bit core §6.1, extension parameters §6.2, one's-complement
> CRC-16/CCITT-FALSE), and `MotPadAdapter` is a thin façade. Review follow-up
> fixed the header-extension **PLI model** (EN 301 234 §6.2 Fig.22:
> 00→len 0, 01→8-bit, 10→32-bit, 11→7/15-bit), the **parameter IDs**
> (§6.2.2.2 Table 5) and the **ContentName charset byte** (§6.2.2.1.1 Fig.24),
> so the fixture's parameters are recovered: 97 objects carry ContentName +
> TriggerTime (e.g. `ThaiPBS_SLS_image_1.jpg`, `GW_106.5_SLS_image_1.jpg`,
> `KU Radio.jpg`; JPEG type 0x02). Fixture parity is unchanged (2773 DGs, 0 CRC,
> 112 header + 2661 body segments, 97 objects → 97 images, 0 MOT errors,
> DLS+ 1114); `ctest` 51/51.

---

## 1. Pre-implementation state (historical snapshot — SUPERSEDED)

> **NOTE:** this section is the original 2026-10-05 reconnaissance snapshot
> written *before* the feature existed. It is retained for the historical
> record only; the "Implemented (2026-10-05)" block and the **T26** update at
> the top of this document describe the current reality (standard-layout
> `MOTProtocol`, thin `MotPadAdapter` façade, slideshow dock implemented).

| Layer | What exists | Gap |
|---|---|---|
| **Core MOT protocol** | `src/core/mot_protocol.{hpp,cpp}` — `MOTHeader` / `MOTDirectory` / **`MOTObject`** and the standard-layout reassembler (header/body entity maps + progress + CRC), `MOTProtocol : QObject` with `processMOTData(service_id, msc_data_group)`, signals `objectComplete(transport_id, MOTObject)`, `reassemblyProgress`, `parseError`; `ContentType` incl. **IMAGE_JPEG (0x02) / PNG / BMP**; `MOTObject` registered as a Qt metatype. **T26:** consumes the standard EN 300 401 §5.3.3 MSC data group directly (no `MOTSegmentHeader`/`SegmentBuffer` legacy types, no re-framing) | — (core is ready) |
| **Unit tests** | `tests/test_mot_protocol.cpp` — MSC data-group parsing, CRC, segment reassembly (single/multi/dup/out-of-order/missing), progress | — |
| **E2E test** | target `test_e2e_mot_slideshow` | ⚠️ **stub** — body is `QSKIP("E2E test stub")`; no real pipeline coverage |
| **GUI** | none — grep for `mot|slideshow|pixmap|QImage|setPixmap` in `src/main.cpp` = **0 hits** | ❌ no widget, no wiring |
| **Data availability in the capture** | The new DAB+ PAD parser already detects X-PAD CI **type 12 (MOT header, 2795 groups) and type 13 (MOT body, 31480 groups)** on `eti/bkk_20062022_141637.eti` → the slideshow stream is really there | assembly of those groups into MSC data groups is not done |

**Conclusion:** the decoder (MOT) and the transport detection (X-PAD 12/13) exist, but nothing assembles the PAD groups into MOT objects and **no GUI widget can render an image**. A slideshow feature is *additive*, not a rewrite.

## 2. Required work (independent of the UI choice)

1. **Adapter (PAD → MOT)**: extend the existing `DabPlusSubchannelParser` (src/main.cpp, X-PAD CI parsing): for CI types 12/13 build the EN 300 401 §5.3.3 MSC Data Group (2-byte header + session/segmentation headers + CRC16/CCITT; X-PAD framing §7.4.2) and feed `MOTProtocol::processMOTData(service_id, group)`. Type 12 = header group (transport id/session/segmentation), type 13 = body/segment group.
2. **Decoder wiring in the GUI**: create `eti::mot::MOTProtocol m_motProtocol` in `DABAnalyserWindow`, connect `objectComplete` → slot; keep per-transport-id bookkeeping; report `reassemblyProgress` to the panel status line ("Receiving image… 65%").
3. **Image render**: `MOTObject.body` → `QImage::loadFromData(body, "JPEG")` (Qt6 base ships the JPEG plugin) → scaled-to-fit `QLabel`; keep a small list of the last N images (thumbnails) + "slide k/n" navigation if desired; show content type/size/transport id + timestamp.
4. **States & errors**: "Waiting for slideshow…", "Receiving (x %)", "No slideshow in this stream" (count check after load), decode-failure → status text; multi-service limitation noted (like DLS+).
5. **Tests**: (a) unit test for the PAD→MSC-data-group adapter (synthetic X-PAD 12/13 → `objectComplete` fires with expected body bytes); (b) **real-fixture E2E**: the Bangkok capture carries MOT → after load the widget must show an image (assert `QImage` non-null / pixmap size > 0); (c) fill the `test_e2e_mot_slideshow` stub with this real path.
6. **Docs**: update `docs/gui/QT_ADS_DOCKING.md` (registry if a new dock) and any MOT notes.

Effort estimate: adapter + wiring + widget + tests ≈ one focused work package (comparable to the DLS+ wave); risk mainly in the PAD data-group framing (well-specified, verifiable against the fixture).

## 3. UI placement options (choose one) — historical, superseded by T27

> **Historical (2026-10-05).** Option **A** below was implemented with inner tabs and then **superseded by T27
> (2026-10-06)**: the two inner tabs were replaced by a left/right `QSplitter` (DLS+ text | 4:3 letterboxed
> MOT Slideshow). The table is retained as the decision record.

| # | Option | Pros | Cons | Layout impact |
|---|---|---|---|---|
| **A** | **Inner tab in `Now Playing (DLS+)`** (user's suggestion; *superseded by T27's left/right split*): dock becomes "Now Playing / Slideshow" with 2 inner tabs (DLS+ text · Slideshow) | No new dock/area; media content stays together (text + image); dock count stays 25 | The dock is currently **compact (82 px fixed)** → must be grown (≥200 px) to be useful, which changes the mockup's "compact Now Playing" intent; small graphics | dock height 82 → ~200 px; `docking/layout_version` **v3 → v4** so the old 82 px geometry is re-applied |
| **B** | **New tab in the centre `Frame Analysis` group** (Navigator · Hex · ETSI · **Slideshow**) | Space is good; group already tabbed; no new area | Not media-related grouping (analysis tools); slideshow hidden behind an analysis tab | registry +1 dock (26); layout v2 unchanged |
| **C** | **New standalone dock in the centre column** (below Now Playing) | Dedicated, always visible; big image | Adds a new row to tab 1 (deviates from current mockup); registry +1; saved layouts → bump `layout_version` v3 | layout change (new area/row) |
| **D** | **7th bottom-strip tab** (System Messages … Constellation · **Slideshow**) | Shared across all 3 tabs; doesn't disturb tab 1 | Strip is a fixed 28 px bar → must grow substantially to show an image; strip is for logs/stats semantically | strip height change |
| **E** | **New tab in `Signal Analysis` group** (Ensemble · Subchannel · CU · **Slideshow**) | Space fine; group = "what the ensemble carries" — defensible semantically | Mixes signal-structure panels with media content | registry +1 |

**Recommendation (historical, 2026-10-05):** **A** if the priority is "no layout churn, media in one place" (grow the dock to ~200 px; inner tabs keep DLS+ text one click away) — this matches the user's instinct and keeps the 25-dock registry. **C** if a large, always-visible slideshow is wanted (accepting a new dock + layout v3). Option **B** is the middle ground if you prefer no height change to Now Playing. **Superseded:** T27 later turned the option-A inner tabs into a left/right splitter.

## 4. Dependencies / sequencing note

The current uncommitted DLS+ wave (spec PAD parser, progress bar, tab styling, title fixes) has an open review with **9 findings (1 HIGH: DLS label length marker truncation for 64–127-char/Thai labels)**. The MOT work builds directly on that same X-PAD parser, so the recommended order is:
1. fix the 9 review findings → commit the DLS+/UI wave;
2. implement the MOT slideshow on top (option A/B/C) as its own commit + doc update.