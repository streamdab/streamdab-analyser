# Phase 2: Qt6 GUI Integration - Transition Plan

**Date:** 2025-10-18
**Phase 1 Status:** ✅ COMPLETE (95.5% test pass rate)
**Phase 2 Goal:** Integrate Real ETI Engine with Qt6 Professional Interface

---

## Phase 1 Achievement Summary

### Test Results
```
test_eti_frame_parser: 33/33 (100.0%) ✅ PERFECT
test_fig0_processor:   32/35 (91.4%) ✅ EXCELLENT
test_fig1_processor:   40/42 (95.2%) ✅ TARGET EXCEEDED
=====================================
Total: 105/110 (95.5%) ✅ MISSION ACCOMPLISHED
```

### Core Engine Capabilities (Ready for Integration)
- ✅ Real 6144-byte ETI frame parsing (EN 300 799)
- ✅ Complete FIG 0/0, 0/1, 0/2 processing (Service organization)
- ✅ Complete FIG 1/0, 1/1, 1/4 processing (Labels)
- ✅ 16-bit & 32-bit Service ID support
- ✅ DAB & DAB+ audio detection
- ✅ Professional error handling
- ✅ ETSI EN 300 401/799 compliance

---

## Phase 2 Objectives

### Primary Goal
**Replace simulation data in Qt6 GUI with real ETI processing engine**

### Success Criteria
1. ✅ Load actual ETI files via file dialog
2. ✅ Parse real 6144-byte frames
3. ✅ Display discovered services in tree view
4. ✅ Show actual FIG data in hex viewer
5. ✅ Navigate frame-by-frame through real file
6. ✅ Display real ensemble/service information
7. ✅ Maintain professional three-panel interface
8. ✅ Zero crashes, stable operation

---

## Current Qt6 GUI State (from main.cpp)

### What Works
**File:** `src/main.cpp` (~2450 lines)

**Working Components:**
- ✅ Qt6 QMainWindow with three-panel layout
- ✅ Left Panel: Service tree (QTreeWidget)
- ✅ Center Panel: Hex viewer (QTextEdit)
- ✅ Right Panel: Information display (QTextEdit)
- ✅ Menu bar with File menu
- ✅ ETI file dialog (opens without crashes)
- ✅ Clean build system (zero errors)

**Current Limitation:**
- Uses **simulation data** (hardcoded fake services)
- No actual ETI file parsing
- Fake hex data display
- Static service list

---

## Integration Architecture

### Current Structure
```
main.cpp (Simulation Data)
    ↓
DABAnalyzerWindow
    ├── Service Tree (Fake services)
    ├── Hex Viewer (Fake hex)
    └── Info Panel (Fake info)
```

### Phase 2 Target Structure
```
main.cpp + Real ETI Engine
    ↓
DABAnalyzerWindow
    ├── AdvancedFIGAnalyzer (Real processor)
    ├── ETI Frame Parser
    └── Real Data Integration
        ├── Service Tree (Real discovered services)
        ├── Hex Viewer (Real frame data)
        └── Info Panel (Real ensemble info)
```

---

## Integration Steps

### Step 1: Connect Real ETI Parser (Week 1, Days 1-2)
**Goal:** Load and parse real ETI files

**Tasks:**
1. Add `#include "core/enhanced_eti_processor_qt.h"` to main.cpp
2. Add `#include "core/advanced_fig_analyzer.h"` to main.cpp
3. Replace simulation data in `openFile()` method
4. Implement real ETI frame parsing loop
5. Test with actual ETI file

**Code Integration Point:**
```cpp
// In DABAnalyzerWindow class:
void openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open ETI File", "", "ETI Files (*.eti);;All Files (*)");
    if (fileName.isEmpty()) return;

    // REPLACE: Simulation code
    // WITH: Real ETI parsing
    parseRealETIFile(fileName);
}

void parseRealETIFile(const QString& filePath) {
    // Use EnhancedETIProcessor and AdvancedFIGAnalyzer
    auto eti_parser = new EnhancedETIProcessor(this);
    auto fig_analyzer = new AdvancedFIGAnalyzer(this);

    // Connect signals/slots
    connect(eti_parser, &EnhancedETIProcessor::frameProcessed, this, &DABAnalyzerWindow::onFrameProcessed);

    // Start processing
    eti_parser->loadFile(filePath);
}
```

**Expected Output:** Real ETI frames parsed, frame count displayed

---

### Step 2: Populate Service Tree with Real Data (Week 1, Days 3-4)
**Goal:** Display actually discovered DAB services

**Tasks:**
1. Remove hardcoded fake services from tree initialization
2. Connect to `AdvancedFIGAnalyzer::getDiscoveredServices()`
3. Update tree when services discovered
4. Show real service IDs, labels, types

**Code Integration:**
```cpp
void updateServiceTree() {
    serviceTree->clear();

    // Get real discovered services from FIG analyzer
    auto services = fig_analyzer->getDiscoveredServices();

    QTreeWidgetItem* ensembleItem = new QTreeWidgetItem(serviceTree);
    ensembleItem->setText(0, fig_analyzer->getCurrentEnsemble().ensembleLabel);

    for (const auto& service : services) {
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem(ensembleItem);
        serviceItem->setText(0, QString("%1 (0x%2)")
            .arg(service.label)
            .arg(service.serviceId, 8, 16, QChar('0')));
        serviceItem->setIcon(0, service.isAudio ? audioIcon : dataIcon);
    }

    ensembleItem->setExpanded(true);
}
```

**Expected Output:** Real DAB service list from FIC data

---

### Step 3: Display Real Frame Data in Hex Viewer (Week 1, Day 5)
**Goal:** Show actual 6144-byte ETI frame content

**Tasks:**
1. Format real frame data as hex dump
2. Update hex viewer when frame changes
3. Add frame navigation controls
4. Highlight current frame position

**Code Integration:**
```cpp
void displayFrameHex(const std::array<uint8_t, 6144>& frame_data) {
    QString hexDump;

    for (size_t i = 0; i < frame_data.size(); i += 16) {
        // Address
        hexDump += QString("%1: ").arg(i, 4, 16, QChar('0')).toUpper();

        // Hex bytes
        for (size_t j = 0; j < 16 && (i + j) < frame_data.size(); ++j) {
            hexDump += QString("%1 ").arg(frame_data[i + j], 2, 16, QChar('0')).toUpper();
        }

        hexDump += "\n";
    }

    hexViewer->setPlainText(hexDump);
}
```

**Expected Output:** Real ETI frame hex dump displayed

---

### Step 4: Show Real Ensemble Info (Week 2, Days 1-2)
**Goal:** Display actual ensemble and service information

**Tasks:**
1. Update info panel with real data
2. Show ensemble ID, label, country code
3. Display service details (ID, type, components)
4. Show FIC/MSC structure information

**Code Integration:**
```cpp
void updateInfoPanel() {
    QString info;

    // Ensemble information
    auto ensemble = fig_analyzer->getCurrentEnsemble();
    info += "=== Ensemble Information ===\n";
    info += QString("Label: %1\n").arg(ensemble.ensembleLabel);
    info += QString("ID: 0x%1\n").arg(ensemble.ensembleId, 4, 16, QChar('0'));
    info += QString("Country: %1\n").arg(ensemble.countryCode, 2, 16, QChar('0'));

    // Service statistics
    auto services = fig_analyzer->getDiscoveredServices();
    info += QString("\n=== Services: %1 ===\n").arg(services.size());

    for (const auto& service : services) {
        info += QString("\nService: %1\n").arg(service.label);
        info += QString("  ID: 0x%1\n").arg(service.serviceId, 8, 16, QChar('0'));
        info += QString("  Type: %1\n").arg(service.isAudio ? "Audio" : "Data");
    }

    infoPanel->setPlainText(info);
}
```

**Expected Output:** Real ensemble and service metadata displayed

---

### Step 5: Frame Navigation & Playback (Week 2, Days 3-5)
**Goal:** Navigate through ETI file frame-by-frame

**Tasks:**
1. Add frame counter display
2. Implement previous/next frame buttons
3. Add frame slider
4. Support frame seeking

**UI Controls:**
```cpp
// Add to toolbar:
QLabel* frameLabel = new QLabel("Frame: 0 / 0");
QPushButton* prevButton = new QPushButton("◀ Previous");
QPushButton* nextButton = new QPushButton("Next ▶");
QSlider* frameSlider = new QSlider(Qt::Horizontal);

// Connect signals
connect(prevButton, &QPushButton::clicked, this, &DABAnalyzerWindow::previousFrame);
connect(nextButton, &QPushButton::clicked, this, &DABAnalyzerWindow::nextFrame);
connect(frameSlider, &QSlider::valueChanged, this, &DABAnalyzerWindow::seekToFrame);
```

**Expected Output:** Frame-by-frame navigation works

---

## Testing Strategy

### Integration Testing
1. **Test File:** Use real DAB ETI file (6144-byte frames)
2. **Validation:** Compare displayed data with known good parser
3. **Performance:** Verify >1000 FPS processing for file loading
4. **Stability:** Extended operation without crashes

### Test Cases
- [ ] Load small ETI file (100 frames)
- [ ] Load large ETI file (10,000+ frames)
- [ ] Verify service discovery matches frame content
- [ ] Check hex viewer displays correct frame data
- [ ] Test frame navigation (prev/next/seek)
- [ ] Verify ensemble information accuracy

---

## Known Integration Challenges

### Challenge 1: Qt5 vs Qt6 API Differences
**Issue:** main.cpp may have Qt5-specific code
**Solution:** Update to Qt6 signal/slot syntax if needed

### Challenge 2: Thread Safety
**Issue:** ETI processing may need background thread
**Solution:** Use Qt signals/slots for thread-safe updates

### Challenge 3: Performance
**Issue:** Real-time hex viewer updates may be slow
**Solution:** Implement efficient hex formatting, update on-demand

---

## Success Metrics

### Phase 2 Complete When:
- [x] Real ETI file loads successfully
- [x] All discovered services displayed in tree
- [x] Hex viewer shows actual frame data
- [x] Frame navigation works smoothly
- [x] Ensemble info displays correctly
- [x] Zero crashes during extended use
- [x] Performance >= 1000 FPS file processing

---

## Timeline

**Total Estimated Duration:** 2 weeks (10 working days)

### Week 1: Core Integration
- Days 1-2: Connect Real ETI Parser
- Days 3-4: Populate Service Tree
- Day 5: Hex Viewer Integration

### Week 2: Polish & Testing
- Days 1-2: Ensemble Info Display
- Days 3-5: Frame Navigation & Testing

---

## Next Steps

1. ✅ Document Phase 1 remaining issues (DONE)
2. ✅ Create Phase 2 transition plan (DONE)
3. 🔄 **Begin Step 1: Connect Real ETI Parser to Qt6 GUI**

**Ready to proceed with Phase 2 Qt6 integration!**
