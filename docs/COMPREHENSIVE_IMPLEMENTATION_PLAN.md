# Comprehensive Implementation Plan - StreamDAB Analyzer
## Addressing GUI Architecture + Phase 3B + Live Streaming

**Date:** October 25, 2025  
**Priority Order:** Option 2 (Phase 3B DLS+) → Option 3 (Live Stream) → Option 1 (Testing)  
**Critical Issue:** Current GUI does NOT follow GUI_PATTERNS.md recommendations  

---

## 🚨 **CRITICAL FINDINGS**

### **Current State Analysis**

**What's Working:**
- ✅ Automated GUI tests (39 tests, 100% pass rate)
- ✅ Basic ETI file loading
- ✅ Zero segmentation faults
- ✅ Build system stable

**What's NOT Working (Per User Report):**
> "some of GUI is no loaded"

**Root Cause:** Current `main.cpp` uses **simplified single-file architecture** that does NOT implement GUI_PATTERNS.md recommendations:

| Pattern | Recommended | Current | Status |
|---------|-------------|---------|--------|
| **Pattern 3** | Worker Thread + Event Loop | ❌ Missing | **CRITICAL** |
| **Pattern 5** | Shared Data Model (QMutex) | ❌ Missing | **CRITICAL** |
| **Pattern 7** | Specialized Signals | ❌ Missing | **HIGH** |
| **Pattern 8** | Update Throttling | ❌ Missing | **HIGH** |

**Impact:**
- GUI blocks during file processing (file read in main thread)
- No streaming capability (41 fps ETI frames)
- Widgets not loading because proper signal/slot architecture missing
- No live stream support infrastructure

---

## 📐 **REQUIRED ARCHITECTURE (Per GUI_PATTERNS.md)**

### **Pattern 3: Worker Thread + Event Loop**

```cpp
class ETIReaderThread : public QThread {
    Q_OBJECT
protected:
    void run() override {
        // Event loop for continuous processing
        QTimer *timer = new QTimer();
        connect(timer, &QTimer::timeout, this, &ETIReaderThread::processFrame);
        timer->start(24); // 41 fps
        exec(); // Start event loop
    }

signals:
    void frameReceived(quint32 frameNumber);
    void ficDataUpdated(QSharedPointer<FICData> data);
    void mscDataUpdated(QSharedPointer<MSCData> data);
};
```

### **Pattern 5: Shared Data Model**

```cpp
class SharedETIData : public QObject {
    Q_OBJECT
public:
    QSharedPointer<ETIFrame> getCurrentFrame() const {
        QMutexLocker locker(&frameMutex);
        return currentFrame;
    }
    
    void updateFrame(QSharedPointer<ETIFrame> frame) {
        QMutexLocker locker(&frameMutex);
        currentFrame = frame;
    }

signals:
    void frameChanged();

private:
    mutable QMutex frameMutex;
    QSharedPointer<ETIFrame> currentFrame;
};
```

### **Pattern 7: Specialized Signals**

```cpp
// BAD (current): Single generic signal
emit dataUpdated(QVariant data);

// GOOD (required): Specialized signals
emit ficDataUpdated(QSharedPointer<FICData> fic);
emit mscDataUpdated(QSharedPointer<MSCData> msc);
emit errorStatsUpdated(ErrorStats stats);
emit ensembleInfoChanged(EnsembleInfo info);
```

### **Pattern 8: Update Throttling**

```cpp
class FrameCounterWidget : public QWidget {
    QTimer updateTimer;
    updateTimer.setInterval(200); // Throttle to 5 fps
    
    void onFrameReceived(quint32 frame) {
        // Don't update immediately
        pendingUpdate = true;
    }
    
    void doUpdate() {
        if (pendingUpdate) {
            label->setText(QString::number(currentFrame));
            pendingUpdate = false;
        }
    }
};
```

---

## 🎯 **IMPLEMENTATION STRATEGY**

### **Approach: Incremental Refactoring**

We will **NOT rewrite main.cpp from scratch**. Instead, we'll implement in phases while maintaining backward compatibility:

### **Phase A: Core Infrastructure (Patterns 3 + 5)**
**Duration:** 1 week  
**Priority:** CRITICAL (enables all other features)  

**Deliverables:**
1. `src/core/eti_reader_thread.cpp/hpp` - Pattern 3 implementation
2. `src/core/shared_eti_data.cpp/hpp` - Pattern 5 implementation
3. Update `main.cpp` to use worker thread architecture
4. Test with existing ETI files (file mode)

**Why First:** This is the foundation for both Phase 3B (DLS+) and live streaming

---

### **Phase B: DLS+ Integration (Option 2)**
**Duration:** 2 weeks  
**Priority:** HIGH (user requested Option 2 first)  

**Deliverables:**
1. Complete `src/core/dls_plus_decoder.cpp/hpp` implementation
2. Integrate with `ETIReaderThread` for PAD extraction
3. Add "Now Playing" panel to GUI
4. Signal: `dlsPlusMetadataReceived(track, artist, album)`
5. Tests: 25+ DLS+ decoder tests

**Dependencies:** Requires Phase A worker thread

---

### **Phase C: Live Stream Input (Option 3)**
**Duration:** 1 week  
**Priority:** HIGH (user requested Option 3 second)  

**Deliverables:**
1. Complete `src/core/network_stream_receiver.cpp/hpp`
2. UDP multicast receiver (239.0.0.1:5004 typical)
3. ETI-over-IP support (41 fps streaming)
4. Input source selector: File / UDP / TCP
5. Connect to `ETIReaderThread`

**Dependencies:** Requires Phase A worker thread

---

### **Phase D: GUI Architecture Completion (Patterns 7 + 8)**
**Duration:** 1 week  
**Priority:** MEDIUM  

**Deliverables:**
1. Implement specialized signals in `ETIReaderThread`
2. Add throttling to all widgets (Pattern 8)
3. Optimize signal routing per GUI_PATTERNS.md table
4. Performance validation (<80% CPU at 41 fps)

**Dependencies:** Phases A, B, C

---

### **Phase E: Integration Testing (Option 1)**
**Duration:** 3 days  
**Priority:** VALIDATION  

**Deliverables:**
1. Test with real Thai ETI files
2. Test UDP live streaming
3. Test DLS+ metadata display
4. Performance benchmarks
5. User acceptance testing

---

## 📅 **TIMELINE (Total: 5-6 weeks)**

```
Week 1:    Phase A - Worker Thread Infrastructure
Week 2-3:  Phase B - DLS+ Implementation
Week 4:    Phase C - Live Streaming
Week 5:    Phase D - GUI Optimization
Week 6:    Phase E - Testing & Validation
```

---

## 🔧 **DETAILED IMPLEMENTATION TASKS**

### **Phase A: Core Infrastructure (Week 1)**

#### **Task A.1: Create ETIReaderThread (Pattern 3)**

**File:** `src/core/eti_reader_thread.hpp`

```cpp
#ifndef ETI_READER_THREAD_HPP
#define ETI_READER_THREAD_HPP

#include <QThread>
#include <QTimer>
#include <QMutex>
#include <memory>
#include "enhanced_eti_processor_qt.h"
#include "advanced_fig_analyzer.h"

enum class InputMode {
    File,        // Read from ETI file
    UDPStream,   // UDP multicast (live)
    TCPStream    // TCP connection (live)
};

class ETIReaderThread : public QThread {
    Q_OBJECT

public:
    explicit ETIReaderThread(QObject *parent = nullptr);
    ~ETIReaderThread() override;

    // Configuration
    void setInputMode(InputMode mode);
    void setFilePath(const QString &path);
    void setUDPEndpoint(const QString &address, quint16 port);
    void setFrameRate(int fps); // Default 41 fps

    // Control
    void startProcessing();
    void stopProcessing();
    void pauseProcessing();
    void resumeProcessing();

signals:
    // Pattern 7: Specialized signals
    void frameReceived(quint32 frameNumber);
    void ficDataUpdated(QSharedPointer<FICData> data);
    void mscDataUpdated(QSharedPointer<MSCData> data);
    void errorStatsUpdated(ErrorStats stats);
    void ensembleInfoChanged(EnsembleInfo info);
    void serviceListUpdated(QList<ServiceInfo> services);
    
    // Progress signals
    void processingStarted();
    void processingComplete(int totalFrames, int timeMs);
    void processingError(QString error);

protected:
    void run() override;

private slots:
    void processNextFrame();

private:
    void processFileFrame();
    void processUDPFrame();
    
    InputMode m_mode;
    QString m_filePath;
    QString m_udpAddress;
    quint16 m_udpPort;
    int m_frameIntervalMs; // 24ms for 41 fps
    
    QTimer *m_frameTimer;
    std::unique_ptr<EnhancedETIProcessorQt> m_processor;
    std::unique_ptr<AdvancedFIGAnalyzer> m_figAnalyzer;
    
    bool m_running;
    bool m_paused;
    QMutex m_controlMutex;
};

#endif // ETI_READER_THREAD_HPP
```

**File:** `src/core/eti_reader_thread.cpp`

```cpp
#include "eti_reader_thread.hpp"
#include <QFile>
#include <QDebug>

ETIReaderThread::ETIReaderThread(QObject *parent)
    : QThread(parent)
    , m_mode(InputMode::File)
    , m_udpPort(5004)
    , m_frameIntervalMs(24) // 41 fps
    , m_frameTimer(nullptr)
    , m_running(false)
    , m_paused(false)
{
    m_processor = std::make_unique<EnhancedETIProcessorQt>();
    m_figAnalyzer = std::make_unique<AdvancedFIGAnalyzer>();
}

ETIReaderThread::~ETIReaderThread()
{
    stopProcessing();
    wait(); // Wait for thread to finish
}

void ETIReaderThread::setInputMode(InputMode mode)
{
    QMutexLocker locker(&m_controlMutex);
    m_mode = mode;
}

void ETIReaderThread::setFilePath(const QString &path)
{
    QMutexLocker locker(&m_controlMutex);
    m_filePath = path;
}

void ETIReaderThread::setFrameRate(int fps)
{
    if (fps > 0) {
        m_frameIntervalMs = 1000 / fps;
    }
}

void ETIReaderThread::startProcessing()
{
    QMutexLocker locker(&m_controlMutex);
    m_running = true;
    m_paused = false;
    start(); // Start QThread
}

void ETIReaderThread::stopProcessing()
{
    QMutexLocker locker(&m_controlMutex);
    m_running = false;
}

void ETIReaderThread::run()
{
    // Pattern 3: Create event loop in worker thread
    m_frameTimer = new QTimer();
    m_frameTimer->setInterval(m_frameIntervalMs);
    
    connect(m_frameTimer, &QTimer::timeout, this, &ETIReaderThread::processNextFrame);
    
    emit processingStarted();
    
    m_frameTimer->start();
    
    // Start event loop (blocks until quit())
    exec();
    
    // Cleanup
    m_frameTimer->stop();
    delete m_frameTimer;
    m_frameTimer = nullptr;
}

void ETIReaderThread::processNextFrame()
{
    if (!m_running || m_paused) {
        return;
    }
    
    switch (m_mode) {
        case InputMode::File:
            processFileFrame();
            break;
        case InputMode::UDPStream:
            processUDPFrame();
            break;
        case InputMode::TCPStream:
            // TODO: Implement TCP
            break;
    }
}

void ETIReaderThread::processFileFrame()
{
    // TODO: Implement file reading logic
    // This will be completed in detailed implementation
    
    // Example structure:
    // 1. Read 6144 bytes from file
    // 2. Parse with m_processor
    // 3. Extract FIC/MSC data
    // 4. Emit specialized signals
    
    static quint32 frameCount = 0;
    frameCount++;
    
    emit frameReceived(frameCount);
}

void ETIReaderThread::processUDPFrame()
{
    // TODO: Implement UDP reading logic
    // This will be completed in Phase C
}
```

#### **Task A.2: Create SharedETIData (Pattern 5)**

**File:** `src/core/shared_eti_data.hpp`

```cpp
#ifndef SHARED_ETI_DATA_HPP
#define SHARED_ETI_DATA_HPP

#include <QObject>
#include <QMutex>
#include <QSharedPointer>
#include <QByteArray>
#include <QMap>
#include <QList>

// Data structures
struct ETIFrame {
    quint32 frameNumber;
    QByteArray rawData; // 6144 bytes
    quint8 dabMode;
    quint32 ficData[24]; // 96 bytes as 32-bit words
    QByteArray mscData;
    qint64 timestamp; // QDateTime::currentMSecsSinceEpoch()
};

struct FICData {
    quint16 ensembleId;
    QString ensembleLabel;
    QList<ServiceInfo> services;
    QMap<int, SubchannelConfig> subchannels;
};

struct MSCData {
    QMap<int, QByteArray> subchannelData;
    QVector<quint8> errorFlags;
};

struct ErrorStats {
    int frameErrors;
    int crcErrors;
    int syncLost;
};

// Shared data container with mutex protection
class SharedETIData : public QObject {
    Q_OBJECT

public:
    explicit SharedETIData(QObject *parent = nullptr);

    // Thread-safe getters
    QSharedPointer<ETIFrame> getCurrentFrame() const;
    QSharedPointer<FICData> getFICData() const;
    QSharedPointer<MSCData> getMSCData() const;
    ErrorStats getErrorStats() const;

    // Thread-safe setters (called by worker thread)
    void updateFrame(QSharedPointer<ETIFrame> frame);
    void updateFIC(QSharedPointer<FICData> fic);
    void updateMSC(QSharedPointer<MSCData> msc);
    void updateErrorStats(const ErrorStats &stats);

signals:
    void frameChanged();
    void ficChanged();
    void mscChanged();
    void errorStatsChanged();

private:
    mutable QMutex m_frameMutex;
    mutable QMutex m_ficMutex;
    mutable QMutex m_mscMutex;
    mutable QMutex m_statsMutex;

    QSharedPointer<ETIFrame> m_currentFrame;
    QSharedPointer<FICData> m_ficData;
    QSharedPointer<MSCData> m_mscData;
    ErrorStats m_errorStats;
};

#endif // SHARED_ETI_DATA_HPP
```

**File:** `src/core/shared_eti_data.cpp`

```cpp
#include "shared_eti_data.hpp"

SharedETIData::SharedETIData(QObject *parent)
    : QObject(parent)
{
    m_errorStats = {0, 0, 0};
}

QSharedPointer<ETIFrame> SharedETIData::getCurrentFrame() const
{
    QMutexLocker locker(&m_frameMutex);
    return m_currentFrame;
}

QSharedPointer<FICData> SharedETIData::getFICData() const
{
    QMutexLocker locker(&m_ficMutex);
    return m_ficData;
}

QSharedPointer<MSCData> SharedETIData::getMSCData() const
{
    QMutexLocker locker(&m_mscMutex);
    return m_mscData;
}

ErrorStats SharedETIData::getErrorStats() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_errorStats;
}

void SharedETIData::updateFrame(QSharedPointer<ETIFrame> frame)
{
    {
        QMutexLocker locker(&m_frameMutex);
        m_currentFrame = frame;
    }
    emit frameChanged();
}

void SharedETIData::updateFIC(QSharedPointer<FICData> fic)
{
    {
        QMutexLocker locker(&m_ficMutex);
        m_ficData = fic;
    }
    emit ficChanged();
}

void SharedETIData::updateMSC(QSharedPointer<MSCData> msc)
{
    {
        QMutexLocker locker(&m_mscMutex);
        m_mscData = msc;
    }
    emit mscChanged();
}

void SharedETIData::updateErrorStats(const ErrorStats &stats)
{
    {
        QMutexLocker locker(&m_statsMutex);
        m_errorStats = stats;
    }
    emit errorStatsChanged();
}
```

#### **Task A.3: Update main.cpp to Use New Architecture**

**Changes Required:**
1. Add `#include "core/eti_reader_thread.hpp"`
2. Add `#include "core/shared_eti_data.hpp"`
3. Create `SharedETIData` instance in `DABAnalyzerWindow`
4. Create `ETIReaderThread` instance
5. Connect signals to existing widgets
6. Replace file processing logic with worker thread

**Example Integration:**

```cpp
class DABAnalyzerWindow : public QMainWindow {
private:
    // Add new members
    SharedETIData *m_sharedData;
    ETIReaderThread *m_readerThread;

    void setupNewArchitecture() {
        // Create shared data
        m_sharedData = new SharedETIData(this);
        
        // Create worker thread
        m_readerThread = new ETIReaderThread(this);
        
        // Connect worker signals to shared data
        connect(m_readerThread, &ETIReaderThread::ficDataUpdated,
                this, [this](QSharedPointer<FICData> fic) {
                    m_sharedData->updateFIC(fic);
                });
        
        // Connect shared data changes to widgets
        connect(m_sharedData, &SharedETIData::ficChanged,
                this, &DABAnalyzerWindow::onFICDataChanged);
    }
};
```

---

### **Phase B: DLS+ Implementation (Weeks 2-3)**

**See:** `docs/PHASE_3B_DLS_PLUS_PLAN.md` for detailed plan

**Integration with Worker Thread:**
```cpp
// In ETIReaderThread::processFileFrame()
void ETIReaderThread::processFileFrame() {
    // ... parse frame ...
    
    // Extract PAD (Program Associated Data) from audio subchannel
    QByteArray padData = extractPADFromMSC(mscData, audioSubchannelId);
    
    // Pass to DLS+ decoder (runs in worker thread)
    m_dlsPlusDecoder->processPADData(padData);
    
    // DLS+ decoder emits nowPlayingUpdated() signal
}
```

---

### **Phase C: Live Stream Input (Week 4)**

**See:** `docs/LIVE_STREAM_INPUT_IMPLEMENTATION.md` (already exists)

**Key Components:**
1. UDP socket in `ETIReaderThread`
2. Multicast group joining
3. Packet buffering and reassembly
4. Jitter buffer (handle network delays)

---

## 🎯 **SUCCESS CRITERIA**

### **Phase A (Infrastructure)**
- ✅ `ETIReaderThread` runs in dedicated thread
- ✅ File processing doesn't block GUI
- ✅ `SharedETIData` passes thread-safety tests
- ✅ All existing widgets still work
- ✅ Zero regression (existing tests pass)

### **Phase B (DLS+)**
- ✅ DLS+ decoder parses ETSI TS 102 980 tags
- ✅ "Now Playing" panel updates in real-time
- ✅ Thai UTF-8 metadata displays correctly
- ✅ 25+ DLS+ tests passing

### **Phase C (Live Streaming)**
- ✅ UDP multicast receiver working
- ✅ 41 fps sustained throughput
- ✅ <50ms latency
- ✅ Input source selector functional

### **Phase D (Optimization)**
- ✅ CPU usage <80% at 41 fps
- ✅ All widgets throttled appropriately
- ✅ Specialized signals implemented
- ✅ Performance benchmarks met

### **Phase E (Testing)**
- ✅ Real Thai ETI files load correctly
- ✅ Live UDP stream works
- ✅ DLS+ metadata displays
- ✅ User acceptance complete

---

## 📊 **RESOURCE REQUIREMENTS**

### **Development Time**
- Phase A: 40 hours (1 week full-time)
- Phase B: 80 hours (2 weeks full-time)
- Phase C: 40 hours (1 week full-time)
- Phase D: 40 hours (1 week full-time)
- Phase E: 24 hours (3 days full-time)
**Total:** 224 hours (~6 weeks)

### **Agent Assignments**
1. **fullstack-backend-pro**: Worker thread architecture (Phase A)
2. **typescript-pro** or **python-pro**: DLS+ decoder (Phase B)
3. **devops-engineer**: Live streaming + network (Phase C)
4. **code-reviewer**: Architecture review after each phase
5. **fullstack-qa-tester**: Integration testing (Phase E)

---

## 🚀 **IMMEDIATE NEXT STEPS**

### **Step 1: Deploy Architecture Design Agent**
Create specialized agent to design the worker thread architecture:

```bash
# Deploy agent to create detailed architecture
Agent: fullstack-backend-pro
Task: Design ETIReaderThread and SharedETIData architecture
Output: Complete .hpp/.cpp files ready to integrate
```

### **Step 2: Implement Phase A (Week 1)**
Start with foundation that enables everything else:
- Day 1-2: `eti_reader_thread.hpp/cpp`
- Day 3-4: `shared_eti_data.hpp/cpp`
- Day 5: Integrate with `main.cpp`

### **Step 3: Validate with Tests**
Run existing automated tests + new worker thread tests

### **Step 4: Phase B (DLS+)**
Build on worker thread foundation

### **Step 5: Phase C (Live Streaming)**
Add network input source to worker thread

---

## ❓ **DECISION REQUIRED**

**Question for User:** Should we proceed with Phase A (Worker Thread Infrastructure) immediately?

**Options:**
1. **YES** - Deploy fullstack-backend-pro agent now to implement worker thread
2. **NO** - Review plan first, then deploy agents
3. **MODIFY** - Change priority order or approach

**Recommendation:** Option 1 (YES) - Start Phase A immediately as it's the foundation for both DLS+ and live streaming.

**What would you like to do?**

---

**Document Version:** 1.0  
**Created:** October 25, 2025  
**Status:** Awaiting User Decision  
**Next Action:** Deploy agent for Phase A implementation
