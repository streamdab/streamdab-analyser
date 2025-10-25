# Phase D: Specialized Signals and Throttling - PDCA Plan

**Phase:** Phase D - GUI Optimization (Patterns 7 + 8)  
**Duration:** 1-2 days  
**Methodology:** PDCA (Plan-Do-Check-Act)  
**Dependencies:** Phase A (ETIReaderThread) + Phase B (SharedETIData) + Phase C (UDP streaming)

---

## 📋 **PLAN Phase**

### **Objectives**
1. Implement Pattern 7: Specialized signals for different data layers
2. Implement Pattern 8: Widget throttling to prevent UI overload
3. Optimize signal routing based on update frequency requirements
4. Achieve <80% CPU usage at 41 fps real-time processing
5. Maintain responsive UI even under high data rates

### **Current State Analysis**

**What Works (Phases A-C Complete):**
- ✅ ETIReaderThread processing at 41 fps (24ms intervals)
- ✅ SharedETIData receiving frame updates
- ✅ Basic signal connections established
- ✅ UDP live streaming operational

**What Needs Optimization:**
- ❌ All widgets receive every frame update (inefficient)
- ❌ No throttling - widgets update at full 41 fps
- ❌ Signals not specialized by data layer
- ❌ Potential UI lag under sustained high frame rates
- ❌ CPU usage not optimized

**Current Signal Flow (Inefficient):**
```
ETIReaderThread → frameReceived(frameNum) [41 fps]
                  ↓
SharedETIData → frameUpdated(frameNum) [41 fps]
                  ↓
All Widgets → Update displays [41 fps] ← Too frequent!
```

**Target Signal Flow (Optimized):**
```
ETIReaderThread → Multiple specialized signals:
                  - frameCounterUpdate (5 fps - throttled)
                  - serviceListUpdate (on change only - rare)
                  - figDataUpdate (1 fps - throttled)
                  - errorStatsUpdate (0.5 fps - throttled)
                  ↓
SharedETIData → Layer-specific signals
                  ↓
Widgets → Throttled updates (appropriate rates)
```

### **GUI Patterns from GUI_PATTERNS.md**

#### **Pattern 7: Specialized Signals**

**Definition:** Emit different signals for different data layers instead of a single "dataUpdated" signal.

**Benefits:**
- Widgets subscribe only to relevant signals
- Reduces unnecessary processing
- Clear separation of concerns
- Easier debugging and maintenance

**Signal Categories by Update Frequency:**

| **Signal Type** | **Source Rate** | **Target Rate** | **Consumer Widgets** |
|----------------|-----------------|-----------------|---------------------|
| Frame Counter | 41 fps | 5 fps | Frame counter display |
| FIG Data | 41 fps | 1 fps | FIG extractor tree |
| Service List | On change | No throttle | Service tree |
| Error Stats | 41 fps | 0.5 fps | Error counter |
| Ensemble Info | Rare | No throttle | Ensemble info panel |
| MSC Data | 41 fps | 2 fps | Hex viewer |

#### **Pattern 8: Throttling**

**Definition:** Limit widget update frequency to prevent UI overload.

**Implementation:**
```cpp
class ThrottledWidget : public QWidget {
    QTimer updateTimer;
    bool pendingUpdate = false;
    
    ThrottledWidget() {
        updateTimer.setInterval(200);  // 5 fps
        updateTimer.setSingleShot(true);
        connect(&updateTimer, &QTimer::timeout, [this]() {
            if (pendingUpdate) {
                actuallyUpdateWidget();
                pendingUpdate = false;
            }
        });
    }
    
    void requestUpdate() {
        pendingUpdate = true;
        if (!updateTimer.isActive()) {
            updateTimer.start();
        }
    }
};
```

**Throttling Rates by Widget Type:**

| **Widget Type** | **Recommended Rate** | **Rationale** |
|----------------|---------------------|---------------|
| Frame counter | 5 fps (200ms) | Human perception limit |
| Service tree | On change only | Rarely changes |
| FIG extractor | 1 fps (1000ms) | Heavy rendering |
| Hex viewer | 2 fps (500ms) | Large data display |
| Error counter | 0.5 fps (2000ms) | Statistics accumulation |
| Constellation | 10 fps (100ms) | Visual smoothness |

### **Success Criteria**
- ✅ ETIReaderThread emits specialized signals (5+ types)
- ✅ Widgets implement throttling with QTimer
- ✅ CPU usage <80% at 41 fps sustained processing
- ✅ UI remains responsive (no lag or freezing)
- ✅ Frame rate maintained at ~41 fps
- ✅ Zero compilation errors
- ✅ All existing functionality preserved

---

## 🎯 **IMPLEMENTATION TASKS**

### **Task D.1: Add Specialized Signals to ETIReaderThread**

**File:** `src/core/eti_reader_thread.hpp`

**Add new signals:**
```cpp
signals:
    // Phase D: Pattern 7 - Specialized signals by data layer
    
    // High-frequency signals (throttled at source)
    void frameCounterUpdated(quint32 frameNumber);      // 5 fps
    void figDataUpdated(QSharedPointer<FIGData> figs);  // 1 fps
    void errorStatsUpdated(ErrorStats stats);           // 0.5 fps
    
    // Medium-frequency signals
    void mscDataUpdated(QSharedPointer<MSCData> data);  // 2 fps
    
    // Low-frequency signals (event-driven)
    void serviceListChanged(QList<ServiceInfo> services);  // On change
    void ensembleInfoChanged(EnsembleInfo info);           // On change
    
    // Existing signals (keep for compatibility)
    void frameReceived(quint32 frameNumber);  // 41 fps (internal use)
```

**Add throttling timers:**
```cpp
private:
    // Phase D: Throttling timers for specialized signals
    QElapsedTimer m_frameCounterThrottle;
    QElapsedTimer m_figDataThrottle;
    QElapsedTimer m_errorStatsThrottle;
    QElapsedTimer m_mscDataThrottle;
    
    static constexpr int FRAME_COUNTER_INTERVAL_MS = 200;  // 5 fps
    static constexpr int FIG_DATA_INTERVAL_MS = 1000;      // 1 fps
    static constexpr int ERROR_STATS_INTERVAL_MS = 2000;   // 0.5 fps
    static constexpr int MSC_DATA_INTERVAL_MS = 500;       // 2 fps
```

**Implementation in processNextFrame():**
```cpp
void ETIReaderThread::processNextFrame()
{
    // ... existing frame processing ...
    
    if (success) {
        m_currentFrame++;
        m_framesProcessed++;
        
        // Always emit internal frame received (for Phase B handlers)
        emit frameReceived(m_currentFrame);
        
        // Phase D: Emit throttled specialized signals
        
        // Frame counter (5 fps throttle)
        if (m_frameCounterThrottle.elapsed() >= FRAME_COUNTER_INTERVAL_MS) {
            emit frameCounterUpdated(m_currentFrame);
            m_frameCounterThrottle.restart();
        }
        
        // FIG data (1 fps throttle)
        if (m_figDataThrottle.elapsed() >= FIG_DATA_INTERVAL_MS) {
            auto figData = extractFIGData();
            if (figData) {
                emit figDataUpdated(figData);
            }
            m_figDataThrottle.restart();
        }
        
        // Error stats (0.5 fps throttle)
        if (m_errorStatsThrottle.elapsed() >= ERROR_STATS_INTERVAL_MS) {
            emit errorStatsUpdated(m_errorStats);
            m_errorStatsThrottle.restart();
        }
        
        // MSC data (2 fps throttle)
        if (m_mscDataThrottle.elapsed() >= MSC_DATA_INTERVAL_MS) {
            auto mscData = extractMSCData();
            if (mscData) {
                emit mscDataUpdated(mscData);
            }
            m_mscDataThrottle.restart();
        }
    }
}
```

---

### **Task D.2: Implement Widget Throttling**

**Create Base Throttled Widget Class:**

**File:** `src/gui/throttled_widget_base.hpp` (new file)

```cpp
#ifndef THROTTLED_WIDGET_BASE_HPP
#define THROTTLED_WIDGET_BASE_HPP

#include <QWidget>
#include <QTimer>

/**
 * @brief Pattern 8: Base class for throttled widget updates
 * 
 * Prevents UI overload by limiting update frequency.
 * Widgets inherit from this and implement actualUpdate().
 */
class ThrottledWidgetBase : public QWidget {
    Q_OBJECT
    
public:
    explicit ThrottledWidgetBase(int intervalMs, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_updateInterval(intervalMs)
        , m_pendingUpdate(false)
    {
        m_updateTimer.setSingleShot(true);
        connect(&m_updateTimer, &QTimer::timeout, this, &ThrottledWidgetBase::performUpdate);
    }
    
    void setUpdateInterval(int ms) {
        m_updateInterval = ms;
    }
    
protected:
    /**
     * @brief Request an update (may be throttled)
     * 
     * Call this instead of directly updating the widget.
     * The actual update will happen after throttle interval.
     */
    void requestUpdate() {
        m_pendingUpdate = true;
        
        if (!m_updateTimer.isActive()) {
            m_updateTimer.start(m_updateInterval);
        }
    }
    
    /**
     * @brief Override this to implement actual widget update
     * 
     * This is called after throttle interval has elapsed.
     */
    virtual void actualUpdate() = 0;
    
private slots:
    void performUpdate() {
        if (m_pendingUpdate) {
            actualUpdate();
            m_pendingUpdate = false;
        }
    }
    
private:
    QTimer m_updateTimer;
    int m_updateInterval;
    bool m_pendingUpdate;
};

#endif // THROTTLED_WIDGET_BASE_HPP
```

**Example Usage in main.cpp:**

```cpp
// Phase D: Connect throttled specialized signals to widgets

// Frame counter widget (5 fps)
connect(m_readerThread, &ETIReaderThread::frameCounterUpdated,
        this, [this](quint32 frameNum) {
            // Update frame counter display (already throttled at source)
            if (m_frameCounterLabel) {
                m_frameCounterLabel->setText(QString("Frame: %1").arg(frameNum));
            }
        });

// FIG data widget (1 fps)
connect(m_readerThread, &ETIReaderThread::figDataUpdated,
        this, [this](QSharedPointer<FIGData> figs) {
            // Update FIG extractor tree (already throttled at source)
            updateFIGExtractorDisplay(figs);
        });

// Error stats widget (0.5 fps)
connect(m_readerThread, &ETIReaderThread::errorStatsUpdated,
        this, [this](ErrorStats stats) {
            // Update error counter display (already throttled at source)
            updateErrorStatsDisplay(stats);
        });

// Service list widget (on change only - not throttled)
connect(m_readerThread, &ETIReaderThread::serviceListChanged,
        this, [this](QList<ServiceInfo> services) {
            // Update service tree (rare event)
            updateServiceTreeDisplay(services);
        });
```

---

### **Task D.3: Optimize Signal Routing**

**Update setupWorkerThread() in main.cpp:**

Replace high-frequency frameUpdated connections with specialized signals:

**Before (Phase B - Inefficient):**
```cpp
connect(m_sharedData, &streamdab::core::SharedETIData::frameUpdated,
        this, [this](uint64_t frameNum) {
            // Called 41 times per second!
            qDebug() << "Phase B: Frame" << frameNum << "updated";
        });
```

**After (Phase D - Optimized):**
```cpp
// Remove high-frequency frameUpdated connection

// Add throttled specialized connections
connect(m_readerThread, &ETIReaderThread::frameCounterUpdated,
        this, &DABAnalyzerWindow::onFrameCounterUpdate);  // 5 fps

connect(m_readerThread, &ETIReaderThread::figDataUpdated,
        this, &DABAnalyzerWindow::onFIGDataUpdate);  // 1 fps

connect(m_readerThread, &ETIReaderThread::errorStatsUpdated,
        this, &DABAnalyzerWindow::onErrorStatsUpdate);  // 0.5 fps
```

---

## 🚀 **DO Phase (Implementation)**

### **Step-by-Step Execution**

**Step 1:** Update `eti_reader_thread.hpp`
- Add specialized signal declarations (6 new signals)
- Add throttling timer members (4 timers)
- Add throttling interval constants

**Step 2:** Update `eti_reader_thread.cpp`
- Initialize throttling timers in constructor
- Modify `processNextFrame()` to emit throttled signals
- Keep existing signals for backward compatibility

**Step 3:** Update `main.cpp`
- Replace high-frequency connections with throttled ones
- Connect specialized signals to appropriate widgets
- Remove or comment out unthrottled frameUpdated connections

**Step 4:** (Optional) Create ThrottledWidgetBase
- Create new base class for throttled widgets
- Migrate existing widgets to inherit from it
- Implement actualUpdate() methods

**Step 5:** Build and test
- Compile with zero errors
- Verify signals emit at correct rates
- Measure CPU usage

---

## ✅ **CHECK Phase (Validation)**

### **Performance Tests**

**Test 1: Signal Emission Rates**
```cpp
// Add debug logging to verify throttling
void ETIReaderThread::processNextFrame() {
    static int frameCounterEmissions = 0;
    static int figDataEmissions = 0;
    
    if (m_frameCounterThrottle.elapsed() >= FRAME_COUNTER_INTERVAL_MS) {
        emit frameCounterUpdated(m_currentFrame);
        frameCounterEmissions++;
        if (frameCounterEmissions % 50 == 0) {
            qDebug() << "Phase D: frameCounterUpdated emitted 50 times";
        }
        m_frameCounterThrottle.restart();
    }
}
```

**Expected Results:**
- frameCounterUpdated: ~5 emissions/sec
- figDataUpdated: ~1 emission/sec
- errorStatsUpdated: ~0.5 emissions/sec
- serviceListChanged: Only on actual changes

**Test 2: CPU Usage**
```bash
# Run application with ETI file or UDP stream
./DABAnalyzer_Enhanced_FilteringBuild

# In another terminal, monitor CPU
top -p $(pgrep DABAnalyzer)

# Expected: CPU usage <80% during sustained 41 fps processing
```

**Test 3: UI Responsiveness**
- Load large ETI file (1000+ frames)
- Enable UDP streaming at 41 fps
- Interact with GUI (click widgets, navigate menus)
- Expected: No lag, immediate response to clicks

**Test 4: Frame Rate Consistency**
```bash
# Check processing rate in application logs
grep "Phase B: Worker thread completed" console.log

# Expected: ~41 fps sustained (1000 frames in ~24 seconds)
```

### **Success Criteria Checklist**

- [ ] ETIReaderThread emits 6+ specialized signals ✅
- [ ] Throttling timers implemented (4 timers) ✅
- [ ] Signal emission rates verified (5 fps, 1 fps, 0.5 fps) ✅
- [ ] CPU usage <80% at 41 fps ✅
- [ ] UI remains responsive (no freezing) ✅
- [ ] Frame rate maintained (~41 fps) ✅
- [ ] Zero compilation errors ✅
- [ ] All Phase A-C functionality preserved ✅

---

## 🔄 **ACT Phase (Improvements)**

### **If All Tests Pass**

**Actions:**
1. Git commit: "Phase D: Specialized signals and throttling complete"
2. Create PHASE_D_COMPLETION_SUMMARY.md
3. Measure performance improvements vs Phase C
4. Document CPU usage reduction
5. Prepare for final integration testing

### **If Performance Issues Remain**

**Common Issues & Fixes:**

**Issue 1:** CPU still >80%
- **Cause:** Widget rendering too heavy
- **Fix:** Reduce throttling rates further (e.g., 1 fps → 0.5 fps)
- **Fix:** Optimize widget paint events (use cached pixmaps)

**Issue 2:** UI lag persists
- **Cause:** Main thread blocked by processing
- **Fix:** Ensure all heavy processing in worker thread
- **Fix:** Use QMetaObject::invokeMethod with Qt::QueuedConnection

**Issue 3:** Signals emit too frequently
- **Cause:** Throttle timers not working
- **Fix:** Verify QElapsedTimer initialization
- **Fix:** Check throttle interval constants

**Issue 4:** Missing data in widgets
- **Cause:** Over-throttling (update rate too slow)
- **Fix:** Increase specific signal rate
- **Fix:** Add "on change" detection for important data

---

## 📊 **DELIVERABLES**

### **Code Changes**

1. **src/core/eti_reader_thread.hpp** (~15 lines)
   - 6 new specialized signal declarations
   - 4 throttling timer members
   - 4 throttling interval constants

2. **src/core/eti_reader_thread.cpp** (~60 lines)
   - Modified processNextFrame() with throttling logic
   - Initialized throttling timers
   - Emit specialized signals at appropriate rates

3. **src/main.cpp** (~40 lines)
   - Updated setupWorkerThread() connections
   - Replaced high-frequency signals with throttled ones
   - Added specialized signal handlers

4. **src/gui/throttled_widget_base.hpp** (optional, ~60 lines)
   - New base class for throttled widgets
   - QTimer-based update mechanism
   - Virtual actualUpdate() method

### **Documentation**

- This PDCA plan document
- Phase D completion summary (at ACT phase)
- Performance measurement results
- CPU usage comparison (before/after)

### **Performance Metrics**

**Target Improvements:**
- Signal emissions: 41 fps → 5 fps average (87% reduction)
- CPU usage: Baseline → <80% sustained
- UI responsiveness: Measurably improved
- Frame processing: Maintained at 41 fps

---

## 📋 **NEXT STEPS**

After Phase D completion:

**Integration & Testing:**
1. End-to-end testing with real ETI files
2. UDP live streaming performance validation
3. Multi-hour stability testing
4. Memory leak detection (Valgrind)

**Future Enhancements (Post Phase D):**
1. **DLS+ "Now Playing"** - Original Phase 3B feature
2. **Advanced visualization** - Constellation diagrams, spectrum
3. **Export capabilities** - JSON, CSV, XML reports
4. **Professional packaging** - Installer, documentation

---

**Document Version:** 1.0  
**Created:** October 25, 2025  
**Status:** Ready for DO phase execution
