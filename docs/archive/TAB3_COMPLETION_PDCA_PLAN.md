# Tab 3 FIC-Extractor Completion - PDCA Plan

**Date:** October 27, 2025  
**Status:** 🔄 IN PROGRESS  
**Goal:** Complete remaining 5% of Tab 3 (bit-level tracking + hex highlighting)  
**Methodology:** PDCA (Plan-Do-Check-Act) with parallel agent execution

---

## 📋 **PLAN Phase**

### **Current State Assessment**

**Tab 3 Status: 95% Complete**
- ✅ FIG Instance Tree (7 columns) - WORKING
- ✅ FIG Item Details Tree (4 columns) - WORKING
- ✅ FIB Hex Viewer - WORKING
- ✅ Signal/slot connections (Phases 8-10) - WORKING
- ⚠️ **Missing:** Bit position/length tracking for "Size" column
- ⚠️ **Missing:** Hex viewer FIG byte highlighting

### **Gap Analysis**

**Problem 1: Bit-Level Field Tracking**
- Current: FIG parsers extract **values** only
- Needed: Track **bit offset** and **bit length** for each field
- Impact: "Size" column in FIG Item Details tree shows "N/A"

**Problem 2: Hex Viewer FIG Highlighting**
- Current: Hex viewer shows all bytes without context
- Needed: Highlight selected FIG bytes with grey background
- Impact: Users can't visually locate FIG data in hex dump

### **Implementation Strategy**

**Parallel Agent Approach:**
1. **Agent 1 (typescript-pro):** Implement BitLevelFIGParser wrapper class
2. **Agent 2 (typescript-pro):** Enhance HexViewerFormatter with highlighting
3. **Both agents work simultaneously** on independent components

---

## 🎯 **Agent 1: Bit-Level FIG Parser**

### **Objective**
Create `BitLevelFIGParser` wrapper that tracks bit positions during FIG parsing.

### **Deliverables**
1. New file: `src/core/bit_level_fig_parser.hpp`
2. New file: `src/core/bit_level_fig_parser.cpp`
3. Updated: `src/main.cpp` (integrate with Tab 3)

### **Technical Specifications**

#### **Data Structure:**
```cpp
namespace eti {
namespace bit_level {

struct BitField {
    std::string name;           // "SId", "TMId", "EId", etc.
    std::string information;    // Human-readable description
    uint32_t value;            // Extracted value
    uint16_t bit_offset;       // Bit position from FIG start (0-based)
    uint8_t bit_length;        // Size in bits (1-32)
    uint8_t nesting_level;     // Tree depth (0-4)
};

struct FIGBitLevelAnalysis {
    uint8_t fig_type;           // FIG type (0-7)
    uint8_t fig_extension;      // FIG extension (0-31)
    uint16_t fig_byte_offset;   // Byte offset in FIB
    uint16_t fig_byte_length;   // Total FIG length in bytes
    std::vector<BitField> fields;  // All extracted fields
};

class BitLevelFIGParser {
public:
    BitLevelFIGParser();
    
    // Parse FIG data and extract bit-level fields
    FIGBitLevelAnalysis parse(const QByteArray& figData, 
                              uint8_t figType, 
                              uint8_t figExtension);
    
private:
    // Helper: Extract bits from byte array
    uint32_t extractBits(const QByteArray& data, 
                        uint16_t bitOffset, 
                        uint8_t bitLength);
    
    // Type-specific parsers (examples)
    void parseFIG0_0(const QByteArray& data, FIGBitLevelAnalysis& result);
    void parseFIG0_1(const QByteArray& data, FIGBitLevelAnalysis& result);
    void parseFIG0_2(const QByteArray& data, FIGBitLevelAnalysis& result);
    // ... add more as needed
};

}} // namespace eti::bit_level
```

#### **Key Methods:**

**extractBits() - Core Bit Extraction:**
```cpp
uint32_t BitLevelFIGParser::extractBits(const QByteArray& data, 
                                        uint16_t bitOffset, 
                                        uint8_t bitLength)
{
    if (bitLength > 32) return 0;
    
    uint16_t startByte = bitOffset / 8;
    uint8_t startBit = bitOffset % 8;
    
    uint32_t result = 0;
    uint8_t bitsRead = 0;
    
    while (bitsRead < bitLength && startByte < data.size()) {
        uint8_t byte = static_cast<uint8_t>(data[startByte]);
        uint8_t bitsAvailable = 8 - startBit;
        uint8_t bitsToRead = std::min<uint8_t>(bitLength - bitsRead, bitsAvailable);
        
        // Extract bits from current byte
        uint8_t mask = ((1 << bitsToRead) - 1) << (bitsAvailable - bitsToRead);
        uint8_t extracted = (byte & mask) >> (bitsAvailable - bitsToRead);
        
        result = (result << bitsToRead) | extracted;
        bitsRead += bitsToRead;
        startByte++;
        startBit = 0;
    }
    
    return result;
}
```

**parseFIG0_2() - Example Service Organization Parser:**
```cpp
void BitLevelFIGParser::parseFIG0_2(const QByteArray& data, 
                                    FIGBitLevelAnalysis& result)
{
    uint16_t currentBit = 8; // Skip FIG header (1 byte)
    
    // Parse C/N flag (1 bit)
    uint8_t cn = extractBits(data, currentBit, 1);
    result.fields.push_back({
        "C/N", "Country/Network flag", cn, currentBit, 1, 0
    });
    currentBit += 1;
    
    // Parse OE flag (1 bit)
    uint8_t oe = extractBits(data, currentBit, 1);
    result.fields.push_back({
        "OE", "Other Ensemble flag", oe, currentBit, 1, 0
    });
    currentBit += 1;
    
    // Parse P/D flag (1 bit)
    uint8_t pd = extractBits(data, currentBit, 1);
    result.fields.push_back({
        "P/D", "Programme/Data flag", pd, currentBit, 1, 0
    });
    currentBit += 1;
    
    // Skip reserved bits (5 bits)
    currentBit += 5;
    
    // Parse Service ID (16 or 32 bits depending on P/D)
    uint8_t sidLength = (pd == 0) ? 16 : 32;
    uint32_t sid = extractBits(data, currentBit, sidLength);
    result.fields.push_back({
        "SId", "Service Identifier", sid, currentBit, sidLength, 1
    });
    currentBit += sidLength;
    
    // Continue parsing service components...
    // (Add more fields as needed)
}
```

#### **Integration with Tab 3:**
```cpp
// In main.cpp - updateTab3FIGItemDetailsTree()
void updateTab3FIGItemDetailsTree(const FIGAnalysisResult& figResult) {
    if (!m_tab3_figItemDetailsTree) return;
    m_tab3_figItemDetailsTree->clear();
    
    // Use bit-level parser
    eti::bit_level::BitLevelFIGParser bitParser;
    auto analysis = bitParser.parse(
        figResult.raw_data, 
        figResult.fig_type, 
        figResult.fig_extension
    );
    
    // Build tree with bit-level details
    for (const auto& field : analysis.fields) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, QString::fromStdString(field.name));
        item->setText(1, QString("%1 bits").arg(field.bit_length));  // ✅ Now shows size!
        item->setText(2, QString("0x%1").arg(field.value, 0, 16));
        item->setText(3, QString::fromStdString(field.information));
        
        // Store bit offset/length for hex highlighting
        item->setData(0, Qt::UserRole, field.bit_offset);
        item->setData(0, Qt::UserRole + 1, field.bit_length);
        
        m_tab3_figItemDetailsTree->addTopLevelItem(item);
    }
}
```

### **Testing Criteria**
- [ ] Bit offsets calculated correctly for FIG 0/2
- [ ] Bit lengths match ETSI EN 300 401 specification
- [ ] "Size" column displays "N bits" instead of "N/A"
- [ ] Nested fields show correct hierarchy (levels 0-4)

---

## 🎨 **Agent 2: Hex Viewer FIG Highlighting**

### **Objective**
Enhance `HexViewerFormatter` to highlight selected FIG byte ranges.

### **Deliverables**
1. Updated: `src/core/hex_viewer_formatter.hpp` (add highlight API)
2. Updated: `src/core/hex_viewer_formatter.cpp` (implement highlighting)
3. Updated: `src/main.cpp` (connect selection signal)

### **Technical Specifications**

#### **API Enhancement:**
```cpp
// In hex_viewer_formatter.hpp
class HexViewerFormatter {
public:
    // ... existing methods ...
    
    // NEW: Highlight byte range with color
    void highlightRange(uint16_t startByte, uint16_t endByte, const QString& color);
    
    // NEW: Clear all highlights
    void clearHighlights();
    
    // NEW: Set multiple highlight ranges (for multi-FIG selection)
    void setHighlightRanges(const std::vector<std::pair<uint16_t, uint16_t>>& ranges,
                           const QString& color);

private:
    struct HighlightRange {
        uint16_t start;
        uint16_t end;
        QString color;
    };
    std::vector<HighlightRange> m_highlights;
};
```

#### **Implementation:**
```cpp
// In hex_viewer_formatter.cpp
void HexViewerFormatter::highlightRange(uint16_t startByte, 
                                        uint16_t endByte, 
                                        const QString& color)
{
    m_highlights.push_back({startByte, endByte, color});
}

void HexViewerFormatter::clearHighlights()
{
    m_highlights.clear();
}

QString HexViewerFormatter::formatToHtmlString(const QByteArray& data)
{
    QString html = "<html><head><style>"
                   "body { font-family: 'Courier New', monospace; font-size: 12px; }"
                   ".addr { color: #0000FF; font-weight: bold; }"
                   ".hex { color: #000000; }"
                   ".ascii { color: #008000; }"
                   ".highlight { background-color: #E0E0E0; }"  // Grey background
                   "</style></head><body><pre>";
    
    for (int i = 0; i < data.size(); i += m_bytesPerLine) {
        // Address column
        html += QString("<span class='addr'>%1</span>  ")
                .arg(i, 4, 16, QChar('0')).toUpper();
        
        // Hex dump
        for (int j = 0; j < m_bytesPerLine && (i + j) < data.size(); ++j) {
            uint8_t byte = static_cast<uint8_t>(data[i + j]);
            bool isHighlighted = false;
            
            // Check if current byte is in highlight range
            for (const auto& hl : m_highlights) {
                if ((i + j) >= hl.start && (i + j) <= hl.end) {
                    isHighlighted = true;
                    break;
                }
            }
            
            if (isHighlighted) {
                html += QString("<span class='highlight'>%1</span> ")
                        .arg(byte, 2, 16, QChar('0')).toUpper();
            } else {
                html += QString("<span class='hex'>%1</span> ")
                        .arg(byte, 2, 16, QChar('0')).toUpper();
            }
        }
        
        // ASCII column (if enabled)
        if (m_asciiColumn) {
            html += "  |";
            for (int j = 0; j < m_bytesPerLine && (i + j) < data.size(); ++j) {
                char c = data[i + j];
                if (c >= 32 && c <= 126) {
                    html += QString("<span class='ascii'>%1</span>").arg(c);
                } else {
                    html += "<span class='ascii'>.</span>";
                }
            }
            html += "|";
        }
        
        html += "\n";
    }
    
    html += "</pre></body></html>";
    return html;
}
```

#### **Integration with Tab 3:**
```cpp
// In main.cpp - Connect FIG Item Details selection to hex viewer
void setupTab3SignalConnections()
{
    // Phase 10: FIG Item Details selection → Hex viewer highlighting
    connect(m_tab3_figItemDetailsTree, &QTreeWidget::itemSelectionChanged,
            this, [this]() {
        auto selectedItems = m_tab3_figItemDetailsTree->selectedItems();
        if (selectedItems.isEmpty() || !m_tab3_fiberViewer) return;
        
        // Get bit offset/length from selected item
        QTreeWidgetItem* item = selectedItems.first();
        uint16_t bitOffset = item->data(0, Qt::UserRole).toUInt();
        uint8_t bitLength = item->data(0, Qt::UserRole + 1).toUInt();
        
        // Convert bit range to byte range
        uint16_t startByte = bitOffset / 8;
        uint16_t endByte = (bitOffset + bitLength - 1) / 8;
        
        // Highlight in hex viewer
        m_hexFormatter->clearHighlights();
        m_hexFormatter->highlightRange(startByte, endByte, "#E0E0E0");
        
        // Refresh hex viewer display
        QByteArray fibData = getCurrentFIBData();  // Get current FIB data
        QString highlightedHtml = m_hexFormatter->formatToHtmlString(fibData);
        m_tab3_fiberViewer->setHtml(highlightedHtml);
    });
}
```

### **Testing Criteria**
- [ ] Selected FIG field highlights correct bytes in hex viewer
- [ ] Grey background (#E0E0E0) applied to highlighted bytes
- [ ] Multi-byte fields highlight full range
- [ ] Bit-to-byte conversion accurate
- [ ] Highlighting clears when selection changes

---

## ✅ **DO Phase - Parallel Execution Plan**

### **Agent 1 Task:**
```
Create BitLevelFIGParser class with:
1. File: src/core/bit_level_fig_parser.hpp (header)
2. File: src/core/bit_level_fig_parser.cpp (implementation)
3. Implement: extractBits() core function
4. Implement: parseFIG0_2() as reference example
5. Add to CMakeLists.txt
6. Build verification
```

### **Agent 2 Task:**
```
Enhance HexViewerFormatter with:
1. Add highlightRange() API to hex_viewer_formatter.hpp
2. Implement highlighting in formatToHtmlString()
3. Add clearHighlights() method
4. Test with sample byte ranges
5. Build verification
```

### **Integration Task (After Agents Complete):**
```
Integrate both components in main.cpp:
1. Add BitLevelFIGParser usage in updateTab3FIGItemDetailsTree()
2. Connect FIG Item Details selection signal
3. Wire hex viewer highlighting on selection
4. Test end-to-end flow
```

---

## 🔍 **CHECK Phase - Verification Criteria**

### **Build Verification**
- [ ] Zero compilation errors
- [ ] Zero warnings
- [ ] All tests pass (existing 205+ tests)

### **Functional Verification**
- [ ] Tab 3 FIG Item Details "Size" column shows bit lengths
- [ ] Hex viewer highlights selected FIG field bytes
- [ ] Grey background applied correctly
- [ ] Selection sync works in both directions
- [ ] Performance: No lag when selecting fields

### **DATA_AVAILABILITY_MATRIX Compliance**
- [ ] Tab 3 LEFT panel: 100% complete
- [ ] Tab 3 RIGHT panel: 100% complete
- [ ] FIG Item Details tree: 4 columns all populated
- [ ] Hex viewer: FIG highlighting operational

---

## 🚀 **ACT Phase - Completion Actions**

### **Documentation Updates**
1. Update CLAUDE.md: Tab 3 status 95% → 100%
2. Update DATA_AVAILABILITY_MATRIX.md: Mark Tab 3 ⚠️ → ✅
3. Create TAB3_COMPLETION_REPORT.md with screenshots
4. Update README.md with new Tab 3 features

### **Git Commits**
1. Agent 1: `feat: Implement BitLevelFIGParser for Tab 3 bit-level tracking`
2. Agent 2: `feat: Add hex viewer FIG highlighting to Tab 3`
3. Integration: `feat: Complete Tab 3 FIC-Extractor - 100% DATA_AVAILABILITY_MATRIX compliance`

### **Testing & Validation**
1. Test with real Bangkok ETI file
2. Verify all FIG types display correctly
3. Validate bit offsets match ETSI EN 300 401 spec
4. Performance test with 5000-frame file

---

## 📊 **Success Metrics**

### **Quantitative**
- ✅ 100% DATA_AVAILABILITY_MATRIX compliance (currently 95%)
- ✅ Zero compilation errors
- ✅ Zero test regressions
- ✅ Tab 3 "Size" column: 100% populated (currently N/A)

### **Qualitative**
- ✅ Professional bit-level FIG analysis (matches commercial tools)
- ✅ Visual hex viewer integration (better UX)
- ✅ ETSI EN 300 401 compliance validation

### **Timeline**
- Agent 1 (BitLevelFIGParser): 1.5-2 hours
- Agent 2 (Hex highlighting): 1-1.5 hours
- Integration + Testing: 0.5-1 hour
- **Total: 3-4 hours** (with parallel execution: 2-2.5 hours)

---

## 📝 **Notes**

### **ETSI EN 300 401 References**
- Section 5.2: FIG Type 0 definitions
- Table 9: FIG 0/2 Service Organization format
- Table 10: FIG 0/1 Sub-channel Organization format

### **Priority FIG Types for Bit-Level Parsing**
1. FIG 0/2 (Service Organization) - HIGH
2. FIG 0/1 (Sub-channel Organization) - HIGH
3. FIG 0/0 (Ensemble Information) - MEDIUM
4. FIG 1/0 (Ensemble Label) - MEDIUM
5. FIG 1/1 (Service Label) - MEDIUM

### **Future Enhancements**
- Add all 20 FIG types to BitLevelFIGParser
- Support FIG Type 1 (labels) bit-level parsing
- Support FIG Type 2 (extended labels) bit-level parsing
- Add FIG field validation (check ranges per ETSI spec)

---

**Status:** 🔄 PLAN Phase Complete - Ready for DO Phase (Parallel Agent Execution)  
**Next Step:** Launch Agent 1 (typescript-pro) + Agent 2 (typescript-pro) simultaneously
