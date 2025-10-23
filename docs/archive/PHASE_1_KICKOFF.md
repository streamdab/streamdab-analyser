# Phase 1 Kickoff - Real ETI Core Engine Implementation
## StreamDAB Analyzer - Weeks 1-2

**Start Date:** Ready to begin
**Duration:** 2 weeks
**Status:** Phase 0 Complete ✅ - Ready to Start Phase 1

---

## 🎯 Phase 1 Overview

**Goal:** Implement real ETI frame parsing engine with complete FIG decoding and service discovery.

**Current State:**
- ✅ Phase 0 Complete: All critical fixes implemented
- ✅ Build Status: Zero errors, clean compilation
- ✅ Foundation: Memory-safe, thread-safe architecture
- ✅ Code Review: Approved (95/100)

**Phase 1 Target:**
- Real 6144-byte ETI frame parsing
- Complete FIG Types 0/0, 0/1, 0/2, 1/0, 1/1, 1/4 decoding
- Service discovery from FIC data
- Replace all simulation data with real processing

---

## 📋 Phase 1 Task Breakdown

### Week 1: ETI Frame Parsing Foundation

#### Day 1-2: Frame Structure Implementation
**Agent:** cpp-qt-developer

**Tasks:**
1. Implement ETIFrame structure according to ETSI EN 300 799
2. Create frame header parser
3. Implement SYNC pattern detection (0xFF1F, 0x491F, 0xC4FF)
4. Add frame counter (FCT) validation
5. Extract LIDATA fields (FICF, NST, FP, MID, FL)

**Deliverables:**
```cpp
struct ETIFrameHeader {
    uint32_t sync;           // SYNC pattern
    uint8_t err;             // Error field
    uint8_t frame_counter;   // FCT (0-249)
    uint8_t ficf;            // FIC flag
    uint8_t nst;             // Number of streams
    uint8_t fp;              // Frame phase
    uint8_t mid;             // Mode identity
    uint16_t fl;             // Frame length
};

class RealETIParser {
    bool parseFrame(const QByteArray& frameData);
    ETIFrameHeader extractHeader(const uint8_t* data);
    bool validateSyncPattern(uint32_t sync);
    bool validateFrameCounter(uint8_t fct);
};
```

#### Day 3-4: FIC Extraction and MSC Parsing
**Agent:** cpp-qt-developer

**Tasks:**
1. Extract FIC data (3 FIBs, 96 bytes total)
2. Validate FIC CRC (per FIB)
3. Parse MSC stream data based on NST
4. Extract sub-channel information
5. Build complete frame structure

**Deliverables:**
```cpp
struct SubChannelData {
    uint8_t sub_channel_id;
    uint16_t start_address;
    uint8_t table_switch;
    uint16_t size;
    QByteArray data;
};

class RealETIParser {
    QByteArray extractFIC(const uint8_t* data, bool ficf);
    std::vector<SubChannelData> extractMSC(const uint8_t* data, uint8_t nst);
    bool validateFIBCRC(const QByteArray& fib);
};
```

#### Day 5: Integration and Testing
**Agent:** test-automator

**Tasks:**
1. Create unit tests for frame parsing
2. Test with real ETI files
3. Validate against ETSI EN 300 799 specification
4. Performance benchmarking

---

### Week 2: FIG Processing Engine

#### Day 1-2: Essential FIG Types (0/0, 0/1, 0/2)
**Agent:** cpp-qt-developer

**Tasks:**
1. **FIG 0/0**: Ensemble information
   - Extract EID (Ensemble ID)
   - Parse country ID
   - Extract ensemble reference

2. **FIG 0/1**: Sub-channel organization
   - Map sub-channels to services
   - Extract bitrate and protection level
   - Build service component structure

3. **FIG 0/2**: Service organization
   - Extract Service ID (SID)
   - Map services to components
   - Identify primary/secondary components

**Deliverables:**
```cpp
struct EnsembleInfo {
    uint16_t ensemble_id;
    uint8_t country_id;
    uint8_t ensemble_reference;
    QString ensemble_label;
};

struct ServiceComponent {
    uint8_t tmid;           // Transport mechanism ID
    uint8_t service_type;   // Audio/Data
    uint32_t service_id;
    uint8_t sub_channel_id;
    uint16_t ca_flag;
};

class FIGProcessor {
    void processFIG0_0(const QByteArray& data);
    void processFIG0_1(const QByteArray& data);
    void processFIG0_2(const QByteArray& data);

    EnsembleInfo getEnsembleInfo() const;
    std::vector<ServiceComponent> getComponents() const;
};
```

#### Day 3-4: Label FIG Types (1/0, 1/1, 1/4)
**Agent:** cpp-qt-developer

**Tasks:**
1. **FIG 1/0**: Ensemble label
   - Extract 16-character label
   - Handle character encoding
   - Parse character flag field

2. **FIG 1/1**: Service label
   - Map labels to Service IDs
   - Extract service names
   - Build complete service list

3. **FIG 1/4**: Service component label
   - Component-level labels
   - Build complete naming hierarchy

**Deliverables:**
```cpp
struct DABLabel {
    QString label;          // Up to 16 characters
    uint16_t char_flag;     // Character flag field

    QString getShortLabel() const;
    QString getFullLabel() const;
};

struct DABService {
    uint32_t service_id;
    QString service_label;
    QString short_label;
    uint8_t service_type;
    bool is_dab_plus;
    std::vector<ServiceComponent> components;
};

class FIGProcessor {
    void processFIG1_0(const QByteArray& data);
    void processFIG1_1(const QByteArray& data);
    void processFIG1_4(const QByteArray& data);

    std::vector<DABService> getServices() const;
};
```

#### Day 5: Service Discovery Integration
**Agent:** cpp-qt-developer

**Tasks:**
1. Integrate FIG processing with frame parsing
2. Build complete service discovery pipeline
3. Replace simulation data in GUI
4. Real-time service tree updates

**Deliverables:**
```cpp
class ServiceDiscovery {
    void processFrame(const ETIFrame& frame);
    void updateFromFIC(const QByteArray& fic_data);

    std::vector<DABService> discoverServices();
    EnsembleInfo getEnsembleInfo();

signals:
    void serviceDiscovered(const DABService& service);
    void ensembleInfoUpdated(const EnsembleInfo& info);
};
```

---

## 🔧 Implementation Strategy

### Parallel Agent Deployment

#### Week 1 - Parallel Track
**Agent 1:** ETI frame parsing (Day 1-4)
**Agent 2:** Test infrastructure setup (Day 1-5)

Both can work simultaneously:
- Agent 1 implements parsing
- Agent 2 creates test framework

#### Week 2 - Sequential Track
**Agent 3:** FIG 0/x types (Day 1-2)
**Agent 4:** FIG 1/x types (Day 3-4)
**Agent 5:** Integration (Day 5)

Sequential because:
- FIG 1/x needs FIG 0/x data structures
- Integration needs all FIG types complete

---

## 📊 Success Criteria

### Week 1 Completion
- [ ] Real ETI frames parsed from files
- [ ] Header fields extracted correctly
- [ ] FIC data extracted and validated
- [ ] MSC sub-channels identified
- [ ] Frame counter sequence validated
- [ ] Unit tests pass with real ETI files

### Week 2 Completion
- [ ] All 6 FIG types decoded
- [ ] Services discovered from real FIC
- [ ] Ensemble information extracted
- [ ] Service labels displayed
- [ ] GUI shows real data (no simulation)
- [ ] Performance: >100 FPS processing

### Phase 1 Final
- [ ] Complete service discovery working
- [ ] Real ETI files load and display services
- [ ] No simulation data remains
- [ ] Code review: Grade A or better
- [ ] Documentation updated

---

## 🧪 Testing Strategy

### Test Files Needed
1. **Valid ETI Files:**
   - Bangkok DAB ensemble
   - Multi-service ensemble
   - DAB+ services
   - Data services

2. **Edge Cases:**
   - Single service ensemble
   - Maximum services (64)
   - Frame counter discontinuities
   - CRC errors

3. **Performance Tests:**
   - Large files (>100MB)
   - Sustained processing (1+ hour)
   - Memory usage monitoring

### Test Coverage Goals
- Frame parsing: 100%
- FIG decoding: 100% (6 types)
- Service discovery: 95%
- Error handling: 90%

---

## 📝 Implementation Notes

### Key ETSI References
- **ETSI EN 300 799**: ETI specification (frame structure)
- **ETSI EN 300 401**: DAB system (FIG definitions)
- Sections 5.2 (FIG Type 0), 5.3 (FIG Type 1)

### Frame Structure Reference
```
ETI Frame (6144 bytes):
┌──────────────┬─────────┬─────────┬─────────┬─────┐
│ SYNC (4)     │ LIDATA  │ FIC(96) │ MSC     │ EOF │
│ 0xFF1F/...   │ (4)     │ 3 FIBs  │ (var)   │ (4) │
└──────────────┴─────────┴─────────┴─────────┴─────┘

FIC Structure (96 bytes):
┌─────────┬─────────┬─────────┐
│ FIB 1   │ FIB 2   │ FIB 3   │
│ (32)    │ (32)    │ (32)    │
└─────────┴─────────┴─────────┘

FIB Structure (32 bytes):
┌──────────────┬─────────┐
│ FIG Data     │ CRC     │
│ (30)         │ (2)     │
└──────────────┴─────────┘
```

### FIG Type Summary
- **FIG 0/0**: Ensemble ID, reference
- **FIG 0/1**: Sub-channel → Service mapping
- **FIG 0/2**: Service organization
- **FIG 1/0**: Ensemble label (16 chars)
- **FIG 1/1**: Service labels
- **FIG 1/4**: Component labels

---

## 🚀 Getting Started

### Pre-Implementation Checklist
- [x] Phase 0 complete and approved
- [x] Build system working (zero errors)
- [x] Code review passed
- [ ] Real ETI test files available
- [ ] ETSI specifications downloaded
- [ ] Test framework planned

### First Steps
1. **Read ETSI EN 300 799** Section 5 (Frame structure)
2. **Read ETSI EN 300 401** Sections 5.2-5.3 (FIG types)
3. **Obtain test ETI files** (Bangkok ensemble available in `eti/`)
4. **Set up test infrastructure**
5. **Deploy first agent** for frame parsing

---

## 📅 Timeline

### Week 1 Schedule
- **Mon-Tue**: Frame header parsing
- **Wed-Thu**: FIC/MSC extraction
- **Fri**: Integration and testing

### Week 2 Schedule
- **Mon-Tue**: FIG 0/x implementation
- **Wed-Thu**: FIG 1/x implementation
- **Fri**: Service discovery integration

### Milestones
- **Day 5**: Week 1 review
- **Day 10**: Phase 1 completion review
- **Day 11**: Phase 2 kickoff

---

## 🔗 Resources

### Documentation
- [ETSI Standards Portal](https://www.etsi.org/standards)
- [Phase 1 Detailed Plan](docs/DEVELOPMENT_PLAN_OPTION_B.md)
- [ETI Processing Spec](docs/ETI_PROCESSING_IMPLEMENTATION.md)
- [FIG Requirements](docs/FIG_PROCESSING_REQUIREMENTS.md)

### Test Data
- `eti/bkk_20062022_141637.eti` - Bangkok ensemble
- Additional test files: TBD

### Code References
- `src/core/enhanced_eti_processor_qt.cpp` - Current ETI processor
- `src/core/advanced_fig_analyzer.cpp` - Current FIG analyzer

---

## 📊 Progress Tracking

### Week 1 Progress
- [ ] Day 1: Frame header parser
- [ ] Day 2: SYNC validation
- [ ] Day 3: FIC extraction
- [ ] Day 4: MSC parsing
- [ ] Day 5: Week 1 tests

### Week 2 Progress
- [ ] Day 1: FIG 0/0, 0/1
- [ ] Day 2: FIG 0/2
- [ ] Day 3: FIG 1/0, 1/1
- [ ] Day 4: FIG 1/4
- [ ] Day 5: Integration

---

**Status:** Ready to Deploy Phase 1 Agents
**Next Action:** Deploy cpp-qt-developer for ETI frame parsing
**Target Completion:** 2 weeks from start

🚀 **Let's begin Phase 1!** 🚀
