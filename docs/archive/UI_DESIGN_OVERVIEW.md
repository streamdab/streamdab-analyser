# StreamDAB Analyzer - Professional 3-Tab UI Design Overview

> **Design Version:** 1.0  
> **Target Platform:** Qt6 Desktop Application  
> **Target Users:** Professional Broadcast Engineers  
> **Reference Analysis:** DAB-XPlorer, ETI-XPlorer, FIC-XPlorer commercial tools  
> **Minimum Resolution:** 1200x800 pixels  
> **Target Resolution:** 1920x1080 pixels

---

## 📊 Executive Summary

This specification defines a professional 3-tab UI reorganization for StreamDAB Analyzer, consolidating 80+ widgets from the current single-view implementation into an intuitive broadcast industry-standard interface pattern based on DAB-XPlorer commercial software analysis.

### Design Philosophy
- **Tab 1 (Primary):** ETI DAB Ensemble Explorer - Main analysis workspace
- **Tab 2:** FIC Analysis Tools - Deep FIG protocol inspection
- **Tab 3:** Advanced Features - Multi-stream processing, error detection, logging
- **Shared Bottom Panel:** Consistent across all tabs - Performance, errors, messages, logs

---

## 🎯 Three-Tab Structure

### Tab 1: ETI DAB Ensemble Explorer (PRIMARY)
**Purpose:** Primary workspace for ETI file analysis, service discovery, and frame navigation.

**Layout:** 3-column design (25% | 50% | 25%)

**Left Panel (25%):**
- DAB Ensemble Tree (hierarchical service display)
- ETI Explorer Details (frame statistics table)

**Center Panel (50%):**
- Player/Decoder Status panels
- ETI Frame Navigator (progress bar + frame list)
- Hex Viewer / ETSI Compliance (tabbed)

**Right Panel (25%):**
- Current Service information
- Signal Quality meter
- Now Playing (DLS+ metadata)

---

### Tab 2: FIC Analysis Tools
**Purpose:** Deep FIG (Fast Information Group) protocol analysis and service extraction.

**Layout:** 2-column design (50% | 50%)

**Left Panel (50%): FIC-Analyzer**
- FIG Type Filter dropdown
- FIG Instance List (hierarchical tree with table-like entries)

**Right Panel (50%): FIC-Extractor**
- Hierarchical Service Tree
- Extracted Information (4 tabs):
  - Service Labels
  - Programme Information
  - Date/Time
  - Frequencies

---

### Tab 3: Advanced Features
**Purpose:** Multi-stream processing, advanced error detection, and professional logging.

**Layout:** 3 sub-tabs for advanced features

**Sub-Tab 1: Multi-stream Processing**
- Stream management controls
- Active ETI Streams table
- Comparative Analysis Results

**Sub-Tab 2: Advanced Error Detection**
- Error detection controls
- Error History tree
- Error Pattern Analysis display

**Sub-Tab 3: Professional Logging**
- Logging controls
- Recent Log Entries tree
- Logging Statistics display

---

## 📊 Shared Bottom Panel (Consistent Across All Tabs)

**Height:** 250px  
**Widget Type:** QTabWidget (5 tabs)

**5 Tabs:**
1. **System Messages** - Real-time status messages
2. **Performance** - FPS, memory, compliance metrics
3. **Error Detection** - Error summary and health status
4. **Logging** - Logging summary and session info
5. **Constellation** - Signal quality visualization (future)

**Key Feature:** The shared bottom panel remains consistent when switching between main tabs, providing persistent access to monitoring information.

---

## 🎨 Visual Design Guidelines

### Color Palette (DAB-XPlorer Light Theme)
- **Primary Background:** #f5f5f5 (Light grey)
- **Secondary Background:** #ffffff (White)
- **Border Color:** #cccccc (Grey)
- **Text Primary:** #212121 (Dark grey)
- **Selection Background:** #e3f2fd (Light blue)

### Status Colors
- **Active/Pass:** #4CAF50 (Green)
- **Warning:** #FF9800 (Orange)
- **Error/Critical:** #F44336 (Red)
- **Info:** #2196F3 (Blue)

### Typography
- **UI Text:** Segoe UI, Arial, sans-serif (10-11px)
- **Data/Code:** Consolas, Monaco, monospace (10px)
- **Headers:** Bold, 11px

### Hex Viewer Color Coding
- **Sync bytes:** #d32f2f (Red)
- **ETI Header:** #f57c00 (Orange)
- **FIC data:** #388e3c (Green)
- **MSC data:** #1976d2 (Blue)
- **ASCII non-printable:** #757575 (Grey)

---

## 📋 Widget Inventory

### Current Implementation Statistics
- **Total widgets in current main.cpp:** 40 major widgets
- **Widgets to keep as-is:** 35 widgets (87.5%)
- **Widgets requiring modification:** 2 widgets (5%)
- **New components to add:** 3 components (7.5%)

### Widget Distribution
- **Tab 1 (ETI Explorer):** 13 widgets
- **Tab 2 (FIC Analysis):** 8 widgets
- **Tab 3 (Advanced Features):** 14 widgets
- **Shared Bottom Panel:** 8 widgets

---

## 🚀 Key Features

### Professional Broadcast Industry Features
- ✅ Real-time ETI frame processing
- ✅ Complete FIG decoding (Types 0/0, 0/1, 0/2, 1/0, 1/1, 1/4)
- ✅ ETSI EN 300 799 compliance validation
- ✅ Multi-stream comparative analysis
- ✅ Advanced error detection and recovery
- ✅ Professional logging system
- ✅ DLS+ metadata display (Now Playing)
- ✅ Hex viewer with color-coded structure highlighting

### User Experience Enhancements
- ✅ Intuitive 3-tab navigation (primary → secondary → advanced)
- ✅ Consistent shared bottom panel across all tabs
- ✅ Resizable panels via QSplitter
- ✅ Keyboard shortcuts (Ctrl+1/2/3 for tab switching)
- ✅ Professional color scheme (DAB-XPlorer light theme)
- ✅ Clear visual hierarchy and status indicators
- ✅ No data loss when switching tabs

---

## 📊 Implementation Approach

### Phase-Based Development
1. **Phase 1 (Day 1):** Core 3-tab structure + shared bottom panel
2. **Phase 2 (Day 1-2):** Tab 1 complete implementation
3. **Phase 3 (Day 2):** Tab 2 FIC Analysis Tools
4. **Phase 4 (Day 2):** Tab 3 Advanced Features
5. **Phase 5 (Day 3):** Integration, polish, and testing
6. **Phase 6 (Day 3):** Documentation and delivery

### Total Estimated Time
**2-3 days** (20-24 hours development effort)

---

## 📚 Related Documentation

This overview document is part of a comprehensive UI design specification package:

1. **UI_DESIGN_OVERVIEW.md** (this file) - Executive summary and structure
2. **UI_DESIGN_TAB1_SPECIFICATION.md** - Complete Tab 1 detailed spec
3. **UI_DESIGN_TAB2_SPECIFICATION.md** - Complete Tab 2 detailed spec
4. **UI_DESIGN_TAB3_SPECIFICATION.md** - Complete Tab 3 detailed spec
5. **UI_DESIGN_SHARED_BOTTOM_PANEL.md** - Shared bottom panel specification
6. **UI_DESIGN_STYLESHEET.qss** - Professional QSS stylesheet
7. **UI_DESIGN_DATA_MAPPING.md** - Widget migration matrix
8. **UI_DESIGN_IMPLEMENTATION_CODE.md** - Qt6 code examples
9. **UI_DESIGN_USER_FLOWS.md** - User interaction flows
10. **UI_DESIGN_TIMELINE.md** - Implementation schedule

---

## ✅ Success Criteria

### Professional Appearance
- ✅ DAB-XPlorer light theme successfully applied
- ✅ Consistent font usage across all components
- ✅ Color-coded status indicators functional
- ✅ Professional panel borders and separators
- ✅ Hover/selection states match reference design

### Functional Requirements
- ✅ All 40 existing widgets accessible in new layout
- ✅ 3-tab navigation works smoothly (<100ms latency)
- ✅ Shared bottom panel consistent across tabs
- ✅ Frame selection updates all relevant displays
- ✅ FIG filtering and extraction functional
- ✅ Multi-stream processing accessible

### User Experience
- ✅ Intuitive navigation (broadcast engineer-friendly)
- ✅ No data loss when switching tabs
- ✅ All panels resizable via splitters
- ✅ Keyboard shortcuts functional
- ✅ Professional status messages and feedback

### Code Quality
- ✅ Clean separation of concerns (UI vs logic)
- ✅ Proper signal/slot connections
- ✅ No memory leaks (smart pointers used)
- ✅ Maintainable code structure
- ✅ Comprehensive comments
- ✅ Ready for Qt6 deployment

---

## 📝 Quick Reference

### Window Layout Formula
```
Main Window (1920x1080)
├── Main Tab Widget (70% height = ~756px)
│   ├── Tab 1: ETI DAB Ensemble Explorer
│   ├── Tab 2: FIC Analysis Tools
│   └── Tab 3: Advanced Features
└── Shared Bottom Panel (30% height = 250px)
    ├── Tab 1: System Messages
    ├── Tab 2: Performance
    ├── Tab 3: Error Detection
    ├── Tab 4: Logging
    └── Tab 5: Constellation
```

### Color Quick Reference
```css
/* Background */
--primary-bg: #f5f5f5;
--secondary-bg: #ffffff;

/* Status */
--status-active: #4CAF50;   /* Green */
--status-warning: #FF9800;  /* Orange */
--status-error: #F44336;    /* Red */
--status-info: #2196F3;     /* Blue */

/* Hex Viewer */
--hex-sync: #d32f2f;        /* Red - Sync */
--hex-header: #f57c00;      /* Orange - Header */
--hex-fic: #388e3c;         /* Green - FIC */
--hex-msc: #1976d2;         /* Blue - MSC */
```

### Widget Hierarchy Quick Map
```
QMainWindow
├── QTabWidget (main_tabs)
│   ├── Tab1: QWidget
│   │   └── QSplitter (3 columns: 25% | 50% | 25%)
│   ├── Tab2: QWidget
│   │   └── QSplitter (2 columns: 50% | 50%)
│   └── Tab3: QWidget
│       └── QTabWidget (3 sub-tabs)
└── QTabWidget (shared_bottom_panel)
    └── 5 tabs (consistent across all main tabs)
```

---

**Document Version:** 1.0  
**Last Updated:** 2025-10-23  
**Author:** UI/UX Design Expert (Claude Sonnet 4.5)  
**Status:** ✅ Ready for Implementation

---

## 📞 Next Steps

1. **Read detailed specifications:** Review UI_DESIGN_TAB1/2/3_SPECIFICATION.md
2. **Review stylesheet:** Check UI_DESIGN_STYLESHEET.qss for professional appearance
3. **Study data mapping:** See UI_DESIGN_DATA_MAPPING.md for widget migration
4. **Follow implementation guide:** Use UI_DESIGN_IMPLEMENTATION_CODE.md for Qt6 code
5. **Test user flows:** Validate against UI_DESIGN_USER_FLOWS.md scenarios

For questions or clarifications, refer to the comprehensive specification documents listed in the "Related Documentation" section above.
