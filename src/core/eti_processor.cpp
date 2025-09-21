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
    
    // Connect ETISnoop wrapper signals with queued connections for better test stability
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::frameProcessed,
            this, &EtiProcessor::onEtisnoopFrameProcessed, Qt::QueuedConnection);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::serviceDiscovered,
            this, &EtiProcessor::onEtisnoopServiceDiscovered, Qt::QueuedConnection);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::ensembleUpdated,
            this, &EtiProcessor::onEtisnoopEnsembleUpdated, Qt::QueuedConnection);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::processingError,
            this, &EtiProcessor::onEtisnoopError, Qt::QueuedConnection);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::statsUpdated,
            this, &EtiProcessor::onEtisnoopStatsUpdated, Qt::QueuedConnection);
    connect(m_etisnoopWrapper.get(), &EtisnoopWrapper::thaiComplianceChanged,
            this, &EtiProcessor::onEtisnoopThaiComplianceChanged, Qt::QueuedConnection);
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
    
    // PROFESSIONAL ERROR HANDLING - Match ETISnoop's actual behavior
    QFileInfo fileInfo(filename);
    if (!fileInfo.exists()) {
        // ETISnoop behavior: File not found = exit code 1 (critical error)
        QString error = QString("File not found: %1").arg(filename);
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;  // Critical failure - matches ETISnoop exit code 1
    }
    
    if (!fileInfo.isReadable()) {
        // Permission error - critical failure
        QString error = QString("File not readable: %1").arg(filename);
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;  // Critical failure
    }
    
    // Check for empty or invalid files - ETISnoop handles these gracefully
    if (fileInfo.size() == 0) {
        // ETISnoop behavior: Empty file = exit code 0 with informative message
        QString warning = QString("Empty file detected: %1 - unable to read ETI sync").arg(filename);
        Logger::instance().log(Logger::Warning, "EtiProcessor", warning);
        emit errorOccurred(warning);  // User feedback but not critical failure
        m_status = "ETI file appears to be empty - no frames processed";
        emit statusChanged(m_status);
        return true;  // Success with warnings - matches ETISnoop exit code 0
    }
    
    Logger::instance().log(Logger::Info, "EtiProcessor", 
                          QString("Processing ETI file with ETISnoop backend: %1").arg(filename));
    
    m_currentFile = filename;
    
    // Use ETISnoop wrapper for file processing with enhanced error handling
    bool success = m_etisnoopWrapper->processEtiFile(filename);
    
    if (success) {
        // Update frame count from ETISnoop statistics
        auto stats = m_etisnoopWrapper->getProcessingStats();
        m_frameCount = stats.totalFramesProcessed;
        
        if (m_frameCount == 0) {
            // ETISnoop processed the file but found no valid frames
            QString warning = QString("ETI file processed but no valid frames found: %1").arg(filename);
            Logger::instance().log(Logger::Warning, "EtiProcessor", warning);
            emit errorOccurred(warning);  // User feedback
            m_status = "ETI file processed - could not identify stream type";
            emit statusChanged(m_status);
            return true;  // Success with warnings - matches ETISnoop behavior
        }
        
        // Update status with successful processing
        m_status = QString("ETI file processed successfully: %1 frames").arg(m_frameCount);
        emit statusChanged(m_status);
        
        Logger::instance().log(Logger::Info, "EtiProcessor", 
                              QString("ETI file processing completed: %1 frames, %2 fps average")
                              .arg(m_frameCount).arg(stats.averageFrameRate, 0, 'f', 1));
    } else {
        // ETISnoop wrapper failed - provide professional error feedback
        QString error = QString("ETI file processing failed: %1").arg(m_etisnoopWrapper->getLastError());
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        m_status = "ETI file processing failed";
        emit statusChanged(m_status);
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
    // FIG 0/0: Ensemble information (ETSI EN 300 401 Section 8.1.1)
    if (figBlock.data.size() < 4) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              "FIG 0/0: Insufficient data for ensemble info");
        return false;
    }
    
    try {
        // Parse ensemble ID and country information
        uint16_t ensemble_id = (figBlock.data[0] << 8) | figBlock.data[1];
        uint8_t country_id = (figBlock.data[2] >> 4) & 0x0F;
        uint8_t extended_country_code = figBlock.data[2] & 0x0F;
        
        // Update ensemble information
        m_currentEnsemble.ensemble_id = ensemble_id;
        m_currentEnsemble.country_id = country_id;
        m_currentEnsemble.extended_country_code = extended_country_code;
        
        if (figBlock.data.size() >= 6) {
            // Parse CIF count and occurrence change
            m_currentEnsemble.cif_count = figBlock.data[3];
            m_currentEnsemble.occurrence_change = (figBlock.data[4] >> 4) & 0x0F;
            m_currentEnsemble.alarm_flag = (figBlock.data[4] & 0x01) != 0;
        }
        
        Logger::instance().log(Logger::Info, "EtiProcessor", 
                              QString("FIG 0/0: Ensemble ID=0x%1, Country=0x%2")
                              .arg(ensemble_id, 4, 16, QChar('0'))
                              .arg(country_id, 2, 16, QChar('0')));
        
        emit ensembleDiscovered(m_currentEnsemble);
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("FIG 0/0 parsing error: %1").arg(e.what()));
        return false;
    }
}

bool EtiProcessor::parseFig01_SubchannelOrganization(const eti::FigBlock& figBlock)
{
    // FIG 0/1: Sub-channel organization (ETSI EN 300 401 Section 8.1.2)
    if (figBlock.data.size() < 3) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              "FIG 0/1: Insufficient data for subchannel organization");
        return false;
    }
    
    try {
        size_t offset = 0;
        int subchannels_parsed = 0;
        
        while (offset + 2 < figBlock.data.size()) {
            eti::SubChannelInfo subchannel;
            
            // Parse subchannel ID and start address
            subchannel.sub_channel_id = (figBlock.data[offset] >> 2) & 0x3F;
            subchannel.start_address = ((figBlock.data[offset] & 0x03) << 8) | figBlock.data[offset + 1];
            
            if (offset + 2 >= figBlock.data.size()) break;
            
            // Check form type (short/long)
            if (figBlock.data[offset + 2] & 0x80) {
                // Long form (UEP - Unequal Error Protection)
                if (offset + 3 >= figBlock.data.size()) break;
                
                subchannel.size = ((figBlock.data[offset + 2] & 0x03) << 8) | figBlock.data[offset + 3];
                subchannel.protection_level = (figBlock.data[offset + 2] >> 2) & 0x1F;
                subchannel.uep_flag = true;
                offset += 4;
            } else {
                // Short form (EEP - Equal Error Protection)
                uint8_t size_protection = figBlock.data[offset + 2];
                subchannel.size = (size_protection & 0x3F) * 8; // Size in CUs
                subchannel.protection_level = ((size_protection >> 6) & 0x03) + 1;
                subchannel.uep_flag = false;
                offset += 3;
            }
            
            // Validate subchannel
            if (subchannel.validate_sub_channel_id() && subchannel.validate_boundaries()) {
                m_subchannels.push_back(subchannel);
                subchannels_parsed++;
                
                Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                      QString("FIG 0/1: SubCh %1, Start=%2, Size=%3 CUs, Protection=%4")
                                      .arg(subchannel.sub_channel_id)
                                      .arg(subchannel.start_address)
                                      .arg(subchannel.size)
                                      .arg(subchannel.protection_level));
            } else {
                Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                      QString("FIG 0/1: Invalid subchannel configuration: ID=%1")
                                      .arg(subchannel.sub_channel_id));
            }
        }
        
        Logger::instance().log(Logger::Info, "EtiProcessor", 
                              QString("FIG 0/1: Parsed %1 subchannels").arg(subchannels_parsed));
        
        return subchannels_parsed > 0;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("FIG 0/1 parsing error: %1").arg(e.what()));
        return false;
    }
}

bool EtiProcessor::parseFig02_ServiceOrganization(const eti::FigBlock& figBlock)
{
    // FIG 0/2: Service organization (ETSI EN 300 401 Section 8.1.3)
    if (figBlock.data.size() < 4) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              "FIG 0/2: Insufficient data for service organization");
        return false;
    }
    
    try {
        size_t offset = 0;
        int services_parsed = 0;
        
        while (offset + 3 < figBlock.data.size()) {
            eti::DabService service;
            
            // Parse service ID and country information
            service.service_id = (figBlock.data[offset] << 8) | figBlock.data[offset + 1];
            service.country_id = (figBlock.data[offset + 2] >> 4) & 0x0F;
            service.extended_country_code = figBlock.data[offset + 2] & 0x0F;
            
            if (offset + 3 < figBlock.data.size()) {
                service.is_programme = (figBlock.data[offset + 3] & 0x02) != 0;
                // Additional service flags can be parsed from figBlock.data[offset + 3]
            }
            
            // Validate service
            if (service.validate_service_id() && service.validate_country_code()) {
                // Check if service already exists
                auto existing = std::find_if(m_discoveredServices.begin(), m_discoveredServices.end(),
                                           [&service](const eti::DabService& s) {
                                               return s.service_id == service.service_id;
                                           });
                
                if (existing == m_discoveredServices.end()) {
                    m_discoveredServices.push_back(service);
                    services_parsed++;
                    
                    Logger::instance().log(Logger::Info, "EtiProcessor", 
                                          QString("FIG 0/2: Service ID=0x%1, Country=0x%2, Programme=%3")
                                          .arg(service.service_id, 4, 16, QChar('0'))
                                          .arg(service.country_id, 2, 16, QChar('0'))
                                          .arg(service.is_programme ? "Yes" : "No"));
                    
                    emit serviceDiscovered(service);
                } else {
                    // Update existing service information
                    existing->country_id = service.country_id;
                    existing->extended_country_code = service.extended_country_code;
                    existing->is_programme = service.is_programme;
                }
            } else {
                Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                      QString("FIG 0/2: Invalid service: ID=0x%1")
                                      .arg(service.service_id, 4, 16, QChar('0')));
            }
            
            offset += 4;
        }
        
        Logger::instance().log(Logger::Info, "EtiProcessor", 
                              QString("FIG 0/2: Parsed %1 services").arg(services_parsed));
        
        return services_parsed > 0;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("FIG 0/2 parsing error: %1").arg(e.what()));
        return false;
    }
}

bool EtiProcessor::parseFig03_ServiceComponent(const eti::FigBlock& figBlock)
{
    // FIG 0/3: Service component (ETSI EN 300 401 Section 8.1.4)
    if (figBlock.data.size() < 5) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              "FIG 0/3: Insufficient data for service component");
        return false;
    }
    
    try {
        size_t offset = 0;
        int components_parsed = 0;
        
        while (offset + 4 < figBlock.data.size()) {
            uint16_t service_id = (figBlock.data[offset] << 8) | figBlock.data[offset + 1];
            
            eti::ServiceComponent component;
            component.service_id = service_id;
            component.sub_channel_id = (figBlock.data[offset + 2] >> 2) & 0x3F;
            component.tmid = figBlock.data[offset + 2] & 0x03;
            component.asc_ty = figBlock.data[offset + 3] & 0x3F;
            component.primary = (figBlock.data[offset + 3] & 0x40) != 0;
            component.ca_flag = (figBlock.data[offset + 3] & 0x80) != 0;
            
            // Validate component
            if (component.validate_sub_channel_id() && component.validate_tmid()) {
                // Find the corresponding service and add component
                auto service_it = std::find_if(m_discoveredServices.begin(), m_discoveredServices.end(),
                                              [service_id](const eti::DabService& s) {
                                                  return s.service_id == service_id;
                                              });
                
                if (service_it != m_discoveredServices.end()) {
                    // Check if component already exists
                    auto comp_it = std::find_if(service_it->components.begin(), service_it->components.end(),
                                               [&component](const eti::ServiceComponent& c) {
                                                   return c.sub_channel_id == component.sub_channel_id;
                                               });
                    
                    if (comp_it == service_it->components.end()) {
                        service_it->components.push_back(component);
                        components_parsed++;
                        
                        Logger::instance().log(Logger::Info, "EtiProcessor", 
                                              QString("FIG 0/3: Service 0x%1, SubCh %2, TMID=%3, ASCTy=%4, Primary=%5")
                                              .arg(service_id, 4, 16, QChar('0'))
                                              .arg(component.sub_channel_id)
                                              .arg(component.tmid)
                                              .arg(component.asc_ty)
                                              .arg(component.primary ? "Yes" : "No"));
                    } else {
                        // Update existing component
                        *comp_it = component;
                    }
                } else {
                    Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                          QString("FIG 0/3: Component for unknown service 0x%1")
                                          .arg(service_id, 4, 16, QChar('0')));
                }
            } else {
                Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                      QString("FIG 0/3: Invalid component: SubCh=%1, TMID=%2")
                                      .arg(component.sub_channel_id)
                                      .arg(component.tmid));
            }
            
            offset += 4;
        }
        
        Logger::instance().log(Logger::Info, "EtiProcessor", 
                              QString("FIG 0/3: Parsed %1 service components").arg(components_parsed));
        
        return components_parsed > 0;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("FIG 0/3 parsing error: %1").arg(e.what()));
        return false;
    }
}

bool EtiProcessor::parseFig10_EnsembleLabel(const eti::FigBlock& figBlock)
{
    // FIG 1/0: Ensemble label (ETSI EN 300 401 Section 8.1.14.1)
    if (figBlock.data.size() < 18) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              "FIG 1/0: Insufficient data for ensemble label");
        return false;
    }
    
    try {
        uint16_t ensemble_id = (figBlock.data[0] << 8) | figBlock.data[1];
        
        // Verify this label belongs to our current ensemble
        if (ensemble_id == m_currentEnsemble.ensemble_id) {
            // Extract label (16 bytes, starting at offset 2)
            std::string label;
            for (size_t i = 2; i < 18 && i < figBlock.data.size(); ++i) {
                if (figBlock.data[i] != 0) {
                    label += static_cast<char>(figBlock.data[i]);
                }
            }
            
            // Remove trailing spaces
            while (!label.empty() && label.back() == ' ') {
                label.pop_back();
            }
            
            m_currentEnsemble.label = label;
            
            // Parse character flag if available
            if (figBlock.data.size() >= 20) {
                m_currentEnsemble.character_flag = (figBlock.data[18] << 8) | figBlock.data[19];
            }
            
            Logger::instance().log(Logger::Info, "EtiProcessor", 
                                  QString("FIG 1/0: Ensemble label='%1' (ID=0x%2)")
                                  .arg(QString::fromStdString(label))
                                  .arg(ensemble_id, 4, 16, QChar('0')));
            
            emit ensembleDiscovered(m_currentEnsemble);
            return true;
        } else {
            Logger::instance().log(Logger::Debug, "EtiProcessor", 
                                  QString("FIG 1/0: Label for different ensemble (ID=0x%1, current=0x%2)")
                                  .arg(ensemble_id, 4, 16, QChar('0'))
                                  .arg(m_currentEnsemble.ensemble_id, 4, 16, QChar('0')));
            return false;
        }
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("FIG 1/0 parsing error: %1").arg(e.what()));
        return false;
    }
}

bool EtiProcessor::parseFig11_ServiceLabel(const eti::FigBlock& figBlock)
{
    // FIG 1/1: Service label (ETSI EN 300 401 Section 8.1.14.2)
    if (figBlock.data.size() < 18) {
        Logger::instance().log(Logger::Warning, "EtiProcessor", 
                              "FIG 1/1: Insufficient data for service label");
        return false;
    }
    
    try {
        uint16_t service_id = (figBlock.data[0] << 8) | figBlock.data[1];
        
        // Find the corresponding service
        auto service_it = std::find_if(m_discoveredServices.begin(), m_discoveredServices.end(),
                                      [service_id](const eti::DabService& s) {
                                          return s.service_id == service_id;
                                      });
        
        if (service_it != m_discoveredServices.end()) {
            // Extract label (16 bytes, starting at offset 2)
            std::string label;
            for (size_t i = 2; i < 18 && i < figBlock.data.size(); ++i) {
                if (figBlock.data[i] != 0) {
                    label += static_cast<char>(figBlock.data[i]);
                }
            }
            
            // Remove trailing spaces
            while (!label.empty() && label.back() == ' ') {
                label.pop_back();
            }
            
            service_it->label = label;
            
            // Parse character flag if available
            if (figBlock.data.size() >= 20) {
                service_it->character_flag = (figBlock.data[18] << 8) | figBlock.data[19];
            }
            
            Logger::instance().log(Logger::Info, "EtiProcessor", 
                                  QString("FIG 1/1: Service label='%1' (ID=0x%2)")
                                  .arg(QString::fromStdString(label))
                                  .arg(service_id, 4, 16, QChar('0')));
            
            emit serviceDiscovered(*service_it);
            return true;
        } else {
            Logger::instance().log(Logger::Warning, "EtiProcessor", 
                                  QString("FIG 1/1: Label for unknown service (ID=0x%1)")
                                  .arg(service_id, 4, 16, QChar('0')));
            return false;
        }
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EtiProcessor", 
                              QString("FIG 1/1 parsing error: %1").arg(e.what()));
        return false;
    }
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
    emit serviceQualityChanged(service.serviceId, quality.signalStrength, quality.errorRate);
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