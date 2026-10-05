# MOT Slideshow (MOT Body) — Evaluation & UI Options

**Date:** 2026-10-05 · **Branch:** `main` · **Status:** evaluation only — implementation pending user's option choice
**Question:** do we already show MOT Body (slideshow images) anywhere? If not, where should the widget go?

---

## 1. Current state (verified by code inspection)

| Layer | What exists | Gap |
|---|---|---|
| **Core MOT protocol** | `src/core/mot_protocol.{hpp,cpp}` — MOTHeader / MOTDirectory / MOTSegmentHeader / **MOTObject** / SegmentBuffer (reassembly + progress + CRC), `MOTProtocol : QObject` with `processMOTData(service_id, msc_data)`, signals `objectComplete(transport_id, MOTObject)`, `reassemblyProgress`, `parseError`; `ContentType` incl. **IMAGE_JPEG (0x02) / PNG / BMP**; `MOTObject` registered as a Qt metatype | — (core is ready) |
| **Unit tests** | `tests/test_mot_protocol.cpp` — MSC data-group parsing, CRC, segment reassembly (single/multi/dup/out-of-order/missing), progress | — |
| **E2E test** | target `test_e2e_mot_slideshow` | ⚠️ **stub** — body is `QSKIP("E2E test stub")`; no real pipeline coverage |
| **GUI** | none — grep for `mot|slideshow|pixmap|QImage|setPixmap` in `src/main.cpp` = **0 hits** | ❌ no widget, no wiring |
| **Data availability in the capture** | The new DAB+ PAD parser already detects X-PAD CI **type 12 (MOT header, 2795 groups) and type 13 (MOT body, 31480 groups)** on `eti/bkk_20062022_141637.eti` → the slideshow stream is really there | assembly of those groups into MSC data groups is not done |

**Conclusion:** the decoder (MOT) and the transport detection (X-PAD 12/13) exist, but nothing assembles the PAD groups into MOT objects and **no GUI widget can render an image**. A slideshow feature is *additive*, not a rewrite.

## 2. Required work (independent of the UI choice)

1. **Adapter (PAD → MOT)**: extend the existing `DabPlusSubchannelParser` (src/main.cpp, X-PAD CI parsing): for CI types 12/13 build the EN 300 401 §5.3.2 MSC Data Group (2-byte header + segment + CRC16/CCITT) and feed `MOTProtocol::processMOTData(service_id, group)`. Type 12 = header group (transport id/session/segmentation), type 13 = body/segment group.
2. **Decoder wiring in the GUI**: create `eti::mot::MOTProtocol m_motProtocol` in `DABAnalyzerWindow`, connect `objectComplete` → slot; keep per-transport-id bookkeeping; report `reassemblyProgress` to the panel status line ("Receiving image… 65%").
3. **Image render**: `MOTObject.body` → `QImage::loadFromData(body, "JPEG")` (Qt6 base ships the JPEG plugin) → scaled-to-fit `QLabel`; keep a small list of the last N images (thumbnails) + "slide k/n" navigation if desired; show content type/size/transport id + timestamp.
4. **States & errors**: "Waiting for slideshow…", "Receiving (x %)", "No slideshow in this stream" (count check after load), decode-failure → status text; multi-service limitation noted (like DLS+).
5. **Tests**: (a) unit test for the PAD→MSC-data-group adapter (synthetic X-PAD 12/13 → `objectComplete` fires with expected body bytes); (b) **real-fixture E2E**: the Bangkok capture carries MOT → after load the widget must show an image (assert `QImage` non-null / pixmap size > 0); (c) fill the `test_e2e_mot_slideshow` stub with this real path.
6. **Docs**: update `docs/gui/QT_ADS_DOCKING.md` (registry if a new dock) and any MOT notes.

Effort estimate: adapter + wiring + widget + tests ≈ one focused work package (comparable to the DLS+ wave); risk mainly in the PAD data-group framing (well-specified, verifiable against the fixture).

## 3. UI placement options (choose one)

| # | Option | Pros | Cons | Layout impact |
|---|---|---|---|---|
| **A** | **Inner tab in `Now Playing (DLS+)`** (user's suggestion): dock becomes "Now Playing / Slideshow" with 2 inner tabs (DLS+ text · Slideshow) | No new dock/area; media content stays together (text + image); zero layout-version churn; dock count stays 25 | The dock is currently **compact (82 px fixed)** → must grow (e.g. ≥200 px) to be useful, which changes the mockup's "compact Now Playing" intent; small graphics | dock height 82 → ~200 px (layout stays v2; saved layouts keep positions) |
| **B** | **New tab in the center `Frame Analysis` group** (Navigator · Hex · ETSI · **Slideshow**) | Space is good; group already tabbed; no new area | Not media-related grouping (analysis tools); slideshow hidden behind an analysis tab | registry +1 dock (26); layout v2 unchanged |
| **C** | **New standalone dock in the center column** (below Now Playing) | Dedicated, always visible; big image | Adds a new row to tab 1 (deviates from current mockup); registry +1; saved layouts → bump `layout_version` v3 | layout change (new area/row) |
| **D** | **7th bottom-strip tab** (System Messages … Constellation · **Slideshow**) | Shared across all 3 tabs; doesn't disturb tab 1 | Strip is a fixed 28 px bar → must grow substantially to show an image; strip is for logs/stats semantically | strip height change |
| **E** | **New tab in `Signal Analysis` group** (Ensemble · Subchannel · CU · **Slideshow**) | Space fine; group = "what the ensemble carries" — defensible semantically | Mixes signal-structure panels with media content | registry +1 |

**Recommendation:** **A** if the priority is "no layout churn, media in one place" (grow the dock to ~200 px; inner tabs keep DLS+ text one click away) — this matches the user's instinct and keeps the 25-dock registry. **C** if a large, always-visible slideshow is wanted (accepting a new dock + layout v3). Option **B** is the middle ground if you prefer no height change to Now Playing.

## 4. Dependencies / sequencing note

The current uncommitted DLS+ wave (spec PAD parser, progress bar, tab styling, title fixes) has an open review with **9 findings (1 HIGH: DLS label length marker truncation for 64–127-char/Thai labels)**. The MOT work builds directly on that same X-PAD parser, so the recommended order is:
1. fix the 9 review findings → commit the DLS+/UI wave;
2. implement the MOT slideshow on top (option A/B/C) as its own commit + doc update.