# CLAUDE.md - ETI Stream Analyzer Professional Desktop Application

## 🎯 Project Overview

**Goal:** Develop a professional ETI Stream Analyzer desktop application based on ETISnoop with Qt GUI framework to meet broadcast industry standards.

**Target:** Create MVP professional tool that bridges gap between basic ETISnoop command-line and expensive commercial solutions.

**Concept:** ETI/DAB equivalent of Wireshark - providing both live capture and offline analysis capabilities for Digital Audio Broadcasting streams.

## 🔄 Operation Modes

### **Live/Real-time Mode**
- [ ] **Live Stream Capture** - Capture ETI streams from network interfaces or hardware
- [ ] **Real-time Analysis** - Process and display analysis while capturing
- [ ] **Live Filtering** - Apply filters during capture to focus on specific data
- [ ] **Buffer Management** - Configurable ring buffers for continuous operation
- [ ] **Trigger Conditions** - Start/stop capture based on specific events

### **Offline Analysis Mode** 
- [ ] **File Import** - Load pre-recorded ETI files for analysis
- [ ] **Batch Processing** - Analyze multiple files in sequence
- [ ] **Historical Analysis** - Deep-dive analysis without real-time constraints
- [ ] **Comparison Mode** - Compare multiple ETI files side-by-side
- [ ] **Timeline Navigation** - Scrub through captured data with timeline controls

## 📋 Professional Requirements Checklist

### ✅ **Core Professional Standards**
- [ ] **Real-time Processing** - Live ETI stream analysis capability
- [ ] **GUI Interface** - Professional desktop application with native widgets
- [ ] **Error Detection** - ETSI standard compliance validation and alerts
- [ ] **Standards Compliance** - ETI (ETSI 300 799) and DAB standards support
- [ ] **Logging & Recording** - Structured logging with historical data storage
- [ ] **Alert System** - Configurable notifications for threshold breaches
- [ ] **Report Generation** - Professional PDF/HTML reports with statistics
- [ ] **Multi-stream Support** - Handle multiple ETI sources simultaneously
- [ ] **Stream Recording** - Record live ETI streams with time-based segmentation
- [ ] **Format Conversion** - Export to multiple industry-standard formats

### ⚠️ **Secondary Professional Features** 
- [ ] **Remote Monitoring** - Network-based monitoring capabilities
- [ ] **Hardware Integration** - Support for professional ETI interfaces
- [ ] **API Interface** - Programmatic access for automation
- [ ] **Database Integration** - Long-term data storage and analytics
- [ ] **User Management** - Role-based access control
- [ ] **Backup & Recovery** - Data protection mechanisms

## 👥 Development Team Structure

### **Specialized Agent Configuration**
The project utilizes 6 specialized AI agents for comprehensive development coverage:

1. **PM Agent** - Project management, timeline coordination, ETSI compliance oversight
2. **UI/UX Agent** - Qt 6.x GUI development, professional desktop interface design
3. **Standards Compliance Agent** - ETSI standards implementation and broadcasting compliance
4. **Tester Agent** - Qt/C++ testing, performance validation, cross-platform compatibility
5. **Build Manager Agent** - CMake build system, cross-platform compilation, professional packaging
6. **Network/Stream Agent** - Real-time ETI capture, buffer management, stream recording

### **Agent Coordination Workflows**
- **Development Phase**: PM → Standards → UI/UX → Network/Stream → Build → Tester
- **ETSI Compliance**: Standards → Network/Stream → Tester → UI/UX → PM
- **Real-time Streaming**: Network/Stream → Standards → UI/UX → Build → Tester

*Reference: [.agents-config.md](./.agents-config.md) for detailed agent specifications*

## 🏗️ Technical Architecture

### **Technology Stack**
- [ ] **Core Engine:** C++ (ETISnoop integration)
- [ ] **GUI Framework:** Qt 6.x (Cross-platform native)
- [ ] **Build System:** CMake
- [ ] **Dependencies:** libfaad-dev, libfftw3-dev, yaml-cpp
- [ ] **Platform:** Windows, Linux, macOS

### **Performance Targets**
- [ ] **Startup Time:** < 3 seconds
- [ ] **Memory Usage:** < 150MB
- [ ] **Processing Speed:** > 900 ETI frames/second
- [ ] **UI Responsiveness:** 60 FPS smooth updates
- [ ] **Real-time Latency:** < 100ms processing delay

## 📁 Project Structure

```
eti-analyzer-desktop/
├── src/
│   ├── core/                    # ETI processing engine
│   ├── gui/                     # Qt GUI components  
│   ├── utils/                   # Utilities and helpers
│   └── main.cpp                 # Application entry point
├── resources/                   # Icons, UI files, assets
├── third_party/                 # External dependencies
│   ├── etisnoop/               # ETISnoop source integration
│   └── yaml-cpp/               # YAML processing library
├── tests/                      # Unit and integration tests
├── docs/                       # Documentation
├── CMakeLists.txt              # Build configuration
└── README.md                   # Project documentation
```

## 🚀 Development Phases

### **Phase 1: Foundation (Week 1-2) - TDD Setup**
- [ ] Project structure setup and CMake configuration
- [ ] **Test framework integration (gtest, Qt Test)**
- [ ] **Basic test infrastructure and CI setup**
- [ ] ETISnoop source integration and wrapper creation
- [ ] **Write tests for ETI parsing before implementation**
- [ ] Basic Qt application skeleton with main window
- [ ] **GUI component tests setup**
- [ ] File open/save functionality for ETI files
- [ ] Basic YAML output display widget

### **Phase 2: Core Features (Week 3-5) - TDD Implementation**
- [ ] **Write tests for real-time processing requirements**
- [ ] Real-time ETI stream processing implementation
- [ ] **Test-driven GUI layout development**
- [ ] Professional GUI layout with splitters and docks
- [ ] **FIG analysis tests before widget implementation**
- [ ] FIG analysis widget with tree view display
- [ ] **Audio monitoring tests and mock data**
- [ ] Audio level monitoring with visual indicators
- [ ] **Error detection test suite**
- [ ] Error detection and validation system

### **Phase 3: Professional Features (Week 6-8) - Advanced TDD**
- [ ] **Alert system test scenarios**
- [ ] Alert system with configurable thresholds
- [ ] **Logging framework tests**
- [ ] Logging framework with structured output
- [ ] **Report generation test cases**
- [ ] Report generation (PDF/HTML export)
- [ ] **Configuration management tests**
- [ ] Configuration management and settings
- [ ] **Multi-stream support integration tests**
- [ ] Multi-stream support and tabbed interface
- [ ] **Stream recording test suite**
- [ ] Stream recording with time-based segmentation
- [ ] **Audio extraction tests**
- [ ] Audio extraction and format conversion
- [ ] **Metadata export validation tests**
- [ ] Metadata export and playlist generation

### **Phase 4: Polish & Testing (Week 9-11) - TDD Validation**
- [ ] **UI/UX automated testing**
- [ ] UI/UX improvements and professional styling
- [ ] **Comprehensive test suite execution**
- [ ] Comprehensive testing suite (unit/integration)
- [ ] **Performance test validation**
- [ ] Performance optimization and memory management
- [ ] **Documentation with test examples**
- [ ] Documentation and user manual creation
- [ ] **Cross-platform test execution**
- [ ] Cross-platform packaging and deployment

## 🎨 UI/UX Requirements

### **Main Window Layout**
- [ ] **Menu Bar** - Standard File/Edit/View/Tools/Help menus
- [ ] **Tool Bar** - Quick access buttons for common operations
- [ ] **Status Bar** - Processing status and frame counters
- [ ] **File Explorer** - ETI file browser and management
- [ ] **YAML Viewer** - Syntax-highlighted analysis output
- [ ] **FIG Analysis** - Hierarchical service and ensemble information
- [ ] **Audio Monitor** - Real-time audio level displays
- [ ] **Error Log** - Professional error tracking and alerts
- [ ] **Recording Panel** - Stream recording controls with time segmentation
- [ ] **Export Manager** - Format conversion and file management

### **Professional UI Standards**
- [ ] Native desktop appearance and behavior
- [ ] Consistent keyboard shortcuts and accessibility
- [ ] Responsive layout with resizable panels
- [ ] Professional color scheme and iconography
- [ ] Context menus and drag-drop functionality
- [ ] Tooltip help and status information

## 📊 Data Management

### **Input Sources**
- [ ] ETI files (local filesystem)
- [ ] ETI-NI live streams (network)
- [ ] ETI-LI raw streams
- [ ] EDI (Encapsulated DAB Interface) streams

### **Output Formats**
- [ ] YAML structured analysis
- [ ] JSON data export
- [ ] CSV statistics export
- [ ] PDF professional reports
- [ ] HTML web-viewable reports

### **Stream Recording & Export**
- [ ] **ETI Raw Recording** - Native ETI-NI/ETI-LI format preservation
- [ ] **Time-based Segmentation** - Configurable recording intervals (minutes/hours)
- [ ] **Audio Extraction** - Individual service audio streams (WAV/FLAC)
- [ ] **Metadata Export** - Service information and EPG data (XML/JSON)
- [ ] **Transport Stream** - Convert to MPEG-TS for compatibility
- [ ] **Playlist Generation** - M3U8/PLS for streaming applications

### **Recording Formats & Standards**
- [ ] **ETI-NI** - Native format (6144 bytes/frame, 24ms audio)
- [ ] **ETI-LI** - Linear format for direct processing
- [ ] **EDI** - Encapsulated format for IP distribution
- [ ] **Raw Binary** - Unprocessed stream data
- [ ] **Compressed Archives** - ZIP/TAR with metadata for long-term storage

### **Data Storage**
- [ ] Configuration settings (Qt Settings)
- [ ] Processing logs (structured format)
- [ ] Error history and statistics
- [ ] User preferences and layouts

## 🧪 Test-Driven Development (TDD) Methodology

### **TDD Implementation Strategy**
- [ ] **Red-Green-Refactor Cycle** - Write failing tests, implement code, refactor
- [ ] **Test-First Approach** - All new features start with tests
- [ ] **Continuous Testing** - Automated test execution during development
- [ ] **Test Coverage Goals** - Minimum 80% code coverage requirement
- [ ] **Integration Testing** - End-to-end workflow validation

### **Testing Framework Stack**
- [ ] **Unit Testing:** Google Test (gtest) for C++ core components
- [ ] **Qt Testing:** Qt Test framework for GUI components
- [ ] **Mocking:** Google Mock (gmock) for dependency isolation
- [ ] **Coverage:** gcov/lcov for coverage reporting
- [ ] **CI Integration:** GitHub Actions for automated testing

### **TDD Test Categories**

#### **Core ETI Processing Tests**
- [ ] ETI frame parsing validation tests
- [ ] FIG decoding accuracy tests  
- [ ] Audio extraction functionality tests
- [ ] Error detection algorithm tests
- [ ] Performance benchmark tests

#### **GUI Component Tests**
- [ ] Widget rendering and layout tests
- [ ] User interaction simulation tests
- [ ] Data binding and model tests
- [ ] Real-time update mechanism tests
- [ ] Memory leak detection tests

#### **Integration Tests**
- [ ] End-to-end ETI processing pipeline tests
- [ ] File I/O operations tests
- [ ] Network stream handling tests
- [ ] Multi-threading safety tests
- [ ] Cross-platform compatibility tests

## 🔧 TDD Development Workflow

### **Pre-Development Setup**
```bash
# Test framework setup commands
git submodule add https://github.com/google/googletest.git third_party/googletest
cmake -DBUILD_TESTING=ON -B build -S .
```

### **TDD Cycle for Each Feature**
1. **🔴 Red:** Write failing test for new feature
2. **🟢 Green:** Write minimal code to pass test  
3. **🔵 Refactor:** Improve code while keeping tests green
4. **🔄 Repeat:** Continue cycle for next feature

### **Test Structure Requirements**
- [ ] **Test Organization:** Separate test files for each component
- [ ] **Test Naming:** Descriptive test names following Given_When_Then pattern
- [ ] **Test Data:** Sample ETI files and mock data for testing
- [ ] **Test Isolation:** Each test runs independently
- [ ] **Test Documentation:** Clear test descriptions and expectations

## 🔍 Quality Assurance (TDD-Driven)

### **Testing Strategy**
- [ ] **Unit Tests** - Core ETI processing functions (TDD approach)
- [ ] **Integration Tests** - GUI and processing pipeline (BDD scenarios)
- [ ] **Performance Tests** - Memory and CPU usage validation
- [ ] **Compatibility Tests** - Cross-platform verification
- [ ] **User Acceptance Tests** - Professional workflow validation

### **Quality Metrics**
- [ ] Code coverage > 80% (enforced by CI/CD)
- [ ] Memory leak detection (Valgrind/sanitizers)
- [ ] Performance benchmarking against ETISnoop
- [ ] UI responsiveness testing (automated)
- [ ] Cross-platform compatibility verification

### **TDD Quality Gates**
- [ ] **Pre-commit:** All tests must pass before code commit
- [ ] **Pull Request:** Code review + test coverage validation
- [ ] **CI Pipeline:** Automated testing on multiple platforms
- [ ] **Release:** Full test suite + performance validation

## 📦 Deployment & Distribution

### **Build Requirements**
- [ ] **Linux:** Qt6, CMake, GCC/Clang, ETI dependencies
- [ ] **Windows:** Visual Studio, Qt6, CMake, vcpkg
- [ ] **macOS:** Xcode, Qt6, CMake, Homebrew dependencies

### **Distribution Packages**
- [ ] **Linux:** AppImage for universal distribution
- [ ] **Windows:** NSIS installer with dependencies
- [ ] **macOS:** DMG bundle with notarization
- [ ] **Source:** GitHub releases with build instructions

## ⚡ Performance Optimization

### **Critical Path Optimization**
- [ ] Multi-threaded ETI processing pipeline
- [ ] Efficient memory management (ring buffers)
- [ ] Qt model/view optimization for large datasets
- [ ] Lazy loading for UI components
- [ ] Background processing with progress indicators

### **Resource Management**
- [ ] Configurable buffer sizes for different hardware
- [ ] Memory pool allocation for frequent operations
- [ ] CPU affinity settings for real-time processing
- [ ] GPU acceleration consideration for visualizations

## 🛡️ Security & Reliability

### **Error Handling**
- [ ] Graceful degradation for malformed ETI data
- [ ] Exception safety throughout application
- [ ] Crash reporting and recovery mechanisms
- [ ] Input validation and sanitization

### **Data Protection**
- [ ] Secure temporary file handling
- [ ] Configuration encryption for sensitive settings
- [ ] Safe handling of network streams
- [ ] Privacy-compliant logging practices

## 📈 Success Metrics

### **MVP Completion Criteria**
- [ ] Successfully processes ETI files with YAML output
- [ ] Professional Qt GUI with all core widgets
- [ ] Real-time stream processing capability
- [ ] Error detection and basic alerting
- [ ] Report generation functionality
- [ ] **Stream recording with time-based segmentation**
- [ ] **Audio extraction and format conversion**
- [ ] Cross-platform compatibility

### **Professional Tool Validation**
- [ ] **Performance:** Matches or exceeds ETISnoop speed
- [ ] **Reliability:** 24/7 operation capability
- [ ] **Usability:** Professional broadcast engineer approval
- [ ] **Standards:** ETSI compliance verification
- [ ] **Maintainability:** Clean, documented codebase

## 🎯 Timeline & Milestones

**Total Duration:** 11 weeks (2.5 months)
**Team Size:** 1-2 developers
**Estimated Effort:** 400-500 developer hours

### **Weekly Milestones**
- [ ] **Week 2:** Basic Qt application with ETI file loading
- [ ] **Week 4:** Real-time processing and core widgets
- [ ] **Week 6:** Professional features and alert system
- [ ] **Week 8:** Report generation and multi-stream support
- [ ] **Week 10:** Testing, optimization, and documentation
- [ ] **Week 11:** Final packaging and deployment ready

## 💰 Budget Considerations

### **Development Costs**
- [ ] **Development Time:** 11 weeks × developer rate
- [ ] **Qt License:** Open source (free) or commercial if needed
- [ ] **Testing Infrastructure:** CI/CD pipeline setup
- [ ] **Documentation Tools:** Professional documentation system

### **Operational Costs**
- [ ] **Maintenance:** Ongoing bug fixes and updates
- [ ] **Support:** User documentation and help system
- [ ] **Distribution:** App store fees or hosting costs
- [ ] **Certification:** Professional tool certification if required

---

**Project Status:** Ready for implementation  
**Risk Level:** Low (proven technology stack)  
**Commercial Viability:** High (fills market gap)  
**Technical Feasibility:** Excellent (Qt + ETISnoop proven)

## 🔍 Comparison with Similar Tools

### **vs Wireshark (Network Protocol Analyzer)**

| **Aspect** | **Wireshark** | **ETI Stream Analyzer** | **Key Differences** |
|------------|---------------|------------------------|-------------------|
| **Target Domain** | Network packets (TCP/IP/etc) | ETI/DAB broadcast streams | Specialized for broadcast |
| **Protocol Knowledge** | 3000+ network protocols | ETI, FIG, DAB+ specific | Domain expertise |
| **Real-time Capture** | ✅ Network interfaces | ✅ ETI stream sources | Different input sources |
| **Offline Analysis** | ✅ PCAP files | ✅ ETI files | Similar workflow |
| **Filtering** | BPF/Display filters | ETI-specific filters | Protocol-specific |
| **Dissection** | Packet dissection | ETI frame dissection | Similar concept |
| **Export** | Multiple formats | Broadcast formats | Industry-specific |

### **vs Professional Broadcast Tools**

| **Tool Type** | **Example** | **Price Range** | **Our Advantage** |
|---------------|-------------|----------------|------------------|
| **ETI Analyzers** | DAB-XPlorer | $5,000-15,000 | Open source, customizable |
| **Stream Analyzers** | Elecard | $2,000-8,000 | Free, specialized for ETI |
| **Broadcast Monitors** | R&S | $20,000-50,000 | Fraction of cost |
| **Software Solutions** | StreamXpert | $1,000-5,000 | No hardware dependency |

### **Why Not Just Use Wireshark?**

**❌ Wireshark Limitations for ETI:**
- No native ETI protocol support
- Cannot decode DAB+ audio streams  
- No FIG (Fast Information Group) analysis
- Missing broadcast-specific features
- Generic UI not optimized for broadcast workflows

**✅ ETI Analyzer Advantages:**
- Native ETI frame understanding
- DAB+ service decoding
- FIG analysis and visualization
- Audio level monitoring
- Broadcast engineer workflow
- ETI-specific error detection