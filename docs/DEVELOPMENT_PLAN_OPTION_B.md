# Option B: Professional Tool Development Plan

**Document Version:** 1.0  
**Date:** October 1, 2025  
**Author:** Development Team  
**Timeline:** 6-8 weeks (40-55 days)  
**Target:** Professional DAB ETI Analysis Tool

## Executive Summary

This document provides the complete implementation plan for Option B: Professional Tool development. Option B transforms the current Qt6 GUI foundation into a professional DAB ETI analysis tool with real 6144-byte frame processing, complete FIG decoding, ETSI compliance validation, and >1000 FPS performance.

**Option B Scope**: Professional tool competing with commercial DAB analyzers  
**Timeline**: 6-8 weeks implementation  
**Future Path**: Option C (Broadcast Industry Solution) as future milestone

## Current Foundation Assessment

### ✅ Completed Foundation (Ready for Implementation)
- **Qt6 GUI Framework**: Professional three-panel broadcast interface
- **Build System**: Zero compilation errors, clean CMake configuration
- **File Operations**: ETI file dialogs working without crashes
- **Architecture**: 80+ core files framework with comprehensive structure
- **Main Application**: Single main.cpp with embedded window class (2450 lines)

### 🔄 Implementation Required
- **ETI Processing**: Currently simulation - needs real 6144-byte frame parsing
- **FIG Decoding**: UI framework only - needs complete FIG processing engine
- **Service Discovery**: Hardcoded fake services - needs real DAB enumeration
- **ETSI Compliance**: Framework placeholder - needs EN 300 799 validation
- **Performance**: UI simulation only - needs validated >1000 FPS processing

## Phase-by-Phase Implementation

### Phase 1: ETI Core Engine (Weeks 1-2)

#### Week 1: ETI Frame Parsing Foundation

**Primary Goal**: Implement real 6144-byte ETI frame parsing

**Core Implementation**:
```cpp
// File: src/core/enhanced_eti_processor_qt.cpp
class ETIFrameParser {
public:
    struct ETIFrame {
        std::array<uint8_t, 6144> raw_data;
        ETIHeader header;
        std::vector<uint8_t> fic_data;
        std::vector<uint8_t> msc_data;
        bool is_valid;
        uint32_t frame_number;
        double timestamp;
    };

    struct ETIHeader {
        uint32_t sync_pattern;      // 0xFF1F, 0x491F, or 0xC4FF
        uint8_t frame_counter;      // FCT field (0-249)
        uint8_t mode;              // DAB mode (I=1, II=2, III=3, IV=4)
        uint8_t ficf;              // FIC flag (0 or 1)
        uint8_t nst;               // Number of streams (0-63)
        uint8_t mid;               // Mode Identity
        uint8_t fp;                // Frame Phase (0-3)
        uint16_t crc;              // Header CRC
    };

    bool parseFrame(const std::array<uint8_t, 6144>& frame_data);
    ETIHeader extractHeader(const uint8_t* data);
    std::vector<uint8_t> extractFIC(const uint8_t* data);
    std::vector<uint8_t> extractMSC(const uint8_t* data);
    bool validateSyncPattern(const uint8_t* data);
    uint16_t calculateCRC(const uint8_t* data, size_t length);
    bool isFrameValid() const { return current_frame.is_valid; }
    
private:
    ETIFrame current_frame;
    std::vector<ETIFrame> parsed_frames;
};
```

**ETI Frame Structure Implementation**:
```cpp
// ETI frame layout (6144 bytes total)
// Bytes 0-3:    Sync pattern (0xFF1F491C for ETI-NI)
// Bytes 4-6:    Frame counter (24-bit)
// Byte 7:       FICF flag
// Byte 8:       NST (Number of sub-channels)
// Byte 9-10:    Mode and frame phase info
// Bytes 11-31:  Header extension and padding
// Bytes 32-127: FIC data (96 bytes when FICF=1)
// Bytes 128+:   MSC data (remainder of frame)

bool ETIFrameParser::parseFrame(const std::array<uint8_t, 6144>& frame_data) {
    current_frame.raw_data = frame_data;
    
    // Extract sync pattern
    uint32_t sync = (frame_data[0] << 24) | (frame_data[1] << 16) | 
                   (frame_data[2] << 8) | frame_data[3];
    
    if (!validateSyncPattern(frame_data.data())) {
        current_frame.is_valid = false;
        return false;
    }
    
    // Extract header
    current_frame.header = extractHeader(frame_data.data());
    
    // Extract FIC (if present)
    if (current_frame.header.ficf) {
        current_frame.fic_data = extractFIC(frame_data.data());
    }
    
    // Extract MSC
    current_frame.msc_data = extractMSC(frame_data.data());
    
    current_frame.is_valid = true;
    return true;
}
```

**Daily Milestones Week 1**:
- Day 1: ETIFrameParser class structure and sync pattern recognition
- Day 2: Header extraction and validation
- Day 3: FIC data extraction
- Day 4: MSC data extraction and CRC validation
- Day 5: Testing with real ETI files and error handling

#### Week 2: FIG Processing Engine

**Primary Goal**: Complete FIG decoding for essential FIG types

**FIG Processing Implementation**:
```cpp
// File: src/core/advanced_fig_analyzer.cpp
class FIGProcessor {
public:
    struct DABService {
        uint32_t service_id;
        std::string service_label;
        uint8_t service_type;       // 0=Audio, 1=Data
        bool is_dab_plus;
        std::vector<ServiceComponent> components;
        uint8_t protection_level;
        uint16_t bitrate;
    };

    struct EnsembleInfo {
        uint16_t ensemble_id;
        std::string ensemble_label;
        uint8_t country_id;
        uint8_t ecc;                // Extended Country Code
        std::vector<DABService> services;
    };

    void processFIC(const std::vector<uint8_t>& fic_data);
    void processFIG(uint8_t fig_type, uint8_t fig_extension, 
                   const std::vector<uint8_t>& fig_data);
    
    // Essential FIG types for Option B
    void processFIG0_0(const std::vector<uint8_t>& fig_data);  // Ensemble info
    void processFIG0_1(const std::vector<uint8_t>& fig_data);  // Sub-channel org
    void processFIG0_2(const std::vector<uint8_t>& fig_data);  // Service org
    void processFIG1_0(const std::vector<uint8_t>& fig_data);  // Ensemble label
    void processFIG1_1(const std::vector<uint8_t>& fig_data);  // Service label
    void processFIG1_4(const std::vector<uint8_t>& fig_data);  // Component label
    
    std::vector<DABService> getDiscoveredServices() const;
    EnsembleInfo getEnsembleInfo() const;
    bool isProcessingComplete() const;

private:
    EnsembleInfo ensemble_info;
    std::map<uint32_t, DABService> discovered_services;
    std::set<uint8_t> processed_fig_types;
};
```

**FIG Processing Logic**:
```cpp
void FIGProcessor::processFIC(const std::vector<uint8_t>& fic_data) {
    size_t offset = 0;
    
    while (offset < fic_data.size()) {
        if (offset + 1 >= fic_data.size()) break;
        
        uint8_t fig_header = fic_data[offset];
        uint8_t fig_length = fic_data[offset + 1];
        
        if (fig_length == 0 || offset + fig_length + 2 > fic_data.size()) {
            break; // Invalid FIG
        }
        
        uint8_t fig_type = (fig_header >> 5) & 0x07;
        uint8_t fig_extension = fig_header & 0x1F;
        
        std::vector<uint8_t> fig_data(
            fic_data.begin() + offset + 2,
            fic_data.begin() + offset + 2 + fig_length
        );
        
        processFIG(fig_type, fig_extension, fig_data);
        
        offset += fig_length + 2;
    }
}

void FIGProcessor::processFIG0_1(const std::vector<uint8_t>& fig_data) {
    // Sub-channel organization (essential for service discovery)
    size_t offset = 0;
    
    while (offset + 3 < fig_data.size()) {
        uint8_t subch_id = (fig_data[offset] >> 2) & 0x3F;
        uint16_t start_address = ((fig_data[offset] & 0x03) << 8) | fig_data[offset + 1];
        uint8_t table_switch = (fig_data[offset + 2] >> 6) & 0x01;
        uint8_t table_index = fig_data[offset + 2] & 0x3F;
        
        // Store sub-channel information for service mapping
        // This data is essential for audio decoding and service organization
        
        offset += 3;
        if (table_switch) offset += 1; // Long form
    }
}
```

**Daily Milestones Week 2**:
- Day 1: FIGProcessor class structure and FIC parsing framework
- Day 2: FIG 0/0 (Ensemble info) and FIG 0/1 (Sub-channel org) implementation
- Day 3: FIG 0/2 (Service org) implementation
- Day 4: FIG 1/0 (Ensemble label) and FIG 1/1 (Service label) implementation
- Day 5: Service discovery integration and testing

### Phase 2: GUI Integration with Real Data (Week 3)

**Primary Goal**: Connect real ETI engine to existing Qt6 interface

**GUI Integration Implementation**:
```cpp
// File: src/main.cpp (update existing DABAnalyzerWindow class)
class DABAnalyzerWindow : public QMainWindow {
    // ... existing Qt6 GUI code ...

private slots:
    void loadETIFile(const QString& filePath) {
        // Replace simulation with real ETI processing
        m_frameList->clear();
        m_ensembleTree->clear();
        m_systemMessages->clear();
        
        m_systemMessages->append(QString("📁 Loading real ETI file: %1").arg(filePath));
        
        // Initialize real processing engines
        auto eti_parser = std::make_unique<ETIFrameParser>();
        auto fig_processor = std::make_unique<FIGProcessor>();
        
        // Process real ETI file
        if (processRealETIFile(filePath, eti_parser.get(), fig_processor.get())) {
            updateServiceTreeWithRealData(fig_processor.get());
            populateFrameListWithRealFrames(eti_parser.get());
            enableRealFrameNavigation();
            m_systemMessages->append("✅ Real ETI processing complete!");
        } else {
            m_systemMessages->append("❌ Failed to process ETI file");
        }
    }
    
    void onFrameSelectionChanged() {
        int frameIndex = m_frameList->currentRow();
        if (frameIndex >= 0 && frameIndex < real_frames.size()) {
            displayRealFrameData(real_frames[frameIndex]);
            showRealHexData(real_frames[frameIndex]);
            updatePropertiesWithRealData(real_frames[frameIndex]);
        }
    }

private:
    bool processRealETIFile(const QString& filePath, 
                           ETIFrameParser* parser, 
                           FIGProcessor* fig_processor);
    void updateServiceTreeWithRealData(FIGProcessor* fig_processor);
    void populateFrameListWithRealFrames(ETIFrameParser* parser);
    void displayRealFrameData(const ETIFrame& frame);
    
    std::vector<ETIFrame> real_frames;
    std::unique_ptr<ETIFrameParser> m_eti_parser;
    std::unique_ptr<FIGProcessor> m_fig_processor;
};
```

**Real Data Integration**:
```cpp
void DABAnalyzerWindow::updateServiceTreeWithRealData(FIGProcessor* fig_processor) {
    auto ensemble_info = fig_processor->getEnsembleInfo();
    auto services = fig_processor->getDiscoveredServices();
    
    // Create ensemble root item with real data
    QTreeWidgetItem* ensembleItem = new QTreeWidgetItem();
    ensembleItem->setText(0, QString("Ensemble: %1").arg(QString::fromStdString(ensemble_info.ensemble_label)));
    ensembleItem->setText(1, QString("ID: 0x%1").arg(ensemble_info.ensemble_id, 4, 16, QChar('0')));
    ensembleItem->setText(2, "Ensemble");
    m_ensembleTree->addTopLevelItem(ensembleItem);
    
    // Add real discovered services
    for (const auto& service : services) {
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem(ensembleItem);
        serviceItem->setText(0, QString::fromStdString(service.service_label));
        serviceItem->setText(1, QString("SID: 0x%1").arg(service.service_id, 8, 16, QChar('0')));
        serviceItem->setText(2, service.is_dab_plus ? "DAB+" : "DAB");
        
        // Add service components
        for (const auto& component : service.components) {
            QTreeWidgetItem* compItem = new QTreeWidgetItem(serviceItem);
            compItem->setText(0, QString("Component %1").arg(component.component_id));
            compItem->setText(1, QString("Type: %1").arg(component.component_type));
            compItem->setText(2, "Component");
        }
    }
    
    m_ensembleTree->expandAll();
    m_systemMessages->append(QString("🌐 Discovered %1 real DAB services").arg(services.size()));
}
```

**Week 3 Milestones**:
- Day 1-2: Real ETI file loading and processing integration
- Day 3: Service tree population with real discovered services
- Day 4: Frame-by-frame navigation with real ETI data
- Day 5: Hex viewer and properties display with actual frame content

### Phase 3: ETSI Compliance & Real-time Processing (Weeks 4-5)

#### Week 4: ETSI Compliance Framework

**Primary Goal**: ETSI EN 300 799 compliance validation engine

**ETSI Compliance Implementation**:
```cpp
// File: src/core/etsi_compliance_engine.cpp
class ETSIComplianceValidator {
public:
    enum class ComplianceLevel {
        PASS,
        WARNING,
        VIOLATION,
        CRITICAL
    };

    struct ComplianceResult {
        ComplianceLevel level;
        std::string description;
        std::string etsi_reference;
        std::string suggested_fix;
        uint32_t frame_number;
        size_t byte_offset;
        QDateTime timestamp;
    };

    ComplianceResult validateFrame(const ETIFrame& frame);
    ComplianceResult validateEnsemble(const EnsembleInfo& ensemble);
    ComplianceResult validateService(const DABService& service);
    
    std::vector<ComplianceResult> getAllComplianceIssues() const;
    double getOverallComplianceScore() const;
    std::string generateComplianceReport() const;

private:
    bool validateSyncPattern(const ETIFrame& frame);
    bool validateFrameStructure(const ETIFrame& frame);
    bool validateFICStructure(const std::vector<uint8_t>& fic_data);
    bool validateServiceOrganization(const DABService& service);
    
    std::vector<ComplianceResult> compliance_issues;
    std::map<std::string, int> violation_counts;
};
```

**ETSI EN 300 799 Validation Rules**:
```cpp
ComplianceResult ETSIComplianceValidator::validateFrame(const ETIFrame& frame) {
    ComplianceResult result;
    result.frame_number = frame.frame_number;
    result.timestamp = QDateTime::currentDateTime();
    
    // EN 300 799 Section 5.2: Frame structure validation
    if (!validateSyncPattern(frame)) {
        result.level = ComplianceLevel::CRITICAL;
        result.description = "Invalid ETI sync pattern";
        result.etsi_reference = "ETSI EN 300 799 v1.2.1, Section 5.2.1";
        result.suggested_fix = "Check transmission synchronization";
        result.byte_offset = 0;
        return result;
    }
    
    // EN 300 799 Section 5.3: Header validation
    if (frame.header.nst > 63) {
        result.level = ComplianceLevel::VIOLATION;
        result.description = "NST field exceeds maximum value";
        result.etsi_reference = "ETSI EN 300 799 v1.2.1, Section 5.3.2";
        result.suggested_fix = "NST must be ≤ 63";
        result.byte_offset = 8;
        return result;
    }
    
    // Additional validation rules...
    
    result.level = ComplianceLevel::PASS;
    result.description = "Frame compliant with ETSI EN 300 799";
    return result;
}
```

#### Week 5: Real-time ETI-over-IP Processing

**Primary Goal**: Network ETI processing capability

**Network Processing Implementation**:
```cpp
// File: src/core/network_stream_receiver.cpp
class ETINetworkReceiver : public QObject {
    Q_OBJECT

public:
    struct NetworkConfig {
        std::string multicast_address = "239.192.0.1";
        uint16_t port = 9200;
        int buffer_size = 1024 * 1024;  // 1MB buffer
        double timeout_seconds = 5.0;
    };

    bool connectToETIStream(const NetworkConfig& config);
    void startRealTimeProcessing();
    void stopRealTimeProcessing();
    bool isConnected() const { return is_connected; }
    
    double getCurrentFPS() const { return current_fps; }
    uint64_t getTotalFramesReceived() const { return total_frames; }

signals:
    void frameReceived(const ETIFrame& frame);
    void networkError(const QString& error);
    void connectionStatusChanged(bool connected);
    void performanceUpdate(double fps, uint64_t total_frames);

private slots:
    void processIncomingData();
    void handleNetworkError();

private:
    QUdpSocket* udp_socket;
    QTimer* performance_timer;
    std::atomic<bool> is_connected{false};
    std::atomic<double> current_fps{0.0};
    std::atomic<uint64_t> total_frames{0};
    
    std::queue<std::array<uint8_t, 6144>> frame_buffer;
    std::mutex buffer_mutex;
};
```

**Real-time Processing Integration**:
```cpp
void DABAnalyzerWindow::enableRealTimeMode() {
    auto network_receiver = std::make_unique<ETINetworkReceiver>();
    
    // Configure real-time processing
    ETINetworkReceiver::NetworkConfig config;
    config.multicast_address = "239.192.0.1";
    config.port = 9200;
    
    connect(network_receiver.get(), &ETINetworkReceiver::frameReceived,
            this, &DABAnalyzerWindow::onRealTimeFrameReceived);
    
    connect(network_receiver.get(), &ETINetworkReceiver::performanceUpdate,
            this, &DABAnalyzerWindow::onPerformanceUpdate);
    
    if (network_receiver->connectToETIStream(config)) {
        network_receiver->startRealTimeProcessing();
        m_systemMessages->append("🌐 Real-time ETI-over-IP processing started");
    }
}

void DABAnalyzerWindow::onRealTimeFrameReceived(const ETIFrame& frame) {
    // Process frame in real-time
    if (m_fig_processor) {
        if (!frame.fic_data.empty()) {
            m_fig_processor->processFIC(frame.fic_data);
        }
    }
    
    // Update GUI with real-time data
    updateRealTimeDisplay(frame);
    
    // Validate compliance in real-time
    if (m_compliance_validator) {
        auto result = m_compliance_validator->validateFrame(frame);
        if (result.level != ETSIComplianceValidator::ComplianceLevel::PASS) {
            displayComplianceIssue(result);
        }
    }
}
```

### Phase 4: Performance Optimization & Audio Processing (Week 6)

**Primary Goal**: >1000 FPS performance and audio extraction

**Performance Optimization**:
```cpp
// File: src/core/optimized_eti_processor.cpp
class OptimizedETIProcessor {
public:
    struct PerformanceMetrics {
        double current_fps;
        double average_fps;
        uint64_t total_frames_processed;
        double cpu_usage_percent;
        size_t memory_usage_mb;
        QDateTime last_update;
    };

    void enableMultiThreadedProcessing(size_t thread_count = std::thread::hardware_concurrency());
    void optimizeMemoryUsage();
    void enableSIMDInstructions();
    
    PerformanceMetrics getCurrentMetrics() const;
    bool achieveTargetFPS(double target_fps);

private:
    class ThreadPool {
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
    public:
        void enqueue(std::function<void()> task);
        ~ThreadPool();
    };
    
    std::unique_ptr<ThreadPool> thread_pool;
    std::atomic<uint64_t> frames_processed{0};
    std::chrono::high_resolution_clock::time_point start_time;
};
```

**Audio Processing Implementation**:
```cpp
// File: src/core/audio_extraction_pipeline.cpp
class AudioProcessor {
public:
    struct AudioQualityMetrics {
        double signal_level_db;
        double noise_floor_db;
        double snr_db;
        double thd_percent;
        bool stereo_present;
        uint32_t sample_rate;
        uint16_t bitrate;
    };

    bool extractAudioFromService(const DABService& service, 
                                const std::vector<ETIFrame>& frames);
    AudioQualityMetrics analyzeAudioQuality(const AudioData& data);
    bool saveAudioToFile(const std::string& filename, AudioFormat format);
    
    void enableRealTimeAudioMonitoring();
    void setAudioOutputDevice(const std::string& device_name);

signals:
    void audioLevelUpdate(double left_level, double right_level);
    void audioQualityUpdate(const AudioQualityMetrics& metrics);

private:
    bool initializeAudioDecoder();
    bool decodeDABPlusFrame(const uint8_t* frame_data, size_t length);
    bool decodeDABFrame(const uint8_t* frame_data, size_t length);
    
    struct AudioDecoderImpl;
    std::unique_ptr<AudioDecoderImpl> decoder_impl;
};
```

### Phase 5: Professional Polish & Testing (Weeks 7-8)

**Primary Goal**: Production-ready professional tool

**Professional Features Implementation**:
- ✅ Comprehensive error handling and recovery
- ✅ Professional logging and diagnostics
- ✅ User documentation and context-sensitive help
- ✅ Professional packaging for Windows/Linux/macOS
- ✅ Automated testing with real ETI files
- ✅ Performance benchmarking and validation

## Implementation File Structure

### Core Engine Files (Phase 1-2)
```
src/core/
├── enhanced_eti_processor_qt.cpp    # Real ETI frame parsing
├── advanced_fig_analyzer.cpp        # Complete FIG decoding
├── etsi_compliance_engine.cpp       # ETSI validation
├── network_stream_receiver.cpp      # ETI-over-IP processing
├── optimized_eti_processor.cpp      # Performance optimization
├── audio_extraction_pipeline.cpp    # Audio processing
└── professional_logging_system.cpp  # Enhanced logging
```

### Updated GUI Integration (Phase 2)
```
src/main.cpp                         # Updated with real data integration
```

### Testing Framework (Phase 5)
```
tests/
├── core/
│   ├── test_eti_frame_parser.cpp   # ETI parsing tests
│   ├── test_fig_processor.cpp      # FIG decoding tests
│   └── test_etsi_compliance.cpp    # Compliance validation tests
├── integration/
│   ├── test_real_eti_files.cpp     # Real file processing tests
│   └── test_performance.cpp        # Performance validation tests
└── e2e/
    └── test_professional_workflows.cpp # End-to-end testing
```

## Success Criteria & Validation

### Technical Validation
- ✅ **Real ETI Processing**: Successfully parse actual 6144-byte ETI frames
- ✅ **Complete FIG Decoding**: All 6 essential FIG types working
- ✅ **Service Discovery**: Real DAB services discovered from FIC data
- ✅ **ETSI Compliance**: EN 300 799 validation with detailed reporting
- ✅ **Performance**: Sustained >1000 FPS processing with real files
- ✅ **Real-time Capability**: ETI-over-IP network processing functional

### Quality Metrics
- ✅ **Stability**: No crashes during extended processing sessions
- ✅ **Accuracy**: Service discovery matches reference tools
- ✅ **Performance**: Benchmarked >1000 FPS with evidence
- ✅ **Compliance**: ETSI validation reports match industry standards
- ✅ **Usability**: Professional GUI workflows intuitive and efficient

### Professional Standards
- ✅ **Build Quality**: Zero compilation errors and warnings
- ✅ **Code Quality**: Clean, maintainable, and well-documented code
- ✅ **Testing**: Comprehensive test coverage with real ETI files
- ✅ **Documentation**: Complete user and developer documentation
- ✅ **Packaging**: Professional installers for target platforms

## Risk Mitigation

### Technical Risks
- **ETI Format Complexity**: Mitigated by Phase 1 focus on core parsing
- **Performance Requirements**: Addressed in dedicated Phase 4
- **ETSI Compliance**: Systematic implementation following specifications
- **Real-time Processing**: Built on proven Qt6 networking foundation

### Timeline Risks
- **Scope Creep**: Clear Phase boundaries and deliverables
- **Integration Complexity**: Incremental integration approach
- **Testing Requirements**: Parallel testing development
- **Performance Optimization**: Dedicated phase for systematic optimization

## Future Option C Integration

Option B provides the foundation for future Option C expansion:
- ✅ **Multi-stream Processing**: Core engine ready for multiple streams
- ✅ **AI Analytics**: Performance engine suitable for AI integration
- ✅ **Cloud Platform**: Network processing foundation established
- ✅ **Hardware Integration**: Optimized engine ready for SDR integration

Option B completion enables seamless transition to Option C broadcast industry solution development when market conditions and requirements justify the additional investment.