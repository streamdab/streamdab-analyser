# Tab 1: ETI DAB Ensemble Explorer - Detailed Specification

> **Tab Purpose:** Primary workspace for ETI file analysis, service discovery, and frame navigation  
> **Priority:** PRIMARY (most-used tab)  
> **Layout:** 3-column design (Left 25% | Center 50% | Right 25%)

---

## 🎯 Tab 1 Visual Layout

```
┌─────────────────────────────────────────────────────────────────────────┐
│  Tab 1: ETI DAB Ensemble Explorer (PRIMARY)                              │
├──────────────┬────────────────────────────────┬──────────────────────────┤
│              │                                │                          │
│  LEFT PANEL  │      CENTER PANEL              │      RIGHT PANEL         │
│  (25%)       │      (50%)                     │      (25%)               │
│              │                                │                          │
│  ┌────────┐  │  ┌──────────────────────────┐  │  ┌─────────────────┐   │
│  │ DAB    │  │  │ Player/Decoder Status    │  │  │ Current Service │   │
│  │ Tree   │  │  │ (Top 15%)                │  │  │ (Top 25%)       │   │
│  │        │  │  └──────────────────────────┘  │  └─────────────────┘   │
│  │(Top 50%)│  │                                │                          │
│  │        │  │  ┌──────────────────────────┐  │  ┌─────────────────┐   │
│  └────────┘  │  │ ETI Frame Navigator      │  │  │ Signal Quality  │   │
│              │  │ (Progress + List)        │  │  │ (Middle 20%)    │   │
│  ┌────────┐  │  │ (Middle 30%)             │  │  └─────────────────┘   │
│  │ ETI    │  │  └──────────────────────────┘  │                          │
│  │Explorer│  │                                │  ┌─────────────────┐   │
│  │Details │  │  ┌──────────────────────────┐  │  │ Now Playing     │   │
│  │(Bottom │  │  │ Hex Viewer / ETSI        │  │  │ (DLS+)          │   │
│  │  50%)  │  │  │ Compliance (QTabWidget)  │  │  │ (Bottom 55%)    │   │
│  └────────┘  │  │ (Bottom 55%)             │  │  └─────────────────┘   │
│              │  └──────────────────────────┘  │                          │
└──────────────┴────────────────────────────────┴──────────────────────────┘
```

---

## 📋 LEFT PANEL (25% width)

### Layout: QSplitter Vertical (50% | 50%)

---

### 1.1 DAB Ensemble Tree (Top 50%)

**Widget Type:** `QTreeWidget`  
**Purpose:** Hierarchical display of DAB ensemble, services, and components  
**Current Implementation:** `m_ensembleTree` (line 2839 in main.cpp)

#### Column Configuration
```cpp
Columns: 3
├── Column 0: "Service" (width: 200px)
├── Column 1: "Label" (width: 180px)
└── Column 2: "Type" (width: 120px)
```

#### Tree Hierarchy (3 levels)
```
Level 1: Ensemble (root)
├─ Bangkok DAB+ Ensemble | NBT Thailand | Ensemble
    │
    ├─ Level 2: Service
    │   ├─ NBT Radio | National Broadcasting | Audio DAB+
    │   │   │
    │   │   ├─ Level 3: Component
    │   │   ├─ Frame Status | Active | Service Detail
    │   │   └─ Component Info | SubCh 1 | 192 kbps
    │   │
    │   ├─ NBT News | News Channel | Audio DAB+
    │   └─ Traffic Info | Real-time | Data Service
```

#### Features
- ✅ Expand/collapse arrows (▼ expanded, ▶ collapsed)
- ✅ Icons for service types:
  - 🎵 Audio services
  - 📡 Data services
  - 📢 Ensemble root
- ✅ Right-click context menu (Export, Copy Info)
- ✅ Selection highlighting (light blue background #e3f2fd)
- ✅ Alternating row colors for better readability

#### Data Population
```cpp
// Example data structure
QTreeWidgetItem* ensembleItem = new QTreeWidgetItem();
ensembleItem->setText(0, "Bangkok DAB+ Ensemble");
ensembleItem->setText(1, "NBT Thailand");
ensembleItem->setText(2, "Ensemble");
ensembleItem->setExpanded(true);

QTreeWidgetItem* serviceItem = new QTreeWidgetItem(ensembleItem);
serviceItem->setText(0, "NBT Radio");
serviceItem->setText(1, "National Broadcasting");
serviceItem->setText(2, "Audio DAB+");

m_ensembleTree->addTopLevelItem(ensembleItem);
```

#### Signals/Slots
- `itemClicked(QTreeWidgetItem*, int)` → `onServiceSelected()`
- `itemSelectionChanged()` → `updateServiceInfo()`

---

### 1.2 ETI Explorer Details (Bottom 50%)

**Widget Type:** `QGroupBox` containing `QTableWidget`  
**Purpose:** Display ETI frame overview statistics (ETI-XPlorer style)  
**Current Implementation:** `m_frameDetails` (line 2865) - **NEEDS CONVERSION from QLabel to QTableWidget**

#### Table Structure
```
┌─────────────────────┬──────────────────┐
│ Property            │ Value            │
├─────────────────────┼──────────────────┤
│ ETI Type            │ ETSI(NI, G.704)  │
│ Error Field         │ Level 0          │
│ DAB Mode            │ 1                │
│ STAT                │ 0x8 64 / 99%     │
│ Total Frames        │ 5001             │
│ ETI-NI Count        │ 0                │
│ ETI-LI-A Count      │ 2501             │
│ ETI-LI-B Count      │ 2500             │
└─────────────────────┴──────────────────┘
```

#### Styling Requirements
- 2 columns (Property | Value)
- Alternating row colors (#f9f9f9 / #ffffff)
- **Bold** property names (column 0)
- Monospace font for values (column 1)
- No vertical header
- Horizontal header stretch last section

#### Code Implementation
```cpp
m_etiExplorerDetails = new QTableWidget(8, 2);
m_etiExplorerDetails->setHorizontalHeaderLabels({"Property", "Value"});
m_etiExplorerDetails->verticalHeader()->setVisible(false);
m_etiExplorerDetails->setAlternatingRowColors(true);
m_etiExplorerDetails->horizontalHeader()->setStretchLastSection(true);

// Populate rows
QStringList properties = {
    "ETI Type", "Error Field", "DAB Mode", "STAT",
    "Total Frames", "ETI-NI Count", "ETI-LI-A Count", "ETI-LI-B Count"
};

for (int i = 0; i < properties.size(); i++) {
    QTableWidgetItem* propItem = new QTableWidgetItem(properties[i]);
    QFont font = propItem->font();
    font.setBold(true);
    propItem->setFont(font);
    m_etiExplorerDetails->setItem(i, 0, propItem);
    
    m_etiExplorerDetails->setItem(i, 1, new QTableWidgetItem("--"));
}
```

#### Data Updates
```cpp
void updateETIExplorerDetails() {
    m_etiExplorerDetails->item(0, 1)->setText("ETSI(NI, G.704)");
    m_etiExplorerDetails->item(1, 1)->setText("Level 0");
    m_etiExplorerDetails->item(2, 1)->setText("1");
    m_etiExplorerDetails->item(3, 1)->setText("0x8 64 / 99%");
    m_etiExplorerDetails->item(4, 1)->setText(QString::number(m_totalFrames));
    m_etiExplorerDetails->item(5, 1)->setText(QString::number(m_etiProcessor->getETINICount()));
    m_etiExplorerDetails->item(6, 1)->setText(QString::number(m_etiProcessor->getETILIACount()));
    m_etiExplorerDetails->item(7, 1)->setText(QString::number(m_etiProcessor->getETILIBCount()));
}
```

---

## 📋 CENTER PANEL (50% width)

### Layout: QSplitter Vertical (15% | 30% | 55%)

---

### 2.1 Player/Decoder Status Panel (Top 15%)

**Widget Type:** `QWidget` containing 2x `QGroupBox` (side-by-side)  
**Purpose:** Display real-time playback and decoder status  
**Current Implementation:** **NEW COMPONENTS** - Not currently implemented

#### Left GroupBox - Player Panel
```
┌─────────────────────────────────────┐
│ Player Panel                        │
├─────────────────────────────────────┤
│ Status:     ⚫ Running               │
│ File:       bkk_20062022.eti        │
│ Start:      00:00:00                │
│ Current:    00:16:47                │
│ End:        00:08:29                │
│ Audio data: Frame 4992              │
│ [====Progress Bar=========>    ]    │
└─────────────────────────────────────┘
```

#### Right GroupBox - Decoder Panel
```
┌─────────────────────────────────────┐
│ Decoder Panel                       │
├─────────────────────────────────────┤
│ Status:     ⚫ Running               │
│ Type:       DAB+ AAC                │
│ Start:      00:00:00                │
│ Current:    00:16:47                │
│ End:        00:08:29                │
│ Frames:     4992 / 5001             │
└─────────────────────────────────────┘
```

#### Code Implementation
```cpp
QWidget* createPlayerDecoderStatus() {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    
    // Player Panel (Left)
    m_playerPanel = new QGroupBox("Player Panel");
    QVBoxLayout* playerLayout = new QVBoxLayout(m_playerPanel);
    m_playerStatusLabel = new QLabel("Status: ⚫ Running");
    m_playerFileLabel = new QLabel("File: --");
    m_playerStartTimeLabel = new QLabel("Start: 00:00:00");
    m_playerCurrentTimeLabel = new QLabel("Current: 00:00:00");
    m_playerEndTimeLabel = new QLabel("End: 00:00:00");
    m_playerFrameLabel = new QLabel("Audio data: Frame 0");
    m_playerProgressBar = new QProgressBar();
    
    playerLayout->addWidget(m_playerStatusLabel);
    playerLayout->addWidget(m_playerFileLabel);
    playerLayout->addWidget(m_playerStartTimeLabel);
    playerLayout->addWidget(m_playerCurrentTimeLabel);
    playerLayout->addWidget(m_playerEndTimeLabel);
    playerLayout->addWidget(m_playerFrameLabel);
    playerLayout->addWidget(m_playerProgressBar);
    
    layout->addWidget(m_playerPanel);
    
    // Decoder Panel (Right) - similar structure
    // ...
    
    return widget;
}
```

---

### 2.2 ETI Frame Navigator (Middle 30%)

**Widget Type:** `QWidget` containing progress bar + `QListWidget`  
**Purpose:** Navigate through ETI frames with visual progress  
**Current Implementation:** 
- `m_frameList` (line 2840)
- `m_progressBar` (line 2914)
- `m_frameCountLabel` (line 2913)

#### Structure
```
┌─────────────────────────────────────────────────────────┐
│ ETI Frame Navigator                                      │
├─────────────────────────────────────────────────────────┤
│ [===================Progress Bar====================] 100%│
│ Frames: 5001 / 5001                                     │
├─────────────────────────────────────────────────────────┤
│ Frame List (Scrollable):                               │
│                                                         │
│  Frame 1: ETI-LI-A (0xff8c549)                         │
│  Frame 2: ETI-LI-B (0xdf073ab6)                        │
│  Frame 3: ETI-LI-A (0xff8c549)                         │
│  Frame 4: ETI-LI-B (0xdf073ab6)                        │
│  ...                                                    │
│  Frame 4992: ETI-LI-A (0xff8c549)    ← Selected        │
│  ...                                                    │
│  Frame 5001: ETI-LI-B (0xdf073ab6)                     │
└─────────────────────────────────────────────────────────┘
```

#### Progress Bar Configuration
```cpp
m_frameProgress = new QProgressBar();
m_frameProgress->setRange(0, 100);
m_frameProgress->setValue(0);
m_frameProgress->setTextVisible(true);
m_frameProgress->setFormat("%p%");  // Shows percentage
```

#### Frame List Configuration
```cpp
m_frameList = new QListWidget();
m_frameList->setAlternatingRowColors(true);
m_frameList->setFontFamily("Consolas");

// Populate frames
for (int i = 0; i < totalFrames; i++) {
    QString frameText = QString("Frame %1: %2 (0x%3)")
                       .arg(i + 1)
                       .arg(formatToString(frame.format))
                       .arg(frame.sync_pattern, 8, 16, QChar('0'));
    m_frameList->addItem(frameText);
}
```

#### Signals/Slots
- `currentRowChanged(int)` → `onFrameSelected(int frameIndex)`
- Updates: Hex viewer, compliance viewer, ensemble tree

---

### 2.3 Hex Viewer / ETSI Compliance (Bottom 55%)

**Widget Type:** `QTabWidget` (2 tabs)  
**Purpose:** Display frame hex dump and ETSI compliance validation  
**Current Implementation:**
- `m_hexViewer` (line 2841)
- `m_complianceViewer` (line 2842)

#### Tab 1: Hex Viewer

**Format:**
```
┌─────────────────────────────────────────────────────────┐
│ ETI Frame #4992 - Hex Viewer (6144 bytes)              │
├─────────────────────────────────────────────────────────┤
│ Address  | Hex Values (16 bytes/row)    | ASCII        │
├─────────────────────────────────────────────────────────┤
│ 0000:    | FF 8C 54 9 00 00 00 13 40 ... | ..T.....@... │
│ 0010:    | 1D 02 1C C4 02 3F 52 FF ...   | .....?R..... │
│ 0020:    | FB 1A 45 02 3F 12 FF E0 ...   | ..E.?....... │
│ 0030:    | D7 FB D9 8F 1E F4 53 46 ...   | ......SF.... │
│ ...                                                     │
│ 1800:    | DD DC 8F FA 1E FA E3 6B ...   | .......k.... │
└─────────────────────────────────────────────────────────┘
```

**Color Coding:**
- Sync bytes (0000-0003): **Red** (#d32f2f)
- ETI Header (0004-001F): **Orange** (#f57c00)
- FIC data (0020-007F): **Green** (#388e3c)
- MSC data (0080-17FF): **Blue** (#1976d2)
- ASCII non-printable: **Grey** (#757575)

**Code Implementation:**
```cpp
void displayFrameHexData(int frameIndex) {
    if (!m_hexFormatter || !m_etiProcessor) return;
    
    QByteArray frameData = m_etiProcessor->getRawFrameData(frameIndex);
    if (frameData.isEmpty()) return;
    
    QString hexHtml = m_hexFormatter->formatToHTML(
        reinterpret_cast<const uint8_t*>(frameData.data()),
        frameData.size(),
        frameIndex
    );
    
    m_hexViewer->setHtml(hexHtml);
}
```

#### Tab 2: ETSI Compliance

**Format:**
```
┌─────────────────────────────────────────────────────────┐
│ ETSI EN 300 799 Compliance Validation - Frame 4992     │
├─────────────────────────────────────────────────────────┤
│ ✅ COMPLIANT                                            │
│                                                         │
│ Description: Frame structure compliant with ETSI spec  │
│ Byte Offset: N/A                                        │
│ Timestamp: 14:35:28.123                                 │
│                                                         │
│ ETSI Reference: EN 300 799 Section 5.1.1               │
│ Suggested Fix: N/A                                      │
└─────────────────────────────────────────────────────────┘
```

**Status Color Coding:**
- ✅ **COMPLIANT** (Green #388e3c)
- ℹ️ **INFO** (Blue #1976d2)
- ⚠️ **WARNING** (Orange #f57c00)
- 🚨 **CRITICAL** (Red #d32f2f)

---

## 📋 RIGHT PANEL (25% width)

### Layout: QSplitter Vertical (25% | 20% | 55%)

---

### 3.1 Current Service (Top 25%)

**Widget Type:** `QGroupBox` containing `QLabel`  
**Purpose:** Display currently selected service details  
**Current Implementation:** `m_serviceInfo` (line 2912)

#### Display Format
```
┌─────────────────────────────────────┐
│ Current Service                     │
├─────────────────────────────────────┤
│                                     │
│ Service: NBT Radio                  │
│ Label: National Broadcasting        │
│ Type: Audio DAB+                    │
│ Bitrate: 192 kbps                   │
│ SubChannel: 1                       │
│ Protection: UEP Level 3             │
│                                     │
└─────────────────────────────────────┘
```

#### Code Implementation
```cpp
void updateServiceInfo() {
    if (!currentService) {
        m_serviceInfo->setText("No service selected");
        return;
    }
    
    QString info = QString(
        "Service: %1\n"
        "Label: %2\n"
        "Type: %3\n"
        "Bitrate: %4 kbps\n"
        "SubChannel: %5\n"
        "Protection: %6"
    ).arg(currentService->name)
     .arg(currentService->label)
     .arg(currentService->type)
     .arg(currentService->bitrate)
     .arg(currentService->subchannel)
     .arg(currentService->protection);
    
    m_serviceInfo->setText(info);
}
```

---

### 3.2 Signal Quality (Middle 20%)

**Widget Type:** `QGroupBox` containing `QProgressBar` + labels  
**Purpose:** Display signal quality metrics  
**Current Implementation:** `m_qualityMeter` (line 2915)

#### Display Format
```
┌─────────────────────────────────────┐
│ Signal Quality                      │
├─────────────────────────────────────┤
│                                     │
│ Quality: Good                       │
│ [==============85%=========    ]    │
│                                     │
│ SNR: 32.5 dB                        │
│ BER: 1.2e-6                         │
│                                     │
└─────────────────────────────────────┘
```

#### Progress Bar Color Coding
```cpp
// Quality ranges
if (quality < 50) {
    m_qualityMeter->setStyleSheet("QProgressBar::chunk { background-color: #F44336; }"); // Red
} else if (quality < 75) {
    m_qualityMeter->setStyleSheet("QProgressBar::chunk { background-color: #FF9800; }"); // Orange
} else {
    m_qualityMeter->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }"); // Green
}
```

---

### 3.3 Now Playing (DLS+) (Bottom 55%)

**Widget Type:** `QGroupBox` with grid layout  
**Purpose:** Display DLS+ metadata (Dynamic Label Plus)  
**Current Implementation:**
- `m_trackLabel` (line 2918)
- `m_artistLabel` (line 2919)
- `m_albumLabel` (line 2920)
- `m_lastUpdatedLabel` (line 2921)

#### Display Format
```
┌─────────────────────────────────────┐
│ Now Playing (DLS+)                  │
├─────────────────────────────────────┤
│                                     │
│ Track:                              │
│ ┌─────────────────────────────────┐ │
│ │ Bangkok Morning Show            │ │
│ └─────────────────────────────────┘ │
│                                     │
│ Artist:                             │
│ ┌─────────────────────────────────┐ │
│ │ NBT Radio                       │ │
│ └─────────────────────────────────┘ │
│                                     │
│ Album:                              │
│ ┌─────────────────────────────────┐ │
│ │ Morning Edition 2022            │ │
│ └─────────────────────────────────┘ │
│                                     │
│ Updated: 2022-06-20 14:16:47        │
└─────────────────────────────────────┘
```

#### Code Implementation
```cpp
QGroupBox* createNowPlayingPanel() {
    QGroupBox* groupBox = new QGroupBox("Now Playing (DLS+)");
    QGridLayout* layout = new QGridLayout(groupBox);
    
    layout->addWidget(new QLabel("Track:"), 0, 0);
    m_trackLabel = new QLabel("--");
    m_trackLabel->setWordWrap(true);
    layout->addWidget(m_trackLabel, 0, 1);
    
    layout->addWidget(new QLabel("Artist:"), 1, 0);
    m_artistLabel = new QLabel("--");
    m_artistLabel->setWordWrap(true);
    layout->addWidget(m_artistLabel, 1, 1);
    
    layout->addWidget(new QLabel("Album:"), 2, 0);
    m_albumLabel = new QLabel("--");
    m_albumLabel->setWordWrap(true);
    layout->addWidget(m_albumLabel, 2, 1);
    
    m_lastUpdatedLabel = new QLabel("No metadata received");
    m_lastUpdatedLabel->setStyleSheet("QLabel { color: #757575; font-style: italic; }");
    layout->addWidget(m_lastUpdatedLabel, 3, 0, 1, 2);
    
    return groupBox;
}
```

#### DLS+ Update Signal
```cpp
// Connected to DLS+ decoder
connect(m_dlsPlusDecoder, &DLSPlusDecoder::nowPlayingUpdated,
        this, &MainWindow::updateNowPlaying);

void updateNowPlaying(const QString& track, const QString& artist, const QString& album) {
    m_trackLabel->setText(track.isEmpty() ? "--" : track);
    m_artistLabel->setText(artist.isEmpty() ? "--" : artist);
    m_albumLabel->setText(album.isEmpty() ? "--" : album);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_lastUpdatedLabel->setText(QString("Updated: %1").arg(timestamp));
}
```

---

## 🔄 User Interaction Flows

### Flow 1: File Load → Display Update
```
1. User: File → Open → bkk_20062022.eti
2. System: Loads ETI file
3. m_frameList: Populates with all frames
4. m_progressBar: Shows 100%
5. m_ensembleTree: Displays discovered services
6. m_etiExplorerDetails: Updates statistics
7. System Messages: "✅ Processing complete"
```

### Flow 2: Frame Selection → Hex Display
```
1. User: Clicks "Frame 4992" in m_frameList
2. Signal: currentRowChanged(4991) [0-indexed]
3. m_hexViewer: Displays frame #4992 hex dump
4. m_complianceViewer: Shows ETSI validation
5. m_ensembleTree: Updates frame-specific services
6. System Messages: "Frame 4992 selected"
```

### Flow 3: Service Selection → Info Update
```
1. User: Clicks "NBT Radio" in m_ensembleTree
2. Signal: itemClicked()
3. m_serviceInfo: Displays service details
4. m_qualityMeter: Updates signal quality
5. m_trackLabel/m_artistLabel: Shows DLS+ metadata
6. System Messages: "Service selected: NBT Radio"
```

---

## ✅ Implementation Checklist

### Left Panel
- [ ] Convert m_frameDetails from QLabel to QTableWidget
- [ ] Populate ETI Explorer Details with 8 properties
- [ ] Connect m_ensembleTree selection to service info update
- [ ] Test expand/collapse functionality

### Center Panel
- [ ] Create Player/Decoder status panels (NEW)
- [ ] Integrate m_frameList with progress bar
- [ ] Test frame selection → hex viewer update
- [ ] Implement ETSI compliance color coding

### Right Panel
- [ ] Update m_serviceInfo with detailed format
- [ ] Implement quality meter color coding
- [ ] Connect DLS+ decoder to Now Playing panel
- [ ] Test real-time metadata updates

### Overall
- [ ] Set up QSplitter proportions (25% | 50% | 25%)
- [ ] Apply professional stylesheet
- [ ] Test all signal/slot connections
- [ ] Verify responsive resizing

---

**Document Version:** 1.0  
**Last Updated:** 2025-10-23  
**Status:** ✅ Ready for Implementation
