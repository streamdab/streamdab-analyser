#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include "eti_types.h"
#include "modern_eti_frame_parser.h"
#include "enhanced_fig_analyzer.h"
#include "performance_profiler.h"

/**
 * @class EtiProcessor
 * @brief Core ETI stream processing engine
 * 
 * Handles ETI frame parsing, FIG analysis, and audio extraction
 * following ETSI standards for broadcast industry compliance.
 */
class EtiProcessor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit EtiProcessor(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~EtiProcessor();

    /**
     * @brief Initialize the ETI processor
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Process ETI data from file using ETISnoop backend
     * @param filename Path to ETI file
     * @return true if processing successful
     */
    bool processFile(const QString& filename);

    /**
     * @brief Process ETI data from buffer using ETISnoop backend
     * @param data ETI data buffer
     * @return true if processing successful
     */
    bool processData(const QByteArray& data);

    /**
     * @brief Get Modern ETI Frame Parser instance for direct access
     * @return Modern ETI parser instance
     */
    eti::modern::ModernETIFrameParser* getETIParser() const { return m_frameParser.get(); }

    /**
     * @brief Get Enhanced FIG Analyzer instance for direct access
     * @return Enhanced FIG analyzer instance
     */
    eti::modern::EnhancedFIGAnalyzer* getFIGAnalyzer() const { return m_figAnalyzer.get(); }

    /**
     * @brief Check if processor is initialized
     * @return true if ready for processing
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Get current frame count
     * @return Number of processed frames
     */
    quint64 getFrameCount() const { return m_frameCount; }

    /**
     * @brief Get processing status
     * @return Current status string
     */
    QString getStatus() const { return m_status; }

    /**
     * @brief Process a single ETI frame
     * @param frameData Raw ETI frame data (6144 bytes)
     * @return true if frame was processed successfully
     */
    bool processEtiFrame(const QByteArray& frameData);

    /**
     * @brief Get current ETSI compliance status
     * @return Current compliance status structure
     */
    eti::ETSIComplianceResult getComplianceStatus() const;

    /**
     * @brief Get discovered services
     * @return Vector of discovered DAB services
     */
    std::vector<eti::DabService> getDiscoveredServices() const { return m_discoveredServices; }

    /**
     * @brief Get current ensemble information
     * @return Current ensemble structure
     */
    eti::Ensemble getCurrentEnsemble() const { return m_currentEnsemble; }

    /**
     * @brief Get subchannel organization
     * @return Vector of subchannel information
     */
    std::vector<eti::SubChannelInfo> getSubchannels() const { return m_subchannels; }

    /**
     * @brief Get processing performance statistics
     * @return Current processing rate in FPS
     */
    double getProcessingRate() const { return m_averageFrameRate; }

    /**
     * @brief Reset processor state for new analysis
     */
    void reset();

private:
    // ETI frame validation methods
    bool validateSyncPattern(const eti::EtiFrame& frame);
    bool validateLidataField(const eti::EtiLidataField& lidata);
    bool validateFrameCrc(const eti::EtiFrame& frame);

    // FIC and MSC processing methods
    bool processFicData(const eti::EtiFicField& fic);
    bool processMscData(const eti::EtiMscField& msc, uint8_t subChannelCount);

    // Enhanced FIG processing methods
    void processFigType0(const eti::FigBlock& figBlock);
    void processFigType1(const eti::FigBlock& figBlock);
    void processFigType2(const eti::FigBlock& figBlock);
    
    // Specific FIG parsers for Phase 1 completion
    bool parseFig00_EnsembleInfo(const eti::FigBlock& figBlock);
    bool parseFig01_SubchannelOrganization(const eti::FigBlock& figBlock);
    bool parseFig02_ServiceOrganization(const eti::FigBlock& figBlock);
    bool parseFig03_ServiceComponent(const eti::FigBlock& figBlock);
    bool parseFig10_EnsembleLabel(const eti::FigBlock& figBlock);
    bool parseFig11_ServiceLabel(const eti::FigBlock& figBlock);
    
    // Enhanced service discovery methods
    void updateEnsembleInformation(const eti::Ensemble& ensemble);
    void updateServiceInformation(const eti::DabService& service);
    void updateSubchannelInformation(const eti::SubChannelInfo& subchannel);
    
    // ETSI compliance validation methods
    bool validateFrameCompliance(const eti::EtiFrame& frame);
    bool validateEnsembleCompliance(const eti::Ensemble& ensemble);
    bool validateServiceCompliance(const eti::DabService& service);
    
    // Performance optimization methods
    void optimizeProcessingPipeline();
    void updateProcessingStatistics();
    
    // Enhanced service management (public methods moved to public section above)
    std::vector<eti::DabService> getCurrentServices() const { return m_discoveredServices; }
    std::vector<eti::SubChannelInfo> getCurrentSubchannels() const { return m_subchannels; }
    
public:
    // Service quality and status indicators
    struct ServiceQuality {
        double signalStrength;
        double errorRate;
        bool isValid;
        std::chrono::system_clock::time_point lastUpdate;
        
        ServiceQuality() : signalStrength(0.0), errorRate(0.0), isValid(false) {}
    };

private:
    
    ServiceQuality getServiceQuality(uint16_t serviceId) const;
    bool isServiceActive(uint16_t serviceId) const;

signals:
    /**
     * @brief Emitted when frame is processed
     * @param frameNumber Frame number
     * @param data Frame data
     */
    void frameProcessed(quint64 frameNumber, const QByteArray& data);

    /**
     * @brief Emitted when error occurs
     * @param error Error message
     */
    void errorOccurred(const QString& error);

    /**
     * @brief Emitted when processing status changes
     * @param status New status
     */
    void statusChanged(const QString& status);
    
    /**
     * @brief Emitted when ensemble information is discovered
     * @param ensemble Ensemble information
     */
    void ensembleDiscovered(const eti::Ensemble& ensemble);
    
    /**
     * @brief Emitted when service is discovered
     * @param service Service information
     */
    void serviceDiscovered(const eti::DabService& service);
    
    /**
     * @brief Emitted when service quality changes
     * @param serviceId Service ID
     * @param signalStrength Signal strength (0.0-1.0)
     * @param errorRate Error rate (0.0-1.0)
     */
    void serviceQualityChanged(uint16_t serviceId, double signalStrength, double errorRate);

private slots:
    // Modern ETI Core Engine signal handlers
    void onModernFrameProcessed(uint32_t frameNumber, const eti::EtiFrame& frame, 
                               std::chrono::nanoseconds parseTime);
    void onFIGAnalysisComplete(uint32_t frameNumber, const eti::modern::FIGAnalysisResult& analysis);
    void onModernEnsembleDiscovered(const eti::modern::EnhancedEnsembleInfo& ensemble);
    void onModernServiceDiscovered(const eti::modern::EnhancedServiceInfo& service);
    void onComplianceIssueDetected(const QString& standard, const QString& issue, const QString& severity);
    void onPerformanceTargetResult(bool targetMet, double currentFps, double targetFps);

private:
    // Modern ETI Core Engine components
    std::unique_ptr<eti::modern::ModernETIFrameParser> m_frameParser;
    std::unique_ptr<eti::modern::EnhancedFIGAnalyzer> m_figAnalyzer;
    std::unique_ptr<eti::modern::PerformanceProfiler> m_profiler;
    
    // Processing configuration
    eti::modern::ProcessingConfig m_processingConfig;
    
    bool m_initialized;
    quint64 m_frameCount;
    QString m_status;
    QString m_currentFile;
    
    // Enhanced service discovery state
    eti::Ensemble m_currentEnsemble;
    std::vector<eti::DabService> m_discoveredServices;
    std::vector<eti::SubChannelInfo> m_subchannels;
    std::map<uint16_t, ServiceQuality> m_serviceQuality;
    
    // Performance statistics
    std::chrono::high_resolution_clock::time_point m_processingStartTime;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    double m_averageFrameRate;
    uint64_t m_totalProcessingTime;
    
    // Modern ETI Core Engine utilities
    double getCurrentMemoryUsage() const;
    void validatePerformanceTargets(double fps, double memoryMB);
    void generatePerformanceReport(const eti::modern::ModernETIFrameParser::PerformanceStats& stats);
    
    // ETSI compliance tracking
    struct ComplianceStatus {
        bool etsi_en_300_799_compliant;
        bool etsi_en_300_401_compliant;
        std::vector<std::string> compliance_warnings;
        std::vector<std::string> compliance_errors;
    } m_complianceStatus;
};