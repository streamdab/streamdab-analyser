# Live Stream Input Design - Network ETI Reception

> **Date:** 2025-10-23  
> **Feature:** Live ETI stream input (UDP/TCP/ZMQ protocols)  
> **Reference:** eti-tools, NetworkStreamReceiver implementation  
> **Target:** Tab 1 integration for real-time stream analysis

---

## Overview

Add professional live stream input capability to Tab 1, allowing users to analyze real-time ETI streams from network sources in addition to file-based analysis.

### Supported Protocols:

| Protocol | Status | Implementation | Use Case |
|----------|--------|----------------|----------|
| **Multicast UDP** | ✅ Ready | NetworkStreamReceiver | Primary broadcast distribution |
| **TCP** | ⚠️ Easy to add | QTcpSocket | Point-to-point streaming |
| **ZMQ TCP** | ⚠️ Needs ZMQ lib | ZeroMQ binding | High-performance IPC |
| **File** | ✅ Existing | Current implementation | Offline analysis |

---

## URL Format Specification

### Standard Formats:

```
Protocol Syntax:
- Multicast UDP:  udp://multicast_address:port
- TCP:            tcp://host:port  
- ZMQ TCP:        zmq+tcp://host:port
- File:           file:///absolute/path/to/file.eti
```

### Examples:

```
# DAB Ensemble Multicast (typical broadcast scenario)
udp://239.192.0.1:9200

# TCP Direct Connection (from modulator/multiplexer)
tcp://192.168.1.100:9200
tcp://dabmux.local:9201

# ZeroMQ TCP (from ODR-DabMux)
zmq+tcp://localhost:9201
zmq+tcp://192.168.1.50:9000

# Local File (existing functionality)
file:///home/user/captures/bangkok_ensemble.eti
file:///mnt/recordings/dab_stream_2025_10_23.eti
```

### Port Conventions:

| Port | Protocol | Typical Use |
|------|----------|-------------|
| 9200 | UDP/TCP | Default ETI-over-IP |
| 9201 | ZMQ | ODR-DabMux default ZMQ output |
| 12000 | UDP | Alternative ETI multicast |
| 9000 | ZMQ | Custom ZMQ configuration |

---

## UI Design - Live Stream Input Panel

### Location: Tab 1 Top Section (before Player/Decoder panels)

```
┌───────────────────────────────────────────────────────────────────────┐
│  Input Source                                                          │
├───────────────────────────────────────────────────────────────────────┤
│  ○ File      ● Network Stream                                         │
│                                                                        │
│  ┌─ File Input (when "File" selected) ────────────────────────────┐  │
│  │  File: [/home/user/bangkok.eti                             ] │  │
│  │  [Browse...] [Load File]                                         │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                        │
│  ┌─ Network Stream Input (when "Network Stream" selected) ────────┐  │
│  │  Stream URL:                                                     │  │
│  │  [udp://239.192.0.1:9200                                     ] │  │
│  │                                                                  │  │
│  │  Quick Examples: [UDP Multicast ▼]                              │  │
│  │    - udp://239.192.0.1:9200     (DAB Ensemble Multicast)        │  │
│  │    - tcp://192.168.1.100:9200   (TCP Direct)                    │  │
│  │    - zmq+tcp://localhost:9201   (ODR-DabMux ZMQ)                │  │
│  │                                                                  │  │
│  │  [Connect] [Disconnect]   Status: ● Connected  (LED indicator)  │  │
│  │                                                                  │  │
│  │  ┌─ Stream Statistics ────────────────────────────────────────┐ │  │
│  │  │  Frame Rate:        250.5 FPS                              │ │  │
│  │  │  Frames Received:   12,543 frames                          │ │  │
│  │  │  Frame Loss:        0.01% (2 frames)                       │ │  │
│  │  │  Network Latency:   15 ms (avg: 12 ms, max: 28 ms)        │ │  │
│  │  │  Buffer Status:     89% (890/1000 frames)                  │ │  │
│  │  │  Data Rate:         1.54 Mbit/s (6144 bytes/frame)         │ │  │
│  │  │  Uptime:            00:08:32                               │ │  │
│  │  └────────────────────────────────────────────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────────┘
```

---

## Widget Structure

### Qt Components:

```cpp
class LiveStreamInputPanel : public QGroupBox {
    Q_OBJECT

public:
    explicit LiveStreamInputPanel(QWidget* parent = nullptr);

private slots:
    void onSourceChanged(int index);  // File vs Network
    void onConnectClicked();
    void onDisconnectClicked();
    void onBrowseFileClicked();
    void onLoadFileClicked();
    void onQuickExampleSelected(int index);
    void updateStreamStatistics();

signals:
    void fileSelected(const QString& filePath);
    void streamConnected(const QString& url);
    void streamDisconnected();
    void streamError(const QString& error);
    void frameReceived(const QByteArray& frameData);

private:
    // Input source selection
    QRadioButton* m_fileRadio;
    QRadioButton* m_networkRadio;
    QStackedWidget* m_inputStack;  // Switch between File/Network UI

    // File input widgets
    QLineEdit* m_filePathEdit;
    QPushButton* m_browseButton;
    QPushButton* m_loadFileButton;

    // Network stream widgets
    QLineEdit* m_streamUrlEdit;
    QComboBox* m_quickExampleCombo;
    QPushButton* m_connectButton;
    QPushButton* m_disconnectButton;
    QLabel* m_statusLabel;
    QWidget* m_ledIndicator;  // Custom LED widget (green/red/yellow)

    // Stream statistics widgets
    QGroupBox* m_statsGroup;
    QLabel* m_frameRateLabel;
    QLabel* m_framesReceivedLabel;
    QLabel* m_frameLossLabel;
    QLabel* m_latencyLabel;
    QLabel* m_bufferStatusLabel;
    QLabel* m_dataRateLabel;
    QLabel* m_uptimeLabel;

    // Stream statistics timer
    QTimer* m_statsUpdateTimer;

    // Network stream receiver
    std::unique_ptr<eti::NetworkStreamReceiver> m_streamReceiver;
    eti::NetworkStreamConfig m_streamConfig;
    
    // Connection state
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Error
    };
    ConnectionState m_connectionState{ConnectionState::Disconnected};

    void setupUI();
    void updateConnectionState(ConnectionState state);
    QString parseProtocol(const QString& url);  // Extract protocol from URL
    bool validateStreamUrl(const QString& url);
    void startStatisticsUpdates();
    void stopStatisticsUpdates();
};
```

---

## Implementation Details

### 1. Protocol Parsing

```cpp
QString LiveStreamInputPanel::parseProtocol(const QString& url) {
    QUrl qurl(url);
    QString scheme = qurl.scheme().toLower();
    
    if (scheme == "udp") return "UDP";
    if (scheme == "tcp") return "TCP";
    if (scheme == "zmq+tcp" || scheme == "zmq") return "ZMQ";
    if (scheme == "file") return "FILE";
    
    return "UNKNOWN";
}

bool LiveStreamInputPanel::validateStreamUrl(const QString& url) {
    QUrl qurl(url);
    
    // Check scheme
    QString protocol = parseProtocol(url);
    if (protocol == "UNKNOWN") {
        QMessageBox::warning(this, "Invalid URL", 
            "Unsupported protocol. Use udp://, tcp://, or zmq+tcp://");
        return false;
    }
    
    // Check host and port
    if (qurl.host().isEmpty()) {
        QMessageBox::warning(this, "Invalid URL", 
            "Missing host address");
        return false;
    }
    
    if (qurl.port() <= 0) {
        QMessageBox::warning(this, "Invalid URL", 
            "Missing or invalid port number");
        return false;
    }
    
    return true;
}
```

### 2. Multicast UDP Connection (Existing)

```cpp
void LiveStreamInputPanel::connectMulticastUDP(const QString& url) {
    QUrl qurl(url);
    
    m_streamConfig.multicast_address = qurl.host();
    m_streamConfig.port = qurl.port();
    m_streamConfig.buffer_size = 1000;
    m_streamConfig.auto_reconnect = true;
    
    m_streamReceiver = std::make_unique<eti::NetworkStreamReceiver>(
        m_streamConfig, this);
    
    // Connect signals
    connect(m_streamReceiver.get(), &eti::NetworkStreamReceiver::frameReceived,
            this, &LiveStreamInputPanel::frameReceived);
    connect(m_streamReceiver.get(), &eti::NetworkStreamReceiver::connectionStateChanged,
            this, [this](bool connected) {
                updateConnectionState(connected ? ConnectionState::Connected 
                                                : ConnectionState::Disconnected);
            });
    connect(m_streamReceiver.get(), &eti::NetworkStreamReceiver::errorOccurred,
            this, [this](const QString& error) {
                updateConnectionState(ConnectionState::Error);
                emit streamError(error);
            });
    
    // Start reception
    if (m_streamReceiver->start()) {
        updateConnectionState(ConnectionState::Connecting);
        startStatisticsUpdates();
    } else {
        updateConnectionState(ConnectionState::Error);
        emit streamError("Failed to start multicast reception");
    }
}
```

### 3. TCP Connection (New Implementation)

```cpp
class TCPStreamReceiver : public QObject {
    Q_OBJECT

public:
    explicit TCPStreamReceiver(const QString& host, quint16 port, 
                              QObject* parent = nullptr);
    
    bool connect();
    void disconnect();
    
signals:
    void frameReceived(const QByteArray& frameData);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

private:
    QTcpSocket* m_socket;
    QString m_host;
    quint16 m_port;
    QByteArray m_buffer;
    
    void processBuffer();
    bool extractFrame(QByteArray& frame);
};

void TCPStreamReceiver::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    processBuffer();
}

void TCPStreamReceiver::processBuffer() {
    while (m_buffer.size() >= 6144) {
        QByteArray frame = m_buffer.left(6144);
        
        // Validate ETI frame (check sync pattern)
        if (isValidETIFrame(frame)) {
            emit frameReceived(frame);
            m_buffer.remove(0, 6144);
        } else {
            // Try to find sync pattern
            int syncPos = findETISyncPattern(m_buffer);
            if (syncPos >= 0) {
                m_buffer.remove(0, syncPos);
            } else {
                // No sync found, keep last 6143 bytes for partial frame
                if (m_buffer.size() > 6143) {
                    m_buffer.remove(0, m_buffer.size() - 6143);
                }
                break;
            }
        }
    }
}

bool TCPStreamReceiver::isValidETIFrame(const QByteArray& frame) {
    if (frame.size() != 6144) return false;
    
    // Check ETI-NI sync patterns
    const uint8_t* data = reinterpret_cast<const uint8_t*>(frame.data());
    uint32_t sync = (data[0] << 24) | (data[1] << 16) | 
                    (data[2] << 8) | data[3];
    
    return (sync == 0x0xFF1F49FF ||  // ETI-NI sync
            sync == 0x0xFF1FC4FF);   // ETI-NI alternate sync
}
```

### 4. ZMQ Connection (New - Requires libzmq)

```cpp
// Requires: sudo apt-get install libzmq3-dev
#include <zmq.hpp>

class ZMQStreamReceiver : public QObject {
    Q_OBJECT

public:
    explicit ZMQStreamReceiver(const QString& endpoint, 
                              QObject* parent = nullptr);
    
    bool connect();
    void disconnect();
    
signals:
    void frameReceived(const QByteArray& frameData);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

private:
    QString m_endpoint;
    std::unique_ptr<zmq::context_t> m_context;
    std::unique_ptr<zmq::socket_t> m_socket;
    QThread* m_receiverThread;
    std::atomic<bool> m_running{false};
    
    void receiveLoop();
};

bool ZMQStreamReceiver::connect() {
    try {
        m_context = std::make_unique<zmq::context_t>(1);
        m_socket = std::make_unique<zmq::socket_t>(*m_context, ZMQ_SUB);
        
        // Subscribe to all messages
        m_socket->setsockopt(ZMQ_SUBSCRIBE, "", 0);
        
        // Connect to endpoint
        std::string endpoint = m_endpoint.toStdString();
        m_socket->connect(endpoint);
        
        // Start receiver thread
        m_running = true;
        m_receiverThread = QThread::create([this]() { receiveLoop(); });
        m_receiverThread->start();
        
        emit connected();
        return true;
    } catch (const zmq::error_t& e) {
        emit errorOccurred(QString("ZMQ error: %1").arg(e.what()));
        return false;
    }
}

void ZMQStreamReceiver::receiveLoop() {
    while (m_running) {
        try {
            zmq::message_t message;
            if (m_socket->recv(&message, ZMQ_DONTWAIT)) {
                // Extract ETI frame from ZMQ message
                QByteArray frameData(
                    static_cast<const char*>(message.data()),
                    message.size()
                );
                
                if (frameData.size() == 6144) {
                    emit frameReceived(frameData);
                }
            }
            
            // Small sleep to avoid busy-wait
            QThread::msleep(1);
        } catch (const zmq::error_t& e) {
            if (e.num() != EAGAIN) {  // Ignore EAGAIN (would block)
                emit errorOccurred(QString("ZMQ recv error: %1").arg(e.what()));
            }
        }
    }
}
```

---

## Stream Statistics Display

### Update Implementation:

```cpp
void LiveStreamInputPanel::updateStreamStatistics() {
    if (!m_streamReceiver || m_connectionState != ConnectionState::Connected) {
        return;
    }
    
    // Get statistics from receiver
    auto stats = m_streamReceiver->getStatisticsSnapshot();
    
    // Update labels
    m_frameRateLabel->setText(QString("Frame Rate: %1 FPS")
        .arg(stats.current_frame_rate, 0, 'f', 1));
    
    m_framesReceivedLabel->setText(QString("Frames Received: %1 frames")
        .arg(stats.frames_received));
    
    m_frameLossLabel->setText(QString("Frame Loss: %1% (%2 frames)")
        .arg(stats.frame_loss_rate * 100, 0, 'f', 2)
        .arg(stats.frames_dropped));
    
    m_latencyLabel->setText(QString("Network Latency: %1 ms (avg: %2 ms, max: %3 ms)")
        .arg(stats.average_latency_us / 1000)
        .arg(stats.average_latency_us / 1000)
        .arg(stats.max_latency_us / 1000));
    
    // Calculate buffer percentage
    double bufferPercent = (double)m_streamReceiver->getBufferFillLevel() / 
                          m_streamConfig.buffer_size * 100.0;
    m_bufferStatusLabel->setText(QString("Buffer Status: %1% (%2/%3 frames)")
        .arg(bufferPercent, 0, 'f', 0)
        .arg(m_streamReceiver->getBufferFillLevel())
        .arg(m_streamConfig.buffer_size));
    
    // Calculate data rate
    double dataRateMbps = stats.current_frame_rate * 6144 * 8 / 1000000.0;
    m_dataRateLabel->setText(QString("Data Rate: %1 Mbit/s (6144 bytes/frame)")
        .arg(dataRateMbps, 0, 'f', 2));
    
    // Calculate uptime
    auto uptime = std::chrono::steady_clock::now() - stats.start_time;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    m_uptimeLabel->setText(QString("Uptime: %1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0')));
}

void LiveStreamInputPanel::startStatisticsUpdates() {
    m_statsUpdateTimer = new QTimer(this);
    connect(m_statsUpdateTimer, &QTimer::timeout,
            this, &LiveStreamInputPanel::updateStreamStatistics);
    m_statsUpdateTimer->start(1000);  // Update every 1 second
}
```

---

## Integration with Main Window

### Connect to ETI Processing:

```cpp
// In DABAnalyzerWindow constructor
m_liveStreamPanel = new LiveStreamInputPanel(this);

connect(m_liveStreamPanel, &LiveStreamInputPanel::frameReceived,
        this, &DABAnalyzerWindow::processLiveFrame);

connect(m_liveStreamPanel, &LiveStreamInputPanel::streamConnected,
        this, [this](const QString& url) {
            m_systemMessages->append(QString("[STREAM] Connected to %1").arg(url));
        });

connect(m_liveStreamPanel, &LiveStreamInputPanel::streamError,
        this, [this](const QString& error) {
            m_systemMessages->append(QString("[ERROR] Stream error: %1").arg(error));
        });

void DABAnalyzerWindow::processLiveFrame(const QByteArray& frameData) {
    // Process frame same as file-based frames
    if (frameData.size() != 6144) return;
    
    // Parse ETI frame
    auto parseResult = m_etiProcessor->parseFrame(frameData);
    
    if (parseResult.success) {
        // Update all UI components
        updateEnsembleTree(parseResult.figAnalysis);
        updateFrameNavigator(parseResult.frame);
        updateHexViewer(frameData);
        updateETIOverview(parseResult.frame);
        updateSubchannelTable(parseResult.figAnalysis);
        
        // Update statistics
        m_totalFrames++;
        m_currentFrameIndex++;
    }
}
```

---

## Error Handling & Recovery

### Connection Error Handling:

```cpp
void LiveStreamInputPanel::handleConnectionError(const QString& error) {
    updateConnectionState(ConnectionState::Error);
    
    QString errorMsg = QString("Stream connection error: %1\n\n"
                              "Possible causes:\n"
                              "- Invalid URL format\n"
                              "- Network unreachable\n"
                              "- Firewall blocking\n"
                              "- Stream source not available\n\n"
                              "Check URL and network settings.")
                      .arg(error);
    
    QMessageBox::critical(this, "Connection Error", errorMsg);
    
    // Auto-reconnect if enabled
    if (m_streamConfig.auto_reconnect && 
        m_reconnectAttempts < m_streamConfig.max_reconnect_attempts) {
        
        m_reconnectAttempts++;
        QTimer::singleShot(m_streamConfig.reconnect_delay.count(),
                          this, &LiveStreamInputPanel::attemptReconnect);
    }
}

void LiveStreamInputPanel::attemptReconnect() {
    m_systemMessages->append(QString("[STREAM] Reconnect attempt %1/%2...")
        .arg(m_reconnectAttempts)
        .arg(m_streamConfig.max_reconnect_attempts));
    
    onConnectClicked();
}
```

---

## CMakeLists.txt Modifications

### Add ZMQ Support (Optional):

```cmake
# Find ZeroMQ (optional)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(ZMQ libzmq)
    if(ZMQ_FOUND)
        add_definitions(-DHAVE_ZMQ)
        include_directories(${ZMQ_INCLUDE_DIRS})
        link_directories(${ZMQ_LIBRARY_DIRS})
        set(STREAM_LIBS ${STREAM_LIBS} ${ZMQ_LIBRARIES})
        message(STATUS "ZeroMQ support enabled")
    else()
        message(STATUS "ZeroMQ not found - ZMQ streaming disabled")
    endif()
endif()

# Add network libraries
target_link_libraries(DABAnalyzer_Enhanced_FilteringBuild
    Qt6::Core
    Qt6::Widgets
    Qt6::Network  # Required for TCP/UDP
    ${STREAM_LIBS}
)
```

---

## Summary - Live Stream Input Features

### Implemented:
✅ **Multicast UDP** - Ready to use via NetworkStreamReceiver
✅ **URL parsing** - Protocol detection and validation
✅ **Stream statistics** - Real-time monitoring
✅ **Connection management** - Connect/disconnect controls
✅ **Auto-reconnect** - Automatic recovery on connection loss
✅ **Buffer monitoring** - Frame buffer status display

### To Implement:
⚠️ **TCP streaming** - Add QTcpSocket receiver (easy)
⚠️ **ZMQ streaming** - Add libzmq binding (medium effort)
⚠️ **LED status indicator** - Custom widget for visual status
⚠️ **Quick example dropdown** - Populate with common URLs

### Benefits:
- ✅ Real-time broadcast monitoring
- ✅ Remote ETI source analysis
- ✅ Professional stream quality metrics
- ✅ Seamless integration with existing file-based workflow
- ✅ Industry-standard protocol support

**Integration Location:** Top of Tab 1, above Player/Decoder panels
**Implementation Effort:** 2-3 days (UDP ready, TCP +4 hours, ZMQ +8 hours)
