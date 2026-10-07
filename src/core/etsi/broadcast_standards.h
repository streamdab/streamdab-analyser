/**
 * @file broadcast_standards.h
 * @brief Professional Broadcast Standards Framework for ETSI Compliance
 * 
 * Comprehensive implementation of professional broadcast standards including:
 * - EBU R 68 Audio Monitoring Compliance
 * - ITU-R BS.1770-4 Loudness Measurement
 * - SMPTE ST 2110 Audio over IP Standards
 * - AES67 Audio Transport Standards
 * - Professional Audio Quality Assurance
 * - Emergency Alert System (EAS) Integration
 * 
 * This framework provides real-time monitoring and validation against
 * international broadcast standards for professional deployment.
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#ifndef ETSI_BROADCAST_STANDARDS_H
#define ETSI_BROADCAST_STANDARDS_H

#include "alert_system.h"
#include "compliance_engine.h"
#include "../eti_types.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <complex>

namespace etsi {
namespace broadcast {

/**
 * @brief Professional broadcast standard identifiers
 */
enum class BroadcastStandard : uint8_t {
    EBU_R68 = 1,        // EBU R 68 Audio Monitoring
    ITU_R_BS_1770_4 = 2, // ITU-R BS.1770-4 Loudness
    SMPTE_ST_2110 = 3,   // SMPTE ST 2110 Audio over IP
    AES67 = 4,          // AES67 Audio Transport
    EBU_R128 = 5,       // EBU R 128 Loudness Normalization
    ITU_R_BS_412 = 6,   // ITU-R BS.412 Planning Standards
    ETSI_TS_103_466 = 7, // ETSI TS 103 466 Metadata
    IEC_61937 = 8       // IEC 61937 Digital Audio Interface
};

/**
 * @brief EBU R 68 Audio monitoring compliance metrics
 */
struct EbuR68Metrics {
    double dynamic_range_db = 0.0;          // Dynamic range in dB
    double stereo_phase_correlation = 0.0;   // Stereo phase correlation
    double stereo_balance_db = 0.0;          // L/R balance in dB
    double frequency_response_deviation = 0.0; // Frequency response deviation
    double thd_plus_noise_percent = 0.0;     // THD+N percentage
    double wow_flutter_percent = 0.0;        // Wow and flutter percentage
    double signal_to_noise_ratio = 0.0;     // SNR in dB
    double crosstalk_db = 0.0;               // Inter-channel crosstalk
    std::vector<double> frequency_spectrum;  // Frequency spectrum analysis
    
    bool is_compliant() const {
        return dynamic_range_db >= 12.0 &&
               std::abs(stereo_phase_correlation) <= 30.0 &&
               std::abs(stereo_balance_db) <= 3.0 &&
               frequency_response_deviation <= 1.0 &&
               thd_plus_noise_percent <= 0.1 &&
               wow_flutter_percent <= 0.05;
    }
};

/**
 * @brief ITU-R BS.1770-4 Loudness measurement compliance
 */
struct ItuR1770Metrics {
    double integrated_lufs = 0.0;           // Integrated loudness (LUFS)
    double momentary_lufs = 0.0;            // Momentary loudness (LUFS)
    double short_term_lufs = 0.0;           // Short-term loudness (LUFS)
    double loudness_range_lu = 0.0;         // Loudness range (LU)
    double true_peak_dbfs = 0.0;            // True peak level (dBFS)
    double gating_threshold = -70.0;        // Gating threshold (LUFS)
    std::vector<double> momentary_history;  // Momentary loudness history
    std::vector<double> short_term_history; // Short-term loudness history
    
    bool is_broadcast_compliant() const {
        return integrated_lufs >= -24.0 && integrated_lufs <= -22.0 &&
               true_peak_dbfs <= -1.0;
    }
    
    bool is_streaming_compliant() const {
        return integrated_lufs >= -16.0 && integrated_lufs <= -14.0 &&
               true_peak_dbfs <= -1.0;
    }
};

/**
 * @brief Audio signal quality analysis results
 */
struct AudioQualityAnalysis {
    EbuR68Metrics ebu_r68;
    ItuR1770Metrics itu_r1770;
    
    // Additional quality metrics
    double bit_error_rate = 0.0;
    double jitter_nanoseconds = 0.0;
    double latency_milliseconds = 0.0;
    bool silence_detected = false;
    bool overmodulation_detected = false;
    bool phase_inversion_detected = false;
    bool dc_offset_detected = false;
    
    std::chrono::system_clock::time_point measurement_time;
    uint32_t sample_count = 0;
    double measurement_duration_seconds = 0.0;
    
    AudioQualityAnalysis() : measurement_time(std::chrono::system_clock::now()) {}
};

/**
 * @brief Emergency Alert System (EAS) configuration
 */
struct EasConfiguration {
    bool enable_eas_monitoring = true;
    bool enable_amber_alerts = true;
    bool enable_weather_alerts = true;
    bool enable_civil_emergency = true;
    bool enable_national_emergency = true;
    
    std::chrono::seconds alert_timeout{300};        // 5 minutes
    std::chrono::seconds alert_repeat_interval{60}; // 1 minute
    uint32_t max_concurrent_alerts = 10;
    
    // EAS tone detection
    bool enable_tone_detection = true;
    std::vector<double> eas_tone_frequencies = {853.0, 960.0}; // Hz
    double tone_detection_threshold = -20.0; // dBFS
    std::chrono::milliseconds tone_duration_min{1000}; // 1 second
};

/**
 * @brief Professional broadcast compliance thresholds
 */
struct BroadcastComplianceThresholds {
    // EBU R 68 Thresholds
    struct {
        double dynamic_range_min = 12.0;           // dB
        double stereo_phase_max = 30.0;            // degrees
        double stereo_balance_max = 3.0;           // dB
        double frequency_response_max = 1.0;       // dB
        double thd_plus_noise_max = 0.1;          // %
        double wow_flutter_max = 0.05;            // %
        double snr_min = 60.0;                    // dB
        double crosstalk_max = -40.0;             // dB
    } ebu_r68;
    
    // ITU-R BS.1770-4 Thresholds
    struct {
        double target_lufs = -23.0;               // LUFS (broadcast)
        double lufs_tolerance = 1.0;              // ±1 LUFS
        double true_peak_max = -1.0;              // dBFS
        double loudness_range_max = 20.0;         // LU
        double momentary_max = -18.0;             // LUFS
        double short_term_max = -20.0;            // LUFS
    } itu_r1770;
    
    // General Audio Quality
    struct {
        double silence_threshold = -60.0;         // dBFS
        std::chrono::seconds silence_duration_max{10}; // seconds
        double overmod_threshold = -0.1;          // dBFS
        double dc_offset_max = 0.01;              // linear
        double phase_correlation_min = 0.9;       // correlation
    } audio_quality;
};

/**
 * @brief Real-time broadcast compliance result
 */
struct BroadcastComplianceResult {
    bool overall_compliant = false;
    std::map<BroadcastStandard, bool> standard_compliance;
    std::map<BroadcastStandard, double> compliance_scores;
    
    AudioQualityAnalysis audio_analysis;
    std::vector<alerts::Alert> generated_alerts;
    
    std::chrono::microseconds analysis_duration;
    std::chrono::system_clock::time_point timestamp;
    
    BroadcastComplianceResult() : timestamp(std::chrono::system_clock::now()) {
        // Initialize all standards as non-compliant
        standard_compliance[BroadcastStandard::EBU_R68] = false;
        standard_compliance[BroadcastStandard::ITU_R_BS_1770_4] = false;
        standard_compliance[BroadcastStandard::SMPTE_ST_2110] = false;
        standard_compliance[BroadcastStandard::AES67] = false;
        standard_compliance[BroadcastStandard::EBU_R128] = false;
        
        // Initialize compliance scores
        compliance_scores[BroadcastStandard::EBU_R68] = 0.0;
        compliance_scores[BroadcastStandard::ITU_R_BS_1770_4] = 0.0;
        compliance_scores[BroadcastStandard::SMPTE_ST_2110] = 0.0;
        compliance_scores[BroadcastStandard::AES67] = 0.0;
        compliance_scores[BroadcastStandard::EBU_R128] = 0.0;
    }
    
    void set_compliance(BroadcastStandard standard, bool compliant, double score = 0.0) {
        standard_compliance[standard] = compliant;
        compliance_scores[standard] = score;
        
        // Update overall compliance
        overall_compliant = std::all_of(standard_compliance.begin(), standard_compliance.end(),
                                      [](const auto& pair) { return pair.second; });
    }
    
    double get_overall_score() const {
        if (compliance_scores.empty()) return 0.0;
        
        double total = 0.0;
        for (const auto& pair : compliance_scores) {
            total += pair.second;
        }
        return total / compliance_scores.size();
    }
};

// Forward declarations
class AudioAnalyser;
class LoudnessProcessor;
class EasMonitor;
class SpectrumAnalyser;

/**
 * @brief Professional Broadcast Standards Compliance Framework
 * 
 * Comprehensive framework for real-time monitoring and validation
 * of professional broadcast standards including EBU R 68, ITU-R BS.1770-4,
 * and emergency alert system compliance.
 */
class BroadcastStandardsFramework {
public:
    /**
     * @brief Framework configuration
     */
    struct Config {
        BroadcastComplianceThresholds thresholds;
        EasConfiguration eas_config;
        
        bool enable_real_time_analysis = true;
        bool enable_spectrum_analysis = true;
        bool enable_loudness_logging = true;
        bool enable_quality_reports = true;
        
        std::chrono::milliseconds analysis_interval{100};    // 100ms
        std::chrono::seconds reporting_interval{60};         // 1 minute
        size_t audio_buffer_size = 4096;                    // samples
        uint32_t spectrum_fft_size = 1024;                  // FFT size
        
        Config() = default;
    };
    
    explicit BroadcastStandardsFramework(const Config& config);
    explicit BroadcastStandardsFramework(); // Default constructor
    ~BroadcastStandardsFramework();
    
    // Disable copy/move for thread safety
    BroadcastStandardsFramework(const BroadcastStandardsFramework&) = delete;
    BroadcastStandardsFramework& operator=(const BroadcastStandardsFramework&) = delete;
    BroadcastStandardsFramework(BroadcastStandardsFramework&&) = delete;
    BroadcastStandardsFramework& operator=(BroadcastStandardsFramework&&) = delete;
    
    /**
     * @brief Initialize the broadcast standards framework
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown the framework
     */
    void shutdown();
    
    /**
     * @brief Analyze audio data for broadcast compliance
     * @param audio_samples Raw audio samples (interleaved for multi-channel)
     * @param sample_rate Audio sample rate in Hz
     * @param channels Number of audio channels
     * @param service_id Associated DAB service ID
     * @return Broadcast compliance analysis result
     */
    BroadcastComplianceResult analyze_audio_compliance(
        const std::vector<float>& audio_samples,
        uint32_t sample_rate,
        uint32_t channels,
        uint32_t service_id
    );
    
    /**
     * @brief Perform EBU R 68 audio monitoring compliance check
     * @param audio_samples Audio data to analyze
     * @param sample_rate Sample rate in Hz
     * @param channels Number of channels
     * @return EBU R 68 compliance metrics
     */
    EbuR68Metrics analyze_ebu_r68_compliance(
        const std::vector<float>& audio_samples,
        uint32_t sample_rate,
        uint32_t channels
    );
    
    /**
     * @brief Perform ITU-R BS.1770-4 loudness measurement
     * @param audio_samples Audio data to analyze
     * @param sample_rate Sample rate in Hz
     * @param channels Number of channels
     * @return ITU-R BS.1770-4 loudness metrics
     */
    ItuR1770Metrics analyze_itu_r1770_loudness(
        const std::vector<float>& audio_samples,
        uint32_t sample_rate,
        uint32_t channels
    );
    
    /**
     * @brief Monitor for Emergency Alert System (EAS) tones
     * @param audio_samples Audio data to monitor
     * @param sample_rate Sample rate in Hz
     * @return true if EAS tones detected
     */
    bool monitor_eas_tones(
        const std::vector<float>& audio_samples,
        uint32_t sample_rate
    );
    
    /**
     * @brief Get current loudness measurement history
     * @param duration Duration of history to retrieve
     * @return Vector of historical loudness measurements
     */
    std::vector<ItuR1770Metrics> get_loudness_history(
        std::chrono::minutes duration = std::chrono::minutes{10}
    ) const;
    
    /**
     * @brief Get current EBU R 68 compliance status
     * @return Current EBU R 68 metrics
     */
    EbuR68Metrics get_current_ebu_r68_status() const;
    
    /**
     * @brief Get current ITU-R BS.1770-4 status
     * @return Current loudness measurement
     */
    ItuR1770Metrics get_current_loudness_status() const;
    
    /**
     * @brief Update compliance thresholds
     * @param thresholds New threshold configuration
     */
    void update_compliance_thresholds(const BroadcastComplianceThresholds& thresholds);
    
    /**
     * @brief Enable/disable specific broadcast standard monitoring
     * @param standard Broadcast standard to enable/disable
     * @param enabled true to enable, false to disable
     */
    void enable_standard_monitoring(BroadcastStandard standard, bool enabled);
    
    /**
     * @brief Check if standard monitoring is enabled
     * @param standard Broadcast standard to check
     * @return true if enabled
     */
    bool is_standard_enabled(BroadcastStandard standard) const;
    
    /**
     * @brief Generate compliance report
     * @param duration Duration to include in report
     * @return Formatted compliance report
     */
    std::string generate_compliance_report(
        std::chrono::hours duration = std::chrono::hours{24}
    ) const;
    
    /**
     * @brief Export compliance data to CSV
     * @param duration Duration to export
     * @return CSV formatted compliance data
     */
    std::string export_compliance_csv(
        std::chrono::hours duration = std::chrono::hours{24}
    ) const;
    
    /**
     * @brief Register compliance violation callback
     * @param callback Function to call on compliance violations
     */
    using ComplianceCallback = std::function<void(const BroadcastComplianceResult&)>;
    void register_compliance_callback(ComplianceCallback callback);
    
    /**
     * @brief Unregister compliance callback
     */
    void unregister_compliance_callback();
    
    /**
     * @brief Get framework performance metrics
     */
    struct PerformanceMetrics {
        std::chrono::microseconds avg_analysis_time;
        std::chrono::microseconds max_analysis_time;
        uint32_t analyses_per_second;
        uint32_t total_analyses;
        double cpu_usage_percent;
        size_t memory_usage_mb;
        uint32_t violations_detected;
    };
    
    PerformanceMetrics get_performance_metrics() const;
    
    /**
     * @brief Reset performance metrics
     */
    void reset_performance_metrics();

private:
    Config config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Analysis components
    std::unique_ptr<AudioAnalyser> audio_analyser_;
    std::unique_ptr<LoudnessProcessor> loudness_processor_;
    std::unique_ptr<EasMonitor> eas_monitor_;
    std::unique_ptr<SpectrumAnalyser> spectrum_analyser_;
    
    // Enabled standards
    std::map<BroadcastStandard, bool> enabled_standards_;
    
    // Historical data storage
    mutable std::mutex history_mutex_;
    std::vector<ItuR1770Metrics> loudness_history_;
    std::vector<EbuR68Metrics> ebu_r68_history_;
    std::vector<BroadcastComplianceResult> compliance_history_;
    
    // Performance tracking
    mutable std::mutex metrics_mutex_;
    PerformanceMetrics performance_metrics_;
    std::vector<std::chrono::microseconds> analysis_times_;
    
    // Callback handling
    std::mutex callback_mutex_;
    ComplianceCallback compliance_callback_;
    
    // Internal methods
    void initialize_analysers();
    void update_performance_metrics(const std::chrono::microseconds& analysis_time);
    void add_to_history(const ItuR1770Metrics& loudness, const EbuR68Metrics& ebu_r68);
    void notify_compliance_violation(const BroadcastComplianceResult& result);
    
    // Audio analysis helper methods
    std::vector<std::complex<double>> compute_fft(const std::vector<float>& samples, size_t fft_size);
    std::vector<double> compute_power_spectrum(const std::vector<std::complex<double>>& fft_result);
    double compute_thd_plus_noise(const std::vector<float>& samples, uint32_t sample_rate);
    double compute_dynamic_range(const std::vector<float>& samples);
    double compute_stereo_phase_correlation(const std::vector<float>& left_channel,
                                          const std::vector<float>& right_channel);
    
    // Loudness measurement helper methods
    std::vector<float> apply_k_weighting_filter(const std::vector<float>& samples, uint32_t sample_rate);
    double compute_gated_loudness(const std::vector<float>& k_weighted_samples, double gating_threshold);
    double compute_true_peak(const std::vector<float>& samples, uint32_t sample_rate);
    
    // EAS detection helper methods
    bool detect_eas_tone_frequency(const std::vector<double>& spectrum, 
                                 double target_frequency, 
                                 uint32_t sample_rate, 
                                 size_t fft_size);
    
    // Utility methods
    static std::string get_standard_name(BroadcastStandard standard);
    static std::string format_compliance_summary(const BroadcastComplianceResult& result);
    
    // Configuration validation
    bool validate_configuration(const Config& config) const;
};

/**
 * @brief Utility functions for broadcast standards
 */
namespace utils {
    /**
     * @brief Convert loudness measurement to human-readable string
     * @param metrics ITU-R BS.1770-4 metrics to format
     * @return Formatted string representation
     */
    std::string format_loudness_metrics(const ItuR1770Metrics& metrics);
    
    /**
     * @brief Convert EBU R 68 metrics to human-readable string
     * @param metrics EBU R 68 metrics to format
     * @return Formatted string representation
     */
    std::string format_ebu_r68_metrics(const EbuR68Metrics& metrics);
    
    /**
     * @brief Check if loudness meets broadcast delivery requirements
     * @param lufs Integrated loudness in LUFS
     * @param delivery_type Type of delivery (broadcast, streaming, etc.)
     * @return true if meets requirements
     */
    bool meets_loudness_requirements(double lufs, const std::string& delivery_type);
    
    /**
     * @brief Calculate loudness correction gain
     * @param current_lufs Current integrated loudness
     * @param target_lufs Target loudness
     * @return Gain adjustment in dB
     */
    double calculate_loudness_correction(double current_lufs, double target_lufs);
    
    /**
     * @brief Validate audio sample rate for broadcast standards
     * @param sample_rate Sample rate to validate
     * @return true if valid for broadcast
     */
    bool is_valid_broadcast_sample_rate(uint32_t sample_rate);
    
    /**
     * @brief Generate compliance summary dashboard
     * @param results Vector of compliance results
     * @return Dashboard summary string
     */
    std::string generate_compliance_dashboard(const std::vector<BroadcastComplianceResult>& results);
}

} // namespace broadcast
} // namespace etsi

#endif // ETSI_BROADCAST_STANDARDS_H