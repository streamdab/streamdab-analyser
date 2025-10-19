# Phase 2A Week 7: FIG Type 1 & 2 Extended Labels Implementation Plan

**Document Version:** 1.0
**Date:** October 20, 2025
**Phase:** Phase 2A Week 7 - FIG Type 1 Extension + FIG Type 2 (Extended Labels)
**Status:** Planning Complete, Ready for Implementation
**Timeline:** 1.5 weeks

---

## Executive Summary

Phase 2A Week 7 implements extended labeling functionality through FIG Type 1/5 and complete FIG Type 2 support, enabling UTF-8 long labels for ensemble, services, and components. This completes the FIG processing foundation with comprehensive internationalization support for Thai language broadcasts.

**Key Achievement:** UTF-8 extended labels support Thai characters (ภาษาไทย) in service names and descriptions.

---

## Implementation Scope

### **FIG Types to Implement:**

| FIG Type | Description | ETSI Reference | Priority | Thai Support |
|----------|-------------|----------------|----------|--------------|
| **FIG 1/5** | Data service label (16-char) | EN 300 401 §5.2.2.6 | HIGH | Yes |
| **FIG 2/0** | Ensemble extended label | EN 300 401 §8.1.13.1 | HIGH | Yes (UTF-8) |
| **FIG 2/1** | Service extended label | EN 300 401 §8.1.13.1 | HIGH | Yes (UTF-8) |
| **FIG 2/4** | Service component extended label | EN 300 401 §8.1.13.1 | MEDIUM | Yes (UTF-8) |
| **FIG 2/5** | Data service extended label | EN 300 401 §8.1.13.1 | MEDIUM | Yes (UTF-8) |

---

## Architecture Decision: Integration into AdvancedFIGAnalyzer

**Decision:** Implement FIG Type 2 parsers **WITHIN** `AdvancedFIGAnalyzer` class rather than creating separate `FIG2Processor`.

**Rationale:**
- FIG Type 2 extends FIG Type 1 labels (same entities: ensemble, services, components)
- Shared data storage (service maps, ensemble info)
- Consistent pattern with Week 5/6 implementations
- Simpler integration and testing
- Follows "processors first" strategy without premature abstraction

**Architecture:**
```
AdvancedFIGAnalyzer (existing class)
├── FIG Type 0 parsers (0/0-0/10, 0/13-0/14, 0/17-0/19) ✅ Complete
├── FIG Type 1 parsers (1/0, 1/1, 1/4) ✅ Complete
├── FIG Type 1 extension (1/5) ← NEW
└── FIG Type 2 parsers (2/0, 2/1, 2/4, 2/5) ← NEW
```

---

## Detailed Implementation Specifications

### **FIG 1/5: Data Service Label**

**ETSI Reference:** EN 300 401 Section 5.2.2.6
**Purpose:** Provides 16-character labels for data services (32-bit Service IDs)

**Structure Definition:**
```cpp
struct FIG1_5_DataServiceLabel {
    uint32_t service_id;           // 32-bit data service ID
    char label[17];                // 16 characters + null terminator
    uint16_t character_flag;       // Character field definition (16 bits)
};
```

**Parsing Logic:**
```cpp
void AdvancedFIGAnalyzer::processFIG1_5(const QByteArray& data) {
    // Structure: 32-bit SId + 16-char label + 16-bit character flag
    // Total: 22 bytes per label

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip FIG header

    while (offset + 22 <= data.size()) {
        FIG1_5_DataServiceLabel label_info;

        // Parse 32-bit service ID
        label_info.service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                                (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                                (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                                bytes[offset + 3];
        offset += 4;

        // Extract 16-character label
        memcpy(label_info.label, &bytes[offset], 16);
        label_info.label[16] = '\0';
        offset += 16;

        // Character flag field
        label_info.character_flag = (static_cast<uint16_t>(bytes[offset]) << 8) |
                                    bytes[offset + 1];
        offset += 2;

        // Store in service map
        m_data_service_labels[label_info.service_id] = label_info;

        qInfo() << "FIG 1/5: Data service" << Qt::hex << label_info.service_id
                << "label:" << label_info.label;
    }
}
```

**Integration:** Update existing service discovery to include data service labels.

---

### **FIG 2/0: Ensemble Extended Label**

**ETSI Reference:** EN 300 401 Section 8.1.13.1
**Purpose:** Provides UTF-8 extended labels (up to 128 bytes) for ensemble

**Structure Definition:**
```cpp
struct FIG2_0_ExtendedLabel {
    uint16_t ensemble_id;          // EId
    uint8_t toggle_flag;           // Toggle bit (segment version)
    uint8_t segment_index;         // Segment index (0-7)
    std::vector<uint8_t> text_segment;  // UTF-8 encoded text
};
```

**Segmentation Strategy:**
Extended labels can be up to 128 bytes, transmitted in 8 segments of 16 bytes each:
- Segment 0: Bytes 0-15
- Segment 1: Bytes 16-31
- ...
- Segment 7: Bytes 112-127

**Parsing Logic:**
```cpp
void AdvancedFIGAnalyzer::processFIG2_0(const QByteArray& data) {
    // Variable length structure:
    // - EId: 16 bits
    // - Rfa: 4 bits
    // - Toggle: 1 bit
    // - SegmentIndex: 3 bits
    // - Text: variable length (remaining bytes)

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;  // Skip FIG header

    while (offset + 3 <= data.size()) {  // Minimum: EId(2) + flags(1)
        FIG2_0_ExtendedLabel ext_label;

        // Parse Ensemble ID (16 bits)
        ext_label.ensemble_id = (static_cast<uint16_t>(bytes[offset]) << 8) |
                                bytes[offset + 1];
        offset += 2;

        // Parse flags: Rfa(4) + Toggle(1) + SegmentIndex(3)
        ext_label.toggle_flag = (bytes[offset] & 0x08) >> 3;
        ext_label.segment_index = bytes[offset] & 0x07;
        offset += 1;

        // Remaining bytes are UTF-8 text segment
        int text_length = data.size() - offset;
        ext_label.text_segment.clear();
        for (int i = 0; i < text_length; ++i) {
            ext_label.text_segment.push_back(bytes[offset + i]);
        }
        offset += text_length;

        // Store segment (reassembly handled by caller)
        m_ensemble_extended_labels[ext_label.ensemble_id].segments[ext_label.segment_index] = ext_label;

        qInfo() << "FIG 2/0: Ensemble" << Qt::hex << ext_label.ensemble_id
                << "segment" << ext_label.segment_index
                << "length" << text_length;
    }
}
```

**Reassembly Strategy:**
```cpp
QString AdvancedFIGAnalyzer::getEnsembleExtendedLabel(uint16_t ensemble_id) {
    auto& segments = m_ensemble_extended_labels[ensemble_id].segments;

    // Concatenate all segments in order
    QByteArray full_label;
    for (int i = 0; i < 8; ++i) {
        if (segments.contains(i)) {
            for (uint8_t byte : segments[i].text_segment) {
                full_label.append(byte);
            }
        }
    }

    // Convert UTF-8 to QString
    return QString::fromUtf8(full_label);
}
```

---

### **FIG 2/1: Service Extended Label**

**ETSI Reference:** EN 300 401 Section 8.1.13.1
**Purpose:** UTF-8 extended labels for services (programme or data)

**Structure Definition:**
```cpp
struct FIG2_1_ExtendedLabel {
    uint32_t service_id;           // 16-bit or 32-bit (depends on P/D flag)
    uint8_t toggle_flag;
    uint8_t segment_index;
    std::vector<uint8_t> text_segment;
};
```

**Parsing Logic:**
```cpp
void AdvancedFIGAnalyzer::processFIG2_1(const QByteArray& data) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    // Extract P/D flag from FIG header (byte 0, bit 2)
    bool pd_flag = (bytes[0] & 0x04) != 0;
    int offset = 1;

    int sid_length = pd_flag ? 4 : 2;  // 32-bit or 16-bit SId

    while (offset + sid_length + 1 <= data.size()) {
        FIG2_1_ExtendedLabel ext_label;

        // Parse Service ID
        if (pd_flag) {
            // 32-bit data service ID
            ext_label.service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                                   (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                                   (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                                   bytes[offset + 3];
        } else {
            // 16-bit programme service ID
            ext_label.service_id = (static_cast<uint32_t>(bytes[offset]) << 8) |
                                   bytes[offset + 1];
        }
        offset += sid_length;

        // Parse flags
        ext_label.toggle_flag = (bytes[offset] & 0x08) >> 3;
        ext_label.segment_index = bytes[offset] & 0x07;
        offset += 1;

        // UTF-8 text segment
        int text_length = data.size() - offset;
        ext_label.text_segment.assign(bytes + offset, bytes + offset + text_length);
        offset += text_length;

        // Store segment
        m_service_extended_labels[ext_label.service_id].segments[ext_label.segment_index] = ext_label;

        qInfo() << "FIG 2/1: Service" << Qt::hex << ext_label.service_id
                << "segment" << ext_label.segment_index;
    }
}
```

---

### **FIG 2/4: Service Component Extended Label**

**ETSI Reference:** EN 300 401 Section 8.1.13.1
**Purpose:** UTF-8 extended labels for service components

**Structure Definition:**
```cpp
struct FIG2_4_ExtendedLabel {
    uint32_t service_id;           // Service containing the component
    uint8_t component_id;          // SCIdS (4 bits)
    uint8_t toggle_flag;
    uint8_t segment_index;
    std::vector<uint8_t> text_segment;
};
```

**Parsing Logic:**
```cpp
void AdvancedFIGAnalyzer::processFIG2_4(const QByteArray& data) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());

    // Extract P/D flag
    bool pd_flag = (bytes[0] & 0x04) != 0;
    int offset = 1;

    int sid_length = pd_flag ? 4 : 2;

    while (offset + sid_length + 1 <= data.size()) {
        FIG2_4_ExtendedLabel ext_label;

        // Parse Service ID (same as FIG 2/1)
        if (pd_flag) {
            ext_label.service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                                   (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                                   (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                                   bytes[offset + 3];
        } else {
            ext_label.service_id = (static_cast<uint32_t>(bytes[offset]) << 8) |
                                   bytes[offset + 1];
        }
        offset += sid_length;

        // Parse SCIdS (4 bits, bits 7-4) + flags
        ext_label.component_id = (bytes[offset] >> 4) & 0x0F;
        ext_label.toggle_flag = (bytes[offset] & 0x08) >> 3;
        ext_label.segment_index = bytes[offset] & 0x07;
        offset += 1;

        // UTF-8 text segment
        int text_length = data.size() - offset;
        ext_label.text_segment.assign(bytes + offset, bytes + offset + text_length);
        offset += text_length;

        // Store segment (keyed by service_id + component_id)
        uint64_t key = (static_cast<uint64_t>(ext_label.service_id) << 8) | ext_label.component_id;
        m_component_extended_labels[key].segments[ext_label.segment_index] = ext_label;

        qInfo() << "FIG 2/4: Service" << Qt::hex << ext_label.service_id
                << "component" << ext_label.component_id
                << "segment" << ext_label.segment_index;
    }
}
```

---

### **FIG 2/5: Data Service Extended Label**

**ETSI Reference:** EN 300 401 Section 8.1.13.1
**Purpose:** UTF-8 extended labels for data services (similar to FIG 2/1 but always 32-bit SId)

**Note:** FIG 2/5 is semantically identical to FIG 2/1 with P/D=1. Can be implemented as:

```cpp
void AdvancedFIGAnalyzer::processFIG2_5(const QByteArray& data) {
    // FIG 2/5 is always for data services (32-bit SId)
    // Reuse FIG 2/1 logic but force pd_flag = true

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.constData());
    int offset = 1;

    while (offset + 5 <= data.size()) {  // Minimum: 32-bit SId(4) + flags(1)
        FIG2_1_ExtendedLabel ext_label;  // Reuse FIG 2/1 structure

        // Always parse as 32-bit service ID (data service)
        ext_label.service_id = (static_cast<uint32_t>(bytes[offset]) << 24) |
                               (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
                               (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
                               bytes[offset + 3];
        offset += 4;

        // Parse flags (same as FIG 2/1)
        ext_label.toggle_flag = (bytes[offset] & 0x08) >> 3;
        ext_label.segment_index = bytes[offset] & 0x07;
        offset += 1;

        // UTF-8 text segment
        int text_length = data.size() - offset;
        ext_label.text_segment.assign(bytes + offset, bytes + offset + text_length);
        offset += text_length;

        // Store in same container as FIG 2/1
        m_service_extended_labels[ext_label.service_id].segments[ext_label.segment_index] = ext_label;

        qInfo() << "FIG 2/5: Data service" << Qt::hex << ext_label.service_id
                << "segment" << ext_label.segment_index;
    }
}
```

---

## Implementation Tasks

### **Task 1: FIG 1/5 Implementation** (2 hours)
- [ ] Add FIG1_5_DataServiceLabel structure to advanced_fig_analyzer.h
- [ ] Implement processFIG1_5() parser method
- [ ] Add data_service_labels storage container
- [ ] Integrate with existing service discovery
- [ ] Update processFIG1() dispatcher

### **Task 2: FIG Type 2 Structures** (1 hour)
- [ ] Add FIG2_0_ExtendedLabel structure
- [ ] Add FIG2_1_ExtendedLabel structure
- [ ] Add FIG2_4_ExtendedLabel structure
- [ ] Add segment reassembly containers (maps with segment indices)
- [ ] Add UTF-8 conversion helper methods

### **Task 3: FIG Type 2 Parsers** (4 hours)
- [ ] Implement processFIG2_0() - Ensemble extended labels
- [ ] Implement processFIG2_1() - Service extended labels
- [ ] Implement processFIG2_4() - Component extended labels
- [ ] Implement processFIG2_5() - Data service extended labels
- [ ] Add processFIG2() dispatcher method

### **Task 4: Segment Reassembly** (2 hours)
- [ ] Implement getEnsembleExtendedLabel() reassembly
- [ ] Implement getServiceExtendedLabel() reassembly
- [ ] Implement getComponentExtendedLabel() reassembly
- [ ] Add toggle flag tracking (detect label updates)
- [ ] UTF-8 validation and sanitization

### **Task 5: Integration** (1 hour)
- [ ] Add FIG Type 2 to initializeDefaultFIGTypes()
- [ ] Update analyzeFICData() to call processFIG2()
- [ ] Update reset() method for new containers
- [ ] Add Qt signals for extended label updates

### **Task 6: Testing** (2 hours)
- [ ] Create unit tests for FIG 1/5 (minimum 3 tests)
- [ ] Create unit tests for FIG 2/0-2/5 (minimum 12 tests)
- [ ] Test Thai UTF-8 characters (ภาษาไทย)
- [ ] Test segment reassembly with partial data
- [ ] Validate against ODR-DabMux generated streams

---

## Success Criteria

### **Functional Requirements:**
- ✅ FIG 1/5 correctly parses data service labels
- ✅ FIG 2/0-2/5 parsers implemented and tested
- ✅ Segment reassembly produces complete UTF-8 labels
- ✅ Thai characters (UTF-8) display correctly
- ✅ Toggle flag tracking detects label updates
- ✅ Build compiles with zero errors

### **ETSI Compliance:**
- ✅ EN 300 401 Section 5.2.2.6 (FIG 1/5) - 100%
- ✅ EN 300 401 Section 8.1.13.1 (FIG 2/0-2/5) - 100%

### **Internationalization:**
- ✅ UTF-8 encoding/decoding functional
- ✅ Thai language support validated
- ✅ Segment reassembly handles multi-byte characters correctly

---

## Thai Language Support

**Example Extended Label:**
```
Short label (FIG 1/1): "Radio 1"
Extended label (FIG 2/1): "สถานีวิทยุแห่งประเทศไทย (Radio Thailand National Broadcasting)"
```

**UTF-8 Encoding:**
- Thai characters use 3 bytes each in UTF-8
- "ภาษาไทย" (Thai language) = 27 bytes
- Segment reassembly must preserve byte boundaries

**Implementation Notes:**
- Use `QString::fromUtf8()` for conversion
- Validate multi-byte sequences
- Handle partial segments gracefully

---

## ODR-DabMux Reference Files

**Base Path:** `/home/seksan/workspace/ODR-DabMux/src/`

- `fig/FIG1.cpp` - FIG Type 1 implementations (includes FIG 1/5)
- `fig/FIG2.h` - FIG Type 2 structure definitions
- `DabMux.cpp` - Shows FIG scheduling and segmentation

**Reference Strategy:**
- Study FIG 2 segmentation logic (8 segments × 16 bytes)
- Understand toggle flag usage for label updates
- Validate UTF-8 encoding approach

---

## Integration with Existing Code

**FIG Type Dispatcher Update:**
```cpp
void AdvancedFIGAnalyzer::analyzeFICData(const QByteArray& ficData) {
    // ... existing code ...

    uint8_t fig_type = (fig_header >> 5) & 0x07;

    switch (fig_type) {
        case 0:
            processFIG0(figData);
            break;
        case 1:
            processFIG1(figData);  // Already handles 1/0, 1/1, 1/4
            break;
        case 2:
            processFIG2(figData);  // NEW: Add FIG Type 2 dispatcher
            break;
        default:
            qDebug() << "Unsupported FIG type:" << fig_type;
            break;
    }
}
```

**FIG Type 2 Dispatcher:**
```cpp
void AdvancedFIGAnalyzer::processFIG2(const QByteArray& figData) {
    if (figData.size() < 1) {
        m_lastError = "FIG 2: Insufficient data";
        return;
    }

    uint8_t extension = (figData[0] >> 3) & 0x1F;

    QString figKey = QString("2.%1").arg(extension);
    if (!m_enabledFIGTypes.value(figKey, false)) {
        m_lastError = QString("FIG 2/%1: Disabled").arg(extension);
        return;
    }

    switch (extension) {
        case 0:
            processFIG2_0(figData);
            break;
        case 1:
            processFIG2_1(figData);
            break;
        case 4:
            processFIG2_4(figData);
            break;
        case 5:
            processFIG2_5(figData);
            break;
        default:
            qDebug() << "Unsupported FIG 2 extension:" << extension;
            break;
    }
}
```

---

## Timeline

**Total Effort:** 1.5 weeks (12 hours development)

- **Day 1-2**: FIG 1/5 implementation (2 hours)
- **Day 3-4**: FIG Type 2 structures and parsers (5 hours)
- **Day 5**: Segment reassembly logic (2 hours)
- **Day 6**: Integration and testing (3 hours)

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| **UTF-8 multi-byte character handling** | MEDIUM | Use Qt QString::fromUtf8(), validate encoding |
| **Segment reassembly complexity** | MEDIUM | Follow ODR-DabMux segmentation strategy |
| **Toggle flag interpretation** | LOW | Reference ETSI specification carefully |
| **Build integration issues** | LOW | Incremental commits, continuous testing |

---

## References

**ETSI Standards:**
- EN 300 401 V2.1.1 (2017-01) Section 5.2.2.6 (FIG 1/5)
- EN 300 401 V2.1.1 (2017-01) Section 8.1.13.1 (FIG Type 2)

**ODR-DabMux Source:**
- `/home/seksan/workspace/ODR-DabMux/src/fig/FIG1.cpp`
- `/home/seksan/workspace/ODR-DabMux/src/fig/FIG2.h`
- `/home/seksan/workspace/ODR-DabMux/src/DabMux.cpp`

**Related Documents:**
- `RELEASE_1A_CORE_PROCESSORS_PLAN.md` - Overall project plan
- `PHASE_2A_WEEK6_FIG_ADVANCED.md` - Previous week implementation

---

**Document Status:** ✅ Ready for Implementation
**Next Step:** Launch backend-developer agent for implementation
