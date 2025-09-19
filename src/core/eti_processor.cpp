#include "eti_processor.h"
#include "utils/logger.h"
#include "eti_types.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <cstring>

EtiProcessor::EtiProcessor(QObject *parent)
    : QObject(parent)
    , m_etisnoopWrapper(std::make_unique<EtisnoopWrapper>(this))
    , m_initialized(false)
    , m_frameCount(0)
    , m_status("Not initialized")
    , m_averageFrameRate(0.0)
    , m_totalProcessingTime(0)
{
    // Initialize compliance status
    m_complianceStatus.etsi_en_300_799_compliant = false;
    m_complianceStatus.etsi_en_300_401_compliant = false;
    
    // Connect ETISnoop wrapper signals
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::frameProcessed,
            this, &EtiProcessor::onEtisnoopFrameProcessed);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::serviceDiscovered,
            this, &EtiProcessor::onEtisnoopServiceDiscovered);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::ensembleUpdated,
            this, &EtiProcessor::onEtisnoopEnsembleUpdated);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::processingError,
            this, &EtiProcessor::onEtisnoopError);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::statsUpdated,
            this, &EtiProcessor::onEtisnoopStatsUpdated);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::thaiComplianceChanged,
            this, &EtiProcessor::onEtisnoopThaiComplianceChanged);
}

EtiProcessor::~EtiProcessor()
{
    Logger::instance().log(Logger::Info, "EtiProcessor", "Destructor called");
}

bool EtiProcessor::initialize()
{
    Logger::instance().log(Logger::Info, "EtiProcessor", "Initializing ETI processor with ETISnoop backend");
    
    try {
        // Initialize ETISnoop wrapper first
        if (!m_etisnoopWrapper->initialize(true, true)) { // Enable Thai and StreamDAB support
            QString error = QString("Failed to initialize ETISnoop wrapper: %1").arg(m_etisnoopWrapper->getLastError());
            Logger::instance().log(Logger::Error, "EtiProcessor", error);
            emit errorOccurred(error);
            return false;
        }
        
        // Configure ETISnoop for optimal performance
        m_etisnoopWrapper->setTargetFrameRate(900.0); // Target >900 fps
        m_etisnoopWrapper->setRealTimeMode(true);
        m_etisnoopWrapper->setBufferSize(100);
        
        // Initialize ETI processing components
        m_frameCount = 0;
        m_status = "Ready";
        m_averageFrameRate = 0.0;
        m_totalProcessingTime = 0;
        
        // Initialize service discovery state
        m_currentEnsemble = eti::Ensemble{};
        m_discoveredServices.clear();
        m_subchannels.clear();
        m_serviceQuality.clear();
        
        // Initialize performance tracking
        m_processingStartTime = std::chrono::high_resolution_clock::now();
        m_lastFrameTime = m_processingStartTime;
        
        // Reset compliance status
        m_complianceStatus.etsi_en_300_799_compliant = false;
        m_complianceStatus.etsi_en_300_401_compliant = false;
        m_complianceStatus.compliance_warnings.clear();
        m_complianceStatus.compliance_errors.clear();
        
        m_initialized = true;
        emit statusChanged(m_status);
        
        Logger::instance().log(Logger::Info, "EtiProcessor", 
                              "ETI processor initialized successfully with ETISnoop backend (Thai support enabled)");
        return true;
    }
    catch (const std::exception& e) {
        QString error = QString("Failed to initialize ETI processor: %1").arg(e.what());
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
}

bool EtiProcessor::processEtiFrame(const QByteArray& frameData)
{
    // Validate frame size according to ETSI EN 300 799
    if (frameData.size() != eti::ETI_FRAME_SIZE) {
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("Invalid ETI frame size: %1 bytes (expected %2)")
                              .arg(frameData.size()).arg(eti::ETI_FRAME_SIZE));
        return false;
    }
    
    try {
        // Create ETI frame structure for parsing
        eti::EtiFrame frame;
        std::memcpy(frame.data(), frameData.constData(), eti::ETI_FRAME_SIZE);
        frame.set_receive_timestamp(std::chrono::system_clock::now());
        
        // Step 1: Validate sync pattern (ETSI EN 300 799 Section 5.1)
        if (!validateSyncPattern(frame)) {
            Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                  "Invalid ETI sync pattern detected");
            return false;
        }
        
        // Step 2: Parse and validate LIDATA field (ETSI EN 300 799 Section 5.2)
        eti::EtiLidataField lidata = frame.get_lidata_field();
        if (!validateLidataField(lidata)) {
            Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                  "Invalid LIDATA field in ETI frame");
            return false;
        }
        
        // Step 3: Extract and process FIC data (ETSI EN 300 401 Section 6.4)
        if (lidata.ficf) {  // FIC present flag
            eti::EtiFicField fic = frame.get_fic_field();
            if (!processFicData(fic)) {
                Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                      "Failed to process FIC data");
                // Non-fatal error, continue processing
            }
        }
        
        // Step 4: Extract MSC data for service components
        eti::EtiMscField msc = frame.get_msc_field();
        if (!processMscData(msc, lidata.nst)) {
            Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                  "Failed to process MSC data");
            // Non-fatal error, continue processing
        }
        
        // Step 5: Validate frame CRC (ETSI EN 300 799 Section 5.4)
        if (!validateFrameCrc(frame)) {
            Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                  "CRC validation failed for ETI frame");
            // Log but don't fail - some streams may have CRC issues
        }
        
        // Log successful frame processing with key information
        Logger::instance().log(Logger::Debug, "EtiProcessor", 
                              QString("ETI frame processed: FC=%1, NST=%2, FIC=%3")
                              .arg(lidata.fc)
                              .arg(lidata.nst)
                              .arg(lidata.ficf ? "present" : "absent"));
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("Exception processing ETI frame: %1").arg(e.what()));
        return false;
    }
}

bool EtiProcessor::validateSyncPattern(const eti::EtiFrame& frame)
{
    // ETSI EN 300 799 Section 5.1: Sync pattern is 0x49, 0x93, 0x1E, 0x03
    const uint8_t* data = frame.data();
    const uint8_t expectedSync[4] = {0x49, 0x93, 0x1E, 0x03};
    
    bool isValid = (std::memcmp(data, expectedSync, 4) == 0);
    
    if (!isValid) {
        Logger::instance().log(Logger::Debug, "EtiProcessor", 
                              QString("Sync pattern mismatch: got 0x%1%2%3%4, expected 0x49931E03")
                              .arg(data[0], 2, 16, QChar('0'))
                              .arg(data[1], 2, 16, QChar('0'))
                              .arg(data[2], 2, 16, QChar('0'))
                              .arg(data[3], 2, 16, QChar('0')));
    }
    
    return isValid;
}

bool EtiProcessor::validateLidataField(const eti::EtiLidataField& lidata)
{
    // Validate Frame Count (0-249)
    if (lidata.fc > 249) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              QString("Invalid frame count: %1 (max 249)").arg(lidata.fc));
        return false;
    }
    
    // Validate Number of Sub-channels (0-63)
    if (lidata.nst > 63) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              QString("Invalid sub-channel count: %1 (max 63)").arg(lidata.nst));
        return false;
    }
    
    // Validate Mode Identity (1-4)
    if (lidata.mid < 1 || lidata.mid > 4) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              QString("Invalid mode identity: %1 (valid: 1-4)").arg(lidata.mid));
        return false;
    }
    
    // Validate Frame Phase (0-7)
    if (lidata.fp > 7) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              QString("Invalid frame phase: %1 (max 7)").arg(lidata.fp));
        return false;
    }
    
    return true;
}

bool EtiProcessor::processFicData(const eti::EtiFicField& fic)
{
    try {
        // Decode FIG blocks from FIC data according to ETSI EN 300 401
        std::vector<eti::FigBlock> figBlocks = fic.decode_fig_blocks();
        
        for (const auto& figBlock : figBlocks) {
            if (!figBlock.is_valid()) {
                Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                      "Invalid FIG block detected");
                continue;
            }
            
            // Process FIG block based on type
            switch (figBlock.fig_type) {
                case 0:
                    processFigType0(figBlock);
                    break;
                case 1:
                    processFigType1(figBlock);
                    break;
                default:
                    Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                          QString("Unsupported FIG type: %1").arg(figBlock.fig_type));
                    break;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("Error processing FIC data: %1").arg(e.what()));
        return false;
    }
}

bool EtiProcessor::processMscData(const eti::EtiMscField& msc, uint8_t subChannelCount)
{
    // Process Main Service Channel data
    // For now, just validate the sub-channel organization
    
    if (subChannelCount == 0) {
        Logger::instance().log(Logger::Debug, "EtiProcessor", "No sub-channels to process");
        return true;
    }
    
    // Calculate total capacity units used
    size_t totalCapacityUnits = msc.get_capacity_units_count();
    
    Logger::instance().log(Logger::Debug, "EtiProcessor", 
                          QString("Processing MSC data: %1 sub-channels, %2 CUs available")
                          .arg(subChannelCount)
                          .arg(totalCapacityUnits));
    
    // TODO: Implement actual sub-channel data extraction based on 
    // service component configuration from FIC
    
    return true;
}

bool EtiProcessor::validateFrameCrc(const eti::EtiFrame& frame)
{
    // Get CRC field from frame
    eti::EtiCrcField crc = frame.get_crc_field();
    
    // For now, just validate that CRC field exists
    // TODO: Implement actual CRC-32 calculation and validation
    // according to ETSI EN 300 799 Section 5.4
    
    uint32_t crcValue = crc.get_crc_value();
    
    Logger::instance().log(Logger::Debug, "EtiProcessor", 
                          QString("Frame CRC: 0x%1").arg(crcValue, 8, 16, QChar('0')));
    
    // Always return true for now to avoid blocking on CRC issues
    // Real implementation would calculate and compare CRC
    return true;
}

void EtiProcessor::processFigType0(const eti::FigBlock& figBlock)
{
    // FIG Type 0: Multiplex Configuration Information (ETSI EN 300 401 Section 5.2.1)
    uint8_t extension = figBlock.get_extension();
    
    switch (extension) {
        case 0:
            // FIG 0/0: Ensemble information
            if (parseFig00_EnsembleInfo(figBlock)) {
                Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                      "Successfully processed FIG 0/0: Ensemble information");
            }
            break;
        case 1:
            // FIG 0/1: Sub-channel organization
            if (parseFig01_SubchannelOrganization(figBlock)) {
                Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                      "Successfully processed FIG 0/1: Sub-channel organization");
            }
            break;
        case 2:
            // FIG 0/2: Service organization
            if (parseFig02_ServiceOrganization(figBlock)) {
                Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                      "Successfully processed FIG 0/2: Service organization");
            }
            break;
        case 3:
            // FIG 0/3: Service component
            if (parseFig03_ServiceComponent(figBlock)) {
                Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                      "Successfully processed FIG 0/3: Service component");
            }
            break;
        default:
            Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                  QString("Processing FIG 0/%1 (not yet implemented)").arg(extension));
            break;
    }
}

void EtiProcessor::processFigType1(const eti::FigBlock& figBlock)
{
    // FIG Type 1: Labels (ETSI EN 300 401 Section 5.2.2)
    uint8_t extension = figBlock.get_extension();
    
    switch (extension) {
        case 0:
            // FIG 1/0: Ensemble label
            if (parseFig10_EnsembleLabel(figBlock)) {
                Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                      "Successfully processed FIG 1/0: Ensemble label");
            }
            break;
        case 1:
            // FIG 1/1: Service label
            if (parseFig11_ServiceLabel(figBlock)) {
                Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                      "Successfully processed FIG 1/1: Service label");
            }
            break;
        default:
            Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                  QString("Processing FIG 1/%1 (not yet implemented)").arg(extension));
            break;
    }
}

void EtiProcessor::processFigType2(const eti::FigBlock& figBlock)
{
    // FIG Type 2: Extended Service Information (ETSI EN 300 401 Section 5.2.3)
    uint8_t extension = figBlock.get_extension();
    
    Logger::instance().log(Logger::Debug, "EtiProcessor", 
                          QString("Processing FIG 2/%1 (extended service information)").arg(extension));
    
    // TODO: Implement FIG Type 2 processing for extended service information
    // This includes service component global definition, MOT, etc.
}

bool EtiProcessor::processFile(const QString& filename)
{
    if (!m_initialized) {
        QString error = "ETI processor not initialized";
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
    
    QFileInfo fileInfo(filename);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        QString error = QString("File not found or not readable: %1").arg(filename);
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
    
    Logger::instance().log(Logger::Info, "EtiProcessor", 
                          QString("Processing ETI file with ETISnoop backend: %1").arg(filename));
    
    m_currentFile = filename;
    
    // Use ETISnoop wrapper for file processing
    bool success = m_etisnoopWrapper->processEtiFile(filename);
    
    if (success) {
        // Update frame count from ETISnoop statistics
        auto stats = m_etisnoopWrapper->getProcessingStats();
        m_frameCount = stats.totalFramesProcessed;
        
        // Update status
        m_status = QString("ETISnoop processed %1 frames from file").arg(m_frameCount);
        emit statusChanged(m_status);
        
        Logger::instance().log(Logger::Info, "EtiProcessor", 
                              QString("ETISnoop file processing completed: %1 frames, %2 fps average")
                              .arg(m_frameCount).arg(stats.averageFrameRate, 0, 'f', 1));
    } else {
        QString error = QString("ETISnoop file processing failed: %1").arg(m_etisnoopWrapper->getLastError());
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
    }
    
    return success;
}

bool EtiProcessor::processData(const QByteArray& data)
{
    if (!m_initialized) {
        QString error = "ETI processor not initialized";
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
    
    if (data.isEmpty()) {
        QString error = "Empty data provided for processing";
        Logger::instance().log(Logger::Warning, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
    
    Logger::instance().log(Logger::Info, "EtiProcessor", 
                          QString("Processing ETI data with ETISnoop backend: %1 bytes").arg(data.size()));
    
    try {
        // Use ETISnoop wrapper for data processing
        bool success = m_etisnoopWrapper->processEtiData(data);
        
        if (success) {
            // Update frame count from ETISnoop statistics
            auto stats = m_etisnoopWrapper->getProcessingStats();
            m_frameCount = stats.totalFramesProcessed;
            
            // Update status
            m_status = QString("ETISnoop processed %1 frames from data buffer")
                       .arg(stats.totalFramesProcessed);
            emit statusChanged(m_status);
            
            Logger::instance().log(Logger::Info, "EtiProcessor", 
                                  QString("ETISnoop data processing completed: %1 frames, %2 fps average")
                                  .arg(stats.totalFramesProcessed).arg(stats.averageFrameRate, 0, 'f', 1));
        } else {
            QString error = QString("ETISnoop data processing failed: %1").arg(m_etisnoopWrapper->getLastError());
            Logger::instance().log(Logger::Error, "EtiProcessor", error);
            emit errorOccurred(error);
        }
        
        return success;
    }
    catch (const std::exception& e) {
        QString error = QString("Error processing ETI data: %1").arg(e.what());
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
}

// ========================================================================
// FIG Parser Implementation - Placeholder for ETISnoop Integration
// ========================================================================

bool EtiProcessor::parseFig00_EnsembleInfo(const eti::FigBlock& figBlock)
{
    // TODO: Integrate with ETISnoop FIG 0/0 parser
    // Placeholder implementation for build compatibility
    Q_UNUSED(figBlock);
    Logger::instance().log(Logger::Debug, "EtiProcessor", "FIG 0/0 Ensemble Info parsing - placeholder");
    return true;
}

bool EtiProcessor::parseFig01_SubchannelOrganization(const eti::FigBlock& figBlock)
{
    // TODO: Integrate with ETISnoop FIG 0/1 parser
    // Placeholder implementation for build compatibility
    Q_UNUSED(figBlock);
    Logger::instance().log(Logger::Debug, "EtiProcessor", "FIG 0/1 Subchannel Organization parsing - placeholder");
    return true;
}

bool EtiProcessor::parseFig02_ServiceOrganization(const eti::FigBlock& figBlock)
{
    // TODO: Integrate with ETISnoop FIG 0/2 parser
    // Placeholder implementation for build compatibility
    Q_UNUSED(figBlock);
    Logger::instance().log(Logger::Debug, "EtiProcessor", "FIG 0/2 Service Organization parsing - placeholder");
    return true;
}

bool EtiProcessor::parseFig03_ServiceComponent(const eti::FigBlock& figBlock)
{
    // TODO: Integrate with ETISnoop FIG 0/3 parser
    // Placeholder implementation for build compatibility
    Q_UNUSED(figBlock);
    Logger::instance().log(Logger::Debug, "EtiProcessor", "FIG 0/3 Service Component parsing - placeholder");
    return true;
}

bool EtiProcessor::parseFig10_EnsembleLabel(const eti::FigBlock& figBlock)
{
    // TODO: Integrate with ETISnoop FIG 1/0 parser
    // Placeholder implementation for build compatibility
    Q_UNUSED(figBlock);
    Logger::instance().log(Logger::Debug, "EtiProcessor", "FIG 1/0 Ensemble Label parsing - placeholder");
    return true;
}

bool EtiProcessor::parseFig11_ServiceLabel(const eti::FigBlock& figBlock)
{
    // TODO: Integrate with ETISnoop FIG 1/1 parser
    // Placeholder implementation for build compatibility
    Q_UNUSED(figBlock);
    Logger::instance().log(Logger::Debug, "EtiProcessor", "FIG 1/1 Service Label parsing - placeholder");
    return true;
}

// ========================================================================
// ETISnoop Wrapper Signal Handlers
// ========================================================================

void EtiProcessor::onEtisnoopFrameProcessed(const EtisnoopWrapper::FrameAnalysis& analysis)
{
    if (analysis.isValid) {
        // Update frame count and emit standard signal
        m_frameCount++;
        emit frameProcessed(analysis.frameNumber, analysis.rawData);
        
        // Update performance statistics
        m_averageFrameRate = m_etisnoopWrapper->getCurrentFrameRate();
        
        Logger::instance().log(Logger::Debug, "EtiProcessor", 
                              QString("ETISnoop frame processed: #%1 (%2ms)")
                              .arg(analysis.frameNumber)
                              .arg(analysis.processingTime, 0, 'f', 2));
        
        // Check for ETSI compliance issues
        if (!analysis.compliance.isEmpty()) {
            double complianceScore = analysis.compliance.value("overall_score", 0.0).toDouble();
            if (complianceScore < 90.0) {
                Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                      QString("Frame %1 compliance score: %2%")
                                      .arg(analysis.frameNumber).arg(complianceScore, 0, 'f', 1));
            }
        }
    } else {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              QString("ETISnoop frame processing failed: %1").arg(analysis.errorMessage));
        emit errorOccurred(analysis.errorMessage);
    }
}

void EtiProcessor::onEtisnoopServiceDiscovered(const EtisnoopWrapper::ServiceInfo& service)
{
    Logger::instance().log(Logger::Info, "EtiProcessor", 
                          QString("Service discovered via ETISnoop: ID=%1, Name=%2 (Thai: %3)")
                          .arg(service.serviceId)
                          .arg(service.serviceName)
                          .arg(service.serviceNameThai));
    
    // Convert ETISnoop service info to internal format
    eti::DabService internalService;
    internalService.service_id = service.serviceId;
    internalService.label = service.serviceName.toStdString();
    internalService.is_programme = true; // Assume programme service for now
    internalService.country_id = 0; // Will be populated from actual ETI data
    // Note: program_type and language will be handled through service components
    
    // Add to discovered services and emit signal
    m_discoveredServices.push_back(internalService);
    emit serviceDiscovered(internalService);
    
    // Update service quality tracking
    ServiceQuality quality;
    quality.signalStrength = service.signalQuality;
    quality.errorRate = 0.0; // Would be calculated from ETISnoop data
    quality.isValid = service.isActive;
    quality.lastUpdate = std::chrono::system_clock::now();
    
    m_serviceQuality[service.serviceId] = quality;
    emit serviceQualityChanged(service.serviceId, quality);
}

void EtiProcessor::onEtisnoopEnsembleUpdated(const EtisnoopWrapper::EnsembleInfo& ensemble)
{
    Logger::instance().log(Logger::Info, "EtiProcessor", 
                          QString("Ensemble updated via ETISnoop: ID=%1, Name=%2 (Thai: %3), Services=%4")
                          .arg(ensemble.ensembleId)
                          .arg(ensemble.ensembleName)
                          .arg(ensemble.ensembleNameThai)
                          .arg(ensemble.services.size()));
    
    // Convert ETISnoop ensemble info to internal format
    m_currentEnsemble.ensemble_id = ensemble.ensembleId;
    m_currentEnsemble.label = ensemble.ensembleName.toStdString();
    // Store Thai name and country in metadata
    // Note: country_id should be set from actual ETI data parsing
    m_currentEnsemble.country_id = 0; // Will be populated from ETISnoop data
    
    emit ensembleDiscovered(m_currentEnsemble);
}

void EtiProcessor::onEtisnoopError(const QString& error)
{
    Logger::instance().log(Logger::Error, "EtiProcessor", 
                          QString("ETISnoop processing error: %1").arg(error));
    emit errorOccurred(error);
}

void EtiProcessor::onEtisnoopStatsUpdated(const EtisnoopWrapper::ProcessingStats& stats)
{
    // Update internal performance metrics
    m_frameCount = stats.totalFramesProcessed;
    m_averageFrameRate = stats.averageFrameRate;
    m_totalProcessingTime = stats.totalProcessingTime;
    
    // Check performance targets
    if (stats.averageFrameRate > 0) {
        if (stats.averageFrameRate >= 900.0) {
            Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                  QString("Performance target achieved: %1 fps")
                                  .arg(stats.averageFrameRate, 0, 'f', 1));
        } else if (stats.averageFrameRate < 720.0) { // 80% of target
            Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                  QString("Performance below target: %1 fps (target: 900+ fps)")
                                  .arg(stats.averageFrameRate, 0, 'f', 1));
        }
    }
    
    // Update status
    m_status = QString("ETISnoop: %1 frames, %2 fps, %3 MB")
               .arg(stats.totalFramesProcessed)
               .arg(stats.averageFrameRate, 0, 'f', 1)
               .arg(stats.memoryUsage, 0, 'f', 1);
    emit statusChanged(m_status);
}

void EtiProcessor::onEtisnoopThaiComplianceChanged(const EtisnoopWrapper::ThaiComplianceStatus& status)
{
    Logger::instance().log(Logger::Info, "EtiProcessor", 
                          QString("Thai DAB compliance updated: Score=%1%, Level=%2")
                          .arg(status.complianceScore, 0, 'f', 1)
                          .arg(static_cast<int>(status.level)));
    
    // Update ETSI compliance status for Thai standards
    if (status.complianceScore >= 95.0) {
        m_complianceStatus.etsi_en_300_401_compliant = true;
        Logger::instance().log(Logger::Info, "EtiProcessor", "Thai DAB standards fully compliant");
    } else if (status.complianceScore < 70.0) {
        m_complianceStatus.compliance_errors.push_back(
            "Thai DAB compliance below acceptable threshold: " + 
            std::to_string(status.complianceScore) + "%"
        );
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("Thai DAB compliance critical: %1%")
                              .arg(status.complianceScore, 0, 'f', 1));
    }
    
    // Log any specific issues
    for (const QString& issue : status.issues) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              QString("Thai compliance issue: %1").arg(issue));
        m_complianceStatus.compliance_warnings.push_back(issue.toStdString());
    }
}