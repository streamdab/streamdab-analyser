# Data Availability Matrix - ETI Parser vs UI Fields (DAB/DAB+ Focus)

> **Analysis Date:** 2025-10-23 (Updated: G.703/G.704/ETI-NA removed)  
> **Purpose:** Map available parsed ETI data to UI field requirements from REF-UI1/2/3  
> **Scope:** ETI-NI format for DAB and DAB+ broadcast analysis  
> **Status:** ✅ Most fields covered, ⚠️ Some need implementation

---

## Executive Summary

### Coverage Analysis (DAB/DAB+ Focus):
- **Tab 1 (DAB Ensemble & ETI Analysis):** 95% data available (G.703 fields removed)
- **Tab 2 (FIC-Analyzer):** 95% data available
- **Tab 3 (FIC-Extractor):** 80% data available (needs bit-level parsing UI)
- **Live Stream Input:** 100% supported (UDP, TCP, ZMQ with libzmq 4.3.5)

### Supported ETSI Standards (DAB/DAB+ Specific):
✅ **ETSI EN 300 799** - ETI-NI frame structure (complete for DAB)
✅ **ETSI EN 300 401** - DAB system (FIG 0/0, 0/1, 0/2, 1/0, 1/1, 1/4 complete)
✅ **ETSI TS 102 563** - DAB+ audio (AAC decoder ready, analysis supported)

**Note:** G.703/G.704 framing and ETI-NA format are out of scope - focusing on DAB/DAB+ broadcast analysis only.

---

## Tab 1: DAB Ensemble & ETI Analysis - Data Availability

### LEFT PANEL

#### 1. Player Panel (QGroupBox)
| Field | Data Source | Status | Notes |
|-------|-------------|--------|-------|
| Status (running/stopped) | File/Stream state | ✅ Available | Track m_fileLoaded state |
| File name | m_currentFilePath | ✅ Available | Current file path variable |
| Start/Current/End Time | Frame timing | ✅ Available | Calculate from FCT (Frame Count) |
| Audio data | Not applicable | ⚠️ N/A | For live playback only |
| Frame number | m_currentFrameIndex | ✅ Available | Current frame counter |
| Loops button | UI control | ✅ Available | Add toggle button |
| Progress bar | Frame progress | ✅ Available | Use m_progressBar |

**Implementation:** Use existing state variables, add time calculation from FCT.

#### 2. Decoder Panel (QGroupBox)
| Field | Data Source | Status | Notes |
|-------|-------------|--------|-------|
| Status | Processing state | ✅ Available | Track processor status |
| Type | File format | ✅ Available | "ETI-NI" from ETI type detection |
| Start/Current/End Time | Frame timing | ✅ Available | Same as Player Panel |
| Frames | m_totalFrames | ✅ Available | Total frame count |

**Implementation:** Mirror Player Panel timing with decoder-specific status.

#### 3. System Panel (QTabWidget - 2 tabs) *(G.703 Transfer tab removed)*

##### Tab: FIC Overview
| Property | Data Source | Status | Parser |
|----------|-------------|--------|--------|
| Property (hierarchical) | FIG parsing | ✅ Available | `FigParser::parseFig*()` |
| Value | FIG data | ✅ Available | Extracted FIG values |
| Data | FIG raw data | ✅ Available | FIG block data |
| Status Code | Validation | ✅ Available | FIG validation results |

**Implementation:** Create QTreeWidget from FIG analysis results, display FIG type breakdown and ensemble configuration.

##### Tab: Control
| Feature | Implementation | Status |
|---------|----------------|--------|
| Processing controls | UI toggles | ✅ Ready |
| Analysis settings | Configuration | ✅ Ready |
| Display options | View settings | ✅ Ready |

#### 4. Messages Panel (QGroupBox)
| Field | Data Source | Status | Implementation |
|-------|-------------|--------|----------------|
| Log File path | Logging system | ✅ Available | ProfessionalLoggingSystem |
| Time | Log timestamp | ✅ Available | Log entry time |
| CIF | Frame CIF number | ✅ Available | From ETI header |
| Source | Log source | ✅ Available | "Analyzer", "Application" |
| Class | Log category | ✅ Available | "HCI", "SBCH", "MCI" |
| Message | Log message | ✅ Available | Full log message |
| Filter button | UI control | ✅ Available | Add filter dialog |
| Clear button | UI control | ✅ Available | Clear log display |

**Implementation:** Integrate ProfessionalLoggingSystem with QTableWidget display.

---

### CENTER PANEL

#### 5. Live Stream Input (QGroupBox with QStackedWidget)
| Component | Data Source | Status | Implementation |
|-----------|-------------|--------|----------------|
| Source Type Radio | UI control | ✅ Available | File / Network Stream selection |
| File Input Page | File system | ✅ Available | Browse + Load buttons |
| Network Input Page | NetworkStreamReceiver | ✅ Available | URL input with protocol examples |
| Connect/Disconnect | Network state | ✅ Available | Start/stop stream reception |
| Stream Statistics | NetworkStreamStatisticsSnapshot | ✅ Available | Frame rate, latency, buffer, loss |

**Supported Protocols:**
- ✅ UDP Multicast: `udp://239.192.0.1:9200`
- ✅ TCP: `tcp://192.168.1.100:9200`
- ✅ ZeroMQ: `zmq+tcp://localhost:9201` (libzmq 4.3.5)

**Implementation:** QStackedWidget switches between File and Network input pages. Network page includes quick examples dropdown and real-time statistics display.

**Location:** Placed at top of CENTER panel (above Ensemble Explorer) per user request for better logical flow - source selection should be near the data it generates.

**See Also:** Section "Live Stream Input - Network Support" at end of document for detailed protocol specifications.

#### 6. DAB Ensemble Explorer (QTreeWidget)
| Level | Field | Data Source | Status | Parser |
|-------|-------|-------------|--------|--------|
| 1 | Ensemble | FIG 1/0 | ✅ Available | `Fig10EnsembleLabel` |
| 1 | Label | FIG 1/0 | ✅ Available | Ensemble label string |
| 1 | Type | Fixed | ✅ Available | "Ensemble" |
| 2 | Service | FIG 0/2 | ✅ Available | `Fig02ServiceInfo` |
| 2 | Label | FIG 1/1 | ✅ Available | `Fig11ServiceLabel` |
| 2 | Type | FIG 0/2 ASCTy | ✅ Available | "Audio DAB+", "Data" |
| 3 | Component | FIG 0/2 components | ✅ Available | ServiceComponent |
| 3 | Status | Processing state | ✅ Available | "Active", "Idle" |
| 3 | Type | Component details | ✅ Available | "Service Detail" |

**Implementation:** Build tree from `FIGAnalysisResult::discovered_services`.

#### 7. ETI Frame Navigator (QListWidget)
| Field | Data Source | Status | Implementation |
|-------|-------------|--------|----------------|
| Frame number | Frame index | ✅ Available | m_currentFrameIndex |
| Frame type | ETI header | ✅ Available | "ETI-LI-A", "ETI-LI-B", "ETI-NI" |
| CRC/checksum | Frame CRC | ✅ Available | Calculate from frame data |

**Implementation:** Format: "Frame X: ETI-NI (0xff8c549)" - already working.

#### 8. Hex Viewer / ETSI Compliance (QTabWidget)
| Tab | Data Source | Status | Implementation |
|-----|-------------|--------|----------------|
| Hex Viewer | Frame raw data | ✅ Available | HexViewerFormatter (working) |
| ETSI Compliance | Validation | ✅ Available | ETSIComplianceEngine |

**Implementation:** Already implemented, enhance with multi-color highlighting.

#### 9. Subchannel Organization (QTableWidget)
| Column | Data Source | Status | Parser |
|--------|-------------|--------|--------|
| Checkbox | UI control | ✅ Available | Add checkboxes |
| Subch ID | FIG 0/1 | ✅ Available | `Fig01SubchannelInfo::subchannel_id` |
| Type | FIG 0/1 | ✅ Available | UEP/EEP from `protection_type` |
| Level | FIG 0/1 | ✅ Available | `protection_level` |
| Bitrate | FIG 0/1 | ✅ Available | Calculate from table/size |
| SAD | FIG 0/1 | ✅ Available | `start_address` |
| Size | FIG 0/1 | ✅ Available | `subchannel_size` (CUs) |
| Errors | Error detection | ✅ Available | AdvancedErrorDetector |

**Implementation:** Extract from `FIGAnalysisResult::subchannels`.

#### 10. Now Playing (DLS+) (QGroupBox)
| Field | Data Source | Status | Implementation |
|-------|-------------|--------|----------------|
| Track | DLS+ decoder | ✅ Available | DLSPlusDecoder (working) |
| Artist | DLS+ decoder | ✅ Available | DLSPlusDecoder |
| Album | DLS+ decoder | ✅ Available | DLSPlusDecoder |
| Last updated | Timestamp | ✅ Available | Track update time |

**Implementation:** Already implemented and working!

---

### RIGHT PANEL

#### 11. ETI Overview (QTableWidget)
| Property | Data Source | Status | Parser |
|----------|-------------|--------|--------|
| ETI Type | Frame header | ✅ Available | "ETSI(NI, G.704)" from frame type |
| Error Field | Frame STAT | ✅ Available | `EtiFrame::stat` error level |
| DAB Mode | Frame MODE | ✅ Available | `EtiFrame::mode` (1,2,3,4) |
| STAT | Frame STAT | ✅ Available | Full STAT field hex + percentage |

**Implementation:** Extract from `EtiFrame` header fields.

#### 12. DAB Error Counter (QTableWidget) *(G.703 removed - DAB focus)*
| Error Type | Data Source | Status | Implementation |
|------------|-------------|--------|----------------|
| FIC CRC Errors | CRC validation | ✅ Available | Track FIC CRC failures |
| MSC CRC Errors | CRC validation | ✅ Available | Track MSC CRC failures |
| Invalid FIG Types | FIG validation | ✅ Available | Count invalid FIG types |
| Service Org Errors | FIG 0/2 validation | ✅ Available | Service errors |
| Subchannel Errors | FIG 0/1 validation | ✅ Available | Subchannel errors |

**Status:** 100% available for DAB/DAB+ error detection

#### 13. Timing (QTableWidget)
| Property | Data Source | Status | Parser |
|----------|-------------|--------|--------|
| FCT = 0 (us) | Calculated | ✅ Available | 24ms * FCT (Mode 1) |
| Step (us) | Fixed by mode | ✅ Available | 24000 us (Mode 1) |
| FCT = 0 (ticks) | FCT value | ✅ Available | Frame Counter `fct` |
| Step (ticks) | Fixed | ✅ Available | 24000 ticks |

**Implementation:** Calculate from `EtiFrame::fct` and `mode`.

#### 14. CU Usage (Stacked Bar Chart)
| Data | Source | Status | Implementation |
|------|--------|--------|----------------|
| CU per subchannel | FIG 0/1 | ✅ Available | SubChannelInfo sizes |
| Total CUs | Sum | ✅ Available | Max 864 CUs for Mode 1 |
| Color coding | UI | ✅ Available | Custom QWidget or QChartView |

**Implementation:** Create custom widget or use QtCharts.

#### 15. DAB Stream Statistics (10+ metrics) *(G.703 counters removed - DAB focus)*
| Metric | Data Source | Status | Implementation |
|--------|-------------|--------|----------------|
| FIC Blocks Processed | FIC count | ✅ Available | Count FIC blocks |
| Total FIG Count | FIG count | ✅ Available | Sum all FIG types |
| Unique Services Found | FIG 0/2 | ✅ Available | Count unique service IDs |
| Active Subchannels | FIG 0/1 | ✅ Available | Count subchannels |
| Total CU Used / 864 | FIG 0/1 | ✅ Available | Sum subchannel sizes |
| DAB Services Count | FIG 0/2 | ✅ Available | Count audio services |
| DAB+ Services Count | FIG 0/2 ASCTy | ✅ Available | Count DAB+ (ASCTy≥60) |
| Data Services Count | FIG 0/2 | ✅ Available | Count data services |
| Average Frame Rate | Performance | ✅ Available | Calculate FPS |
| Processing Time | Performance | ✅ Available | ms per frame |

**Status:** 100% available - all DAB/DAB+ broadcast metrics covered!

---

## Tab 2: FIC-Analyzer - Data Availability

### LEFT PANEL

#### Overview (QGroupBox)
| Field | Data Source | Status |
|-------|-------------|--------|
| Ensemble name | FIG 1/0 | ✅ Available |
| FIC Content summary | FIG count | ✅ Available |
| Sub-Channel Org summary | FIG 0/1 count | ✅ Available |

#### Service Tree (QTreeWidget)
| Level | Data Source | Status |
|-------|-------------|--------|
| SubCh 1-N | FIG 0/1 | ✅ Available |
| Service types | FIG 0/2 | ✅ Available |
| Component IDs | FIG 0/2 components | ✅ Available |

#### Digital services summary (folded into the Overview panel — T33, 2026-10-06)
| Field | Data Source | Status | Notes |
|-------|-------------|--------|-------|
| Digital (data) service IDs + names | FIG 0/2 (P/D=1) + FIG 1/1 | ✅ Available | Overview line: `Digital services: N — 0x…` (or "none"); the former standalone "Digital Org Info" panel was removed as dead UI. |

#### Service Components (QTreeWidget)
| Field | Data Source | Status |
|-------|-------------|--------|
| Audio subchannels | FIG 0/2 | ✅ Available |
| FIDC | FIG 0/3 | ⚠️ Partial |
| Packet mode | FIG 0/3 | ⚠️ Partial |

**Status:** Core service organization fully available, packet mode needs FIG 0/3 parser enhancement.

---

### CENTER PANEL

#### Ensemble Table (10 columns)
| Column | Data Source | Status | Parser |
|--------|-------------|--------|--------|
| Type | Service type | ✅ Available | FIG 0/2 ASCTy |
| SubCH Id | Subchannel | ✅ Available | FIG 0/1 subchannel_id |
| FIGCH | FIG channel | ⚠️ Optional | Add if needed |
| PADDR | Packet address | ✅ Available | FIG 0/2 packet_address |
| SC1d | Service component | ✅ Available | Component ID |
| CA | Conditional access | ✅ Available | FIG 0/2 ca_flag |
| CAOrg | CA organization | ⚠️ Optional | Add if needed |
| Bitrate | Bitrate | ✅ Available | Calculate from FIG 0/1 |
| ASCTy+ | Audio service type | ✅ Available | FIG 0/2 audio_service_type |
| Info | Additional | ✅ Available | Extra metadata |

**Status:** 90% available, optional fields can be added.

#### Sub-Channel Table (10 columns)
| Column | Data Source | Status |
|--------|-------------|--------|
| SubCh Id | FIG 0/1 | ✅ Available |
| SAD | FIG 0/1 start_address | ✅ Available |
| Bitrate | Calculated | ✅ Available |
| P.Level | FIG 0/1 protection_level | ✅ Available |
| P.Type | FIG 0/1 UEP/EEP | ✅ Available |
| SubCh Type | Audio/Data/Packet | ✅ Available |
| SId | FIG 0/2 service_id | ✅ Available |
| Service Label | FIG 1/1 | ✅ Available |
| Short Label | FIG 1/1 | ✅ Available |
| CAID | FIG 0/2 CA info | ✅ Available |

**Status:** 100% available!

#### Service Table (4 columns)
| Column | Data Source | Status |
|--------|-------------|--------|
| SId | FIG 0/2 | ✅ Available |
| Service Label | FIG 1/1 | ✅ Available |
| Short Label | FIG 1/1 | ✅ Available |
| CAID | FIG 0/2 | ✅ Available |

**Status:** 100% available!

---

### RIGHT PANEL

#### FIG Content Table (4 columns)
| Column | Data Source | Status | Implementation |
|--------|-------------|--------|----------------|
| Type | FIG type | ✅ Available | FIG header type (0-7) |
| Extension | FIG extension | ✅ Available | FIG extension (0-31) |
| Number | FIG count | ✅ Available | Occurrence count |
| Subject | FIG description | ✅ Available | Hard-coded descriptions |

**Subjects mapping:**
- FIG 0/0: "Ensemble information"
- FIG 0/1: "Sub-Channel information"
- FIG 0/2: "Service organisation"
- FIG 1/0: "Ensemble label"
- FIG 1/1: "Service label"
- etc.

**Status:** 100% available - just need to format and count FIG occurrences.

---

## Tab 3: FIC-Extractor - Data Availability

### LEFT PANEL - Frame List

#### FIG Instance Tree (7 columns)
| Column | Data Source | Status | Implementation |
|--------|-------------|--------|----------------|
| FIG Type | FIG header | ✅ Available | "FIG 0/2" |
| Index | Occurrence | ✅ Available | Sequential index |
| Frame# | Frame counter | ✅ Available | Format "10:76" (frame:subframe) |
| CIF# | CIF number | ✅ Available | 0-3 CIF index |
| FIB# | FIB number | ✅ Available | 0-30 FIB index |
| Length | FIG length | ✅ Available | FIG data length |
| Items | Item count | ✅ Available | Count of elements in FIG |

**Status:** ✅ All data available - needs UI formatting.

**Implementation Notes:**
- Track FIG occurrences across frames
- Store frame number, CIF, FIB positions
- Calculate item counts from FIG parsing

---

### RIGHT PANEL - FIG Item Details

#### FIG Item Tree (4 columns, 5 levels)
| Column | Data Source | Status | Implementation |
|--------|-------------|--------|----------------|
| Property | Field name | ✅ Available | "FIG Type", "SId", "TMId", etc. |
| Size | Bit size | ⚠️ **NEEDS WORK** | **Add bit-level tracking** |
| Value | Field value | ✅ Available | Hex/decimal values |
| Information | Decoded info | ✅ Available | Human-readable strings |

**Status:** ⚠️ **75% available - Need bit-level field size tracking**

**Missing Implementation:**
Current parsers extract **values** but don't track **bit positions/sizes**.

**Example - Current:**
```cpp
uint16_t service_id;  // We know the value
```

**Example - Needed:**
```cpp
struct BitField {
    uint16_t value;
    uint8_t bit_offset;    // Position in byte stream
    uint8_t bit_length;    // Size in bits
    std::string name;      // "SId"
    std::string info;      // "Service ID"
};
```

**Solution:** Create `BitLevelFIGParser` wrapper that tracks:
- Bit offset within FIG
- Bit length of each field
- Field nesting hierarchy (for tree display)
- Icons for each level (🔧🌺🌸🌼🌷)

---

### FIB Hex Viewer
| Feature | Status | Implementation |
|---------|--------|----------------|
| Hex dump | ✅ Available | Use HexViewerFormatter |
| Address column | ✅ Available | Show 0000, 0008, 0010, ... |
| ASCII column | ✅ Available | HexViewerFormatter.setAsciiColumn(true) |
| Highlight selected FIG | ⚠️ **NEEDS WORK** | Add grey background for selected FIG bytes |

**Implementation:** Enhance HexViewerFormatter with:
- `highlightRange(start_byte, end_byte, color)`
- Sync with Frame List selection

---

## Live Stream Input - Network Support

### Supported Protocols

✅ **Multicast UDP** - Implemented in `NetworkStreamReceiver`
✅ **TCP** - Qt sockets support
✅ **ZMQ (ZeroMQ)** - libzmq 4.3.5 integrated via CMake

### URL Format Design

Based on eti-tools and broadcast industry standards:

| Protocol | URL Format | Example |
|----------|-----------|---------|
| Multicast UDP | `udp://multicast_ip:port` | `udp://239.192.0.1:9200` |
| TCP | `tcp://host:port` | `tcp://192.168.1.100:9200` |
| ZMQ TCP | `zmq+tcp://host:port` | `zmq+tcp://localhost:9201` |
| File | `file:///path/to/file.eti` | `file:///home/user/stream.eti` |

### Live Stream Panel Design (Tab 1)

```
┌─────────────────────────────────────────────────────────┐
│  Live Stream Input                                       │
├─────────────────────────────────────────────────────────┤
│  Source:  ○ File   ● Network Stream                     │
│                                                          │
│  URL:  [udp://239.192.0.1:9200                      ] │
│        Examples:                                         │
│        - udp://239.192.0.1:9200 (Multicast)             │
│        - tcp://192.168.1.100:9200 (TCP)                 │
│        - zmq+tcp://localhost:9201 (ZeroMQ)              │
│                                                          │
│  [Connect]  [Disconnect]  Status: ● Connected           │
│                                                          │
│  Stream Stats:                                           │
│  - Frame Rate: 250.5 FPS                                │
│  - Frames Received: 12,543                              │
│  - Frame Loss: 0.01% (2 frames)                         │
│  - Network Latency: 15 ms                               │
│  - Buffer: 89% (890/1000 frames)                        │
└─────────────────────────────────────────────────────────┘
```

**Implementation:**
- QRadioButton: File / Network Stream
- QLineEdit: URL input with examples
- QPushButton: Connect/Disconnect
- QLabel: Status with LED indicator
- QGroupBox: Stream Stats (real-time updates)

**Data Source:** `NetworkStreamReceiver` + `NetworkStreamStatisticsSnapshot`

---

## Summary - Implementation Priorities

### ✅ Fully Available (95% - DAB/DAB+ Focus):
- ✅ ETI-NI frame structure parsing (complete)
- ✅ FIG 0/0, 0/1, 0/2, 1/0, 1/1, 1/4 parsing (all essential FIGs)
- ✅ Service discovery (DAB and DAB+)
- ✅ Subchannel organization
- ✅ DAB Ensemble tree (3-level hierarchy)
- ✅ DLS+ "Now Playing" metadata
- ✅ Network stream reception (multicast UDP ready)
- ✅ ETSI EN 300 799 / EN 300 401 compliance validation
- ✅ DAB+ audio service detection (ASCTy analysis)
- ✅ Error detection and logging

### ⚠️ Needs Enhancement (5%):
- ⚠️ **Bit-level FIG field tracking** for Tab 3 FIG Item tree (protocol details)
- ⚠️ **TCP/ZMQ stream protocols** (add socket support - easy)
- ⚠️ **CU Usage chart** (create custom widget or use QtCharts)
- ⚠️ **Hex viewer FIG highlighting** (enhance HexViewerFormatter)
- ⚠️ **FIG 0/3 packet mode** parsing (enhance for data services)

**Removed from scope (not applicable for DAB/DAB+ ETI-NI):**
- ❌ G.703/G.704 framing fields (not in ETI-NI format)
- ❌ ETI-NA specific features (focusing on ETI-NI only)
- ❌ Non-standard vendor-specific fields

---

## Recommended Implementation Order

### Phase 1: Core Data Display (Tab 1 + Tab 2) - DAB/DAB+ Focus
1. ✅ Implement comprehensive Tab 1 with all DAB/DAB+ fields
2. ✅ Add live stream input panel with UDP/TCP support
3. ✅ Implement Tab 2 FIC-Analyzer with all service/subchannel tables
4. ✅ Remove G.703 fields entirely (not applicable)

### Phase 2: Advanced Protocol Analysis (Tab 3)
1. ⚠️ Add bit-level field tracking to FIG parsers
2. ⚠️ Implement FIG Item tree with 5-level hierarchy
3. ⚠️ Enhance Hex viewer with FIG highlighting
4. ⚠️ Create FIG Statistic charts

### Phase 3: Optional Enhancements
1. Add ZMQ protocol support
2. Implement CU Usage stacked bar chart
3. Add Reed Solomon error detection
4. Enhance FIG 0/3 packet mode parsing

---

## Conclusion

**We have 95% of the data needed to implement the comprehensive professional DAB/DAB+ analyzer UI!**

The existing ETI parsers (`ModernETIFrameParser`, `FigParser`, `NetworkStreamReceiver`) provide:
- ✅ Complete ETI-NI frame parsing (6144-byte frames)
- ✅ Comprehensive FIG 0/0, 0/1, 0/2, 1/0, 1/1, 1/4 support (all essential DAB FIGs)
- ✅ Service and subchannel discovery (DAB and DAB+ detection)
- ✅ Network streaming (multicast UDP ready, TCP/ZMQ easy to add)
- ✅ ETSI EN 300 799 / EN 300 401 compliance validation
- ✅ DLS+ "Now Playing" metadata extraction
- ✅ DAB+ audio service identification (ASCTy analysis)

**Main gap:** Bit-level field size tracking for Tab 3 FIG-Extractor tree (5% - can be added as wrapper).

**Scope clarification:** 
- ✅ **IN SCOPE:** DAB and DAB+ broadcast analysis (ETI-NI format)
- ❌ **OUT OF SCOPE:** G.703/G.704 framing, ETI-NA format, vendor-specific extensions

**Strategy:** 
1. ✅ Implement UI with all available DAB/DAB+ data first (95% ready)
2. ⚠️ Add bit-level tracking wrapper for Tab 3 protocol details (5% remaining)
3. ✅ Focus purely on DAB/DAB+ broadcast analysis
4. 🎯 Show rich, detailed professional analyzer interface matching commercial tools!
