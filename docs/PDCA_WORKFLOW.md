# PDCA Development Workflow
## Plan-Do-Check-Act Methodology for StreamDAB Analyser

**Document Version:** 1.0  
**Date:** October 22, 2025  
**Effective:** Immediate (replaces TDD methodology)

---

## 📋 **WORKFLOW OVERVIEW**

The StreamDAB Analyser project follows the **PDCA (Plan-Do-Check-Act)** continuous improvement cycle:

```
┌──────────┐
│   PLAN   │ ← PM Agent defines phases, subphases, and waves
└────┬─────┘
     │
     ↓
┌──────────┐
│    DO    │ ← cpp-qt-developer implements tasks
└────┬─────┘
     │
     ↓
┌──────────┐
│  CHECK   │ ← code-reviewer reviews at wave/subphase completion
└────┬─────┘
     │
     ↓
┌──────────┐
│   ACT    │ ← Apply improvements and iterate
└────┬─────┘
     │
     └──────────→ (Return to PLAN for next iteration)
```

---

## 🎯 **AGENT ROLES & RESPONSIBILITIES**

### **1. PM Agent (Plan)**

**Primary Role:** Strategic planning and project structure

**Responsibilities:**
- Define project phases (e.g., Phase 3A, Phase 3B)
- Break down phases into subphases (logical feature groups)
- Organise subphases into waves (implementation units)
- Set clear success criteria for each wave
- Track overall project progress
- Adjust plans based on feedback from Check phase

**Deliverables:**
- Phase implementation plans
- Subphase breakdowns
- Wave task lists with acceptance criteria
- Project status reports
- Risk assessment and mitigation plans

**Example Output:**
```markdown
## Phase 3A: Audio & Data Services

### Subphase 1: Audio Stream Processing (Weeks 8-9)
- Wave 1.1: DAB+ Stream Validator (3 days)
  - Task: Implement FireCode CRC validation
  - Task: Implement Reed-Solomon FEC
  - Acceptance: Validate 120ms superframes per ETSI TS 102 563
  
- Wave 1.2: MOT Protocol Parser (4 days)
  - Task: Implement MSC Data Group parsing
  - Task: Implement segment reassembly
  - Acceptance: Parse MOT SlideShow images
```

---

### **2. cpp-qt-developer (Do)**

**Primary Role:** Implementation, development, and testing

**Core Responsibilities:**
- Implement tasks defined in waves by PM agent
- Write production-quality C++ code with Qt6 integration
- Create comprehensive inline documentation
- Follow ETSI standards specifications
- **Create unit tests and integration tests (CRITICAL)**
- Report blockers and issues immediately
- Request clarification when requirements are ambiguous

**DO Phase Workflow:**
```
Implementation → Unit Testing → Integration Testing → Documentation → Ready for Review
```

---

#### **DO Phase: Implementation**

**Code Development:**
- Write C++20 compliant code with Qt6 integration
- Follow ETSI specifications exactly
- Use RAII and smart pointers for memory safety
- Implement thread-safe designs with QMutex where needed
- Add comprehensive Doxygen comments
- Handle all error cases gracefully

**Example Implementation:**
```cpp
/**
 * @brief Calculate FireCode CRC-16 for DAB+ audio superframe
 * @param data Superframe data (120 bytes)
 * @return Calculated 16-bit CRC value
 * 
 * Implements polynomial: g(x) = x^16 + x^14 + ... (0x782F)
 * Per ETSI TS 102 563 Section 6.2: FireCode Inner Coding
 */
uint16_t DABPlusStreamValidator::calculateFireCodeCRC(
    const std::vector<uint8_t>& data) 
{
    // Implementation with lookup table for performance
    uint16_t crc = 0xFFFF;
    for (const auto& byte : data) {
        crc = (crc << 8) ^ m_firecode_table[(crc >> 8) ^ byte];
    }
    return crc;
}
```

---

#### **DO Phase: Unit Testing (MANDATORY)**

**Unit Test Creation:**
After implementing each significant function/class, cpp-qt-developer **MUST** create unit tests using Qt Test framework.

**Unit Test Structure:**
```cpp
// tests/test_dabplus_validator.cpp
#include <QtTest/QtTest>
#include "../src/core/dabplus_stream_validator.hpp"

class TestDABPlusValidator : public QObject {
    Q_OBJECT

private slots:
    // Test lifecycle
    void initTestCase();    // Run once before all tests
    void init();            // Run before each test
    void cleanup();         // Run after each test
    void cleanupTestCase(); // Run once after all tests
    
    // Unit tests for FireCode CRC
    void testFireCodeCRC_ValidData();
    void testFireCodeCRC_EmptyData();
    void testFireCodeCRC_KnownVector();
    void testFireCodeCRC_TableInitialization();
    
    // Unit tests for Reed-Solomon
    void testReedSolomon_RS120_110();
    void testReedSolomon_RS120_96();
    void testReedSolomon_InvalidProfile();
    
    // Unit tests for audio metrics
    void testAudioMetrics_HEAACv2Detection();
    void testAudioMetrics_BitrateExtraction();
    void testAudioMetrics_SampleRateDetection();
    
private:
    DABPlusStreamValidator* m_validator;
};

void TestDABPlusValidator::testFireCodeCRC_KnownVector() {
    // Test with known good data
    std::vector<uint8_t> test_data = {
        0x01, 0x02, 0x03, 0x04, 0x05  // Known test vector
    };
    
    uint16_t expected_crc = 0x1234;  // Known correct CRC
    uint16_t actual_crc = m_validator->calculateFireCodeCRC(test_data);
    
    QCOMPARE(actual_crc, expected_crc);
}

void TestDABPlusValidator::testFireCodeCRC_EmptyData() {
    // Test edge case: empty data
    std::vector<uint8_t> empty_data;
    
    // Should not crash, should return initial CRC value
    QVERIFY_NO_EXCEPTION(m_validator->calculateFireCodeCRC(empty_data));
}

QTEST_MAIN(TestDABPlusValidator)
#include "test_dabplus_validator.moc"
```

**Unit Test Requirements:**
- ✅ Test normal operation (happy path)
- ✅ Test edge cases (empty data, boundary values)
- ✅ Test error conditions (invalid input)
- ✅ Test ETSI compliance (known test vectors)
- ✅ Each public method should have at least 1 test
- ✅ Critical algorithms should have 3+ tests

**CMakeLists.txt Integration:**
```cmake
# Add unit test
add_executable(test_dabplus_validator
    tests/test_dabplus_validator.cpp
    src/core/dabplus_stream_validator.cpp
)
target_link_libraries(test_dabplus_validator
    Qt6::Test Qt6::Core pthread
)
add_test(NAME test_dabplus_validator COMMAND test_dabplus_validator)
```

---

#### **DO Phase: Integration Testing (MANDATORY)**

**Integration Test Creation:**
After completing a wave, cpp-qt-developer creates integration tests that verify components work together.

**Integration Test Structure:**
```cpp
// tests/integration/test_audio_pipeline.cpp
#include <QtTest/QtTest>
#include "../src/core/enhanced_eti_processor_qt.hpp"
#include "../src/core/advanced_fig_analyser.hpp"
#include "../src/core/dabplus_stream_validator.hpp"

class TestAudioPipeline : public QObject {
    Q_OBJECT

private slots:
    void testETIToAudioQualityMetrics();
    void testServiceDiscoveryToAudioValidation();
    void testSignalSlotChain();

private:
    ETIFrameParser* m_eti_parser;
    AdvancedFIGAnalyser* m_fig_analyser;
    DABPlusStreamValidator* m_audio_validator;
};

void TestAudioPipeline::testETIToAudioQualityMetrics() {
    // Load real ETI file
    QFile eti_file("testdata/bangkok_dabplus.eti");
    QVERIFY(eti_file.open(QIODevice::ReadOnly));
    
    // Parse ETI frame
    QByteArray frame_data = eti_file.read(6144);
    ETIFrameData frame;
    QVERIFY(m_eti_parser->parseFrame(frame_data, frame));
    
    // Process FIG data to discover services
    m_fig_analyser->processFIC(frame.fic_data);
    
    // Extract MSC data and validate audio
    QByteArray msc_data = m_eti_parser->extractMSC(frame, 0);
    bool result = m_audio_validator->processAudioData(0xC221, msc_data);
    QVERIFY(result);
    
    // Verify quality metrics extracted
    auto metrics = m_audio_validator->getMetrics(0xC221);
    QVERIFY(metrics != nullptr);
    QVERIFY(metrics->bitrate_kbps > 0);
    QVERIFY(metrics->quality_estimate > 0);
}

void TestAudioPipeline::testSignalSlotChain() {
    // Verify Qt signal/slot connections
    QSignalSpy spy_audio_detected(m_audio_validator, 
                                   &DABPlusStreamValidator::audioStreamDetected);
    
    // Trigger pipeline
    QByteArray test_msc_data = generateTestMSCData();
    m_audio_validator->processAudioData(0xC221, test_msc_data);
    
    // Verify signal emitted
    QCOMPARE(spy_audio_detected.count(), 1);
}

QTEST_MAIN(TestAudioPipeline)
#include "test_audio_pipeline.moc"
```

**Integration Test Requirements:**
- ✅ Test component interactions (ETI → FIG → Audio)
- ✅ Test Qt signal/slot chains
- ✅ Test with real or realistic test data
- ✅ Verify end-to-end workflows
- ✅ Test thread safety (if multi-threaded)
- ✅ Test error propagation between components

---

#### **DO Phase: Test Data Preparation**

**Test Data Requirements:**
cpp-qt-developer should create or identify test data files:

```
tests/testdata/
├── bangkok_dabplus.eti          # Real ETI file (if available)
├── synthetic_audio_frame.bin    # Generated test audio data
├── mot_slideshow_sample.bin     # MOT SlideShow test data
├── epg_test_data.bin            # EPG test data
└── known_test_vectors.json      # ETSI test vectors
```

**Creating Synthetic Test Data:**
```cpp
// tests/test_data_generator.cpp
QByteArray generateTestAudioSuperframe(bool valid_crc = true) {
    QByteArray superframe(120, 0x00);
    
    // Set up audio superframe structure per ETSI TS 102 563
    superframe[0] = 0x00;  // FireCode header byte 1
    superframe[1] = 0x00;  // FireCode header byte 2
    superframe[2] = 0x04;  // Number of AUs: 4
    superframe[3] = 0x80;  // SBR flag = 1, PS flag = 0 (HE-AAC)
    
    // Calculate and set CRC
    if (valid_crc) {
        uint16_t crc = calculateFireCodeCRC(superframe.mid(2, 118));
        superframe[0] = (crc >> 8) & 0xFF;
        superframe[1] = crc & 0xFF;
    }
    
    return superframe;
}
```

---

#### **DO Phase: Documentation**

**Inline Documentation (Doxygen):**
```cpp
/**
 * @class DABPlusStreamValidator
 * @brief Validates DAB+ audio superframes per ETSI TS 102 563
 *
 * This class performs stream validation only - no audio decoding.
 * It validates FireCode CRC, Reed-Solomon FEC, and extracts quality metrics.
 *
 * Thread-safe: All public methods protected with QMutex.
 *
 * @example
 * @code
 * DABPlusStreamValidator validator;
 * QByteArray msc_data = extractMSCFromETI(eti_frame);
 * if (validator.processAudioData(service_id, msc_data)) {
 *     auto metrics = validator.getMetrics(service_id);
 *     qInfo() << "Bitrate:" << metrics->bitrate_kbps << "kbps";
 * }
 * @endcode
 *
 * @see ETSI TS 102 563 Section 4 (Audio Superframe Structure)
 * @see ETSI TS 102 563 Section 6.2 (FireCode Inner Coding)
 */
```

**Implementation Notes for Reviewer:**
```cpp
// Implementation note for code-reviewer:
// FireCode polynomial 0x782F is specific to ETSI TS 102 563.
// Do not confuse with CCITT CRC-16 (0x1021) used elsewhere.
// Lookup table generated at construction for O(n) performance.
```

---

#### **DO Phase: Deliverables Checklist**

Before marking wave as complete, cpp-qt-developer must deliver:

- [ ] **Implementation files** (.cpp/.hpp) with production-quality code
- [ ] **Unit tests** (tests/test_<component>.cpp) with 80%+ coverage of critical functions
- [ ] **Integration tests** (tests/integration/test_<feature>.cpp) for component interactions
- [ ] **Test data** (tests/testdata/) if needed for validation
- [ ] **Doxygen documentation** inline in code
- [ ] **Build integration** (CMakeLists.txt updated)
- [ ] **All tests passing** (ctest shows 100% pass rate)
- [ ] **Zero compilation warnings**
- [ ] **Implementation notes** for code-reviewer in comments

---

**Code Quality Standards:**
- C++20 standard compliance
- Qt6 best practices (signals/slots, QObject hierarchy)
- ETSI specification adherence
- Thread-safe where applicable (QMutex)
- Memory-safe (RAII, smart pointers)
- Professional error handling
- **Comprehensive test coverage**

**Example Workflow:**
```cpp
// Wave task: Implement FireCode CRC validation
// Acceptance criteria: Calculate CRC per ETSI TS 102 563 Section 6.2

/**
 * @brief Calculate FireCode CRC-16 for DAB+ audio superframe
 * @param data Superframe data (120 bytes)
 * @return Calculated 16-bit CRC value
 * 
 * Implements polynomial: g(x) = x^16 + x^14 + ... (0x782F)
 * Per ETSI TS 102 563 Section 6.2: FireCode Inner Coding
 */
uint16_t DABPlusStreamValidator::calculateFireCodeCRC(
    const std::vector<uint8_t>& data) 
{
    // Implementation with lookup table for performance
    // ...
}
```

---

### **3. code-reviewer (Check)**

**Primary Role:** Quality assurance and validation

**Responsibilities:**
- Review code at the end of each wave or subphase
- Verify implementation meets acceptance criteria
- Check ETSI compliance where applicable
- Validate code quality (memory safety, thread safety, style)
- Test functionality with sample data
- Identify bugs, issues, and improvements
- Provide constructive feedback with specific recommendations

**Review Scope:**
- **Wave Completion:** Quick review (30-60 minutes)
  - Verify acceptance criteria met
  - Check critical bugs
  - Basic code quality validation
  
- **Subphase Completion:** Comprehensive review (2-4 hours)
  - Full code quality assessment
  - Integration testing recommendations
  - Performance considerations
  - Documentation completeness
  - ETSI compliance verification

**Deliverables:**
- Code review report with grade (A+ to D)
- List of issues by severity (Critical, High, Medium, Low)
- Specific recommendations for improvements
- Approval/rejection decision for wave/subphase
- Suggestions for Act phase improvements

**Review Criteria:**
```markdown
## Code Review Checklist

### Functionality (40%)
- [ ] Meets all acceptance criteria
- [ ] Handles edge cases
- [ ] Error handling implemented
- [ ] No crashes or undefined behavior

### Code Quality (30%)
- [ ] Clean, readable code
- [ ] Follows project conventions
- [ ] No code duplication
- [ ] Proper abstraction levels

### Standards Compliance (20%)
- [ ] ETSI specification adherence
- [ ] Qt6 best practices
- [ ] C++20 standard compliance

### Documentation (10%)
- [ ] Comprehensive inline comments
- [ ] Doxygen documentation
- [ ] Usage examples where applicable
```

---

## 🔄 **PDCA CYCLE WORKFLOW**

### **Phase Structure**

```
Phase (e.g., Phase 3A: Audio & Data Services)
├── Subphase 1 (e.g., Audio Stream Processing)
│   ├── Wave 1.1 (DAB+ Stream Validator)
│   │   ├── Task: FireCode CRC
│   │   ├── Task: Reed-Solomon FEC
│   │   └── Task: Quality metrics extraction
│   │   → CODE REVIEW (Check)
│   │
│   ├── Wave 1.2 (MOT Protocol Parser)
│   │   ├── Task: MSC Data Group parsing
│   │   ├── Task: Segment reassembly
│   │   └── Task: Content type detection
│   │   → CODE REVIEW (Check)
│   │
│   └── Subphase Review → COMPREHENSIVE CHECK
│
├── Subphase 2 (e.g., Data Services)
│   └── (Similar wave structure)
│
└── Phase Completion Review → INTEGRATION CHECK
```

---

### **Wave Execution Flow**

#### **Step 1: PLAN (PM Agent)**
```markdown
## Wave 1.1: DAB+ Stream Validator

**Duration:** 3 days
**Priority:** High
**Dependencies:** ETI frame parser, FIG 0/2 processor

**Tasks:**
1. Implement FireCode CRC validation (1 day)
2. Implement Reed-Solomon FEC validation (1 day)
3. Extract audio quality metrics (1 day)

**Acceptance Criteria:**
- Parse 120ms DAB+ superframes correctly
- Validate FireCode CRC per ETSI TS 102 563 Section 6.2
- Validate Reed-Solomon RS(120,110) and RS(120,96)
- Extract bitrate, sample rate, profile (HE-AAC v2)
- Qt signals emit for audio stream events

**Success Metrics:**
- Zero compilation errors
- Process 1000+ superframes without crash
- Correctly identify HE-AAC v2 profile
```

#### **Step 2: DO (cpp-qt-developer)**
Developer implements tasks:
```bash
# Create implementation files
src/core/dabplus_stream_validator.hpp  # 496 lines
src/core/dabplus_stream_validator.cpp  # 643 lines

# Build and basic testing
cd build && make
./StreamDABAnalyser  # Manual verification

# Report completion
"Wave 1.1 complete - DAB+ Stream Validator implemented
 - FireCode CRC with lookup table ✓
 - Reed-Solomon validation ✓
 - Quality metrics extraction ✓
 - All acceptance criteria met
 - Ready for code review"
```

#### **Step 3: CHECK (code-reviewer)**
Reviewer evaluates implementation:
```markdown
## Code Review Report - Wave 1.1

**Grade:** A+ (99/100)
**Status:** APPROVED ✓

### Strengths:
- Excellent ETSI TS 102 563 compliance
- Professional code quality with comprehensive documentation
- Efficient FireCode lookup table implementation
- Thread-safe design with QMutex
- Comprehensive error handling

### Issues Found:
- None (exemplary implementation)

### Recommendations:
- Consider caching Reed-Solomon validation results
- Add integration test with real ETI files (deferred to subphase review)

**Approval:** Wave 1.1 approved for merge
```

#### **Step 4: ACT (PM Agent + cpp-qt-developer + Related Subagents)**

**Primary Role:** Apply improvements, iterate on quality, plan next steps

**ACT Phase Activities:**

---

##### **ACT Phase: If Code Review APPROVED**

**Immediate Actions (PM Agent):**
```markdown
## Wave 1.1 Review Result: APPROVED ✓

### 1. Merge Implementation
- ✓ Merge Wave 1.1 code to main branch
- ✓ Update project documentation
- ✓ Mark wave as complete in tracking

### 2. Capture Improvements for Future
- Add Reed-Solomon caching to optimization backlog
- Schedule integration testing for Subphase 1 review
- Document lessons learned:
  * Lookup table approach worked well for CRC
  * Thread-safe design pattern successful
  * ETSI test vectors valuable for validation

### 3. Plan Next Iteration
- Proceed to Wave 1.2: MOT Protocol Parser
- Allocate 4 days (complexity analysis suggests more time needed)
- Pre-emptively gather MOT SlideShow test files
- Identify dependencies: FIG 0/2 parser, MSC extraction
```

**Update Project Metrics:**
- Wave velocity: 3 days planned vs 3 days actual ✓
- Code quality: A+ grade achieved
- Defect density: 0 bugs per 1000 LOC
- Test coverage: 85% (unit tests created)

---

##### **ACT Phase: If Code Review REJECTED or Has Issues**

**Scenario:** Code review finds Critical or High severity issues

**Immediate Actions (PM Agent + cpp-qt-developer):**

**1. Issue Triage and Planning:**
```markdown
## Wave 1.1 Review Result: NEEDS IMPROVEMENT (Grade B)

### Critical Issues (Must Fix):
1. Memory leak in FireCode table initialization
   - **Assigned to:** cpp-qt-developer
   - **Priority:** P0 (Critical)
   - **Estimated effort:** 2 hours
   - **Fix approach:** Use std::array instead of raw pointer

2. Thread safety violation in getMetrics()
   - **Assigned to:** cpp-qt-developer
   - **Priority:** P0 (Critical)
   - **Estimated effort:** 1 hour
   - **Fix approach:** Add QMutexLocker in getter method

### High Priority Issues (Should Fix):
3. Missing edge case handling for zero-length superframes
   - **Assigned to:** cpp-qt-developer
   - **Priority:** P1 (High)
   - **Estimated effort:** 3 hours
   - **Fix approach:** Add input validation with unit test

### Medium Priority Issues (Can Defer):
4. Improve Doxygen documentation examples
   - **Priority:** P2 (Medium)
   - **Can defer to:** Subphase review
```

**2. Rework Execution (cpp-qt-developer):**

**Rework Workflow:**
```
Analyze Issues → Fix Code → Add/Update Tests → Re-test → Request Re-review
```

**Example Rework: Memory Leak Fix**
```cpp
// BEFORE (Issue found by code-reviewer):
class DABPlusStreamValidator {
private:
    uint16_t* m_firecode_table;  // ❌ Raw pointer, potential leak
    
    DABPlusStreamValidator() {
        m_firecode_table = new uint16_t[256];
        initializeFireCodeTable();
    }
    // ❌ Missing destructor - memory leak!
};

// AFTER (Fixed by cpp-qt-developer):
class DABPlusStreamValidator {
private:
    std::array<uint16_t, 256> m_firecode_table;  // ✅ RAII, no leak
    
    DABPlusStreamValidator() {
        initializeFireCodeTable();
    }
    // ✅ No manual cleanup needed
};
```

**Add Regression Test:**
```cpp
void TestDABPlusValidator::testMemoryLeaks() {
    // Create and destroy multiple validators
    for (int i = 0; i < 1000; ++i) {
        auto validator = std::make_unique<DABPlusStreamValidator>();
        // Destructor called automatically
    }
    // Run with Valgrind to verify no leaks
}
```

**3. Re-testing (cpp-qt-developer):**
```bash
# Run all tests after fixes
cd build && make && ctest --output-on-failure

# Run Valgrind to verify memory fix
valgrind --leak-check=full ./test_dabplus_validator

# Verify thread safety with ThreadSanitizer
cd build-tsan && cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" .. && make
./test_dabplus_validator
```

**4. Request Re-review:**
```markdown
User: "code-reviewer: re-review Wave 1.1 DAB+ Stream Validator
       Fixes applied:
       - Issue #1: Memory leak fixed (std::array)
       - Issue #2: Thread safety added (QMutexLocker)
       - Issue #3: Edge case handling with unit test
       
       Verification:
       - Valgrind: 0 bytes leaked
       - ThreadSanitizer: 0 data races
       - All unit tests passing
       
       Ready for re-review."
```

**5. Track Rework Metrics (PM Agent):**
```markdown
## Rework Analysis:
- Issues found: 3 (2 critical, 1 high)
- Rework effort: 6 hours
- Root cause: Insufficient memory safety review during implementation
- Prevention: Add Valgrind check to DO phase checklist
- Outcome: Re-review grade improved from B to A-
```

---

##### **ACT Phase: Continuous Improvement Activities**

**Process Improvements (PM Agent):**

**1. Update Wave Templates:**
```markdown
## Improvement: Add Memory Safety Checklist to DO Phase

Based on Wave 1.1 review findings, update DO phase checklist:

- [ ] Use RAII for all resources (no raw new/delete)
- [ ] Use smart pointers (unique_ptr, shared_ptr)
- [ ] Run Valgrind before marking wave complete
- [ ] Run ThreadSanitizer if multi-threaded code
- [ ] Add regression test for each bug found
```

**2. Update Acceptance Criteria:**
```markdown
## Wave Acceptance Criteria Enhancement:

OLD: "Zero compilation errors"
NEW: "Zero compilation errors AND warnings (-Wall -Wextra)"

NEW: "Valgrind clean (0 bytes leaked)"
NEW: "ThreadSanitizer clean (0 data races) if multi-threaded"
```

**3. Knowledge Sharing:**
```markdown
## Lessons Learned - Wave 1.1:

### What Worked Well:
- Lookup table approach for CRC (good performance)
- Qt Test framework integration smooth
- ETSI test vectors valuable for validation

### What Needs Improvement:
- Need Valgrind check in DO phase (missed memory leak)
- Thread safety review should be mandatory for QObject classes
- Edge case testing needs more attention

### Actions Taken:
- Updated DO phase checklist with memory safety items
- Added ThreadSanitizer to CI pipeline
- Created edge case testing template
```

**4. Backlog Management:**
```markdown
## Optimisation Backlog (Future Waves):

Items identified during Wave 1.1 for future optimization:
1. Reed-Solomon validation result caching (Wave 4.2)
   - Priority: Medium
   - Expected improvement: 15-20% performance gain
   
2. SIMD acceleration for FireCode CRC (Wave 4.3)
   - Priority: Low
   - Expected improvement: 30-40% CRC calculation speedup
   
3. Batch audio frame processing (Wave 4.4)
   - Priority: Medium
   - Expected improvement: Reduce signal/slot overhead
```

**5. Agent Skill Refinement:**

**For cpp-qt-developer:**
```markdown
## cpp-qt-developer Improvement Actions:

Based on Wave 1.1 feedback:
1. Review Qt memory management best practices (QObject parent-child)
2. Study RAII patterns for C++ resource management
3. Practice with Valgrind and ThreadSanitizer tools
4. Create personal checklist for code quality before review
```

**For code-reviewer:**
```markdown
## code-reviewer Improvement Actions:

Based on Wave 1.1 review:
1. Create checklist for memory safety review
2. Document common Qt pitfalls to watch for
3. Develop standard test data sets for validators
```

---

##### **ACT Phase: Planning Next Wave**

**Iterative Planning (PM Agent):**

```markdown
## Transition to Wave 1.2: MOT Protocol Parser

### 1. Apply Lessons from Wave 1.1:
- Ensure memory safety checklist in planning
- Allocate time for Valgrind/TSan testing
- Plan for edge case testing from start
- Include test data preparation in estimate

### 2. Adjust Wave 1.2 Estimates:
Original estimate: 3 days
Adjusted estimate: 4 days (based on complexity + quality checks)

Breakdown:
- Day 1: MSC Data Group parsing + unit tests
- Day 2: Segment reassembly + unit tests
- Day 3: Content type detection + integration tests
- Day 4: Memory safety validation + documentation

### 3. Risk Mitigation:
Identified risks for Wave 1.2:
- MOT segment reassembly complexity → Allocate extra time
- Test data scarcity → Pre-gather MOT SlideShow samples
- UTF-8 Thai text handling → Review encoding libraries early

### 4. Success Criteria for Wave 1.2:
- All Wave 1.1 quality standards met
- PLUS: Valgrind clean confirmed
- PLUS: Integration test with real MOT data
- Target grade: A or higher
```

---

##### **ACT Phase: Metrics-Driven Improvement**

**Velocity Analysis:**
```markdown
## Wave Velocity Tracking:

Wave 1.1:
- Planned: 3 days
- Actual: 3 days (initial) + 0.5 days (rework) = 3.5 days
- Efficiency: 86%
- Rework ratio: 14%

Target for Wave 1.2:
- Reduce rework ratio to <10%
- Maintain or improve efficiency
- Achieve A grade on first review
```

**Quality Trend Analysis:**
```markdown
## Code Quality Trends:

Recent waves:
- Wave 1.0 (ETI Parser): Grade A- (2 medium issues)
- Wave 1.1 (DAB+ Validator): Grade A+ (0 issues after rework)

Trend: Improving ↑
Action: Maintain quality focus, share best practices
```

---

##### **ACT Phase: Team Collaboration**

**Cross-Agent Improvements:**

**1. cpp-qt-developer ↔ code-reviewer Feedback Loop:**
```markdown
## Feedback Loop Established:

cpp-qt-developer observes that code-reviewer frequently catches:
- Memory leaks
- Thread safety issues
- Missing edge cases

Actions:
- cpp-qt-developer creates pre-submission checklist
- code-reviewer provides checklist template
- Both agents align on quality standards
```

**2. Related Subagent Coordination:**

When ACT phase identifies need for specialized help:
```markdown
## Example: Performance Optimisation Needed

ACT phase identifies: "FireCode CRC is bottleneck at >5000 FPS"

PM Agent deploys:
- **performance-optimization-agent**: Analyze CRC hotspot
- **cpp-qt-developer**: Implement SIMD version
- **test-automator**: Create performance regression tests
- **code-reviewer**: Validate optimization correctness

Result: Improved performance integrated in next optimization wave
```

---

**ACT Phase Complete When:**
- [ ] Improvements applied (if review had issues)
- [ ] Process refinements documented
- [ ] Lessons learned captured
- [ ] Next wave planned with adjustments
- [ ] Metrics updated
- [ ] Team knowledge shared
- [ ] Ready to return to PLAN phase for next wave

---

## 📊 **REVIEW FREQUENCY & SCOPE**

| **Review Type** | **Trigger** | **Duration** | **Scope** | **Reviewer** |
|-----------------|-------------|--------------|-----------|--------------|
| **Wave Review** | Wave completion (2-4 days work) | 30-60 min | Acceptance criteria, basic quality | code-reviewer |
| **Subphase Review** | Subphase completion (1-2 weeks) | 2-4 hours | Comprehensive quality, integration | code-reviewer |
| **Phase Review** | Phase completion (3-6 weeks) | 4-8 hours | Full integration, performance, docs | code-reviewer + PM |

---

## ✅ **QUALITY GATES**

### **Wave Completion Gates**
- [ ] All tasks in wave completed
- [ ] Acceptance criteria met
- [ ] Code compiles without errors
- [ ] Basic functionality validated
- [ ] Code review approved (grade B+ or higher)

### **Subphase Completion Gates**
- [ ] All waves in subphase completed
- [ ] Integration between waves verified
- [ ] Comprehensive code review passed (grade A- or higher)
- [ ] Documentation updated
- [ ] No critical or high-priority bugs

### **Phase Completion Gates**
- [ ] All subphases completed
- [ ] Full integration testing passed
- [ ] Performance benchmarks met
- [ ] ETSI compliance validated
- [ ] User documentation complete
- [ ] Zero critical bugs, <5 medium bugs

---

## 🎯 **SUCCESS METRICS**

### **Code Quality Grades**
- **A+ (95-100):** Exemplary, production-ready
- **A (90-94):** Excellent, minor improvements
- **A- (85-89):** Very good, some refinements needed
- **B+ (80-84):** Good, moderate improvements required
- **B (75-79):** Acceptable, significant improvements needed
- **B- (70-74):** Marginal, major refactoring required
- **C or lower:** Rejected, rewrite required

### **Velocity Tracking**
- Planned vs actual wave completion time
- Defect density (bugs per 1000 lines of code)
- Rework percentage (code rejected in review)
- Review efficiency (time to review vs code size)

---

## 📝 **DOCUMENTATION REQUIREMENTS**

### **PM Agent Deliverables**
- Phase plan (Markdown in docs/)
- Wave breakdown with task lists
- Progress tracking reports
- Risk and issue logs

### **cpp-qt-developer Deliverables**
- Inline code documentation (Doxygen)
- Implementation notes for reviewers
- Basic test cases
- Usage examples in headers

### **code-reviewer Deliverables**
- Wave review reports (short form)
- Subphase review reports (comprehensive)
- Issue tracking with severity levels
- Improvement recommendations

---

## 🔧 **TOOLS & INFRASTRUCTURE**

### **Build System**
```bash
# Clean build
cd build && rm -rf * && cmake .. && make -j4

# Run core validation tests
ctest --output-on-failure

# Build main application
./StreamDABAnalyser
```

### **Code Review Tools**
- Manual code inspection
- Static analysis (cppcheck, clang-tidy - optional)
- Valgrind for memory safety
- Qt Test framework for validation

### **Documentation**
- Markdown files in docs/ directory
- Doxygen inline comments in source
- Git commit messages following convention

---

## 🚀 **EXAMPLE: PHASE 3A EXECUTION**

### **Phase 3A: Audio & Data Services**

**PM Agent Planning:**
```markdown
## Subphase 1: Audio Stream Processing (2 weeks)

### Wave 1.1: DAB+ Stream Validator (3 days)
**cpp-qt-developer tasks:**
1. Create dabplus_stream_validator.hpp/cpp
2. Implement FireCode CRC validation
3. Implement Reed-Solomon FEC validation
4. Extract audio quality metrics
5. Add Qt signal integration

**Acceptance:** Parse DAB+ superframes per ETSI TS 102 563

### Wave 1.2: MOT Protocol Parser (4 days)
**cpp-qt-developer tasks:**
1. Create mot_protocol.hpp/cpp
2. Implement MSC Data Group parsing
3. Implement segment reassembly
4. Add content type detection

**Acceptance:** Parse MOT SlideShow images per ETSI EN 301 234

**Subphase 1 Review Trigger:** Both waves complete
```

**cpp-qt-developer Execution:**
- Implements Wave 1.1 over 3 days
- Reports completion with acceptance criteria met
- Waits for code review before starting Wave 1.2

**code-reviewer Check:**
- Reviews Wave 1.1 code (30-60 minutes)
- Grades: A+ (approved)
- cpp-qt-developer proceeds to Wave 1.2

**Cycle Repeats:**
- Wave 1.2 implemented → reviewed → approved
- Subphase 1 comprehensive review → approved
- Act phase: Improvements applied, next subphase planned

---

## 📈 **CONTINUOUS IMPROVEMENT**

### **Act Phase Activities**
1. **Apply Feedback:** Implement recommendations from reviews
2. **Process Refinement:** Adjust wave sizes based on velocity
3. **Risk Mitigation:** Address identified risks proactively
4. **Knowledge Sharing:** Document lessons learned
5. **Tool Improvement:** Enhance build/test infrastructure

### **Metrics-Driven Improvement**
- Track review rejection rate → adjust planning granularity
- Monitor bug density → improve developer practices
- Measure velocity → optimize wave sizing
- Assess rework percentage → refine acceptance criteria

---

## 🎓 **BEST PRACTICES**

### **For PM Agent:**
- Keep waves small (2-4 days max)
- Define clear, testable acceptance criteria
- Include ETSI specification references
- Plan for integration time between subphases

### **For cpp-qt-developer:**
- Implement exactly what's planned (no scope creep)
- Write self-documenting code with clear comments
- Test incrementally during development
- Ask questions early if requirements unclear

### **For code-reviewer:**
- Be constructive and specific in feedback
- Prioritize issues by severity
- Provide code examples in recommendations
- Balance thoroughness with efficiency

---

## 🔄 **WORKFLOW STATE TRANSITIONS**

```
PLANNED (PM) → ASSIGNED (cpp-qt-developer) → IN_PROGRESS (dev) 
    → REVIEW_READY (dev) → IN_REVIEW (code-reviewer) 
    → APPROVED/REJECTED (reviewer)
        → If APPROVED: MERGED → Plan next wave
        → If REJECTED: REWORK → IN_PROGRESS (dev)
```

---

## 📞 **ESCALATION PROCESS**

### **When to Escalate:**
- Review rejected twice (same issues)
- Acceptance criteria ambiguous/unachievable
- Dependencies blocking progress >2 days
- ETSI specification interpretation unclear

### **Escalation Path:**
1. Developer → PM Agent (clarify requirements)
2. Reviewer → PM Agent (adjust acceptance criteria)
3. PM Agent → Team discussion (resolve blockers)

---

## ✅ **WORKFLOW ACTIVATION CHECKLIST**

- [x] TDD framework removed from project
- [x] Simplified test system in place (4 core tests)
- [x] PDCA workflow documented
- [x] Agent roles clearly defined
- [ ] Phase 3A plan created (next step)
- [ ] First wave defined and ready for execution

---

**Document Status:** ✅ Complete  
**Next Action:** PM Agent defines Phase 3A detailed plan  
**Workflow Status:** ACTIVE

