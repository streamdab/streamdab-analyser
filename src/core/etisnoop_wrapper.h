#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QList>
#include <QVariantMap>
#include <memory>
#include <chrono>

// Forward declarations for ETISnoop integration
struct eti_analyse_config_t;
class ETI_Analyser;
class FIGalyser;

#ifdef HAVE_NLOHMANN_JSON
namespace StreamDAB {
    class StreamDABIntegrationEngine;
    class ETSIStandardsAnalyzer;
    struct ETIAnalysisReport;
    struct ComplianceResult;
}

namespace Thai {
    class ThaiAnalysisEngine;
    struct ThaiMetadata;
    enum class ComplianceLevel;
}
#endif

/**
 * @class EtisnoopWrapper
 * @brief High-level C++ wrapper around ETISnoop functionality
 * 
 * Provides a clean Qt-compatible interface to the proven ETISnoop backend
 * with Thai DAB standards support and real-time processing capabilities.
 * Designed for >900 fps performance with Qt signal/slot integration.
 */
class EtisnoopWrapper : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Service information structure
     */
    struct ServiceInfo {
        uint32_t serviceId;
        uint16_t subChannelId;
        QString serviceName;
        QString serviceNameThai;
        QString programType;
        QString language;
        bool isActive;
        double signalQuality;
        QVariantMap additionalData;
        
        ServiceInfo() : serviceId(0), subChannelId(0), isActive(false), signalQuality(0.0) {}
    };

    /**
     * @brief Ensemble information structure
     */
    struct EnsembleInfo {
        uint16_t ensembleId;
        QString ensembleName;
        QString ensembleNameThai;
        QString country;
        QList<ServiceInfo> services;
        int totalSubchannels;
        double overallQuality;
        QVariantMap metadata;
        
        EnsembleInfo() : ensembleId(0), totalSubchannels(0), overallQuality(0.0) {}
    };

    /**
     * @brief ETI frame analysis result
     */
    struct FrameAnalysis {
        bool isValid;
        uint32_t frameNumber;
        QByteArray rawData;
        QString errorMessage;
        double processingTime; // milliseconds
        QVariantMap figData;   // FIG analysis results
        QVariantMap compliance; // ETSI compliance data
        
        FrameAnalysis() : isValid(false), frameNumber(0), processingTime(0.0) {}
    };

    /**
     * @brief Thai DAB compliance status
     */
    struct ThaiComplianceStatus {
#ifdef HAVE_NLOHMANN_JSON
        Thai::ComplianceLevel level;
#else
        int level; // Fallback when Thai support not available
#endif
        double complianceScore;
        QString statusMessage;
        QStringList issues;
        QStringList recommendations;
        QVariantMap culturalAnalysis;
        
        ThaiComplianceStatus() : level(0), complianceScore(0.0) {}
    };

    /**
     * @brief Processing statistics for performance monitoring
     */
    struct ProcessingStats {
        qint64 totalFramesProcessed;
        double averageFrameRate; // frames per second
        double averageProcessingTime; // milliseconds per frame
        qint64 totalProcessingTime; // total milliseconds
        double memoryUsage; // MB
        double cpuUsage; // percentage
        std::chrono::system_clock::time_point lastUpdate;
        
        ProcessingStats() : totalFramesProcessed(0), averageFrameRate(0.0), 
                           averageProcessingTime(0.0), totalProcessingTime(0),
                           memoryUsage(0.0), cpuUsage(0.0) {}
    };

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit EtisnoopWrapper(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~EtisnoopWrapper();

    /**
     * @brief Initialize the ETISnoop wrapper
     * @param enableThaiSupport Enable Thai language analysis
     * @param enableStreamDAB Enable StreamDAB integration
     * @return true if initialization successful
     */
    bool initialize(bool enableThaiSupport = true, bool enableStreamDAB = true);

    /**
     * @brief Check if wrapper is initialized and ready
     * @return true if ready for processing
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Process ETI file using ETISnoop backend
     * @param filename Path to ETI file
     * @return true if processing successful
     */
    bool processEtiFile(const QString& filename);

    /**
     * @brief Process ETI data from memory buffer
     * @param data ETI data buffer
     * @return true if processing successful
     */
    bool processEtiData(const QByteArray& data);

    /**
     * @brief Process single ETI frame for real-time analysis
     * @param frameData Raw ETI frame data (6144 bytes)
     * @return Frame analysis result
     */
    FrameAnalysis processEtiFrame(const QByteArray& frameData);

    /**
     * @brief Get discovered services from current ETI stream
     * @return List of discovered services
     */
    QList<ServiceInfo> getDiscoveredServices() const;

    /**
     * @brief Get ensemble information from current ETI stream
     * @return Ensemble information structure
     */
    EnsembleInfo getEnsembleInformation() const;

    /**
     * @brief Get service by ID
     * @param serviceId Service identifier
     * @return Service information or invalid service if not found
     */
    ServiceInfo getServiceById(uint32_t serviceId) const;

    /**
     * @brief Check if a service is currently active
     * @param serviceId Service identifier
     * @return true if service is active and broadcasting
     */
    bool isServiceActive(uint32_t serviceId) const;

    // Thai DAB Standards Support
    /**
     * @brief Enable/disable Thai localization support
     * @param enabled Enable Thai analysis
     */
    void setThaiLocalization(bool enabled);

    /**
     * @brief Get Thai service name for service ID
     * @param serviceId Service identifier
     * @return Thai service name or empty string if not available
     */
    QString getThaiServiceName(uint32_t serviceId) const;

    /**
     * @brief Get Thai compliance status for current stream
     * @return Thai compliance analysis
     */
    ThaiComplianceStatus getThaiComplianceStatus() const;

    /**
     * @brief Validate Thai character encoding in text
     * @param text Text to validate
     * @return true if text is properly encoded for Thai DAB
     */
    bool validateThaiEncoding(const QString& text) const;

    // Performance and Statistics
    /**
     * @brief Get current processing statistics
     * @return Processing performance metrics
     */
    ProcessingStats getProcessingStats() const;

    /**
     * @brief Reset processing statistics
     */
    void resetStats();

    /**
     * @brief Set processing target frame rate for optimization
     * @param targetFps Target frames per second (default: 900+)
     */
    void setTargetFrameRate(double targetFps);

    /**
     * @brief Get current average frame processing rate
     * @return Current frames per second
     */
    double getCurrentFrameRate() const;

    // ETSI Compliance and Quality
    /**
     * @brief Get ETSI compliance score for current stream
     * @return Compliance score (0-100)
     */
    double getETSIComplianceScore() const;

    /**
     * @brief Get detailed ETSI compliance analysis
     * @return Map of compliance results by standard
     */
    QVariantMap getETSIComplianceDetails() const;

    /**
     * @brief Get signal quality metrics for service
     * @param serviceId Service identifier
     * @return Signal quality (0.0-1.0) or -1.0 if service not found
     */
    double getServiceSignalQuality(uint32_t serviceId) const;

    // Audio and Content Extraction
    /**
     * @brief Extract audio data from service
     * @param serviceId Service identifier
     * @param outputFormat Output format ("wav", "mp3", "aac")
     * @return Extracted audio data or empty if extraction failed
     */
    QByteArray extractServiceAudio(uint32_t serviceId, const QString& outputFormat = "wav");

    /**
     * @brief Extract Dynamic Label Segment (DLS) text from service
     * @param serviceId Service identifier
     * @return Current DLS text or empty string
     */
    QString getServiceDLS(uint32_t serviceId) const;

    /**
     * @brief Get complete analysis output in YAML format
     * @return Full ETI analysis results as YAML string
     */
    QString getAnalysisOutput() const;

    /**
     * @brief Get latest frame analysis results
     * @return Recent frame analysis data
     */
    QString getFrameAnalysisOutput() const;

    /**
     * @brief Extract Dynamic Label Segment (DLS) text from service (internal method)
     * @param serviceId Service identifier
     * @return Current DLS text or empty string
     */
    QString extractServiceDLS(uint32_t serviceId) const;

    /**
     * @brief Extract Program Associated Data (PAD) from service
     * @param serviceId Service identifier
     * @return PAD data as variant map
     */
    QVariantMap extractServicePAD(uint32_t serviceId) const;

    // Advanced Configuration
    /**
     * @brief Configure ETISnoop analysis options
     * @param options Configuration options map
     */
    void setAnalysisOptions(const QVariantMap& options);

    /**
     * @brief Enable real-time processing mode
     * @param enabled Enable real-time mode
     */
    void setRealTimeMode(bool enabled);

    /**
     * @brief Set buffer size for real-time processing
     * @param bufferSize Buffer size in frames
     */
    void setBufferSize(int bufferSize);

    /**
     * @brief Get last error message
     * @return Last error message or empty string
     */
    QString getLastError() const { return m_lastError; }

    /**
     * @brief Clear all cached data and reset internal state
     */
    void clearCache();

signals:
    /**
     * @brief Emitted when ETI frame is processed
     * @param analysis Frame analysis result
     */
    void frameProcessed(const FrameAnalysis& analysis);

    /**
     * @brief Emitted when new service is discovered
     * @param service Service information
     */
    void serviceDiscovered(const ServiceInfo& service);

    /**
     * @brief Emitted when ensemble information is updated
     * @param ensemble Ensemble information
     */
    void ensembleUpdated(const EnsembleInfo& ensemble);

    /**
     * @brief Emitted when service status changes
     * @param serviceId Service identifier
     * @param isActive Current active status
     */
    void serviceStatusChanged(uint32_t serviceId, bool isActive);

    /**
     * @brief Emitted when Thai compliance status changes
     * @param status Thai compliance status
     */
    void thaiComplianceChanged(const ThaiComplianceStatus& status);

    /**
     * @brief Emitted when processing statistics are updated
     * @param stats Current processing statistics
     */
    void statsUpdated(const ProcessingStats& stats);

    /**
     * @brief Emitted when ETSI compliance issue is detected
     * @param standard ETSI standard name
     * @param issue Issue description
     * @param severity Issue severity (Info, Warning, Error, Critical)
     */
    void etsiComplianceIssue(const QString& standard, const QString& issue, const QString& severity);

    /**
     * @brief Emitted when processing error occurs
     * @param error Error message
     */
    void processingError(const QString& error);

    /**
     * @brief Emitted when audio is extracted from service
     * @param serviceId Service identifier
     * @param audioData Extracted audio data
     * @param format Audio format
     */
    void audioExtracted(uint32_t serviceId, const QByteArray& audioData, const QString& format);

    /**
     * @brief Emitted when DLS text is updated for service
     * @param serviceId Service identifier
     * @param dlsText Current DLS text
     */
    void dlsTextUpdated(uint32_t serviceId, const QString& dlsText);

private slots:
    void onEtisnoopAnalysisComplete();
    void onStreamDABUpdate();
    void updateProcessingStats();

private:
    // Core ETISnoop integration
    std::unique_ptr<eti_analyse_config_t> m_config;
    std::unique_ptr<ETI_Analyser> m_analyser;
    std::unique_ptr<FIGalyser> m_figAnalyser;

    // StreamDAB and Thai integration (conditional)
#ifdef HAVE_NLOHMANN_JSON
    std::unique_ptr<StreamDAB::StreamDABIntegrationEngine> m_streamDAB;
    std::unique_ptr<StreamDAB::ETSIStandardsAnalyzer> m_etsiAnalyzer;
    std::unique_ptr<Thai::ThaiAnalysisEngine> m_thaiAnalyzer;
#endif

    // Internal state
    bool m_initialized;
    bool m_thaiEnabled;
    bool m_streamDABEnabled;
    bool m_realTimeMode;
    int m_bufferSize;
    double m_targetFrameRate;
    QString m_lastError;

    // Cached data
    EnsembleInfo m_currentEnsemble;
    QList<ServiceInfo> m_discoveredServices;
    QMap<uint32_t, ServiceInfo> m_serviceMap;
    ThaiComplianceStatus m_thaiStatus;
    ProcessingStats m_stats;

    // Performance tracking
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    std::chrono::high_resolution_clock::time_point m_processingStartTime;
    QList<double> m_frameTimes; // Rolling window of frame processing times

    // Internal methods
    bool initializeETISnoop();
    bool initializeStreamDAB();
    bool initializeThaiAnalysis();
    void updateServiceCache();
    void updateEnsembleCache();
    void processComplianceResults(const QVariantMap& results);
    void processThaiAnalysis(const QVariantMap& metadata);
    void calculatePerformanceMetrics();
    ServiceInfo convertToServiceInfo(const QVariantMap& rawServiceData);
    EnsembleInfo convertToEnsembleInfo(const QVariantMap& rawEnsembleData);
    void emitSignalsForUpdates();
    QString formatThaiText(const QString& rawText) const;
    bool validateFrameData(const QByteArray& data) const;
};

Q_DECLARE_METATYPE(EtisnoopWrapper::ServiceInfo)
Q_DECLARE_METATYPE(EtisnoopWrapper::EnsembleInfo)
Q_DECLARE_METATYPE(EtisnoopWrapper::FrameAnalysis)
Q_DECLARE_METATYPE(EtisnoopWrapper::ThaiComplianceStatus)
Q_DECLARE_METATYPE(EtisnoopWrapper::ProcessingStats)