# Comprehensive UI Redesign Plan - Professional DAB/DAB+ Analyzer

> **Based on REF-UI1.md, REF-UI2.md, REF-UI3.md**  
> **Target: Professional DAB/DAB+ broadcast analyzer with detailed field display**  
> **Date: 2025-10-23 (Updated: Focus on DAB/DAB+ only, G.703/G.704/ETI-NA removed)**  
> **Scope: ETI-NI format analysis for DAB and DAB+ ensembles**

---

## Executive Summary

The current 3-tab UI is **too simplified**. Commercial DAB-XPlorer tools show **MANY more detailed fields** for professional analysis. This redesign will implement ALL fields from the reference documents to create a true professional analyzer.

### Tab Structure (4 tabs total):

1. **Tab 1: DAB Ensemble & ETI Analysis** (REF-UI1.md)
2. **Tab 2: FIC-Analyzer** (REF-UI2.md)  
3. **Tab 3: FIC-Extractor** (REF-UI3.md)
4. **Tab 4: Advanced Features** (current multi-stream, error detection, logging)

---

## Tab 1: DAB Ensemble & ETI Analysis (REF-UI1)

### Complete Layout Structure:

```
┌─────────────────────────────────────────────────────────────────────┐
│  Tab 1: DAB Ensemble & ETI Analysis                                 │
├──────────────┬────────────────────────────────┬──────────────────────┤
│              │                                │                      │
│  LEFT PANEL  │      CENTER PANEL              │    RIGHT PANEL       │
│  (25%)       │      (50%)                     │    (25%)             │
│              │                                │                      │
│  ┌────────┐  │  ┌──────────────────────────┐  │ ┌─────────────────┐│
│  │ Player │  │  │ DAB Ensemble Explorer    │  │ │ ETI Overview    ││
│  │ Panel  │  │  │ (QTreeWidget - 3 levels) │  │ │ (QTableWidget)  ││
│  │        │  │  │                          │  │ │ 4 properties    ││
│  └────────┘  │  └──────────────────────────┘  │ └─────────────────┘│
│              │                                │                      │
│  ┌────────┐  │  ┌──────────────────────────┐  │ ┌─────────────────┐│
│  │Decoder │  │  │ ETI Frame Navigator      │  │ │ Error Counter   ││
│  │ Panel  │  │  │ (QListWidget)            │  │ │ (QTableWidget)  ││
│  │        │  │  │                          │  │ │ 4 error types   ││
│  └────────┘  │  └──────────────────────────┘  │ └─────────────────┘│
│              │                                │                      │
│  ┌────────┐  │  ┌──────────────────────────┐  │ ┌─────────────────┐│
│  │ System │  │  │ Hex Viewer / ETSI        │  │ │ Timing          ││
│  │ Panel  │  │  │ Compliance (QTabWidget)  │  │ │ (QTableWidget)  ││
│  │(QTabWgt)│  │  │                          │  │ │ 4 properties    ││
│  │Transfer│  │  └──────────────────────────┘  │ └─────────────────┘│
│  │Control │  │                                │                      │
│  │Overview│  │  ┌──────────────────────────┐  │ ┌─────────────────┐│
│  └────────┘  │  │ Subch Organization       │  │ │ CU Usage Chart  ││
│              │  │ (QTableWidget)           │  │ │ (Stacked Bar)   ││
│  ┌────────┐  │  │ 7 columns + checkbox     │  │ │                 ││
│  │Messages│  │  └──────────────────────────┘  │ └─────────────────┘│
│  │ Panel  │  │                                │                      │
│  │(QTable)│  │  ┌──────────────────────────┐  │ ┌─────────────────┐│
│  │5 cols  │  │  │ Now Playing (DLS+)       │  │ │Additional Error ││
│  └────────┘  │  │ (QGroupBox)              │  │ │Counters (12+)   ││
│              │  └──────────────────────────┘  │ └─────────────────┘│
└──────────────┴────────────────────────────────┴──────────────────────┘
```

### Detailed Widget Specifications:

#### LEFT PANEL Widgets:

**1. Player Panel (QGroupBox)**
- QLabel: Status (running/stopped/streaming)
- QLabel: Source (file name or stream URL)
- QLabel: Start Time / Current Time / End Time
- QLabel: Current Frame Number
- QPushButton: Pause/Resume
- QProgressBar: Time progress (for files) or buffer status (for streams)

**2. Decoder Panel (QGroupBox)**
- QLabel: Status (processing/idle)
- QLabel: ETI Type (ETI-NI)
- QLabel: DAB Mode (Mode I/II/III/IV)
- QLabel: Total Frames Processed
- QLabel: Processing Speed (FPS)

**3. System Panel (QTabWidget with 2 tabs)** *(G.703 Transfer tab removed - ETI-NI only)*

*Tab 1: FIC Overview*
- QTreeWidget: FIC Properties (4 columns)
  - Property | Value | Data | Status
  - Display FIG type analysis results
  - Show ensemble configuration
  - DAB/DAB+ service details

*Tab 2: Control*
- Processing controls
- Analysis settings
- Display options

**4. Messages Panel (QGroupBox)**
- QLabel: Log File path
- QTableWidget: 5 columns (Checkbox | Time | CIF | Source | Class | Message)
- Rows show: Analyzer, Application logs with timestamps
- QPushButton: Filter, Clear

#### CENTER PANEL Widgets:

**1. DAB Ensemble Explorer (QTreeWidget)**
- 3 Levels:
  - Level 1 (Ensemble): Service | Label | Type
  - Level 2 (Service): Service name | Type
  - Level 3 (Component): Frame Status | Type

**2. ETI Frame Navigator (QListWidget)**
- Format: "Frame X: ETI-LI-A (0xff8c549)" or "ETI-LI-B (0xdf073ab6)"
- Alternating A/B frames
- Scrollable list

**3. Hex Viewer / ETSI Compliance (QTabWidget)**
- Tab 1: Hex Viewer (QTextEdit with syntax highlighting)
  - Format: Address | Hex (16 bytes/row) | ASCII
  - Multi-color highlighting (red, green, yellow, orange)
- Tab 2: ETSI Compliance (QTextEdit)
  - Compliance validation results

**4. Subch Organization (QTableWidget)**
- Columns: Checkbox | Subch ID | Type | Level | Bitrate | SAD | Size | Errors
- Multiple rows for SubCh 0, 1D, 1E, etc.
- Warning icons (⚠️) for errors

**5. Now Playing (DLS+) (QGroupBox)**
- QLabel: Track (bold, 14pt, blue)
- QLabel: Artist (12pt)
- QLabel: Album (11pt, italic)
- QFrame: Separator line
- QLabel: Last updated timestamp

#### RIGHT PANEL Widgets:

**1. ETI Overview (QTableWidget)**
- 2 columns (Property | Value)
- Rows:
  - ETI Type: ETI-NI
  - DAB Mode: I / II / III / IV
  - Frame Type: ETI-LI-A / ETI-LI-B / ETI-NI
  - STAT Field: 0x8 (64) - 99%

**2. DAB Error Counter (QTableWidget)** *(G.703 errors removed - DAB/DAB+ only)*
- 2 columns (Error Type | Count)
- Rows:
  - FIC CRC Errors
  - MSC CRC Errors
  - Invalid FIG Types
  - Service Organization Errors
  - Subchannel Errors
- Warning icons (⚠️) for non-zero counts

**3. Timing (QTableWidget)**
- 2 columns (Property | Value)
- Rows:
  - FCT = 0: 900000.0 us
  - Step: 24000.0 us
  - FCT = 0: 900000
  - Step: 24000.0 us

**4. CU Usage (Stacked Bar Chart)**
- Custom widget or QChartView
- Shows Capacity Units usage
- Color-coded segments (blue, orange, grey)
- Divided by SubChannel

**5. DAB Stream Statistics (QTableWidget)** *(G.703 counters removed - DAB/DAB+ focus)*
- 2 columns (Property | Value)
- Rows (DAB-specific metrics):
  - FIC Blocks Processed
  - Total FIG Count
  - Unique Services Found
  - Active Subchannels
  - Total CU Used / 864
  - DAB Services Count
  - DAB+ Services Count
  - Data Services Count
  - Average Frame Rate (FPS)
  - Processing Time (ms/frame)

---

## Tab 2: FIC-Analyzer (REF-UI2)

### Complete Layout Structure:

```
┌─────────────────────────────────────────────────────────────────────┐
│  Tab 2: FIC-Analyzer                                                 │
├──────────────┬────────────────────────────────┬──────────────────────┤
│              │                                │                      │
│  LEFT PANEL  │      CENTER PANEL              │    RIGHT PANEL       │
│  (25%)       │      (50%)                     │    (25%)             │
│              │                                │                      │
│  ┌────────┐  │  ┌──────────────────────────┐  │ ┌─────────────────┐│
│  │Overview│  │  │ QTabWidget               │  │ │ Service         ││
│  │ Label  │  │  │ - Ensemble Tab           │  │ │ Component       ││
│  │        │  │  │ - Service Component Tab  │  │ │ Table           ││
│  └────────┘  │  │                          │  │ │ (10 columns)    ││
│              │  │ Ensemble Table           │  │ │                 ││
│  ┌────────┐  │  │ (10 columns)             │  │ └─────────────────┘│
│  │Service │  │  │                          │  │                      │
│  │  Tree  │  │  └──────────────────────────┘  │ ┌─────────────────┐│
│  │(QTree) │  │                                │ │ List of         ││
│  │3 levels│  │  ┌──────────────────────────┐  │ │ Services        ││
│  │        │  │  │ List of Services         │  │ │ (QTableWidget)  ││
│  └────────┘  │  │ (QTableWidget)           │  │ │ 6 columns       ││
│              │  │ 6 columns                │  │ │                 ││
│  ┌────────┐  │  └──────────────────────────┘  │ └─────────────────┘│
│  │Digital │  │                                │                      │
│  │  Org   │  │  ┌──────────────────────────┐  │ ┌─────────────────┐│
│  │ Info   │  │  │ QTabWidget               │  │ │ Ensemble        ││
│  │(QLabel)│  │  │ - Sub-Channel Tab        │  │ │ Properties      ││
│  │        │  │  │ - Service Tab            │  │ │ (QGroupBox)     ││
│  └────────┘  │  │                          │  │ │ 6 properties    ││
│              │  │ Sub-Channel Table        │  │ │                 ││
│  ┌────────┐  │  │ (10 columns)             │  │ └─────────────────┘│
│  │Service │  │  │                          │  │                      │
│  │Compnts │  │  └──────────────────────────┘  │ ┌─────────────────┐│
│  │(QTree) │  │                                │ │ FIG Content     ││
│  │        │  │  ┌──────────────────────────┐  │ │ Table           ││
│  └────────┘  │  │ Service Table            │  │ │ (QTableWidget)  ││
│              │  │ (4 columns)              │  │ │ 4 columns       ││
│              │  │                          │  │ │ 20+ rows        ││
│              │  └──────────────────────────┘  │ └─────────────────┘│
└──────────────┴────────────────────────────────┴──────────────────────┘
```

### Detailed Widget Specifications:

#### LEFT PANEL Widgets:

**1. Overview (QGroupBox)**
- QLabel: Ensemble name (e.g., "Sydney Mux1 3C")
- QLabel: "FIC Content:"
- QLabel: "Sub-Channel Org."

**2. Service Tree (QTreeWidget)**
- 3 Levels (simple list structure):
  - Level 1: SubCh 1-9 (parent nodes)
  - Level 2: Service types (Audio, Data, Packet) with IDs
  - Level 3: Sub-items (if any)

**3. Digital Org Info (QLabel list)**
- Multiple QLabel entries:
  - "0x202E: Digital Org. '"
  - "0x2B1E: None 969"
  - "0x2D31: ABC dig"
  - "0x2D32: '885 Digital Org.'"
  - etc. (scrollable)

**4. Service Components (QTreeWidget)**
- Simple tree list:
  - Audio
    - SubCh 1
    - SubCh 2
    - ...
  - FIDC
  - Packet
    - SC1d 0x1b, PA#1 0x1
    - SC2d 0x2c, PA#d 0x1

#### CENTER PANEL Widgets:

**TOP Section: QTabWidget (Ensemble / Service Component)**

*Ensemble Tab:*

**1. Ensemble Table (QTableWidget)**
- 10 columns: Type | SubCH Id | FIGCH | PADDR | SC1d | CA | CAOrg | Bitrate | ASCTy+ | Info
- Rows: Audio (multiple), Data, Packet
- Show bitrates: 160 kbps, 96 kbps, 56 kbps, 48 kbps
- CA column: "No"
- ASCTy+ values: 0x00, 0x2F, 0x3F, 0x2E

**2. List of Services (QTableWidget)**
- 6 columns: SId | Service | P/S | SC1dS | SC Label | SC Short Label
- Example row: 0x2022 | '885 Digital Org.' | 0 | (blank) | (blank) | (blank)

**BOTTOM Section: QTabWidget (Sub-Channel / Service)**

*Sub-Channel Tab:*

**3. Sub-Channel Table (QTableWidget)**
- 10 columns: SubCh Id | SAD | Bitrate | P.Level | P.Type | SubCh Type | SId | Service Label | Short Label | CAID
- Multiple rows:
  - SubCh 1: SAD 0, 192 kbps, 3, UEP, Audio, 0x2E1, '885 Digital Org.', '885 Dig', No access control
  - SubCh 2: SAD 48, 160 kbps, 3, UEP, Audio, 0x2D21, ABC dig, ABC dig, No access control
  - ... (more subchannels)

*Service Tab:*

**4. Service Table (QTableWidget)**
- 4 columns: SId | Service Label | Short Label | CAID
- Multiple rows:
  - 0x1746: 'TrustRadio, 'TrustRadio, No access control
  - 0xd210: 'DEUTSCHLANDFUN...', 'DLF', No access control
  - ... (more services)

**5. List of Service Components (QTableWidget)**
- 6 columns: Type | Id | PA&Dr | P/S | SC1dS | SC Label
- Rows: Audio entries with details

#### RIGHT PANEL Widgets:

**1. Service Component Panel (QGroupBox)**
- QTableWidget:
  - 10 columns: Type | SubCH Id | FIGCH | PADDR | SC1d | CA | CAOrg | Bitrate | ASCTy+ | InCA-...
  - Rows: Audio and Packet entries with bitrates (64 kbps, 192 kbps)

**2. List of Services (QTableWidget)**
- 6 columns: SId | Service | P/S | SC1dS | SC Label | SC Short Label
- Example: 0xd387 | 'NDR Traffic ' | P | 1 | NDR Traffic | Traffic

**3. Ensemble Properties (QGroupBox)**
- QLabel pairs (Property | Value):
  - Ensemble Label: '10N Norders: '
  - Short Label: '10N Nds'
  - ECC: -
  - Country: -
  - Recording: no change
  - Number of Services: 12
  - Number of Sub-Channel: 12

**4. FIG Content Table (QTableWidget)**
- 4 columns: Type | Extension | Number | Subject
- 20+ rows (scrollable):
  - 0 | 0 | 40 | Ensemble information
  - 0 | 1 | 132 | Sub-Channel information
  - 0 | 2 | 19 | Service organisation
  - 0 | 2 | 17 | Service organisation (audio)
  - ... (many more FIG types)
  - 1 | 0 | 4 | Ensemble label
  - 1 | 1 | 26 | Service label
  - 5 | 1 | 1 | Traffic Message Channel

---

## Tab 3: FIC-Extractor (REF-UI3)

### Complete Layout Structure:

```
┌─────────────────────────────────────────────────────────────────────┐
│  Tab 3: FIC-Extractor (Protocol Analyzer)                            │
├──────────────────────────────────┬───────────────────────────────────┤
│                                  │                                   │
│  LEFT PANEL (40%)                │    RIGHT PANEL (60%)              │
│                                  │                                   │
│  ┌────────────────────────────┐  │  ┌─────────────────────────────┐ │
│  │ Toolbar                    │  │  │ Toolbar                     │ │
│  │ - Stream LED indicator     │  │  │ - Type/Ext dropdown         │ │
│  │ - File open/save buttons   │  │  │                             │ │
│  │ - Media controls           │  │  └─────────────────────────────┘ │
│  └────────────────────────────┘  │                                   │
│                                  │  ┌─────────────────────────────┐ │
│  ┌────────────────────────────┐  │  │ QTabWidget (3 tabs)         │ │
│  │ QTabWidget (3 tabs)        │  │  │ - FIB Hex                   │ │
│  │ - Frame List (active)      │  │  │ - FIG Item (active)         │ │
│  │ - FIG List                 │  │  │ - FIG Statistic             │ │
│  │ - FIG Database             │  │  │                             │ │
│  │                            │  │  │ FIG Item Tree (QTreeWidget) │ │
│  │ ┌────────────────────────┐ │  │  │ 5 levels with icons:        │ │
│  │ │ Type/Ext Dropdown      │ │  │  │ 🔧 FIG Header               │ │
│  │ │ "Type 0 - MCI and SI"  │ │  │  │ 🌺 FIG Type Details         │ │
│  │ └────────────────────────┘ │  │  │ 🌸 FIG Content              │ │
│  │                            │  │  │ 🌼 Element Groups           │ │
│  │ ┌────────────────────────┐ │  │  │ 🌷 Detailed Elements        │ │
│  │ │ Frame List Tree        │ │  │  │                             │ │
│  │ │ (QTreeWidget)          │ │  │  │ Columns:                    │ │
│  │ │                        │ │  │  │ Property | Size | Value |   │ │
│  │ │ ⊟ Type 0 - MCI (1259F) │ │  │  │ Information                 │ │
│  │ │ ⊞ FIG 0/0 - Ens. (2409)│ │  │  │                             │ │
│  │ │ ⊞ FIG 0/1 - SubCh(5403)│ │  │  │ Example entries:            │ │
│  │ │ ⊟ FIG 0/2 - Svc (12609)│ │  │  │ ✓ FIG Type: 3 bits, 0       │ │
│  │ │   FIG 0/2  0  10:76 2..│ │  │  │ ✓ Length: 5 bits, 29        │ │
│  │ │   FIG 0/2  1  10:77 0..│ │  │  │ ✓ C/N flag: 1 bit, false    │ │
│  │ │   FIG 0/2  2  10:78 1..│ │  │  │ ✓ SId: 16 bits, 0x10C4      │ │
│  │ │   FIG 0/2  3  10:79 0..│ │  │  │ ✓ TMId: 2 bits, 0x0, "MSC"  │ │
│  │ │   (selected - grey)    │ │  │  │ ✓ ASCTy: 6 bits, 63, "MPEG" │ │
│  │ │   ... (scrollable)     │ │  │  │                             │ │
│  │ └────────────────────────┘ │  │  └─────────────────────────────┘ │
│  │                            │  │                                   │
│  └────────────────────────────┘  │  ┌─────────────────────────────┐ │
│                                  │  │ FIB Hex Viewer              │ │
│                                  │  │ (QTextEdit)                 │ │
│                                  │  │                             │ │
│                                  │  │ 0000  1D 02 1G C4 02 3F 52..│ │
│                                  │  │ 0008  FB 1A 45 02 JF 12 FF..│ │
│                                  │  │ 0010  D7 FB D9 8F 1E F4 53..│ │
│                                  │  │ 0018  DD DC 8F FA 1E FA E3..│ │
│                                  │  │                             │ │
│                                  │  │ Grey highlight: selected FIG│ │
│                                  │  └─────────────────────────────┘ │
└──────────────────────────────────┴───────────────────────────────────┘
```

### Detailed Widget Specifications:

#### TOP Toolbar (spanning both panels):

**Left Section:**
- QLabel: "Stream"
- LED Indicator (custom widget): Green/Red status
- QToolButton: Reload button
- QLabel: "File"
- QToolButton: Open file button
- QToolButton: Save button

**Media Controls:**
- QToolButton: Pause (||)
- QToolButton: Previous (<<)
- QToolButton: Next frame (>)
- QToolButton: Fast forward (>>)
- QToolButton: Skip to end (|>)

**Right Section:**
- QToolButton: Window layout

#### LEFT PANEL Widgets:

**QTabWidget (3 tabs):**

*Tab 1: Frame List (active)*

**1. Type/Ext Dropdown (QComboBox)**
- Entries:
  - "Type 0 - MCI and SI"
  - "Type 1 - Labels"
  - "Type 5 - FIDC"
  - etc.

**2. Frame List Tree (QTreeWidget)**
- Multi-level tree (2-3 levels):

*Level 1: FIG Type Groups*
- ⊟ Type 0 - MCI and SI (1259 F) - expanded
- ⊞ FIG 0/0 - Ensemble information (2409) - collapsed
- ⊞ FIG 0/1 - Sub-Channel information (5403) - collapsed
- ⊟ FIG 0/2 - Service organisation (12609) - expanded

*Level 2: FIG Instances (table format)*
- Columns: FIG Type | Index | Frame# | CIF# | FIB# | Length | Items
- Rows (under expanded FIG 0/2):
  - FIG 0/2 | 0 | 10:76 | 2 | 24 | 4
  - FIG 0/2 | 1 | 10:77 | 0 | 23 | 3
  - FIG 0/2 | 2 | 10:78 | 1 | 30 | 4
  - FIG 0/2 | 3 | 10:79 | 0 | 30 | 4 (selected - grey highlight)
  - ... (scrollable, 100+ entries)

*Features:*
- Collapsible nodes (⊞/⊟)
- Selection highlighting (grey)
- Icon indicators for errors (⚠️, 🔴, 🔄)
- Scrollable content

*Tab 2: FIG List*
- Alternative organization (by FIG type/extension)

*Tab 3: FIG Database*
- Reference database view

#### RIGHT PANEL Widgets:

**QTabWidget (3 tabs):**

*Tab 1: FIB Hex*

**1. FIB Hex Viewer (QTextEdit/QPlainTextEdit)**
- Format: Address | Hex bytes | ASCII
- Example:
  ```
  0000   1D 02 1G C4 02 3F 52 FF   .....?R.
  0008   FB 1A 45 02 JF 12 FF E0   ..E.....
  0010   D7 FB D9 8F 1E F4 53 46   .......SF
  0018   DD DC 8F FA 1E FA E3 6B   .......k
  ```
- Grey background: Highlight selected FIG from Frame List
- Syntax highlighting for patterns

*Tab 2: FIG Item (active)*

**2. FIG Item Tree (QTreeWidget)**
- 5-level hierarchical tree with colored icons:

*Level 1: 🔧 FIG Header*
- ✓ FIG Type: 3 bits, Value: 0
- ✓ Length of FIG data field: 5 bits, Value: 29

*Level 2: 🌺 FIG Type Details*
- ✓ C/N flag: 1 bit, Value: false
- ✓ OE flag: 1 bit, Value: false
- ✓ P/D flag: 1 bit, Value: false
- ✓ Extension: 5 bits, Value: 2

*Level 3: 🌸 FIG Content*
- Node: FIG 0/2 - Service organisation
  - Sub-node: 🌼 FIG 0/2 (expandable)

*Level 4: 🌼 Element Groups*
- ✓ SId: 16 bits, Value: 0x10C4
- ✓ Local flag: 1 bit, Value: false
- ✓ CAId: 3 bits, Value: 0x0, Info: "No Access Control"
- ✓ Number of SC: 4 bits, Value: 2
- 🔻 Xxx (expandable)

*Level 5: 🌷 Detailed Elements*
- Node: Service Component description
  - ✓ TMId: 2 bits, Value: 0x0, Info: "MSC - Stream mode"
  - ✓ ASCTy: 6 bits, Value: 63, Info: "MPEG-1 HE-AAC-v"
  - ✓ SubChId: 6 bits, Value: 20

*Columns:*
- Property | Size | Value | Information

*Features:*
- Colored icons per level (🔧🌺🌸🌼🌷)
- Checkmark icons (✓) for leaf nodes
- Expandable nodes (🔻)
- Bit-level precision (1 bit, 2 bits, 3 bits, etc.)
- Hex and decimal values (0x10C4, 63)
- Human-readable info ("No Access Control", "MSC - Stream mode")

*Tab 3: FIG Statistic*

**3. FIG Statistic (QTableWidget or QChartView)**
- Statistical analysis of FIG occurrences
- Charts and graphs

---

## Tab 4: Advanced Features

Keep the current implementation:
- Multi-stream Processing
- Error Detection
- Professional Logging

---

## Implementation Priority

### Phase 1: Tab 1 - DAB Ensemble & ETI Analysis
1. Create all LEFT panel widgets (Player, Decoder, System, Messages)
2. Enhance CENTER panel with detailed tables
3. Add RIGHT panel analyzer widgets (Overview, Error Counter, Timing, CU Chart, Additional Counters)

### Phase 2: Tab 2 - FIC-Analyzer
1. Create LEFT panel (Overview, Service Tree, Digital Org, Service Components)
2. Create CENTER panel with dual QTabWidgets (Ensemble/Service Component, Sub-Channel/Service)
3. Create RIGHT panel (Service Component, List of Services, Ensemble Properties, FIG Content)

### Phase 3: Tab 3 - FIC-Extractor
1. Create toolbar with Stream LED, media controls
2. Create LEFT panel QTabWidget (Frame List tree with FIG instances)
3. Create RIGHT panel QTabWidget (FIB Hex, FIG Item tree, FIG Statistic)
4. Implement synchronization between Frame List and FIG Item/Hex views

### Phase 4: Integration & Testing
1. Test all tabs
2. Verify data flow between tabs
3. Ensure performance with large datasets

---

## Summary

This comprehensive redesign implements **ALL fields** from the reference commercial DAB-XPlorer tools:

**Tab 1 Total Widgets: ~35 widgets**
- 4 QGroupBox panels (Player, Decoder, System, Messages)
- 1 QTabWidget (System: Transfer/Control/Overview)
- 8 QTableWidget instances (various analyzer tables)
- 1 QTreeWidget (DAB Ensemble)
- 1 QListWidget (Frame Navigator)
- 1 Custom Chart Widget (CU Usage)
- Multiple QLabel, QPushButton, QProgressBar

**Tab 2 Total Widgets: ~25 widgets**
- 4 QTreeWidget instances (Service Trees)
- 2 QTabWidget instances (top and bottom sections)
- 10+ QTableWidget instances (various service/subchannel tables)
- QGroupBox for Ensemble Properties
- Multiple QLabel entries

**Tab 3 Total Widgets: ~15 widgets**
- QToolBar with 10+ buttons
- 2 QTabWidget instances (left and right panels)
- 2 QTreeWidget instances (Frame List, FIG Item)
- 1 QTextEdit (FIB Hex Viewer)
- 1 QComboBox (Type/Ext selector)
- LED indicator widget

**Total: 75+ specialized analyzer widgets** - This is a true professional broadcast analyzer tool.
