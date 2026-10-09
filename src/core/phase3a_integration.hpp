/**
 * @file phase3a_integration.hpp
 * @brief Phase 3A Integration Manager - Audio & Data Services Component Integration
 *
 * Provides unified integration layer connecting all Phase 3A components:
 * - ETI Frame Parser (core ETI processing)
 * - Advanced FIG Analyser (service discovery)
 * - DAB+ Stream Validator (audio quality analysis)
 * - MOT Protocol (multimedia object transfer)
 * - EPG Decoder (electronic program guide)
 * - Journaline Decoder (news service - when available)
 *
 * This integration manager establishes complete signal/slot pipelines for:
 * 1. ETI → FIG → Service Discovery
 * 2. ETI → MSC → Audio Validation
 * 3. ETI → MSC → MOT Extraction
 * 4. MOT → EPG Decoding
 * 5. MOT → Journaline Decoding
 *
 * Features:
 * - Automated signal/slot connection management
 * - Component lifecycle management (initialization, cleanup)
 * - Unified processing interface for ETI files and frames
 * - Comprehensive statistics aggregation across components
 * - Thread-safe operation with Qt signal/slot threading
 * - Professional error handling and recovery
 *
 * Phase 3A Wave 3.1: Component Integration (Day 1-2)
 * Implementation Status: Complete integration layer with all pipelines
 *
 * @see ETSI EN 300 799 - ETI Frame Structure
 * @see ETSI EN 300 401 - DAB System Specification
 * @see ETSI TS 102 563 - DAB+ Audio Coding
 * @see ETSI EN 301 234 - MOT Protocol
 * @see ETSI TS 102 371 - EPG Specification
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 * @version 1.0
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>
#include <memory>
#include <cstdint>
#include <vector>

// Phase 3A Component Headers
#include "enhanced_eti_processor_qt.h"
#include "advanced_fig_analyser.h"
#include "dabplus_stream_validator.hpp"
#include "mot_protocol.hpp"
#include "epg_decoder.hpp"
// TODO: Add journaline_decoder.hpp when available from parallel agent

// Phase 3B: DLS+ decoder integration (activated)
#include "dls_plus_decoder.hpp"

namespace eti::integration {

/**
 * @brief Phase 3A Integration Manager
 *
 * Central integration point for all Phase 3A audio and data service components.
 * Manages component initialization, signal/slot connections, and provides
 * unified processing interface for the entire Phase 3A pipeline.
 *
 * Architecture:
 * ```
 * ETI File/Stream
 *       ↓
 * ETIFrameParser → FIC Data → AdvancedFIGAnalyser → Service Discovery
 *       ↓
 *       └→ MSC Data → DABPlusStreamValidator → Audio Quality
 *       └→ MSC Data → MOTProtocol → MOT Objects
 *                            ↓
 *                            ├→ EPGDecoder → Program Guide
 *                            └→ JournalineDecoder → News (future)
 * ```
 *
 * Thread Safety:
 * - All public methods are thread-safe (mutex protected)
 * - Signal/slot connections use Qt::AutoConnection (automatic threading)
 * - Components may run in separate threads via Qt event loop
 *
 * Usage:
 * @code
 *   Phase3AIntegrationManager manager;
 *   manager.initializeComponents();
 *
 *   // Connect to aggregated signals
 *   connect(&manager, &Phase3AIntegrationManager::serviceDiscovered,
 *           this, &MyClass::handleService);
 *
 *   // Process ETI file
 *   manager.processETIFile("/path/to/stream.eti");
 *
 *   // Get statistics
 *   auto stats = manager.getStatistics();
 *   qInfo() << "Services:" << stats.services_discovered;
 * @endcode
 */
class Phase3AIntegrationManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(uint32_t framesProcessed READ framesProcessed NOTIFY statisticsUpdated)
    Q_PROPERTY(uint32_t servicesDiscovered READ servicesDiscovered NOTIFY statisticsUpdated)

public:
    explicit Phase3AIntegrationManager(QObject* parent = nullptr);
    ~Phase3AIntegrationManager() override;

    // ========================================================================
    // Component Initialization & Management
    // ========================================================================

    /**
     * @brief Initialize all Phase 3A components
     * @return true if all components initialized successfully
     *
     * Creates component instances and establishes all signal/slot connections:
     * - ETI → FIG pipeline
     * - ETI → Audio validation pipeline
     * - ETI → MOT extraction pipeline
     * - MOT → EPG decoding pipeline
     * - MOT → Journaline decoding pipeline (when available)
     *
     * Must be called before processETIFile() or processETIFrame().
     */
    bool initializeComponents();

    /**
     * @brief Connect all signal/slot pipelines
     * @return true if all connections successful
     *
     * Establishes inter-component signal/slot connections:
     * - Data flow connections (ETI → FIG → Services → Audio/Data)
     * - Error propagation connections
     * - Statistics aggregation connections
     *
     * Called automatically by initializeComponents().
     */
    bool connectSignals();

    /**
     * @brief Check if components are initialized
     */
    bool isInitialized() const;

    /**
     * @brief Shutdown and cleanup all components
     */
    void shutdownComponents();

    // ========================================================================
    // Component Access (for advanced use cases)
    // ========================================================================

    ETIFrameParser* etiParser() const { return m_etiParser.get(); }
    AdvancedFIGAnalyser* figAnalyser() const { return m_figAnalyser; }
    eti::audio::DABPlusStreamValidator* audioValidator() const { return m_audioValidator; }
    eti::mot::MOTProtocol* motProtocol() const { return m_motProtocol; }
    eti::epg::EPGDecoder* epgDecoder() const { return m_epgDecoder; }
    // TODO: Add journalineDecoder() getter when available
    
    // Phase 3B: DLS+ decoder accessor
    eti::dls_plus::DLSPlusDecoder* dlsPlusDecoder() const { return m_dls_plus_decoder; }

    // ========================================================================
    // Processing Interface
    // ========================================================================

    /**
     * @brief Process ETI file through entire Phase 3A pipeline
     * @param filePath Absolute path to ETI file
     * @return true if processing started successfully
     *
     * Processes ETI file frame-by-frame through complete pipeline:
     * 1. Parse ETI frames
     * 2. Extract and analyze FIC data (service discovery)
     * 3. Extract and validate audio streams
     * 4. Extract MOT objects (SlideShow, EPG, Journaline)
     * 5. Decode EPG and Journaline data
     *
     * Progress emitted via frameProcessed signal.
     */
    bool processETIFile(const QString& filePath);

    /**
     * @brief Process single ETI frame through pipeline
     * @param frameData 6144-byte ETI frame
     * @return true if processing successful
     *
     * Single-frame processing for real-time streams or selective processing.
     */
    bool processETIFrame(const QByteArray& frameData);

    /**
     * @brief Stop current processing operation
     */
    void stopProcessing();

    /**
     * @brief Check if processing is active
     */
    bool isProcessing() const;

    // ========================================================================
    // Statistics & Status
    // ========================================================================

    /**
     * @brief Aggregated statistics from all Phase 3A components
     */
    struct IntegrationStatistics {
        // ETI Frame Processing
        uint32_t frames_processed = 0;
        uint32_t frames_valid = 0;
        uint32_t frames_invalid = 0;

        // Service Discovery (FIG)
        uint32_t services_discovered = 0;
        uint32_t ensembles_discovered = 0;
        uint32_t fig_types_decoded = 0;

        // Audio Validation (DAB+)
        uint32_t audio_streams_validated = 0;
        uint32_t audio_quality_warnings = 0;
        uint32_t audio_stream_errors = 0;

        // MOT Protocol
        uint32_t mot_objects_extracted = 0;
        uint32_t mot_objects_complete = 0;
        uint32_t mot_reassembly_errors = 0;

        // EPG Decoding
        uint32_t epg_events_parsed = 0;
        uint32_t epg_schedules_updated = 0;

        // Journaline Decoding (future)
        uint32_t journaline_objects_parsed = 0;

        // Error tracking
        uint32_t total_errors = 0;
        uint32_t total_warnings = 0;

        QString toString() const;
    };

    /**
     * @brief Get aggregated statistics
     * @return Current statistics from all components
     */
    IntegrationStatistics getStatistics() const;

    /**
     * @brief Reset all statistics counters
     */
    void resetStatistics();

    // ========================================================================
    // Property Accessors (for QML/Qt Property System)
    // ========================================================================

    uint32_t framesProcessed() const;
    uint32_t servicesDiscovered() const;

signals:
    // ========================================================================
    // Lifecycle Signals
    // ========================================================================

    /**
     * @brief Emitted when all components initialized successfully
     */
    void integrationReady();

    /**
     * @brief Emitted when component initialization fails
     * @param component Component name that failed
     * @param error Error message
     */
    void initializationError(const QString& component, const QString& error);

    // ========================================================================
    // Processing Progress Signals
    // ========================================================================

    /**
     * @brief Emitted when ETI frame is processed
     * @param frame_number Frame sequence number
     */
    void frameProcessed(uint32_t frame_number);

    /**
     * @brief Emitted periodically with processing progress
     * @param progress Progress percentage (0.0 - 100.0)
     */
    void processingProgress(double progress);

    /**
     * @brief Emitted when file processing completes
     * @param frames_processed Total frames processed
     */
    void processingComplete(uint32_t frames_processed);

    // ========================================================================
    // Service Discovery Signals (aggregated from FIG analyser)
    // ========================================================================

    /**
     * @brief Emitted when DAB service is discovered
     * @param service_id Service ID (SId)
     * @param service_name Service label
     */
    void serviceDiscovered(uint32_t service_id, const QString& service_name);

    /**
     * @brief Emitted when ensemble information discovered
     * @param ensemble_id Ensemble ID (EId)
     * @param ensemble_name Ensemble label
     */
    void ensembleDiscovered(uint16_t ensemble_id, const QString& ensemble_name);

    // ========================================================================
    // Audio Stream Signals (aggregated from audio validator)
    // ========================================================================

    /**
     * @brief Emitted when audio stream detected and validated
     * @param service_id Service ID
     * @param bitrate Audio bitrate in kbps
     * @param profile Audio profile (HE-AAC v2, HE-AAC, AAC-LC)
     */
    void audioStreamValidated(uint32_t service_id, uint16_t bitrate, const QString& profile);

    // ========================================================================
    // Data Service Signals (aggregated from MOT/EPG/Journaline)
    // ========================================================================

    /**
     * @brief Emitted when MOT object extracted
     * @param transport_id MOT transport ID
     * @param content_type Content type (JPEG, PNG, HTML, etc.)
     */
    void motObjectExtracted(uint32_t transport_id, const QString& content_type);

    /**
     * @brief Emitted when EPG event parsed
     * @param service_id Service ID
     * @param program_name Program name
     */
    void epgEventParsed(uint32_t service_id, const QString& program_name);

    // Phase 3B: DLS+ signals (activated)
    /**
     * @brief Emitted when DLS+ message is received
     * @param service_id Service ID
     * @param artist Artist name
     * @param title Track title
     */
    void dlsPlusMessageReceived(uint32_t service_id, const QString& artist, const QString& title);

    // ========================================================================
    // Error & Warning Signals (aggregated)
    // ========================================================================

    /**
     * @brief Emitted on integration error
     * @param error Error message
     */
    void integrationError(const QString& error);

    /**
     * @brief Emitted on integration warning
     * @param warning Warning message
     */
    void integrationWarning(const QString& warning);

    /**
     * @brief Emitted when statistics change
     */
    void statisticsUpdated();

private slots:
    // ========================================================================
    // ETI Frame Parser Slots
    // ========================================================================

    /**
     * @brief Handle processed ETI frame
     * @param frame Processed frame data
     */
    void onFrameProcessed(const ::ProcessedFrame& frame);

    // ========================================================================
    // FIG Analyser Slots
    // ========================================================================

    /**
     * @brief Handle discovered service
     * @param service_id Service ID
     * @param label Service label
     */
    void onServiceDiscovered(uint32_t service_id, const QString& label);

    /**
     * @brief Handle discovered ensemble
     * @param ensemble_id Ensemble ID
     * @param label Ensemble label
     */
    void onEnsembleDiscovered(uint16_t ensemble_id, const QString& label);

    // ========================================================================
    // Audio Validator Slots
    // ========================================================================

    /**
     * @brief Handle detected audio stream
     * @param service_id Service ID
     * @param metrics Audio quality metrics
     */
    void onAudioStreamDetected(uint32_t service_id,
                               const eti::audio::AudioQualityMetrics& metrics);

    /**
     * @brief Handle audio quality warning
     * @param service_id Service ID
     * @param warning Warning message
     */
    void onAudioQualityWarning(uint32_t service_id, const QString& warning);

    // ========================================================================
    // MOT Protocol Slots
    // ========================================================================

    /**
     * @brief Handle completed MOT object
     * @param transport_id Transport ID
     * @param object Complete MOT object
     */
    void onMOTObjectComplete(uint32_t transport_id,
                             const eti::mot::MOTObject& object);

    /**
     * @brief Handle MOT parsing error
     * @param transport_id Transport ID
     * @param error Error message
     */
    void onMOTParseError(uint32_t transport_id, const QString& error);

    // ========================================================================
    // EPG Decoder Slots
    // ========================================================================

    /**
     * @brief Handle discovered EPG event
     * @param service_id Service ID
     * @param event EPG event
     */
    void onEPGEventDiscovered(uint32_t service_id,
                              const eti::epg::EPGEvent& event);

    /**
     * @brief Handle EPG decoding error
     * @param error Error message
     */
    void onEPGDecodingError(const QString& error);

private:
    // ========================================================================
    // Component Instances
    // ========================================================================

    std::unique_ptr<ETIFrameParser> m_etiParser;
    AdvancedFIGAnalyser* m_figAnalyser;
    eti::audio::DABPlusStreamValidator* m_audioValidator;
    eti::mot::MOTProtocol* m_motProtocol;
    eti::epg::EPGDecoder* m_epgDecoder;
    // TODO: Add JournalineDecoder* m_journalineDecoder when available
    
    // Phase 3B: DLS+ decoder (activated)
    eti::dls_plus::DLSPlusDecoder* m_dls_plus_decoder;

    // ========================================================================
    // State Management
    // ========================================================================

    bool m_initialized;
    bool m_processing;
    uint32_t m_current_frame_number;

    // ========================================================================
    // Statistics
    // ========================================================================

    IntegrationStatistics m_statistics;

    // ========================================================================
    // Thread Safety
    // ========================================================================

    mutable QMutex m_mutex;

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Update aggregated statistics from all components
     */
    void updateStatistics();

    /**
     * @brief Route MSC data to appropriate handlers
     * @param msc_data MSC data from ETI frame
     * @param service_id Associated service ID
     */
    void routeMSCData(const QByteArray& msc_data, uint32_t service_id);
};

} // namespace eti::integration
