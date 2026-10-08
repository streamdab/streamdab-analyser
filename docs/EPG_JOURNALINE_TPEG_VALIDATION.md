# EPG / Journaline / TPEG — Data-Path Validation (v1.4, phase 1)

Status: **complete for this wave** (data-path wave 1 of the EPG/Journaline/TPEG
track). Coverage claims are scoped per the v1.4 user decision: **TPEG/TEPG is
base/partial only**. Everything below was validated against real World DAB
captures in `eti/` (PRIVATE test half); nothing synthetic.

---

## 1. What the captures carry

### 1.1 `eti/bkk_20062022_141637.eti` — Bangkok DAB+ (5001 frames)

| Subchannel | Bit rate | Contents (observed) |
|---|---|---|
| SCId 0–13 | 64 kb/s each | DAB+ audio (validated by `test_dabplus_audio_decoder`) |
| **SCId 14** | **32 kb/s** | **EPG data service — declared but effectively idle** |
| **SCId 22** | **8 kb/s** | **TEPG data service — TPEG2-style display carousel** |

**EPG (SCId 14).** The service is signalled (FIG 1/5 data-service label `EPG`,
SId/Cf 0xf320). The payload is NOT decodable TS 102 371 EPG in this capture:

- ~88 % of the 96-byte capacity-unit frames are a fixed idle-padding pattern
  (`CC 00 00 … 00 43 10`); the pattern repeats in every idle frame.
- The remaining ~12 % of frames carry packet-mode traffic (marker patterns
  such as `03 FE …`, `07 FE …`, `0B FE …` at 24-byte boundaries) but no
  complete, CRC-valid MSC data group set reconstructable as MOT/EPG.
- Feeding the real bytes to `EPGDecoder::processMOTObject` **honestly
  rejects** them (no fabricated schedules), yielding **0 events / 0 schedules**.

Conclusion: *bkk contains no decodable EPG schedule in the recorded window.*
The EPG decoder, the data-subchannel tap and the PRIVATE test all report this
rather than inventing data.

**What the EPG path currently validates — non-fabrication ONLY (A-M2).** The
real gate is decoder-side: `epg_decoder.cpp` (`validateMJD`, ~:142-146)
rejects implausible dates before an event can be created, and
`processMOTObject()` accepts only a complete, CRC-valid MOT object — that is
what makes "0 events" an honest result rather than a lucky one. **Stream →
MSC data-group → MOT reassembly is NOT implemented**: the raw subchannel
bytes are handed to the decoder as captured, so no schedule can be assembled
from them today. Full TS 102 371 validation of a real schedule is deferred
together with that reassembler — there is no reassembler in this path, and
the earlier "wiring seam" wording must not be read as if one existed. See
§6.

**TEPG (SCId 22).** 24-byte frames (3 CUs × 8 bytes), stream-mode (no DAB
packet headers). Observed framing:

```
frame@  0: 10 01 13 03 04 02 01 01 03 01 01 04 01 00 00 00 1f 04 54 50 45 47 10 ee
frame@ 24: 20 01 13 19 54 50 45 47 20 54 45 43 2b 54 46 50 2b 50 4b 49 2b 45 da 37
frame@ 48: 30 01 13 4d 49 20 64 65 6d 6f 03 00 15 ed 01 03 54 45 43 02 03 54 b6 55
frame@ 72: 00 01 13 46 50 03 03 50 4b 49 04 03 45 4d 49 0b 00 38 37 5b c2 a9 e1 cc
```

- A 96-byte / 4-frame rotation repeats with marker prefixes
  `10 01 13` / `20 01 13` / `30 01 13` / `00 01 13` (+ variants with a group
  low nibble, e.g. `14 01 11`, `1C 01 11`).
- **No TS 102 894-1 sync byte pair `0A 46` occurs anywhere** in the subchannel
  (verified over all ~120000 captured bytes).
- The stream is a **TPEG2-style display carousel** of short traffic messages:
  - application banner: `TPEG TEC+TFP+PKI+E`
  - service description: `2022 bmt TPEG ON AI`
  - version marker: `R-3.1-wip|000078113`
  - parking / traffic texts: `BTS Mo Chit Parking`, `PTT Pracha Uthit Rd,
    Khwaeng Samsri … Bangkok 10320, Thailand`, `999, 2-3 Pracha Uthit`, etc.

The same text set repeats (36 unique runs over 2662 visible runs in the full
capture) — a carousel, not a one-shot payload.

### 1.2 `eti/HessischerRundfunk-Warntag2024-2024-09-12_journalline/` — "hr Radio" EId 0x107a

The fixture is primarily an **EWS Warntag 2024** recording (alert signalling via
FIG 0/19, alert audio on SCId 12 = "hr3 Rhein-Main"; see `HR_20240912.txt`).
Its name contains "journalline"; empirically **SCId 8 (32 kb/s) is the
Journaline-capable data subchannel** (the only non-DAB+-audio subchannel, at
96 bytes/frame), and it carries a **stream-mode Journaline display carousel**:

```
CIF@…: … c0 02 5b 34 02 0f 68 72 34 20 2d 20 42 72 69 74 74 61 20 61 6d …
       → "hr4 - Britta am"
       … 00 01 13 46 50 03 03 50 4b 49 04 03 45 4d 49 0b 00 38 37 5b …
       … "Die ARD-Hitnacht ist eine Produktion des MDR für zwölf
          Radiowellen deutschlandweit …"
```

Real decoded items include titles (≤26 chars: `hr4 - Britta am Vormittag`,
`hr4 - Mein Morgen in Hessen`, `ARD - Hitnacht`) and longer article bodies
(`Die ARD-Hitnacht …`, `Musikalisch liegt der Schwerpunkt … German utf-8
umlauts preserved`). The data is **not** wrapped in standard MOT data groups
(no CRC-valid `0x7X` groups under the EN 300 401 header/length interpretation),
so the JournalineDecoder consumes it through the new `processStreamData()`
path (display-text extraction).

**Re-baselined after the A-H1 plausibility gate (Phase 2 probe, 4 000-frame
slice = 384 000 bytes):**

| metric | value |
|---|---|
| candidate runs (≥6-byte text runs) | **11 544** |
| items passing the plausibility gate | **3 014** |
| pass rate | **26.1 %** |

Before the gate the same scan emitted ~11.4 k items of which ≈71 % were
binary garbage (no ≥4-letter word and/or `U+FFFD`; code-review measurement on
this capture) — that ratio is why the routing probe uses the *post-gate*
share (`streamItemCount() / streamCandidateCount()`), not an item count.

---

## 2. Gaps found in the existing decoders + fixes

### `JournalineDecoder` (`src/core/journaline_decoder.*`)
- **Gap:** `processMOTObject()` assumed a synthetic private byte layout; real
  stream-mode Journaline (as broadcast by the HR mux) never reaches it.
- **Fix (additive):** `bool processStreamData(const QByteArray&)` extracts the
  human-visible carousel text (titles ≤ 26 chars, bodies > 26 chars) with
  UTF-8/Thai preservation, stores them as `JournalineObject`s and exposes a
  `Display Carousel` menu (`0x7FFF`). Public API and the 45 synthetic MOT
  tests are untouched.
- **File:line (wave):** `journaline_decoder.hpp` `processStreamData`
  declaration; `journaline_decoder.cpp` `scanStreamTailLocked()`.
- **Explicitly deferred:** full TS 102 979 Object-Element parsing (LevelId /
  object headers / category links) — the HR capture does not expose those
  headers in a form this wave could verify.

### `EPGDecoder` (`src/core/epg_decoder.*`)
- **Gap found:** none in decode logic for real data — because **no real EPG
  schedule exists in bkk** (see §1.1). The decoder's MJD→QDateTime, event
  layout and Thai text handling were already sound.
- **Fix (verification/coverage):** the capture-backed PRIVATE test feeds the
  real SCId 14 bytes and asserts the honest rejection path (0 events,
  `epgDecodingError` emitted). No fabricated events.

### New `TpegDecoder` (`src/core/tpeg_decoder.*`)
- Implements the base/partial TPEG coverage (see §3).
- Emits `tpegFramingDetected()` with the empirically observed framing and
  `tpegMessageDecoded()` per unique inventory entry.

---

## 3. TPEG coverage statement (per user decision)

> **TPEG: base — TS 102 894-1-style transport framing identification +
> the actual application observed in the Bangkok TEPG capture (a TPEG2-style
> carousel carrying TEC/TFP/PKI texts) decoded at the text-inventory level
> (partial; full TTI/RTM later).**

Covered in this wave:
1. stream-framing identification (24-byte frame cadence, 4-phase markers);
2. sync-pair reporting — **explicitly reports that 0x0A 0x46 is absent**;
3. deduplicated text-inventory of the observed messages;
4. application-family classification (TEC / TFP / PKI / EMI / banner /
   version) from the observed content.

Deferred (explicitly **not** claimed):
- full TS 102 894-2 TTI binary message-component decoding;
- TS 102 894-1 transport-frame de-framing (no such frames in the capture);
- multi-frame TMID/navigation semantics;
- RTM / other TPEG applications.

---

## 4. Wiring

- `src/main.cpp`: a clean, byte- and slot-capped (`kMaxDataSubChannelBytes = 4
  MB`, `kMaxDataSubChannelSlots = 32`, both cap hits reported once per
  capture) tap accumulates every non-audio subchannel's per-frame bytes at
  load time (`m_dataSubChannelStreams`); DAB+ and MPEG audio components are
  excluded from the very first frame (`isAudioServiceComponent()`, A-M5a/B-M1).
  Decoding is **not** run per-frame — the streams are fed once, at load
  completion (`feedDataServiceSubchannels()`), routed per subchannel to the
  EPG / Journaline / TPEG decoders, so the parity gates
  `dls_msgs=1114 / mot_objects=97 / mot_images=97` are unchanged.
- `src/main.cpp` inner tabs: the Slideshow | EPG | Journaline | TPEG pages
  render the decoded content + honest empty states; routing is
  framing (bounded 240-byte probe) → FIG 1/1 / FIG 1/5 label → Journaline
  content probe → honest EPG fallback, and same-route subchannels accumulate
  into one panel.
- `src/core/eti_file_scanner.*`: synchronous EN 300 799 scanner used by the
  PRIVATE tests (and the GUI wave) to obtain per-subchannel byte streams.
- `src/core/data_service_store.*`: GUI-agnostic per-service data model
  (plain structs + accessors) produced from the real decoded content only.

---

## 5. Tests

| Target | Label | Requires capture | Content |
|---|---|---|---|
| `test_tpeg_framing` | PUBLIC | no | framing helpers, sync scanning, dedup on synthetic bytes |
| `test_journaline_decoder` | PUBLIC | no | existing 45 synthetic MOT tests + 3 stream-data tests (48 slots) |
| `test_epg_decoder` | PUBLIC | no | unchanged (synthetic) |
| `test_tpeg_decoder` | PRIVATE | bkk | real TEPG bytes: no sync pair, framing, ≥20 unique messages, TEC/TFP/PKI families |
| `test_e2e_epg_journaline` | PRIVATE | bkk + HR | real EPG rejection (0 events), TEPG inventory (36 unique / 4998 frames), HR Journaline clean-item count (3 014 of 11 544 candidates, pass rate 0.261), DataServiceStore |

Expected counts (verified with `ctest -N`): **PUBLIC 45/45, PRIVATE 13,
total 58.**

## 6. Known limitations / not determined

- **EPG:** no decodable EPG schedule exists in bkk; the EPG decoder is
  exercised against synthetic MOT fixtures only (PUBLIC tests) plus the honest
  rejection path (PRIVATE). **Validated scope = non-fabrication only (A-M2):**
  the gate that produces the "0 events" result is the decoder-side
  `validateMJD` rejection (`epg_decoder.cpp:142-146`) plus `processMOTObject()`'s
  CRC/completeness check. Stream → MSC data-group → MOT **reassembly is
  deferred** (not implemented — no reassembler exists in the current wiring),
  along with full TS 102 371 validation of real schedules. A capture with real
  EPG (e.g. German "MOT EPG") is needed to validate full TS 102 371 event
  decoding against real data.
- **Journaline:** the HR stream-mode format is decoded at the display-text
  level; we did **not** determine the byte-exact vendor "object element"
  encoding of the HR carousel (marker phases `10/20/30/00 01 13` + 16-char
  slots) — it is documented and extracted empirically, not fully reverse
  engineered. A MOT-encapsulated Journaline capture (standard TS 102 979
  profile) is still needed to exercise `processMOTObject()` on reality.
- **TEPG:** the actual message component bit-layout of the observed TPEG2
  carousel is not publicly documented; full TTI decode is deferred (see §3).