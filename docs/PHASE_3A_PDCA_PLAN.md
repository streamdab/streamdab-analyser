# Phase 3A Implementation Plan - PDCA Methodology
## Audio & Data Services - Complete Breakdown

**Document Version:** 1.0  
**Date:** October 22, 2025  
**Methodology:** PDCA (Plan-Do-Check-Act)  
**Phase Duration:** 2-3 weeks  
**Status:** Ready for execution

---

## 📋 **PHASE 3A OVERVIEW**

**Goal:** Implement complete audio and data services processing for DAB+ broadcasts

**Scope:**
- ✅ DAB+ audio stream validation (Week 8 - COMPLETE)
- 🔄 MOT Protocol parser (Week 9 - 95% COMPLETE)
- ⏳ EPG & Journaline decoders (Week 10 - PLANNED)
- ⏳ Integration & testing (Week 11 - PLANNED)

**Success Criteria:**
- Parse and validate DAB+ audio superframes
- Extract MOT SlideShow images from data services
- Decode EPG (Electronic Program Guide) information
- Decode Journaline text services
- All components integrated with Qt6 GUI
- Zero critical bugs, <5 medium bugs

---

## 🎯 **SUBPHASE BREAKDOWN**

### **Subphase 1: Audio Stream Processing** ✅ COMPLETE
**Duration:** 2 weeks (Weeks 8-9)  
**Status:** COMPLETE

#### Wave 1.1: DAB+ Stream Validator ✅ COMPLETE
**Duration:** 3 days  
**Status:** COMPLETE - Grade A+ (99/100)

**Implementation Summary:**
- FireCode CRC validation (polynomial 0x782F)
- Reed-Solomon FEC validation (RS(120,110) and RS(120,96))
- HE-AAC v2 profile detection (SBR + PS flags)
- Audio quality metrics extraction (bitrate 8-192 kbps)
- Qt6 signal integration (3 signals)
- Thread-safe design with QMutex

**Files Delivered:**
- `src/core/dabplus_stream_validator.hpp` (496 lines)
- `src/core/dabplus_stream_validator.cpp` (643 lines)

**Code Review:** A+ - Production ready

---

#### Wave 1.2: MOT Protocol Parser 🔄 95% COMPLETE
**Duration:** 4 days  
**Status:** Implementation complete, testing pending

**Tasks Completed:**
- ✅ MOT header/directory parsing (ETSI EN 301 234)
- ✅ MSC Data Group parsing
- ✅ Segment reassembly with duplicate detection
- ✅ Content type detection (JPEG, PNG, HTML)
- ✅ Header parameter extraction (name, trigger time, category, URL)
- ✅ Transport ID management (0-1023)
- ✅ CRC validation for data groups
- ✅ UTF-8 support for Thai language content

**Files Delivered:**
- `src/core/mot_protocol.hpp` (20 KB)
- `src/core/mot_protocol.cpp` (27 KB)

**Remaining Work:**
- ⏳ **Unit testing (cpp-qt-developer - MANDATORY)**
  - Test MSC Data Group parsing
  - Test segment reassembly logic
  - Test content type detection
  - Test CRC validation
  - Test Thai UTF-8 text handling
  - Edge case testing (empty data, corrupted segments)
  
- ⏳ **Integration testing with real ETI files**
  - Test MOT → FIG analyzer integration
  - Test complete MOT object reassembly
  - Test SlideShow image extraction
  - Verify Qt signal/slot chains
  
- ⏳ **Code review (code-reviewer)**
  - ETSI EN 301 234 compliance verification
  - Memory safety validation (Valgrind)
  - Thread safety check
  - Code quality assessment

**Next Action:** Deploy cpp-qt-developer for testing implementation, then code-reviewer for approval

---

### **Subphase 2: Data Services** ⏳ PLANNED
**Duration:** 1.5 weeks (Week 10-11)  
**Status:** Ready to plan

#### Wave 2.1: EPG (Electronic Program Guide) Decoder
**Duration:** 4 days  
**Priority:** High  
**Dependencies:** MOT Protocol Parser (Wave 1.2)

**PLAN Phase (PM Agent):**

**Objective:** Implement EPG data extraction per ETSI TS 102 371

**DO Phase Tasks for cpp-qt-developer:**

**IMPORTANT:** All implementation must include unit tests and integration tests per PDCA DO phase requirements.

1. **Day 1: EPG Data Structure Implementation + Unit Tests**
   - Create `src/core/epg_decoder.hpp` header
   - Define EPG data structures:
     ```cpp
     struct EPGEvent {
         uint32_t event_id;
         QDateTime start_time;
         QDateTime end_time;
         QString program_name;
         QString description;
         QString genre;
         uint8_t content_type;
     };
     
     struct EPGSchedule {
         uint32_t service_id;
         QVector<EPGEvent> events;
         QDateTime schedule_date;
     };
     ```
   - Implement `EPGDecoder` class with Qt6 integration

2. **Day 2: EPG Parsing Logic**
   - Implement schedule information parsing
   - Extract event timing (start/end times from MJD/UTC)
   - Parse program metadata (name, description)
   - Handle Thai UTF-8 text in EPG data

3. **Day 3: Qt Integration & Signals**
   - Implement Qt signals for EPG updates:
     ```cpp
     signals:
         void epgEventDiscovered(uint32_t service_id, const EPGEvent& event);
         void epgScheduleUpdated(uint32_t service_id);
         void epgDecodingError(const QString& error);
     ```
   - Add Q_PROPERTY for QML binding
   - Thread-safe design with QMutex

4. **Day 4: Comprehensive Testing & Documentation**
   
   **Unit Testing (MANDATORY):**
   ```cpp
   // tests/test_epg_decoder.cpp
   class TestEPGDecoder : public QObject {
       Q_OBJECT
   private slots:
       void testEPGEventParsing();
       void testMJDtoQDateTimeConversion();
       void testThaiTextInProgramName();
       void testScheduleParsing();
       void testInvalidEventHandling();
       void testEmptyEPGData();
   };
   ```
   
   **Integration Testing:**
   ```cpp
   // tests/integration/test_epg_pipeline.cpp
   void TestEPGPipeline::testMOTtoEPGExtraction() {
       // Load MOT object with EPG data
       // Verify EPG decoder extracts events correctly
       // Check Qt signals emitted
   }
   ```
   
   **Test Data Preparation:**
   - Create synthetic EPG test data (testdata/epg_sample.bin)
   - Use ETSI test vectors if available
   - Generate Thai language EPG samples
   
   **Documentation:**
   - Add comprehensive Doxygen comments
   - Create usage examples
   - Document ETSI TS 102 371 compliance
   
   **Memory Safety Validation:**
   ```bash
   # Run before marking wave complete
   valgrind --leak-check=full ./test_epg_decoder
   # Must show: 0 bytes leaked
   ```

**Acceptance Criteria:**
- [ ] Parse EPG schedule information per ETSI TS 102 371
- [ ] Extract event timing (MJD/UTC to QDateTime conversion)
- [ ] Parse program metadata (Thai UTF-8 support)
- [ ] Qt signals emit for EPG events
- [ ] Zero compilation errors
- [ ] Process 100+ EPG events without crash
- [ ] Thread-safe concurrent access

**Success Metrics:**
- Code compiles cleanly
- Basic validation tests pass
- EPG events extracted correctly from test data
- Thai text renders properly
- Ready for code review

**DO Phase:** cpp-qt-developer implements above tasks  
**CHECK Phase:** code-reviewer evaluates at completion  
**ACT Phase:** Apply improvements and merge if approved

---

#### Wave 2.2: Journaline Text Service Decoder
**Duration:** 3 days  
**Priority:** Medium  
**Dependencies:** MOT Protocol Parser (Wave 1.2)

**PLAN Phase (PM Agent):**

**Objective:** Implement Journaline decoder per ETSI TS 102 979

**Tasks for cpp-qt-developer:**
1. **Day 1: Journaline Data Structures**
   - Create `src/core/journaline_decoder.hpp`
   - Define Journaline structures:
     ```cpp
     struct JournalineObject {
         uint16_t object_id;
         QString text_content;
         QString title;
         QDateTime timestamp;
         uint8_t category;
         bool is_link;
         uint16_t link_target;
     };
     
     struct JournalineMenu {
         QString menu_title;
         QVector<JournalineObject> items;
         uint16_t menu_id;
     };
     ```

2. **Day 2: Journaline Parsing**
   - Implement object decoding
   - Parse hierarchical menu structure
   - Handle text content encoding (UTF-8, Thai)
   - Extract links and navigation

3. **Day 3: Testing & Integration**
   - Qt signal integration
   - Thread safety implementation
   - Basic unit tests
   - Documentation

**Acceptance Criteria:**
- [ ] Parse Journaline objects per ETSI TS 102 979
- [ ] Extract hierarchical menu structure
- [ ] Support Thai UTF-8 text content
- [ ] Qt signals for Journaline updates
- [ ] Zero compilation errors
- [ ] Thread-safe design

**DO Phase:** cpp-qt-developer implements  
**CHECK Phase:** code-reviewer evaluates  
**ACT Phase:** Merge and improve

---

### **Subphase 3: Integration & Testing** ⏳ PLANNED
**Duration:** 3-4 days  
**Status:** Planned for after Subphase 2

#### Wave 3.1: Component Integration
**Duration:** 2 days  
**Priority:** Critical

**PLAN Phase (PM Agent):**

**Objective:** Integrate all Phase 3A components into unified system

**Tasks for cpp-qt-developer:**
1. **Integration Points Implementation**
   ```cpp
   // Connect DAB+ validator to ETI processor
   connect(etiProcessor, &ETIProcessor::mscDataExtracted,
           dabPlusValidator, &DABPlusStreamValidator::processAudioData);
   
   // Connect MOT parser to FIG analyzer
   connect(figAnalyzer, &AdvancedFIGAnalyzer::serviceComponentDiscovered,
           motParser, &MOTProtocol::onServiceComponentDetected);
   
   // Connect EPG decoder to MOT parser
   connect(motParser, &MOTProtocol::motObjectComplete,
           epgDecoder, &EPGDecoder::processMOTObject);
   
   // Connect Journaline to MOT parser
   connect(motParser, &MOTProtocol::motObjectComplete,
           journalineDecoder, &JournalineDecoder::processMOTObject);
   ```

2. **Data Flow Validation**
   - Verify ETI frame → FIG → Service discovery flow
   - Validate MSC extraction → Audio validation flow
   - Test MOT object → EPG/Journaline flow
   - Check signal/slot chains execute correctly

3. **GUI Integration**
   - Connect validators to Qt6 main window
   - Display audio quality metrics
   - Show MOT SlideShow images
   - Render EPG schedule
   - Display Journaline menu

**Acceptance Criteria:**
- [ ] All components connected via Qt signals/slots
- [ ] Data flows correctly through pipeline
- [ ] GUI updates with real data
- [ ] No signal/slot connection failures
- [ ] Thread-safe inter-component communication

---

#### Wave 3.2: End-to-End Testing
**Duration:** 2 days  
**Priority:** Critical

**PLAN Phase (PM Agent):**

**Objective:** Validate complete Phase 3A functionality with real ETI files

**Testing Tasks:**
1. **Real ETI File Testing**
   - Test with Bangkok DAB+ streams (if available)
   - Validate audio stream detection
   - Verify MOT SlideShow extraction
   - Check EPG parsing (if present in stream)
   - Test Journaline decoding (if present)

2. **Performance Testing**
   - Measure processing FPS with real data
   - Check memory usage under load
   - Validate thread safety (ThreadSanitizer)
   - Test sustained operation (30+ minutes)

3. **Error Handling Testing**
   - Test with corrupted ETI frames
   - Validate CRC error recovery
   - Test missing FIG data handling
   - Verify graceful degradation

**Acceptance Criteria:**
- [ ] Successfully process real ETI files
- [ ] Extract audio quality metrics correctly
- [ ] MOT SlideShow images extracted (if present)
- [ ] EPG data parsed (if present)
- [ ] No crashes during extended operation
- [ ] Memory stable (no leaks)
- [ ] Performance meets targets (>1000 FPS)

**DO Phase:** cpp-qt-developer performs testing  
**CHECK Phase:** code-reviewer validates results  
**ACT Phase:** Fix issues, re-test, approve

---

## 📊 **PHASE 3A QUALITY GATES**

### **Wave Completion Gate**
Must pass before moving to next wave:
- [ ] All tasks completed
- [ ] Code compiles without errors/warnings
- [ ] Acceptance criteria met
- [ ] Basic functionality validated
- [ ] Code review grade: B+ or higher

### **Subphase Completion Gate**
Must pass before moving to next subphase:
- [ ] All waves completed
- [ ] Comprehensive code review passed (A- or higher)
- [ ] Integration between waves verified
- [ ] Documentation updated
- [ ] No critical bugs, <3 medium bugs

### **Phase 3A Completion Gate**
Must pass before declaring phase complete:
- [ ] All subphases completed
- [ ] Full integration testing passed
- [ ] End-to-end testing with real ETI files successful
- [ ] Performance benchmarks met (>1000 FPS)
- [ ] Qt6 GUI integration working
- [ ] All documentation complete
- [ ] Code review final grade: A or higher
- [ ] Zero critical bugs, <5 medium bugs
- [ ] User acceptance testing passed

---

## 🔄 **PDCA EXECUTION WORKFLOW**

### **For Each Wave:**

```
1. PLAN (PM Agent)
   ├─ Define tasks for cpp-qt-developer
   ├─ Set acceptance criteria
   ├─ Identify dependencies
   └─ Estimate duration

2. DO (cpp-qt-developer)
   ├─ Implement tasks
   ├─ Write code with documentation
   ├─ Create basic unit tests
   └─ Report completion

3. CHECK (code-reviewer)
   ├─ Review code quality
   ├─ Verify acceptance criteria
   ├─ Test functionality
   ├─ Grade implementation
   └─ Provide feedback

4. ACT (PM Agent + Team)
   ├─ If APPROVED: Merge and proceed
   ├─ If REJECTED: Rework and re-review
   ├─ Apply improvements
   └─ Plan next wave
```

---

## 📅 **PHASE 3A TIMELINE**

| **Week** | **Subphase** | **Wave** | **Component** | **Status** | **Duration** |
|----------|-------------|---------|---------------|-----------|--------------|
| Week 8 | Subphase 1 | Wave 1.1 | DAB+ Stream Validator | ✅ COMPLETE | 3 days |
| Week 9 | Subphase 1 | Wave 1.2 | MOT Protocol Parser | 🔄 95% | 4 days |
| Week 10 | Subphase 2 | Wave 2.1 | EPG Decoder | ⏳ PLANNED | 4 days |
| Week 10-11 | Subphase 2 | Wave 2.2 | Journaline Decoder | ⏳ PLANNED | 3 days |
| Week 11 | Subphase 3 | Wave 3.1 | Integration | ⏳ PLANNED | 2 days |
| Week 11 | Subphase 3 | Wave 3.2 | E2E Testing | ⏳ PLANNED | 2 days |

**Total Duration:** 18 days (2.5-3 weeks)  
**Current Progress:** ~40% complete

---

## 🎯 **CURRENT STATUS & NEXT ACTIONS**

### **Completed:**
- ✅ Wave 1.1: DAB+ Stream Validator (A+ grade)
- ✅ Wave 1.2: MOT Protocol Parser (implementation done)
- ✅ TDD framework removed
- ✅ PDCA workflow established
- ✅ Agents configured for Sonnet 4.5

### **In Progress:**
- 🔄 Wave 1.2: MOT Protocol testing and code review

### **Next Immediate Actions:**

1. **Complete Wave 1.2 (MOT Protocol)**
   ```
   User: "cpp-qt-developer: Create unit tests for MOT Protocol parser
          Focus on:
          - MSC Data Group parsing
          - Segment reassembly
          - Content type detection
          - CRC validation
          Use Qt Test framework"
   ```

2. **Code Review for Wave 1.2**
   ```
   User: "code-reviewer: Review MOT Protocol implementation
          Files: src/core/mot_protocol.hpp, src/core/mot_protocol.cpp
          Check: ETSI EN 301 234 compliance, code quality, Thai UTF-8 support"
   ```

3. **Start Wave 2.1 (EPG Decoder)**
   ```
   User: "cpp-qt-developer: Implement EPG decoder per Wave 2.1 plan
          Follow PLAN phase tasks outlined in docs/PHASE_3A_PDCA_PLAN.md"
   ```

---

## 📚 **REFERENCE DOCUMENTS**

### **PDCA Workflow:**
- [PDCA_WORKFLOW.md](PDCA_WORKFLOW.md) - Complete methodology guide

### **Phase Documentation:**
- [PHASE_3A_AUDIO_DATA_SERVICES.md](PHASE_3A_AUDIO_DATA_SERVICES.md) - Original phase plan
- [PHASE_3A_WEEK8_IMPLEMENTATION_REPORT.md](PHASE_3A_WEEK8_IMPLEMENTATION_REPORT.md) - Week 8 completion

### **ETSI Standards:**
- ETSI TS 102 563 - DAB+ Audio Coding (HE-AAC v2)
- ETSI EN 301 234 - MOT Protocol Specification
- ETSI TS 102 371 - Electronic Program Guide (EPG)
- ETSI TS 102 979 - Journaline Specification
- ETSI EN 300 401 - DAB System Specification

---

## ✅ **PHASE 3A SUCCESS METRICS**

### **Technical Metrics:**
- [ ] Audio stream validation working (✅ COMPLETE)
- [ ] MOT SlideShow extraction working (🔄 95%)
- [ ] EPG parsing working (⏳ PLANNED)
- [ ] Journaline decoding working (⏳ PLANNED)
- [ ] All components integrated (⏳ PLANNED)
- [ ] Performance >1000 FPS (⏳ TO VALIDATE)

### **Quality Metrics:**
- [ ] Code review grade A or higher
- [ ] Zero critical bugs
- [ ] <5 medium bugs
- [ ] 100% wave completion
- [ ] All acceptance criteria met

### **Documentation Metrics:**
- [ ] All components documented (Doxygen)
- [ ] Implementation reports complete
- [ ] User guide updated
- [ ] ETSI compliance validated

---

## 🚀 **READY FOR EXECUTION**

**Phase 3A Status:** Ready to proceed with Wave 1.2 completion

**Agent Deployment Ready:**
- ✅ pm-agent configured (Sonnet 4.5)
- ✅ cpp-qt-developer configured (Sonnet 4.5)
- ✅ code-reviewer available
- ✅ PDCA workflow established
- ✅ Build system simplified

**User Commands to Proceed:**

```bash
# Complete MOT Protocol testing
User: "cpp-qt-developer: implement unit tests for MOT Protocol"

# After testing, request code review
User: "code-reviewer: review Wave 1.2 (MOT Protocol)"

# If approved, start EPG
User: "cpp-qt-developer: implement EPG decoder per Wave 2.1 plan"
```

---

**Document Status:** ✅ COMPLETE  
**Next Review:** After Wave 1.2 completion  
**PM Agent Status:** Standing by for deployment commands


---

## 🧪 **PHASE 3A TESTING STRATEGY (PDCA DO PHASE)**

### **Testing Requirements Summary**

**Every Wave Must Include:**

1. ✅ **Unit Tests (MANDATORY)**
   - Test individual functions and classes
   - Cover normal operation, edge cases, error conditions
   - Use ETSI test vectors where available
   - Minimum 80% coverage for critical code paths

2. ✅ **Integration Tests (MANDATORY)**
   - Test component interactions
   - Verify Qt signal/slot chains
   - Test with realistic or real data
   - End-to-end workflow validation

3. ✅ **Memory Safety Validation (MANDATORY)**
   - Run Valgrind before wave completion
   - Must achieve 0 bytes leaked
   - Document results in wave completion report

4. ✅ **Thread Safety Validation (If Applicable)**
   - Run ThreadSanitizer for multi-threaded code
   - Must achieve 0 data races
   - Document concurrent access patterns

---

### **Test Framework & Tools**

**Unit Testing:**
- **Framework:** Qt Test (QTest)
- **Build Integration:** CMakeLists.txt in tests/
- **Execution:** `cd build && ctest --output-on-failure`

**Integration Testing:**
- **Location:** tests/integration/
- **Framework:** Qt Test with real component instances
- **Test Data:** tests/testdata/

**Memory Safety:**
- **Tool:** Valgrind
- **Command:** `valgrind --leak-check=full --show-leak-kinds=all ./test_<component>`
- **Requirement:** "All heap blocks were freed -- no leaks are possible"

**Thread Safety:**
- **Tool:** ThreadSanitizer (TSan)
- **Build:** `cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..`
- **Requirement:** "ThreadSanitizer: reported 0 warnings"

---

### **Test Coverage by Wave**

| **Wave** | **Unit Tests** | **Integration Tests** | **Memory Safety** | **Thread Safety** |
|----------|----------------|----------------------|-------------------|-------------------|
| **1.1 DAB+ Validator** | ✅ DONE (10 tests) | ✅ DONE | ✅ Valgrind clean | ✅ TSan clean |
| **1.2 MOT Protocol** | ⏳ TODO (8 tests) | ⏳ TODO | ⏳ TODO | ⏳ TODO |
| **2.1 EPG Decoder** | ⏳ PLANNED (6 tests) | ⏳ PLANNED | ⏳ PLANNED | N/A |
| **2.2 Journaline** | ⏳ PLANNED (5 tests) | ⏳ PLANNED | ⏳ PLANNED | N/A |
| **3.1 Integration** | N/A | ⏳ PLANNED (E2E) | ⏳ PLANNED | ⏳ PLANNED |
| **3.2 E2E Testing** | N/A | ⏳ PLANNED (Full) | ⏳ PLANNED | ⏳ PLANNED |

---

### **Sample Test Implementation Templates**

#### **Unit Test Template**
```cpp
// tests/test_<component>.cpp
#include <QtTest/QtTest>
#include "../src/core/<component>.hpp"

class Test<Component> : public QObject {
    Q_OBJECT

private slots:
    // Test lifecycle
    void initTestCase();     // Setup once before all tests
    void init();             // Setup before each test
    void cleanup();          // Cleanup after each test
    void cleanupTestCase();  // Cleanup once after all tests
    
    // Normal operation tests
    void test<Feature>_ValidInput();
    void test<Feature>_NormalOperation();
    
    // Edge case tests
    void test<Feature>_EmptyData();
    void test<Feature>_BoundaryValues();
    void test<Feature>_MaximumLoad();
    
    // Error handling tests
    void test<Feature>_InvalidInput();
    void test<Feature>_CorruptedData();
    void test<Feature>_NullPointer();
    
    // ETSI compliance tests (if applicable)
    void test<Feature>_ETSITestVector1();
    void test<Feature>_ETSICompliance();
    
private:
    <Component>* m_component;
};

void Test<Component>::initTestCase() {
    qInfo() << "Starting" << metaObject()->className() << "test suite";
}

void Test<Component>::init() {
    m_component = new <Component>();
}

void Test<Component>::cleanup() {
    delete m_component;
    m_component = nullptr;
}

void Test<Component>::test<Feature>_ValidInput() {
    // Arrange
    QByteArray test_data = generateValidTestData();
    
    // Act
    bool result = m_component->process(test_data);
    
    // Assert
    QVERIFY(result);
    QCOMPARE(m_component->getStatus(), Status::Success);
}

QTEST_MAIN(Test<Component>)
#include "test_<component>.moc"
```

#### **Integration Test Template**
```cpp
// tests/integration/test_<feature>_pipeline.cpp
#include <QtTest/QtTest>
#include "../src/core/enhanced_eti_processor_qt.hpp"
#include "../src/core/<component1>.hpp"
#include "../src/core/<component2>.hpp"

class Test<Feature>Pipeline : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testEndToEndWorkflow();
    void testSignalSlotChain();
    void testComponentInteraction();
    void testErrorPropagation();

private:
    ETIFrameParser* m_eti_parser;
    <Component1>* m_component1;
    <Component2>* m_component2;
};

void Test<Feature>Pipeline::testEndToEndWorkflow() {
    // Load real test data
    QFile test_file("testdata/<feature>_test.eti");
    QVERIFY(test_file.open(QIODevice::ReadOnly));
    
    // Process through full pipeline
    QByteArray frame_data = test_file.read(6144);
    ETIFrameData frame;
    QVERIFY(m_eti_parser->parseFrame(frame_data, frame));
    
    // Verify component 1 processing
    m_component1->processFrame(frame);
    
    // Verify component 2 receives data
    QSignalSpy spy(m_component2, &<Component2>::dataReceived);
    // ... trigger pipeline
    QCOMPARE(spy.count(), 1);
    
    // Verify final output
    auto result = m_component2->getResult();
    QVERIFY(result.isValid());
}

QTEST_MAIN(Test<Feature>Pipeline)
#include "test_<feature>_pipeline.moc"
```

---

### **Test Data Management**

**Directory Structure:**
```
tests/testdata/
├── README.md                        # Test data documentation
├── eti_samples/
│   ├── bangkok_dabplus.eti         # Real ETI stream (if available)
│   ├── synthetic_audio.eti         # Generated test stream
│   └── etsi_test_vectors.eti       # ETSI standard test data
├── mot_samples/
│   ├── slideshow_image.bin         # MOT SlideShow test data
│   ├── mot_directory.bin           # MOT directory structure
│   └── thai_text_mot.bin           # Thai language MOT object
├── epg_samples/
│   ├── epg_schedule.bin            # EPG test data
│   └── thai_epg.bin                # Thai language EPG
└── journaline_samples/
    ├── journaline_menu.bin         # Journaline test data
    └── thai_journaline.bin         # Thai language Journaline
```

**Test Data Generation:**
```cpp
// tests/test_data_generator.hpp
class TestDataGenerator {
public:
    static QByteArray generateETIFrame(bool valid_crc = true);
    static QByteArray generateMOTObject(ContentType type);
    static QByteArray generateEPGEvent(const QString& program_name);
    static QByteArray generateThaiText(const QString& text);
};
```

---

### **Continuous Testing Integration**

**Build System Integration:**
```cmake
# tests/CMakeLists.txt

# Enable testing
enable_testing()

# Add all tests
set(TEST_SOURCES
    test_eti_frame_parser.cpp
    test_fig0_processor.cpp
    test_fig1_processor.cpp
    test_dabplus_validator.cpp
    test_mot_protocol.cpp
    test_epg_decoder.cpp
    test_journaline_decoder.cpp
)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src} ${CORE_SOURCES})
    target_link_libraries(${test_name} Qt6::Test Qt6::Core pthread)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Add integration tests
add_subdirectory(integration)
```

**Makefile Targets:**
```makefile
# Run all tests
test: build
	cd build && ctest --output-on-failure

# Run with coverage
test-coverage: build
	cd build && ctest --output-on-failure
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage-report

# Run memory check
test-memory: build
	cd build && ctest -T memcheck

# Run specific test
test-%: build
	cd build && ./tests/test_$*
```

---

### **Testing Checklist for cpp-qt-developer**

**Before Marking Wave Complete:**

- [ ] **Unit Tests Written**
  - [ ] All public methods tested
  - [ ] Edge cases covered
  - [ ] Error conditions tested
  - [ ] ETSI test vectors used (if applicable)

- [ ] **Integration Tests Written**
  - [ ] Component interactions tested
  - [ ] Qt signals/slots verified
  - [ ] Real/realistic test data used

- [ ] **All Tests Passing**
  - [ ] `ctest --output-on-failure` shows 100% pass
  - [ ] No test failures or warnings

- [ ] **Memory Safety Verified**
  - [ ] Valgrind run: `valgrind --leak-check=full ./test_<component>`
  - [ ] Result: "0 bytes leaked" ✓

- [ ] **Thread Safety Verified (if multi-threaded)**
  - [ ] ThreadSanitizer run with test suite
  - [ ] Result: "0 data races" ✓

- [ ] **Build Clean**
  - [ ] Zero compilation errors
  - [ ] Zero compilation warnings (-Wall -Wextra)

- [ ] **Documentation Updated**
  - [ ] Test cases documented in code
  - [ ] Test data documented
  - [ ] Test execution instructions in README

**Only when ALL checkboxes are ✓ should wave be submitted for code review.**

---

