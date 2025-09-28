/**
 * @file eti_ni_processor.cpp
 * @brief ETI-NI Format Processor Implementation
 * 
 * Implementation of ETI-NI format processing with TIST timestamp extraction
 * and synchronization for ODR-DabMux integration.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 */

#include "eti_ni_processor.hpp"
#include "utils/logger.h"

#include <QDateTime>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace eti::ni {

// ============================================================================
// TISTInfo Implementation
// ============================================================================

void TISTInfo::parseTimestamp()
{
    if (raw_tist == 0x000000) {
        type = TimestampType::INVALID;
        valid = false;
        return;
    }
    
    // Determine TIST type based on value range and context
    type = determineTISTType(raw_tist);
    
    switch (type) {
        case TimestampType::UTC_SECONDS:
            parseUTCSeconds();
            break;
        case TimestampType::RELATIVE_24MS:
            parseRelative24ms();
            break;
        case TimestampType::SAMPLE_COUNT:
            parseSampleCount();
            break;
        case TimestampType::MILLISECONDS:
            parseMilliseconds();
            break;
        default:
            valid = false;
            return;
    }
    
    valid = true;
}

TISTInfo::TimestampType TISTInfo::determineTISTType(uint32_t tist_value) const
{
    // ETSI EN 300 799 Section 5.2.2 - TIST interpretation
    
    // If value is very large, likely UTC seconds since epoch
    if (tist_value > 1000000000) { // After year 2001
        return TimestampType::UTC_SECONDS;
    }
    
    // If value is moderate, likely milliseconds
    if (tist_value > 1000000) {
        return TimestampType::MILLISECONDS;
    }
    
    // If value is small, likely 24ms frame units or sample counts
    if (tist_value < 100000) {
        return TimestampType::RELATIVE_24MS;
    }
    
    // Default to sample count for intermediate values
    return TimestampType::SAMPLE_COUNT;
}

void TISTInfo::parseUTCSeconds()
{
    // TIST contains UTC seconds since epoch
    utc_timestamp = std::chrono::system_clock::from_time_t(raw_tist);
    sync_accuracy = std::chrono::microseconds{1000000}; // 1 second accuracy
}

void TISTInfo::parseRelative24ms()
{
    // TIST contains frame count in 24ms units
    auto frame_duration = std::chrono::microseconds{24000}; // 24ms per frame
    auto total_microseconds = frame_duration * raw_tist;
    
    // Reference to current time (simplified - would need better reference)
    auto now = std::chrono::system_clock::now();
    utc_timestamp = now - total_microseconds;
    
    sync_accuracy = std::chrono::microseconds{24000}; // 24ms accuracy
    frame_offset = std::chrono::microseconds{0};
}

void TISTInfo::parseSampleCount()
{
    // TIST contains sample count at 2.048 MHz
    constexpr uint32_t SAMPLE_RATE = 2048000; // 2.048 MHz
    
    auto microseconds = std::chrono::microseconds{
        static_cast<uint64_t>(raw_tist) * 1000000 / SAMPLE_RATE
    };
    
    // Reference to current time (simplified)
    auto now = std::chrono::system_clock::now();
    utc_timestamp = now - microseconds;
    
    sync_accuracy = std::chrono::microseconds{500}; // ~0.5ms accuracy
    sample_offset = static_cast<int64_t>(raw_tist % (SAMPLE_RATE / 1000)); // Sub-ms offset
}

void TISTInfo::parseMilliseconds()
{
    // TIST contains milliseconds
    auto milliseconds = std::chrono::milliseconds{raw_tist};
    
    // Reference to current time (simplified)
    auto now = std::chrono::system_clock::now();
    utc_timestamp = now - milliseconds;
    
    sync_accuracy = std::chrono::microseconds{1000}; // 1ms accuracy
}

// ============================================================================
// ETINIFrame Implementation
// ============================================================================

void ETINIFrame::parseFrame()
{
    process_time = std::chrono::system_clock::now();
    
    // Validate basic ETI structure first
    if (!validateStructure()) {
        return;
    }
    
    // Extract TIST from LIDATA field
    extractTIST();
    
    // Check if this is ETI-NI format
    validateETINIFormat();
}

bool ETINIFrame::validateStructure() const
{
    // Check ETI sync pattern
    const uint8_t expected_sync[4] = {0x49, 0x93, 0x1E, 0x03};
    if (std::memcmp(frame_data.data(), expected_sync, 4) != 0) {
        return false;
    }
    
    // Basic LIDATA validation
    if (frame_data.size() < 12) {
        return false;
    }
    
    const uint8_t* lidata = frame_data.data() + 4;
    uint8_t fc = lidata[0];      // Frame count
    uint8_t nst = lidata[1] & 0x7F; // Number of sub-channels
    
    // Validate ranges
    if (fc > 249 || nst > 63) {
        return false;
    }
    
    return true;
}

void ETINIFrame::extractTIST()
{
    // TIST is in LIDATA field at bytes 8-11 (32-bit value)
    if (frame_data.size() < 12) {
        return;
    }
    
    const uint8_t* lidata = frame_data.data() + 4;
    uint32_t tist_value = (static_cast<uint32_t>(lidata[4]) << 24) |
                         (static_cast<uint32_t>(lidata[5]) << 16) |
                         (static_cast<uint32_t>(lidata[6]) << 8) |
                         static_cast<uint32_t>(lidata[7]);
    
    // Only use lower 24 bits for TIST (upper 8 bits may be reserved)
    tist_value &= 0x00FFFFFF;
    
    tist_info = TISTInfo(tist_value);
}

void ETINIFrame::validateETINIFormat()
{
    // ETI-NI format detection based on ETSI EN 302 077
    // This is simplified - full implementation would check specific ETI-NI markers
    
    // For now, assume all frames from ZeroMQ are ETI-NI format
    is_eti_ni_format = true;
    version = 1; // ETI-NI version 1
    payload_length = eti::ETI_FRAME_SIZE; // Full frame payload
}

// ============================================================================
// ETINIProcessor Implementation
// ============================================================================

ETINIProcessor::ETINIProcessor(QObject* parent)
    : QObject(parent)
{
    // Initialize synchronization tracking
    sync_accuracy_history_.reserve(1000);
    drift_measurements_.reserve(MAX_DRIFT_SAMPLES);
    
    reference_time_ = std::chrono::system_clock::now();
    last_sync_update_ = reference_time_;
    
    Logger::instance().log(Logger::Info, "ETINIProcessor", 
                          "ETI-NI processor created");
}

ETINIProcessor::~ETINIProcessor()
{
    Logger::instance().log(Logger::Info, "ETINIProcessor", 
                          "ETI-NI processor destroyed");
}

bool ETINIProcessor::initialize()
{
    if (initialized_) {
        return true;
    }
    
    // Reset all state
    resetSynchronization();
    
    initialized_ = true;
    
    Logger::instance().log(Logger::Info, "ETINIProcessor", 
                          "ETI-NI processor initialized");
    return true;
}

ETINIFrame ETINIProcessor::processFrame(std::span<const uint8_t, eti::ETI_FRAME_SIZE> frame_data)
{
    if (!initialized_) {
        Logger::instance().log(Logger::Warning, "ETINIProcessor", 
                              "Processor not initialized");
        return ETINIFrame{};
    }
    
    // Create ETI-NI frame
    ETINIFrame frame(frame_data);
    frame.frame_number = ++frames_processed_;
    
    // Process TIST if available
    if (frame.tist_info.isValid()) {
        emit tistExtracted(frame.tist_info, frame.frame_number);
        
        // Update synchronization if auto-sync enabled
        if (auto_sync_enabled_) {
            synchronizeWithTIST(frame.tist_info);
        }
        
        // Update metrics
        updateSynchronizationMetrics(frame.tist_info);
    }
    
    // Validate ETI-NI compliance
    auto validation_result = validateETINICompliance(frame);
    if (!validation_result.valid) {
        for (const auto& error : validation_result.errors) {
            emit complianceIssue(QString::fromStdString(error), frame.frame_number);
        }
    }
    
    emit frameProcessed(frame);
    
    Logger::instance().log(Logger::Debug, "ETINIProcessor", 
                          QString("Processed ETI-NI frame #%1, TIST: %2, Valid: %3")
                          .arg(frame.frame_number)
                          .arg(frame.tist_info.raw_tist, 6, 16, QChar('0'))
                          .arg(frame.tist_info.isValid() ? "Yes" : "No"));
    
    return frame;
}

ETINIFrame ETINIProcessor::processFrame(const QByteArray& frame_data)
{
    if (frame_data.size() != static_cast<int>(eti::ETI_FRAME_SIZE)) {
        Logger::instance().log(Logger::Error, "ETINIProcessor", 
                              QString("Invalid frame size: %1 (expected %2)")
                              .arg(frame_data.size()).arg(eti::ETI_FRAME_SIZE));
        return ETINIFrame{};
    }
    
    auto data_span = std::span<const uint8_t, eti::ETI_FRAME_SIZE>(
        reinterpret_cast<const uint8_t*>(frame_data.constData()), 
        eti::ETI_FRAME_SIZE
    );
    
    return processFrame(data_span);
}

TISTInfo ETINIProcessor::extractTIST(const eti::EtiFrame& frame) const
{
    // Extract TIST from standard ETI frame
    auto lidata = frame.get_lidata_field();
    return TISTInfo(lidata.tist);
}

bool ETINIProcessor::synchronizeWithTIST(const TISTInfo& tist_info)
{
    if (!tist_info.isValid()) {
        return false;
    }
    
    auto current_time = std::chrono::system_clock::now();
    
    // Calculate expected local time based on TIST
    auto reference_time = calculateReferenceTime(tist_info);
    
    // Calculate synchronization error
    auto sync_error = std::chrono::duration_cast<std::chrono::microseconds>(
        std::abs((current_time - reference_time).count())
    );
    
    // Update synchronization status
    sync_status_.last_sync_time = current_time;
    sync_status_.sync_accuracy = sync_error;
    sync_status_.frames_synchronized++;
    
    // Check if we meet accuracy target
    bool meets_target = sync_error < sync_accuracy_target_;
    
    if (meets_target != sync_status_.is_synchronized) {
        sync_status_.is_synchronized = meets_target;
        emit synchronizationStatusChanged(sync_status_);
        
        if (meets_target) {
            emit syncAccuracyAchieved(sync_error, sync_accuracy_target_);
            Logger::instance().log(Logger::Info, "ETINIProcessor", 
                                  QString("Synchronization achieved: %1μs accuracy")
                                  .arg(sync_error.count()));
        }
    }
    
    // Update clock drift measurements
    updateClockDrift(tist_info);
    
    // Store for history tracking
    sync_accuracy_history_.push_back(sync_error);
    if (sync_accuracy_history_.size() > 1000) {
        sync_accuracy_history_.erase(sync_accuracy_history_.begin(), 
                                    sync_accuracy_history_.begin() + 100);
    }
    
    last_tist_ = tist_info;
    return meets_target;
}

ETINIProcessor::SyncStatus ETINIProcessor::getSynchronizationStatus() const
{
    return sync_status_;
}

void ETINIProcessor::resetSynchronization()
{
    sync_status_ = SyncStatus{};
    sync_accuracy_history_.clear();
    drift_measurements_.clear();
    last_tist_.reset();
    reference_time_ = std::chrono::system_clock::now();
    frames_processed_ = 0;
    sync_corrections_ = 0;
    
    Logger::instance().log(Logger::Info, "ETINIProcessor", 
                          "Synchronization state reset");
}

eti::EtiFrame ETINIProcessor::convertToStandardETI(const ETINIFrame& eti_ni_frame) const
{
    // Convert ETI-NI frame to standard ETI format
    eti::EtiFrame standard_frame;
    std::memcpy(standard_frame.data(), eti_ni_frame.frame_data.data(), eti::ETI_FRAME_SIZE);
    standard_frame.set_receive_timestamp(eti_ni_frame.receive_time);
    return standard_frame;
}

eti::ValidationResult ETINIProcessor::validateETINICompliance(const ETINIFrame& frame) const
{
    eti::ValidationResult result;
    result.valid = true;
    
    // Validate ETI-NI structure
    if (!frame.validateStructure()) {
        result.add_error("Invalid ETI-NI frame structure");
        return result;
    }
    
    // Validate TIST if present
    if (frame.tist_info.raw_tist != 0) {
        if (!frame.tist_info.isValid()) {
            result.add_error("Invalid TIST value in ETI-NI frame");
        }
        
        if (!utils::validateTISTRange(frame.tist_info.raw_tist)) {
            result.add_warning("TIST value outside expected range");
        }
    }
    
    // Validate ETI-NI version
    if (frame.is_eti_ni_format && frame.version == 0) {
        result.add_warning("ETI-NI version not specified");
    }
    
    // Check synchronization status
    if (sync_status_.is_synchronized && frame.tist_info.isValid()) {
        auto sync_accuracy = frame.tist_info.sync_accuracy;
        if (sync_accuracy > std::chrono::microseconds{10000}) { // 10ms threshold
            result.add_warning("Poor synchronization accuracy detected");
        }
    }
    
    return result;
}

// ============================================================================
// Private Implementation Methods
// ============================================================================

void ETINIProcessor::updateSynchronizationMetrics(const TISTInfo& tist_info)
{
    // Calculate average synchronization accuracy
    if (!sync_accuracy_history_.empty()) {
        auto total_accuracy = std::chrono::microseconds{0};
        for (const auto& accuracy : sync_accuracy_history_) {
            total_accuracy += accuracy;
        }
        sync_status_.sync_accuracy = total_accuracy / sync_accuracy_history_.size();
    }
    
    // Validate TIST consistency
    validateTISTConsistency(tist_info);
    
    // Update status timestamp
    sync_status_.last_sync_time = std::chrono::system_clock::now();
}

std::chrono::system_clock::time_point ETINIProcessor::calculateReferenceTime(
    const TISTInfo& tist_info) const
{
    // This is a simplified reference time calculation
    // In a real implementation, this would use a more sophisticated algorithm
    // to correlate TIST values with actual UTC time
    
    if (tist_info.type == TISTInfo::TimestampType::UTC_SECONDS) {
        return tist_info.utc_timestamp;
    }
    
    // For other TIST types, use current reference time as baseline
    return reference_time_;
}

void ETINIProcessor::validateTISTConsistency(const TISTInfo& tist_info)
{
    if (!last_tist_ || !tist_info.isValid()) {
        return;
    }
    
    // Check for TIST jumps or inconsistencies
    auto time_diff = tist_info.timeDifference(*last_tist_);
    auto expected_diff = std::chrono::microseconds{24000}; // 24ms frame duration
    
    if (std::abs(time_diff.count() - expected_diff.count()) > 5000) { // 5ms tolerance
        Logger::instance().log(Logger::Warning, "ETINIProcessor", 
                              QString("TIST inconsistency detected: %1μs difference (expected ~24000μs)")
                              .arg(time_diff.count()));
    }
}

void ETINIProcessor::updateClockDrift(const TISTInfo& tist_info)
{
    auto local_time = std::chrono::system_clock::now();
    auto tist_time = tist_info.utc_timestamp;
    auto difference = std::chrono::duration_cast<std::chrono::microseconds>(
        local_time - tist_time);
    
    // Store drift measurement
    DriftMeasurement measurement{local_time, tist_time, difference};
    drift_measurements_.push_back(measurement);
    
    // Keep only recent measurements
    if (drift_measurements_.size() > MAX_DRIFT_SAMPLES) {
        drift_measurements_.erase(drift_measurements_.begin());
    }
    
    // Calculate drift rate (simplified linear regression)
    if (drift_measurements_.size() >= 10) {
        // Calculate slope of drift over time
        auto first = drift_measurements_.front();
        auto last = drift_measurements_.back();
        
        auto time_span = std::chrono::duration_cast<std::chrono::seconds>(
            last.local_time - first.local_time);
        auto drift_change = last.difference - first.difference;
        
        if (time_span.count() > 0) {
            // Drift rate in microseconds per second = ppm
            sync_status_.clock_drift_ppm = static_cast<double>(drift_change.count()) / 
                                          time_span.count();
        }
    }
}

bool ETINIProcessor::isWithinSyncTolerance(const TISTInfo& tist_info) const
{
    return tist_info.sync_accuracy <= sync_accuracy_target_;
}

// ============================================================================
// Utility Functions
// ============================================================================

namespace utils {

uint32_t calculateTIST(const std::chrono::system_clock::time_point& utc_time,
                      TISTInfo::TimestampType type)
{
    switch (type) {
        case TISTInfo::TimestampType::UTC_SECONDS: {
            auto time_t_value = std::chrono::system_clock::to_time_t(utc_time);
            return static_cast<uint32_t>(time_t_value) & 0x00FFFFFF;
        }
        
        case TISTInfo::TimestampType::MILLISECONDS: {
            auto epoch = std::chrono::system_clock::from_time_t(0);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                utc_time - epoch);
            return static_cast<uint32_t>(duration.count()) & 0x00FFFFFF;
        }
        
        case TISTInfo::TimestampType::SAMPLE_COUNT: {
            auto epoch = std::chrono::system_clock::from_time_t(0);
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                utc_time - epoch);
            constexpr uint32_t SAMPLE_RATE = 2048000;
            auto samples = (duration.count() * SAMPLE_RATE) / 1000000;
            return static_cast<uint32_t>(samples) & 0x00FFFFFF;
        }
        
        case TISTInfo::TimestampType::RELATIVE_24MS: {
            // Frame count since some reference point
            auto epoch = std::chrono::system_clock::from_time_t(0);
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                utc_time - epoch);
            auto frames = duration.count() / 24000; // 24ms per frame
            return static_cast<uint32_t>(frames) & 0x00FFFFFF;
        }
        
        default:
            return 0x000000;
    }
}

bool validateTISTRange(uint32_t tist_value)
{
    // TIST is 24-bit value, so upper 8 bits should be zero
    if ((tist_value & 0xFF000000) != 0) {
        return false;
    }
    
    // Zero value indicates invalid TIST
    if (tist_value == 0x000000) {
        return false;
    }
    
    // All other values in 24-bit range are potentially valid
    return true;
}

QString formatTISTInfo(const TISTInfo& tist_info)
{
    if (!tist_info.isValid()) {
        return QString("Invalid TIST (0x%1)").arg(tist_info.raw_tist, 6, 16, QChar('0'));
    }
    
    QString type_str;
    switch (tist_info.type) {
        case TISTInfo::TimestampType::UTC_SECONDS:
            type_str = "UTC Seconds";
            break;
        case TISTInfo::TimestampType::RELATIVE_24MS:
            type_str = "24ms Frames";
            break;
        case TISTInfo::TimestampType::SAMPLE_COUNT:
            type_str = "Sample Count";
            break;
        case TISTInfo::TimestampType::MILLISECONDS:
            type_str = "Milliseconds";
            break;
        default:
            type_str = "Unknown";
            break;
    }
    
    return QString("TIST: 0x%1 (%2) - %3, Accuracy: %4μs, Quality: %5%")
           .arg(tist_info.raw_tist, 6, 16, QChar('0'))
           .arg(type_str)
           .arg(tist_info.getISO8601String())
           .arg(tist_info.sync_accuracy.count())
           .arg(tist_info.getSyncQuality(), 0, 'f', 1);
}

std::chrono::microseconds calculateSyncAccuracy(const TISTInfo& tist1, const TISTInfo& tist2)
{
    if (!tist1.isValid() || !tist2.isValid()) {
        return std::chrono::microseconds{999999}; // Max error for invalid TIST
    }
    
    auto time_diff = tist1.timeDifference(tist2);
    return std::chrono::microseconds{std::abs(time_diff.count())};
}

} // namespace utils

} // namespace eti::ni