# Release 1B: GUI Integration Plan (Polish & User Experience)

**Document Version:** 1.0
**Date:** October 19, 2025
**Phase:** GUI Integration & User Experience
**Prerequisite:** Release 1A Core Processors 100% Complete
**Timeline:** 3-4 weeks
**Focus:** Connect complete backends to professional Qt6 interface

---

## Executive Summary

Release 1B integrates the complete processors from Release 1A (100% backend) into the existing Qt6 GUI framework. This phase focuses on **user experience and professional presentation**, not core functionality (already complete in 1A).

### Strategy: "Rapid Integration with Existing GUI Framework"

**Current GUI State:**
- ✅ Professional three-panel Qt6 interface exists ([src/main.cpp](../src/main.cpp))
- ✅ Service tree widget functional
- ✅ Frame navigation working
- ✅ Hex viewer operational
- ✅ File dialogs complete

**1B Additions:**
- 🎨 Connect 1A processors to existing widgets
- 🎨 Add new panels for EDI, audio quality, SlideShow, EWS
- 🎨 Enhance existing widgets with real data
- 🎨 Professional polish and usability improvements

---

## Table of Contents

1. [Current GUI Assessment](#current-gui-assessment)
2. [Week 1: EDI Network & Enhanced Service Tree](#week-1-edi-network--enhanced-service-tree)
3. [Week 2: Audio Quality & FIG Details](#week-2-audio-quality--fig-details)
4. [Week 3: SlideShow & Data Services](#week-3-slideshow--data-services)
5. [Week 4: Emergency Alerts & Final Polish](#week-4-emergency-alerts--final-polish)
6. [Success Criteria](#success-criteria)

---

## Current GUI Assessment

### ✅ **Existing Qt6 Framework (Ready for Integration)**

**Main Window** - `src/main.cpp` (lines 1-2450)
```cpp
class DABAnalyzerWindow : public QMainWindow {
    // Already has:
    ✅ Three-panel layout (service tree | frame list | details)
    ✅ Menu bar with File/Tools/Help
    ✅ Status bar
    ✅ Toolbar with common actions

    // Existing widgets:
    ✅ QTreeWidget* m_serviceTree          // Service/component tree
    ✅ QListWidget* m_frameListWidget      // Frame navigation
    ✅ QTabWidget* m_detailsTabWidget      // FIG/hex/etc tabs
    ✅ QTextEdit* m_hexViewerWidget        // Hex viewer
    ✅ QTextEdit* m_figDetailsWidget       // FIG analysis text

    // Existing actions:
    ✅ File → Open ETI File
    ✅ Tools → Export Service List
    ✅ View → Toggle Panels
};
```

**What Already Works:**
- ✅ File loading with progress dialog
- ✅ Service tree population (currently simulation data)
- ✅ Frame-by-frame navigation
- ✅ Hex viewer with frame content
- ✅ Basic FIG details display

### 🎨 **1B Integration Points**

**Replace Simulation with Real Data:**
```cpp
// BEFORE (Release 1A state - simulation):
void populateServiceTree() {
    addTreeItem("Ensemble Sim-1", "Service Sim-A");
}

// AFTER (Release 1B - real data from processors):
void populateServiceTree() {
    auto services = m_fig0Processor->getDiscoveredServices();
    for (const auto& service : services) {
        addTreeItem(ensemble.label, service.label, service.extended_label);
    }
}
```

**Add New Widgets:**
- 🆕 EDI Network Monitor Panel
- 🆕 Audio Quality Metrics Panel
- 🆕 SlideShow Image Viewer
- 🆕 Emergency Alert Notification Widget

---

## Week 1: EDI Network & Enhanced Service Tree

### **Day 1-2: EDI Network Source Integration**

**New Menu Item:**
```cpp
// Modify: src/main.cpp

// Add to File menu
QAction* openNetworkAction = new QAction("Open EDI Network Stream...", this);
connect(openNetworkAction, &QAction::triggered, this,
        &DABAnalyzerWindow::onOpenNetworkStream);
```

**New Dialog:**
```cpp
// NEW: src/gui/network_stream_dialog.hpp
class NetworkStreamDialog : public QDialog {
    Q_OBJECT
public:
    struct NetworkConfig {
        QString address;      // e.g., "239.255.1.1"
        quint16 port;        // e.g., 12000
        bool enable_pft;
        quint8 fec_level;
    };
    NetworkConfig getConfig() const;

private:
    QLineEdit* m_addressEdit;
    QSpinBox* m_portSpin;
    QCheckBox* m_pftCheckbox;
    QSpinBox* m_fecSpin;
};
```

**Tasks Days 1-2:**
- [ ] Create NetworkStreamDialog
- [ ] Add "Open EDI Network Stream" menu action
- [ ] Connect dialog to EDINetworkReceiver (from 1A)
- [ ] Add network connection status to status bar
- [ ] Test network stream connection

### **Day 3-4: EDI Network Monitor Panel**

**New Dockable Panel:**
```cpp
// NEW: src/gui/edi_network_panel.hpp
class EDINetworkPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit EDINetworkPanel(QWidget* parent = nullptr);

public slots:
    void updateNetworkStats(const EDINetworkStats& stats);

private:
    QLabel* m_sourceAddressLabel;
    QLabel* m_bitrateLabel;
    QLabel* m_packetLossLabel;
    QLabel* m_fecCorrectionsLabel;
    QProgressBar* m_bufferLevel;

    // Real-time statistics chart (optional - using QCustomPlot)
    QCustomPlot* m_bitrateChart;
    QCustomPlot* m_packetLossChart;
};
```

**Connect to 1A Backend:**
```cpp
// In DABAnalyzerWindow constructor
m_ediNetworkPanel = new EDINetworkPanel(this);
addDockWidget(Qt::RightDockWidgetArea, m_ediNetworkPanel);

// Connect signals from EDINetworkReceiver (1A processor)
connect(m_ediReceiver, &EDINetworkReceiver::networkStatsUpdated,
        m_ediNetworkPanel, &EDINetworkPanel::updateNetworkStats);
```

**Tasks Days 3-4:**
- [ ] Create EDINetworkPanel widget
- [ ] Add network statistics labels
- [ ] Add real-time bitrate/packet loss charts (optional)
- [ ] Integrate with EDINetworkReceiver signals
- [ ] Add to View menu (toggle visibility)
- [ ] Test with real EDI stream

### **Day 5: Enhanced Service Tree with Real Data**

**Enhance Existing Service Tree:**
```cpp
// MODIFY: src/main.cpp

void DABAnalyzerWindow::populateServiceTree() {
    m_serviceTree->clear();

    // Get real data from FIG processors (1A)
    auto ensemble = m_fig0Processor->getEnsembleInfo();
    auto services = m_fig0Processor->getDiscoveredServices();

    // Ensemble root item
    QTreeWidgetItem* ensembleItem = new QTreeWidgetItem(m_serviceTree);
    ensembleItem->setText(0, QString::fromStdString(ensemble.label));
    ensembleItem->setText(1, QString("EID: 0x%1").arg(ensemble.id, 4, 16, QChar('0')).toUpper());

    // Add services
    for (const auto& service : services) {
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem(ensembleItem);
        serviceItem->setText(0, QString::fromStdString(service.label));

        // Extended label (Thai support!)
        if (!service.extended_label.empty()) {
            serviceItem->setToolTip(0, QString::fromStdString(service.extended_label));
        }

        serviceItem->setText(1, QString("SID: 0x%1").arg(service.service_id, 4, 16, QChar('0')).toUpper());

        // Service type icon
        if (service.is_dab_plus) {
            serviceItem->setIcon(0, QIcon(":/icons/audio_dab_plus.png"));
        } else if (service.service_type == ServiceType::Audio) {
            serviceItem->setIcon(0, QIcon(":/icons/audio_dab.png"));
        } else {
            serviceItem->setIcon(0, QIcon(":/icons/data_service.png"));
        }

        // Add components
        for (const auto& component : service.components) {
            QTreeWidgetItem* compItem = new QTreeWidgetItem(serviceItem);
            compItem->setText(0, QString::fromStdString(component.label));
            compItem->setText(1, QString("SubCh: %1").arg(component.subchannel_id));
        }
    }

    m_serviceTree->expandAll();
}
```

**Add Context Menu:**
```cpp
void DABAnalyzerWindow::onServiceTreeContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = m_serviceTree->itemAt(pos);
    if (!item) return;

    QMenu menu;
    menu.addAction("Show Service Details");
    menu.addAction("Show Audio Quality");  // NEW (for audio services)
    menu.addAction("Show SlideShow");      // NEW (if available)
    menu.addAction("Export Service Info");
    menu.exec(m_serviceTree->mapToGlobal(pos));
}
```

**Tasks Day 5:**
- [ ] Replace simulated service tree with real FIG data
- [ ] Add service type icons
- [ ] Add extended label tooltips (Thai support)
- [ ] Enhance context menu
- [ ] Add color coding for service types
- [ ] Test with Thai DAB broadcasts

---

## Week 2: Audio Quality & FIG Details

### **Day 1-2: Audio Quality Panel**

**New Dockable Panel:**
```cpp
// NEW: src/gui/audio_quality_panel.hpp
class AudioQualityPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit AudioQualityPanel(QWidget* parent = nullptr);

public slots:
    void updateAudioMetrics(const AudioQualityMetrics& metrics);

private:
    QLabel* m_bitrateLabel;
    QLabel* m_protectionLabel;
    QLabel* m_sbrStatusLabel;      // Spectral Band Replication
    QLabel* m_psStatusLabel;       // Parametric Stereo
    QProgressBar* m_qualityBar;    // Quality estimate 0-100%

    // Audio coding indicators
    QLabel* m_rsValidIcon;         // Reed-Solomon ✓/✗
    QLabel* m_firecodeValidIcon;   // FireCode ✓/✗

    // Detailed metrics (collapsible)
    QGroupBox* m_detailedMetricsGroup;
    QLabel* m_sampleRateLabel;
    QLabel* m_channelsLabel;
    QLabel* m_codecVersionLabel;
};
```

**Connect to 1A Backend:**
```cpp
// Connect to DABPlusStreamValidator signals
connect(m_audioValidator, &DABPlusStreamValidator::metricsUpdated,
        m_audioQualityPanel, &AudioQualityPanel::updateAudioMetrics);

// Update when service selected in tree
connect(m_serviceTree, &QTreeWidget::currentItemChanged,
        this, &DABAnalyzerWindow::onServiceSelected);

void DABAnalyzerWindow::onServiceSelected(QTreeWidgetItem* item) {
    uint16_t service_id = getServiceIdFromItem(item);

    // Request audio metrics for this service from validator
    m_audioValidator->analyzeService(service_id);
}
```

**Tasks Days 1-2:**
- [ ] Create AudioQualityPanel widget
- [ ] Add audio parameter labels with icons
- [ ] Add quality progress bar with color coding
- [ ] Add Reed-Solomon/FireCode validation indicators
- [ ] Connect to DABPlusStreamValidator
- [ ] Add to View menu
- [ ] Test with DAB+ audio services

### **Day 3-4: Enhanced FIG Details View**

**Enhance Existing FIG Tab:**
```cpp
// MODIFY: src/main.cpp - FIG Details tab

void DABAnalyzerWindow::updateFIGDetails() {
    QString html;
    html += "<h2>FIG Analysis</h2>";

    // FIG 0 Summary
    html += "<h3>FIG Type 0 (MCI - Multiplex Configuration Information)</h3>";
    html += "<table border='1' cellpadding='5'>";
    html += "<tr><th>FIG</th><th>Extension</th><th>Count</th><th>Description</th></tr>";

    auto fig0_stats = m_fig0Processor->getFIGStatistics();
    for (const auto& [extension, count] : fig0_stats) {
        html += QString("<tr><td>FIG 0/%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                .arg(extension)
                .arg(getFIG0ExtensionName(extension))
                .arg(count)
                .arg(getFIG0Description(extension));
    }
    html += "</table><br>";

    // FIG 1 Summary
    html += "<h3>FIG Type 1 (Labels)</h3>";
    html += "<table border='1' cellpadding='5'>";
    html += "<tr><th>Label Type</th><th>Value</th></tr>";

    auto labels = m_fig1Processor->getAllLabels();
    html += QString("<tr><td>Ensemble Label</td><td><b>%1</b></td></tr>")
            .arg(QString::fromStdString(labels.ensemble_label));

    for (const auto& [sid, label] : labels.service_labels) {
        html += QString("<tr><td>Service 0x%1</td><td><b>%2</b></td></tr>")
                .arg(sid, 4, 16, QChar('0')).toUpper()
                .arg(QString::fromStdString(label));
    }
    html += "</table><br>";

    // FIG 2 Extended Labels (if any)
    if (!labels.extended_labels.empty()) {
        html += "<h3>FIG Type 2 (Extended Labels - UTF-8)</h3>";
        html += "<table border='1' cellpadding='5'>";
        for (const auto& [id, ext_label] : labels.extended_labels) {
            html += QString("<tr><td>0x%1</td><td>%2</td></tr>")
                    .arg(id, 4, 16, QChar('0')).toUpper()
                    .arg(QString::fromUtf8(ext_label.c_str()));  // Thai support!
        }
        html += "</table><br>";
    }

    m_figDetailsWidget->setHtml(html);
}
```

**Add FIG Type Filter:**
```cpp
// Add filter toolbar above FIG details
QToolBar* figFilterToolbar = new QToolBar("FIG Filter");
figFilterToolbar->addWidget(new QLabel("Show: "));
QCheckBox* showFIG0 = new QCheckBox("FIG 0");
QCheckBox* showFIG1 = new QCheckBox("FIG 1");
QCheckBox* showFIG2 = new QCheckBox("FIG 2");
showFIG0->setChecked(true);
showFIG1->setChecked(true);
showFIG2->setChecked(true);
figFilterToolbar->addWidget(showFIG0);
figFilterToolbar->addWidget(showFIG1);
figFilterToolbar->addWidget(showFIG2);
```

**Tasks Days 3-4:**
- [ ] Enhance FIG details tab with HTML tables
- [ ] Add FIG type filter toolbar
- [ ] Display all FIG 0 types with statistics
- [ ] Display FIG 1 labels with formatting
- [ ] Display FIG 2 extended labels (UTF-8)
- [ ] Add collapsible sections
- [ ] Test with real FIG data

### **Day 5: ETSI Compliance Dashboard**

**New Tab in Details Panel:**
```cpp
// NEW: Add "Compliance" tab
QWidget* complianceTab = new QWidget();
QVBoxLayout* complianceLayout = new QVBoxLayout(complianceTab);

// Compliance score widget
QLabel* scoreLabel = new QLabel();
QFont scoreFont = scoreLabel->font();
scoreFont.setPointSize(24);
scoreFont.setBold(true);
scoreLabel->setFont(scoreFont);
scoreLabel->setAlignment(Qt::AlignCenter);

// Standards compliance table
QTableWidget* standardsTable = new QTableWidget();
standardsTable->setColumnCount(3);
standardsTable->setHorizontalHeaderLabels({"Standard", "Compliance %", "Status"});

// Populate from ETSIComplianceEngine
auto compliance = m_complianceEngine->getOverallCompliance();
scoreLabel->setText(QString("%1%").arg(compliance, 0, 'f', 1));

// Color code based on score
if (compliance >= 95.0) {
    scoreLabel->setStyleSheet("color: green;");
} else if (compliance >= 80.0) {
    scoreLabel->setStyleSheet("color: orange;");
} else {
    scoreLabel->setStyleSheet("color: red;");
}

complianceLayout->addWidget(scoreLabel);
complianceLayout->addWidget(standardsTable);

m_detailsTabWidget->addTab(complianceTab, "Compliance");
```

**Tasks Day 5:**
- [ ] Create Compliance tab
- [ ] Add overall compliance score display
- [ ] Add standards compliance table
- [ ] Add color coding for compliance levels
- [ ] Add violation/warning details
- [ ] Link to ETSI documentation
- [ ] Test compliance reporting

---

## Week 3: SlideShow & Data Services

### **Day 1-3: SlideShow Image Viewer**

**New Dockable Panel:**
```cpp
// NEW: src/gui/slideshow_viewer.hpp
class SlideShowViewer : public QDockWidget {
    Q_OBJECT
public:
    explicit SlideShowViewer(QWidget* parent = nullptr);

public slots:
    void displaySlideShow(const SlideShowImage& image);
    void onImageRotation(int interval_ms);  // Auto-rotate timer

private:
    QLabel* m_imageLabel;                   // Display JPEG
    QLabel* m_titleLabel;                   // Image title
    QLabel* m_categoryLabel;                // Category
    QListWidget* m_imageHistory;            // Previous images
    QPushButton* m_saveButton;              // Save current image
    QTimer* m_autoRotateTimer;              // Auto-rotation

    std::map<int, SlideShowImage> m_imageCache;
};
```

**Implementation:**
```cpp
void SlideShowViewer::displaySlideShow(const SlideShowImage& image) {
    // Load JPEG from saved file (from 1A SlideShowDecoder)
    QPixmap pixmap(QString::fromStdString(image.jpeg_filepath));

    // Scale to fit while maintaining aspect ratio
    int maxWidth = m_imageLabel->width();
    int maxHeight = m_imageLabel->height();
    pixmap = pixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    m_imageLabel->setPixmap(pixmap);
    m_titleLabel->setText(QString::fromStdString(image.title));
    m_categoryLabel->setText(QString::fromStdString(image.category));

    // Add to history list
    QListWidgetItem* historyItem = new QListWidgetItem(
        QString("%1 - %2").arg(image.received_time.toString("HH:mm:ss"))
                          .arg(QString::fromStdString(image.title))
    );
    m_imageHistory->insertItem(0, historyItem);

    // Start auto-rotate timer if configured
    if (image.display_duration_ms > 0) {
        m_autoRotateTimer->start(image.display_duration_ms);
    }
}
```

**Connect to 1A Backend:**
```cpp
connect(m_slideShowDecoder, &SlideShowDecoder::slideShowImageReady,
        m_slideShowViewer, &SlideShowViewer::displaySlideShow);
```

**Tasks Days 1-3:**
- [ ] Create SlideShowViewer widget
- [ ] Implement JPEG display with scaling
- [ ] Add image title and metadata display
- [ ] Add image history list
- [ ] Implement auto-rotation timer
- [ ] Add "Save Image" functionality
- [ ] Add thumbnail preview in history
- [ ] Connect to SlideShowDecoder signals
- [ ] Test with Thai DAB SlideShow broadcasts

### **Day 4-5: Data Services Panel**

**New Tab in Details:**
```cpp
// NEW: Data Services tab
QWidget* dataServicesTab = new QWidget();
QVBoxLayout* dataLayout = new QVBoxLayout(dataServicesTab);

// MOT Object List
QGroupBox* motGroup = new QGroupBox("MOT Objects");
QTableWidget* motTable = new QTableWidget();
motTable->setColumnCount(5);
motTable->setHorizontalHeaderLabels({"Transport ID", "Type", "Size", "Progress", "Status"});

// Update from MOTProtocol
auto mot_objects = m_motProtocol->getActiveObjects();
for (const auto& obj : mot_objects) {
    int row = motTable->rowCount();
    motTable->insertRow(row);
    motTable->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(obj.transport_id, 4, 16, QChar('0'))));
    motTable->setItem(row, 1, new QTableWidgetItem(getContentTypeName(obj.content_type)));
    motTable->setItem(row, 2, new QTableWidgetItem(QString("%1 KB").arg(obj.body_size / 1024)));

    // Progress bar
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setValue(obj.reassembly_progress * 100);
    motTable->setCellWidget(row, 3, progressBar);

    motTable->setItem(row, 4, new QTableWidgetItem(obj.is_complete ? "Complete" : "Receiving"));
}

motGroup->setLayout(new QVBoxLayout());
motGroup->layout()->addWidget(motTable);
dataLayout->addWidget(motGroup);

m_detailsTabWidget->addTab(dataServicesTab, "Data Services");
```

**Tasks Days 4-5:**
- [ ] Create Data Services tab
- [ ] Add MOT object list table
- [ ] Add object reassembly progress bars
- [ ] Add content type indicators
- [ ] Add "Save Object" functionality
- [ ] Show SlideShow objects with preview
- [ ] Connect to MOTProtocol signals
- [ ] Test with data service streams

---

## Week 4: Emergency Alerts & Final Polish

### **Day 1-2: Emergency Alert Notification System**

**Alert Notification Widget:**
```cpp
// NEW: src/gui/emergency_alert_widget.hpp
class EmergencyAlertWidget : public QDialog {
    Q_OBJECT
public:
    explicit EmergencyAlertWidget(const EmergencyAlert& alert, QWidget* parent = nullptr);

    // Alert display based on severity
    void setAlertSeverity(uint8_t severity);  // 0=Minor, 3=Extreme

private:
    QLabel* m_severityIcon;        // Large warning icon
    QLabel* m_alertTypeLabel;      // "Earthquake", "Flood", etc.
    QLabel* m_headlineLabel;       // Bold headline
    QTextEdit* m_descriptionText;  // Alert description
    QTextEdit* m_instructionText;  // What to do
    QLabel* m_onsetTimeLabel;      // Alert onset time
    QLabel* m_expiresTimeLabel;    // Alert expiration
    QCheckBox* m_muteCheckbox;     // Mute alerts of this type
    QPushButton* m_acknowledgeButton;

    // Color scheme based on severity
    void setupSeverityColors(uint8_t severity);
};
```

**Alert Manager in Main Window:**
```cpp
// MODIFY: src/main.cpp

void DABAnalyzerWindow::onEmergencyAlertReceived(const EmergencyAlert& alert) {
    // Show modal alert dialog for Severe/Extreme
    if (alert.severity >= 2) {
        EmergencyAlertWidget* alertDialog = new EmergencyAlertWidget(alert, this);
        alertDialog->show();

        // Play alert sound (if configured)
        if (m_settings->value("alerts/sound_enabled", true).toBool()) {
            QSound::play(":/sounds/emergency_alert.wav");
        }
    }

    // Add to alert history
    m_alertHistoryPanel->addAlert(alert);

    // Update status bar
    m_statusBar->showMessage(
        QString("⚠️ EMERGENCY ALERT: %1").arg(QString::fromStdString(alert.headline)),
        10000  // 10 seconds
    );
}
```

**Alert History Panel:**
```cpp
// NEW: src/gui/alert_history_panel.hpp
class AlertHistoryPanel : public QDockWidget {
    Q_OBJECT
public:
    void addAlert(const EmergencyAlert& alert);

private:
    QTableWidget* m_alertTable;
    // Columns: Time | Type | Severity | Headline | Status
};
```

**Tasks Days 1-2:**
- [ ] Create EmergencyAlertWidget dialog
- [ ] Add severity-based color coding
- [ ] Implement alert sound playback
- [ ] Create AlertHistoryPanel
- [ ] Add alert filtering by severity/type
- [ ] Add "Acknowledge" functionality
- [ ] Connect to EWSAlertProcessor signals
- [ ] Test with simulated emergency alerts

### **Day 3: Settings & Configuration Dialog**

**Enhanced Settings:**
```cpp
// NEW: src/gui/settings_dialog.hpp
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private:
    // General settings
    QComboBox* m_languageCombo;         // UI language (English/Thai)
    QCheckBox* m_autoLoadLastFile;

    // Network settings
    QLineEdit* m_defaultEDIAddress;
    QSpinBox* m_defaultEDIPort;
    QCheckBox* m_enablePFT;
    QSpinBox* m_fecLevel;

    // Display settings
    QCheckBox* m_showExtendedLabels;
    QFontComboBox* m_treeFontCombo;
    QCheckBox* m_enableSlideShowAutoRotate;
    QSpinBox* m_slideShowInterval;

    // Emergency alert settings
    QCheckBox* m_enableAlertSounds;
    QComboBox* m_minAlertSeverity;      // Minimum severity to show
    QCheckBox* m_muteExpiredAlerts;

    // Compliance settings
    QComboBox* m_targetComplianceLevel; // Strict/Broadcast/Basic

    void loadSettings();
    void saveSettings();
};
```

**Tasks Day 3:**
- [ ] Create SettingsDialog
- [ ] Add all configuration options
- [ ] Implement settings persistence (QSettings)
- [ ] Add settings validation
- [ ] Add "Restore Defaults" button
- [ ] Test settings application

### **Day 4: Professional Polish**

**Visual Enhancements:**
- [ ] Add application icon
- [ ] Add toolbar icons for all actions
- [ ] Add service type icons (DAB/DAB+/Data)
- [ ] Add status icons for validation (✓/✗/⚠)
- [ ] Improve color scheme consistency
- [ ] Add keyboard shortcuts
- [ ] Add tooltips to all widgets

**Usability Improvements:**
- [ ] Add recent files menu
- [ ] Add drag-and-drop for ETI files
- [ ] Add double-click actions on service tree
- [ ] Add copy/paste for hex viewer
- [ ] Add export functions (PDF/HTML reports)
- [ ] Add help documentation (F1)

**Performance Optimization:**
- [ ] Optimize service tree updates (batch updates)
- [ ] Optimize FIG details HTML rendering
- [ ] Add UI update throttling for high-rate updates
- [ ] Memory usage optimization

**Tasks Day 4:**
- [ ] Apply all visual enhancements
- [ ] Implement usability features
- [ ] Performance profiling and optimization
- [ ] Fix any UI glitches
- [ ] Cross-platform testing (if applicable)

### **Day 5: Final Integration Testing**

**End-to-End Scenarios:**
1. **EDI Network Stream:**
   - Open network stream → Monitor statistics → Browse services → View audio quality

2. **File Analysis:**
   - Open ETI file → Navigate frames → Analyze FIG → View compliance

3. **SlideShow:**
   - Detect SlideShow service → Display images → Auto-rotate → Save images

4. **Emergency Alerts:**
   - Receive alert → Display notification → Check alert history → Acknowledge

5. **Export Reports:**
   - Generate service list → Generate compliance report → Export to PDF/HTML

**Tasks Day 5:**
- [ ] Run all end-to-end scenarios
- [ ] Test with Thai DAB broadcasts
- [ ] Validate all GUI elements with real data
- [ ] Fix any integration bugs
- [ ] Document any known issues
- [ ] Prepare for user acceptance testing

---

## Success Criteria

### **Release 1B Completion Requirements**

#### **Functional Requirements:**
- ✅ All 1A processors integrated with GUI
- ✅ EDI network stream source selectable
- ✅ Service tree populated with real data
- ✅ Audio quality metrics displayed
- ✅ SlideShow images displayed
- ✅ Emergency alerts shown with notifications
- ✅ ETSI compliance dashboard functional
- ✅ Settings configurable and persistent

#### **Quality Metrics:**
- ✅ **UI Responsiveness:** No freezing during processing
- ✅ **Data Accuracy:** GUI shows correct processor data
- ✅ **Thai Support:** Extended labels display correctly
- ✅ **Professional Appearance:** Consistent styling
- ✅ **Usability:** Intuitive navigation and workflows
- ✅ **Stability:** No GUI crashes

#### **User Experience:**
- ✅ Easy to open ETI files or network streams
- ✅ Clear service discovery and browsing
- ✅ Informative audio quality indicators
- ✅ Engaging SlideShow viewer
- ✅ Attention-grabbing emergency alerts
- ✅ Comprehensive compliance reporting
- ✅ Professional presentation suitable for broadcast engineers

---

## Timeline Summary

### **Release 1B: 3-4 Weeks**

| Week | Focus | Deliverables |
|------|-------|--------------|
| **Week 1** | EDI Network & Service Tree | Network source, enhanced tree |
| **Week 2** | Audio & FIG Details | Quality panel, FIG viewer, compliance |
| **Week 3** | SlideShow & Data Services | Image viewer, MOT panel |
| **Week 4** | Alerts & Polish | EWS notifications, settings, final polish |

**Total Release 1 Timeline:**
- Release 1A (Processors): 10-12 weeks
- Release 1B (GUI): 3-4 weeks
- **Combined:** 13-16 weeks to 100% complete Thai DAB analyzer

---

## Integration with Release 1A

All Release 1B GUI components connect to completed 1A processors via Qt signals/slots:

```cpp
// Example: Connecting all processors to GUI
class DABAnalyzerWindow : public QMainWindow {
private:
    // 1A Processors (backend)
    EDINetworkReceiver* m_ediReceiver;
    FIG0Processor* m_fig0Processor;
    FIG1Processor* m_fig1Processor;
    FIG2Processor* m_fig2Processor;
    DABPlusStreamValidator* m_audioValidator;
    SlideShowDecoder* m_slideShowDecoder;
    EWSAlertProcessor* m_alertProcessor;
    ETSIComplianceEngine* m_complianceEngine;

    // 1B GUI Widgets
    EDINetworkPanel* m_ediNetworkPanel;
    AudioQualityPanel* m_audioQualityPanel;
    SlideShowViewer* m_slideShowViewer;
    AlertHistoryPanel* m_alertHistoryPanel;
    EmergencyAlertWidget* m_currentAlert;

    void connectProcessorsToGUI() {
        // EDI
        connect(m_ediReceiver, &EDINetworkReceiver::networkStatsUpdated,
                m_ediNetworkPanel, &EDINetworkPanel::updateNetworkStats);

        // Services
        connect(m_fig0Processor, &FIG0Processor::servicesUpdated,
                this, &DABAnalyzerWindow::populateServiceTree);

        // Audio
        connect(m_audioValidator, &DABPlusStreamValidator::metricsUpdated,
                m_audioQualityPanel, &AudioQualityPanel::updateAudioMetrics);

        // SlideShow
        connect(m_slideShowDecoder, &SlideShowDecoder::slideShowImageReady,
                m_slideShowViewer, &SlideShowViewer::displaySlideShow);

        // Emergency Alerts
        connect(m_alertProcessor, &EWSAlertProcessor::emergencyAlertReceived,
                this, &DABAnalyzerWindow::onEmergencyAlertReceived);

        // Compliance
        connect(m_complianceEngine, &ETSIComplianceEngine::complianceUpdated,
                this, &DABAnalyzerWindow::updateComplianceDashboard);
    }
};
```

---

## Document Status

**Status:** READY FOR REVIEW
**Prerequisites:** Release 1A must be 100% complete
**Next Actions:**
1. Review Release 1B plan
2. Confirm GUI framework is ready
3. Begin Week 1 integration after 1A completion

**Related Documents:**
- [RELEASE_1A_CORE_PROCESSORS_PLAN.md](RELEASE_1A_CORE_PROCESSORS_PLAN.md) - Backend processors
- [THAIDAB_RELEASE_1_IMPLEMENTATION_PLAN.md](THAIDAB_RELEASE_1_IMPLEMENTATION_PLAN.md) - Overall plan

---

**Last Updated:** October 19, 2025
**Version:** 1.0
