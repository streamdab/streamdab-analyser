# Phase A: Worker Thread Infrastructure - PDCA Plan

**Phase:** Phase A - Core Infrastructure (Patterns 3 + 5)  
**Duration:** Week 1 (5 days)  
**Methodology:** PDCA (Plan-Do-Check-Act)  
**Execution:** Parallel agents for maximum efficiency  

---

## 📋 **PLAN Phase**

### **Objectives**
1. Implement Pattern 3: ETIReaderThread with event loop
2. Implement Pattern 5: SharedETIData with mutex protection
3. Integrate with existing main.cpp without breaking current functionality
4. Enable foundation for Phase B (DLS+) and Phase C (Live Streaming)

### **Success Criteria**
- ✅ ETIReaderThread runs in dedicated QThread
- ✅ SharedETIData provides thread-safe access
- ✅ File processing moves to worker thread (GUI doesn't block)
- ✅ All existing 39 GUI tests still pass
- ✅ Zero compilation errors
- ✅ Zero segmentation faults
- ✅ Memory leak free (Valgrind clean)

### **Parallel Agent Assignment**

#### **Agent 1: fullstack-backend-pro**
**Task:** Implement ETIReaderThread (Pattern 3)  
**Deliverables:**
- `src/core/eti_reader_thread.hpp` (complete)
- `src/core/eti_reader_thread.cpp` (complete)
- File mode implementation (UDP stubbed for Phase C)
- Unit tests: `tests/test_eti_reader_thread.cpp`

**Key Requirements:**
- QThread subclass with event loop (exec())
- QTimer for frame processing (24ms interval = 41 fps)
- Specialized signals per Pattern 7
- Integration with EnhancedETIProcessorQt
- Integration with AdvancedFIGAnalyzer
- Thread-safe start/stop/pause controls

#### **Agent 2: python-pro**
**Task:** Implement SharedETIData (Pattern 5)  
**Deliverables:**
- `src/core/shared_eti_data.hpp` (complete)
- `src/core/shared_eti_data.cpp` (complete)
- Data structures (ETIFrame, FICData, MSCData, ErrorStats)
- Unit tests: `tests/test_shared_eti_data.cpp`

**Key Requirements:**
- QMutex protection for all shared data
- QSharedPointer for memory management
- Thread-safe getters/setters with QMutexLocker
- Signals for data changes (frameChanged, ficChanged, etc.)
- Zero-copy access pattern

#### **Agent 3: typescript-pro**
**Task:** Update main.cpp Integration  
**Deliverables:**
- Updated `src/main.cpp` with new architecture
- Connection setup between ETIReaderThread and SharedETIData
- Widget signal connections to shared data
- Backward compatibility with existing widgets

**Key Requirements:**
- Preserve existing widget structure
- Add SharedETIData instance
- Add ETIReaderThread instance
- Connect signals using Qt::QueuedConnection
- Replace blocking file processing with worker thread
- Maintain existing menu bar and UI layout

---

## 🚀 **DO Phase (Parallel Execution)**

### **Agent Deployment Strategy**

**Execution Mode:** Parallel (all agents work simultaneously)

**Agent 1 Focus:**
```
Priority: CRITICAL
File: eti_reader_thread.hpp/cpp
Time: 60 minutes estimated
Dependencies: None (can start immediately)
```

**Agent 2 Focus:**
```
Priority: CRITICAL
File: shared_eti_data.hpp/cpp
Time: 45 minutes estimated
Dependencies: None (can start immediately)
```

**Agent 3 Focus:**
```
Priority: HIGH
File: main.cpp integration
Time: 30 minutes estimated
Dependencies: Wait for Agent 1 & 2 headers
```

### **Coordination Points**

1. **T+0 min:** Deploy Agent 1 & 2 in parallel
2. **T+15 min:** Agent 1 & 2 provide header files (.hpp)
3. **T+15 min:** Deploy Agent 3 with header dependencies
4. **T+60 min:** All agents complete implementation
5. **T+75 min:** Build verification
6. **T+90 min:** Test execution

---

## ✅ **CHECK Phase**

### **Verification Checklist**

#### **Build Verification**
- [ ] Zero compilation errors
- [ ] Zero compilation warnings
- [ ] CMakeLists.txt updated correctly
- [ ] All includes resolve
- [ ] Qt MOC generation successful

#### **Test Verification**
- [ ] All existing 39 GUI tests pass (100%)
- [ ] New ETIReaderThread tests pass
- [ ] New SharedETIData tests pass
- [ ] Thread-safety tests pass
- [ ] Memory leak tests pass (Valgrind)

#### **Functional Verification**
- [ ] File loading works (GUI doesn't freeze)
- [ ] Frame counter updates
- [ ] Service list populates
- [ ] ETI Explorer shows statistics
- [ ] Frame viewer works without crashes
- [ ] FIG Extractor displays data

#### **Performance Verification**
- [ ] File processing < 100ms for 1000 frames
- [ ] CPU usage < 20% during file processing
- [ ] Memory usage < 100 MB
- [ ] No thread deadlocks
- [ ] Clean thread shutdown

---

## 🔄 **ACT Phase**

### **Success Path**
If all checks pass:
1. Commit Phase A changes with detailed message
2. Update CLAUDE.md with Phase A completion
3. Create Phase A completion report
4. **Move to Phase B (DLS+)** with new PDCA cycle

### **Failure Path**
If any checks fail:
1. Identify root cause
2. Deploy fix agent to address specific issue
3. Re-run CHECK phase
4. Iterate until all checks pass

### **Rollback Plan**
If critical issues found:
1. Git revert to pre-Phase A state
2. Document issues in `docs/PHASE_A_ISSUES.md`
3. Redesign approach
4. Restart PLAN phase with lessons learned

---

## 📊 **Progress Tracking**

### **PLAN Phase Metrics**
- [ ] Design document complete
- [ ] Agent tasks defined
- [ ] Success criteria established
- [ ] Parallel execution plan ready

### **DO Phase Metrics**
- [ ] Agent 1 deployed
- [ ] Agent 2 deployed
- [ ] Agent 3 deployed
- [ ] All agents completed

### **CHECK Phase Metrics**
- [ ] Build verification: PASS/FAIL
- [ ] Test verification: X/Y tests passing
- [ ] Functional verification: PASS/FAIL
- [ ] Performance verification: PASS/FAIL

### **ACT Phase Metrics**
- [ ] Changes committed
- [ ] Documentation updated
- [ ] Phase B ready to start

---

## 🎯 **Agent Prompts (Ready to Deploy)**

### **Agent 1 Prompt (fullstack-backend-pro)**

```
You are implementing Pattern 3 (Worker Thread + Event Loop) for a DAB/ETI stream analyzer.

TASK: Create ETIReaderThread class with event loop for continuous ETI frame processing.

REQUIREMENTS:
1. File: src/core/eti_reader_thread.hpp (header with full class definition)
2. File: src/core/eti_reader_thread.cpp (implementation)
3. QThread subclass with event loop using exec()
4. QTimer for 24ms frame processing (41 fps)
5. Specialized signals: frameReceived(), ficDataUpdated(), mscDataUpdated(), etc.
6. Integration with existing EnhancedETIProcessorQt
7. Integration with existing AdvancedFIGAnalyzer
8. Thread-safe controls: start, stop, pause, resume
9. File reading mode (UDP mode stub for future Phase C)
10. Unit tests: tests/test_eti_reader_thread.cpp

ARCHITECTURE REFERENCE:
Read /home/seksan/workspace/streamdab-analyzer/docs/GUI_PATTERNS.md Pattern 3

EXISTING CODE:
- src/core/enhanced_eti_processor_qt.h
- src/core/advanced_fig_analyzer.h

OUTPUT:
- Complete .hpp file with full class declaration
- Complete .cpp file with all methods implemented
- Unit test file with 15+ test cases
- CMakeLists.txt update snippet

CRITICAL: Follow Pattern 3 exactly as specified in GUI_PATTERNS.md
```

### **Agent 2 Prompt (python-pro)**

```
You are implementing Pattern 5 (Shared Data Model with Mutex) for a DAB/ETI stream analyzer.

TASK: Create SharedETIData class with thread-safe data access.

REQUIREMENTS:
1. File: src/core/shared_eti_data.hpp (header with data structures)
2. File: src/core/shared_eti_data.cpp (implementation)
3. QObject subclass for signal emission
4. QMutex protection for ALL shared data
5. QSharedPointer for memory management
6. Data structures: ETIFrame, FICData, MSCData, ErrorStats
7. Thread-safe getters with QMutexLocker
8. Thread-safe setters with QMutexLocker
9. Signals: frameChanged(), ficChanged(), mscChanged(), errorStatsChanged()
10. Unit tests: tests/test_shared_eti_data.cpp

ARCHITECTURE REFERENCE:
Read /home/seksan/workspace/streamdab-analyzer/docs/GUI_PATTERNS.md Pattern 5

CRITICAL REQUIREMENTS:
- Zero-copy access using QSharedPointer
- ALL data access MUST use QMutexLocker
- Signals emitted AFTER mutex unlock
- Thread-safety tests with concurrent readers/writers

OUTPUT:
- Complete .hpp file with all data structures
- Complete .cpp file with thread-safe implementation
- Unit test file with 20+ test cases including concurrency tests
- CMakeLists.txt update snippet
```

### **Agent 3 Prompt (typescript-pro)**

```
You are integrating Pattern 3 + 5 into existing Qt6 GUI application.

TASK: Update main.cpp to use ETIReaderThread and SharedETIData.

REQUIREMENTS:
1. Read existing src/main.cpp (current simplified architecture)
2. Add #include "core/eti_reader_thread.hpp"
3. Add #include "core/shared_eti_data.hpp"
4. Add SharedETIData *m_sharedData member to DABAnalyzerWindow
5. Add ETIReaderThread *m_readerThread member to DABAnalyzerWindow
6. Connect ETIReaderThread signals to SharedETIData updates
7. Connect SharedETIData changes to existing widget updates
8. Replace blocking analyzeFile() with m_readerThread->startProcessing()
9. Preserve ALL existing widget structure
10. Maintain backward compatibility

CRITICAL REQUIREMENTS:
- Do NOT break existing GUI widgets
- Use Qt::QueuedConnection for cross-thread signals
- Clean thread shutdown in destructor
- Error handling for file open failures

DEPENDENCIES:
Wait for Agent 1 & 2 to provide eti_reader_thread.hpp and shared_eti_data.hpp headers before starting implementation.

OUTPUT:
- Updated src/main.cpp with new architecture
- Preserve all existing functionality
- Add proper thread management
- Document all changes with comments
```

---

## 📅 **Timeline**

```
T+0:   Deploy Agent 1 & 2 (parallel)
T+15:  Agent 1 & 2 provide headers
T+15:  Deploy Agent 3
T+60:  All agents complete
T+75:  Build and test
T+90:  CHECK phase complete
T+105: ACT phase - commit or fix
```

**Total Duration:** 105 minutes (~2 hours for Phase A)

---

**Status:** PLAN Complete - Ready to Deploy Agents  
**Next Action:** Deploy Agent 1, 2, 3 in parallel
