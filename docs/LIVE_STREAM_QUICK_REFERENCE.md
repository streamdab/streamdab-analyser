# Live Stream Input Panel - Quick Reference Card

**Quick Access Guide for Developers**

---

## File Locations

```
Main Implementation:  /home/seksan/workspace/streamdab-analyzer/src/main.cpp
Design Document:      docs/LIVE_STREAM_INPUT_DESIGN.md
Implementation Doc:   docs/LIVE_STREAM_INPUT_IMPLEMENTATION.md
```

---

## Code Locations in main.cpp

| Component | Line Range | Description |
|-----------|------------|-------------|
| **Qt Headers** | ~28-33 | Added QRadioButton, QLineEdit, QStackedWidget, QGridLayout |
| **Slot Functions** | ~805-960 | 7 slot implementations for Live Stream panel |
| **createLiveStreamInputPanel()** | ~1968-2158 | Main panel creation function (~195 lines) |
| **Member Variables** | ~3424-3450 | 27 widget pointers for Live Stream UI |
| **Tab 1 Integration** | ~2161-2175 | Added to createTab1_CenterPanel() |

---

## Quick Integration Points

### To Connect Real Network Stream:

**Replace TODO in `onConnectClicked()` (line ~853):**
```cpp
// Current (simulated):
QTimer::singleShot(1000, this, [this, url]() {
    if (m_streamStatus) {
        m_streamStatus->setText("<span style='color: #107C10;'>● Connected</span>");
    }
    m_systemMessages->append(QString("[STREAM] Connected to %1").arg(url));
});

// Replace with:
m_networkReceiver = std::make_unique<eti::NetworkStreamReceiver>(config, this);
connect(m_networkReceiver.get(), &eti::NetworkStreamReceiver::frameReceived,
        this, &DABAnalyzerWindow::onNetworkFrameReceived);
if (m_networkReceiver->start()) {
    m_streamStatus->setText("<span style='color: #107C10;'>● Connected</span>");
}
```

### To Add Real Statistics:

**Replace simulated data in `updateStreamStatistics()` (line ~893):**
```cpp
// Current (simulated):
m_streamFrameRate->setText("250.5 FPS");

// Replace with:
if (m_networkReceiver) {
    auto stats = m_networkReceiver->getStatistics();
    m_streamFrameRate->setText(QString("%1 FPS").arg(stats.fps, 0, 'f', 1));
    m_streamFramesReceived->setText(QString("%1 frames").arg(stats.frameCount));
    // ... etc
}
```

---

## Widget Access

All widgets accessible via member variables with `m_` prefix:

```cpp
// Source selection
m_fileSourceRadio         // QRadioButton: File mode
m_networkSourceRadio      // QRadioButton: Network mode
m_sourceInputStack        // QStackedWidget: Switch UI

// File input (Page 0)
m_filePathEdit           // QLineEdit: File path display
m_browseFileButton       // QPushButton: Browse button
m_loadFileButton         // QPushButton: Load File button

// Network input (Page 1)
m_streamUrlEdit          // QLineEdit: URL input
m_quickExampleCombo      // QComboBox: Quick examples
m_connectBtn             // QPushButton: Connect button
m_disconnectBtn          // QPushButton: Disconnect button
m_streamStatus           // QLabel: Status LED indicator

// Statistics (7 metrics)
m_streamFrameRate        // QLabel: FPS display
m_streamFramesReceived   // QLabel: Frame count
m_streamFrameLoss        // QLabel: Loss percentage
m_streamLatency          // QLabel: Latency (ms)
m_streamBuffer           // QLabel: Buffer fill level
m_streamDataRate         // QLabel: Data rate (Mbit/s)
m_streamUptime           // QLabel: Connection uptime

// Timer
m_statsTimer             // QTimer: 1-second statistics updates
```

---

## Signal/Slot Quick Reference

```cpp
// Radio button switching
m_fileSourceRadio->toggled(bool) → onSourceTypeChanged(!checked)
m_networkSourceRadio->toggled(bool) → onSourceTypeChanged(checked)

// Quick examples
m_quickExampleCombo->currentIndexChanged(int) → onQuickExampleSelected(index)

// Connection controls
m_connectBtn->clicked() → onConnectClicked()
m_disconnectBtn->clicked() → onDisconnectClicked()

// File operations
m_browseFileButton->clicked() → onBrowseFileClicked()
m_loadFileButton->clicked() → onLoadFileClicked()

// Statistics timer
m_statsTimer->timeout() → updateStreamStatistics()
```

---

## Slot Function Summary

| Slot | Purpose | Key Actions |
|------|---------|-------------|
| `onSourceTypeChanged(bool)` | Switch File/Network UI | Change QStackedWidget index |
| `onQuickExampleSelected(int)` | Populate URL from dropdown | Set m_streamUrlEdit text |
| `onConnectClicked()` | Start network connection | Enable disconnect, start timer, **TODO: real connection** |
| `onDisconnectClicked()` | Stop network connection | Enable connect, stop timer, reset stats |
| `updateStreamStatistics()` | Update stats every 1s | Update 7 statistic labels, **TODO: real data** |
| `onBrowseFileClicked()` | Open file dialog | QFileDialog → m_filePathEdit |
| `onLoadFileClicked()` | Load selected file | Call existing openFile() |

---

## Quick URL Examples

```cpp
// UDP Multicast (index 0)
udp://239.192.0.1:9200

// TCP Direct (index 1)
tcp://192.168.1.100:9200

// ZeroMQ (index 2)
zmq+tcp://localhost:9201
```

---

## Status LED Color Codes

```cpp
// Disconnected (gray)
m_streamStatus->setText("<span style='color: #666;'>● Disconnected</span>");

// Connecting (yellow/orange)
m_streamStatus->setText("<span style='color: #FFA500;'>● Connecting...</span>");

// Connected (green)
m_streamStatus->setText("<span style='color: #107C10;'>● Connected</span>");

// Error (red) - not currently used
m_streamStatus->setText("<span style='color: #DC3545;'>● Error</span>");
```

---

## Build & Test

```bash
# Build
cd /home/seksan/workspace/streamdab-analyzer/build
make -j$(nproc)

# Run
./DABAnalyzer_Enhanced_FilteringBuild

# Verify
# 1. Tab 1 should show "Input Source" panel at top
# 2. Default: File mode selected
# 3. Switch to Network: Shows URL input + statistics
# 4. Click Connect: Status → Connecting → Connected
# 5. Statistics update every 1 second
```

---

## Common Tasks

### Add New Quick Example URL
**File:** main.cpp, Line ~2023
```cpp
m_quickExampleCombo->addItem("NEW://URL:PORT (Description)");
```
**Then update:** `onQuickExampleSelected()` switch statement (line ~814)

### Change Statistics Update Interval
**File:** main.cpp, Line ~2154
```cpp
m_statsTimer->start(1000);  // Change 1000 to desired ms
```

### Customize Button Colors
**File:** main.cpp, Lines ~2012, 2045
```cpp
m_connectBtn->setStyleSheet("font-weight: bold; background-color: #107C10; color: white;");
m_loadFileButton->setStyleSheet("font-weight: bold; background-color: #0078D4; color: white;");
```

### Add New Statistic
**1. Add member variable (line ~3450):**
```cpp
QLabel* m_streamNewMetric;
```

**2. Create label in createLiveStreamInputPanel() (line ~2070+):**
```cpp
QLabel* newLabel = new QLabel("New Metric:");
m_streamNewMetric = new QLabel("--");
statsLayout->addWidget(newLabel, ROW, COL);
statsLayout->addWidget(m_streamNewMetric, ROW, COL+1);
```

**3. Update in updateStreamStatistics() (line ~920+):**
```cpp
if (m_streamNewMetric) {
    m_streamNewMetric->setText("value");
}
```

**4. Reset in onDisconnectClicked() (line ~885+):**
```cpp
if (m_streamNewMetric) m_streamNewMetric->setText("--");
```

---

## Debugging Tips

### Enable Debug Output
```cpp
// In onConnectClicked()
qDebug() << "[STREAM] Connecting to:" << url;

// In updateStreamStatistics()
qDebug() << "[STATS] Update:" << frameCount << "frames";
```

### Check Widget Pointers
```cpp
// Before using widgets
if (!m_streamUrlEdit) {
    qWarning() << "Stream URL widget not initialized!";
    return;
}
```

### Monitor Timer
```cpp
// Check if timer is running
if (m_statsTimer && m_statsTimer->isActive()) {
    qDebug() << "Statistics timer running";
}
```

---

## TODO List for Network Integration

**Priority 1 (Essential):**
- [ ] Replace simulated connection in `onConnectClicked()` with NetworkStreamReceiver
- [ ] Replace simulated statistics in `updateStreamStatistics()` with real data
- [ ] Add URL validation (protocol, host, port)
- [ ] Implement error handling for connection failures

**Priority 2 (Important):**
- [ ] Add auto-reconnect on connection loss
- [ ] Implement real buffer monitoring
- [ ] Add protocol-specific connection logic (UDP/TCP/ZMQ)
- [ ] Create network frame processing pipeline

**Priority 3 (Enhancement):**
- [ ] Add connection history (recent URLs)
- [ ] Implement stream recording capability
- [ ] Add network diagnostics panel
- [ ] Create bandwidth throttling controls

---

## Testing Checklist

```
☐ Application launches without crashes
☐ File mode shows correctly (default)
☐ Network mode shows when radio selected
☐ Browse button opens file dialog
☐ Load File button processes file
☐ Quick Examples populate URL field
☐ Connect button changes state
☐ Disconnect button resets state
☐ Statistics update every 1 second
☐ Status LED changes color
☐ All labels display correctly
☐ Layout proportions correct (10% height)
```

---

## Contact & Support

**Implementation:** Claude AI Assistant  
**Date:** 2025-10-24  
**Documentation:** docs/LIVE_STREAM_INPUT_IMPLEMENTATION.md  
**Design Reference:** docs/LIVE_STREAM_INPUT_DESIGN.md

---

**Quick Build Verification:**
```bash
cd build && make && echo "✅ BUILD SUCCESS" || echo "❌ BUILD FAILED"
```

**Line Count:**
```bash
wc -l src/main.cpp
# Should show: 3902 lines
```

**Git Status:**
```bash
git diff src/main.cpp | wc -l
# Should show: ~450+ lines changed
```
