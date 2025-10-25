# Phase B: Widget Migration to SharedETIData - PDCA Plan

**Phase:** Phase B - Widget Integration with SharedETIData (Pattern 5 Integration)  
**Duration:** 1-2 days  
**Methodology:** PDCA (Plan-Do-Check-Act)  
**Dependencies:** Phase A complete (ETIReaderThread + SharedETIData implemented)

---

## 📋 **PLAN Phase**

### **Objectives**
1. Connect ETIReaderThread signals to SharedETIData updates
2. Migrate existing widgets to read from SharedETIData instead of direct processor
3. Maintain backward compatibility during migration
4. Enable proper thread-safe data flow: Worker Thread → SharedETIData → GUI Widgets

### **Current State Analysis**

**What Works (Phase A Complete):**
- ✅ ETIReaderThread running in worker thread with QTimer event loop
- ✅ SharedETIData class with thread-safe getters/setters
- ✅ Both components instantiated in main.cpp
- ✅ Signals defined but not fully connected

**What Needs Migration:**
- ❌ Widgets still connect directly to old `EnhancedETIProcessorQt` signals
- ❌ ETIReaderThread doesn't update SharedETIData with processed data
- ❌ SharedETIData signals not connected to widgets
- ❌ Data flow incomplete: Worker → SharedETIData → Widgets

**Current Data Flow (Phase A - Incomplete):**
```
ETIReaderThread (worker) → [processes frames] → emits frameReceived(frameNum)
                                                  ↓ (not connected)
SharedETIData (main)     → [empty, not updated] → emits frameUpdated()
                                                  ↓ (not connected)  
Widgets (main)           → [still using old processor signals]
```

**Target Data Flow (Phase B - Complete):**
```
ETIReaderThread (worker) → processes frames → emits frameReceived()
                                              ↓ (queued connection)
                           → updates SharedETIData via setCurrentFrame()
                                              ↓
SharedETIData (main)     → thread-safe storage → emits frameUpdated()
                                              ↓ (direct connection)
Widgets (main)           → read via getters → update UI displays
```

### **Success Criteria**
- ✅ ETIReaderThread updates SharedETIData after each frame
- ✅ Service list widget displays services from SharedETIData
- ✅ ETI Explorer shows frame values from SharedETIData
- ✅ Frame list doesn't crash when clicking frames
- ✅ FIG Extractor shows FIG data from SharedETIData
- ✅ Zero compilation errors
- ✅ All existing widgets functional (fixes original GUI issues)
- ✅ Thread-safe operation validated

---

## 🎯 **IMPLEMENTATION TASKS**

### **Task B.1: Connect ETIReaderThread → SharedETIData**

**File:** `src/main.cpp` (setupWorkerThread method)

**Current Code (Phase A):**
```cpp
void setupWorkerThread() {
    connect(m_readerThread, &ETIReaderThread::frameReceived,
            this, [this](quint32 frameNumber) {
                qDebug() << "Phase A: Frame" << frameNumber << "received";
            });
}
```

**Target Code (Phase B):**
```cpp
void setupWorkerThread() {
    // Pattern 5: Worker thread updates shared data model
    connect(m_readerThread, &ETIReaderThread::frameReceived,
            this, [this](quint32 frameNumber) {
                // Update SharedETIData from worker thread
                // This uses queued connection (cross-thread safe)
                if (m_sharedData && m_etiProcessor) {
                    // Get latest processed frame from processor
                    auto frameData = m_etiProcessor->getCurrentFrameData();
                    
                    // Update shared data (thread-safe with mutex)
                    streamdab::core::ETIFrame etiFrame;
                    etiFrame.frame_number = frameNumber;
                    etiFrame.raw_data = frameData;
                    etiFrame.timestamp = QDateTime::currentDateTime();
                    // ... populate other fields from processor
                    
                    m_sharedData->setCurrentFrame(etiFrame);
                }
            });
    
    // Connect other ETIReaderThread signals
    connect(m_readerThread, &ETIReaderThread::ficDataUpdated,
            this, &DABAnalyzerWindow::onFICDataUpdated);
    
    connect(m_readerThread, &ETIReaderThread::mscDataUpdated,
            this, &DABAnalyzerWindow::onMSCDataUpdated);
            
    // ... connect other specialized signals
}
```

**Implementation Steps:**
1. Add slot methods to DABAnalyzerWindow for each signal type
2. Extract data from EnhancedETIProcessorQt
3. Convert to SharedETIData structures
4. Call SharedETIData setters (thread-safe)

---

### **Task B.2: Connect SharedETIData → Widgets**

**Widgets to Migrate:**
1. **Service List Widget** - Show DAB/DAB+ services
2. **ETI Explorer Widget** - Show frame header values
3. **Frame List Widget** - Show frame numbers without crashing
4. **FIG Extractor Widget** - Show FIG data

**Pattern for Each Widget:**
```cpp
// In constructor or setupWorkerThread():
connect(m_sharedData, &streamdab::core::SharedETIData::frameUpdated,
        this, [this](uint64_t frameNum) {
            // Read from SharedETIData (thread-safe getter)
            auto frame = m_sharedData->getCurrentFrame();
            
            // Update widget display
            updateFrameDisplay(frame);
        });

connect(m_sharedData, &streamdab::core::SharedETIData::serviceListUpdated,
        this, [this]() {
            auto services = m_sharedData->getServices();
            updateServiceList(services);
        });
```

---

### **Task B.3: Populate SharedETIData from ETIReaderThread**

**Location:** `src/core/eti_reader_thread.cpp` (processNextFrame method)

**Current Code:**
```cpp
void ETIReaderThread::processNextFrame() {
    // ... read frame ...
    
    if (m_processor && frameData.size() == ETI_FRAME_SIZE) {
        bool success = m_processor->processLiveFrame(frameData);
        if (success) {
            m_currentFrame++;
            m_framesProcessed++;
            emit frameReceived(m_currentFrame);
        }
    }
}
```

**Target Code:**
```cpp
void ETIReaderThread::processNextFrame() {
    // ... read frame ...
    
    if (m_processor && frameData.size() == ETI_FRAME_SIZE) {
        bool success = m_processor->processLiveFrame(frameData);
        if (success) {
            m_currentFrame++;
            m_framesProcessed++;
            
            // Emit specialized signals for SharedETIData population
            emit frameReceived(m_currentFrame);
            
            // Extract and emit FIC data
            // NOTE: We emit signals, main thread updates SharedETIData
            // This maintains clean separation: worker emits, main updates
        }
    }
}
```

**Note:** The worker thread should NOT directly call SharedETIData methods. Instead:
- Worker thread emits signals with data
- Main thread slot receives signal (queued connection)
- Main thread slot updates SharedETIData

This maintains thread ownership and prevents cross-thread direct calls.

---

### **Task B.4: Add Helper Methods to Extract Data**

**File:** `src/main.cpp`

**New Slot Methods:**
```cpp
private slots:
    void onFICDataUpdated(QSharedPointer<FICDataSignal> ficData);
    void onMSCDataUpdated(QSharedPointer<MSCData> mscData);
    void onErrorStatsUpdated(ErrorStats stats);
    void onEnsembleInfoChanged(EnsembleInfo info);
```

**Implementation:**
```cpp
void DABAnalyzerWindow::onFICDataUpdated(QSharedPointer<FICDataSignal> ficData) {
    if (!m_sharedData || !ficData) return;
    
    // Convert FICDataSignal to SharedETIData::FICData format
    streamdab::core::FICData sharedFicData;
    // ... populate fields ...
    
    m_sharedData->setFICData(sharedFicData);
}
```

---

## 🚀 **DO Phase (Implementation)**

### **Step-by-Step Execution**

**Step 1:** Modify setupWorkerThread() in main.cpp
- Add all signal connections from ETIReaderThread
- Create slot methods for each signal type
- Connect SharedETIData signals to widgets

**Step 2:** Implement slot methods
- onFICDataUpdated()
- onMSCDataUpdated()
- onErrorStatsUpdated()
- onEnsembleInfoChanged()

**Step 3:** Update widgets to read from SharedETIData
- Find all widgets that currently connect to m_etiProcessor signals
- Replace with connections to m_sharedData signals
- Use SharedETIData getters instead of direct processor access

**Step 4:** Test data flow
- Load ETI file
- Verify SharedETIData receives updates
- Verify widgets display correct data

---

## ✅ **CHECK Phase (Validation)**

### **Functional Tests**

1. **Service List Test**
   - Load ETI file with known services
   - Verify service list populates correctly
   - Check service names, types (DAB/DAB+)

2. **ETI Explorer Test**
   - Load ETI file
   - Verify frame header values display (ERR, FCT, NST, etc.)
   - Navigate through frames

3. **Frame List Test**
   - Load ETI file with 100 frames
   - Click on frame in list
   - Verify: No crash, frame details shown

4. **FIG Extractor Test**
   - Load ETI file with FIG data
   - Verify FIG types shown
   - Check FIG decoding results

### **Thread Safety Validation**

```bash
# Build with ThreadSanitizer
cd build
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" ..
make

# Run and check for data races
./streamdab-analyzer
# Load file, let it process 100+ frames
# Expected: No ThreadSanitizer warnings
```

### **Success Criteria Checklist**

- [ ] Service list widget displays services ✅
- [ ] ETI explorer shows frame values ✅
- [ ] Frame list doesn't crash on click ✅
- [ ] FIG Extractor shows FIG data ✅
- [ ] Zero ThreadSanitizer warnings ✅
- [ ] Zero compilation errors ✅
- [ ] CPU usage reasonable (<50% for file playback) ✅

---

## 🔄 **ACT Phase (Improvements)**

### **If All Tests Pass**

**Actions:**
1. Git commit with message: "Phase B: Widget migration to SharedETIData complete"
2. Update documentation with data flow diagrams
3. Move to Phase C (UDP live streaming)

### **If Tests Fail**

**Common Issues & Fixes:**

**Issue 1:** Widgets not updating
- **Cause:** Signals not connected properly
- **Fix:** Verify connections with qDebug in slots
- **Validation:** Check console for signal emissions

**Issue 2:** Thread safety warnings
- **Cause:** Direct access to SharedETIData from wrong thread
- **Fix:** Ensure all SharedETIData updates from main thread only
- **Validation:** ThreadSanitizer clean

**Issue 3:** Crash on widget access
- **Cause:** Null pointer or data race
- **Fix:** Check mutex protection, null checks
- **Validation:** Valgrind clean, no crashes

---

## 📊 **DELIVERABLES**

### **Code Changes**

1. **src/main.cpp**
   - Updated setupWorkerThread() with full signal connections
   - Added 4+ slot methods for data updates
   - Connected widgets to SharedETIData signals
   - Removed direct processor connections from widgets

2. **Documentation**
   - This PDCA plan document
   - Data flow diagram (optional but recommended)

### **Validation Results**

- Screenshot: Service list populated ✅
- Screenshot: ETI Explorer showing values ✅
- Screenshot: Frame list functional ✅
- Screenshot: FIG Extractor with data ✅
- ThreadSanitizer report: 0 warnings ✅

---

## 📋 **NEXT STEPS**

After Phase B completion:
1. **Phase C:** UDP live streaming implementation
2. **Phase D:** Specialized signals and throttling optimization
3. **Phase E:** DLS+ "Now Playing" metadata (original Phase 3B)

---

**Document Version:** 1.0  
**Created:** October 25, 2025  
**Status:** Ready for DO phase execution
