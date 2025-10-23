# Phase 1 Quick Start Guide
## Real ETI Core Engine Implementation - Ready to Begin

**Target:** Weeks 1-2 of Option B Implementation
**Goal:** Real ETI frame parsing with complete FIG decoding
**Foundation:** Phase 0 complete with A- code quality

---

## 🚀 Quick Start Commands

### Verify Current State
```bash
cd /home/seksan/workspace/streamdab-analyzer

# Check git status
git status  # Should show Phase 0 changes ready to commit

# Verify clean build
cd build && make
./DABAnalyzer_Enhanced_FilteringBuild  # Should launch Qt6 GUI

# Check code files
wc -l src/main.cpp  # Should show ~2450 lines
ls -la src/core/enhanced_eti_processor_qt.*  # Target files for Phase 1
```

### Start Phase 1 Implementation
```bash
# Deploy PM agent for Phase 1
# User command: "pm-agent: start Phase 1 Week 1 - ETI frame parsing"

# PM agent will deploy cpp-qt-developer following mandatory workflow:
# 1. Coding (cpp-qt-developer)
# 2. Unit Tests (test-automator or cpp-qt-developer)
# 3. Build Verification
# 4. E2E Tests
# 5. Code Review (code-reviewer) - MANDATORY
# 6. Fix if needed (loop back to step 1)
# 7. Commit changes
```

---

## 📋 Phase 1 Week 1 Tasks

### Day 1-2: ETI Frame Structure Implementation
**Agent:** cpp-qt-developer
**Files:** `src/core/enhanced_eti_processor_qt.h/cpp`

**Implementation Tasks:**
1. Create `ETIFrameParser` class
2. Implement 6144-byte frame structure parsing
3. Add SYNC pattern recognition (0xFF1F, 0x491F, 0xC4FF)
4. Extract LIDATA fields (FICF, NST, FP, MID, FL)
5. Implement frame counter (FCT) validation

**Code Structure:**
```cpp
// In enhanced_eti_processor_qt.h
class ETIFrameParser {
public:
    bool parseFrame(const std::array<uint8_t, 6144>& frame_data);
    ETIHeader extractHeader(const uint8_t* data);
    bool validateSyncPattern(uint32_t sync);
    bool validateFrameCounter(uint8_t fct);

private:
    uint8_t m_last_frame_counter{0};
};

struct ETIHeader {
    uint32_t sync;           // SYNC pattern
    uint8_t err;             // Error field
    uint8_t frame_counter;   // FCT (0-249)
    uint8_t ficf;            // FIC flag
    uint8_t nst;             // Number of streams
    uint8_t fp;              // Frame phase
    uint8_t mid;             // Mode identity
    uint16_t fl;             // Frame length
};
```

**Testing:**
```bash
# Unit tests for frame parsing
cd build
./test_eti_frame_parser

# Test with real ETI file
./DABAnalyzer_Enhanced_FilteringBuild eti/bkk_20062022_141637.eti
```

### Day 3-4: FIC Extraction and MSC Parsing
**Agent:** cpp-qt-developer
**Files:** `src/core/enhanced_eti_processor_qt.h/cpp`

**Implementation Tasks:**
1. Extract FIC data (3 FIBs, 96 bytes total)
2. Validate FIC CRC (per FIB) - already implemented in Phase 0
3. Parse MSC stream data based on NST
4. Extract sub-channel information
5. Build complete frame structure

**Code Structure:**
```cpp
struct SubChannelData {
    uint8_t sub_channel_id;
    uint16_t start_address;
    uint8_t table_switch;
    uint16_t size;
    QByteArray data;
};

class ETIFrameParser {
    QByteArray extractFIC(const uint8_t* data, bool ficf);
    std::vector<SubChannelData> extractMSC(const uint8_t* data, uint8_t nst);
    bool validateFIBCRC(const QByteArray& fib);  // Already exists from Phase 0
};
```

### Day 5: Week 1 Integration and Testing
**Agent:** test-automator
**Files:** `tests/test_eti_parsing.cpp`

**Testing Tasks:**
1. Create comprehensive unit tests
2. Test with multiple real ETI files
3. Validate against ETSI EN 300 799 specification
4. Performance benchmarking
5. Code review preparation

---

## 📋 Phase 1 Week 2 Tasks

### Day 1-2: Essential FIG Types (0/0, 0/1, 0/2)
**Agent:** cpp-qt-developer
**Files:** `src/core/advanced_fig_analyzer.cpp`

**FIG Implementation:**
```cpp
// FIG 0/0: Ensemble information
void processFIG0_0(const QByteArray& data) {
    // Extract EID (Ensemble ID)
    // Parse country ID
    // Extract ensemble reference
}

// FIG 0/1: Sub-channel organization
void processFIG0_1(const QByteArray& data) {
    // Map sub-channels to services
    // Extract bitrate and protection level
    // Build service component structure
}

// FIG 0/2: Service organization
void processFIG0_2(const QByteArray& data) {
    // Extract Service ID (SID)
    // Map services to components
    // Identify primary/secondary components
}
```

### Day 3-4: Label FIG Types (1/0, 1/1, 1/4)
**Agent:** cpp-qt-developer
**Files:** `src/core/advanced_fig_analyzer.cpp`

**Label Processing:**
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
```

### Day 5: Service Discovery Integration
**Agent:** cpp-qt-developer
**Files:** `src/core/enhanced_eti_processor_qt.cpp`, `src/main.cpp`

**Integration Tasks:**
1. Connect FIG processing to frame parsing
2. Build complete service discovery pipeline
3. Replace simulation data in GUI
4. Real-time service tree updates

---

## 📊 Success Criteria for Phase 1

### Week 1 Completion Checklist
- [ ] Real ETI frames parsed from files
- [ ] Header fields extracted correctly
- [ ] FIC data extracted and validated
- [ ] MSC sub-channels identified
- [ ] Frame counter sequence validated
- [ ] Unit tests pass with real ETI files
- [ ] Code review passed (Grade A or better)

### Week 2 Completion Checklist
- [ ] All 6 FIG types decoded
- [ ] Services discovered from real FIC
- [ ] Ensemble information extracted
- [ ] Service labels displayed
- [ ] GUI shows real data (no simulation)
- [ ] Performance: >100 FPS processing
- [ ] Code review passed (Grade A or better)

### Phase 1 Final Verification
- [ ] Complete service discovery working
- [ ] Real ETI files load and display services
- [ ] No simulation data remains
- [ ] Code quality: Grade A or better
- [ ] Documentation updated
- [ ] Ready for Phase 2 (GUI Integration)

---

## 🔧 Development Workflow (MANDATORY)

**Every feature/fix MUST follow this workflow:**

```
Step 1: CODING (cpp-qt-developer)
   └─> Implement ETI frame parsing / FIG decoding

Step 2: UNIT TESTS (test-automator OR cpp-qt-developer)
   └─> Create tests, run with: cd build && ctest

Step 3: BUILD VERIFICATION
   └─> Clean build: cd build && cmake .. && make

Step 4: E2E TESTS (test-automator)
   └─> Test with real ETI files from eti/ directory

Step 5: CODE REVIEW (code-reviewer) - MANDATORY
   └─> Comprehensive review of all changes

Step 6a: IF REVIEW FAILS → Back to Step 1 with fixes
Step 6b: IF REVIEW PASSES → Proceed to Step 7

Step 7: COMMIT CHANGES
   └─> Git commit with descriptive message
```

**No shortcuts allowed!**

---

## 📁 Key Files for Phase 1

### Implementation Files
```
src/core/enhanced_eti_processor_qt.h      # ETI frame parsing class
src/core/enhanced_eti_processor_qt.cpp    # ETI implementation
src/core/advanced_fig_analyzer.h          # FIG processing class
src/core/advanced_fig_analyzer.cpp        # FIG implementation
src/main.cpp                              # GUI integration (2450 lines)
```

### Test Files
```
tests/test_eti_parsing.cpp                # ETI parser unit tests
tests/test_fig_processing.cpp             # FIG decoder unit tests
tests/test_service_discovery.cpp          # Integration tests
```

### Test Data
```
eti/bkk_20062022_141637.eti              # Bangkok DAB ensemble
eti/                                      # Additional test files
```

### Documentation
```
PHASE_1_KICKOFF.md                        # Detailed Phase 1 plan
docs/UPDATED_IMPLEMENTATION_PLAN_2025.md  # Complete implementation plan
ETSI EN 300 799                           # ETI specification
ETSI EN 300 401                           # DAB system specification
```

---

## 🎯 PM Agent Deployment Commands

### Start Phase 1 Week 1
```
User: "pm-agent: start Phase 1 Week 1 - ETI frame parsing"

PM agent will:
1. Deploy cpp-qt-developer for frame parsing implementation
2. Deploy test-automator for test creation (parallel)
3. Monitor progress with todo lists
4. Run build verification after implementation
5. Run unit tests and E2E tests
6. Deploy code-reviewer (MANDATORY)
7. Fix any issues found
8. Commit changes only after review PASSES
```

### Continue to Week 2
```
User: "pm-agent: start Phase 1 Week 2 - FIG processing"

PM agent will:
1. Deploy cpp-qt-developer for FIG 0/x types (Day 1-2)
2. Deploy cpp-qt-developer for FIG 1/x types (Day 3-4)
3. Deploy cpp-qt-developer for integration (Day 5)
4. Follow mandatory workflow for each task
5. Run code-reviewer before completion
6. Generate Phase 1 completion report
```

---

## 📊 Performance Targets

### Week 1 Target
- **Frame Parsing:** >100 FPS
- **Memory Usage:** <50 MB for typical file
- **Test Coverage:** >80% on new code

### Week 2 Target
- **FIG Processing:** >100 FPS
- **Service Discovery:** <1 second for typical ensemble
- **Test Coverage:** >80% on new code

### Phase 1 Final Target
- **Combined Processing:** >100 FPS
- **Memory Efficiency:** Optimized allocation
- **Test Coverage:** >80% overall

---

## 🔍 Troubleshooting

### Build Issues
```bash
# Clean rebuild
cd build
rm -rf *
cmake ..
make -j$(nproc)
```

### Test Failures
```bash
# Run tests with verbose output
cd build
ctest --verbose --output-on-failure
```

### ETI File Issues
```bash
# Verify ETI file format
hexdump -C eti/bkk_20062022_141637.eti | head -20

# Check file size (should be multiple of 6144)
ls -lh eti/bkk_20062022_141637.eti
echo "$(stat -c%s eti/bkk_20062022_141637.eti) % 6144" | bc
```

---

## ✅ Ready to Start Phase 1

**Current Status:**
- ✅ Phase 0 complete with A- code quality
- ✅ Zero compilation errors
- ✅ Production-ready foundation
- ✅ PM agent v2.0 with mandatory workflow
- ✅ Comprehensive documentation
- ✅ Real ETI test files available

**Next Command:**
```
pm-agent: start Phase 1 Week 1 - ETI frame parsing
```

**Expected Timeline:**
- Week 1: 5 days (frame parsing + FIC extraction)
- Week 2: 5 days (FIG processing + integration)
- Total: 10 working days for complete Phase 1

**Expected Outcome:**
- Real ETI processing working
- Service discovery functional
- GUI showing actual DAB services
- Performance >100 FPS validated
- Code quality Grade A
- Ready for Phase 2 (GUI Integration)

---

🚀 **Phase 1 is ready to begin! Deploy pm-agent when ready to start Week 1 implementation.** 🚀
