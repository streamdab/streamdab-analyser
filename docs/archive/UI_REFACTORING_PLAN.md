# UI Refactoring Plan - Align with DATA_AVAILABILITY_MATRIX.md

> **Date:** 2025-10-25  
> **Goal:** Refactor main.cpp to match 3-tab specification from DATA_AVAILABILITY_MATRIX.md  
> **Strategy:** Option 3 - Remove Multi-stream (future feature), relocate Error Detection and Logging to appropriate tabs  
> **Status:** 🔄 Planning → Implementation

---

## Executive Summary

### Current State (4 tabs - CONFLICTING):
```
Tab 1: DAB Ensemble & ETI Analysis (3-panel layout)
Tab 2: FIC-Analyzer (3-panel layout)
Tab 3: FIC-Extractor (2-panel layout)
Tab 4: Advanced Features ❌ NOT IN SPEC
  ├── Sub-Tab 1: Multi-stream Processing (~200 lines) ❌ REMOVE
  ├── Sub-Tab 2: Advanced Error Detection (~150 lines) ✂️ RELOCATE
  └── Sub-Tab 3: Professional Logging (~180 lines) ✂️ RELOCATE
```

### Target State (3 tabs - PER SPEC):
```
Tab 1: DAB Ensemble & ETI Analysis (3-panel layout)
  ├── LEFT PANEL:
  │   ├── Player Panel ✅
  │   ├── Decoder Panel ✅
  │   ├── System Panel (FIC Overview + Control) ✅
  │   └── Messages Panel (Professional Logging) ✂️ MOVED FROM TAB 4
  ├── CENTER PANEL:
  │   ├── DAB Ensemble Explorer ✅
  │   ├── ETI Frame Navigator ✅
  │   ├── Hex Viewer / ETSI Compliance ✅
  │   ├── Subchannel Organization Table ❌ MISSING
  │   └── Now Playing (DLS+) ✅
  ├── RIGHT PANEL:
  │   ├── ETI Overview ✅
  │   ├── DAB Error Counter ❌ MISSING (Error Detection) ✂️ MOVED FROM TAB 4
  │   ├── Timing ✅
  │   ├── CU Usage Chart ❌ MISSING
  │   └── DAB Stream Statistics ❌ MISSING

Tab 2: FIC-Analyzer (3-panel layout)
  ├── LEFT PANEL: Overview, Service Tree, Digital Org Info ✅
  ├── CENTER PANEL: Ensemble/Subchannel/Service Tables ✅
  └── RIGHT PANEL: FIG Content Table ✅

Tab 3: FIC-Extractor (2-panel layout)
  ├── LEFT PANEL: FIG Instance Tree ✅
  └── RIGHT PANEL: FIG Item Details + FIB Hex Viewer ✅
```

---

## Phase 1: Remove Tab 4 Multi-stream Processing

### 1.1 Code to Remove (~200 lines)

**File:** `src/main.cpp`

**Lines to Delete:**
- Line 2126-2129: `createTab4_AdvancedFeatures()` call
- Line 2623-2650: `createTab4_AdvancedFeatures()` method
- Line 2647-2677: `createTab4_MultiStream()` method

**Member Variables to Remove:**
```cpp
// From class MainWindow private section
QTreeWidget* m_activeStreamsTree;      // Line ~1200
QTextEdit* m_comparativeResults;       // Line ~1210
```

**Signals/Slots to Remove:**
```cpp
// Any connections related to Multi-stream
connect(addStreamBtn, ...) 
connect(compareBtn, ...)
```

**Documentation to Update:**
```markdown
# docs/FUTURE_FEATURES.md (CREATE NEW)

## Future Features - StreamDAB Analyzer

### Multi-stream Comparative Analysis (Phase 5)

**Status:** ⏳ Planned for Phase 5 (not implemented)

**Goal:** Simultaneous analysis of multiple ETI streams for comparison

**Features:**
- Load and analyze 2-4 ETI streams simultaneously
- Side-by-side FIG comparison
- Service organization differences
- Frame timing comparison
- Ensemble metadata comparison

**Use Cases:**
- Compare DAB multiplexers from different vendors
- Before/after analysis for configuration changes
- Multi-region DAB broadcast analysis
- Quality assurance testing

**Architecture Pattern:** Pattern 3 + 5 (Multi-worker threads + Shared data model)

**Estimated Timeline:** 3-4 weeks
**Priority:** Low (advanced feature for professional use)
```

---

## Phase 2: Relocate Advanced Error Detection to Tab 1 Right Panel

### 2.1 Current Implementation (~150 lines)

**Source:** `createTab4_ErrorDetection()` at line 2678-2710

**Widgets to Relocate:**
```cpp
// Status widgets
QLabel* m_errorDetectionStatus;  // "Status: Active"
QLabel* m_systemHealthScore;     // "Health: 95%"
QLabel* m_errorCountLabel;       // "Errors: 0"

// History tree
QTreeWidget* m_errorHistoryTree;  // Headers: Timestamp, Severity, Type, Description

// Patterns display
QTextEdit* m_errorPatternsDisplay;  // Read-only error patterns
```

**Backend Integration:**
```cpp
// Already exists: src/core/advanced_error_detector.cpp/hpp
class AdvancedErrorDetector {
    // FIC CRC errors
    // MSC CRC errors
    // Invalid FIG types
    // Service organization errors
    // Subchannel errors
};
```

### 2.2 Target Location: Tab 1 Right Panel

**Position:** After "Timing" QGroupBox, before bottom margin

**New QGroupBox:** "DAB Error Counter"

**Implementation Strategy:**

```cpp
QWidget* createTab1_RightPanel()
{
    QVBoxLayout* layout = new QVBoxLayout();
    
    // ... existing widgets (ETI Overview, Timing) ...
    
    // NEW: DAB Error Counter (from Tab 4 Error Detection)
    QGroupBox* errorGroup = createTab1_ErrorCounterPanel();
    layout->addWidget(errorGroup);
    
    // NEW: DAB Stream Statistics (missing)
    QGroupBox* statsGroup = createTab1_StreamStatisticsPanel();
    layout->addWidget(statsGroup);
    
    layout->addStretch();
    return widget;
}

QGroupBox* createTab1_ErrorCounterPanel()
{
    QGroupBox* group = new QGroupBox("DAB Error Counter");
    QVBoxLayout* layout = new QVBoxLayout(group);
    
    // Status bar (compact version)
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_errorCountLabel = new QLabel("Errors: 0");
    m_systemHealthScore = new QLabel("Health: 100%");
    statusLayout->addWidget(m_errorCountLabel);
    statusLayout->addWidget(m_systemHealthScore);
    statusLayout->addStretch();
    layout->addLayout(statusLayout);
    
    // Error table (QTableWidget instead of QTreeWidget for compactness)
    m_errorCounterTable = new QTableWidget();
    m_errorCounterTable->setColumnCount(2);
    m_errorCounterTable->setHorizontalHeaderLabels({"Error Type", "Count"});
    m_errorCounterTable->setMaximumHeight(150);  // Compact
    
    // Populate with DAB error categories
    QStringList errorTypes = {
        "FIC CRC Errors",
        "MSC CRC Errors",
        "Invalid FIG Types",
        "Service Org Errors",
        "Subchannel Errors"
    };
    
    m_errorCounterTable->setRowCount(errorTypes.size());
    for (int i = 0; i < errorTypes.size(); i++) {
        m_errorCounterTable->setItem(i, 0, new QTableWidgetItem(errorTypes[i]));
        m_errorCounterTable->setItem(i, 1, new QTableWidgetItem("0"));
    }
    
    layout->addWidget(m_errorCounterTable);
    
    // Add "View Details" button (expands to show history tree)
    QPushButton* detailsBtn = new QPushButton("View Error History");
    connect(detailsBtn, &QPushButton::clicked, this, &MainWindow::showErrorHistoryDialog);
    layout->addWidget(detailsBtn);
    
    return group;
}

// Separate dialog for detailed error history (optional)
void MainWindow::showErrorHistoryDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Error History");
    dialog->resize(800, 600);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    // Full error history tree (from Tab 4)
    QTreeWidget* errorHistoryTree = new QTreeWidget();
    errorHistoryTree->setHeaderLabels({"Timestamp", "Severity", "Type", "Description"});
    layout->addWidget(errorHistoryTree);
    
    // Error patterns display
    QGroupBox* patternsGroup = new QGroupBox("Error Patterns Analysis");
    QVBoxLayout* patternsLayout = new QVBoxLayout(patternsGroup);
    QTextEdit* patternsDisplay = new QTextEdit();
    patternsDisplay->setReadOnly(true);
    patternsLayout->addWidget(patternsDisplay);
    layout->addWidget(patternsGroup);
    
    // Copy data from main window's error detector
    // ... populate errorHistoryTree and patternsDisplay ...
    
    dialog->exec();
}
```

**Key Changes:**
- ✅ **Compact Table** in Tab 1 Right Panel (always visible)
- ✅ **Detailed Dialog** on demand (preserves full functionality from Tab 4)
- ✅ **Data Binding** to `AdvancedErrorDetector` backend (already implemented)

---

## Phase 3: Relocate Professional Logging to Tab 1 Left Panel

### 3.1 Current Implementation (~180 lines)

**Source:** `createTab4_Logging()` at line 2711-2750

**Widgets to Relocate:**
```cpp
// Status widgets
QLabel* m_loggingStatus;      // "Status: Active"
QLabel* m_loggingLevel;       // "Level: INFO"
QLabel* m_logCountLabel;      // "Entries: 0"

// Log entries tree
QTreeWidget* m_logEntriesTree;  // Headers: Timestamp, Level, Category, Message

// Statistics display
QTextEdit* m_loggingStatsDisplay;  // Read-only logging stats
```

**Backend Integration:**
```cpp
// Already exists: src/utils/professional_logging_system.cpp/hpp
class ProfessionalLoggingSystem {
    // Log file management
    // Category filtering (HCI, SBCH, MCI)
    // Severity levels (INFO, WARNING, ERROR)
    // CIF tracking
};
```

### 3.2 Target Location: Tab 1 Left Panel

**Position:** After "System Panel", before bottom margin

**New QGroupBox:** "Messages Panel"

**Implementation Strategy:**

```cpp
QWidget* createTab1_LeftPanel()
{
    QVBoxLayout* layout = new QVBoxLayout();
    
    // ... existing widgets (Player, Decoder, System) ...
    
    // NEW: Messages Panel (from Tab 4 Professional Logging)
    QGroupBox* messagesGroup = createTab1_MessagesPanel();
    layout->addWidget(messagesGroup);
    
    return widget;
}

QGroupBox* createTab1_MessagesPanel()
{
    QGroupBox* group = new QGroupBox("Messages");
    QVBoxLayout* layout = new QVBoxLayout(group);
    
    // Control bar
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    
    // Log file path display
    m_logFilePathLabel = new QLabel("Log: /tmp/streamdab.log");
    m_logFilePathLabel->setStyleSheet("QLabel { color: #888; font-size: 9pt; }");
    controlsLayout->addWidget(m_logFilePathLabel);
    
    controlsLayout->addStretch();
    
    // Filter button
    QPushButton* filterBtn = new QPushButton("Filter");
    filterBtn->setMaximumWidth(60);
    connect(filterBtn, &QPushButton::clicked, this, &MainWindow::showLogFilterDialog);
    controlsLayout->addWidget(filterBtn);
    
    // Clear button
    QPushButton* clearBtn = new QPushButton("Clear");
    clearBtn->setMaximumWidth(60);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearLogMessages);
    controlsLayout->addWidget(clearBtn);
    
    layout->addLayout(controlsLayout);
    
    // Messages table (compact 6-column version from spec)
    m_messagesTable = new QTableWidget();
    m_messagesTable->setColumnCount(6);
    m_messagesTable->setHorizontalHeaderLabels({
        "Time", "CIF", "Source", "Class", "Message", "Action"
    });
    
    // Column widths (spec requirements)
    m_messagesTable->setColumnWidth(0, 80);   // Time: HH:MM:SS
    m_messagesTable->setColumnWidth(1, 40);   // CIF: 0-3
    m_messagesTable->setColumnWidth(2, 80);   // Source: Analyzer/Application
    m_messagesTable->setColumnWidth(3, 60);   // Class: HCI/SBCH/MCI
    m_messagesTable->setColumnWidth(4, 400);  // Message: Full text
    m_messagesTable->setColumnWidth(5, 40);   // Action: Icon button
    
    m_messagesTable->setMaximumHeight(200);  // Compact for Left Panel
    m_messagesTable->setAlternatingRowColors(true);
    
    layout->addWidget(m_messagesTable);
    
    // Status bar
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_logCountLabel = new QLabel("Entries: 0");
    m_loggingStatus = new QLabel("Status: Active");
    statusLayout->addWidget(m_logCountLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_loggingStatus);
    layout->addLayout(statusLayout);
    
    return group;
}

// Filter dialog
void MainWindow::showLogFilterDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Message Filter");
    dialog->resize(400, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    // Severity checkboxes
    QGroupBox* severityGroup = new QGroupBox("Severity Levels");
    QVBoxLayout* severityLayout = new QVBoxLayout(severityGroup);
    QCheckBox* infoCheck = new QCheckBox("INFO");
    QCheckBox* warningCheck = new QCheckBox("WARNING");
    QCheckBox* errorCheck = new QCheckBox("ERROR");
    infoCheck->setChecked(true);
    warningCheck->setChecked(true);
    errorCheck->setChecked(true);
    severityLayout->addWidget(infoCheck);
    severityLayout->addWidget(warningCheck);
    severityLayout->addWidget(errorCheck);
    layout->addWidget(severityGroup);
    
    // Category checkboxes
    QGroupBox* categoryGroup = new QGroupBox("Message Classes");
    QVBoxLayout* categoryLayout = new QVBoxLayout(categoryGroup);
    QCheckBox* hciCheck = new QCheckBox("HCI (Human Computer Interface)");
    QCheckBox* sbchCheck = new QCheckBox("SBCH (Service Component Header)");
    QCheckBox* mciCheck = new QCheckBox("MCI (Multiplex Configuration Info)");
    hciCheck->setChecked(true);
    sbchCheck->setChecked(true);
    mciCheck->setChecked(true);
    categoryLayout->addWidget(hciCheck);
    categoryLayout->addWidget(sbchCheck);
    categoryLayout->addWidget(mciCheck);
    layout->addWidget(categoryGroup);
    
    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* applyBtn = new QPushButton("Apply");
    QPushButton* cancelBtn = new QPushButton("Cancel");
    connect(applyBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
    btnLayout->addStretch();
    btnLayout->addWidget(applyBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);
    
    if (dialog->exec() == QDialog::Accepted) {
        // Apply filter settings to m_messagesTable
        // ... implementation ...
    }
}

void MainWindow::clearLogMessages()
{
    m_messagesTable->setRowCount(0);
    m_logCountLabel->setText("Entries: 0");
}
```

**Data Binding (connect to ProfessionalLoggingSystem):**

```cpp
// In MainWindow::setupConnections()
connect(m_loggingSystem, &ProfessionalLoggingSystem::logMessageReceived,
        this, &MainWindow::onLogMessageReceived);

void MainWindow::onLogMessageReceived(const LogEntry& entry)
{
    // Add row to m_messagesTable
    int row = m_messagesTable->rowCount();
    m_messagesTable->insertRow(row);
    
    m_messagesTable->setItem(row, 0, new QTableWidgetItem(
        entry.timestamp.toString("HH:mm:ss")));
    m_messagesTable->setItem(row, 1, new QTableWidgetItem(
        QString::number(entry.cif)));
    m_messagesTable->setItem(row, 2, new QTableWidgetItem(entry.source));
    m_messagesTable->setItem(row, 3, new QTableWidgetItem(entry.category));
    m_messagesTable->setItem(row, 4, new QTableWidgetItem(entry.message));
    
    // Action button (e.g., copy to clipboard)
    QPushButton* actionBtn = new QPushButton("📋");
    actionBtn->setMaximumWidth(30);
    connect(actionBtn, &QPushButton::clicked, [=]() {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(entry.message);
    });
    m_messagesTable->setCellWidget(row, 5, actionBtn);
    
    // Update count
    m_logCountLabel->setText(QString("Entries: %1").arg(m_messagesTable->rowCount()));
    
    // Scroll to bottom
    m_messagesTable->scrollToBottom();
}
```

**Key Changes:**
- ✅ **Compact Table** in Tab 1 Left Panel (always visible, max 200px height)
- ✅ **Filter Dialog** for advanced filtering (preserves functionality)
- ✅ **6-Column Format** per DATA_AVAILABILITY_MATRIX.md spec
- ✅ **Data Binding** to `ProfessionalLoggingSystem` backend
- ✅ **Action Buttons** for per-message operations (copy, details)

---

## Phase 4: Add Missing Panels to Tab 1

### 4.1 Missing Panel: Subchannel Organization Table

**Location:** Tab 1 Center Panel, after "ETI Frame Navigator"

**Data Source:** `FIGAnalysisResult::subchannels` (FIG 0/1 parser)

**Implementation:**

```cpp
QGroupBox* createTab1_SubchannelOrganizationTable()
{
    QGroupBox* group = new QGroupBox("Subchannel Organization");
    QVBoxLayout* layout = new QVBoxLayout(group);
    
    // Table with 8 columns per spec
    m_subchannelTable = new QTableWidget();
    m_subchannelTable->setColumnCount(8);
    m_subchannelTable->setHorizontalHeaderLabels({
        "☐",       // Checkbox
        "Subch ID",
        "Type",    // UEP/EEP
        "Level",   // Protection level
        "Bitrate", // kbps
        "SAD",     // Start Address
        "Size",    // CUs
        "Errors"   // Error count
    });
    
    // Column widths
    m_subchannelTable->setColumnWidth(0, 30);   // Checkbox
    m_subchannelTable->setColumnWidth(1, 70);   // Subch ID
    m_subchannelTable->setColumnWidth(2, 50);   // Type
    m_subchannelTable->setColumnWidth(3, 50);   // Level
    m_subchannelTable->setColumnWidth(4, 70);   // Bitrate
    m_subchannelTable->setColumnWidth(5, 50);   // SAD
    m_subchannelTable->setColumnWidth(6, 50);   // Size
    m_subchannelTable->setColumnWidth(7, 60);   // Errors
    
    m_subchannelTable->setMaximumHeight(200);  // Compact
    m_subchannelTable->setAlternatingRowColors(true);
    
    layout->addWidget(m_subchannelTable);
    
    return group;
}

// Data binding (call when FIG 0/1 updated)
void MainWindow::updateSubchannelTable(const QVector<SubChannelInfo>& subchannels)
{
    m_subchannelTable->setRowCount(subchannels.size());
    
    for (int i = 0; i < subchannels.size(); i++) {
        const auto& subch = subchannels[i];
        
        // Checkbox
        QCheckBox* checkbox = new QCheckBox();
        checkbox->setChecked(true);  // Default: all enabled
        m_subchannelTable->setCellWidget(i, 0, checkbox);
        
        // Subch ID
        m_subchannelTable->setItem(i, 1, new QTableWidgetItem(
            QString::number(subch.subchannel_id)));
        
        // Type (UEP/EEP)
        QString type = (subch.protection_type == 0) ? "UEP" : "EEP";
        m_subchannelTable->setItem(i, 2, new QTableWidgetItem(type));
        
        // Level
        m_subchannelTable->setItem(i, 3, new QTableWidgetItem(
            QString::number(subch.protection_level)));
        
        // Bitrate (calculate from size and protection)
        int bitrate = calculateBitrate(subch.subchannel_size, 
                                        subch.protection_type,
                                        subch.protection_level);
        m_subchannelTable->setItem(i, 4, new QTableWidgetItem(
            QString("%1 kbps").arg(bitrate)));
        
        // SAD
        m_subchannelTable->setItem(i, 5, new QTableWidgetItem(
            QString::number(subch.start_address)));
        
        // Size (CUs)
        m_subchannelTable->setItem(i, 6, new QTableWidgetItem(
            QString("%1 CU").arg(subch.subchannel_size)));
        
        // Errors (TODO: integrate with AdvancedErrorDetector)
        m_subchannelTable->setItem(i, 7, new QTableWidgetItem("0"));
    }
}
```

**Status:** ✅ Data available from FIG 0/1, UI implementation needed

---

### 4.2 Missing Panel: CU Usage Chart

**Location:** Tab 1 Right Panel, after "DAB Error Counter"

**Data Source:** Sum of `SubChannelInfo::subchannel_size` from all subchannels

**Implementation:**

```cpp
QGroupBox* createTab1_CUUsageChart()
{
    QGroupBox* group = new QGroupBox("CU Usage (Capacity Units)");
    QVBoxLayout* layout = new QVBoxLayout(group);
    
    // Custom stacked bar chart widget
    m_cuUsageChart = new CUUsageChartWidget();
    m_cuUsageChart->setMaximumHeight(150);
    layout->addWidget(m_cuUsageChart);
    
    // Summary label
    m_cuUsageSummary = new QLabel("Total: 0 / 864 CU (0%)");
    m_cuUsageSummary->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_cuUsageSummary);
    
    return group;
}

// Custom widget for stacked bar chart
class CUUsageChartWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit CUUsageChartWidget(QWidget* parent = nullptr) : QWidget(parent) {
        m_totalCU = 864;  // Mode 1 default
    }
    
    void setSubchannelData(const QVector<SubChannelInfo>& subchannels) {
        m_subchannels = subchannels;
        update();  // Trigger repaint
    }
    
protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw background
        painter.fillRect(rect(), QColor(240, 240, 240));
        
        // Calculate bar dimensions
        int barHeight = 40;
        int barY = (height() - barHeight) / 2;
        int barWidth = width() - 40;
        int barX = 20;
        
        // Draw frame
        painter.setPen(Qt::black);
        painter.drawRect(barX, barY, barWidth, barHeight);
        
        // Draw subchannel segments (stacked)
        QVector<QColor> colors = {
            QColor(52, 152, 219),   // Blue
            QColor(46, 204, 113),   // Green
            QColor(155, 89, 182),   // Purple
            QColor(241, 196, 15),   // Yellow
            QColor(231, 76, 60),    // Red
            QColor(26, 188, 156),   // Turquoise
            QColor(230, 126, 34),   // Orange
            QColor(149, 165, 166),  // Gray
        };
        
        int currentX = barX;
        int totalUsed = 0;
        
        for (int i = 0; i < m_subchannels.size(); i++) {
            const auto& subch = m_subchannels[i];
            int size = subch.subchannel_size;
            totalUsed += size;
            
            // Calculate segment width
            int segmentWidth = (barWidth * size) / m_totalCU;
            
            // Draw segment
            QColor color = colors[i % colors.size()];
            painter.fillRect(currentX, barY, segmentWidth, barHeight, color);
            
            // Draw label if segment is wide enough
            if (segmentWidth > 30) {
                painter.setPen(Qt::white);
                QFont font = painter.font();
                font.setBold(true);
                painter.setFont(font);
                QString label = QString::number(subch.subchannel_id);
                painter.drawText(currentX, barY, segmentWidth, barHeight,
                                Qt::AlignCenter, label);
            }
            
            currentX += segmentWidth;
        }
        
        // Draw unused capacity (gray)
        int unusedWidth = barWidth - (currentX - barX);
        if (unusedWidth > 0) {
            painter.fillRect(currentX, barY, unusedWidth, barHeight,
                            QColor(220, 220, 220));
        }
        
        // Draw percentage labels
        painter.setPen(Qt::black);
        QFont font = painter.font();
        font.setPointSize(9);
        painter.setFont(font);
        
        QString usageText = QString("%1 / %2 CU (%3%)")
            .arg(totalUsed)
            .arg(m_totalCU)
            .arg(totalUsed * 100 / m_totalCU);
        
        painter.drawText(barX, barY - 5, usageText);
    }
    
private:
    QVector<SubChannelInfo> m_subchannels;
    int m_totalCU;
};
```

**Alternative:** Use QtCharts library (requires `Qt6::Charts` dependency)

**Status:** ⚠️ Custom implementation needed (no external dependency preferred)

---

### 4.3 Missing Panel: DAB Stream Statistics

**Location:** Tab 1 Right Panel or Bottom Panel

**Data Source:** Aggregate statistics from parsing session

**Implementation:**

```cpp
QGroupBox* createTab1_StreamStatistics()
{
    QGroupBox* group = new QGroupBox("DAB Stream Statistics");
    QVBoxLayout* layout = new QVBoxLayout(group);
    
    // Statistics table (2 columns: Metric, Value)
    m_streamStatsTable = new QTableWidget();
    m_streamStatsTable->setColumnCount(2);
    m_streamStatsTable->setHorizontalHeaderLabels({"Metric", "Value"});
    m_streamStatsTable->horizontalHeader()->setStretchLastSection(true);
    m_streamStatsTable->setMaximumHeight(250);
    
    // Define 10+ metrics per spec
    QStringList metrics = {
        "FIC Blocks Processed",
        "Total FIG Count",
        "Unique Services Found",
        "Active Subchannels",
        "Total CU Used / 864",
        "DAB Services Count",
        "DAB+ Services Count",
        "Data Services Count",
        "Average Frame Rate",
        "Processing Time (ms/frame)",
        "FIC CRC Errors",
        "MSC CRC Errors"
    };
    
    m_streamStatsTable->setRowCount(metrics.size());
    for (int i = 0; i < metrics.size(); i++) {
        m_streamStatsTable->setItem(i, 0, new QTableWidgetItem(metrics[i]));
        m_streamStatsTable->setItem(i, 1, new QTableWidgetItem("0"));
    }
    
    layout->addWidget(m_streamStatsTable);
    
    return group;
}

// Update method (call periodically or on frame processed)
void MainWindow::updateStreamStatistics()
{
    if (!m_figAnalyzer) return;
    
    auto stats = m_figAnalyzer->getStatistics();
    
    m_streamStatsTable->item(0, 1)->setText(QString::number(stats.fic_blocks_processed));
    m_streamStatsTable->item(1, 1)->setText(QString::number(stats.total_fig_count));
    m_streamStatsTable->item(2, 1)->setText(QString::number(stats.unique_services));
    m_streamStatsTable->item(3, 1)->setText(QString::number(stats.active_subchannels));
    m_streamStatsTable->item(4, 1)->setText(QString("%1 / 864").arg(stats.total_cu_used));
    m_streamStatsTable->item(5, 1)->setText(QString::number(stats.dab_services));
    m_streamStatsTable->item(6, 1)->setText(QString::number(stats.dab_plus_services));
    m_streamStatsTable->item(7, 1)->setText(QString::number(stats.data_services));
    m_streamStatsTable->item(8, 1)->setText(QString("%1 fps").arg(stats.average_frame_rate, 0, 'f', 2));
    m_streamStatsTable->item(9, 1)->setText(QString("%1 ms").arg(stats.processing_time_ms, 0, 'f', 2));
    m_streamStatsTable->item(10, 1)->setText(QString::number(stats.fic_crc_errors));
    m_streamStatsTable->item(11, 1)->setText(QString::number(stats.msc_crc_errors));
}
```

**Status:** ✅ Data available, UI implementation straightforward

---

## Phase 5: Update Documentation

### 5.1 Files to Update

1. **docs/UI_DESIGN_CONFLICT_ANALYSIS.md**
   - Add resolution section: "Implemented Option 3"
   - Document actual changes made

2. **docs/DATA_AVAILABILITY_MATRIX.md**
   - Update status: All panels implemented ✅
   - Verify 100% coverage

3. **docs/FUTURE_FEATURES.md** (CREATE NEW)
   - Document Multi-stream as future feature (Phase 5)

4. **CLAUDE.md**
   - Update "Current Implementation Status" section
   - Update UI screenshot/description
   - Increment version to v1.1

5. **README.md**
   - Update feature list
   - Remove Multi-stream from current features
   - Add to roadmap

---

## Phase 6: Testing & Validation

### 6.1 Build Test
```bash
cd build
cmake --build . --clean-first
# Expected: Zero compilation errors
```

### 6.2 UI Verification Checklist

**Tab 1: DAB Ensemble & ETI Analysis**
- [ ] LEFT PANEL
  - [ ] Player Panel displays correctly
  - [ ] Decoder Panel displays correctly
  - [ ] System Panel (2 tabs: FIC Overview, Control)
  - [ ] Messages Panel with 6-column table
  - [ ] Filter button opens filter dialog
  - [ ] Clear button clears messages
  
- [ ] CENTER PANEL
  - [ ] DAB Ensemble Explorer tree (3 levels)
  - [ ] ETI Frame Navigator list
  - [ ] Hex Viewer tab displays hex dump
  - [ ] ETSI Compliance tab shows validation
  - [ ] Subchannel Organization table (8 columns)
  - [ ] Now Playing (DLS+) panel shows metadata
  
- [ ] RIGHT PANEL
  - [ ] ETI Overview table (4 rows)
  - [ ] DAB Error Counter table (5 error types)
  - [ ] "View Error History" button opens dialog
  - [ ] Timing table (4 rows)
  - [ ] CU Usage chart displays stacked bar
  - [ ] DAB Stream Statistics table (12+ metrics)

**Tab 2: FIC-Analyzer**
- [ ] All panels display correctly (no changes)

**Tab 3: FIC-Extractor**
- [ ] All panels display correctly (no changes)

**Tab 4: Advanced Features**
- [ ] ❌ Tab removed entirely
- [ ] ✅ No compilation errors related to removed code

### 6.3 E2E GUI Test Update

```bash
# Update test_gui_click_through_workflow.cpp
# Remove testWorkflow6_LiveStreamPanelClicks (if it tests Tab 4)
# Add testWorkflow6_MessagesPanel
# Add testWorkflow7_ErrorCounterPanel
# Add testWorkflow8_SubchannelTable

cd build/tests/gui
./test_gui_click_through_workflow
# Expected: All tests pass
```

### 6.4 Data Binding Verification

1. **Load ETI file** with known content
2. **Verify Tab 1 updates:**
   - Messages Panel populates with log entries
   - Subchannel Table shows correct subchannels
   - Error Counter increments on errors
   - CU Usage chart displays correctly
   - Stream Statistics update in real-time

3. **Verify Error Detection:**
   - Inject FIC CRC error → Error Counter updates
   - Inject invalid FIG → Error Counter updates
   - "View Error History" shows details

4. **Verify Logging:**
   - Log messages appear in Messages Panel
   - Filter dialog filters correctly
   - Clear button works
   - Log file path displays correctly

---

## Implementation Timeline

| Phase | Task | Estimated Time | Priority |
|-------|------|----------------|----------|
| **Phase 1** | Remove Multi-stream (Tab 4 Sub-Tab 1) | 30 min | **HIGH** |
| **Phase 2** | Relocate Error Detection to Tab 1 | 2 hours | **HIGH** |
| **Phase 3** | Relocate Logging to Tab 1 | 2 hours | **HIGH** |
| **Phase 4.1** | Add Subchannel Organization Table | 1.5 hours | **MEDIUM** |
| **Phase 4.2** | Add CU Usage Chart | 2 hours | **MEDIUM** |
| **Phase 4.3** | Add Stream Statistics | 1 hour | **MEDIUM** |
| **Phase 5** | Update Documentation | 1 hour | **LOW** |
| **Phase 6** | Testing & Validation | 2 hours | **HIGH** |
| **TOTAL** | **Complete Refactoring** | **~12 hours** | - |

**Estimated Completion:** 1-2 working days

---

## Success Criteria

### Must Have (✅ Required):
- ✅ Tab 4 removed entirely (no compilation errors)
- ✅ Multi-stream marked as future feature in docs
- ✅ Error Detection relocated to Tab 1 Right Panel (compact table + dialog)
- ✅ Professional Logging relocated to Tab 1 Left Panel (6-column Messages table)
- ✅ Subchannel Organization Table added to Tab 1 Center Panel
- ✅ Zero compilation errors, zero test failures
- ✅ UI matches DATA_AVAILABILITY_MATRIX.md specification 100%

### Should Have (⚠️ Preferred):
- ⚠️ CU Usage Chart implemented (custom or QtCharts)
- ⚠️ DAB Stream Statistics panel implemented
- ⚠️ E2E GUI tests updated for new layout
- ⚠️ All documentation updated

### Nice to Have (💡 Optional):
- 💡 Error History dialog with patterns analysis
- 💡 Log Filter dialog with advanced options
- 💡 Per-message action buttons (copy, details)
- 💡 Animated CU Usage chart

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| **Breaking existing functionality** | Medium | High | Thorough testing, incremental changes |
| **UI layout too crowded** | Low | Medium | Use compact widgets, collapsible groups |
| **Missing backend integration** | Low | High | Verify all data sources exist |
| **E2E tests fail** | Medium | Medium | Update tests in parallel with UI changes |
| **Performance degradation** | Low | Medium | Profile after implementation |

---

## Rollback Plan

If refactoring causes issues:

1. **Git Revert:**
   ```bash
   git log --oneline -5
   git revert <commit-hash>
   ```

2. **Archive Refactored Code:**
   ```bash
   git branch refactor-tab4-removal-backup
   git checkout main
   ```

3. **Restore Tab 4:**
   - Restore deleted lines from git history
   - Re-add Tab 4 to main tabs
   - Document as "temporary restoration"

---

## Next Steps

1. **Review this plan** with user for approval
2. **Execute Phase 1** (remove Multi-stream)
3. **Execute Phase 2-3** (relocate Error Detection and Logging)
4. **Execute Phase 4** (add missing panels)
5. **Execute Phase 5-6** (documentation and testing)
6. **Commit and push** with detailed commit message

---

**Document Version:** 1.0  
**Date:** 2025-10-25  
**Status:** 📋 Planning Complete → ⏳ Awaiting User Approval → 🚀 Implementation
