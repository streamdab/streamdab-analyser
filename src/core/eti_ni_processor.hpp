/**
 * @file eti_ni_processor.hpp
 * @brief ETI-NI Format Processor with TIST Support
 * 
 * Implementation of ETI-NI (Network Independent) format processing
 * according to ETSI EN 302 077 v2.3.1 with TIST timestamp extraction
 * and synchronization support for ODR-DabMux integration.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include "eti_types.hpp"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <chrono>
#include <optional>
#include <span>

namespace eti::ni {

/**
 * @brief TIST (Timestamp) Information Structure
 * 
 * TIST provides timestamp information for ETI frames according to
 * ETSI EN 300 799 Section 5.2.2 and EN 302 077 Section 7.1.
 */
struct TISTInfo {
    enum class TimestampType : uint8_t {
        INVALID = 0,     // TIST = 0x000000 (invalid)
        UTC_SECONDS,     // TIST in UTC seconds since epoch
        RELATIVE_24MS,   // TIST in 24ms frame units
        SAMPLE_COUNT,    // TIST in 2.048MHz sample counts
        MILLISECONDS     // TIST in milliseconds
    };
    
    uint32_t raw_tist{0};                    // Raw 24-bit TIST value from frame
    TimestampType type{TimestampType::INVALID};
    std::chrono::system_clock::time_point utc_timestamp;
    std::chrono::microseconds frame_offset{0};  // Offset within 24ms frame
    bool valid{false};                       // Whether TIST is valid and synchronized
    
    // Synchronization accuracy metrics
    std::chrono::microseconds sync_accuracy{0};   // Sync accuracy (target <1ms)
    int64_t sample_offset{0};                // Sample offset for fine synchronization
    double drift_ppm{0.0};                   // Clock drift in parts per million
    
    /**
     * @brief Default constructor - invalid TIST
     */
    TISTInfo() = default;
    
    /**
     * @brief Construct from raw TIST value
     * @param tist_value Raw 24-bit TIST from ETI frame
     */
    explicit TISTInfo(uint32_t tist_value) : raw_tist(tist_value) {
        parseTimestamp();
    }
    
    /**
     * @brief Check if TIST is valid (non-zero)
     */
    [[nodiscard]] bool isValid() const noexcept {
        return valid && raw_tist != 0x000000;
    }
    
    /**
     * @brief Get UTC timestamp as QDateTime for Qt integration
     */
    [[nodiscard]] QDateTime getQDateTime() const {
        if (!isValid()) {
            return QDateTime();
        }
        
        auto time_t_value = std::chrono::system_clock::to_time_t(utc_timestamp);
        return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(time_t_value), Qt::UTC);
    }
    
    /**
     * @brief Get timestamp in ISO 8601 format
     */
    [[nodiscard]] QString getISO8601String() const {
        return getQDateTime().toString(Qt::ISODateWithMs);
    }
    
    /**
     * @brief Calculate time difference from another TIST
     * @param other Other TIST info to compare with
     * @return Time difference in microseconds
     */
    [[nodiscard]] std::chrono::microseconds timeDifference(const TISTInfo& other) const {
        if (!isValid() || !other.isValid()) {
            return std::chrono::microseconds{0};
        }
        
        return std::chrono::duration_cast<std::chrono::microseconds>(
            utc_timestamp - other.utc_timestamp);
    }
    
    /**
     * @brief Get synchronization quality score (0.0-100.0)
     */
    [[nodiscard]] double getSyncQuality() const {
        if (!isValid()) {
            return 0.0;
        }
        
        // Quality based on sync accuracy (target <1ms)
        const auto target_accuracy = std::chrono::microseconds{1000};
        if (sync_accuracy <= target_accuracy) {
            return 100.0;
        } else if (sync_accuracy <= std::chrono::microseconds{10000}) {
            // Linear degradation from 100% at 1ms to 50% at 10ms
            double ratio = static_cast<double>(sync_accuracy.count()) / 10000.0;
            return 100.0 - (ratio * 50.0);
        } else {
            return std::max(0.0, 50.0 - (static_cast<double>(sync_accuracy.count() - 10000) / 1000.0));
        }
    }

private:
    void parseTimestamp();
};

/**
 * @brief ETI-NI Frame Structure
 * 
 * Enhanced ETI frame structure for Network Independent format
 * with proper TIST handling and ODR-DabMux compatibility.
 */
struct ETINIFrame {
    // Raw frame data (6144 bytes for ETI-NI)
    std::array<uint8_t, eti::ETI_FRAME_SIZE> frame_data;
    
    // Parsed timestamp information
    TISTInfo tist_info;
    
    // Frame metadata
    uint32_t frame_number{0};
    std::chrono::system_clock::time_point receive_time;
    std::chrono::system_clock::time_point process_time;
    
    // ETI-NI specific fields
    bool is_eti_ni_format{false};
    uint8_t version{0};             // ETI-NI version
    uint16_t payload_length{0};     // Actual payload length
    
    // Quality metrics
    bool sync_locked{false};        // Whether timestamp sync is locked
    double signal_quality{100.0};   // Signal quality percentage
    
    /**
     * @brief Default constructor
     */
    ETINIFrame() {
        frame_data.fill(0);
        receive_time = std::chrono::system_clock::now();
    }
    
    /**
     * @brief Construct from raw data
     * @param data Raw ETI frame data
     */
    explicit ETINIFrame(std::span<const uint8_t, eti::ETI_FRAME_SIZE> data) {
        std::memcpy(frame_data.data(), data.data(), eti::ETI_FRAME_SIZE);
        receive_time = std::chrono::system_clock::now();
        parseFrame();
    }
    
    /**
     * @brief Get standard ETI frame representation
     */
    [[nodiscard]] eti::EtiFrame toStandardETI() const {
        eti::EtiFrame standard_frame;
        std::memcpy(standard_frame.data(), frame_data.data(), eti::ETI_FRAME_SIZE);
        standard_frame.set_receive_timestamp(receive_time);
        return standard_frame;
    }
    
    /**
     * @brief Validate ETI-NI frame structure
     */
    [[nodiscard]] bool validateStructure() const;
    
    /**
     * @brief Get frame processing latency
     */
    [[nodiscard]] std::chrono::microseconds getProcessingLatency() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            process_time - receive_time);
    }

private:
    void parseFrame();
    void extractTIST();
    void validateETINIFormat();
};

/**
 * @brief ETI-NI Processor with TIST Support
 * 
 * Processes ETI-NI format frames from ODR-DabMux with timestamp extraction,
 * synchronization, and conversion to standard ETI format for compatibility.
 */
class ETINIProcessor : public QObject {
    Q_OBJECT
    
public:
    explicit ETINIProcessor(QObject* parent = nullptr);
    ~ETINIProcessor() override;
    
    /**
     * @brief Initialize processor with configuration
     */
    bool initialize();
    
    /**
     * @brief Check if processor is initialized
     */
    [[nodiscard]] bool isInitialized() const noexcept {
        return initialized_;
    }
    
    /**
     * @brief Process ETI-NI frame
     * @param frame_data Raw ETI-NI frame data (6144 bytes)
     * @return Processed ETI-NI frame with TIST information
     */
    [[nodiscard]] ETINIFrame processFrame(std::span<const uint8_t, eti::ETI_FRAME_SIZE> frame_data);
    
    /**
     * @brief Process ETI-NI frame from QByteArray
     * @param frame_data Qt byte array containing frame
     * @return Processed ETI-NI frame
     */
    [[nodiscard]] ETINIFrame processFrame(const QByteArray& frame_data);
    
    /**
     * @brief Extract TIST from ETI frame
     * @param frame ETI frame to extract TIST from
     * @return TIST information structure
     */
    [[nodiscard]] TISTInfo extractTIST(const eti::EtiFrame& frame) const;
    
    /**
     * @brief Synchronize local clock with TIST
     * @param tist_info TIST information to synchronize with
     * @return true if synchronization successful
     */
    bool synchronizeWithTIST(const TISTInfo& tist_info);
    
    /**
     * @brief Get current synchronization status
     */
    struct SyncStatus {
        bool is_synchronized{false};
        std::chrono::microseconds sync_accuracy{0};
        double clock_drift_ppm{0.0};
        uint64_t frames_synchronized{0};
        std::chrono::system_clock::time_point last_sync_time;
        
        [[nodiscard]] bool meetsAccuracyTarget() const {
            return sync_accuracy < std::chrono::microseconds{1000}; // <1ms target
        }
    };
    
    [[nodiscard]] SyncStatus getSynchronizationStatus() const;
    
    /**
     * @brief Reset synchronization state
     */
    void resetSynchronization();
    
    /**
     * @brief Set synchronization accuracy target
     * @param target_accuracy Target accuracy in microseconds
     */
    void setSyncAccuracyTarget(std::chrono::microseconds target_accuracy) {
        sync_accuracy_target_ = target_accuracy;
    }
    
    /**
     * @brief Enable/disable automatic synchronization
     * @param enable true to enable auto-sync
     */
    void setAutoSynchronization(bool enable) {
        auto_sync_enabled_ = enable;
    }
    
    /**
     * @brief Convert ETI-NI frame to standard ETI format
     * @param eti_ni_frame ETI-NI frame to convert
     * @return Standard ETI frame
     */
    [[nodiscard]] eti::EtiFrame convertToStandardETI(const ETINIFrame& eti_ni_frame) const;
    
    /**
     * @brief Validate ETI-NI frame format compliance
     * @param frame ETI-NI frame to validate
     * @return Validation result with compliance details
     */
    [[nodiscard]] eti::ValidationResult validateETINICompliance(const ETINIFrame& frame) const;

signals:
    /**
     * @brief Emitted when ETI-NI frame is processed
     * @param frame Processed ETI-NI frame
     */
    void frameProcessed(const ETINIFrame& frame);
    
    /**
     * @brief Emitted when TIST is extracted and parsed
     * @param tist_info TIST information
     * @param frame_number Frame number for reference
     */
    void tistExtracted(const TISTInfo& tist_info, uint32_t frame_number);
    
    /**
     * @brief Emitted when synchronization status changes
     * @param status Current synchronization status
     */
    void synchronizationStatusChanged(const SyncStatus& status);
    
    /**
     * @brief Emitted when synchronization accuracy target is met
     * @param accuracy Current synchronization accuracy
     * @param target Target accuracy
     */
    void syncAccuracyAchieved(std::chrono::microseconds accuracy, 
                             std::chrono::microseconds target);
    
    /**
     * @brief Emitted when ETI-NI compliance issue is detected
     * @param issue Compliance issue description
     * @param frame_number Frame number where issue occurred
     */
    void complianceIssue(const QString& issue, uint32_t frame_number);

private:
    // Core processing methods
    void updateSynchronizationMetrics(const TISTInfo& tist_info);
    std::chrono::system_clock::time_point calculateReferenceTime(const TISTInfo& tist_info) const;
    void validateTISTConsistency(const TISTInfo& tist_info);
    
    // TIST parsing helpers
    TISTInfo::TimestampType determineTISTType(uint32_t tist_value) const;
    std::chrono::system_clock::time_point parseTISTValue(uint32_t tist_value, 
                                                         TISTInfo::TimestampType type) const;
    
    // Synchronization algorithms
    void updateClockDrift(const TISTInfo& tist_info);
    bool isWithinSyncTolerance(const TISTInfo& tist_info) const;
    
    // Member variables
    bool initialized_{false};
    
    // Synchronization state
    SyncStatus sync_status_;
    std::chrono::microseconds sync_accuracy_target_{1000}; // 1ms default target
    bool auto_sync_enabled_{true};
    
    // Clock synchronization tracking
    std::chrono::system_clock::time_point reference_time_;
    std::optional<TISTInfo> last_tist_;
    std::vector<std::chrono::microseconds> sync_accuracy_history_;
    
    // Performance tracking
    uint64_t frames_processed_{0};
    uint64_t sync_corrections_{0};
    std::chrono::system_clock::time_point last_sync_update_;
    
    // Drift calculation
    struct DriftMeasurement {
        std::chrono::system_clock::time_point local_time;
        std::chrono::system_clock::time_point tist_time;
        std::chrono::microseconds difference;
    };
    std::vector<DriftMeasurement> drift_measurements_;
    static constexpr size_t MAX_DRIFT_SAMPLES = 100;
};

/**
 * @brief Utility functions for ETI-NI processing
 */
namespace utils {
    /**
     * @brief Calculate TIST from UTC timestamp
     * @param utc_time UTC timestamp
     * @param type TIST encoding type
     * @return 24-bit TIST value
     */
    [[nodiscard]] uint32_t calculateTIST(const std::chrono::system_clock::time_point& utc_time,
                                        TISTInfo::TimestampType type);
    
    /**
     * @brief Validate TIST value range and format
     * @param tist_value TIST value to validate
     * @return true if TIST is within valid range
     */
    [[nodiscard]] bool validateTISTRange(uint32_t tist_value);
    
    /**
     * @brief Convert TIST to human-readable string
     * @param tist_info TIST information to format
     * @return Formatted timestamp string
     */
    [[nodiscard]] QString formatTISTInfo(const TISTInfo& tist_info);
    
    /**
     * @brief Calculate synchronization accuracy between two TISTs
     * @param tist1 First TIST
     * @param tist2 Second TIST
     * @return Synchronization accuracy in microseconds
     */
    [[nodiscard]] std::chrono::microseconds calculateSyncAccuracy(
        const TISTInfo& tist1, const TISTInfo& tist2);
}

} // namespace eti::ni