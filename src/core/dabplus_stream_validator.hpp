/**
 * @file dabplus_stream_validator.hpp
 * @brief DAB+ Audio Stream Validation (ETSI TS 102 563 Compliance)
 *
 * Implements DAB+ audio superframe validation including:
 * - Audio superframe parsing (120ms superframes)
 * - FireCode CRC validation (16-bit inner coding)
 * - Reed-Solomon FEC validation (RS(120,110) and RS(120,96))
 * - Audio quality metrics extraction (bitrate, protection level, profile)
 * - HE-AAC v2 profile detection (SBR + PS flags)
 *
 * IMPORTANT: This is STREAM VALIDATION only, NOT audio decoding.
 * No PCM decoding, no audio playback - only superframe structure validation
 * and quality metrics extraction per ETSI TS 102 563 specification.
 *
 * Features:
 * - ETSI TS 102 563 compliant superframe parsing
 * - FireCode polynomial validation (16-bit CRC)
 * - Reed-Solomon outer coding validation
 * - Protection level detection (UEP 1-5)
 * - Bitrate range validation (8-192 kbps)
 * - HE-AAC v2 detection (SBR + PS flags)
 * - Professional error handling with detailed logging
 * - Qt signal emission for GUI integration
 *
 * @see ETSI TS 102 563 - DAB+ Audio Coding (HE-AAC v2)
 * @see ETSI EN 300 401 - DAB Radio Broadcasting System
 *
 * @author StreamDAB Development Team
 * @date October 20, 2025
 * @version 1.0
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <vector>
#include <cstdint>
#include <array>
#include <optional>
#include <memory>
#include <map>

// Phase 3B: DLS+ decoder integration (Agent 1 complete)
#include "dls_plus_decoder.hpp"

namespace eti::audio {

/**
 * @brief Reed-Solomon coding structure for DAB+ superframes
 *
 * ETSI TS 102 563 Section 6.1: Outer Coding
 * - RS(120, 110): 110 data bytes, 10 parity bytes (UEP-1/2)
 * - RS(120, 96): 96 data bytes, 24 parity bytes (UEP-3/4/5)
 */
struct RSCoding {
    uint8_t rs_k = 0;              ///< Data bytes (110 or 96)
    uint8_t rs_z = 0;              ///< Parity bytes (10 or 24)
    bool valid_crc = false;        ///< RS validation result

    /**
     * @brief Check if RS profile is valid per ETSI TS 102 563
     */
    bool isValidProfile() const {
        return (rs_k == 110 && rs_z == 10) ||  // RS(120,110) - UEP 1/2
               (rs_k == 96 && rs_z == 24);      // RS(120,96) - UEP 3/4/5
    }

    /**
     * @brief Get total RS codeword length
     */
    uint16_t getTotalLength() const {
        return static_cast<uint16_t>(rs_k + rs_z);
    }

    QString toString() const;
};

/**
 * @brief Audio Access Unit structure
 *
 * ETSI TS 102 563 Section 5: Audio Access Units within superframe
 * Each superframe contains 1-4 Audio Access Units (AAC frames)
 */
struct AudioAccessUnit {
    uint16_t au_size = 0;          ///< AU size in bytes
    std::vector<uint8_t> au_data;  ///< Raw AAC frame data

    bool isEmpty() const {
        return au_size == 0 || au_data.empty();
    }

    bool isValid() const {
        return au_size > 0 && au_data.size() == au_size;
    }
};

/**
 * @brief DAB+ Audio Superframe structure (120ms duration)
 *
 * ETSI TS 102 563 Section 4: Audio Superframe Structure
 *
 * Superframe Components:
 * - FireCode Header (16-bit CRC for header protection)
 * - Number of Audio Access Units (1-4)
 * - SBR flag (Spectral Band Replication)
 * - PS flag (Parametric Stereo)
 * - DAC rate index (sample rate)
 * - Reed-Solomon FEC (outer coding)
 * - Audio Access Units (AAC frames)
 */
struct AudioSuperframe {
    // Superframe Header
    uint16_t firecode_status = 0;           ///< FireCode CRC validation result
    uint8_t num_aus = 0;                    ///< Number of Audio Access Units (1-4)
    bool sbr_flag = false;                  ///< Spectral Band Replication enabled
    bool ps_flag = false;                   ///< Parametric Stereo enabled
    uint8_t dac_rate = 0;                   ///< DAC rate index (0=48kHz, 1=24kHz)

    // Reed-Solomon FEC
    RSCoding rs_coding;                     ///< RS outer coding parameters

    // Audio Access Units
    std::vector<AudioAccessUnit> access_units; ///< AAC frames (1-4 AUs)

    // Protection Profile
    uint8_t protection_level = 0;           ///< UEP protection level (1-5)
    uint16_t bitrate_kbps = 0;              ///< Audio bitrate in kbps

    // Superframe metadata
    QDateTime received_time;                ///< Reception timestamp
    bool is_valid = false;                  ///< Overall superframe validity

    /**
     * @brief Get sample rate from DAC rate index
     */
    uint32_t getSampleRate() const {
        return (dac_rate == 0) ? 48000 : 24000;
    }

    /**
     * @brief Check if HE-AAC v2 profile (SBR + PS)
     */
    bool isHEAACv2() const {
        return sbr_flag && ps_flag;
    }

    /**
     * @brief Get AAC profile string
     */
    QString getProfile() const {
        if (sbr_flag && ps_flag) return "HE-AAC v2";
        if (sbr_flag) return "HE-AAC";
        return "AAC-LC";
    }

    QString toString() const;
};

/**
 * @brief Audio quality metrics (extracted from superframe, no decoding required)
 *
 * Quality metrics derived from superframe header and protection parameters
 * without performing actual PCM audio decoding.
 */
struct AudioQualityMetrics {
    uint16_t bitrate_kbps = 0;      ///< Audio bitrate (8-192 kbps)
    uint8_t sample_rate_khz = 0;    ///< Sample rate (24 or 48 kHz)
    uint8_t channels = 0;           ///< Number of channels (1=mono, 2=stereo)
    bool is_stereo_ps = false;      ///< Parametric Stereo active
    bool is_heaac_v2 = false;       ///< HE-AAC v2 profile (SBR+PS)
    uint8_t protection_level = 0;   ///< UEP protection level (1-5)
    double quality_estimate = 0.0;  ///< Quality estimate 0-100 based on bitrate/protection

    // FEC statistics
    uint32_t firecode_errors = 0;       ///< FireCode validation failures
    uint32_t reed_solomon_errors = 0;   ///< Reed-Solomon validation failures

    QString getProfileString() const {
        if (is_heaac_v2) return "HE-AAC v2";
        if (is_stereo_ps) return "HE-AAC";
        return "AAC-LC";
    }

    QString toString() const;
};

/**
 * @brief Validation result structure
 *
 * Contains validation status, error messages, and warnings
 * for comprehensive stream analysis reporting.
 */
struct ValidationResult {
    bool is_valid = false;                      ///< Overall validation result
    std::vector<QString> errors;                ///< Validation errors
    std::vector<QString> warnings;              ///< Validation warnings
    QString error_summary;                      ///< Summary of errors

    void addError(const QString& error) {
        errors.push_back(error);
        is_valid = false;
    }

    void addWarning(const QString& warning) {
        warnings.push_back(warning);
    }

    bool hasErrors() const {
        return !errors.empty();
    }

    bool hasWarnings() const {
        return !warnings.empty();
    }

    QString getSummary() const;
};

/**
 * @brief DAB+ Stream Validator Class
 *
 * Professional DAB+ audio stream validation per ETSI TS 102 563.
 * Validates superframe structure, FEC coding, and extracts quality metrics
 * without performing actual audio decoding.
 *
 * Thread-safe for real-time processing with Qt signal/slot integration.
 *
 * Usage:
 * @code
 *   DABPlusStreamValidator validator;
 *
 *   // Process MSC data for a service
 *   if (validator.processAudioData(service_id, msc_data)) {
 *       AudioQualityMetrics metrics = validator.getMetrics(service_id);
 *       qInfo() << "Bitrate:" << metrics.bitrate_kbps << "kbps";
 *       qInfo() << "Profile:" << metrics.getProfileString();
 *   }
 * @endcode
 */
class DABPlusStreamValidator : public QObject {
    Q_OBJECT

public:
    explicit DABPlusStreamValidator(QObject* parent = nullptr);
    ~DABPlusStreamValidator() override;

    // ========================================================================
    // Main Processing Interface
    // ========================================================================

    /**
     * @brief Process audio data from MSC (Main Service Channel)
     * @param service_id DAB service ID (SID)
     * @param msc_data Raw MSC data containing audio superframe
     * @return true if processing successful, false on error
     *
     * Extracts and validates audio superframe from MSC data stream.
     * Emits audioStreamDetected signal on successful validation.
     */
    bool processAudioData(uint32_t service_id, const QByteArray& msc_data);

    /**
     * @brief Parse audio superframe from MSC data
     * @param msc_data Raw MSC data (120ms superframe)
     * @param superframe Output: parsed superframe structure
     * @return Validation result with errors/warnings
     *
     * ETSI TS 102 563 Section 4: Superframe Structure Parsing
     */
    ValidationResult parseAudioSuperframe(const QByteArray& msc_data,
                                          AudioSuperframe& superframe);

    // ========================================================================
    // Validation Functions (ETSI TS 102 563)
    // ========================================================================

    /**
     * @brief Validate FireCode CRC (16-bit inner coding)
     * @param sf_data Superframe data
     * @return Validation result
     *
     * ETSI TS 102 563 Section 6.2: FireCode Inner Coding
     * FireCode polynomial: g(x) = x^16 + x^14 + x^13 + x^11 + x^10 +
     *                             x^8 + x^6 + x^5 + x^2 + x + 1
     */
    ValidationResult validateFireCode(const std::vector<uint8_t>& sf_data);

    /**
     * @brief Validate Reed-Solomon outer coding
     * @param rs Reed-Solomon coding structure
     * @return Validation result
     *
     * ETSI TS 102 563 Section 6.1: Outer Coding
     * - RS(120, 110): UEP-1/2 protection
     * - RS(120, 96): UEP-3/4/5 protection
     */
    ValidationResult validateReedSolomon(const RSCoding& rs);

    /**
     * @brief Validate Audio Access Unit structure
     * @param aus Vector of Audio Access Units
     * @return Validation result
     *
     * Validates AU count (1-4), sizes, and structure per ETSI TS 102 563.
     */
    ValidationResult validateAUStructure(const std::vector<AudioAccessUnit>& aus);

    // ========================================================================
    // Quality Metrics Extraction (No Decoding)
    // ========================================================================

    /**
     * @brief Extract audio quality metrics from superframe
     * @param superframe Parsed audio superframe
     * @return Audio quality metrics (bitrate, profile, protection level)
     *
     * Extracts quality metrics from superframe header without PCM decoding:
     * - Bitrate from superframe header
     * - Sample rate from DAC rate index
     * - Channel configuration from SBR/PS flags
     * - HE-AAC v2 detection (SBR + PS)
     * - Quality estimate based on bitrate and protection level
     */
    AudioQualityMetrics extractAudioMetrics(const AudioSuperframe& superframe);

    /**
     * @brief Detect AAC profile from superframe flags
     * @param superframe Audio superframe
     * @return Profile string: "HE-AAC v2", "HE-AAC", or "AAC-LC"
     *
     * Profile detection:
     * - HE-AAC v2: SBR=1, PS=1
     * - HE-AAC: SBR=1, PS=0
     * - AAC-LC: SBR=0, PS=0
     */
    QString detectAACProfile(const AudioSuperframe& superframe);

    // ========================================================================
    // Bitrate & Protection Compliance
    // ========================================================================

    /**
     * @brief Check if bitrate is valid for protection level
     * @param bitrate_kbps Bitrate in kbps
     * @param protection_level UEP protection level (1-5)
     * @return true if valid per ETSI TS 102 563 Table 10
     *
     * Valid bitrate range: 8-192 kbps
     * Protection level constraints apply.
     */
    bool isValidBitrate(uint16_t bitrate_kbps, uint8_t protection_level);

    /**
     * @brief Check if protection profile is valid
     * @param level UEP protection level
     * @return true if level is 1-5
     *
     * ETSI TS 102 563: UEP levels 1-5 supported
     */
    bool isValidProtectionProfile(uint8_t level);

    // ========================================================================
    // Status & Statistics
    // ========================================================================

    /**
     * @brief Get audio quality metrics for a service
     * @param service_id DAB service ID
     * @return Optional metrics (nullopt if service not found)
     */
    std::optional<AudioQualityMetrics> getMetrics(uint32_t service_id) const;

    /**
     * @brief Get last validated superframe for a service
     * @param service_id DAB service ID
     * @return Optional superframe (nullopt if service not found)
     */
    std::optional<AudioSuperframe> getLastSuperframe(uint32_t service_id) const;

    /**
     * @brief Clear all cached data
     */
    void clear();

    /**
     * @brief Get validation statistics
     */
    struct Statistics {
        uint64_t superframes_processed = 0;
        uint64_t superframes_valid = 0;
        uint64_t superframes_invalid = 0;
        uint64_t firecode_errors = 0;
        uint64_t reed_solomon_errors = 0;
        uint64_t au_structure_errors = 0;
    };
    Statistics getStatistics() const;

signals:
    /**
     * @brief Emitted when audio stream is detected and validated
     * @param service_id DAB service ID
     * @param metrics Audio quality metrics
     */
    void audioStreamDetected(uint32_t service_id, const AudioQualityMetrics& metrics);

    /**
     * @brief Emitted on audio quality warning
     * @param service_id DAB service ID
     * @param warning Warning message
     */
    void audioQualityWarning(uint32_t service_id, const QString& warning);

    /**
     * @brief Emitted on audio stream error
     * @param service_id DAB service ID
     * @param error Error message
     */
    void audioStreamError(uint32_t service_id, const QString& error);

    /**
     * @brief Emitted when PAD data is extracted from audio superframe
     * @param service_id Service ID
     * @param pad_data Programme Associated Data for DLS+/MOT SlideShow
     *
     * Phase 3B Week 2: DLS+ Integration
     * This signal connects DAB+ validator to DLS+ decoder for metadata extraction.
     */
    void padDataExtracted(uint32_t service_id, const QByteArray& pad_data);

private:
    // ========================================================================
    // FireCode CRC Implementation
    // ========================================================================

    /**
     * @brief Calculate FireCode CRC-16
     * @param data Input data
     * @return 16-bit CRC
     *
     * ETSI TS 102 563 Section 6.2: FireCode polynomial
     * g(x) = x^16 + x^14 + x^13 + x^11 + x^10 + x^8 + x^6 + x^5 + x^2 + x + 1
     * Polynomial value: 0x782F
     */
    uint16_t calculateFireCodeCRC(const std::vector<uint8_t>& data);

    /**
     * @brief Initialize FireCode lookup table for fast CRC calculation
     */
    void initializeFireCodeTable();

    // ========================================================================
    // Data Extraction Helpers
    // ========================================================================

    /**
     * @brief Extract superframe header from MSC data
     */
    bool extractSuperframeHeader(const uint8_t* data, size_t length,
                                 AudioSuperframe& superframe);

    /**
     * @brief Extract Audio Access Units from superframe
     */
    bool extractAudioAccessUnits(const uint8_t* data, size_t offset,
                                 AudioSuperframe& superframe);

    /**
     * @brief Calculate quality estimate based on bitrate and protection
     */
    double calculateQualityEstimate(uint16_t bitrate_kbps,
                                    uint8_t protection_level,
                                    bool is_heaac_v2,
                                    bool sbr_flag);

    /**
     * @brief Extract Programme Associated Data (PAD) from MSC data
     * @param msc_data MSC data containing audio superframe
     * @param superframe Parsed audio superframe
     * @return Extracted PAD data (X-PAD + F-PAD)
     *
     * Phase 3B Week 2: PAD extraction for DLS+ and MOT SlideShow
     * ETSI TS 102 563 Section 7: Programme Associated Data structure
     */
    QByteArray extractPADData(const QByteArray& msc_data,
                             const AudioSuperframe& superframe);

    // ========================================================================
    // Private Data Members
    // ========================================================================

    // FireCode CRC lookup table (256 entries)
    std::array<uint16_t, 256> m_firecode_table;

    // Service tracking
    struct ServiceAudioData {
        AudioSuperframe last_superframe;
        AudioQualityMetrics metrics;
        QDateTime last_update;
    };
    std::map<uint32_t, ServiceAudioData> m_service_data;

    // Statistics
    Statistics m_statistics;

    // Mutex for thread-safe access
    mutable QMutex m_mutex;

    // Phase 3B: DLS+ decoder integration (activated)
    eti::dls_plus::DLSPlusDecoder* m_dls_plus_decoder = nullptr;

public:
    // Phase 3B: DLS+ decoder accessor
    eti::dls_plus::DLSPlusDecoder* dlsPlusDecoder() const { return m_dls_plus_decoder; }
};

} // namespace eti::audio

// Qt metatype registration for signals/slots
Q_DECLARE_METATYPE(eti::audio::AudioSuperframe)
Q_DECLARE_METATYPE(eti::audio::AudioQualityMetrics)
Q_DECLARE_METATYPE(eti::audio::ValidationResult)
