# Phase C: UDP Live Streaming - PDCA Plan

**Phase:** Phase C - UDP Live Streaming Implementation  
**Duration:** 2-3 days  
**Methodology:** PDCA (Plan-Do-Check-Act)  
**Dependencies:** Phase A (ETIReaderThread) + Phase B (SharedETIData signals)

---

## 📋 **PLAN Phase**

### **Objectives**
1. Implement UDP socket receiver in ETIReaderThread
2. Support ETI-over-IP streaming (UDP unicast and multicast)
3. Add input source selector to GUI (File / UDP toggle)
4. Maintain 41 fps real-time processing (24ms frame interval)
5. Handle network errors gracefully (packet loss, late frames)

### **Current State Analysis**

**What Works (Phase A + B Complete):**
- ✅ ETIReaderThread with QTimer event loop (24ms intervals)
- ✅ File-based ETI processing operational
- ✅ SharedETIData receiving frame updates
- ✅ Signal-based widget updates
- ✅ Thread-safe architecture

**What Needs Implementation:**
- ❌ UDP socket creation and binding
- ❌ Multicast group subscription
- ❌ Network frame reception
- ❌ GUI input source selector
- ❌ Network error handling

**Current Input Flow (File Mode):**
```
QFile (eti_file.eti) → readFrame() → 6144 bytes → processFrame()
                                                    ↓
                                          ETIReaderThread::processNextFrame()
```

**Target Input Flow (UDP Mode):**
```
QUdpSocket (239.0.0.1:5004) → receiveFrame() → 6144 bytes → processFrame()
                                                              ↓
                                                ETIReaderThread::processNextFrame()
```

### **ETI-over-IP Standards**

**Common ETI Streaming Configurations:**

| **Parameter** | **Typical Value** | **Notes** |
|---------------|-------------------|-----------|
| **Protocol** | UDP | Low latency, real-time |
| **Multicast Address** | 239.0.0.0 - 239.255.255.255 | IPv4 multicast range |
| **Common Addresses** | 239.0.0.1, 239.192.0.1 | DAB streaming typical |
| **Port** | 5004, 12000, 9200 | Configurable |
| **Frame Size** | 6144 bytes (ETI-LI) | ETSI EN 300 799 |
| **Frame Rate** | 41.67 fps (24ms) | 1000 / 24 = 41.67 |
| **Bitrate** | ~2 Mbps | 6144 * 8 * 41.67 = 2.05 Mbps |

**Network Considerations:**
- **Jitter Buffer:** Not required (stateless frame processing)
- **Packet Loss:** Log warning, continue processing
- **Out-of-Order:** ETI frames are sequential, order matters
- **Late Frames:** Process anyway (QTimer drives pace, not network)

### **Success Criteria**
- ✅ UDP socket binds to specified port
- ✅ Multicast group subscription works
- ✅ Receives ETI frames from network (6144 bytes)
- ✅ Processes frames at ~41 fps (real-time)
- ✅ GUI source selector switches File ↔ UDP
- ✅ Network errors handled without crash
- ✅ SharedETIData receives updates from UDP source
- ✅ Widgets update in real-time from network stream

---

## 🎯 **IMPLEMENTATION TASKS**

### **Task C.1: Update ETIReaderThread for UDP Support**

**Files to Modify:**
1. `src/core/eti_reader_thread.hpp` - Add UDP members
2. `src/core/eti_reader_thread.cpp` - Implement UDP methods
3. `src/main.cpp` - Add GUI source selector

#### **C.1.1: Add UDP Members to Header**

**File:** `src/core/eti_reader_thread.hpp`

**Add to class definition:**
```cpp
private:
    // === Phase C: UDP Streaming Support ===
    QUdpSocket* m_udpSocket;          // UDP socket for network streaming
    QString m_multicastAddress;       // Multicast group address
    quint16 m_udpPort;                // UDP port (default 12000)
    bool m_useMulticast;              // Enable multicast subscription
    QByteArray m_networkBuffer;       // Buffer for partial frames
    int m_packetsReceived;            // Network statistics
    int m_packetsLost;                // Packet loss counter
```

**Add public methods:**
```cpp
public:
    // Phase C: UDP configuration
    void setUDPPort(quint16 port);
    void setMulticastAddress(const QString& address);
    void setUseMulticast(bool enable);
    
    // Network statistics
    int getPacketsReceived() const;
    int getPacketsLost() const;
```

#### **C.1.2: Implement UDP Socket Methods**

**File:** `src/core/eti_reader_thread.cpp`

**Initialize in Constructor:**
```cpp
ETIReaderThread::ETIReaderThread(QObject *parent)
    : QThread(parent)
    // ... existing initialization ...
    , m_udpSocket(nullptr)
    , m_udpPort(12000)  // Default DAB streaming port
    , m_useMulticast(false)
    , m_packetsReceived(0)
    , m_packetsLost(0)
{
    // Existing code...
}
```

**Implement openUDPSocket():**
```cpp
bool ETIReaderThread::openUDPSocket()
{
    // Create UDP socket in worker thread context
    m_udpSocket = new QUdpSocket(this);
    
    // Bind to port
    if (!m_udpSocket->bind(QHostAddress::AnyIPv4, m_udpPort, 
                           QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qCritical() << "Phase C: Failed to bind UDP socket to port" << m_udpPort
                    << "Error:" << m_udpSocket->errorString();
        delete m_udpSocket;
        m_udpSocket = nullptr;
        return false;
    }
    
    qDebug() << "Phase C: UDP socket bound to port" << m_udpPort;
    
    // Join multicast group if enabled
    if (m_useMulticast && !m_multicastAddress.isEmpty()) {
        QHostAddress groupAddress(m_multicastAddress);
        if (m_udpSocket->joinMulticastGroup(groupAddress)) {
            qDebug() << "Phase C: Joined multicast group" << m_multicastAddress;
        } else {
            qWarning() << "Phase C: Failed to join multicast group" 
                       << m_multicastAddress
                       << "Error:" << m_udpSocket->errorString();
            // Continue anyway - unicast might still work
        }
    }
    
    // Set socket buffer size for high throughput
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 
                                  256 * 1024); // 256 KB buffer
    
    m_packetsReceived = 0;
    m_packetsLost = 0;
    
    return true;
}
```

**Implement closeUDPSocket():**
```cpp
void ETIReaderThread::closeUDPSocket()
{
    if (m_udpSocket) {
        // Leave multicast group if joined
        if (m_useMulticast && !m_multicastAddress.isEmpty()) {
            m_udpSocket->leaveMulticastGroup(QHostAddress(m_multicastAddress));
            qDebug() << "Phase C: Left multicast group" << m_multicastAddress;
        }
        
        m_udpSocket->close();
        delete m_udpSocket;
        m_udpSocket = nullptr;
        
        qDebug() << "Phase C: UDP socket closed. Stats - Received:" 
                 << m_packetsReceived << "Lost:" << m_packetsLost;
    }
}
```

**Implement receiveFrame():**
```cpp
bool ETIReaderThread::receiveFrame(QByteArray& frameData)
{
    if (!m_udpSocket || !m_udpSocket->hasPendingDatagrams()) {
        // No data available (this is normal - QTimer fires faster than network)
        return false;
    }
    
    // Read all pending datagrams
    while (m_udpSocket->hasPendingDatagrams()) {
        qint64 datagramSize = m_udpSocket->pendingDatagramSize();
        QByteArray datagram;
        datagram.resize(datagramSize);
        
        qint64 bytesRead = m_udpSocket->readDatagram(datagram.data(), datagram.size());
        
        if (bytesRead < 0) {
            qWarning() << "Phase C: UDP read error:" << m_udpSocket->errorString();
            m_packetsLost++;
            continue;
        }
        
        m_packetsReceived++;
        
        // Check if datagram is exactly one ETI frame
        if (bytesRead == ETI_FRAME_SIZE) {
            frameData = datagram;
            return true;
        }
        
        // Handle fragmented frames (if protocol sends multiple packets per frame)
        m_networkBuffer.append(datagram);
        
        if (m_networkBuffer.size() >= ETI_FRAME_SIZE) {
            frameData = m_networkBuffer.left(ETI_FRAME_SIZE);
            m_networkBuffer.remove(0, ETI_FRAME_SIZE);
            return true;
        }
    }
    
    // No complete frame available yet
    return false;
}
```

**Update run() method:**
```cpp
void ETIReaderThread::run()
{
    // Initialize in worker thread context
    m_processor = std::make_unique<EnhancedETIProcessorQt>();
    m_figAnalyzer = std::make_unique<AdvancedFIGAnalyzer>();
    
    m_currentFrame = 0;
    m_framesProcessed = 0;
    m_elapsedTimer.start();
    
    // Open input source (File or UDP)
    bool opened = false;
    {
        QMutexLocker locker(&m_controlMutex);
        if (m_inputMode == InputMode::FileMode) {
            opened = openFile();
        } else {
            opened = openUDPSocket();  // Phase C: UDP support
        }
    }
    
    if (!opened) {
        emit processingError("Failed to open input source");
        return;
    }
    
    emit processingStarted();
    
    // Create QTimer in worker thread context (Pattern 3)
    QTimer timer;
    timer.setInterval(FRAME_INTERVAL_MS);  // 24ms = 41 fps
    timer.setTimerType(Qt::PreciseTimer);
    
    connect(&timer, &QTimer::timeout, this, &ETIReaderThread::processNextFrame);
    
    timer.start();
    
    // Enter event loop (blocks until quit())
    exec();
    
    // Cleanup
    timer.stop();
    
    {
        QMutexLocker locker(&m_controlMutex);
        if (m_inputMode == InputMode::FileMode) {
            closeFile();
        } else {
            closeUDPSocket();  // Phase C: UDP cleanup
        }
    }
    
    m_processingTimeMs = m_elapsedTimer.elapsed();
    emit processingComplete(m_framesProcessed, m_processingTimeMs);
}
```

---

### **Task C.2: Add GUI Input Source Selector**

**File:** `src/main.cpp`

**Add to GUI Setup:**
```cpp
// Phase C: Input source selector in menu bar
QMenu* inputMenu = menuBar()->addMenu("&Input");

QActionGroup* inputGroup = new QActionGroup(this);

QAction* fileInputAction = new QAction("File Input", this);
fileInputAction->setCheckable(true);
fileInputAction->setChecked(true);  // Default to file
fileInputAction->setActionGroup(inputGroup);

QAction* udpInputAction = new QAction("UDP Streaming", this);
udpInputAction->setCheckable(true);
udpInputAction->setActionGroup(inputGroup);

inputMenu->addAction(fileInputAction);
inputMenu->addAction(udpInputAction);
inputMenu->addSeparator();

QAction* udpSettingsAction = new QAction("UDP Settings...", this);
inputMenu->addAction(udpSettingsAction);

// Connect actions
connect(fileInputAction, &QAction::triggered, [this]() {
    if (m_readerThread) {
        m_readerThread->setInputMode(ETIReaderThread::InputMode::FileMode);
        qDebug() << "Phase C: Switched to File input mode";
    }
});

connect(udpInputAction, &QAction::triggered, [this]() {
    if (m_readerThread) {
        m_readerThread->setInputMode(ETIReaderThread::InputMode::UDPMode);
        qDebug() << "Phase C: Switched to UDP input mode";
    }
});

connect(udpSettingsAction, &QAction::triggered, this, &DABAnalyzerWindow::showUDPSettings);
```

**Add UDP Settings Dialog:**
```cpp
void DABAnalyzerWindow::showUDPSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle("UDP Streaming Settings");
    
    QFormLayout* layout = new QFormLayout(&dialog);
    
    // Port setting
    QSpinBox* portSpinBox = new QSpinBox(&dialog);
    portSpinBox->setRange(1024, 65535);
    portSpinBox->setValue(12000);  // Default
    layout->addRow("UDP Port:", portSpinBox);
    
    // Multicast address
    QLineEdit* addressEdit = new QLineEdit("239.0.0.1", &dialog);
    layout->addRow("Multicast Address:", addressEdit);
    
    // Enable multicast checkbox
    QCheckBox* multicastCheck = new QCheckBox("Enable Multicast", &dialog);
    multicastCheck->setChecked(false);
    layout->addRow(multicastCheck);
    
    // Info label
    QLabel* infoLabel = new QLabel(
        "Common ETI streaming ports: 5004, 12000, 9200\n"
        "Multicast range: 239.0.0.0 - 239.255.255.255", &dialog);
    infoLabel->setWordWrap(true);
    layout->addRow(infoLabel);
    
    // Buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        if (m_readerThread) {
            m_readerThread->setUDPPort(portSpinBox->value());
            m_readerThread->setMulticastAddress(addressEdit->text());
            m_readerThread->setUseMulticast(multicastCheck->isChecked());
            
            qDebug() << "Phase C: UDP settings updated - Port:" << portSpinBox->value()
                     << "Multicast:" << addressEdit->text()
                     << "Enabled:" << multicastCheck->isChecked();
        }
    }
}
```

---

## 🚀 **DO Phase (Implementation)**

### **Step-by-Step Execution**

**Step 1:** Update `eti_reader_thread.hpp`
- Add UDP-related member variables
- Add public UDP configuration methods
- Add Qt Network includes

**Step 2:** Update `eti_reader_thread.cpp`
- Implement `openUDPSocket()`, `closeUDPSocket()`, `receiveFrame()`
- Update constructor for UDP initialization
- Modify `run()` to handle UDP mode

**Step 3:** Update `CMakeLists.txt`
- Add Qt6::Network to target_link_libraries

**Step 4:** Update `main.cpp`
- Add Input menu with File/UDP toggle
- Implement UDP settings dialog
- Add `showUDPSettings()` slot

**Step 5:** Build and test
- Compile with zero errors
- Test file mode still works
- Test UDP mode with test stream

---

## ✅ **CHECK Phase (Validation)**

### **Functional Tests**

**Test 1: File Mode Regression**
- Load ETI file
- Verify processing still works
- Expected: No breaking changes

**Test 2: UDP Socket Binding**
- Set UDP port to 12000
- Start UDP mode
- Expected: Socket binds successfully

**Test 3: Multicast Subscription**
- Enable multicast (239.0.0.1)
- Start UDP mode
- Expected: Joins multicast group

**Test 4: Frame Reception**
- Send test ETI frames to UDP port
- Verify frames received
- Expected: 6144-byte frames processed

**Test 5: Real-Time Processing**
- Stream at 41 fps
- Monitor frame rate
- Expected: ~41 fps sustained

**Test 6: Network Error Handling**
- Stop stream mid-processing
- Expected: Graceful error, no crash

### **Performance Validation**

```bash
# Test with netcat (send test frames)
while true; do
    cat bkk_20062022_141637.eti | nc -u 127.0.0.1 12000
    sleep 0.024  # 24ms = 41 fps
done
```

**Metrics to Monitor:**
- CPU usage (<50% for UDP streaming)
- Frame rate consistency (41 ± 2 fps)
- Packet loss (log warnings)
- Memory usage (stable over time)

---

## 🔄 **ACT Phase (Improvements)**

### **If All Tests Pass**

**Actions:**
1. Git commit with message: "Phase C: UDP live streaming complete"
2. Create PHASE_C_COMPLETION_SUMMARY.md
3. Move to Phase D (specialized signals and throttling)

### **If Tests Fail**

**Common Issues & Fixes:**

**Issue 1:** UDP socket bind fails
- **Cause:** Port already in use
- **Fix:** Try different port, check with `netstat -an | grep 12000`

**Issue 2:** No frames received
- **Cause:** Firewall blocking UDP
- **Fix:** Check firewall rules, try localhost first

**Issue 3:** Multicast not working
- **Cause:** Router doesn't support multicast
- **Fix:** Test with unicast first, verify network config

**Issue 4:** Frame rate too slow
- **Cause:** Network jitter or processing overhead
- **Fix:** Check QTimer interval, optimize processFrame()

---

## 📊 **DELIVERABLES**

### **Code Changes**

1. **src/core/eti_reader_thread.hpp**
   - UDP socket members (~10 lines)
   - UDP configuration methods (~4 methods)

2. **src/core/eti_reader_thread.cpp**
   - openUDPSocket() implementation (~40 lines)
   - closeUDPSocket() implementation (~15 lines)
   - receiveFrame() implementation (~50 lines)
   - Updated run() method (~10 lines)

3. **src/main.cpp**
   - Input menu with File/UDP toggle (~30 lines)
   - UDP settings dialog (~60 lines)
   - showUDPSettings() slot (~1 method)

4. **CMakeLists.txt**
   - Add Qt6::Network linkage (~1 line)

### **Documentation**

- This PDCA plan document
- Phase C completion summary (at ACT phase)
- Inline code comments

---

## 📋 **NEXT STEPS**

After Phase C completion:
1. **Phase D:** Specialized signals and throttling (GUI optimization)
2. **Phase E:** Performance benchmarking and optimization
3. **Phase F:** DLS+ "Now Playing" metadata (original Phase 3B)

---

**Document Version:** 1.0  
**Created:** October 25, 2025  
**Status:** Ready for DO phase execution
