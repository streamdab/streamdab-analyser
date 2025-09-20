Claude Code Prompt: DAB/DAB+ Stream Analyzer - ETI Analysis Tool
Project Overview
Create a professional DAB/DAB+ stream analyzer desktop application focused on ETSI 300 799 standards compliance. The application should primarily support DAB+ decoding with ETI-NI, ETI-LI, and EDI/ETI file analysis using etisnoop wrapper integration.
Primary Focus: DAB/DAB+ Analysis (ETSI 300 799)

Main Target: DAB/DAB+ stream analysis and decoding
Standards Compliance: ETSI 300 799 (DAB/DAB+ primary)
Secondary Support: TR 101-290 (as additional feature)
Core Functionality: ETI file analysis with YAML output and tree view display

Technical Architecture
Core Components Checklist:

 ETI file parser and decoder
 etisnoop wrapper integration
 YAML output generation and parsing
 FIG (Fast Information Group) analysis engine
 Audio level monitoring system
 Real-time stream processing
 Error detection and reporting
 Export functionality (YAML, reports)

File Format Support:

 ETI-NI (Ensemble Transport Interface - Network Independent)
 ETI-LI (Ensemble Transport Interface - Local Independent)
 EDI/ETI (Ensemble Data Interface/ETI)
 Real-time DAB/DAB+ streams
 Recorded ETI files

GUI Design Requirements  (check component list below but feel free to improvise or use existing design patterns but keep it professional and clean with good UX)
Qt-based Implementation Layout:
┌─────────────────────────────────────────────────────────┐
│ File  Edit  View  Tools  Help                          │
├─────────────────────────────────────────────────────────┤
│ 📁 Open │ ⏵ Start │ ⏹ Stop │ ⚙️ Settings │ 📊 Report   │
├─────────────────┬───────────────────────────────────────┤
│   File Tree     │         YAML Analysis               │
│   📁 ETI Files  │  ┌─────────────────────────────────┐  │
│   📄 file1.eti  │  │ frame:                          │  │
│   📄 file2.eti  │  │   fic:                          │  │
│   📁 Recordings │  │     fig_0_0: Service Info       │  │
│                 │  │     fig_0_1: Subchannel Org    │  │
├─────────────────┼───────────────────────────────────────┤
│   FIG Analysis  │         Audio Monitoring            │
│   📊 Services   │  ████████████████░░░░ 85% ♪ BBC R1  │
│   📻 Ensembles  │  ██████████░░░░░░░░░░ 52% ♪ BBC R2  │
│   🔧 Subchannels│  ████████████████████ 98% ♪ BBC R3  │
└─────────────────┴───────────────────────────────────────┘
│ Status: Processing file1.eti | Frame: 1234/5000 | ✅ OK │
└─────────────────────────────────────────────────────────┘

Core Features Implementation Checklist
1. ETI File Processing:

 File browser with ETI file filtering
 Drag-and-drop ETI file support
 Batch processing capability
 Progress indicators for large files
 File validation and format detection

2. etisnoop Integration:

 Wrapper class for etisnoop command-line tool
 YAML output capture and parsing
 Error handling for malformed ETI files
 Real-time processing feedback
 Command-line parameter configuration

3. FIG Analysis Display:

 Hierarchical tree view of FIG data
 Service information (FIG 0/0)
 Subchannel organization (FIG 0/1)
 Service organization (FIG 0/2)
 Service component information
 Ensemble information display
 Country and service labels

4. Audio Monitoring:

 Audio level meters per service
 Quality indicators
 Bitrate display
 Audio codec information (DAB+/HE-AAC v2)
 Service name display
 Real-time audio level updates

5. Error Detection & Reporting:

 Frame synchronization errors
 CRC error detection
 Missing service components
 Invalid FIG data alerts
 Signal quality indicators
 Error count statistics

Data Structure Requirements
YAML Structure Handling:

 Frame-level data parsing
 FIC (Fast Information Channel) processing
 FIG (Fast Information Group) extraction
 Service metadata organization
 Ensemble configuration data
 Audio component information

Tree View Organization:

 Ensemble level (top-level)
 Service level organization
 Subchannel grouping
 Component detail view
 FIG type categorization
 Error and warning nodes

Technical Implementation Guidelines
Cross-Platform Considerations:

 Windows, Linux, and macOS compatibility
 etisnoop binary distribution handling
 File path normalization
 Platform-specific audio system integration
 Resource bundling for deployment

Performance Requirements:

 Real-time processing capability
 Large ETI file handling (>100MB)
 Memory-efficient YAML parsing
 Responsive UI during processing
 Background processing threads

User Experience Features:

 Intuitive toolbar with clear icons
 Context menus for tree items
 Keyboard shortcuts for common actions
 Resizable panels and splitters
 Status bar with processing information
 Settings dialog for configuration

Integration Requirements
etisnoop Wrapper Implementation:

 Process management for etisnoop execution
 YAML output streaming and parsing
 Error output capture and display
 Command timeout handling
 Parameter passing for analysis options

Export Functionality:

 YAML file export
 Analysis report generation (PDF/HTML)
 Service list export (CSV)
 Error report export
 Screenshot/image export capability

Testing & Validation Checklist
Test Data Requirements:

 Valid ETI files for testing
 Corrupted ETI files for error testing
 Various DAB+ configurations
 Different ensemble sizes
 Multi-language service testing

Compliance Verification:

 ETSI 300 799 standard compliance
 FIG type coverage validation
 Audio codec support verification
 Metadata accuracy checking
 Performance benchmarking

Documentation Requirements
User Documentation:

 Installation guide
 User manual with screenshots
 Feature walkthrough tutorials
 Troubleshooting guide
 FAQ section

Technical Documentation:

 Architecture overview
 API documentation
 Build instructions
 Deployment procedures
 Configuration reference

StreamXpert-Inspired Features
Advanced Analysis Capabilities:

 Comprehensive FIG metadata display
 Real-time stream decoding
 In-depth DAB/DAB+ parameter analysis
 Regulatory compliance checking
 Professional-grade error reporting

Future Enhancement Roadmap:

 ETI file recording capability
 X-PAD embedded picture display
 Custom export options
 Plugin architecture for extensions
 Network stream input support

Implementation Priority
Phase 1 (Core Functionality):

ETI file loading and basic parsing
etisnoop wrapper integration
Basic YAML display
Simple tree view implementation

Phase 2 (Enhanced Features):

Advanced FIG analysis
Audio level monitoring
Error detection system
Export functionality

Phase 3 (Professional Features):

Real-time stream processing
Advanced reporting
Settings and configuration
Performance optimization

This prompt provides a comprehensive foundation for developing a professional DAB/DAB+ stream analyzer that rivals commercial tools while maintaining focus on ETSI 300 799 standards compliance and user-friendly operation.
