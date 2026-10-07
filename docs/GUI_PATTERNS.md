# CLAUDE.md - DAB+ Stream Analyzer Architecture Guide

## Project Overview
Building a professional-grade ETSI DAB+ stream packet analyzer (similar to Elecard Stream Analyzer and DAB-XPlorer) using Qt6 C++. The application needs to parse and display ETI (Ensemble Transport Interface) frames in real-time with multiple synchronized views.

## Stream Characteristics
- **Protocol**: ETSI DAB+ ETI frames
- **Frame Size**: 6144 bytes per frame
- **Frame Rate**: ~41 fps (24ms per frame)
- **Data Layers**: FIC (Fast Information Channel), MSC (Main Service Channel), Subchannels
- **Input Sources**: UDP stream, file playback
- **Parsing**: Multi-layer protocol with high CPU requirements

---

## Architecture Patterns Reference

### What are the Patterns?

#### Pattern 1: Signal/Slot with Qt::QueuedConnection
**Definition**: Qt's standard mechanism for thread-safe communication between objects across threads.

**How it works**:
```cpp
// In worker thread
emit dataReady(result);

// In main thread (widget)
connect(worker, &Worker::dataReady, this, &Widget::updateDisplay, Qt::QueuedConnection);
```

**Characteristics**:
- Thread-safe by default when crossing thread boundaries
- Uses Qt's event queue (adds latency)
- Simple to implement
- Automatic marshalling of arguments

**When to use**: Simple, infrequent updates between threads

---

#### Pattern 2: QMetaObject::invokeMethod()
**Definition**: Direct method invocation across threads with explicit control over execution mode.

**How it works**:
```cpp
// From any thread, call method on main thread object
QMetaObject::invokeMethod(widget, "updateData", 
    Qt::QueuedConnection,
    Q_ARG(QString, data));

// Or blocking call
QMetaObject::invokeMethod(widget, "getData", 
    Qt::BlockingQueuedConnection,
    Q_RETURN_ARG(QString, result));
```

**Characteristics**:
- More control than signal/slot
- Can be blocking or non-blocking
- Useful for one-off calls
- Requires explicit connection type

**When to use**: Direct control needed, mixed blocking/non-blocking scenarios

---

#### Pattern 3: Worker Thread + Event Loop
**Definition**: Dedicated QThread with event loop for continuous processing, emitting signals to UI.

**How it works**:
```cpp
class ETIReaderThread : public QThread {
    Q_OBJECT
protected:
    void run() override {
        // Setup
        QTimer *timer = new QTimer();
        connect(timer, &QTimer::timeout, this, &ETIReaderThread::processFrame);
        timer->start(24); // 41 fps
        
        exec(); // Start event loop
    }
    
private slots:
    void processFrame() {
        // Read and parse ETI frame
        ETIFrame frame = readFrame();
        
        // Emit multiple signals
        emit frameReceived(frame.number);
        emit ficUpdated(frame.fic);
        emit mscUpdated(frame.msc);
    }
    
signals:
    void frameReceived(quint32 number);
    void ficUpdated(QSharedPointer<FICData> data);
    void mscUpdated(QSharedPointer<MSCData> data);
};

// Usage in main
thread = new ETIReaderThread();
connect(thread, &ETIReaderThread::frameReceived, widget, &Widget::onFrame);
thread->start();
```

**Characteristics**:
- Dedicated thread with event loop
- Can process continuous streams
- Separates processing from UI completely
- Professional architecture for streaming applications

**When to use**: Continuous data streams (audio, video, network protocols)

---

#### Pattern 4: QFuture + QtConcurrent
**Definition**: High-level API for running functions asynchronously using thread pool.

**How it works**:
```cpp
#include <QtConcurrent>

// Run function in thread pool
QFuture<Result> future = QtConcurrent::run([]() {
    return processLargeData();
});

// Monitor with watcher
QFutureWatcher<Result> *watcher = new QFutureWatcher<Result>();
connect(watcher, &QFutureWatcher<Result>::finished, [=]() {
    Result r = watcher->result();
    updateUI(r);
});
watcher->setFuture(future);
```

**Characteristics**:
- Automatic thread pool management
- Modern, high-level API
- Best for batch/one-shot operations
- Not suitable for continuous streams

**When to use**: File processing, batch analysis, one-time computations

---

#### Pattern 5: Shared Data Model with Mutex
**Definition**: Thread-safe shared data structure accessed by multiple consumers.

**How it works**:
```cpp
// Shared data container
class SharedETIData {
public:
    QSharedPointer<ETIFrame> getFrame() {
        QMutexLocker locker(&mutex);
        return currentFrame;
    }
    
    void setFrame(QSharedPointer<ETIFrame> frame) {
        QMutexLocker locker(&mutex);
        currentFrame = frame;
        emit frameChanged();
    }
    
signals:
    void frameChanged();
    
private:
    QSharedPointer<ETIFrame> currentFrame;
    QMutex mutex;
};

// Producer (worker thread)
sharedData->setFrame(newFrame);

// Consumers (widgets on main thread)
connect(sharedData, &SharedETIData::frameChanged, [=]() {
    auto frame = sharedData->getFrame();
    displayFrame(frame);
});
```

**Characteristics**:
- Zero-copy data sharing
- Decouples producer from consumers
- Requires careful locking
- Efficient for large data
- QSharedPointer handles memory management

**When to use**: Large data (buffers, images) shared by multiple widgets

---

#### Pattern 6: Producer-Consumer with Thread-Safe Queue
**Definition**: Buffered communication using thread-safe queue between producer and consumer.

**How it works**:
```cpp
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

class FrameQueue {
public:
    void enqueue(ETIFrame frame) {
        QMutexLocker locker(&mutex);
        if (queue.size() < maxSize) {
            queue.enqueue(frame);
            condition.wakeOne();
        }
    }
    
    ETIFrame dequeue() {
        QMutexLocker locker(&mutex);
        while (queue.isEmpty()) {
            condition.wait(&mutex);
        }
        return queue.dequeue();
    }
    
private:
    QQueue<ETIFrame> queue;
    QMutex mutex;
    QWaitCondition condition;
    int maxSize = 100;
};

// Producer thread
void run() {
    while (running) {
        ETIFrame frame = readFrame();
        frameQueue->enqueue(frame);
    }
}

// Consumer (main thread with timer)
void onTimer() {
    if (!frameQueue->isEmpty()) {
        ETIFrame frame = frameQueue->dequeue();
        processFrame(frame);
    }
}
```

**Characteristics**:
- Buffers data between threads
- Handles speed mismatches
- Adds latency (buffering delay)
- Controls update rate
- Prevents data loss

**When to use**: When producer/consumer speeds differ, packet processing

---

#### Pattern 7: Multiple Specialized Signals
**Definition**: Emit separate signals for different data types/layers instead of single generic signal.

**How it works**:
```cpp
// BAD: Single generic signal
class Worker : public QObject {
signals:
    void dataUpdated(QVariant data); // All widgets get everything
};

// GOOD: Specialized signals
class ETIWorker : public QObject {
signals:
    // Different signals for different data types
    void ficDataUpdated(QSharedPointer<FICData> fic);
    void mscDataUpdated(QSharedPointer<MSCData> msc);
    void errorStatsUpdated(ErrorStats stats);
    void ensembleInfoChanged(EnsembleInfo info);
};

// Widgets subscribe only to relevant signals
connect(worker, &ETIWorker::ficDataUpdated, treeWidget, &TreeWidget::updateFIC);
connect(worker, &ETIWorker::mscDataUpdated, hexWidget, &HexWidget::updateMSC);
connect(worker, &ETIWorker::errorStatsUpdated, statsWidget, &StatsWidget::updateStats);
```

**Characteristics**:
- Selective subscription by widgets
- Reduces unnecessary processing
- Clear separation of concerns
- Better performance
- More signals to maintain

**When to use**: Multi-layer protocols, complex applications with many widgets

---

#### Pattern 8: Update Throttling
**Definition**: Limit widget update frequency to prevent UI overload.

**How it works**:
```cpp
class ThrottledWidget : public QWidget {
public:
    ThrottledWidget() {
        updateTimer.setSingleShot(false);
        updateTimer.setInterval(33); // 30 fps
        connect(&updateTimer, &QTimer::timeout, this, &ThrottledWidget::doUpdate);
        updateTimer.start();
    }
    
    void onDataChanged() {
        // Don't update immediately, just mark dirty
        pendingUpdate = true;
    }
    
private slots:
    void doUpdate() {
        if (pendingUpdate) {
            // Actually update widget
            repaint();
            pendingUpdate = false;
        }
    }
    
private:
    QTimer updateTimer;
    bool pendingUpdate = false;
};

// Alternative: QElapsedTimer for timestamp checking
void onDataArrived() {
    if (lastUpdate.elapsed() > 33) { // Max 30 fps
        updateDisplay();
        lastUpdate.restart();
    }
}
```

**Characteristics**:
- Limits UI refresh rate
- Saves CPU for rendering
- May skip intermediate updates
- User-configurable rates

**When to use**: High-frequency data sources, heavy rendering widgets

---

## Recommended Architecture: Pattern 3 + 5 + 7 + 8

### Layer 1: Input & Parsing (Pattern 3)
**Component**: `ETIReaderThread`

**Responsibilities**:
- Read ETI frames from UDP/file at 41 fps
- Parse frame structure (SYNC, LIDATA, FC, NST, EOH, MST, EOF)
- Decode FIC and MSC data
- Run in dedicated thread with event loop

**Implementation sketch**:
```cpp
class ETIReaderThread : public QThread {
    Q_OBJECT
    
public:
    explicit ETIReaderThread(QObject *parent = nullptr);
    void setInputSource(const QString &source); // UDP or file path
    void stop();
    
protected:
    void run() override;
    
private slots:
    void processNextFrame();
    void handleTimeout();
    
signals:
    // Pattern 7: Specialized signals
    void etiFrameReceived(quint32 frameNumber);
    void ficDataUpdated(QSharedPointer<FICData> fic);
    void mscDataUpdated(QSharedPointer<MSCData> msc);
    void subchannelConfigChanged(int subchId, SubchannelConfig config);
    void errorStatsUpdated(ErrorStats stats);
    void ensembleInfoChanged(EnsembleInfo info);
    void serviceListUpdated(QList<ServiceInfo> services);
    
private:
    void parseETIFrame(const QByteArray &rawFrame);
    void decodeFIC(const QByteArray &ficData);
    void decodeMSC(const QByteArray &mscData);
    
    QTimer *frameTimer;
    InputSource *inputSource; // UDP or File reader
    ETIParser *parser;
    bool running;
};
```

---

### Layer 2: Data Storage (Pattern 5)
**Component**: Shared data containers with mutex protection

**Data structures**:
```cpp
// Shared current frame data
class SharedETIData : public QObject {
    Q_OBJECT
    
public:
    // Thread-safe getters
    QSharedPointer<ETIFrame> getCurrentFrame() const {
        QMutexLocker locker(&frameMutex);
        return currentFrame;
    }
    
    QSharedPointer<FICDatabase> getFICData() const {
        QMutexLocker locker(&ficMutex);
        return ficDatabase;
    }
    
    QSharedPointer<MSCData> getMSCData() const {
        QMutexLocker locker(&mscMutex);
        return mscData;
    }
    
    // Thread-safe setters (called by worker thread)
    void updateFrame(QSharedPointer<ETIFrame> frame) {
        QMutexLocker locker(&frameMutex);
        currentFrame = frame;
    }
    
    void updateFIC(QSharedPointer<FICDatabase> fic) {
        QMutexLocker locker(&ficMutex);
        ficDatabase = fic;
    }
    
    void updateMSC(QSharedPointer<MSCData> msc) {
        QMutexLocker locker(&mscMutex);
        mscData = msc;
    }
    
private:
    mutable QMutex frameMutex;
    mutable QMutex ficMutex;
    mutable QMutex mscMutex;
    
    QSharedPointer<ETIFrame> currentFrame;
    QSharedPointer<FICDatabase> ficDatabase;
    QSharedPointer<MSCData> mscData;
};

// Data structures
struct ETIFrame {
    quint32 frameNumber;
    QByteArray rawData; // 6144 bytes
    quint8 ficData[96];
    QByteArray mscData;
    // ... other fields
};

struct FICDatabase {
    EnsembleInfo ensemble;
    QList<ServiceInfo> services;
    QMap<int, SubchannelConfig> subchannels;
};

struct MSCData {
    QMap<int, QByteArray> subchannelData;
    QVector<quint8> errorFlags;
};
```

**Why shared data model**:
- Multiple widgets need access to same frame data
- Hex view shows raw bytes
- Tree view shows parsed structure
- Graphs show statistics
- Zero-copy access with QSharedPointer

---

### Layer 3: Widget Updates (Pattern 7 + 8)

#### Signal Routing Table

| Signal | Update Rate | Throttle To | Connected Widgets |
|--------|-------------|-------------|-------------------|
| `etiFrameReceived()` | 41 fps | 5 fps | Frame counter, status bar |
| `ficDataUpdated()` | ~1 fps | 1 fps | Service tree, ensemble info |
| `mscDataUpdated()` | 41 fps | 30 fps | Hex view, subchannel tabs |
| `subchannelConfigChanged()` | On change | No throttle | Configuration panel |
| `errorStatsUpdated()` | 1 fps | 0.5 fps | Error counter widget |
| `ensembleInfoChanged()` | Rare | No throttle | Ensemble info panel |
| `serviceListUpdated()` | Rare | No throttle | Service tree widget |

#### Widget Implementation Examples

**Example 1: Frame Counter (throttled)**
```cpp
class FrameCounterWidget : public QWidget {
    Q_OBJECT
    
public:
    FrameCounterWidget(QWidget *parent = nullptr) : QWidget(parent) {
        // Pattern 8: Throttle to 5 fps
        updateTimer.setInterval(200); // 5 fps
        updateTimer.setSingleShot(false);
        connect(&updateTimer, &QTimer::timeout, this, &FrameCounterWidget::doUpdate);
        updateTimer.start();
    }
    
public slots:
    void onFrameReceived(quint32 frameNumber) {
        // Don't update immediately, just store
        currentFrame = frameNumber;
        pendingUpdate = true;
    }
    
private slots:
    void doUpdate() {
        if (pendingUpdate) {
            label->setText(QString("Frame: %1").arg(currentFrame));
            pendingUpdate = false;
        }
    }
    
private:
    QTimer updateTimer;
    quint32 currentFrame = 0;
    bool pendingUpdate = false;
    QLabel *label;
};
```

**Example 2: Service Tree (slow updates)**
```cpp
class ServiceTreeWidget : public QTreeWidget {
    Q_OBJECT
    
public:
    ServiceTreeWidget(QWidget *parent = nullptr) : QTreeWidget(parent) {
        // Pattern 8: Very slow updates (1 fps) - metadata changes rarely
        updateTimer.setInterval(1000);
        updateTimer.setSingleShot(false);
        connect(&updateTimer, &QTimer::timeout, this, &ServiceTreeWidget::doUpdate);
        updateTimer.start();
    }
    
public slots:
    void onServiceListUpdated(const QList<ServiceInfo> &services) {
        pendingServices = services;
        needsUpdate = true;
    }
    
    void onEnsembleInfoChanged(const EnsembleInfo &info) {
        pendingEnsemble = info;
        needsUpdate = true;
    }
    
private slots:
    void doUpdate() {
        if (needsUpdate) {
            rebuildTree();
            needsUpdate = false;
        }
    }
    
    void rebuildTree() {
        clear();
        
        // Add ensemble
        QTreeWidgetItem *ensembleItem = new QTreeWidgetItem(this);
        ensembleItem->setText(0, pendingEnsemble.label);
        
        // Add services
        for (const auto &service : pendingServices) {
            QTreeWidgetItem *serviceItem = new QTreeWidgetItem(ensembleItem);
            serviceItem->setText(0, service.label);
            serviceItem->setText(1, QString("SId: 0x%1").arg(service.sid, 0, 16));
        }
        
        expandAll();
    }
    
private:
    QTimer updateTimer;
    QList<ServiceInfo> pendingServices;
    EnsembleInfo pendingEnsemble;
    bool needsUpdate = false;
};
```

**Example 3: Hex View (Pattern 5, on-demand)**
```cpp
class HexViewWidget : public QWidget {
    Q_OBJECT
    
public:
    HexViewWidget(SharedETIData *sharedData, QWidget *parent = nullptr)
        : QWidget(parent), shared(sharedData) {
        
        // NO throttling - only updates on user scroll or explicit request
        scrollBar = new QScrollBar(Qt::Vertical, this);
        connect(scrollBar, &QScrollBar::valueChanged, this, &HexViewWidget::onScrollChanged);
    }
    
public slots:
    void onFrameSelected(quint32 frameNumber) {
        // User selected a frame, update immediately
        updateDisplay();
    }
    
private slots:
    void onScrollChanged(int value) {
        // User scrolled, update visible portion
        updateDisplay();
    }
    
private:
    void updateDisplay() {
        // Pattern 5: Read from shared data
        auto frame = shared->getCurrentFrame();
        if (!frame) return;
        
        // Render only visible portion of hex dump
        int offset = scrollBar->value() * 16;
        int length = viewport()->height() / lineHeight * 16;
        
        renderHexDump(frame->rawData, offset, length);
    }
    
    SharedETIData *shared;
    QScrollBar *scrollBar;
};
```

**Example 4: Constellation Diagram (medium throttle)**
```cpp
class ConstellationWidget : public QWidget {
    Q_OBJECT
    
public:
    ConstellationWidget(SharedETIData *sharedData, QWidget *parent = nullptr)
        : QWidget(parent), shared(sharedData) {
        
        // Pattern 8: Throttle to 20 fps for smooth but efficient rendering
        updateTimer.setInterval(50); // 20 fps
        updateTimer.setSingleShot(false);
        connect(&updateTimer, &QTimer::timeout, this, &ConstellationWidget::doUpdate);
        updateTimer.start();
    }
    
public slots:
    void onMSCDataUpdated(QSharedPointer<MSCData> msc) {
        // Signal arrived, mark for update
        needsUpdate = true;
    }
    
private slots:
    void doUpdate() {
        if (needsUpdate) {
            // Pattern 5: Read from shared data for constellation points
            auto msc = shared->getMSCData();
            if (msc) {
                extractConstellationPoints(msc);
                update(); // Trigger paintEvent
            }
            needsUpdate = false;
        }
    }
    
protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw constellation diagram
        for (const auto &point : constellationPoints) {
            painter.drawEllipse(point, 2, 2);
        }
    }
    
private:
    void extractConstellationPoints(QSharedPointer<MSCData> msc) {
        // Extract I/Q samples for constellation
        // This is application-specific logic
        constellationPoints.clear();
        // ... populate points
    }
    
    SharedETIData *shared;
    QTimer updateTimer;
    bool needsUpdate = false;
    QVector<QPointF> constellationPoints;
};
```

---

## Performance Targets

### CPU Usage Breakdown
```
ETI Reading:        5-10%   (Pattern 3)
Frame Parsing:      15-25%  (Pattern 3)
Widget Updates:     10-15%  (Pattern 8 keeps this low)
Rendering:          5-10%   (Throttled by Pattern 8)
Idle/Other:         40-50%
-------------------------
Total Target:       <80% on single core
```

### Memory Usage
```
Raw Frame Buffer:   ~600 KB  (100 frames * 6KB)
Parsed FIC Data:    ~50 KB   (accumulated)
MSC Buffers:        ~1-2 MB  (depends on subchannels)
Widget Buffers:     ~500 KB  (display caches)
Qt Overhead:        ~5-10 MB
-------------------------
Total Target:       <50 MB
```

### Latency Requirements
```
Input to Parse:     <10 ms   (Pattern 3 dedicated thread)
Parse to Signal:    <5 ms    (Direct emit)
Signal to Widget:   <20 ms   (Qt event queue)
Widget to Display:  <16 ms   (60 Hz refresh)
-------------------------
Total Target:       <50 ms end-to-end
```

---

## Connection Setup (Main Window)

```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupConnections();
        
        // Start processing
        readerThread->start();
    }
    
    ~MainWindow() {
        readerThread->stop();
        readerThread->wait();
    }
    
private:
    void setupUI() {
        // Create shared data model
        sharedData = new SharedETIData(this);
        
        // Create widgets
        frameCounter = new FrameCounterWidget(this);
        serviceTree = new ServiceTreeWidget(this);
        hexView = new HexViewWidget(sharedData, this);
        constellationView = new ConstellationWidget(sharedData, this);
        statsPanel = new StatsWidget(this);
        
        // Layout widgets...
    }
    
    void setupConnections() {
        // Create and configure worker thread
        readerThread = new ETIReaderThread(this);
        readerThread->setInputSource("udp://239.0.0.1:5004");
        
        // Pattern 7: Connect specialized signals to appropriate widgets
        
        // Frame counter (throttled internally to 5 fps)
        connect(readerThread, &ETIReaderThread::etiFrameReceived,
                frameCounter, &FrameCounterWidget::onFrameReceived);
        
        // Service tree (throttled internally to 1 fps)
        connect(readerThread, &ETIReaderThread::serviceListUpdated,
                serviceTree, &ServiceTreeWidget::onServiceListUpdated);
        connect(readerThread, &ETIReaderThread::ensembleInfoChanged,
                serviceTree, &ServiceTreeWidget::onEnsembleInfoChanged);
        
        // Hex view (on-demand, no throttle)
        connect(readerThread, &ETIReaderThread::etiFrameReceived,
                hexView, &HexViewWidget::onFrameSelected);
        
        // Constellation (throttled internally to 20 fps)
        connect(readerThread, &ETIReaderThread::mscDataUpdated,
                constellationView, &ConstellationWidget::onMSCDataUpdated);
        
        // Stats panel (throttled internally to 0.5 fps)
        connect(readerThread, &ETIReaderThread::errorStatsUpdated,
                statsPanel, &StatsWidget::onErrorStatsUpdated);
        
        // Update shared data model when new data arrives
        connect(readerThread, &ETIReaderThread::ficDataUpdated,
                this, [this](QSharedPointer<FICData> fic) {
                    sharedData->updateFIC(fic);
                });
        
        connect(readerThread, &ETIReaderThread::mscDataUpdated,
                this, [this](QSharedPointer<MSCData> msc) {
                    sharedData->updateMSC(msc);
                });
    }
    
private:
    ETIReaderThread *readerThread;
    SharedETIData *sharedData;
    
    // Widgets
    FrameCounterWidget *frameCounter;
    ServiceTreeWidget *serviceTree;
    HexViewWidget *hexView;
    ConstellationWidget *constellationView;
    StatsWidget *statsPanel;
};
```

---

## Implementation Phases

### Phase 1: Core Infrastructure (Pattern 3)
**Goal**: Get basic frame reading working

**Tasks**:
- [ ] Create `ETIReaderThread` class skeleton
- [ ] Implement basic QThread with event loop
- [ ] Add UDP/File input source abstraction
- [ ] Test reading at 41 fps without parsing
- [ ] Add basic frame counter to verify throughput

**Validation**: Console output showing frame numbers at ~41 fps

---

### Phase 2: Data Structures (Pattern 5)
**Goal**: Define shared data model

**Tasks**:
- [ ] Define `ETIFrame` structure (match ETSI spec)
- [ ] Define `FICDatabase` structure
- [ ] Define `MSCData` structure
- [ ] Create `SharedETIData` class with mutexes
- [ ] Implement thread-safe getters/setters
- [ ] Add QSharedPointer memory management

**Validation**: Unit tests for thread-safe access

---

### Phase 3: Basic Parsing
**Goal**: Parse ETI frame structure

**Tasks**:
- [ ] Implement ETI frame parser (SYNC, FC, NST, etc.)
- [ ] Extract FIC data (96 bytes)
- [ ] Extract MSC data (variable size)
- [ ] Validate CRC checks
- [ ] Store parsed data in shared model

**Validation**: Compare parsed output with reference tool (ODR-DabMux)

---

### Phase 4: Signal Layer (Pattern 7)
**Goal**: Implement specialized signals

**Tasks**:
- [ ] Add all signal declarations to `ETIReaderThread`
- [ ] Emit `etiFrameReceived()` on each frame
- [ ] Emit `ficDataUpdated()` when FIC changes
- [ ] Emit `mscDataUpdated()` on each frame
- [ ] Emit `errorStatsUpdated()` periodically
- [ ] Test signal emission rates

**Validation**: Log all signals and verify emission frequency

---

### Phase 5: Basic Widgets
**Goal**: Create initial UI widgets

**Tasks**:
- [ ] Implement `FrameCounterWidget` with throttling
- [ ] Implement `ServiceTreeWidget` with slow updates
- [ ] Implement basic `HexViewWidget` (no scrolling yet)
- [ ] Create `MainWindow` layout
- [ ] Connect signals to widgets

**Validation**: UI updates smoothly without lag

---

### Phase 6: Throttling (Pattern 8)
**Goal**: Optimize widget update rates

**Tasks**:
- [ ] Add QTimer-based throttling to each widget
- [ ] Configure appropriate rates per widget type
- [ ] Measure CPU usage before/after throttling
- [ ] Add configurable throttle rates in settings
- [ ] Profile and tune throttle values

**Validation**: CPU usage <80%, UI remains responsive at 41 fps input

---

### Phase 7: Advanced Widgets
**Goal**: Complete the analyzer UI

**Tasks**:
- [ ] Full `HexViewWidget` with scrolling
- [ ] `ConstellationWidget` for signal quality
- [ ] `WaterfallWidget` for spectrum display
- [ ] `BitrateGraphWidget` for statistics
- [ ] `ErrorCounterWidget` for CRC errors
- [ ] Subchannel tabs

**Validation**: Compare with Elecard/DAB-XPlorer features

---

### Phase 8: FIC Decoding
**Goal**: Decode Fast Information Channel

**Tasks**:
- [ ] Implement FIG parser (Fast Information Groups)
- [ ] Decode ensemble label (FIG 1/0)
- [ ] Decode service list (FIG 0/2)
- [ ] Decode subchannel organization (FIG 0/1)
- [ ] Update `FICDatabase` structure
- [ ] Emit `serviceListUpdated()` signals

**Validation**: Service list matches reference decoders

---

### Phase 9: MSC Decoding (Optional)
**Goal**: Decode audio subchannels

**Tasks**:
- [ ] Implement subchannel extraction
- [ ] Add DAB/DAB+ detection
- [ ] Optional: Integrate SuperH FEC decoder
- [ ] Optional: Integrate AAC decoder for audio
- [ ] Add audio level meters

**Validation**: Successfully decode and play audio

---

### Phase 10: Polish & Optimization
**Goal**: Professional quality

**Tasks**:
- [ ] Add dark/light theme support
- [ ] Implement save/load session
- [ ] Add export functionality (CSV, JSON)
- [ ] Add keyboard shortcuts
- [ ] Create user manual
- [ ] Performance profiling and optimization
- [ ] Memory leak detection
- [ ] Add unit tests

**Validation**: Ready for production use

---

## Critical Implementation Notes

### Thread Safety Checklist
- [ ] All widget updates use Qt::QueuedConnection (automatic for cross-thread)
- [ ] Shared data access always protected by QMutexLocker
- [ ] QSharedPointer used for all large data structures
- [ ] No raw pointers passed between threads
- [ ] Worker thread only emits signals, never calls widget methods directly

### Performance Checklist
- [ ] Worker thread has minimal allocations in hot path
- [ ] Throttling implemented on all high-frequency widgets
- [ ] Large buffers use QSharedPointer (no copying)
- [ ] Profiling shows CPU usage <80%
- [ ] No UI freezing even at 41 fps input

### Qt Best Practices
- [ ] Use Qt::QueuedConnection for all cross-thread signals
- [ ] Never call QWidget methods from non-GUI thread
- [ ] Always use QMutexLocker (RAII pattern)
- [ ] Parent QObjects properly for automatic cleanup
- [ ] Use Q_OBJECT macro on all QObject subclasses
- [ ] Connect signals in main thread (Qt::AutoConnection works)

---

## Testing Strategy

### Unit Tests
```cpp
// Test shared data thread-safety
void testSharedDataConcurrentAccess() {
    SharedETIData sharedData;
    
    // Writer thread
    QThread writerThread;
    auto writer = new QObject();
    writer->moveToThread(&writerThread);
    QTimer::singleShot(0, writer, [&]() {
        for (int i = 0; i < 1000; i++) {
            auto frame = QSharedPointer<ETIFrame>::create();
            frame->frameNumber = i;
            sharedData.updateFrame(frame);
        }
    });
    writerThread.start();
    
    // Reader threads (multiple)
    for (int t = 0; t < 5; t++) {
        QThread *readerThread = new QThread();
        auto reader = new QObject();
        reader->moveToThread(readerThread);
        QTimer::singleShot(0, reader, [&]() {
            for (int i = 0; i < 1000; i++) {
                auto frame = sharedData.getCurrentFrame();
                QVERIFY(frame != nullptr);
            }
        });
        readerThread->start();
    }
    
    // Wait for all threads
    // Verify no crashes, no data corruption
}
```

### Integration Tests
- ETI file playback at various speeds
- UDP multicast reception
- Frame counter accuracy
- FIC decoding correctness
- UI responsiveness under load

### Performance Tests
- Measure CPU usage over 1 hour
- Check memory usage (no leaks)
- Verify 41 fps sustained throughput
- Profile with QtCreator or perf

---

## Troubleshooting Guide

### Problem: UI freezing
**Symptoms**: Application becomes unresponsive

**Diagnosis**:
- Check if widget code is blocking
- Verify throttling is active
- Profile to find hot spots

**Solution**:
- Add/adjust Pattern 8 throttling
- Move heavy computations to worker thread
- Use QSharedPointer to avoid copies

---

### Problem: High CPU usage
**Symptoms**: CPU constantly at 100%

**Diagnosis**:
- Profile with QtCreator
- Check update rates of widgets
- Verify parsing efficiency

**Solution**:
- Increase throttle intervals (Pattern 8)
- Optimize parsing code
- Reduce signal emission frequency

---

### Problem: Crashes
**Symptoms**: Segmentation fault or unexpected exit

**Diagnosis**:
- Check for raw pointer access across threads
- Verify QObject parent/child relationships
- Look for race conditions in shared data

**Solution**:
- Always use QSharedPointer for shared data
- Add QMutexLocker to all shared data access
- Run with valgrind or AddressSanitizer

---

### Problem: Data corruption
**Symptoms**: Wrong values displayed, garbled output

**Diagnosis**:
- Missing mutex locks
- Race condition in shared data
- Incorrect signal/slot connection types

**Solution**:
- Add QMutexLocker to ALL shared data methods
- Verify Qt::QueuedConnection is used
- Review all QSharedPointer usage

---

## References

### ETSI Standards
- **ETSI EN 300 401**: DAB System specification
- **ETSI TS 102 563**: DAB+ audio codec
- **ETSI EN 300 799**: ETI specification

### Qt Documentation
- QThread: https://doc.qt.io/qt-6/qthread.html
- Signals & Slots: https://doc.qt.io/qt-6/signalsandslots.html
- QSharedPointer: https://doc.qt.io/qt-6/qsharedpointer.html
- Thread-Safety: https://doc.qt.io/qt-6/threads-synchronizing.html

### Reference Tools
- **Elecard Stream Analyzer**: Commercial analyzer (reference UI)
- **DAB-XPlorer**: Open-source DAB analyzer
- **ODR-DabMux**: ETI multiplexer (parser reference)
- **ODR-AudioEnc**: Audio encoder
- **CRC-DabMux**: Original DAB multiplexer

### Open Source Libraries
- **ODR-mmbTools**: Complete DAB+ toolchain
- **dablin**: DAB+ SDR receiver with GTK UI
- **welle.io**: DAB+ receiver with Qt UI
- **rtl-sdr**: Software Defined Radio library

---

## Glossary

- **ETI**: Ensemble Transport Interface - container format for DAB frames
- **FIC**: Fast Information Channel - metadata about ensemble/services
- **MSC**: Main Service Channel - actual audio/data payload
- **DAB**: Digital Audio Broadcasting (original, MP2 audio)
- **DAB+**: Enhanced DAB (HE-AAC v2 audio, better quality)
- **FIG**: Fast Information Group - structured data in FIC
- **Subchannel**: Logical channel within MSC carrying one service
- **Ensemble**: Group of services transmitted together
- **Service**: Radio station or data service
- **CRC**: Cyclic Redundancy Check - error detection

---

## Summary

This architecture uses **Pattern 3 + 5 + 7 + 8** to achieve professional-grade DAB+ stream analysis:

1. **Pattern 3** (Worker Thread): Continuous ETI processing isolated from UI
2. **Pattern 5** (Shared Data): Efficient zero-copy data sharing with mutex protection
3. **Pattern 7** (Specialized Signals): Selective widget updates per data layer
4. **Pattern 8** (Throttling): Optimized refresh rates prevent UI overload

**Expected Results**:
- ✅ Smooth UI at 41 fps input rate
- ✅ CPU usage <80% on single core
- ✅ Responsive even with 10+ widgets
- ✅ Professional architecture matching commercial tools
- ✅ Easy to extend with new widgets/features

**Key Success Factors**:
- Dedicated thread keeps parsing independent
- Shared pointers eliminate data copying
- Specialized signals reduce unnecessary processing
- Throttling prevents UI overload
- Matches architecture of proven tools (Elecard, DAB-XPlorer)

---

## Quick Start Commands

```bash
# Clone or create project
mkdir dab-analyzer && cd dab-analyzer

# CMakeLists.txt setup
cmake_minimum_required(VERSION 3.16)
project(DABAnalyzer LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

add_executable(dab-analyzer
    main.cpp
    mainwindow.cpp
    etireaderthread.cpp
    shareddata.cpp
    # ... add widget sources
)

target_link_libraries(dab-analyzer PRIVATE 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network
)

# Build
mkdir build && cd build
cmake ..
make -j"$(bash scripts/build_jobs.sh)"

# Run
./dab-analyzer
```

---

**Document Version**: 1.0  
**Last Updated**: 2025-10-25  
**Target Qt Version**: Qt 6.x  
**Language**: C++17 or later
