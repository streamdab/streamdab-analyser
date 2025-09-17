# ETI Stream Analyzer - Professional Desktop Application

A professional ETI Stream Analyzer desktop application based on ETISnoop with Qt GUI framework, designed to meet broadcast industry standards. This is the ETI/DAB equivalent of Wireshark - providing both live capture and offline analysis capabilities for Digital Audio Broadcasting streams.

## 🎯 Project Overview

**Goal:** Bridge the gap between basic ETISnoop command-line tools and expensive commercial solutions by creating a professional desktop application for ETI stream analysis.

**Target Users:** Broadcast engineers, DAB+ network operators, and broadcasting professionals who need real-time ETI stream monitoring and analysis.

## ✨ Key Features

### 🔴 Live/Real-time Mode
- **Live Stream Capture** - Capture ETI streams from network interfaces or hardware
- **Real-time Analysis** - Process and display analysis while capturing
- **Live Filtering** - Apply filters during capture to focus on specific data
- **Buffer Management** - Configurable ring buffers for continuous operation
- **Trigger Conditions** - Start/stop capture based on specific events

### 📁 Offline Analysis Mode
- **File Import** - Load pre-recorded ETI files for analysis
- **Batch Processing** - Analyze multiple files in sequence
- **Historical Analysis** - Deep-dive analysis without real-time constraints
- **Comparison Mode** - Compare multiple ETI files side-by-side
- **Timeline Navigation** - Scrub through captured data with timeline controls

### 📊 Professional Features
- **ETSI Standards Compliance** - Full ETI (ETSI 300 799) and DAB standards support
- **Error Detection** - Real-time validation and alerts
- **Report Generation** - Professional PDF/HTML reports with statistics
- **Multi-stream Support** - Handle multiple ETI sources simultaneously
- **Stream Recording** - Record live ETI streams with time-based segmentation
- **Format Conversion** - Export to multiple industry-standard formats

## 🏗️ Technology Stack

- **Core Engine:** C++ (ETISnoop integration)
- **GUI Framework:** Qt 6.x (Cross-platform native)
- **Build System:** CMake
- **Dependencies:** libfaad-dev, libfftw3-dev, yaml-cpp
- **Platforms:** Windows, Linux, macOS

## 🚀 Performance Targets

- **Startup Time:** < 3 seconds
- **Memory Usage:** < 150MB baseline
- **Processing Speed:** > 900 ETI frames/second
- **UI Responsiveness:** 60 FPS smooth updates
- **Real-time Latency:** < 100ms processing delay

## 👥 Development Team Structure

This project utilizes 6 specialized AI agents for comprehensive development coverage:

1. **PM Agent** - Project management, timeline coordination, ETSI compliance oversight
2. **UI/UX Agent** - Qt 6.x GUI development, professional desktop interface design
3. **Standards Compliance Agent** - ETSI standards implementation and broadcasting compliance
4. **Tester Agent** - Qt/C++ testing, performance validation, cross-platform compatibility
5. **Build Manager Agent** - CMake build system, cross-platform compilation, professional packaging
6. **Network/Stream Agent** - Real-time ETI capture, buffer management, stream recording

*See [.agents-config.md](./.agents-config.md) for detailed agent specifications*

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
└── README.md                   # This file
```

## 🚀 Development Timeline & Current Status

**Total Duration:** 11 weeks (2.5 months)  
**Project Start:** September 17, 2025  
**Target Completion:** December 3, 2025  
**Current Status:** 🔥 **Phase 1 in Progress** (Week 1 of 11)

### Phase 1: Foundation (Week 1-2) - 🟡 IN PROGRESS
**Status:** Started September 17, 2025 | **Progress:** 5% complete

**Week 1 Current Focus (September 17-24):**
- [x] **Project Coordination:** Documentation setup and agent assignments
- [ ] **Build System:** CMake configuration with Qt 6.x integration
- [ ] **Core Integration:** ETISnoop source analysis and wrapper design
- [ ] **GUI Foundation:** Qt application skeleton development

**Week 2 Planned (September 25 - October 1):**
- [ ] **File Operations:** ETI file loading and saving functionality
- [ ] **Display Components:** Basic YAML output visualization
- [ ] **Compliance Framework:** Initial ETSI validation infrastructure
- [ ] **Testing Setup:** Unit testing framework and CI/CD foundation

### Phase 2: Core Features (Week 3-5) - 🔵 PLANNED
**Target:** October 2-22, 2025
- Real-time ETI stream processing implementation
- Professional GUI layout with splitters and docks
- FIG analysis widget with tree view display
- Audio level monitoring with visual indicators

### Phase 3: Professional Features (Week 6-8) - 🔵 PLANNED
**Target:** October 23 - November 12, 2025
- Alert system with configurable thresholds
- Logging framework with structured output
- Report generation (PDF/HTML export)
- Multi-stream support and tabbed interface
- Stream recording with time-based segmentation

### Phase 4: Polish & Testing (Week 9-11) - 🔵 PLANNED
**Target:** November 13 - December 3, 2025
- UI/UX improvements and professional styling
- Comprehensive testing suite (unit/integration)
- Performance optimization and memory management
- Cross-platform packaging and deployment

### 📊 Live Project Tracking
For real-time development progress, see **[PROJECT_STATUS.md](./PROJECT_STATUS.md)**

## 🔍 Comparison with Similar Tools

### vs Wireshark (Network Protocol Analyzer)
- **Domain:** ETI/DAB broadcast streams vs Network packets
- **Expertise:** Specialized for broadcast industry vs General networking
- **Workflow:** Optimized for broadcast engineers vs Network administrators

### vs Professional Broadcast Tools
- **Cost:** Open source vs $5,000-50,000 commercial solutions
- **Flexibility:** Customizable vs Fixed feature sets
- **Hardware:** Software-only vs Expensive hardware dependency

## 🛠️ Build Requirements

### Linux
- Qt 6.x development packages
- CMake 3.16+
- GCC/Clang compiler
- libfaad-dev, libfftw3-dev

### Windows
- Visual Studio 2019+
- Qt 6.x with MSVC
- CMake 3.16+
- vcpkg for dependencies

### macOS
- Xcode Command Line Tools
- Qt 6.x framework
- CMake 3.16+
- Homebrew for dependencies

## 📦 Distribution

- **Linux:** AppImage for universal distribution
- **Windows:** NSIS installer with dependencies
- **macOS:** DMG bundle with notarization

## 📄 Documentation

- **[CLAUDE.md](./CLAUDE.md)** - Complete development guide and project specifications
- **[PROJECT_STATUS.md](./PROJECT_STATUS.md)** - Real-time project tracking and coordination status
- **[.agents-config.md](./.agents-config.md)** - AI agent configuration and workflows

### 📊 Project Tracking & Coordination
- **Live Status:** [PROJECT_STATUS.md](./PROJECT_STATUS.md) - Updated daily with progress, blockers, and next actions
- **Agent Matrix:** Current assignments, dependencies, and coordination workflows
- **Risk Management:** Active issue tracking and mitigation strategies
- **Performance Metrics:** Development velocity, quality gates, and success indicators

## 🎯 Success Metrics

### MVP Completion Criteria
- Successfully processes ETI files with YAML output
- Professional Qt GUI with all core widgets
- Real-time stream processing capability
- Error detection and basic alerting
- Report generation functionality
- Stream recording with time-based segmentation
- Cross-platform compatibility

### Professional Tool Validation
- **Performance:** Matches or exceeds ETISnoop speed
- **Reliability:** 24/7 operation capability
- **Usability:** Professional broadcast engineer approval
- **Standards:** ETSI compliance verification

## 📈 Project Status

**Current Phase:** Phase 1 Implementation (Foundation) - Week 1 of 11  
**Started:** September 17, 2025  
**Next Milestone:** October 1, 2025 (Phase 1 completion)  
**Overall Progress:** 5% (Documentation and coordination established)  
**Risk Level:** Low (proven technology stack)  
**Commercial Viability:** High (fills market gap)  
**Technical Feasibility:** Excellent (Qt + ETISnoop proven)

### 🎯 Current Week Objectives (September 17-24, 2025)
- **Build Manager Agent:** CMake project structure with Qt 6.x integration
- **Network/Stream Agent:** ETISnoop source integration and wrapper architecture
- **UI/UX Agent:** Qt application skeleton and main window framework
- **Standards Compliance Agent:** ETSI compliance framework planning
- **PM Agent:** Daily coordination and milestone tracking

### 📋 Active Development Areas
1. **Technology Integration:** CMake + Qt 6.x + ETISnoop compatibility
2. **Cross-Platform Setup:** Windows, Linux, macOS build foundation
3. **Professional Architecture:** Broadcast industry standard implementation
4. **Agent Coordination:** 6-agent specialized development workflow

## 🤝 Contributing

This is a professional development project using AI agents. Contributions are welcome following the established agent workflows and ETSI compliance requirements.

## 📜 License

[To be determined - likely open source for broadcast industry benefit]

## 🏢 Commercial Context

**Target Market:** Professional broadcast industry, DAB+ network operators  
**Competitive Advantage:** Open source alternative to expensive commercial tools  
**Industry Impact:** Democratizes access to professional ETI analysis capabilities