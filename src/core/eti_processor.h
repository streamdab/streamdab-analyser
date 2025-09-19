#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include "eti_types.h"
#include "etisnoop_wrapper.h"

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
     * @brief Get ETISnoop wrapper instance for direct access
     * @return ETISnoop wrapper instance
     */
    EtisnoopWrapper* getEtisnoopWrapper() const { return m_etisnoopWrapper.get(); }

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
    
    // Enhanced service management
    eti::Ensemble getCurrentEnsemble() const { return m_currentEnsemble; }
    std::vector<eti::DabService> getCurrentServices() const { return m_discoveredServices; }
    std::vector<eti::SubChannelInfo> getCurrentSubchannels() const { return m_subchannels; }
    
    // Service quality and status indicators
    struct ServiceQuality {
        double signalStrength;
        double errorRate;
        bool isValid;
        std::chrono::system_clock::time_point lastUpdate;
    };
    
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
     * @param quality Quality metrics
     */
    void serviceQualityChanged(uint16_t serviceId, const ServiceQuality& quality);

private slots:
    // ETISnoop wrapper signal handlers
    void onEtisnoopFrameProcessed(const EtisnoopWrapper::FrameAnalysis& analysis);
    void onEtisnoopServiceDiscovered(const EtisnoopWrapper::ServiceInfo& service);
    void onEtisnoopEnsembleUpdated(const EtisnoopWrapper::EnsembleInfo& ensemble);
    void onEtisnoopError(const QString& error);
    void onEtisnoopStatsUpdated(const EtisnoopWrapper::ProcessingStats& stats);
    void onEtisnoopThaiComplianceChanged(const EtisnoopWrapper::ThaiComplianceStatus& status);

private:
    // ETISnoop integration
    std::unique_ptr<EtisnoopWrapper> m_etisnoopWrapper;
    
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
    
    // ETSI compliance tracking
    struct ComplianceStatus {
        bool etsi_en_300_799_compliant;
        bool etsi_en_300_401_compliant;
        std::vector<std::string> compliance_warnings;
        std::vector<std::string> compliance_errors;
    } m_complianceStatus;
};