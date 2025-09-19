# Claude Code Prompt: Desktop Application GUI Update - Stream Analyzer UI Pattern

## Context
You are a UX/UI sub-agent tasked with updating an existing desktop application's GUI to follow modern stream analyzer application patterns. Use the following professional stream analyzer interfaces as reference for creating a clean, functional, and industry-standard desktop application interface.

## Reference Applications to Emulate
- **Elecard Stream Analyzer 2024** - Professional stream analysis tool with comprehensive UI layout
- **DekTec StreamXpert** - Real-time stream analyzer with tabbed interface and multi-panel design
- **Industry-standard broadcasting/streaming analysis tools**

## Observed Key UI Patterns from Reference Applications

### From Elecard Stream Analyzer:
- **Left Explorer Panel**: Hierarchical tree structure showing Transport Stream, Services, PIDs, and Elementary Streams
- **Center Main Panel**: Packet listing with color-coded entries, offset information, and expandable details
- **Right Property Panel**: Detailed parameter information for selected items with sync/compare modes
- **Bottom Tabbed Panels**: Hex Viewer, Messages, TR101-290, Time Dynamics, Graphics, Comments, EPG
- **Top Ribbon Menu**: File, Navigation, View controls with search functionality
- **Status Bar**: Connection info, bitrate, and analysis statistics

### From DekTec StreamXpert:
- **Multi-tab Interface**: Clean tabs for Home, Decoding, TR 101-290, Recording, View
- **Service Tree**: Left panel showing services with bitrate percentages and PID information
- **Main Content Area**: Large central space for video preview, charts, or data tables
- **Real-time Charts**: PCR analysis graphs with grid lines and measurement tools
- **Control Buttons**: Freeze, Refresh, Reset with clear iconography
- **Input Adapter Info**: Device connection status prominently displayed

## Core UI/UX Requirements Checklist

### 1. Main Window Layout
- [ ] **Three-panel dockable layout**: Explorer (left) + Main Content (center) + Properties (right)
- [ ] **Top ribbon interface** with grouped command sections (File, Navigation, View)
- [ ] **Tabbed bottom panel** for secondary tools (Hex Viewer, Messages, TR101-290, Graphics, etc.)
- [ ] **Resizable splitters** between all panel sections with remember positions
- [ ] **Status bar** showing stream info, bitrate statistics, and connection status
- [ ] **Toolbar with icon buttons** for common actions (Freeze, Refresh, Reset, Record)

### 2. Panel Structure and Content Organization
- [ ] **Explorer Tree Panel** (left ~25% width):
  - Hierarchical expandable tree with (+/-) controls
  - Color-coded icons for different stream types (Transport Stream, Services, PIDs)
  - Percentage information showing stream utilization
  - Checkboxes for filtering displayed content
- [ ] **Main Content Area** (center ~50% width):
  - Packet/frame listing with offset addresses (hex format)
  - Color-coded rows (different packet types use different colors)
  - Expandable detail sections with braced parameters
  - Scrollbar with colored markers indicating errors/warnings
- [ ] **Properties Panel** (right ~25% width):
  - Detailed parameter display for selected items
  - Sync/Compare mode toggle buttons
  - Export/Dump functionality buttons
- [ ] **Bottom Tabbed Panel** (~25% height):
  - Multiple specialist tools accessible via tabs
  - Each tab contains domain-specific analysis tools

### 3. Stream Tree Navigation (ETI/DAB Specific)
- [ ] **Ensemble Tree Structure** with expandable hierarchy for ETI analysis
- [ ] **Service-based organization** (not PID-based) with Service ID (SID) identification
- [ ] **Subchannel breakdown** showing protection levels and bitrates
- [ ] **FIG Information display** for Fast Information Groups (0-31 types)
- [ ] **Color coding for content types** (Audio=green, Data=orange, Ensemble info=blue)
- [ ] **Right-click context menus** for service-specific actions (Extract audio, Analyze FIGs)

### 4. Main Display Area Features (ETI-Focused)
- [ ] **ETI Frame listing** with frame numbers (not packet offsets)
- [ ] **Service information display** with labels, program types, language codes
- [ ] **FIG analysis view** showing decoded Fast Information Groups
- [ ] **Audio parameters display** (DAB vs DAB+, protection level, bitrate)
- [ ] **Frame navigation** with 24ms precision for audio analysis
- [ ] **YAML output integration** displaying structured etisnoop results

### 5. Professional Tool Aesthetics (DAB Broadcasting Industry)
- [ ] **Broadcasting industry color scheme** (darker backgrounds suitable for long analysis sessions)
- [ ] **Monospace fonts for technical data** (ETI frame numbers, Service IDs, subchannel info)
- [ ] **Consistent iconography** for DAB/DAB+ services vs data services
- [ ] **Professional audio analysis styling** suitable for broadcast engineers
- [ ] **Frame-accurate timeline** displays for audio synchronization analysis

### 6. Data Display Standards
- [ ] **Monospace fonts** for technical data (Consolas, Courier New)
- [ ] **Color-coded status indicators** (Green=OK, Red=Error, Yellow=Warning)
- [ ] **Sortable table headers** with up/down arrows
- [ ] **Alternating row colors** for better readability
- [ ] **Tooltip help** on hover for technical parameters

### 7. Toolbar and Menu Organization
- [ ] **File operations** (Open, Save, Export, Recent files)
- [ ] **Stream control** (Start/Stop analysis, Record, Pause)
- [ ] **View options** (Zoom, Layout, Panels visibility)
- [ ] **Analysis tools** (Statistics, Reports, Alerts)
- [ ] **Configuration** (Settings, Preferences, Device setup)

### 8. Status and Feedback Elements
- [ ] **Connection status indicators** with LED-style icons
- [ ] **Real-time counters** (packets, errors, bitrate)
- [ ] **Progress bars** for file operations
- [ ] **Alert notifications** for errors or warnings
- [ ] **Timestamp displays** with precise formatting

### 9. Window Behavior
- [ ] **Dockable panels** that can be moved and resized
- [ ] **Splitter controls** between resizable sections
- [ ] **Remember window layouts** and user preferences
- [ ] **Full-screen mode** for main display
- [ ] **Multiple monitor support**

### 10. Professional Workflow Features
- [ ] **Keyboard shortcuts** for common operations
- [ ] **Quick filters** and search boxes
- [ ] **Export functionality** (CSV, XML, Reports)
- [ ] **Print support** for reports and data
- [ ] **Session management** (save/restore analysis sessions)

## Implementation Guidelines

### Color Palette Recommendations (Based on Reference Applications)
```
Background Colors:
- Main panels: #F8F8F8 (light mode) or #2D2D30 (dark mode)
- Explorer tree: #FFFFFF with #F0F0F0 alternating rows
- Selected items: #CCE8FF (light blue selection)
- Active tab: #FFFFFF with blue accent border

Content Color Coding (from StreamXpert/Elecard examples):
- Transport packets: Light blue background
- Video streams: Green text/icons
- Audio streams: Purple text/icons
- Data/teletext: Orange text/icons
- Error indicators: Red background/text
- Warning indicators: Yellow background

Status Colors:
- Connected/OK: Green (#89C442)
- Disconnected/Error: Red (#F44747)
- Warning: Orange (#FFCC02)
- Info/Neutral: Blue (#0078D4)
```

### Typography Hierarchy (Professional Tool Standards)
```
Window titles: 12pt Bold, Segoe UI
Panel headers: 11pt Bold, Segoe UI
Data labels: 10pt Regular, Segoe UI
Technical data: 9pt Monospace, Consolas/Courier New
Status text: 9pt Regular, Segoe UI
Hex/binary data: 9pt Monospace, Consolas
```

### Spacing Standards (Grid-based Layout)
```
Panel margins: 12px
Inter-element spacing: 8px
Section spacing: 16px
Tab padding: 8px x 16px
Button padding: 6px x 12px
Tree item height: 20px
List row height: 18px
```

## User Workflow Patterns & UX Flows (From Elecard Stream Analyzer)

### Primary User Workflows

#### 1. File Analysis Workflow
**Typical User Journey:**
```
File → Open → Analyze → Navigate → Export Results
```

**Detailed Flow:**
1. **File Loading**:
   - User opens file via File → Open or drag-and-drop
   - Application automatically parses stream structure
   - Explorer panel populates with hierarchical stream breakdown
   - Main panel shows initial packet listing
   - Properties panel displays file-level information

2. **Stream Exploration** (Core workflow):
   - User expands Transport Stream in Explorer tree
   - Services become visible with bitrate percentages
   - User checks/unchecks items to filter Main panel display
   - Single-click in Explorer → Properties panel updates
   - Double-click in Explorer → Opens detailed view in Main panel

3. **Detailed Analysis**:
   - User clicks packets in Main panel → Properties shows packet details
   - User can expand packet details inline (arrow keys navigation)
   - Error/warning indicators guide user to problems
   - Right-click → Context menu for actions (Dump, Export, etc.)

#### 2. Error Detection Workflow
**Problem-Solving Journey:**
```
Open File → Check Messages Panel → Navigate to Errors → Analyze Details → Generate Report
```

**Interaction Flow:**
1. User opens potentially problematic stream
2. Messages panel automatically populates with detected issues
3. User reviews error categories (Priority 1, 2, 3)
4. Double-click error → Main panel jumps to problematic packet
5. User examines packet details in Properties panel
6. User can save error report via Save → Stream Info

#### 3. Real-time Analysis Workflow (Native Integration Capabilities)
**Live ETI Streaming Analysis:**
```
Connect Source → Enable Real-time Mode → Stream Processing → Live Updates → Record/Export
```

**Qt-Integrated Flow:**
1. User selects real-time mode via UI toggle
2. ETISnoop wrapper switches to `setRealTimeMode(true)`
3. Incoming ETI data triggers `processEtiFrame()` for each frame
4. UI receives immediate `frameProcessed()` signals
5. All panels update automatically via Qt signal/slot connections
6. User can record analysis or export results without interrupting stream

#### 4. Dual-Mode Workflow (File + Real-time)
**Seamless Mode Switching:**
```
File Analysis → Switch to Real-time → Compare Live vs File → Return to File
```

**Interaction Pattern:**
1. User loads ETI file and completes analysis
2. User switches to real-time mode while keeping file data visible
3. UI shows both file and live data in split-view or tabbed interface
4. User can compare live stream against reference file
5. Analysis conclusions can be saved encompassing both datasets

### Panel Interaction Modes & Qt Integration

#### Signal-Driven Panel Updates
- **Explorer Panel**: Receives `serviceDiscovered()` signals and auto-updates tree
- **Properties Panel**: Connects to `frameProcessed()` for real-time parameter display
- **Main Panel**: Handles `ensembleUpdated()` signals for complete refresh
- **Status Panel**: Responds to `audioStatisticsUpdated()` for continuous monitoring

#### Qt Model-View Integration
- **Service Tree Model**: Automatically reflects backend data changes
- **Frame Analysis Model**: Updates in real-time without manual refreshes
- **Statistics Model**: Maintains rolling averages and historical data
- **Error Log Model**: Captures all `errorOccurred()` signals with timestamps

### Panel Interaction Modes & State Management

#### Explorer Panel Interaction States
- **Selection Mode**: Single-click selects, updates other panels
- **Filtering Mode**: Checkboxes control Main panel content visibility
- **Navigation Mode**: Tree expansion/collapse for stream hierarchy
- **Context Mode**: Right-click reveals actions per item type

#### Properties Panel Operational Modes
- **Sync Mode** (default): Auto-updates when selections change
- **Compare Mode**: Shows differences between selected items
- **Dump Mode**: Preparation for data extraction
- **Pin Mode**: Locks display to specific item (implied from manual)

#### Main Panel View States
- **Packet View**: Sequential packet listing with expandable details
- **Filtered View**: Shows only checked items from Explorer
- **Search Results View**: Highlights search matches in orange
- **Error Focus View**: Jumps to error locations from Messages panel

### User Validation & Feedback Patterns

#### Error State Communications
- **Visual Indicators**: Red circles (errors), blue circles (warnings/info)
- **Progressive Disclosure**: Errors in Messages → Details in Properties → Context in Main
- **Severity Hierarchy**: Priority 1 (red) → Priority 2 (blue) → Priority 3 (warnings)
- **Navigation Assistance**: Double-click error jumps to packet location

#### Success State Confirmations
- **File Load**: Explorer tree populates + status bar updates
- **Export Complete**: File saved confirmation + location display
- **Search Results**: Orange highlighting + result count
- **Stream Info**: Bitrate statistics + connection status indicators

### Modal Workflows & Dialog Patterns

#### Save/Export Workflow
```
Save → Select Data Type → Configure Options → Choose Location → Confirm → Complete
```
- Stream Info vs Dump selection affects available options
- Range selection (All vs Offset-based)
- Format selection (CSV separator/delimiter options)
- Progress indication for large exports

#### Search Workflow
```
Open Search → Enter Criteria → Execute → Navigate Results → Refine/Clear
```
- Text search in navigation bar
- Offset-based jumping (hex addresses)
- Previous/Next result navigation
- Search within Properties panel content

#### Options/Settings Workflow
```
File → Options → Select Tab → Modify Settings → Apply → Restart if needed
```
- Tab-based organization (Main, Visualization, Binding, Save, License)
- Immediate preview where possible
- "Restore Defaults" safety net
- Settings persistence across sessions

### Application State Management Patterns

#### Cross-Panel Communication Flow
```
Explorer Selection → Properties Update → Main Panel Filter → Messages Sync
```

**State Synchronization Rules:**
- **Explorer tree selection** drives Properties panel content
- **Main panel packet selection** updates Properties details
- **Checkbox states in Explorer** filter Main panel visibility
- **Messages panel errors** can navigate Main panel position
- **Search results** highlight relevant items across all panels

#### Data Flow Architecture (for Implementation)
```
File Input → Parser → Data Model → UI Controllers → Panel Views
```

**Critical Data Dependencies:**
1. **Stream Structure**: Explorer tree depends on parsed file structure
2. **Selection State**: Properties panel reflects current selection across all sources
3. **Filter State**: Main panel content filtered by Explorer checkboxes + search terms
4. **Error State**: Messages panel drives error indicators in Main panel scrollbar
5. **Export State**: Save operations depend on current selection + filter states

#### Mode State Transitions
```
Normal Mode ←→ Compare Mode ←→ Dump Mode
     ↓              ↓              ↓
Sync Panel     Diff Panel     Extract Panel
```

**State Persistence Requirements:**
- Window layout and panel sizes
- Explorer tree expansion states
- Recent files list and file locations
- User preferences (themes, display options)
- Last used export/save configurations

### Testing & Validation Scenarios for Implementation

#### End-to-End Qt Integration Tests
1. **File-to-Real-time Transition Flow**:
   ```
   Load ETI File → Analyze Services → Switch to Real-time → Compare Live vs File → Export Combined Analysis
   ```
   - Validate: Mode switching preserves UI state, real-time updates don't interfere with file data

2. **Signal/Slot Integration Flow**:
   ```
   Backend Signal → Qt Signal/Slot → UI Update → User Interaction → Backend Call
   ```
   - Validate: All signals properly connected, UI updates occur without lag, no signal loops

3. **Multi-threaded Processing Flow**:
   ```
   UI Thread → Background Processing → Signal Emission → UI Thread Updates
   ```
   - Validate: Background processing doesn't block UI, signals marshaled properly to UI thread

4. **Memory Management Flow**:
   ```
   Large ETI File → Progressive Processing → UI Updates → Memory Cleanup
   ```
   - Validate: Memory usage remains constant during long analysis sessions

#### Qt-Specific Component Tests
- **Model-View Synchronization**: ETISnoop data changes automatically update all connected views
- **Real-time Performance**: UI remains responsive during high-frequency `frameProcessed()` signals
- **Signal Queuing**: High-volume signals don't overwhelm Qt's event system
- **Thread Safety**: Background ETI processing doesn't corrupt UI thread data

#### Architecture Validation Benchmarks
- **Signal Latency**: Backend signals reach UI within < 1ms for real-time responsiveness
- **Memory Efficiency**: Qt object sharing reduces memory footprint vs data copying
- **Processing Throughput**: Handle > 1000 ETI frames/second in real-time mode
- **UI Responsiveness**: Interface remains interactive during intensive analysis

#### Success Criteria for Qt Integration
✅ **Real-time mode provides < 50ms latency from ETI frame to UI display**
✅ **Mode switching between file and real-time occurs without UI corruption**
✅ **Memory usage remains stable during extended real-time analysis sessions**
✅ **All UI panels automatically update via Qt signals without manual refresh**
✅ **Background processing never blocks user interactions**
✅ **Error handling propagates cleanly from backend to UI via signal system**

## Output Expectations

When implementing this GUI update:

1. **Analyze the existing codebase** to understand current UI framework
2. **Identify components** that need updating or replacement
3. **Create a implementation plan** with priorities
4. **Maintain existing functionality** while improving appearance
5. **Test thoroughly** on different screen sizes and resolutions

## Success Criteria

The updated GUI should:
- Look professional and industry-standard
- Improve user workflow efficiency
- Maintain all existing functionality
- Be intuitive for technical professionals
- Follow modern desktop application conventions
- Support both keyboard and mouse interactions effectively

## Backend Integration: ETISnoop C++/Qt Wrapper Architecture

### Important: Direct Library Integration vs Command-Line
Your application uses **ETISnoop C++/Qt wrapper** with direct API integration. This significantly improves UI design possibilities because:

**ETISnoop Wrapper provides:**
- Direct C++ function calls (no subprocess management)
- Qt signal/slot integration for real-time updates
- Unified processing interface for file and live data
- Structured data objects (not YAML parsing)
- Built-in mode switching between file and real-time analysis

**Key Architecture Benefits:**
- **Real-time Responsiveness**: Direct function calls vs subprocess overhead
- **Qt Integration**: Native signals for UI updates without polling
- **Memory Efficiency**: Shared data structures vs text parsing
- **Error Handling**: C++ exceptions vs parsing stderr output

### ETISnoop Wrapper Integration Patterns

#### Core Processing Interface
```cpp
class ETISnoopWrapper : public QObject {
    // FILE MODE: Complete file processing
    bool processEtiFile(const QString& filename);

    // BUFFER MODE: Chunk/stream data processing
    bool processEtiData(const QByteArray& data);

    // REAL-TIME MODE: Single frame processing
    FrameAnalysis processEtiFrame(const QByteArray& frameData);

    // Mode control
    void setRealTimeMode(bool enabled);
};
```

#### Signal-Based UI Updates
```cpp
signals:
    void serviceDiscovered(const ServiceInfo& service);
    void frameProcessed(const FrameAnalysis& frame);
    void ensembleUpdated(const EnsembleInfo& ensemble);
    void figAnalysisReady(const FigAnalysis& figData);
    void audioStatisticsUpdated(const AudioStats& stats);
    void errorOccurred(const QString& error);
```

### UI Architecture for Direct Integration

#### Data Flow Pattern
```
File/Network → ETISnoop Wrapper → Qt Signals → UI Slots → Panel Updates
```

**Instead of:** Command → YAML → Parser → UI Update
**You have:** Direct Call → Signal → Slot → UI Update

#### Real-Time Processing Integration
```cpp
// UI Controller pattern
class AnalysisController : public QObject {
private slots:
    void onServiceDiscovered(const ServiceInfo& service) {
        // Update Explorer panel immediately
        explorerPanel->addService(service);
        propertyPanel->showServiceDetails(service);
    }

    void onFrameProcessed(const FrameAnalysis& frame) {
        // Update main display in real-time
        mainPanel->addFrameData(frame);
        if (realTimeMode) {
            statusBar->updateFrameCount(frame.frameNumber);
        }
    }

    void onEnsembleUpdated(const EnsembleInfo& ensemble) {
        // Refresh entire tree structure
        explorerPanel->updateEnsemble(ensemble);
    }
};
```

### UI State Management for Direct Integration

#### Synchronous vs Asynchronous Operations
- **File Loading**: `processEtiFile()` can be synchronous with progress callbacks
- **Real-time Analysis**: `processEtiFrame()` asynchronous with immediate signal emission
- **Buffer Processing**: `processEtiData()` can handle streaming data chunks

#### Mode Switching Workflow
```cpp
// User switches from file to live mode
void MainWindow::switchToRealTimeMode(bool enabled) {
    etisnoopWrapper->setRealTimeMode(enabled);

    if (enabled) {
        // Configure UI for real-time updates
        mainPanel->enableRealTimeMode();
        statusBar->showLiveIndicator();
        toolbar->enableRecordingControls();
    } else {
        // Configure UI for file analysis
        mainPanel->enableFileMode();
        statusBar->showFileProgress();
        toolbar->enableFileControls();
    }
}
```

### Performance Implications for UI Design

#### Direct Memory Access Benefits
- **No Text Parsing**: Structured C++ objects instead of YAML parsing overhead
- **Shared Data**: Reference sharing instead of data copying
- **Type Safety**: Compile-time type checking vs runtime string validation
- **Memory Management**: Qt's automatic memory management vs manual cleanup

#### Real-Time Update Optimization
```cpp
// Non-blocking signal emission using Qt's event system
void ETISnoopWrapper::processEtiFrame(const QByteArray& frameData) {
    FrameAnalysis analysis = analyzeFrame(frameData);

    // Non-blocking emission for UI responsiveness
    QTimer::singleShot(0, [this, analysis]() {
        emit frameProcessed(analysis);
    });
}
```

### UI Component Design Patterns

#### Model-View Architecture
```cpp
// Data models receive signals directly from wrapper
class ServiceTreeModel : public QAbstractItemModel {
private slots:
    void onServiceDiscovered(const ServiceInfo& service) {
        beginInsertRows(parent, row, row);
        services.append(service);
        endInsertRows();
    }
};

// Views connect to models automatically
ServiceTreeView* explorerTree = new ServiceTreeView();
explorerTree->setModel(serviceTreeModel);
```

#### Progress Tracking
```cpp
// Real-time progress updates without polling
class ProgressTracker : public QObject {
private slots:
    void onFrameProcessed(const FrameAnalysis& frame) {
        currentFrame = frame.frameNumber;
        progressBar->setValue(calculateProgress());
        timeLabel->setText(formatTimestamp(frame.timestamp));
    }
};
```

### Error Handling & Validation

#### Exception-Based Error Handling
```cpp
// Synchronous error handling
try {
    bool success = etisnoopWrapper->processEtiFile(filename);
    if (!success) {
        showError("File processing failed");
    }
} catch (const EtiAnalysisException& e) {
    showError(e.what());
}

// Asynchronous error handling via signals
connect(etisnoopWrapper, &ETISnoopWrapper::errorOccurred,
        this, &MainWindow::handleAnalysisError);
```

#### Input Validation
```cpp
// Validate before processing
bool MainWindow::loadEtiFile(const QString& filename) {
    if (!isValidEtiFile(filename)) {
        QMessageBox::warning(this, "Invalid File",
                           "Selected file is not a valid ETI format");
        return false;
    }

    return etisnoopWrapper->processEtiFile(filename);
}
```

### Testing Integration Points

#### Unit Testing Wrapper Integration
- **Mock ETISnoop signals** for UI testing without actual ETI data
- **Test mode switching** between file and real-time processing
- **Validate signal/slot connections** for proper data flow
- **Memory leak testing** for continuous real-time operation

#### Integration Testing
- **File processing workflow**: Load → Process → Display → Export
- **Real-time workflow**: Connect → Stream → Display → Record
- **Mode switching**: File ↔ Real-time without UI corruption
- **Error recovery**: Handle processing failures gracefully

### Implementation Notes for Claude Code

#### Critical Design Advantages
1. **Native Qt Integration**: Use Qt's MVC patterns, not custom data handling
2. **Real-time Capability**: Design for live streaming analysis, not just file processing
3. **Type-Safe Data**: Leverage C++ structures vs text parsing
4. **Signal-Driven Updates**: Event-driven UI updates vs polling-based refreshes
5. **Memory Efficiency**: Direct object references vs data serialization

#### Architecture Decisions
- **Use Qt Models**: For automatic UI updates when data changes
- **Implement Progressive Loading**: For large ETI files with millions of frames
- **Design for Dual Mode**: File analysis and real-time streaming in same interface
- **Leverage Qt Threading**: Background processing without blocking UI
- **Use Qt's Property System**: For automatic data binding between wrapper and UI

### UI Framework Considerations
- [ ] **Docking Framework**: Implement panel docking similar to Visual Studio or Eclipse
- [ ] **Splitter Controls**: Resizable panels with memory of user preferences
- [ ] **Tree Control**: Expandable hierarchy with checkboxes and custom icons
- [ ] **Tab Control**: Support for closeable tabs with different content types
- [ ] **Data Grid**: Virtual scrolling for large datasets with custom renderers

### Specific Component Requirements
- [ ] **Explorer Tree Component**:
  - Custom tree nodes with icons, text, and percentage displays
  - Tri-state checkboxes for filtering
  - Context menu support per node type
  - Lazy loading for performance with large streams

- [ ] **Main Content Grid**:
  - Virtual list control for packet display
  - Custom drawing for color-coded backgrounds
  - Expandable rows with indented detail sections
  - Hex address display in fixed-width column
  - Scroll position markers for errors/warnings

- [ ] **Properties Panel**:
  - Key-value pair display with expandable sections
  - Toggle buttons for different view modes
  - Copy-to-clipboard functionality for technical values
  - Search/filter capability within properties

- [ ] **Bottom Tool Panels**:
  - Hex viewer with address, hex, and ASCII columns
  - Chart/graph components for time dynamics
  - Message list with severity-based color coding
  - Error analysis with expandable detail sections

### Performance Considerations
- [ ] **Large File Handling**: Stream-based processing, not loading entire files to memory
- [ ] **Real-time Updates**: Efficient UI update mechanisms for live data
- [ ] **Search Optimization**: Indexed searching for large packet streams
- [ ] **Memory Management**: Proper disposal of UI resources and data caches
- [ ] **Threading**: Background processing with UI thread marshaling

### Integration Requirements
- [ ] **File Format Support**: ETI file format support (primary focus)
  - ETI-NI (Network Independent) format processing
  - ETI-LI (Logical Interface) format handling
  - Future versions may include additional container formats
- [ ] **Export Functionality**:
  - YAML output (native ETISnoop format)
  - CSV exports for statistical analysis and spreadsheet compatibility
  - XML structured reports for integration with other DAB tools
  - Binary dump capabilities for raw ETI frame extraction
  - Audio extraction (WAV files from DAB+ subchannels)
- [ ] **Print Support**:
  - Formatted analysis reports with ensemble overview
  - Service listing summaries with technical parameters
  - FIG analysis reports for broadcast engineers
  - Audio quality statistics reports
  - Error analysis summaries with frame-level details
- [ ] **Settings Persistence**: Save window layouts, preferences, and recent files
- [ ] **Multi-monitor Support**: Proper window positioning and DPI awareness
- [ ] **Real-time Data Handling**: Support for live ETI streams via network or file monitoring

---

**Note**: This prompt emphasizes the professional, technical tool aesthetic that stream analyzer applications exemplify. Focus on information density, precise data display, and workflow efficiency rather than consumer application design patterns.
