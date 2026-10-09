/**
 * @file phase3a_integration.cpp
 * @brief Phase 3A Integration Manager Implementation
 *
 * Implementation of Phase 3A component integration with complete
 * signal/slot pipeline connections and unified processing interface.
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 * @version 1.0
 */

#include "phase3a_integration.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace eti::integration {

// ============================================================================
// Constructor & Destructor
// ============================================================================

Phase3AIntegrationManager::Phase3AIntegrationManager(QObject* parent)
    : QObject(parent)
    , m_etiParser(nullptr)
    , m_figAnalyser(nullptr)
    , m_audioValidator(nullptr)
    , m_motProtocol(nullptr)
    , m_epgDecoder(nullptr)
    , m_initialized(false)
    , m_processing(false)
    , m_current_frame_number(0)
{
    qDebug() << "[Phase3A Integration] Manager created";
}

Phase3AIntegrationManager::~Phase3AIntegrationManager()
{
    shutdownComponents();
    qDebug() << "[Phase3A Integration] Manager destroyed";
}

// ============================================================================
// Component Initialization & Management
// ============================================================================

bool Phase3AIntegrationManager::initializeComponents()
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        qWarning() << "[Phase3A Integration] Components already initialized";
        return true;
    }

    qInfo() << "[Phase3A Integration] Initializing all Phase 3A components...";

    try {
        // Create ETI Frame Parser
        m_etiParser = std::make_unique<ETIFrameParser>();
        if (!m_etiParser) {
            emit initializationError("ETIFrameParser", "Failed to create instance");
            return false;
        }
        qDebug() << "[Phase3A Integration] ✓ ETI Frame Parser created";

        // Create FIG Analyser
        m_figAnalyser = new AdvancedFIGAnalyser(this);
        if (!m_figAnalyser) {
            emit initializationError("AdvancedFIGAnalyser", "Failed to create instance");
            return false;
        }
        qDebug() << "[Phase3A Integration] ✓ FIG Analyser created";

        // Create Audio Validator
        m_audioValidator = new eti::audio::DABPlusStreamValidator(this);
        if (!m_audioValidator) {
            emit initializationError("DABPlusStreamValidator", "Failed to create instance");
            return false;
        }
        qDebug() << "[Phase3A Integration] ✓ Audio Validator created";

        // Create MOT Protocol
        m_motProtocol = new eti::mot::MOTProtocol(this);
        if (!m_motProtocol) {
            emit initializationError("MOTProtocol", "Failed to create instance");
            return false;
        }
        qDebug() << "[Phase3A Integration] ✓ MOT Protocol created";

        // Create EPG Decoder
        m_epgDecoder = new eti::epg::EPGDecoder(this);
        if (!m_epgDecoder) {
            emit initializationError("EPGDecoder", "Failed to create instance");
            return false;
        }
        qDebug() << "[Phase3A Integration] ✓ EPG Decoder created";

        // TODO: Create Journaline Decoder when available
        // m_journalineDecoder = new journaline::JournalineDecoder(this);

        // Phase 3B: Create DLS+ decoder (activated)
        m_dls_plus_decoder = new eti::dls_plus::DLSPlusDecoder(this);
        if (!m_dls_plus_decoder) {
            emit initializationError("DLSPlusDecoder", "Failed to create instance");
            return false;
        }
        qDebug() << "[Phase3A Integration] ✓ DLS+ Decoder created";

        // Connect all signal/slot pipelines
        if (!connectSignals()) {
            emit initializationError("SignalConnection", "Failed to connect signals");
            return false;
        }

        m_initialized = true;
        qInfo() << "[Phase3A Integration] All components initialized successfully";
        emit integrationReady();
        return true;

    } catch (const std::exception& e) {
        QString error = QString("Exception during initialization: %1").arg(e.what());
        qCritical() << "[Phase3A Integration]" << error;
        emit initializationError("Exception", error);
        return false;
    }
}

bool Phase3AIntegrationManager::connectSignals()
{
    qInfo() << "[Phase3A Integration] Connecting signal/slot pipelines...";

    // ========================================================================
    // Pipeline 1: FIG Analyser Signals → Integration Manager
    // ========================================================================

    // Service discovery
    connect(m_figAnalyser, &AdvancedFIGAnalyser::serviceDiscovered,
            this, [this](const ServiceInfo& service) {
                onServiceDiscovered(service.serviceId, service.label);
            });

    // Ensemble discovery (ensembleUpdated signal, not ensembleDiscovered)
    connect(m_figAnalyser, &AdvancedFIGAnalyser::ensembleUpdated,
            this, [this](const EnsembleInfo& ensemble) {
                onEnsembleDiscovered(static_cast<uint16_t>(ensemble.ensembleId), 
                                   ensemble.ensembleLabel);
            });

    qDebug() << "[Phase3A Integration] ✓ FIG Analyser → Integration Manager connected";

    // ========================================================================
    // Pipeline 2: Audio Validator Signals → Integration Manager
    // ========================================================================

    connect(m_audioValidator, &eti::audio::DABPlusStreamValidator::audioStreamDetected,
            this, &Phase3AIntegrationManager::onAudioStreamDetected);

    connect(m_audioValidator, &eti::audio::DABPlusStreamValidator::audioQualityWarning,
            this, &Phase3AIntegrationManager::onAudioQualityWarning);

    connect(m_audioValidator, &eti::audio::DABPlusStreamValidator::audioStreamError,
            this, [this](uint32_t service_id, const QString& error) {
                Q_UNUSED(service_id);
                m_statistics.audio_stream_errors++;
                m_statistics.total_errors++;
                emit integrationError(QString("Audio Stream Error: %1").arg(error));
            });

    qDebug() << "[Phase3A Integration] ✓ Audio Validator → Integration Manager connected";

    // ========================================================================
    // Pipeline 3: MOT Protocol Signals → Integration Manager
    // ========================================================================

    connect(m_motProtocol, &eti::mot::MOTProtocol::objectComplete,
            this, &Phase3AIntegrationManager::onMOTObjectComplete);

    connect(m_motProtocol, &eti::mot::MOTProtocol::parseError,
            this, &Phase3AIntegrationManager::onMOTParseError);

    qDebug() << "[Phase3A Integration] ✓ MOT Protocol → Integration Manager connected";

    // ========================================================================
    // Pipeline 4: EPG Decoder Signals → Integration Manager
    // ========================================================================

    connect(m_epgDecoder, &eti::epg::EPGDecoder::epgEventDiscovered,
            this, &Phase3AIntegrationManager::onEPGEventDiscovered);

    connect(m_epgDecoder, &eti::epg::EPGDecoder::epgScheduleUpdated,
            this, [this](uint32_t service_id) {
                Q_UNUSED(service_id);
                m_statistics.epg_schedules_updated++;
                updateStatistics();
            });

    connect(m_epgDecoder, &eti::epg::EPGDecoder::epgDecodingError,
            this, &Phase3AIntegrationManager::onEPGDecodingError);

    qDebug() << "[Phase3A Integration] ✓ EPG Decoder → Integration Manager connected";

    // ========================================================================
    // Pipeline 5: DLS+ Integration (Phase 3B - activated)
    // ========================================================================

    // Connect Audio Validator PAD → DLS+ Decoder (lambda adapts parameters)
    connect(m_audioValidator, &eti::audio::DABPlusStreamValidator::padDataExtracted,
            this, [this](uint32_t service_id, const QByteArray& pad_data) {
                Q_UNUSED(service_id); // processPADData doesn't need service_id
                m_dls_plus_decoder->processPADData(pad_data);
            });
    
    connect(m_dls_plus_decoder, &eti::dls_plus::DLSPlusDecoder::dlsPlusMessageReceived,
            this, [this](uint32_t service_id, const eti::dls_plus::DLSPlusMessage& msg) {
                emit dlsPlusMessageReceived(service_id, msg.getArtist(), msg.getItem());
            });
    
    qDebug() << "[Phase3A Integration] ✓ Audio Validator PAD → DLS+ Decoder connected";

    // ========================================================================
    // Pipeline 6: MOT → EPG Cross-Component Connection
    // ========================================================================

    // When MOT object completes, check if it's EPG data and forward to EPG decoder
    connect(m_motProtocol, &eti::mot::MOTProtocol::objectComplete,
            this, [this](uint32_t transport_id, const eti::mot::MOTObject& object) {
                Q_UNUSED(transport_id);
                
                // FIX MEDIUM-002: Validate object before pointer casting
                if (!object.isValid()) {
                    qWarning() << "[Phase3A Integration] Invalid MOT object, skipping EPG processing";
                    return;
                }
                
                if (object.body.empty()) {
                    qDebug() << "[Phase3A Integration] Empty MOT object body, skipping";
                    return;
                }
                
                // Safe conversion with validation
                // Check if MOT object contains EPG data (content type check)
                // EPG data typically has specific content types or UserApplicationType
                // For now, forward all MOT objects to EPG decoder - it will filter internally
                QByteArray mot_data(reinterpret_cast<const char*>(object.body.data()),
                                   static_cast<int>(object.body.size()));
                m_epgDecoder->processMOTObject(mot_data);
            });

    qDebug() << "[Phase3A Integration] ✓ MOT → EPG cross-component connected";

    // ========================================================================
    // Pipeline 7: TODO - MOT → Journaline (when available)
    // ========================================================================
    // TODO: Connect MOT objectComplete → Journaline decoder
    // connect(m_motProtocol, &eti::mot::MOTProtocol::objectComplete,
    //         m_journalineDecoder, &journaline::JournalineDecoder::processMOTObject);

    qInfo() << "[Phase3A Integration] All signal/slot pipelines connected successfully";
    qInfo() << "[Phase3A Integration] Note: DLS+ pipeline will be enabled when Agent 1 completes decoder";
    return true;
}

bool Phase3AIntegrationManager::isInitialized() const
{
    QMutexLocker locker(&m_mutex);
    return m_initialized;
}

void Phase3AIntegrationManager::shutdownComponents()
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        return;
    }

    qInfo() << "[Phase3A Integration] Shutting down components...";

    // Stop any active processing
    if (m_processing) {
        m_processing = false;
    }

    // Components are QObject children, will be deleted automatically
    // Just clear our pointers
    m_figAnalyser = nullptr;
    m_audioValidator = nullptr;
    m_motProtocol = nullptr;
    m_epgDecoder = nullptr;
    m_etiParser.reset();

    m_initialized = false;
    qInfo() << "[Phase3A Integration] Components shutdown complete";
}

// ============================================================================
// Processing Interface
// ============================================================================

bool Phase3AIntegrationManager::processETIFile(const QString& filePath)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        QString error = "Components not initialized. Call initializeComponents() first.";
        qCritical() << "[Phase3A Integration]" << error;
        emit integrationError(error);
        return false;
    }

    if (m_processing) {
        QString error = "Processing already in progress";
        qWarning() << "[Phase3A Integration]" << error;
        emit integrationWarning(error);
        return false;
    }

    // Verify file exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QString error = QString("File does not exist: %1").arg(filePath);
        qCritical() << "[Phase3A Integration]" << error;
        emit integrationError(error);
        return false;
    }

    if (!fileInfo.isReadable()) {
        QString error = QString("File is not readable: %1").arg(filePath);
        qCritical() << "[Phase3A Integration]" << error;
        emit integrationError(error);
        return false;
    }

    qInfo() << "[Phase3A Integration] Processing ETI file:" << filePath;
    qInfo() << "[Phase3A Integration] File size:" << fileInfo.size() << "bytes";

    // Open file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QString error = QString("Failed to open file: %1 - %2")
                        .arg(filePath)
                        .arg(file.errorString());
        qCritical() << "[Phase3A Integration]" << error;
        emit integrationError(error);
        return false;
    }

    m_processing = true;
    m_current_frame_number = 0;
    uint32_t total_frames = static_cast<uint32_t>(fileInfo.size() / 6144);

    qInfo() << "[Phase3A Integration] Total frames:" << total_frames;

    // Process frames
    while (!file.atEnd() && m_processing) {
        QByteArray frameData = file.read(6144);

        if (frameData.size() != 6144) {
            qWarning() << "[Phase3A Integration] Incomplete frame at offset"
                      << file.pos() - frameData.size()
                      << "- size:" << frameData.size();
            break;
        }

        // Process frame through pipeline
        if (!processETIFrame(frameData)) {
            qWarning() << "[Phase3A Integration] Frame processing failed at frame"
                      << m_current_frame_number;
            m_statistics.frames_invalid++;
        }

        m_current_frame_number++;

        // Emit progress every 100 frames
        if (m_current_frame_number % 100 == 0) {
            double progress = (static_cast<double>(m_current_frame_number) / total_frames) * 100.0;
            emit processingProgress(progress);
        }
    }

    file.close();
    m_processing = false;

    qInfo() << "[Phase3A Integration] Processing complete - Frames:" << m_current_frame_number;
    emit processingComplete(m_current_frame_number);

    return true;
}

bool Phase3AIntegrationManager::processETIFrame(const QByteArray& frameData)
{
    // Don't lock mutex here - called from processETIFile which already holds lock
    // or from external single-frame calls

    if (!m_initialized) {
        return false;
    }

    if (frameData.size() != 6144) {
        qWarning() << "[Phase3A Integration] Invalid frame size:" << frameData.size();
        return false;
    }

    // Parse ETI frame
    ETIFrameData parsedFrame;
    if (!m_etiParser->parseFrame(frameData, parsedFrame)) {
        m_statistics.frames_invalid++;
        return false;
    }

    m_statistics.frames_processed++;
    m_statistics.frames_valid++;

    // Emit frame processed signal
    emit frameProcessed(m_current_frame_number);

    // Process FIC data through FIG analyser
    if (!parsedFrame.fic_data.isEmpty()) {
        m_figAnalyser->analyzeFICData(parsedFrame.fic_data);
    }

    // Process MSC data through audio validator and MOT protocol
    // Note: We need service IDs from FIG analysis to properly route MSC data
    // For now, process all MSC data - components will filter internally

    for (const auto& subchannel : parsedFrame.sub_channels) {
        if (!subchannel.data.isEmpty()) {
            // Try audio validation (will skip if not audio)
            m_audioValidator->processAudioData(subchannel.sub_channel_id,
                                              subchannel.data);

            // NOTE (T26): MOT is NOT fed from raw sub-channel bytes. A MOT
            // object is carried in standard EN 300 401 §5.3.3 MSC data groups
            // assembled from the DAB+ X-PAD CI 12/13 sub-fields (see
            // DABAnalyserWindow's PAD parser). Feeding `subchannel.data` (raw
            // sub-channel bytes) to MOTProtocol::processMOTData() can never
            // parse, so this hook is intentionally removed. Any future MSC
            // data-service path must assemble the data group first, then call
            // m_motProtocol->processMOTData(serviceId, dataGroup).
        }
    }

    updateStatistics();
    return true;
}

void Phase3AIntegrationManager::stopProcessing()
{
    QMutexLocker locker(&m_mutex);
    if (m_processing) {
        qInfo() << "[Phase3A Integration] Stop requested";
        m_processing = false;
    }
}

bool Phase3AIntegrationManager::isProcessing() const
{
    QMutexLocker locker(&m_mutex);
    return m_processing;
}

// ============================================================================
// Statistics & Status
// ============================================================================

Phase3AIntegrationManager::IntegrationStatistics
Phase3AIntegrationManager::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    return m_statistics;
}

void Phase3AIntegrationManager::resetStatistics()
{
    QMutexLocker locker(&m_mutex);
    m_statistics = IntegrationStatistics();
    m_current_frame_number = 0;
    qDebug() << "[Phase3A Integration] Statistics reset";
}

uint32_t Phase3AIntegrationManager::framesProcessed() const
{
    QMutexLocker locker(&m_mutex);
    return m_statistics.frames_processed;
}

uint32_t Phase3AIntegrationManager::servicesDiscovered() const
{
    QMutexLocker locker(&m_mutex);
    return m_statistics.services_discovered;
}

void Phase3AIntegrationManager::updateStatistics()
{
    // Update aggregated statistics from all components
    // Note: Component-specific stats are updated via signal handlers

    if (m_audioValidator) {
        auto audio_stats = m_audioValidator->getStatistics();
        m_statistics.audio_streams_validated = static_cast<uint32_t>(
            audio_stats.superframes_valid);
    }

    if (m_motProtocol) {
        auto mot_stats = m_motProtocol->getStatistics();
        m_statistics.mot_objects_complete = static_cast<uint32_t>(
            mot_stats.objects_completed);
        m_statistics.mot_reassembly_errors = static_cast<uint32_t>(
            mot_stats.crc_errors);
    }

    if (m_epgDecoder) {
        auto epg_stats = m_epgDecoder->getStatistics();
        m_statistics.epg_events_parsed = epg_stats.events_extracted;
    }

    emit statisticsUpdated();
}

QString Phase3AIntegrationManager::IntegrationStatistics::toString() const
{
    QString stats;
    stats += QString("=== Phase 3A Integration Statistics ===\n");
    stats += QString("Frames: %1 processed (%2 valid, %3 invalid)\n")
             .arg(frames_processed).arg(frames_valid).arg(frames_invalid);
    stats += QString("Services: %1 discovered, %2 ensembles\n")
             .arg(services_discovered).arg(ensembles_discovered);
    stats += QString("Audio: %1 streams validated, %2 warnings, %3 errors\n")
             .arg(audio_streams_validated).arg(audio_quality_warnings)
             .arg(audio_stream_errors);
    stats += QString("MOT: %1 objects extracted (%2 complete), %3 errors\n")
             .arg(mot_objects_extracted).arg(mot_objects_complete)
             .arg(mot_reassembly_errors);
    stats += QString("EPG: %1 events parsed, %2 schedules updated\n")
             .arg(epg_events_parsed).arg(epg_schedules_updated);
    stats += QString("Errors: %1 total, %2 warnings\n")
             .arg(total_errors).arg(total_warnings);
    return stats;
}

// ============================================================================
// Private Slots - Signal Handlers
// ============================================================================

void Phase3AIntegrationManager::onFrameProcessed(const ::ProcessedFrame& frame)
{
    Q_UNUSED(frame);
    // Frame processing handled in processETIFrame
}

void Phase3AIntegrationManager::onServiceDiscovered(uint32_t service_id,
                                                   const QString& label)
{
    m_statistics.services_discovered++;
    m_statistics.fig_types_decoded++;
    
    qInfo() << "[Phase3A Integration] Service discovered:"
            << "SId=" << QString::number(service_id, 16).toUpper()
            << "Label=" << label;

    emit serviceDiscovered(service_id, label);
    updateStatistics();
}

void Phase3AIntegrationManager::onEnsembleDiscovered(uint16_t ensemble_id,
                                                     const QString& label)
{
    m_statistics.ensembles_discovered++;
    m_statistics.fig_types_decoded++;

    qInfo() << "[Phase3A Integration] Ensemble discovered:"
            << "EId=" << QString::number(ensemble_id, 16).toUpper()
            << "Label=" << label;

    emit ensembleDiscovered(ensemble_id, label);
    updateStatistics();
}

void Phase3AIntegrationManager::onAudioStreamDetected(
    uint32_t service_id,
    const eti::audio::AudioQualityMetrics& metrics)
{
    m_statistics.audio_streams_validated++;

    qInfo() << "[Phase3A Integration] Audio stream detected:"
            << "SId=" << QString::number(service_id, 16).toUpper()
            << "Bitrate=" << metrics.bitrate_kbps << "kbps"
            << "Profile=" << metrics.getProfileString();

    emit audioStreamValidated(service_id, metrics.bitrate_kbps,
                             metrics.getProfileString());
    updateStatistics();
}

void Phase3AIntegrationManager::onAudioQualityWarning(uint32_t service_id,
                                                      const QString& warning)
{
    m_statistics.audio_quality_warnings++;
    m_statistics.total_warnings++;

    qWarning() << "[Phase3A Integration] Audio quality warning:"
               << "SId=" << QString::number(service_id, 16).toUpper()
               << warning;

    emit integrationWarning(QString("Audio: %1").arg(warning));
    updateStatistics();
}

void Phase3AIntegrationManager::onMOTObjectComplete(
    uint32_t transport_id,
    const eti::mot::MOTObject& object)
{
    m_statistics.mot_objects_extracted++;
    m_statistics.mot_objects_complete++;

    QString content_type = object.header.getContentTypeString();

    qInfo() << "[Phase3A Integration] MOT object complete:"
            << "TransportID=" << transport_id
            << "Type=" << content_type
            << "Size=" << object.body.size() << "bytes";

    emit motObjectExtracted(transport_id, content_type);
    updateStatistics();
}

void Phase3AIntegrationManager::onMOTParseError(uint32_t transport_id,
                                               const QString& error)
{
    m_statistics.mot_reassembly_errors++;
    m_statistics.total_errors++;

    qWarning() << "[Phase3A Integration] MOT parse error:"
               << "TransportID=" << transport_id
               << error;

    emit integrationError(QString("MOT: %1").arg(error));
    updateStatistics();
}

void Phase3AIntegrationManager::onEPGEventDiscovered(
    uint32_t service_id,
    const eti::epg::EPGEvent& event)
{
    m_statistics.epg_events_parsed++;

    qInfo() << "[Phase3A Integration] EPG event discovered:"
            << "SId=" << QString::number(service_id, 16).toUpper()
            << "Program=" << event.program_name
            << "Start=" << event.start_time.toString();

    emit epgEventParsed(service_id, event.program_name);
    updateStatistics();
}

void Phase3AIntegrationManager::onEPGDecodingError(const QString& error)
{
    m_statistics.total_errors++;

    qWarning() << "[Phase3A Integration] EPG decoding error:" << error;
    emit integrationError(QString("EPG: %1").arg(error));
    updateStatistics();
}

} // namespace eti::integration
