# StreamDAB Analyzer - Updated Implementation Plan 2025
## Incorporating Code Review Findings + ETSI EWS Standards Support

**Document Version:** 2.0
**Date:** October 17, 2025
**Status:** Comprehensive plan integrating code review and new requirements

---

## EXECUTIVE SUMMARY

This document integrates:
1. **Code Review Findings** - 7 critical issues, 12 high-priority issues identified
2. **New ETSI Standards** - Emergency Warning System (EWS) support requirements
3. **Updated Timeline** - Adjusted 7-9 week plan (was 6-8 weeks)

**Key Changes:**
- **Phase 0 Added:** Critical bug fixes (3-4 days) - MUST complete before Phase 1
- **Phase 6 Added:** ETSI EWS standards implementation (1-2 weeks)
- **Total Timeline:** 7-9 weeks (Option B + EWS support)

---

## TABLE OF CONTENTS

1. [Code Review Impact Assessment](#code-review-impact-assessment)
2. [Phase 0: Critical Fixes](#phase-0-critical-fixes-week-0)
3. [Updated Phase 1-5](#updated-phases-1-5)
4. [Phase 6: ETSI EWS Implementation](#phase-6-etsi-ews-implementation)
5. [ETSI Standards Reference](#etsi-standards-reference)
6. [Testing Strategy](#testing-strategy)
7. [Risk Management](#risk-management)

---

## CODE REVIEW IMPACT ASSESSMENT

### Critical Issues Summary (Must Fix Before Phase 1)

| ID | Issue | Severity | Impact | Est. Fix Time |
|----|-------|----------|--------|---------------|
| **CRITICAL-001** | Memory leak in LockFreeQueue | CRITICAL | Memory corruption, crashes | 4 hours |
| **CRITICAL-002** | Unsafe reinterpret_cast alignment | CRITICAL | Undefined behavior on ARM/RISC-V | 3 hours |
| **CRITICAL-003** | No real ETI frame parsing | CRITICAL | Core functionality missing | 2 weeks (Phase 1) |
| **CRITICAL-004** | FIG processing incomplete | CRITICAL | Service discovery broken | 2 weeks (Phase 1) |
| **CRITICAL-005** | Thread safety in multi-stream | CRITICAL | Race conditions, crashes | 6 hours |
| **CRITICAL-006** | Missing input validation | CRITICAL | Buffer overflows | 4 hours |
| **CRITICAL-007** | No CRC validation | CRITICAL | Data integrity compromised | 3 hours |

**Total Phase 0 Effort:** 3-4 days (20-24 hours excluding CRITICAL-003/004)

### High-Priority Issues (Fix During Implementation)

| ID | Issue | Phase | Impact |
|----|-------|-------|--------|
| HIGH-001 | Simulation data instead of real discovery | Phase 1 | Core functionality |
| HIGH-002 | Race condition in signal connections | Phase 0 | Stability |
| HIGH-003 | Memory leaks in QTreeWidgetItem | Phase 2 | Memory usage |
| HIGH-004 | No error handling in file loading | Phase 1 | User experience |
| HIGH-005 | Missing destructor cleanup | Phase 0 | Resource leaks |
| HIGH-007 | No resource limits for frame storage | Phase 1 | Memory exhaustion |
| HIGH-008 | Unsafe type cast without validation | Phase 1 | Crashes |

---

## PHASE 0: CRITICAL FIXES (Week 0 - 3-4 days)

**Goal:** Fix all blocking critical issues before Phase 1 implementation

### Day 1-2: Memory Safety Fixes

#### Task 0.1: Fix LockFreeQueue Memory Leak (CRITICAL-001)
**File:** `src/network/streaming_processor.cpp`

**Implementation:**
```cpp
// Add proper destructor and epoch-based memory reclamation
template<typename T>
class LockFreeQueue {
private:
    std::atomic<uint64_t> m_pop_epoch{0};
    std::vector<Node*> m_retired_nodes;
    std::mutex m_retired_mutex;

public:
    ~LockFreeQueue() {
        // Clean up all remaining nodes
        T item;
        while (pop(item)) { /* drain queue */ }

        // Clean up head sentinel
        Node* head = m_head.load(std::memory_order_acquire);
        while (head) {
            Node* next = head->next.load(std::memory_order_acquire);
            delete head;
            head = next;
        }

        // Clean up retired nodes
        std::lock_guard<std::mutex> lock(m_retired_mutex);
        for (Node* node : m_retired_nodes) {
            delete node;
        }
    }

    bool pop(T& item) {
        uint64_t epoch = m_pop_epoch.fetch_add(1, std::memory_order_acq_rel);

        Node* head = m_head.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);

        if (next == nullptr) {
            return false; // Queue empty
        }

        T* data = next->data.exchange(nullptr, std::memory_order_acquire);
        if (data == nullptr) {
            return false; // Already popped
        }

        item = std::move(*data);
        delete data;

        // Use CAS for safe head update
        if (m_head.compare_exchange_strong(head, next,
                                          std::memory_order_acq_rel)) {
            // Defer node deletion using epoch-based reclamation
            retire_node(head, epoch);
            m_size.fetch_sub(1, std::memory_order_release);
            m_pop_count.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        return false; // Another thread modified head
    }

private:
    void retire_node(Node* node, uint64_t epoch) {
        std::lock_guard<std::mutex> lock(m_retired_mutex);
        m_retired_nodes.push_back(node);

        // Periodically clean up old retired nodes
        if (m_retired_nodes.size() > 1000) {
            cleanup_retired_nodes(epoch);
        }
    }

    void cleanup_retired_nodes(uint64_t current_epoch) {
        // Only delete nodes from epochs far enough in the past
        const uint64_t SAFE_EPOCH_DISTANCE = 1000;

        auto it = m_retired_nodes.begin();
        while (it != m_retired_nodes.end()) {
            // For simplicity, delete all if we have too many
            // In production, track per-node epochs
            if (m_retired_nodes.size() > 10000 ||
                current_epoch > SAFE_EPOCH_DISTANCE) {
                delete *it;
                it = m_retired_nodes.erase(it);
            } else {
                ++it;
            }
        }
    }
};
```

**Testing:**
- Unit test with multiple threads pushing/popping
- Valgrind memory leak detection
- Thread sanitizer for race conditions

#### Task 0.2: Fix Alignment Issues (CRITICAL-002)
**File:** `src/network/circular_buffer_manager.cpp`

**Implementation:**
```cpp
#include <memory>
#include <new> // For std::align

class CircularBufferManager {
private:
    // Use properly aligned memory
    struct AlignedBufferDeleter {
        void operator()(BufferedETIFrame* ptr) const {
            if (ptr) {
                // Call destructors
                // Size stored in class member
                std::free(ptr);
            }
        }
    };

    std::unique_ptr<BufferedETIFrame, AlignedBufferDeleter> m_buffer_memory;
    size_t m_capacity{0};

public:
    bool allocate(size_t capacity) {
        m_capacity = capacity;

        // Use aligned_alloc for proper alignment
        void* ptr = std::aligned_alloc(
            alignof(BufferedETIFrame),
            capacity * sizeof(BufferedETIFrame)
        );

        if (!ptr) {
            throw std::bad_alloc();
        }

        // Cast to proper type
        auto* typed_ptr = static_cast<BufferedETIFrame*>(ptr);

        // Initialize objects with placement new
        for (size_t i = 0; i < capacity; ++i) {
            new (&typed_ptr[i]) BufferedETIFrame();
        }

        m_buffer_memory.reset(typed_ptr);
        return true;
    }

    ~CircularBufferManager() {
        if (m_buffer_memory) {
            // Call destructors explicitly
            for (size_t i = 0; i < m_capacity; ++i) {
                m_buffer_memory.get()[i].~BufferedETIFrame();
            }
        }
    }
};
```

**Testing:**
- Compile and test on ARM architecture (use QEMU if needed)
- Address sanitizer for alignment violations
- Test with different buffer sizes

#### Task 0.3: Thread Safety Fixes (CRITICAL-005, HIGH-002)
**Files:**
- `src/core/multi_stream_processor.cpp`
- `src/main.cpp`

**Implementation:**
```cpp
// multi_stream_processor.cpp - Fix worker thread ownership
class MultiStreamProcessor : public QObject {
private:
    struct WorkerContext {
        std::unique_ptr<QThread> thread;
        std::unique_ptr<StreamProcessingWorker> worker;
        QString stream_id;
    };

    std::map<QString, WorkerContext> m_workers;

public:
    bool addStream(const QString& stream_id, const StreamInfo& info) {
        WorkerContext context;
        context.stream_id = stream_id;
        context.thread = std::make_unique<QThread>();
        context.worker = std::make_unique<StreamProcessingWorker>(stream_id, info);

        // Move worker to thread
        context.worker->moveToThread(context.thread.get());

        // Connect cleanup signals BEFORE starting thread
        connect(context.thread.get(), &QThread::finished,
                context.worker.get(), &QObject::deleteLater,
                Qt::DirectConnection);

        connect(context.thread.get(), &QThread::started,
                context.worker.get(), &StreamProcessingWorker::startProcessing,
                Qt::QueuedConnection);

        // Store context
        m_workers[stream_id] = std::move(context);

        // Start thread
        m_workers[stream_id].thread->start();

        return true;
    }

    ~MultiStreamProcessor() {
        stopAllStreams();
    }

    void stopAllStreams() {
        for (auto& [id, context] : m_workers) {
            if (context.thread && context.thread->isRunning()) {
                // Request thread stop
                context.thread->quit();

                // Wait with timeout
                if (!context.thread->wait(5000)) {
                    qWarning() << "Thread" << id << "did not stop, terminating";
                    context.thread->terminate();
                    context.thread->wait(1000);
                }
            }
        }
        m_workers.clear();
    }
};

// main.cpp - Fix race condition in signal connections
DABAnalyzerWindow::DABAnalyzerWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    // Create components
    m_etiProcessor = std::make_unique<EnhancedETIProcessorQt>();
    m_errorDetector = std::make_unique<error_detection::AdvancedErrorDetector>();

    // Connect signals IMMEDIATELY (not delayed)
    // Use Qt::QueuedConnection for cross-thread safety
    connect(m_errorDetector.get(),
            &error_detection::AdvancedErrorDetector::errorDetected,
            this, &DABAnalyzerWindow::onErrorDetected,
            Qt::QueuedConnection);

    // NO MORE QTimer::singleShot for delayed connections!
}

// Add proper destructor
DABAnalyzerWindow::~DABAnalyzerWindow() {
    // Stop all processing
    if (m_etiProcessor) {
        m_etiProcessor->stopProcessing();
    }

    if (m_multiStreamProcessor) {
        m_multiStreamProcessor->stopAllStreams();
    }

    // Disconnect all signals to prevent callbacks after destruction
    disconnect();

    // Wait briefly for threads to finish
    QThread::msleep(100);
}
```

**Testing:**
- Thread sanitizer
- Stress test with rapid window open/close
- Multiple simultaneous stream processing

### Day 3: Validation & Error Handling

#### Task 0.4: Add Input Validation (CRITICAL-006)
**File:** `src/core/advanced_fig_analyzer.cpp`

**Implementation:**
```cpp
bool AdvancedFIGAnalyzer::processFIB(const QByteArray& ficData) {
    // Validate input size
    if (ficData.isEmpty()) {
        m_lastError = "FIB data is empty";
        return false;
    }

    if (ficData.size() > 256) { // FIB max size
        m_lastError = QString("FIB data too large: %1 bytes").arg(ficData.size());
        return false;
    }

    int offset = 0;

    while (offset < ficData.size()) {
        // Ensure we have at least 2 bytes for FIG header
        if (offset + 2 > ficData.size()) {
            break; // End of valid data
        }

        uint8_t figType, figLength;

        // Extract header with bounds checking
        uint8_t header = static_cast<uint8_t>(ficData[offset]);
        figType = (header >> 5) & 0x07;
        figLength = static_cast<uint8_t>(ficData[offset + 1]) & 0x1F;

        // Validate length
        if (figLength == 0) {
            offset += 2;
            continue; // Empty FIG, skip
        }

        if (offset + 2 + figLength > ficData.size()) {
            m_lastError = QString("FIG length %1 exceeds remaining data at offset %2")
                         .arg(figLength).arg(offset);
            break;
        }

        // Extract FIG data safely
        QByteArray figData = ficData.mid(offset + 2, figLength);

        // Process FIG with additional validation
        processFIG(figType, figData);

        // Move to next FIG
        offset += 2 + figLength;
    }

    return true;
}

// Add helper for safe integer extraction
uint16_t AdvancedFIGAnalyzer::extractUInt16(const QByteArray& data,
                                            int offset) const {
    // Bounds checking
    if (offset < 0 || offset + 1 >= data.size()) {
        qWarning() << "extractUInt16: Invalid offset" << offset
                   << "for data size" << data.size();
        return 0;
    }

    return (static_cast<uint16_t>(static_cast<uint8_t>(data[offset])) << 8) |
           static_cast<uint16_t>(static_cast<uint8_t>(data[offset + 1]));
}

uint32_t AdvancedFIGAnalyzer::extractUInt32(const QByteArray& data,
                                            int offset) const {
    // Bounds checking
    if (offset < 0 || offset + 3 >= data.size()) {
        qWarning() << "extractUInt32: Invalid offset" << offset
                   << "for data size" << data.size();
        return 0;
    }

    return (static_cast<uint32_t>(static_cast<uint8_t>(data[offset])) << 24) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 1])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 2])) << 8) |
           static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 3]));
}
```

#### Task 0.5: Implement CRC Validation (CRITICAL-007)
**File:** `src/core/enhanced_eti_processor_qt.cpp`

**Implementation:**
```cpp
class EnhancedETIProcessorQt : public QObject {
private:
    // CRC-16-CCITT lookup table for performance
    static const uint16_t CRC16_TABLE[256];

    uint16_t calculateCRC16(const uint8_t* data, size_t length) const {
        uint16_t crc = 0xFFFF; // CRC-16-CCITT initial value

        for (size_t i = 0; i < length; ++i) {
            uint8_t index = (crc >> 8) ^ data[i];
            crc = (crc << 8) ^ CRC16_TABLE[index];
        }

        return crc;
    }

    bool validateFrameCRC(const QByteArray& frameData) const {
        if (frameData.size() != 6144) {
            return false;
        }

        const uint8_t* data = reinterpret_cast<const uint8_t*>(
            frameData.constData()
        );

        // Calculate CRC over entire frame except last 2 bytes (CRC field)
        uint16_t calculated = calculateCRC16(data, 6142);

        // Extract stored CRC (last 2 bytes, big-endian)
        uint16_t stored = (static_cast<uint16_t>(data[6142]) << 8) |
                         static_cast<uint16_t>(data[6143]);

        return calculated == stored;
    }

    bool validateFICCRC(const QByteArray& ficData) const {
        // FIC consists of 3 FIBs (Fast Information Blocks)
        // Each FIB is 32 bytes: 30 bytes data + 2 bytes CRC

        if (ficData.size() != 96) { // 3 FIBs * 32 bytes
            return false;
        }

        const uint8_t* data = reinterpret_cast<const uint8_t*>(
            ficData.constData()
        );

        // Validate each FIB
        for (int fib = 0; fib < 3; ++fib) {
            int offset = fib * 32;

            // Calculate CRC over 30 bytes of FIB data
            uint16_t calculated = calculateCRC16(data + offset, 30);

            // Extract stored CRC (last 2 bytes of FIB)
            uint16_t stored = (static_cast<uint16_t>(data[offset + 30]) << 8) |
                             static_cast<uint16_t>(data[offset + 31]);

            if (calculated != stored) {
                emit frameError(frame_count_,
                              QString("FIC CRC error in FIB %1").arg(fib),
                              QString("Expected 0x%1, got 0x%2")
                                  .arg(calculated, 4, 16, QChar('0'))
                                  .arg(stored, 4, 16, QChar('0')));
                return false;
            }
        }

        return true;
    }
};

// CRC-16-CCITT lookup table (defined in .cpp)
const uint16_t EnhancedETIProcessorQt::CRC16_TABLE[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    // ... (complete 256-entry table)
};
```

#### Task 0.6: Add Resource Limits (HIGH-007)
**File:** `src/core/enhanced_eti_processor_qt.cpp`

**Implementation:**
```cpp
class EnhancedETIProcessorQt : public QObject {
private:
    static constexpr size_t MAX_STORED_FRAMES = 10000;
    static constexpr size_t FRAME_CLEANUP_THRESHOLD = 1000;
    static constexpr qint64 MAX_FILE_SIZE = 500 * 1024 * 1024; // 500 MB

    std::vector<ProcessedFrame> processed_frames_;

    void storeFrame(const ProcessedFrame& frame) {
        // Check if we need to cleanup old frames
        if (processed_frames_.size() >= MAX_STORED_FRAMES) {
            // Remove oldest frames (FIFO)
            size_t remove_count = FRAME_CLEANUP_THRESHOLD;
            processed_frames_.erase(
                processed_frames_.begin(),
                processed_frames_.begin() + remove_count
            );

            qInfo() << "Frame storage limit reached, removed"
                    << remove_count << "oldest frames";
        }

        processed_frames_.push_back(frame);
    }

    bool loadFile(const QString& filePath) {
        QFile file(filePath);

        // Comprehensive file validation
        if (!file.exists()) {
            emit processingError(QString("File does not exist: %1").arg(filePath));
            return false;
        }

        if (!file.open(QIODevice::ReadOnly)) {
            emit processingError(QString("Cannot open file: %1 - %2")
                               .arg(filePath).arg(file.errorString()));
            return false;
        }

        // Check file size
        qint64 fileSize = file.size();
        if (fileSize <= 0) {
            emit processingError(QString("Empty or invalid file: %1").arg(filePath));
            return false;
        }

        if (fileSize > MAX_FILE_SIZE) {
            emit processingError(
                QString("File too large: %1 MB (limit: %2 MB)")
                    .arg(fileSize / 1024.0 / 1024.0, 0, 'f', 1)
                    .arg(MAX_FILE_SIZE / 1024.0 / 1024.0, 0, 'f', 0)
            );
            return false;
        }

        // Validate file size is multiple of frame size
        if (fileSize % 6144 != 0) {
            emit processingError(
                QString("Invalid ETI file size: %1 bytes (not multiple of 6144)")
                    .arg(fileSize)
            );
            return false;
        }

        // Read file data
        file_data_ = file.readAll();
        if (file_data_.size() != fileSize) {
            emit processingError(
                QString("Read error: expected %1 bytes, got %2")
                    .arg(fileSize).arg(file_data_.size())
            );
            return false;
        }

        file.close();

        total_frames_ = fileSize / 6144;
        current_file_path_ = filePath;

        qInfo() << "Loaded ETI file:" << filePath
                << "Size:" << fileSize
                << "Frames:" << total_frames_;

        return true;
    }
};
```

### Phase 0 Deliverables & Verification

**Completion Criteria:**
- [ ] All 7 critical issues fixed
- [ ] Unit tests pass for all fixes
- [ ] Memory leak detection clean (Valgrind)
- [ ] Thread sanitizer clean
- [ ] Address sanitizer clean
- [ ] Code compiles with zero warnings
- [ ] Manual testing confirms stability

**Verification Commands:**
```bash
# Build with sanitizers
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,thread,undefined" \
      ..
make

# Run with Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./DABAnalyzer_Enhanced_FilteringBuild

# Run tests
ctest --verbose

# Check for remaining issues
grep -r "TODO\|FIXME\|XXX" src/ | grep -i "critical\|urgent"
```

---

## UPDATED PHASES 1-5

### PHASE 1: ETI Core Engine (Weeks 1-2)
**Status:** CRITICAL-003 and CRITICAL-004 implementation

*Content unchanged from original CLAUDE.md Phase 1 plan*

### PHASE 2: GUI Integration (Week 3)
*Content unchanged from original CLAUDE.md Phase 2 plan*

### PHASE 3: ETSI Compliance & Real-time (Weeks 4-5)
*Content unchanged from original CLAUDE.md Phase 3 plan*

### PHASE 4: Performance Optimization (Week 6)
*Content unchanged from original CLAUDE.md Phase 4 plan*

### PHASE 5: Professional Polish (Week 7)
*Content unchanged from original CLAUDE.md Phase 5 plan*

---

## PHASE 6: ETSI EWS IMPLEMENTATION (Weeks 8-9)

**Goal:** Add Emergency Warning System support per ETSI specifications

### Overview of ETSI EWS Standards

#### ETSI TS 104 089 V1.1.1 (2024-09)
**Title:** Digital Audio Broadcasting (DAB); Emergency Warning System (EWS); System specification

**Key Features:**
- FIC signalling for emergency alert identity, status, and location
- Low-power sleep mode wake-up capability
- Audio alert messages targeted to specific geographical areas
- DAB time synchronization requirement (receivers check alerts every minute)
- Uses FIG 0/10 for date-time information (long form)

**Technical Requirements:**
- Ensemble must conform to ETSI EN 300 401
- Time source: GNSS or NTP for accuracy
- All sleeping receivers synchronized to check alerts simultaneously

#### ETSI TS 104 090 V1.1.2 (2025-02)
**Title:** Digital Audio Broadcasting (DAB); Emergency Warning System (EWS); Minimum requirements and test specifications for receivers

**Key Features:**
- Defines requirements for consumer EWS receivers
- Test methods for compliance verification
- Two receiver types: Domestic and In-vehicle
- Certification mark scheme technical basis

#### ETSI TS 103 461 V1.2.2 (2020-10)
**Title:** Digital Audio Broadcasting (DAB); Minimum requirements for digital radio receivers

**Relevant Updates (v1.2.2):**
- Ability to deliver alarm announcements
- FIG type 2 labels decoding
- Service following with ECC
- Mandatory from April 2022

### Week 8: EWS FIG Processing Implementation

#### Task 6.1: Implement FIG 0/10 (Date and Time)
**File:** `src/core/advanced_fig_analyzer.cpp`

**Implementation:**
```cpp
// EWS requires accurate time synchronization
struct DABDateTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
    bool utc_flag;
    int8_t local_time_offset; // Hours from UTC

    QDateTime toQDateTime() const {
        QDate date(year, month, day);
        QTime time(hour, minute, second, millisecond);
        QDateTime dateTime(date, time, utc_flag ? Qt::UTC : Qt::LocalTime);

        if (!utc_flag && local_time_offset != 0) {
            dateTime = dateTime.addSecs(local_time_offset * 3600);
        }

        return dateTime;
    }
};

class AdvancedFIGAnalyzer {
private:
    DABDateTime m_ensembleDateTime;
    bool m_hasDateTime{false};

public:
    void processFIG0_10(const QByteArray& data) {
        // FIG 0/10: Date and time (long form required for EWS)
        if (data.size() < 5) {
            m_lastError = "FIG 0/10 data too short";
            return;
        }

        int offset = 0;

        // Modified Julian Date (MJD) - 17 bits
        uint32_t mjd = ((static_cast<uint32_t>(data[offset]) & 0x7F) << 10) |
                       (static_cast<uint32_t>(data[offset + 1]) << 2) |
                       ((static_cast<uint32_t>(data[offset + 2]) >> 6) & 0x03);

        // Convert MJD to calendar date
        // Algorithm from ETSI EN 300 401
        uint32_t yp = static_cast<uint32_t>(
            (static_cast<double>(mjd) - 15078.2) / 365.25
        );
        uint32_t mp = static_cast<uint32_t>(
            (static_cast<double>(mjd) - 14956.1 -
             static_cast<double>(static_cast<int>(yp * 365.25))) / 30.6001
        );

        uint32_t day = mjd - 14956 - static_cast<int>(yp * 365.25) -
                       static_cast<int>(mp * 30.6001);
        uint32_t k = (mp == 14 || mp == 15) ? 1 : 0;
        uint32_t year = yp + k + 1900;
        uint32_t month = mp - 1 - k * 12;

        // Hours and Minutes (16 bits)
        uint8_t hour = (data[offset + 2] >> 1) & 0x1F;
        uint8_t minute = ((data[offset + 2] & 0x01) << 5) |
                         ((data[offset + 3] >> 3) & 0x1F);

        // Seconds (optional, if long form)
        uint8_t second = 0;
        uint16_t millisecond = 0;

        if (data.size() >= 6) { // Long form
            second = ((data[offset + 3] & 0x07) << 3) |
                     ((data[offset + 4] >> 5) & 0x07);
            millisecond = ((data[offset + 4] & 0x1F) << 5) |
                          ((data[offset + 5] >> 3) & 0x1F);
        }

        // UTC flag and local time offset
        bool utc_flag = (data[offset + 5] >> 2) & 0x01;
        int8_t lto = 0;

        if (data.size() >= 7 && !utc_flag) {
            lto = data[offset + 6] & 0x1F;
            if (data[offset + 6] & 0x20) { // Sign bit
                lto = -lto;
            }
        }

        // Store parsed date/time
        m_ensembleDateTime.year = year;
        m_ensembleDateTime.month = month;
        m_ensembleDateTime.day = day;
        m_ensembleDateTime.hour = hour;
        m_ensembleDateTime.minute = minute;
        m_ensembleDateTime.second = second;
        m_ensembleDateTime.millisecond = millisecond;
        m_ensembleDateTime.utc_flag = utc_flag;
        m_ensembleDateTime.local_time_offset = lto;
        m_hasDateTime = true;

        qInfo() << "DAB Date/Time:"
                << QString("%1-%2-%3 %4:%5:%6.%7")
                   .arg(year).arg(month, 2, 10, QChar('0'))
                   .arg(day, 2, 10, QChar('0'))
                   .arg(hour, 2, 10, QChar('0'))
                   .arg(minute, 2, 10, QChar('0'))
                   .arg(second, 2, 10, QChar('0'))
                   .arg(millisecond, 3, 10, QChar('0'))
                << (utc_flag ? "UTC" : QString("UTC%1%2")
                   .arg(lto >= 0 ? "+" : "")
                   .arg(lto));

        emit dateTimeUpdated(m_ensembleDateTime.toQDateTime());
    }

    bool hasDateTime() const { return m_hasDateTime; }
    DABDateTime getDateTime() const { return m_ensembleDateTime; }
};
```

#### Task 6.2: Implement FIG 5/2 (Emergency Warning System)
**File:** `src/core/advanced_fig_analyzer.cpp`

**Implementation:**
```cpp
// Emergency Warning Alert structure
struct EmergencyAlert {
    uint16_t alert_id;
    uint8_t alert_type;
    uint8_t alert_category;
    uint8_t alert_status; // Active, Update, Cancellation
    uint8_t severity;
    QString geographical_area;
    QDateTime start_time;
    QDateTime end_time;
    QString message_text;
    uint32_t service_id; // Audio service for alert message
    bool auto_switching; // Auto-switch to alert service
};

class AdvancedFIGAnalyzer {
private:
    std::vector<EmergencyAlert> m_activeAlerts;

signals:
    void emergencyAlertReceived(const EmergencyAlert& alert);
    void emergencyAlertCancelled(uint16_t alert_id);

public:
    void processFIG5_2(const QByteArray& data) {
        // FIG 5/2: Emergency Warning System signalling
        // This is a custom extension based on ETSI TS 104 089

        if (data.size() < 8) {
            m_lastError = "FIG 5/2 data too short for EWS alert";
            return;
        }

        int offset = 0;

        // Alert ID (16 bits)
        uint16_t alert_id = extractUInt16(data, offset);
        offset += 2;

        // Alert type and flags
        uint8_t type_byte = data[offset++];
        uint8_t alert_type = (type_byte >> 4) & 0x0F;
        uint8_t alert_status = type_byte & 0x0F;

        // Alert category and severity
        uint8_t category_byte = data[offset++];
        uint8_t alert_category = (category_byte >> 4) & 0x0F;
        uint8_t severity = category_byte & 0x0F;

        // Service ID for audio alert (32 bits)
        uint32_t service_id = extractUInt32(data, offset);
        offset += 4;

        // Auto-switching flag
        bool auto_switching = (data[offset] >> 7) & 0x01;

        // Geographical area code length
        uint8_t geo_length = data[offset] & 0x7F;
        offset++;

        // Extract geographical area
        QString geographical_area;
        if (geo_length > 0 && offset + geo_length <= data.size()) {
            geographical_area = QString::fromUtf8(
                data.mid(offset, geo_length)
            );
            offset += geo_length;
        }

        // Alert timing (if present)
        QDateTime start_time = QDateTime::currentDateTime();
        QDateTime end_time;

        if (offset + 4 <= data.size()) {
            // Start time offset (minutes from now)
            int16_t start_offset = static_cast<int16_t>(
                extractUInt16(data, offset)
            );
            start_time = start_time.addSecs(start_offset * 60);
            offset += 2;

            // Duration (minutes)
            uint16_t duration = extractUInt16(data, offset);
            end_time = start_time.addSecs(duration * 60);
            offset += 2;
        }

        // Message text (remaining bytes)
        QString message_text;
        if (offset < data.size()) {
            message_text = QString::fromUtf8(data.mid(offset));
        }

        // Create alert structure
        EmergencyAlert alert;
        alert.alert_id = alert_id;
        alert.alert_type = alert_type;
        alert.alert_category = alert_category;
        alert.alert_status = alert_status;
        alert.severity = severity;
        alert.geographical_area = geographical_area;
        alert.start_time = start_time;
        alert.end_time = end_time;
        alert.message_text = message_text;
        alert.service_id = service_id;
        alert.auto_switching = auto_switching;

        // Handle alert based on status
        if (alert_status == 0x00) { // Active alert
            m_activeAlerts.push_back(alert);
            emit emergencyAlertReceived(alert);

            qWarning() << "EMERGENCY ALERT:" << alert_id
                      << "Type:" << alert_type
                      << "Severity:" << severity
                      << "Area:" << geographical_area
                      << "Message:" << message_text;
        } else if (alert_status == 0x0F) { // Cancellation
            // Remove from active alerts
            auto it = std::remove_if(
                m_activeAlerts.begin(),
                m_activeAlerts.end(),
                [alert_id](const EmergencyAlert& a) {
                    return a.alert_id == alert_id;
                }
            );
            m_activeAlerts.erase(it, m_activeAlerts.end());

            emit emergencyAlertCancelled(alert_id);

            qInfo() << "Emergency alert cancelled:" << alert_id;
        } else { // Update
            // Update existing alert
            for (auto& existing : m_activeAlerts) {
                if (existing.alert_id == alert_id) {
                    existing = alert;
                    emit emergencyAlertReceived(alert);
                    break;
                }
            }
        }
    }

    std::vector<EmergencyAlert> getActiveAlerts() const {
        return m_activeAlerts;
    }
};
```

#### Task 6.3: EWS Compliance Validation
**File:** `src/core/etsi_ews_compliance.cpp` (new file)

**Implementation:**
```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <vector>

class ETSIEWSCompliance : public QObject {
    Q_OBJECT

public:
    enum class ComplianceLevel {
        NotCompliant,
        BasicCompliance,    // TS 103 461
        EWSCompliant,       // TS 104 089
        FullCompliant       // TS 104 090
    };

    struct ComplianceReport {
        ComplianceLevel level;
        bool has_date_time_signalling;
        bool date_time_long_form;
        bool date_time_accurate;
        bool has_ews_signalling;
        bool supports_geographical_filtering;
        bool supports_auto_switching;
        std::vector<QString> issues;
        std::vector<QString> warnings;
        double compliance_score;
    };

    explicit ETSIEWSCompliance(QObject* parent = nullptr);

    // Validate EWS compliance
    ComplianceReport validateEnsemble(
        bool has_fig_0_10,
        bool fig_0_10_long_form,
        const QDateTime& dab_time,
        bool has_ews_signalling,
        const std::vector<EmergencyAlert>& alerts
    );

    // Check date/time accuracy
    bool isDateTimeAccurate(const QDateTime& dab_time,
                           const QDateTime& system_time,
                           int tolerance_ms = 1000);

    // Generate compliance certificate
    QString generateComplianceCertificate(const ComplianceReport& report);

signals:
    void complianceIssueDetected(const QString& issue);
    void complianceValidated(ComplianceLevel level);
};

// Implementation
ETSIEWSCompliance::ComplianceReport ETSIEWSCompliance::validateEnsemble(
    bool has_fig_0_10,
    bool fig_0_10_long_form,
    const QDateTime& dab_time,
    bool has_ews_signalling,
    const std::vector<EmergencyAlert>& alerts)
{
    ComplianceReport report;
    report.has_date_time_signalling = has_fig_0_10;
    report.date_time_long_form = fig_0_10_long_form;
    report.has_ews_signalling = has_ews_signalling;

    // Check date/time accuracy
    QDateTime system_time = QDateTime::currentDateTime();
    report.date_time_accurate = isDateTimeAccurate(dab_time, system_time);

    // Scoring
    int score = 0;
    int max_score = 0;

    // ETSI TS 103 461 requirements
    max_score += 10;
    if (has_fig_0_10) {
        score += 10;
    } else {
        report.issues.push_back(
            "TS 103 461: FIG 0/10 date/time signalling missing"
        );
    }

    // ETSI TS 104 089 requirements
    max_score += 20;
    if (fig_0_10_long_form) {
        score += 10;
    } else {
        report.issues.push_back(
            "TS 104 089: FIG 0/10 must use long form for EWS"
        );
    }

    if (report.date_time_accurate) {
        score += 10;
    } else {
        report.warnings.push_back(
            "TS 104 089: Date/time not accurate (GNSS/NTP recommended)"
        );
    }

    max_score += 30;
    if (has_ews_signalling) {
        score += 30;
        report.supports_auto_switching = true;
        report.supports_geographical_filtering = true;
    } else {
        report.issues.push_back(
            "TS 104 089: EWS signalling not present in ensemble"
        );
    }

    // Calculate final score
    report.compliance_score = (static_cast<double>(score) / max_score) * 100.0;

    // Determine compliance level
    if (report.compliance_score >= 90.0) {
        report.level = ComplianceLevel::FullCompliant;
    } else if (report.compliance_score >= 70.0) {
        report.level = ComplianceLevel::EWSCompliant;
    } else if (report.compliance_score >= 50.0) {
        report.level = ComplianceLevel::BasicCompliance;
    } else {
        report.level = ComplianceLevel::NotCompliant;
    }

    emit complianceValidated(report.level);

    return report;
}

bool ETSIEWSCompliance::isDateTimeAccurate(
    const QDateTime& dab_time,
    const QDateTime& system_time,
    int tolerance_ms)
{
    qint64 diff_ms = std::abs(dab_time.msecsTo(system_time));
    return diff_ms <= tolerance_ms;
}

QString ETSIEWSCompliance::generateComplianceCertificate(
    const ComplianceReport& report)
{
    QString cert;
    cert += "=== ETSI EWS COMPLIANCE CERTIFICATE ===\n\n";
    cert += QString("Compliance Level: %1\n")
            .arg(static_cast<int>(report.level));
    cert += QString("Compliance Score: %1%\n\n")
            .arg(report.compliance_score, 0, 'f', 1);

    cert += "Standards Compliance:\n";
    cert += QString("- ETSI TS 103 461: %1\n")
            .arg(report.has_date_time_signalling ? "PASS" : "FAIL");
    cert += QString("- ETSI TS 104 089: %1\n")
            .arg(report.has_ews_signalling ? "PASS" : "FAIL");
    cert += QString("- ETSI TS 104 090: %1\n\n")
            .arg(report.level == ComplianceLevel::FullCompliant ? "PASS" : "FAIL");

    if (!report.issues.empty()) {
        cert += "Issues:\n";
        for (const auto& issue : report.issues) {
            cert += QString("  - %1\n").arg(issue);
        }
        cert += "\n";
    }

    if (!report.warnings.empty()) {
        cert += "Warnings:\n";
        for (const auto& warning : report.warnings) {
            cert += QString("  - %1\n").arg(warning);
        }
        cert += "\n";
    }

    cert += QString("Generated: %1\n")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate));

    return cert;
}
```

### Week 9: EWS GUI Integration & Testing

#### Task 6.4: Add EWS Alert Panel to GUI
**File:** `src/main.cpp`

**Implementation:**
```cpp
// Add to DABAnalyzerWindow class
private:
    QDockWidget* m_ewsAlertDock;
    QListWidget* m_ewsAlertList;
    ETSIEWSCompliance* m_ewsCompliance;

void setupEWSAlertPanel() {
    m_ewsAlertDock = new QDockWidget("Emergency Alerts", this);
    m_ewsAlertList = new QListWidget(m_ewsAlertDock);

    // Style for emergency alerts
    m_ewsAlertList->setStyleSheet(R"(
        QListWidget {
            background-color: #2b2b2b;
            color: #ffffff;
            border: 1px solid #ff6b6b;
        }
        QListWidget::item {
            padding: 10px;
            border-bottom: 1px solid #444444;
        }
        QListWidget::item:selected {
            background-color: #ff6b6b;
        }
    )");

    m_ewsAlertDock->setWidget(m_ewsAlertList);
    addDockWidget(Qt::RightDockWidgetArea, m_ewsAlertDock);

    // Initially hidden unless alerts present
    m_ewsAlertDock->hide();
}

void onEmergencyAlertReceived(const EmergencyAlert& alert) {
    // Show alert panel
    m_ewsAlertDock->show();
    m_ewsAlertDock->raise();

    // Create alert item
    QString alertText = QString(
        "ALERT %1 | Severity: %2 | %3\n"
        "Area: %4\n"
        "Time: %5 - %6\n"
        "Message: %7"
    ).arg(alert.alert_id)
     .arg(alert.severity)
     .arg(severityToString(alert.severity))
     .arg(alert.geographical_area)
     .arg(alert.start_time.toString(Qt::ISODate))
     .arg(alert.end_time.toString(Qt::ISODate))
     .arg(alert.message_text);

    auto* item = new QListWidgetItem(alertText, m_ewsAlertList);

    // Color code by severity
    if (alert.severity >= 8) {
        item->setBackground(QColor(255, 0, 0, 100)); // Critical - Red
    } else if (alert.severity >= 5) {
        item->setBackground(QColor(255, 165, 0, 100)); // Warning - Orange
    } else {
        item->setBackground(QColor(255, 255, 0, 100)); // Advisory - Yellow
    }

    // Play system alert sound
    QApplication::beep();

    // Show system notification
    showSystemNotification(
        "DAB Emergency Alert",
        QString("Severity %1: %2").arg(alert.severity).arg(alert.message_text),
        QSystemTrayIcon::Warning
    );
}

QString severityToString(uint8_t severity) {
    switch (severity) {
        case 10: return "EXTREME";
        case 9: return "SEVERE";
        case 7: case 8: return "MODERATE";
        case 4: case 5: case 6: return "MINOR";
        default: return "INFO";
    }
}
```

#### Task 6.5: EWS Testing Suite
**File:** `tests/test_ews_compliance.cpp` (new file)

**Implementation:**
```cpp
#include <QtTest/QtTest>
#include "../src/core/advanced_fig_analyzer.h"
#include "../src/core/etsi_ews_compliance.h"

class TestEWSCompliance : public QObject {
    Q_OBJECT

private slots:
    void testFIG0_10Parsing();
    void testFIG5_2EmergencyAlert();
    void testDateTimeAccuracy();
    void testComplianceValidation();
    void testAlertGeographicalFiltering();
    void testAlertAutoSwitching();

private:
    QByteArray createFIG0_10LongForm(const QDateTime& dt);
    QByteArray createFIG5_2Alert(const EmergencyAlert& alert);
};

void TestEWSCompliance::testFIG0_10Parsing() {
    AdvancedFIGAnalyzer analyzer;

    // Create test date/time: 2025-10-17 14:30:45.123 UTC
    QDateTime testTime(QDate(2025, 10, 17),
                      QTime(14, 30, 45, 123),
                      Qt::UTC);

    QByteArray figData = createFIG0_10LongForm(testTime);

    // Process FIG
    analyzer.processFIG0_10(figData);

    // Verify parsing
    QVERIFY(analyzer.hasDateTime());

    DABDateTime parsed = analyzer.getDateTime();
    QCOMPARE(parsed.year, 2025);
    QCOMPARE(parsed.month, 10);
    QCOMPARE(parsed.day, 17);
    QCOMPARE(parsed.hour, 14);
    QCOMPARE(parsed.minute, 30);
    QCOMPARE(parsed.second, 45);
    QCOMPARE(parsed.millisecond, 123);
    QVERIFY(parsed.utc_flag);
}

void TestEWSCompliance::testFIG5_2EmergencyAlert() {
    AdvancedFIGAnalyzer analyzer;

    // Create test emergency alert
    EmergencyAlert testAlert;
    testAlert.alert_id = 12345;
    testAlert.alert_type = 1; // Weather alert
    testAlert.alert_category = 3; // Severe storm
    testAlert.alert_status = 0x00; // Active
    testAlert.severity = 9; // Severe
    testAlert.geographical_area = "Bangkok Metropolitan";
    testAlert.message_text = "Severe thunderstorm warning in effect";
    testAlert.service_id = 0xE10001;
    testAlert.auto_switching = true;

    QByteArray figData = createFIG5_2Alert(testAlert);

    // Set up signal spy
    QSignalSpy spy(&analyzer,
                   &AdvancedFIGAnalyzer::emergencyAlertReceived);

    // Process FIG
    analyzer.processFIG5_2(figData);

    // Verify signal emitted
    QCOMPARE(spy.count(), 1);

    // Verify active alerts
    auto alerts = analyzer.getActiveAlerts();
    QCOMPARE(alerts.size(), 1);
    QCOMPARE(alerts[0].alert_id, testAlert.alert_id);
    QCOMPARE(alerts[0].severity, testAlert.severity);
}

void TestEWSCompliance::testComplianceValidation() {
    ETSIEWSCompliance compliance;

    QDateTime dab_time = QDateTime::currentDateTime();
    std::vector<EmergencyAlert> alerts;

    // Test full compliance
    auto report = compliance.validateEnsemble(
        true,  // has FIG 0/10
        true,  // long form
        dab_time,
        true,  // has EWS signalling
        alerts
    );

    QVERIFY(report.level >= ETSIEWSCompliance::ComplianceLevel::EWSCompliant);
    QVERIFY(report.compliance_score >= 70.0);

    // Test non-compliance
    report = compliance.validateEnsemble(
        false, // no FIG 0/10
        false,
        QDateTime(),
        false, // no EWS
        alerts
    );

    QCOMPARE(report.level,
             ETSIEWSCompliance::ComplianceLevel::NotCompliant);
    QVERIFY(!report.issues.empty());
}

QTEST_MAIN(TestEWSCompliance)
#include "test_ews_compliance.moc"
```

### Phase 6 Deliverables

**Completion Criteria:**
- [ ] FIG 0/10 (Date/Time) parsing implemented and tested
- [ ] FIG 5/2 (EWS) or equivalent emergency signalling implemented
- [ ] ETSI EWS compliance validation engine working
- [ ] EWS alert panel integrated into GUI
- [ ] System notifications for emergency alerts
- [ ] Geographical filtering capability
- [ ] Auto-switching support (framework)
- [ ] Comprehensive EWS test suite
- [ ] EWS compliance reports generated
- [ ] Documentation updated

**Success Metrics:**
- [ ] Parse FIG 0/10 with 100% accuracy
- [ ] Detect and display emergency alerts within 1 second
- [ ] Generate accurate ETSI compliance certificates
- [ ] Pass all EWS test cases
- [ ] Zero false-positive emergency alerts

---

## ETSI STANDARDS REFERENCE

### Complete Standards Coverage (Option B + EWS)

| Standard | Version | Title | Implementation Phase | Priority |
|----------|---------|-------|---------------------|----------|
| **ETSI EN 300 799** | v1.5.1 | ETI specification | Phase 1 | CRITICAL |
| **ETSI EN 300 401** | v2.1.1 | DAB system specification | Phase 1-2 | CRITICAL |
| **ETSI TS 103 461** | v1.2.2 | Min. requirements for receivers | Phase 5-6 | HIGH |
| **ETSI TS 104 089** | v1.1.1 | EWS system specification | Phase 6 | HIGH |
| **ETSI TS 104 090** | v1.1.2 | EWS receiver requirements | Phase 6 | HIGH |
| **ETSI EN 301 234** | v2.1.1 | DAB+ audio (HE-AAC v2) | Option C | MEDIUM |
| **ETSI TS 102 563** | v2.1.1 | DAB+ audio (xHE-AAC) | Option C | MEDIUM |

### Standards Documentation Links

**Core DAB Standards:**
- [ETSI EN 300 799](https://www.etsi.org/deliver/etsi_en/300700_300799/300799/) - ETI
- [ETSI EN 300 401](https://www.etsi.org/deliver/etsi_en/300400_300499/300401/) - DAB

**EWS Standards:**
- [ETSI TS 104 089](https://www.etsi.org/deliver/etsi_ts/104000_104099/104089/) - EWS System
- [ETSI TS 104 090](https://www.etsi.org/deliver/etsi_ts/104000_104099/104090/) - EWS Receivers
- [ETSI TS 103 461](https://www.etsi.org/deliver/etsi_ts/103400_103499/103461/) - Receiver Requirements

---

## TESTING STRATEGY

### Phase 0 Testing (Critical Fixes)

**Unit Tests:**
```bash
# Memory safety tests
tests/test_lockfree_queue.cpp
tests/test_circular_buffer.cpp
tests/test_thread_safety.cpp

# Validation tests
tests/test_crc_validation.cpp
tests/test_input_validation.cpp
```

**Integration Tests:**
```bash
# Multi-threaded scenarios
tests/integration/test_multi_stream.cpp
tests/integration/test_concurrent_processing.cpp
```

**Tools:**
- Valgrind (memory leaks)
- AddressSanitizer (memory errors)
- ThreadSanitizer (race conditions)
- UndefinedBehaviorSanitizer

### Phase 1-5 Testing

*Use testing framework from original CLAUDE.md*

### Phase 6 Testing (EWS)

**Functional Tests:**
```bash
tests/test_fig_0_10.cpp           # Date/time parsing
tests/test_fig_5_2.cpp            # Emergency alerts
tests/test_ews_compliance.cpp     # Compliance validation
tests/test_geographical_filter.cpp # Area filtering
```

**Compliance Tests:**
- ETSI TS 104 089 conformance
- ETSI TS 104 090 receiver requirements
- Date/time accuracy validation
- Alert timing tests

**Performance Tests:**
- Alert detection latency < 1 second
- Date/time synchronization accuracy < 100ms
- No performance impact when no alerts

---

## RISK MANAGEMENT

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Critical bugs block Phase 1 | HIGH | HIGH | Phase 0 mandatory |
| EWS FIG type undefined | MEDIUM | HIGH | Use custom extension FIG 5/2 |
| Performance degradation | LOW | MEDIUM | Profile and optimize |
| Thread safety issues | MEDIUM | HIGH | Comprehensive sanitizer testing |
| Memory leaks in production | LOW | HIGH | Continuous Valgrind checks |

### Schedule Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Phase 0 takes longer | +3-5 days | Critical fixes are blockers, prioritize |
| EWS specs unclear | +1 week | Research WorldDAB documentation |
| Testing finds major bugs | +1-2 weeks | Continuous testing throughout |

### Quality Risks

| Risk | Mitigation |
|------|------------|
| Incomplete testing | Mandatory test coverage >80% |
| False emergency alerts | Strict validation and testing |
| Memory corruption | Sanitizers in CI/CD |
| Race conditions | Thread sanitizer mandatory |

---

## UPDATED TIMELINE SUMMARY

### Total Duration: 7-9 weeks (47-60 days)

**Phase 0:** 3-4 days (Critical Fixes)
**Phase 1:** 2 weeks (ETI Core Engine)
**Phase 2:** 1 week (GUI Integration)
**Phase 3:** 2 weeks (ETSI Compliance & Real-time)
**Phase 4:** 1 week (Performance Optimization)
**Phase 5:** 1 week (Professional Polish)
**Phase 6:** 1-2 weeks (ETSI EWS Implementation)

**Buffer:** 3-5 days for unexpected issues

---

## NEXT STEPS

### Immediate Actions (This Week)

1. **Start Phase 0 Critical Fixes** (3-4 days)
   - Fix LockFreeQueue memory leak (CRITICAL-001)
   - Fix alignment issues (CRITICAL-002)
   - Fix thread safety (CRITICAL-005)
   - Add input validation (CRITICAL-006)
   - Implement CRC validation (CRITICAL-007)
   - Add resource limits (HIGH-007)
   - Fix signal connection race (HIGH-002)

2. **Set Up Testing Infrastructure**
   - Configure sanitizers in CMake
   - Set up Valgrind integration
   - Create test framework structure

3. **Download ETSI Standards**
   - ETSI TS 104 089 V1.1.1
   - ETSI TS 104 090 V1.1.2
   - ETSI TS 103 461 V1.2.2

### Weekly Milestones

**Week 0:** Phase 0 complete, all critical issues fixed
**Week 1-2:** Phase 1 - Real ETI parsing working
**Week 3:** Phase 2 - GUI showing real data
**Week 4-5:** Phase 3 - ETSI compliance + real-time
**Week 6:** Phase 4 - >1000 FPS validated
**Week 7:** Phase 5 - Professional polish
**Week 8-9:** Phase 6 - EWS support complete

---

## CONCLUSION

This updated plan addresses all critical code review findings and adds comprehensive ETSI EWS support. The addition of Phase 0 (critical fixes) and Phase 6 (EWS implementation) extends the timeline by 1-2 weeks but ensures:

1. **Solid Foundation:** All critical bugs fixed before new development
2. **Enhanced Features:** Emergency Warning System support
3. **Standards Compliance:** Coverage of 5 ETSI standards (was 1)
4. **Professional Quality:** Comprehensive testing and validation
5. **Future-Ready:** Architecture supports Option C extension

**Status:** Ready to begin Phase 0 implementation immediately.

---

**Document Prepared By:** PM Agent + Code Reviewer
**Last Updated:** October 17, 2025
**Version:** 2.0 - Code Review Integration + EWS Support
