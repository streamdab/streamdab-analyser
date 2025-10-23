# UI Reorganization Plan - Based on Manual Testing Feedback

**Date:** October 23, 2025  
**Status:** User Testing Complete - Redesign Required  
**Reference:** REF-UI1.md, UI_DESIGN_GUIDE.md, DAB-XPlorer patterns  

## 📊 User Testing Results

### ✅ What Works
1. **ETI Loading:** Files load successfully
2. **Data Processing:** All Phase 3B DLS+ processing functional
3. **Core Engine:** ETI frame parsing working correctly

### ⚠️ What Needs Redesign
1. **UI Organization:** Current single window with top/bottom split is confusing
2. **Navigation:** Hard to find FIC Analyzer and FIC Extractor features
3. **Layout Pattern:** Need to follow DAB-XPlorer professional interface standard

## 🎯 Required Changes

### Current Structure (Before)
```
Main Window
├── Top Section (70% height)
│   ├── Left: DAB Ensemble Explorer (25%)
│   ├── Center: ETI Frame Navigator + Hex Viewer (50%)
│   └── Right: Service Details + DLS+ (25%)
└── Bottom Section (30% height)
    └── Tabs: System Messages | Performance | Error Detection | Logging | FIC-Analyzer | FIC-Extractor
```

### New Structure (After - User Requirements)
```
Main Window with 2 MAJOR TABS
│
├── **TAB 1: ETI DAB Ensemble Explorer** ⭐ PRIMARY
│   ├── Top Section (70% height)
│   │   ├── Left: DAB Ensemble Tree + ETI Explorer (30%)
│   │   ├── Center: ETI Frame Navigator + Hex Viewer (45%)
│   │   └── Right: Service Details + Signal Quality (25%)
│   └── Bottom Section (30% height)
│       └── Tabs: System Messages | Performance | Error Detection | Logging | Constellation
│
└── **TAB 2: FIC Analysis Tools** ⭐ SECONDARY
    ├── Top Section (70% height)
    │   ├── FIC-Analyzer (50%) - ref/fic-explorer.jpg + fic-explorer2.jpg
    │   └── FIC-Extractor (50%) - ref/fic-extractor.jpg
    └── Bottom Section (30% height) - SAME AS TAB 1
        └── Tabs: System Messages | Performance | Error Detection | Logging | Constellation
```

## 📋 Detailed Tab Specifications

### **Tab 1: ETI DAB Ensemble Explorer** (Main Working Tab)

#### **Reference Images:**
- `ref/dab-explorer.jpg` - DAB service tree and player controls
- `ref/eti-explorer.jpg` - ETI frame statistics and overview

#### **Top Section Layout (70% height):**

**Left Panel (30% width):**
```
┌─ DAB Ensemble Explorer ──────────────┐
│ QTreeWidget (3-level hierarchy)      │
│ ├─ □ Bangkok DAB (Ensemble)         │
│ │  ├─ □ NBT Thailand (Service)      │
│ │  │  └─ □ Active (Component)       │
│ │  ├─ □ NBT News Channel            │
│ │  └─ □ Evening Channel              │
│ └─ Service selection with checkboxes│
├─ ETI Explorer Stats ─────────────────┤
│ QGroupBox "ETI Overview"             │
│ ├─ ETI Type: ETSI(NI, G.704)       │
│ ├─ Error Field: Level 0             │
│ ├─ DAB Mode: 1                       │
│ └─ STAT: 0x8 64 / 99%               │
├─ Frame Details ──────────────────────┤
│ Selected Frame: #4992                │
│ ETI Frame Size: 6144 bytes           │
│ Frame Offset: 30664704               │
│ SYNC: ✅ CRC: ✅                     │
└───────────────────────────────────────┘
```

**Center Panel (45% width):**
```
┌─ ETI Frame Navigator ────────────────┐
│ Progress Bar: [████████████] 100%   │
│ Frames: 5001 / 5001                  │
├─ Frame List (Selectable) ────────────┤
│ QListWidget                          │
│ □ Frame 1: ETI-LI-A (0xff8c549)     │
│ □ Frame 2: ETI-LI-B (0xdf073ab6)    │
│ ☑ Frame 4992: ETI-LI-A ← Selected   │
│ □ Frame 4993: ETI-LI-B              │
│ (Scrollable, clickable selection)    │
├─ Frame Content Viewer ───────────────┤
│ QTabWidget                           │
│ ├─ [Hex Viewer] ─────────────────── │
│ │   0000: FF F8 C5 49 1A 2B 3C 4D  │
│ │   0010: 5E 6F 70 81 92 A3 B4 C5  │
│ │   (Color-coded hex display)       │
│ └─ [ETSI Compliance] ────────────── │
│     ETSI EN 300 799 validation      │
│     ✅ All checks passed             │
└───────────────────────────────────────┘
```

**Right Panel (25% width):**
```
┌─ Current Service ────────────────────┐
│ QGroupBox                            │
│ SID: 0x1001                          │
│ Type: DAB+ Audio                     │
│ Bitrate: 128 kbps                    │
│ Protection: 3-A                      │
│ SubCh: 5                             │
├─ Signal Quality ─────────────────────┤
│ Signal: -45 dBm                      │
│ SNR: 25.4 dB                         │
│ Quality: [████████░░] 85%           │
├─ Now Playing (DLS+) ─────────────────┤
│ Track: [Track Title]                 │
│ Artist: [Artist Name]                │
│ Album: [Album Name]                  │
│ Updated: 2025-10-23 14:16:37         │
└───────────────────────────────────────┘
```

#### **Bottom Section Layout (30% height):**

**QTabWidget with 5 tabs:**

1. **System Messages Tab:**
   - QTextEdit with log messages
   - Time-stamped status updates
   - Processing completion messages

2. **Performance Tab:**
   - FPS gauge
   - Memory usage
   - Processing latency
   - Current: >1000 FPS

3. **Error Detection Tab:**
   - Advanced error detector status
   - Error history tree
   - Pattern analysis

4. **Logging Tab:**
   - Professional logging entries
   - Log level filtering
   - Export capabilities

5. **Constellation Tab:** (Deferred - future implementation)
   - Placeholder for future QPSK/QAM display

### **Tab 2: FIC Analysis Tools**

#### **Reference Images:**
- `ref/fic-explorer.jpg` - FIG type filtering and properties
- `ref/fic-explorer2.jpg` - CU Usage chart and subchannel organization
- `ref/fic-extractor.jpg` - Hierarchical service information extraction

#### **Top Section Layout (70% height):**

**Left Half (50% width): FIC-Analyzer**
```
┌─ FIC-Analyzer (FIC-XPlorer Pattern) ─┐
│ ┌─ FIG Type Analysis ─────────────── │
│ │ QGroupBox with FIG selection       │
│ │ □ FIG 0/0 - Ensemble info          │
│ │ □ FIG 0/1 - Service organization   │
│ │ ☑ FIG 0/2 - Service component      │
│ │ ☑ FIG 1/0 - Ensemble label         │
│ │ □ FIG 1/1 - Service label          │
│ │ □ FIG 1/4 - Component label        │
│ │ □ Other FIGs (0/3-0/19, 2/0-2/3)  │
│ │ [Apply Filter] [Clear All]         │
│ └────────────────────────────────────│
├─ FIG Analysis Results ────────────────┤
│ QTreeWidget (hierarchical display)   │
│ ├─ FIG 0/2 - Service Component       │
│ │  ├─ Property: SCId=5              │
│ │  ├─ Value: Audio Stream           │
│ │  └─ Data: 0x1A2B3C                │
│ ├─ FIG 1/0 - Ensemble Label          │
│ │  └─ Label: "Bangkok DAB"          │
│ └─ (Expandable tree structure)       │
├─ CU Usage Chart ─────────────────────┤
│ QChartView (Stacked bar chart)       │
│ Shows capacity units per subchannel  │
│ [█ SubCh0][█ SubCh1][█ SubCh2]      │
│ Color-coded by protection level      │
├─ Subchannel Organization ────────────┤
│ QTableWidget                         │
│ SubCh│Type│Level│Bitrate│SAD│Errors │
│ ─────┼────┼─────┼───────┼───┼────── │
│ □ 0  │EEP │1    │192kbps│120│  0   │
│ □ 1D │EEP │3    │192kbps│140│  0   │
│ □ 1E │EEP │2-A  │104kbps│104│  0   │
│ (Scrollable table with warnings)     │
└───────────────────────────────────────┘
```

**Right Half (50% width): FIC-Extractor**
```
┌─ FIC-Extractor (Tree Tool Pattern) ──┐
│ ┌─ Extracted Service Information ─── │
│ │ QTreeWidget (3-level hierarchy)    │
│ │ ├─ Bangkok DAB Ensemble            │
│ │ │  ├─ EId: 0x1A2B                 │
│ │ │  ├─ Label: "Bangkok DAB"        │
│ │ │  └─ Services: 5                 │
│ │ ├─ NBT Thailand (SID: 0x1001)     │
│ │ │  ├─ Type: Audio DAB+            │
│ │ │  ├─ Label: "NBT Thailand"       │
│ │ │  ├─ PTy: News (1)               │
│ │ │  └─ Components:                 │
│ │ │     └─ Audio: SubCh 5, 128kbps │
│ │ └─ (More services...)              │
│ └────────────────────────────────────│
├─ Programme Information ───────────────┤
│ QGroupBox                            │
│ PTy: News (1)                        │
│ Time: 14:16:37 UTC                   │
│ Date: 23/10/2025                     │
│ Language: Thai (th)                  │
│ Country: Thailand (TH)               │
├─ Service Labels ─────────────────────┤
│ QTableWidget                         │
│ SID    │ Label            │ Type    │
│ ───────┼──────────────────┼─────────│
│ 0x1001 │ NBT Thailand     │ Audio   │
│ 0x1002 │ NBT News Channel │ Audio   │
│ 0x1003 │ Evening Channel  │ Audio   │
│ (Extracted from FIG 1/1)             │
└───────────────────────────────────────┘
```

#### **Bottom Section (30% height):**
**SAME as Tab 1** - Shared bottom tabs for consistency

## 📊 Data Availability Matrix

### ✅ Data Currently Available

| Data Element | Current Source | Location | Status |
|--------------|---------------|----------|--------|
| **DAB Ensemble Tree** | `m_ensembleTree` | main.cpp:1723 | ✅ Ready |
| **ETI Frame List** | `m_frameList` | main.cpp:1753 | ✅ Ready |
| **Frame Details** | `m_frameDetails` | main.cpp:1738 | ✅ Ready |
| **Hex Viewer** | `m_hexViewer` | main.cpp:1760 | ✅ Ready |
| **Service Info** | `m_serviceInfo` | main.cpp:1786 | ✅ Ready |
| **Signal Quality** | `m_qualityMeter` | main.cpp:1795 | ✅ Ready |
| **DLS+ Now Playing** | `m_trackLabel`, etc. | main.cpp:1809 | ✅ Ready |
| **System Messages** | `m_systemMessages` | main.cpp:1889 | ✅ Ready |
| **Performance Metrics** | Performance tab | main.cpp:2030 | ✅ Ready |
| **Error Detection** | Error tab | main.cpp:2158 | ✅ Ready |
| **Logging** | Logging tab | main.cpp:2273 | ✅ Ready |
| **FIG Analysis** | `m_figAnalysisResults` | main.cpp:1963 | ✅ Ready |
| **Hierarchical Service** | `m_hierarchicalServiceTree` | main.cpp:1984 | ✅ Ready |
| **ETSI Compliance** | `m_complianceViewer` | main.cpp:1774 | ✅ Ready |

### ⚠️ Data Needs Enhancement

| Data Element | Current State | Enhancement Needed | Priority |
|--------------|---------------|-------------------|----------|
| **ETI Overview Stats** | Basic frame count | Add ETI Type, Error Field, DAB Mode, STAT | **HIGH** |
| **CU Usage Chart** | Not implemented | Add QChartView stacked bar chart | **MEDIUM** |
| **Subchannel Organization Table** | Not implemented | Add detailed subchannel table | **MEDIUM** |
| **Programme Information** | Basic service info | Add PTy, Time, Date, Language, Country | **MEDIUM** |
| **Error Counter** | Basic error detection | Add detailed error counters by type | **LOW** |
| **Timing Panel** | Not implemented | Add FCT timing display | **LOW** |

### ❌ Data Not Available (Future Implementation)

| Data Element | Reason | Implementation Plan |
|--------------|--------|-------------------|
| **Constellation Display** | No QPSK/QAM visualization | Phase 4: Signal processing visualization |
| **CU Usage Stacked Bar** | No chart rendering | Phase 3: Add QtCharts dependency |
| **Real-time Signal Strength** | File-based analysis only | Phase 5: Network streaming support |
| **G.703/G.704 Properties** | ETI format specific | Phase 3: Enhanced ETI property extraction |

## 🛠️ Implementation Plan

### **Phase 1: Main Tab Widget Structure (Day 1)**

**File:** `src/main.cpp`

**Changes Required:**

1. **Wrap entire UI in QTabWidget:**
```cpp
void setupUI()
{
    // Create main tab widget
    QTabWidget* mainTabs = new QTabWidget(this);
    mainTabs->setTabPosition(QTabWidget::North);
    mainTabs->setStyleSheet("QTabWidget::pane { border: 1px solid #ccc; }");
    setCentralWidget(mainTabs);
    
    // Tab 1: ETI DAB Ensemble Explorer
    QWidget* ensembleTab = createEnsembleExplorerTab();
    mainTabs->addTab(ensembleTab, "🎵 ETI DAB Ensemble Explorer");
    
    // Tab 2: FIC Analysis Tools
    QWidget* ficTab = createFICAnalysisTab();
    mainTabs->addTab(ficTab, "📊 FIC Analysis Tools");
}
```

2. **Extract existing UI into Tab 1:**
```cpp
QWidget* createEnsembleExplorerTab()
{
    QWidget* tabWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(tabWidget);
    
    // Top section (existing code)
    QSplitter* topSplitter = createTopSection();
    mainLayout->addWidget(topSplitter);
    
    // Bottom section (existing tabs)
    QTabWidget* bottomTabs = createBottomTabs();
    mainLayout->addWidget(bottomTabs);
    
    mainLayout->setStretch(0, 7); // Top 70%
    mainLayout->setStretch(1, 3); // Bottom 30%
    
    return tabWidget;
}
```

3. **Create new Tab 2 with FIC tools:**
```cpp
QWidget* createFICAnalysisTab()
{
    QWidget* tabWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(tabWidget);
    
    // Top section: FIC-Analyzer + FIC-Extractor
    QSplitter* topSplitter = new QSplitter(Qt::Horizontal);
    
    // Left: FIC-Analyzer (existing ficAnalyzerTab code)
    QWidget* ficAnalyzer = createFICAnalyzerPanel();
    topSplitter->addWidget(ficAnalyzer);
    
    // Right: FIC-Extractor (existing ficExtractorTab code)
    QWidget* ficExtractor = createFICExtractorPanel();
    topSplitter->addWidget(ficExtractor);
    
    topSplitter->setSizes({600, 600}); // 50-50 split
    mainLayout->addWidget(topSplitter);
    
    // Bottom section: SAME tabs as Tab 1
    QTabWidget* bottomTabs = createBottomTabs(); // Shared function
    mainLayout->addWidget(bottomTabs);
    
    mainLayout->setStretch(0, 7); // Top 70%
    mainLayout->setStretch(1, 3); // Bottom 30%
    
    return tabWidget;
}
```

### **Phase 2: Enhanced Left Panel - DAB + ETI Explorer (Day 1)**

**Add ETI Overview Stats below Ensemble Tree:**

```cpp
void createEnsemblePanel(QSplitter* parent)
{
    QWidget* ensembleWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(ensembleWidget);

    // 1. DAB Ensemble Tree (existing)
    layout->addWidget(new QLabel("<b>DAB Ensemble Explorer</b>"));
    m_ensembleTree = new QTreeWidget();
    m_ensembleTree->setHeaderLabels({"Service", "Label", "Type"});
    layout->addWidget(m_ensembleTree, 3); // 3 parts

    // 2. ETI Explorer Stats (NEW - ref/eti-explorer.jpg)
    QGroupBox* etiOverviewGroup = new QGroupBox("ETI Overview");
    QFormLayout* overviewLayout = new QFormLayout(etiOverviewGroup);
    
    m_etiTypeLabel = new QLabel("ETSI(NI, G.704)");
    overviewLayout->addRow("ETI Type:", m_etiTypeLabel);
    
    m_errorFieldLabel = new QLabel("Level 0");
    overviewLayout->addRow("Error Field:", m_errorFieldLabel);
    
    m_dabModeLabel = new QLabel("1");
    overviewLayout->addRow("DAB Mode:", m_dabModeLabel);
    
    m_statLabel = new QLabel("0x8 64 / 99%");
    overviewLayout->addRow("STAT:", m_statLabel);
    
    layout->addWidget(etiOverviewGroup, 1); // 1 part

    // 3. Frame Details (existing)
    QGroupBox* detailsGroup = new QGroupBox("Frame Details");
    // ... existing code ...
    layout->addWidget(detailsGroup, 1); // 1 part

    parent->addWidget(ensembleWidget);
}
```

### **Phase 3: Enhanced FIC-Analyzer Panel (Day 2)**

**Add CU Usage Chart and Subchannel Organization:**

```cpp
QWidget* createFICAnalyzerPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    // 1. FIG Type Analysis (existing)
    QGroupBox* figControlsGroup = new QGroupBox("FIG Type Analysis");
    // ... existing code from line 1929 ...
    layout->addWidget(figControlsGroup, 1);
    
    // 2. FIG Analysis Results (existing)
    QGroupBox* resultsGroup = new QGroupBox("FIG Analysis Results");
    // ... existing code from line 1960 ...
    layout->addWidget(resultsGroup, 2);
    
    // 3. CU Usage Chart (NEW - ref/fic-explorer2.jpg)
    QGroupBox* cuUsageGroup = new QGroupBox("CU Usage");
    QVBoxLayout* cuLayout = new QVBoxLayout(cuUsageGroup);
    
    // TODO: Add QtCharts dependency
    // m_cuUsageChart = new QChartView();
    // cuLayout->addWidget(m_cuUsageChart);
    
    // Placeholder for now
    QLabel* placeholder = new QLabel("CU Usage Chart\n(Requires QtCharts - deferred)");
    placeholder->setAlignment(Qt::AlignCenter);
    cuLayout->addWidget(placeholder);
    
    layout->addWidget(cuUsageGroup, 1);
    
    // 4. Subchannel Organization Table (NEW - ref/fic-explorer2.jpg)
    QGroupBox* subchGroup = new QGroupBox("Subchannel Organization");
    QVBoxLayout* subchLayout = new QVBoxLayout(subchGroup);
    
    m_subchannelTable = new QTableWidget();
    m_subchannelTable->setColumnCount(7);
    m_subchannelTable->setHorizontalHeaderLabels({
        "", "Subch ID", "Type", "Level", "Bitrate", "SAD", "Size", "Errors"
    });
    subchLayout->addWidget(m_subchannelTable);
    
    layout->addWidget(subchGroup, 2);
    
    return panel;
}
```

### **Phase 4: Enhanced FIC-Extractor Panel (Day 2)**

**Add Programme Information:**

```cpp
QWidget* createFICExtractorPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    // 1. Hierarchical Service Tree (existing)
    QGroupBox* treeGroup = new QGroupBox("Hierarchical Service Tree");
    // ... existing code from line 1981 ...
    layout->addWidget(treeGroup, 3);
    
    // 2. Programme Information (NEW - ref/fic-extractor.jpg)
    QGroupBox* progInfoGroup = new QGroupBox("Programme Information");
    QFormLayout* progLayout = new QFormLayout(progInfoGroup);
    
    m_ptyLabel = new QLabel("--");
    progLayout->addRow("PTy:", m_ptyLabel);
    
    m_timeLabel = new QLabel("--:--:-- UTC");
    progLayout->addRow("Time:", m_timeLabel);
    
    m_dateLabel = new QLabel("--/--/----");
    progLayout->addRow("Date:", m_dateLabel);
    
    m_languageLabel = new QLabel("--");
    progLayout->addRow("Language:", m_languageLabel);
    
    m_countryLabel = new QLabel("--");
    progLayout->addRow("Country:", m_countryLabel);
    
    layout->addWidget(progInfoGroup, 1);
    
    // 3. Extracted Information (existing, enhanced)
    QGroupBox* extractedGroup = new QGroupBox("Service Labels");
    // ... existing code from line 1990, enhanced ...
    layout->addWidget(extractedGroup, 1);
    
    return panel;
}
```

### **Phase 5: Shared Bottom Tabs Function (Day 1)**

**Extract bottom tabs to shared function:**

```cpp
QTabWidget* createBottomTabs()
{
    QTabWidget* bottomTabs = new QTabWidget();
    
    // Tab 1: System Messages
    m_systemMessages = new QTextEdit();
    m_systemMessages->setFont(QFont("Consolas", 9));
    bottomTabs->addTab(m_systemMessages, "System Messages");
    
    // Tab 2: Performance
    QWidget* perfTab = createPerformanceTab();
    bottomTabs->addTab(perfTab, "Performance");
    
    // Tab 3: Error Detection
    QWidget* errorTab = createErrorDetectionTab();
    bottomTabs->addTab(errorTab, "Error Detection");
    
    // Tab 4: Logging
    QWidget* loggingTab = createLoggingTab();
    bottomTabs->addTab(loggingTab, "Logging");
    
    // Tab 5: Constellation (deferred)
    QLabel* constellationPlaceholder = new QLabel("Constellation Display\n(Deferred - Phase 4)");
    constellationPlaceholder->setAlignment(Qt::AlignCenter);
    bottomTabs->addTab(constellationPlaceholder, "Constellation");
    
    return bottomTabs;
}
```

## 📝 Summary of Changes

### **Files to Modify:**
1. `src/main.cpp` - Main GUI reorganization (all changes in this file)

### **New Member Variables Needed:**
```cpp
// ETI Overview (Tab 1 left panel)
QLabel* m_etiTypeLabel;
QLabel* m_errorFieldLabel;
QLabel* m_dabModeLabel;
QLabel* m_statLabel;

// Subchannel Organization (Tab 2 FIC-Analyzer)
QTableWidget* m_subchannelTable;

// Programme Information (Tab 2 FIC-Extractor)
QLabel* m_ptyLabel;
QLabel* m_timeLabel;
QLabel* m_dateLabel;
QLabel* m_languageLabel;
QLabel* m_countryLabel;

// CU Usage Chart (deferred - Phase 3)
// QChartView* m_cuUsageChart;
```

### **Estimated Effort:**
- **Day 1 (4-6 hours):** Main tab structure + Tab 1 reorganization
- **Day 2 (4-6 hours):** Tab 2 FIC tools + data enhancements
- **Day 3 (2-3 hours):** Testing, polish, bug fixes

### **Total:** 2-3 days for complete UI reorganization

## 🎯 Success Criteria

### **User Acceptance:**
- ✅ Two clear main tabs (Ensemble Explorer, FIC Analysis)
- ✅ DAB Ensemble Tree + ETI Explorer combined in left panel
- ✅ FIC-Analyzer and FIC-Extractor side-by-side in Tab 2
- ✅ Consistent bottom tabs across both main tabs
- ✅ All existing data preserved and functional
- ✅ Follows DAB-XPlorer professional interface patterns

### **Technical Validation:**
- ✅ Zero compilation errors
- ✅ All existing 215+ tests still passing
- ✅ No data loss or processing changes
- ✅ Smooth tab switching (<50ms)
- ✅ Proper memory cleanup

## 📚 Reference Checklist

### **Images Reviewed:**
- ✅ `ref/dab-explorer.jpg` - DAB service tree pattern
- ✅ `ref/eti-explorer.jpg` - ETI overview statistics
- ✅ `ref/fic-explorer.jpg` - FIG type filtering
- ✅ `ref/fic-explorer2.jpg` - CU usage + subchannel org
- ✅ `ref/fic-extractor.jpg` - Service information extraction

### **Documents Reviewed:**
- ✅ `ref/REF-UI1.md` - Detailed UI component analysis
- ✅ `docs/UI_DESIGN_GUIDE.md` - Professional interface specifications

### **Current Code Reviewed:**
- ✅ `src/main.cpp` - Existing GUI structure (3000+ lines)
- ✅ Widget members: All trees, lists, tables identified
- ✅ Data flow: Signal/slot connections understood

---

**Next Step:** Begin implementation following Phase 1-5 plan above.  
**User Feedback Loop:** Test after Day 2, adjust based on findings.  
**Final Delivery:** Day 3 with polished, tested UI reorganization.
