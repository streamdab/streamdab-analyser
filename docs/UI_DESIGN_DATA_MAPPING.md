# StreamDAB Analyzer - Widget Data Mapping Matrix

> **Purpose:** Complete mapping of existing widgets to new 3-tab organization  
> **Source:** Current main.cpp implementation (3000+ lines)  
> **Target:** Professional 3-tab UI structure  
> **Migration Strategy:** Preserve 87.5% of existing widgets

---

## 📊 Widget Inventory Summary

| Category | Count | Percentage |
|----------|-------|------------|
| **Total Widgets in Current Implementation** | 40 | 100% |
| **Widgets to Keep As-Is** | 35 | 87.5% |
| **Widgets Requiring Modification** | 2 | 5.0% |
| **New Components to Add** | 3 | 7.5% |

---

## 📋 Complete Widget Mapping Matrix

### Legend
- ✅ **Keep as-is** - No changes required, just relocate
- 🔄 **Modify** - Requires conversion or restructuring
- ➕ **Add new** - New component to implement
- 🗑️ **Remove** - Unused/redundant component

---

## Tab 1: ETI DAB Ensemble Explorer Widgets

### Left Panel Widgets

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_ensembleTree` | 2839 | QTreeWidget | Tab 1 → Left → Top (50%) | ✅ | None - Keep hierarchy structure |
| `m_frameDetails` | 2865 | QLabel | Tab 1 → Left → Bottom (50%) | 🔄 | **Convert to QTableWidget** with 8 rows × 2 columns |

**m_frameDetails Conversion Details:**
```cpp
// OLD (current):
QLabel* m_frameDetails;
m_frameDetails->setText("Selected Frame: 4992\nETI Frame Size: 6144 bytes\n...");

// NEW (required):
QTableWidget* m_etiExplorerDetails;
// 8 rows: ETI Type, Error Field, DAB Mode, STAT, Total Frames, 
//         ETI-NI Count, ETI-LI-A Count, ETI-LI-B Count
// 2 columns: Property (bold) | Value (monospace)
```

---

### Center Panel Widgets

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| **Player/Decoder Panels** | - | - | Tab 1 → Center → Top (15%) | ➕ | **New components**: 2x QGroupBox with status labels and progress bars |
| `m_progressBar` | 2914 | QProgressBar | Tab 1 → Center → Middle (30%) | ✅ | Keep - Frame loading progress |
| `m_frameCountLabel` | 2913 | QLabel | Tab 1 → Center → Middle (30%) | ✅ | Keep - "Frames: X / Y" display |
| `m_frameList` | 2840 | QListWidget | Tab 1 → Center → Middle (30%) | ✅ | Keep - Frame navigator list |
| `m_hexViewer` | 2841 | QTextEdit | Tab 1 → Center → Bottom (55%) Tab 1 | ✅ | Keep - Hex dump display |
| `m_complianceViewer` | 2842 | QTextEdit | Tab 1 → Center → Bottom (55%) Tab 2 | ✅ | Keep - ETSI validation |

**New Player/Decoder Components:**
```cpp
// NEW components needed (line ~1750 in refactored code):
QGroupBox* m_playerPanel;           // Player status panel
QGroupBox* m_decoderPanel;          // Decoder status panel
QLabel* m_playerStatusLabel;        // "Status: Running"
QLabel* m_playerFileLabel;          // "File: bkk_2022.eti"
QLabel* m_playerTimeLabels[3];     // Start/Current/End times
QProgressBar* m_playerProgressBar;  // Playback position
QLabel* m_decoderStatusLabel;       // Decoder status
QLabel* m_decoderTypeLabel;         // "Type: DAB+ AAC"
QLabel* m_decoderFramesLabel;       // "Frames: X / Y"
```

---

### Right Panel Widgets

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_serviceInfo` | 2912 | QLabel | Tab 1 → Right → Top (25%) | ✅ | Keep - Service details display |
| `m_qualityMeter` | 2915 | QProgressBar | Tab 1 → Right → Middle (20%) | ✅ | Keep - Signal quality bar |
| `m_trackLabel` | 2918 | QLabel | Tab 1 → Right → Bottom (55%) | ✅ | Keep - DLS+ track name |
| `m_artistLabel` | 2919 | QLabel | Tab 1 → Right → Bottom (55%) | ✅ | Keep - DLS+ artist name |
| `m_albumLabel` | 2920 | QLabel | Tab 1 → Right → Bottom (55%) | ✅ | Keep - DLS+ album name |
| `m_lastUpdatedLabel` | 2921 | QLabel | Tab 1 → Right → Bottom (55%) | ✅ | Keep - DLS+ update timestamp |

**Signal Quality Additional Labels (optional enhancement):**
```cpp
// Consider adding (not in current implementation):
QLabel* m_snrLabel;   // "SNR: 32.5 dB"
QLabel* m_berLabel;   // "BER: 1.2e-6"
```

---

## Tab 2: FIC Analysis Tools Widgets

### Left Panel (FIC-Analyzer)

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| **FIG Type Filter** | - | QComboBox | Tab 2 → Left → Top (50px) | ➕ | **New component**: FIG type dropdown filter |
| `m_figAnalysisResults` | 2868 | QTreeWidget | Tab 2 → Left → Full height | 🔄 | **Restructure**: Change to FIC-Analyzer pattern with table-like child items |

**m_figAnalysisResults Restructuring:**
```cpp
// OLD (current structure):
QTreeWidget with basic hierarchy:
├─ FIG Type 0
│  ├─ FIG 0/0: Ensemble info
│  └─ FIG 0/2: Service org

// NEW (required FIC-Analyzer structure):
QTreeWidget with table-like entries:
⊟ Type 0 - MCI and SI (1259 F)
  ⊞ FIG 0/0 - Ensemble information (2409)
  ⊟ FIG 0/2 - Service organisation (12609)
    ┌──────────┬───────┬─────────┬─────┬─────┬────────┬───────┐
    │ FIG Type │ Index │ Frame#  │ CIF │ FIB │ Length │ Items │
    ├──────────┼───────┼─────────┼─────┼─────┼────────┼───────┤
    │ FIG 0/2  │   0   │ 10:76   │  2  │ 24  │   4    │       │
    │ FIG 0/2  │   1   │ 10:77   │  0  │ 23  │   3    │       │

// Implementation: Use QTreeWidgetItem with custom columns
// Each child item has 7 columns showing instance details
```

**New FIG Type Filter:**
```cpp
// NEW component (line ~1950 in refactored code):
QComboBox* m_figTypeFilter;
// Options: "All FIG Types", "Type 0 - MCI and SI", "Type 1 - Labels",
//          "FIG 0/0", "FIG 0/1", "FIG 0/2", "FIG 1/0", "FIG 1/1", etc.
```

---

### Right Panel (FIC-Extractor)

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_hierarchicalServiceTree` | 2884 | QTreeWidget | Tab 2 → Right → Top (50%) | ✅ | Keep - Hierarchical service display |
| `m_extractedServiceLabels` | 2885 | QTextEdit | Tab 2 → Right → Bottom Tab 1 | ✅ | Keep - Service labels extraction |
| `m_extractedProgrammeInfo` | 2887 | QTextEdit | Tab 2 → Right → Bottom Tab 2 | ✅ | Keep - Programme information |
| `m_extractedTimeInfo` | 2890 | QLabel | Tab 2 → Right → Bottom Tab 3 | ✅ | Keep - Date/Time display |
| `m_extractedFrequencies` | 2888 | QListWidget | Tab 2 → Right → Bottom Tab 4 | ✅ | Keep - Frequency list |

**Note:** These 4 widgets will be organized into a QTabWidget in the right bottom panel (50% height)

---

## Tab 3: Advanced Features Widgets

### Sub-Tab 1: Multi-stream Processing

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_activeStreamsTree` | 2846 | QTreeWidget | Tab 3 → Multi-stream → Top (40%) | ✅ | Keep - Active streams table |
| `m_comparativeResults` | 2847 | QTextEdit | Tab 3 → Multi-stream → Bottom (60%) | ✅ | Keep - Analysis results |

**Associated Controls (existing):**
```cpp
// Buttons for multi-stream operations (existing in current implementation):
// - Add Stream button
// - Remove Stream button  
// - Start Processing button
// - Stop Processing button
// - Compare Streams button
```

---

### Sub-Tab 2: Advanced Error Detection

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_errorDetectionStatus` | 2850 | QLabel | Tab 3 → Error Detection → Left (30%) | ✅ | Keep - Status label |
| `m_systemHealthScore` | 2851 | QLabel | Tab 3 → Error Detection → Left (30%) | ✅ | Keep - Health score |
| `m_errorCountLabel` | 2852 | QLabel | Tab 3 → Error Detection → Left (30%) | ✅ | Keep - Error count |
| `m_errorHistoryTree` | 2853 | QTreeWidget | Tab 3 → Error Detection → Right Top (60%) | ✅ | Keep - Error history table |
| `m_errorPatternsDisplay` | 2854 | QTextEdit | Tab 3 → Error Detection → Right Bottom (40%) | ✅ | Keep - Pattern analysis |

---

### Sub-Tab 3: Professional Logging

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_loggingStatus` | 2857 | QLabel | Tab 3 → Logging → Left (30%) | ✅ | Keep - Logging status |
| `m_loggingLevel` | 2858 | QLabel | Tab 3 → Logging → Left (30%) | ✅ | Keep - Log level |
| `m_logCountLabel` | 2859 | QLabel | Tab 3 → Logging → Left (30%) | ✅ | Keep - Log count |
| `m_sessionIdLabel` | 2860 | QLabel | Tab 3 → Logging → Left (30%) | ✅ | Keep - Session ID |
| `m_logEntriesTree` | 2863 | QTreeWidget | Tab 3 → Logging → Right Top (70%) | ✅ | Keep - Log entries table |
| `m_loggingStatsDisplay` | 2864 | QTextEdit | Tab 3 → Logging → Right Bottom (30%) | ✅ | Keep - Statistics display |

---

## Shared Bottom Panel Widgets (Consistent Across All Tabs)

### Tab 1: System Messages

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_systemMessages` | 2843 | QTextEdit | Shared Bottom → Tab 1 | ✅ | Keep - System message log |

---

### Tab 2: Performance

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_fpsLabel` | 2894 | QLabel | Shared Bottom → Tab 2 | ✅ | Keep - FPS counter |
| `m_memoryLabel` | 2896 | QLabel | Shared Bottom → Tab 2 | ✅ | Keep - Memory usage |
| `m_frameRateLabel` | 2900 | QLabel | Shared Bottom → Tab 2 | ✅ | Keep - Frame rate |
| `m_processingTimeLabel` | 2902 | QLabel | Shared Bottom → Tab 2 | ✅ | Keep - Processing time |
| `m_complianceScore` | 2898 | QProgressBar | Shared Bottom → Tab 2 | ✅ | Keep - Compliance bar |
| `m_complianceLabel` | 2903 | QLabel | Shared Bottom → Tab 2 | ✅ | Keep - Compliance text |

**Note:** Performance widgets also exist with alternative names:
- `m_fpsCounter` (line 2893) - duplicate of m_fpsLabel
- `m_memoryUsage` (line 2895) - duplicate of m_memoryLabel
- `m_frameRate` (line 2899) - duplicate of m_frameRateLabel
- `m_errorCount` (line 2901) - different from m_errorCountLabel (used in Tab 3)
- `m_etsiCompliance` (line 2897) - duplicate of m_complianceScore

**Cleanup Required:** Remove duplicate widget declarations and use single instances.

---

### Tab 3: Error Detection (Summary View)

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_errorDetectionStatus` | 2850 | QLabel | Shared Bottom → Tab 3 | ✅ | **Reused** from Tab 3 Sub-Tab 2 |
| `m_systemHealthScore` | 2851 | QLabel | Shared Bottom → Tab 3 | ✅ | **Reused** from Tab 3 Sub-Tab 2 |
| `m_errorCountLabel` | 2852 | QLabel | Shared Bottom → Tab 3 | ✅ | **Reused** from Tab 3 Sub-Tab 2 |

**Note:** These widgets are **shared references** - same widgets displayed in both locations (Tab 3 Sub-Tab 2 and Shared Bottom Tab 3). No duplication needed.

---

### Tab 4: Logging (Summary View)

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| `m_loggingStatus` | 2857 | QLabel | Shared Bottom → Tab 4 | ✅ | **Reused** from Tab 3 Sub-Tab 3 |
| `m_loggingLevel` | 2858 | QLabel | Shared Bottom → Tab 4 | ✅ | **Reused** from Tab 3 Sub-Tab 3 |
| `m_logCountLabel` | 2859 | QLabel | Shared Bottom → Tab 4 | ✅ | **Reused** from Tab 3 Sub-Tab 3 |
| `m_sessionIdLabel` | 2860 | QLabel | Shared Bottom → Tab 4 | ✅ | **Reused** from Tab 3 Sub-Tab 3 |

---

### Tab 5: Constellation

| Current Widget | Line # | Type | New Location | Status | Modifications |
|----------------|--------|------|--------------|--------|---------------|
| **Constellation Diagram** | - | QLabel (placeholder) | Shared Bottom → Tab 5 | ✅ | Keep placeholder - Future: QCustomPlot |

**Current Implementation:**
```cpp
// Line ~2066 in current main.cpp:
QLabel* placeholder = new QLabel("Constellation Diagram\n\n"
                                "Signal quality visualization will be displayed here\n"
                                "when implementing professional signal analysis features.");
```

---

## 🗑️ Unused/Redundant Widgets to Remove

### Duplicate Widget Declarations

| Widget | Primary Line | Duplicate Line | Action |
|--------|--------------|----------------|--------|
| FPS counter | `m_fpsLabel` (2894) | `m_fpsCounter` (2893) | Remove m_fpsCounter |
| Memory usage | `m_memoryLabel` (2896) | `m_memoryUsage` (2895) | Remove m_memoryUsage |
| Frame rate | `m_frameRateLabel` (2900) | `m_frameRate` (2899) | Remove m_frameRate |
| Compliance | `m_complianceScore` (2898) | `m_etsiCompliance` (2897) | Remove m_etsiCompliance |

**Code Cleanup:**
```cpp
// REMOVE these duplicate declarations:
QLabel* m_fpsCounter;      // Use m_fpsLabel instead
QLabel* m_memoryUsage;     // Use m_memoryLabel instead  
QLabel* m_frameRate;       // Use m_frameRateLabel instead
QProgressBar* m_etsiCompliance;  // Use m_complianceScore instead
```

### Unused Widget Declarations

| Widget | Line # | Type | Reason for Removal |
|--------|--------|------|--------------------|
| `m_figAnalysisDetails` | 2869 | QTextEdit | Not actively used in current code |
| `m_analysisProgress` | 2881 | QProgressBar | Not connected to any functionality |
| `m_extractedProgramTypes` | 2886 | QTextEdit | Not populated or displayed |
| `m_extractedTimeDate` | 2889 | QTextEdit | Redundant with m_extractedTimeInfo |

---

## 📦 Widget Organization by Container

### Main Window Structure

```
QMainWindow
├── QTabWidget (m_mainTabWidget) - Main 3 tabs
│   │
│   ├── Tab 1: ETI DAB Ensemble Explorer (QWidget)
│   │   └── QSplitter horizontal (25% | 50% | 25%)
│   │       ├── Left Panel (QWidget)
│   │       │   └── QSplitter vertical (50% | 50%)
│   │       │       ├── m_ensembleTree (QTreeWidget)
│   │       │       └── m_etiExplorerDetails (QTableWidget) [MODIFIED]
│   │       │
│   │       ├── Center Panel (QWidget)
│   │       │   └── QSplitter vertical (15% | 30% | 55%)
│   │       │       ├── Player/Decoder panels (QWidget) [NEW]
│   │       │       │   ├── m_playerPanel (QGroupBox) [NEW]
│   │       │       │   └── m_decoderPanel (QGroupBox) [NEW]
│   │       │       ├── Frame Navigator (QGroupBox)
│   │       │       │   ├── m_progressBar (QProgressBar)
│   │       │       │   ├── m_frameCountLabel (QLabel)
│   │       │       │   └── m_frameList (QListWidget)
│   │       │       └── Viewer Tabs (QTabWidget)
│   │       │           ├── m_hexViewer (QTextEdit)
│   │       │           └── m_complianceViewer (QTextEdit)
│   │       │
│   │       └── Right Panel (QWidget)
│   │           └── QSplitter vertical (25% | 20% | 55%)
│   │               ├── m_serviceInfo (QLabel)
│   │               ├── m_qualityMeter (QProgressBar)
│   │               └── Now Playing (QGroupBox)
│   │                   ├── m_trackLabel (QLabel)
│   │                   ├── m_artistLabel (QLabel)
│   │                   ├── m_albumLabel (QLabel)
│   │                   └── m_lastUpdatedLabel (QLabel)
│   │
│   ├── Tab 2: FIC Analysis Tools (QWidget)
│   │   └── QSplitter horizontal (50% | 50%)
│   │       ├── FIC-Analyzer (QWidget)
│   │       │   ├── m_figTypeFilter (QComboBox) [NEW]
│   │       │   └── m_figAnalysisResults (QTreeWidget) [MODIFIED]
│   │       │
│   │       └── FIC-Extractor (QWidget)
│   │           └── QSplitter vertical (50% | 50%)
│   │               ├── m_hierarchicalServiceTree (QTreeWidget)
│   │               └── Extraction Tabs (QTabWidget)
│   │                   ├── m_extractedServiceLabels (QTextEdit)
│   │                   ├── m_extractedProgrammeInfo (QTextEdit)
│   │                   ├── m_extractedTimeInfo (QLabel)
│   │                   └── m_extractedFrequencies (QListWidget)
│   │
│   └── Tab 3: Advanced Features (QWidget)
│       └── QTabWidget (Sub-tabs)
│           ├── Multi-stream (QWidget)
│           │   └── QSplitter vertical (40% | 60%)
│           │       ├── m_activeStreamsTree (QTreeWidget)
│           │       └── m_comparativeResults (QTextEdit)
│           │
│           ├── Error Detection (QWidget)
│           │   └── QSplitter horizontal (30% | 70%)
│           │       ├── Status Panel (QWidget)
│           │       │   ├── m_errorDetectionStatus (QLabel)
│           │       │   ├── m_systemHealthScore (QLabel)
│           │       │   └── m_errorCountLabel (QLabel)
│           │       └── QSplitter vertical (60% | 40%)
│           │           ├── m_errorHistoryTree (QTreeWidget)
│           │           └── m_errorPatternsDisplay (QTextEdit)
│           │
│           └── Logging (QWidget)
│               └── QSplitter horizontal (30% | 70%)
│                   ├── Status Panel (QWidget)
│                   │   ├── m_loggingStatus (QLabel)
│                   │   ├── m_loggingLevel (QLabel)
│                   │   ├── m_logCountLabel (QLabel)
│                   │   └── m_sessionIdLabel (QLabel)
│                   └── QSplitter vertical (70% | 30%)
│                       ├── m_logEntriesTree (QTreeWidget)
│                       └── m_loggingStatsDisplay (QTextEdit)
│
└── QTabWidget (m_sharedBottomPanel) - Shared 5 tabs [HEIGHT: 250px]
    ├── System Messages: m_systemMessages (QTextEdit)
    ├── Performance: m_fpsLabel, m_memoryLabel, etc.
    ├── Error Detection: [Reused from Tab 3]
    ├── Logging: [Reused from Tab 3]
    └── Constellation: Placeholder (QLabel)
```

---

## 🔄 Migration Implementation Order

### Phase 1: Core Structure (Priority 1)
1. ✅ Create main 3-tab QTabWidget structure
2. ✅ Create shared bottom panel QTabWidget
3. ✅ Set up QSplitter hierarchy for Tab 1

### Phase 2: Tab 1 Widgets (Priority 2)
4. ✅ Migrate m_ensembleTree to left panel
5. 🔄 Convert m_frameDetails to m_etiExplorerDetails (QTableWidget)
6. ➕ Create Player/Decoder status panels
7. ✅ Migrate m_frameList, m_progressBar, m_frameCountLabel to center
8. ✅ Migrate m_hexViewer, m_complianceViewer to center bottom tabs
9. ✅ Migrate m_serviceInfo, m_qualityMeter to right panel
10. ✅ Migrate Now Playing widgets to right bottom

### Phase 3: Tab 2 Widgets (Priority 3)
11. ➕ Create m_figTypeFilter combobox
12. 🔄 Restructure m_figAnalysisResults for FIC-Analyzer pattern
13. ✅ Migrate m_hierarchicalServiceTree to right panel
14. ✅ Organize extraction widgets into tabbed interface

### Phase 4: Tab 3 Widgets (Priority 4)
15. ✅ Migrate multi-stream widgets to Sub-Tab 1
16. ✅ Migrate error detection widgets to Sub-Tab 2
17. ✅ Migrate logging widgets to Sub-Tab 3

### Phase 5: Shared Bottom Panel (Priority 5)
18. ✅ Migrate m_systemMessages to Tab 1
19. ✅ Migrate performance widgets to Tab 2
20. ✅ Create reused references for Tab 3 & 4
21. ✅ Keep constellation placeholder in Tab 5

### Phase 6: Cleanup (Priority 6)
22. 🗑️ Remove duplicate widget declarations
23. 🗑️ Remove unused widget declarations
24. ✅ Update all signal/slot connections
25. ✅ Test all widget interactions

---

## 🎯 Signal/Slot Connection Updates

### Critical Connections to Preserve

```cpp
// Frame selection → Multiple updates
connect(m_frameList, &QListWidget::currentRowChanged,
        this, &MainWindow::onFrameSelected);
// Updates: m_hexViewer, m_complianceViewer, m_ensembleTree, m_systemMessages

// Service selection → Info display
connect(m_ensembleTree, &QTreeWidget::itemClicked,
        this, &MainWindow::onServiceSelected);
// Updates: m_serviceInfo, m_qualityMeter, Now Playing labels

// FIG filtering → Results update  
connect(m_figTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onFIGTypeFilterChanged);
// Updates: m_figAnalysisResults (collapse/expand nodes)

// ETI processing signals → UI updates
connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::frameProcessed,
        this, &MainWindow::onFrameProcessed);
connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingProgress,
        this, &MainWindow::onProcessingProgress);
connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingComplete,
        this, &MainWindow::onProcessingComplete);

// DLS+ decoder → Now Playing updates
connect(m_dlsPlusDecoder, &DLSPlusDecoder::nowPlayingUpdated,
        this, &MainWindow::updateNowPlaying);
```

---

## ✅ Final Widget Count

| Category | Count |
|----------|-------|
| **Existing widgets preserved** | 35 |
| **Widgets modified** | 2 |
| **New widgets added** | 3 |
| **Duplicate widgets removed** | 4 |
| **Unused widgets removed** | 4 |
| **Total active widgets in new design** | 40 |

**Net Result:** Same total widget count (40), but better organized and no redundancy.

---

**Document Version:** 1.0  
**Last Updated:** 2025-10-23  
**Status:** ✅ Complete Widget Mapping Ready for Implementation
