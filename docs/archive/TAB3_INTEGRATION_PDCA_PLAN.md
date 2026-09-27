# Tab 3 Integration - PDCA Plan (Phase 2)

**Date:** October 27, 2025  
**Status:** 🔄 IN PROGRESS  
**Goal:** Integrate BitLevelFIGParser + HexViewerFormatter in main.cpp  
**Methodology:** PDCA (Plan-Do-Check-Act) with parallel agent execution

---

## 📋 **PLAN Phase**

### **Current State Assessment**

**Phase 1 Complete:**
- ✅ Agent 1: BitLevelFIGParser implemented and tested (12 tests passing)
- ✅ Agent 2: HexViewerFormatter enhanced and tested (8 tests passing)
- ✅ Build status: Zero compilation errors
- ✅ Components ready for integration

**Phase 2 Goal:**
- Integrate both components in main.cpp
- Connect signal/slot architecture for Tab 3
- Populate "Size" column with bit lengths
- Enable hex viewer highlighting on FIG selection

### **Gap Analysis**

**Missing Integration Points:**

1. **Tab 3 FIG Item Details Tree Update:**
   - Current: Uses FigParser results (no bit-level tracking)
   - Needed: Use BitLevelFIGParser to get bit offsets/lengths
   - Impact: "Size" column currently shows "N/A"

2. **Hex Viewer Highlighting Connection:**
   - Current: No signal connection for FIG selection
   - Needed: Connect FIG tree selection → hex viewer highlight
   - Impact: Users can't visualize FIG byte ranges

3. **Bit-to-Byte Conversion:**
   - Needed: Convert bit_offset to byte offset for hex viewer
   - Algorithm: `byte_offset = bit_offset / 8`
   - Handle: Partial byte highlighting (bit granularity)

---

## 🎯 **PARALLEL AGENT STRATEGY**

### **Agent 3 (typescript-pro): Main.cpp Tab 3 Integration**

**Responsibility:** Integrate BitLevelFIGParser with Tab 3 UI

**Tasks:**
1. Add `#include "bit_level_fig_parser.hpp"` to main.cpp
2. Update `updateTab3FIGItemDetailsTree()` method:
   - Create BitLevelFIGParser instance
   - Parse FIG data to get bit-level analysis
   - Populate tree items with bit_length in "Size" column
   - Store bit_offset/bit_length in item->setData() for hex sync
   - Handle nesting levels for hierarchical display

3. Add helper method `getCurrentFIBData()`:
   - Return current FIB raw data for hex viewer
   - Cache FIB data from frame processing

4. Update `onFIGDiscovered()` slot:
   - Store FIG raw data for bit-level parsing
   - Ensure FIG data is available for Tab 3

**Expected Output:**
- Modified main.cpp (+50 lines)
- "Size" column populated with bit lengths
- FIG data cached for hex viewer access

---

### **Agent 4 (typescript-pro): Signal/Slot Hex Highlighting**

**Responsibility:** Connect FIG selection to hex viewer highlighting

**Tasks:**
1. Update `setupTab3SignalConnections()` method:
   - Add signal connection: `m_tab3_figItemDetailsTree::itemSelectionChanged`
   - Create lambda slot for hex highlighting
   - Extract bit_offset/bit_length from selected item
   - Convert bit range to byte range
   - Apply highlighting to hex viewer

2. Implement bit-to-byte conversion:
   ```cpp
   uint16_t startByte = bitOffset / 8;
   uint16_t endByte = (bitOffset + bitLength - 1) / 8;
   ```

3. Handle edge cases:
   - No FIB data available
   - Multiple item selection (highlight first only)
   - Deselection (clear highlights)

4. Optimize performance:
   - Throttle updates if needed
   - Cache hex HTML generation

**Expected Output:**
- Modified main.cpp (+30 lines)
- Signal connection established
- Hex viewer highlights on FIG selection

---

## 🔧 **TECHNICAL SPECIFICATIONS**

### **Agent 3: Tab 3 Tree Population**

#### **Method: updateTab3FIGItemDetailsTree()**

**Current Implementation (Simplified):**
```cpp
void updateTab3FIGItemDetailsTree() {
    if (!m_tab3_figItemDetailsTree) return;
    m_tab3_figItemDetailsTree->clear();
    
    // Current: Use FigParser results (no bit tracking)
    // Populate tree with field names and values
    // "Size" column shows "N/A"
}
```

**Target Implementation:**
```cpp
void updateTab3FIGItemDetailsTree(const FIGAnalysisResult& figResult) {
    if (!m_tab3_figItemDetailsTree) return;
    m_tab3_figItemDetailsTree->clear();
    
    // Use BitLevelFIGParser for bit-precise field tracking
    eti::bit_level::BitLevelFIGParser bitParser;
    eti::bit_level::FIGBitLevelAnalysis analysis;
    
    // Parse FIG data with bit-level tracking
    if (!eti::bit_level::BitLevelFIGParser::parse(
            figResult.raw_data,
            figResult.fig_type,
            figResult.fig_extension,
            analysis)) {
        // Parsing failed - show error or fallback
        qWarning() << "BitLevelFIGParser: Failed to parse FIG"
                   << figResult.fig_type << "/" << figResult.fig_extension;
        return;
    }
    
    // Track parent items for nesting
    QMap<int, QTreeWidgetItem*> levelParents;
    levelParents[0] = nullptr; // Top-level has no parent
    
    // Build tree from bit-level analysis
    for (const auto& field : analysis.fields) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        
        // Column 0: Property (field name)
        item->setText(0, field.name);
        
        // Column 1: Size (bit length) - THIS IS THE KEY FEATURE!
        item->setText(1, QString("%1 bits").arg(field.bit_length));
        
        // Column 2: Value (hex)
        item->setText(2, QString("0x%1").arg(field.value, 0, 16));
        
        // Column 3: Information (human-readable)
        item->setText(3, field.information);
        
        // Store bit_offset and bit_length for hex viewer sync
        item->setData(0, Qt::UserRole, field.bit_offset);
        item->setData(0, Qt::UserRole + 1, field.bit_length);
        
        // Add item at correct nesting level
        if (field.nesting_level == 0) {
            m_tab3_figItemDetailsTree->addTopLevelItem(item);
            levelParents[0] = item; // Track for children
        } else {
            // Find parent at level-1
            QTreeWidgetItem* parent = levelParents.value(field.nesting_level - 1);
            if (parent) {
                parent->addChild(item);
                levelParents[field.nesting_level] = item; // Track for grandchildren
            } else {
                // Fallback: add as top-level if parent not found
                m_tab3_figItemDetailsTree->addTopLevelItem(item);
            }
        }
    }
    
    // Expand all items for better visibility
    m_tab3_figItemDetailsTree->expandAll();
}
```

**Key Changes:**
- ✅ Use BitLevelFIGParser instead of FigParser
- ✅ Populate "Size" column with bit_length
- ✅ Store bit_offset/bit_length in item data
- ✅ Handle nesting levels for hierarchical display
- ✅ Error handling for unsupported FIG types

---

### **Agent 4: Hex Viewer Highlighting Connection**

#### **Method: setupTab3SignalConnections()**

**Add to Existing Connections:**
```cpp
void setupTab3SignalConnections() {
    // Existing Phase 8-10 connections...
    
    // NEW: Phase 11 - FIG Item Details selection → Hex viewer highlighting
    connect(m_tab3_figItemDetailsTree, &QTreeWidget::itemSelectionChanged,
            this, [this]() {
        // Guard: Check if widgets exist
        if (!m_tab3_figItemDetailsTree || !m_tab3_fibHexViewer) {
            return;
        }
        
        // Get selected items
        auto selectedItems = m_tab3_figItemDetailsTree->selectedItems();
        
        // Clear highlights if no selection
        if (selectedItems.isEmpty()) {
            if (m_hexFormatter) {
                m_hexFormatter->clearHighlights();
                // Refresh hex viewer without highlights
                QByteArray fibData = getCurrentFIBData();
                if (!fibData.isEmpty()) {
                    QString html = m_hexFormatter->formatToHtmlString(fibData);
                    m_tab3_fibHexViewer->setHtml(html);
                }
            }
            return;
        }
        
        // Get first selected item (ignore multiple selection)
        QTreeWidgetItem* item = selectedItems.first();
        
        // Extract bit offset and length from item data
        bool ok1 = false, ok2 = false;
        uint16_t bitOffset = item->data(0, Qt::UserRole).toUInt(&ok1);
        uint8_t bitLength = item->data(0, Qt::UserRole + 1).toUInt(&ok2);
        
        // Validate data extraction
        if (!ok1 || !ok2 || bitLength == 0) {
            qWarning() << "Tab3: Invalid bit offset/length data in tree item";
            return;
        }
        
        // Convert bit range to byte range
        // Formula: startByte = bitOffset / 8
        //          endByte = (bitOffset + bitLength - 1) / 8
        uint16_t startByte = bitOffset / 8;
        uint16_t endByte = (bitOffset + bitLength - 1) / 8;
        
        qDebug() << "Tab3: Highlighting FIG field:"
                 << "bit_offset=" << bitOffset
                 << "bit_length=" << bitLength
                 << "→ bytes [" << startByte << "-" << endByte << "]";
        
        // Apply highlighting
        if (m_hexFormatter) {
            m_hexFormatter->clearHighlights();
            m_hexFormatter->highlightRange(startByte, endByte, "#E0E0E0");
            
            // Get current FIB data
            QByteArray fibData = getCurrentFIBData();
            if (fibData.isEmpty()) {
                qWarning() << "Tab3: No FIB data available for hex viewer";
                return;
            }
            
            // Generate HTML with highlighting and update viewer
            QString highlightedHtml = m_hexFormatter->formatToHtmlString(fibData);
            m_tab3_fibHexViewer->setHtml(highlightedHtml);
        }
    });
    
    qDebug() << "[PHASE 11] Tab 3 hex viewer highlighting connected";
}
```

**Helper Method: getCurrentFIBData()**
```cpp
QByteArray getCurrentFIBData() const {
    // Return cached FIB data from current frame
    // This should be set during frame processing
    
    if (m_currentFrameIndex >= 0 && m_currentFrameIndex < m_totalFrames) {
        // Check if we have cached FIB data
        if (m_currentFIBData.isEmpty()) {
            qWarning() << "getCurrentFIBData: No FIB data cached for frame"
                       << m_currentFrameIndex;
        }
        return m_currentFIBData;
    }
    
    return QByteArray();
}
```

**Member Variable Addition:**
```cpp
private:
    QByteArray m_currentFIBData;  // Cache current FIB for hex viewer
```

**Update Frame Processing to Cache FIB:**
```cpp
void onFrameProcessed(ProcessedFrame frame) {
    // Existing frame processing...
    
    // Cache FIB data for Tab 3 hex viewer
    if (!frame.fic_data.isEmpty()) {
        // Extract first FIB (96 bytes) for Tab 3 display
        if (frame.fic_data.size() >= 96) {
            m_currentFIBData = frame.fic_data.left(96);
        } else {
            m_currentFIBData = frame.fic_data;
        }
    }
}
```

---

## ✅ **DO Phase - Parallel Execution Plan**

### **Agent 3 Task:**
```
Integrate BitLevelFIGParser in main.cpp:
1. Add #include "bit_level_fig_parser.hpp"
2. Update updateTab3FIGItemDetailsTree() to use BitLevelFIGParser
3. Populate "Size" column with bit_length
4. Store bit_offset/bit_length in item data
5. Handle nesting levels for tree hierarchy
6. Add getCurrentFIBData() helper method
7. Cache FIB data in onFrameProcessed()
8. Build verification
```

### **Agent 4 Task:**
```
Connect hex viewer highlighting signals:
1. Update setupTab3SignalConnections()
2. Add itemSelectionChanged signal connection
3. Implement bit-to-byte conversion
4. Apply highlighting with HexViewerFormatter
5. Handle edge cases (no data, deselection)
6. Add debug logging for troubleshooting
7. Build verification
```

### **Integration Coordination:**
- Both agents modify main.cpp (different methods - no conflicts)
- Agent 3 focuses on tree population (updateTab3FIGItemDetailsTree)
- Agent 4 focuses on signal connections (setupTab3SignalConnections)
- Both agents need to understand member variable additions

---

## 🔍 **CHECK Phase - Verification Criteria**

### **Build Verification**
- [ ] Zero compilation errors
- [ ] Zero warnings
- [ ] All existing tests pass (205+ tests)
- [ ] No regressions in Tab 1 or Tab 2

### **Functional Verification - Tab 3 FIG Item Details**
- [ ] "Size" column displays bit lengths (e.g., "1 bits", "16 bits")
- [ ] "Property" column shows field names
- [ ] "Value" column shows hex values
- [ ] "Information" column shows descriptions
- [ ] Tree hierarchy matches nesting levels (Level 0, Level 1)
- [ ] All FIG 0/2 fields parsed correctly

### **Functional Verification - Hex Viewer Highlighting**
- [ ] Selecting FIG field highlights corresponding bytes in hex viewer
- [ ] Grey background (#E0E0E0) applied to highlighted bytes
- [ ] Highlighting works in both HEX and ASCII columns
- [ ] Deselecting field clears highlighting
- [ ] Multiple selections highlight first item only
- [ ] Byte range conversion accurate (bit_offset → byte_offset)

### **End-to-End Testing**
- [ ] Load Bangkok ETI file (5000+ frames)
- [ ] Navigate to Tab 3 FIC-Extractor
- [ ] Select FIG 0/2 in Instance Tree
- [ ] Verify Item Details tree populates with bit lengths
- [ ] Select field in Item Details tree
- [ ] Verify hex viewer highlights correct bytes
- [ ] Test with different FIG types (0/0, 0/1, 0/2)

### **Performance Verification**
- [ ] Tree population: <10ms for typical FIG
- [ ] Highlighting: <5ms for selection change
- [ ] No UI lag or freezing
- [ ] Memory usage stable (no leaks)

---

## 🚀 **ACT Phase - Completion Actions**

### **Documentation Updates**
1. Update CLAUDE.md:
   - Tab 3 status: 95% → 100% ✅
   - Mark "Size" column: ⚠️ → ✅
   - Mark "Hex highlighting": ⚠️ → ✅

2. Update DATA_AVAILABILITY_MATRIX.md:
   - Tab 3 LEFT panel: ✅ 100%
   - Tab 3 RIGHT panel: ✅ 100%
   - Tab 3 "Size" column: ✅ 100%
   - Tab 3 hex highlighting: ✅ 100%
   - Overall Tab 3: ⚠️ 95% → ✅ 100%

3. Create TAB3_COMPLETION_REPORT.md:
   - Summary of Phase 1 + Phase 2
   - Screenshots of working Tab 3
   - Performance metrics
   - ETSI compliance validation

4. Update README.md:
   - Add Tab 3 features to feature list
   - Update screenshots if available
   - Mention bit-level FIG analysis capability

### **Git Commits**
```bash
# Commit 1: Agent 3 - Tree integration
git add src/core/bit_level_fig_parser.* src/main.cpp
git commit -m "feat: Integrate BitLevelFIGParser in Tab 3 FIG Item Details tree

- Add bit-level field tracking to updateTab3FIGItemDetailsTree()
- Populate 'Size' column with bit lengths per ETSI EN 300 401
- Store bit_offset/bit_length in tree item data for hex sync
- Handle hierarchical nesting levels (Level 0, Level 1)
- Add getCurrentFIBData() helper for hex viewer access
- Cache FIB data in onFrameProcessed() for Tab 3

PDCA Phase: DO (Agent 3)
Status: Tab 3 tree population complete"

# Commit 2: Agent 4 - Hex highlighting
git add src/core/hex_viewer_formatter.* src/main.cpp
git commit -m "feat: Connect Tab 3 hex viewer highlighting on FIG selection

- Add signal connection: FIG Item Details selection → hex highlight
- Implement bit-to-byte conversion for byte range highlighting
- Apply grey background (#E0E0E0) to selected FIG field bytes
- Handle edge cases: no data, deselection, multiple items
- Add debug logging for troubleshooting

PDCA Phase: DO (Agent 4)
Status: Hex viewer highlighting complete"

# Commit 3: Integration complete
git add docs/*.md
git commit -m "docs: Complete Tab 3 FIC-Extractor - 100% DATA_AVAILABILITY_MATRIX compliance

Phase 1 (Agents 1+2):
- ✅ BitLevelFIGParser: Bit-precise field tracking (ETSI EN 300 401)
- ✅ HexViewerFormatter: Byte range highlighting capability

Phase 2 (Agents 3+4):
- ✅ Tab 3 tree integration with bit-level parser
- ✅ Hex viewer highlighting on FIG selection

Results:
- ✅ 'Size' column: 0% → 100% populated
- ✅ Hex highlighting: 0% → 100% functional
- ✅ Tab 3: 95% → 100% complete
- ✅ DATA_AVAILABILITY_MATRIX: 100% compliance

Test Coverage: 20 new tests, 100% pass rate
Build Status: Zero compilation errors
ETSI Compliance: EN 300 401 Section 5.2, Table 9

PDCA: PLAN → DO → CHECK → ACT (Complete)"
```

---

## 📊 **Success Metrics**

### **Quantitative**
- ✅ Zero compilation errors (required)
- ✅ Zero test regressions (required)
- ✅ Tab 3 "Size" column: 100% populated (target)
- ✅ Tab 3 hex highlighting: 100% functional (target)
- ✅ DATA_AVAILABILITY_MATRIX: 100% compliance (target)

### **Qualitative**
- ✅ Professional bit-level FIG analysis (commercial-grade)
- ✅ Visual hex viewer integration (better UX than commercial tools)
- ✅ ETSI EN 300 401 compliance (standards-based)
- ✅ Parallel agent execution (efficient workflow)

### **Timeline**
- ✅ **Phase 1:** 2 hours (Agents 1+2 parallel)
- ⏳ **Phase 2:** 1.5 hours (Agents 3+4 parallel - estimated)
- ⏳ **Testing:** 0.5 hours (end-to-end validation)
- ⏳ **Documentation:** 0.5 hours (updates + git commits)
- **Total:** ~4.5 hours (from 95% → 100%)

---

## 📝 **Notes**

### **Agent Coordination**
- Agent 3: Modifies `updateTab3FIGItemDetailsTree()` + adds helpers
- Agent 4: Modifies `setupTab3SignalConnections()` + adds member vars
- Potential conflict: Both may add member variables (coordinate carefully)
- Solution: Agent 3 adds `m_currentFIBData`, Agent 4 only uses it

### **Testing Strategy**
1. Build after each agent completes
2. Run existing test suite (ensure no regressions)
3. Manual testing with real ETI file
4. Verify all Tab 3 features working
5. Performance profiling if needed

### **Risk Mitigation**
- Risk: Integration conflicts in main.cpp
  - Mitigation: Clear method separation, agents work on different methods
- Risk: FIB data not available
  - Mitigation: Add null checks, error messages, fallback behavior
- Risk: Bit-to-byte conversion errors
  - Mitigation: Unit test conversion logic, add debug logging

---

**Status:** 🔄 PLAN Phase Complete - Ready for DO Phase (Parallel Agent Execution)  
**Next Step:** Launch Agent 3 (typescript-pro) + Agent 4 (typescript-pro) simultaneously
