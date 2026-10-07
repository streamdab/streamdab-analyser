/**
 * @file test_audio_quality_optimization.cpp
 * @brief Audio Quality Threshold Optimization Validation Tests
 * 
 * Comprehensive test suite for validating audio quality threshold optimizations
 * that prevent false positive violations while maintaining broadcast standards.
 * 
 * @author TDD Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>
#include <vector>
#include <memory>
#include <random>
#include <algorithm>
#include <numeric>

// Core system headers
#include "../../src/core/etsi/alert_system.h"
#include "../../src/core/etsi/compliance_engine.h"
#include "../../src/core/etsi/broadcast_standards.h"

// Audio processing headers
#include "../../src/core/audio/audio_analyser.h"
#include "../../src/core/audio/loudness_meter.h"
#include "../../src/core/audio/quality_monitor.h"

using namespace etsi::alerts;
using namespace etsi::compliance;
using namespace etsi::broadcast;
using namespace audio;

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

/**
 * @brief Audio data structure for testing
 */
struct AudioTestData {
    std::vector<float> samples;
    double sample_rate = 48000.0;
    int channels = 2;
    double duration_seconds = 0.0;
    
    // Calculated metrics
    double lufs_level = 0.0;
    double peak_level = 0.0;
    double rms_level = 0.0;
    double dynamic_range = 0.0;
    double stereo_correlation = 0.0;
    double silence_duration = 0.0;
    double thd_percent = 0.0;
    
    // Quality flags
    bool has_clipping = false;
    bool has_overmodulation = false;
    bool has_dc_offset = false;
    bool has_phase_issues = false;
};

/**
 * @brief Test fixture for audio quality optimization validation
 */
class AudioQualityOptimizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure optimized alert system with broadcast tolerance
        AlertSystemConfig config;
        config.enable_broadcast_tolerance = true;
        config.enable_operational_flexibility = true;
        
        // Optimized audio thresholds
        config.audio_thresholds.lufs_target = -23.0;              // EBU R128 target
        config.audio_thresholds.lufs_warning_deviation = 3.0;     // ±3 LUFS tolerance
        config.audio_thresholds.lufs_critical_deviation = 6.0;    // ±6 LUFS critical
        config.audio_thresholds.peak_warning_level = 0.9;         // -0.9 dBFS
        config.audio_thresholds.peak_critical_level = 1.0;        // 0 dBFS
        config.audio_thresholds.silence_warning_duration = 5.0;   // 5 seconds
        config.audio_thresholds.silence_critical_duration = 10.0; // 10 seconds
        config.audio_thresholds.dynamic_range_minimum = 6.0;      // 6 dB minimum
        config.audio_thresholds.thd_warning_percent = 1.0;        // 1% THD
        config.audio_thresholds.thd_critical_percent = 3.0;       // 3% THD
        
        // Broadcast tolerance settings
        config.broadcast_tolerance.enable_lufs_flexibility = true;
        config.broadcast_tolerance.lufs_tolerance_range = 1.5;    // ±1.5 LUFS extra
        config.broadcast_tolerance.enable_peak_tolerance = true;
        config.broadcast_tolerance.peak_tolerance_db = 1.0;       // 1 dB extra headroom
        config.broadcast_tolerance.enable_context_awareness = true;
        
        alert_system_ = std::make_unique<EtsiAlertSystem>(config);
        ASSERT_TRUE(alert_system_->initialize());
        
        // Configure audio analyser
        AudioAnalyserConfig analyser_config;
        analyser_config.enable_broadcast_mode = true;
        analyser_config.enable_ebu_r128_measurement = true;
        analyser_config.enable_quality_analysis = true;
        analyser_config.measurement_window_seconds = 3.0;
        
        audio_analyser_ = std::make_unique<AudioAnalyser>(analyser_config);
        ASSERT_TRUE(audio_analyser_->initialize());
        
        // Configure loudness meter
        LoudnessMeterConfig loudness_config;
        loudness_config.standard = LoudnessStandard::EBU_R128;
        loudness_config.gate_threshold = -70.0; // dB
        loudness_config.integration_time = 3.0; // seconds
        
        loudness_meter_ = std::make_unique<LoudnessMeter>(loudness_config);
        ASSERT_TRUE(loudness_meter_->initialize());
        
        // Configure quality monitor
        QualityMonitorConfig quality_config;
        quality_config.enable_broadcast_quality_checks = true;
        quality_config.enable_realtime_monitoring = true;
        quality_config.quality_assessment_interval = 1.0; // seconds
        
        quality_monitor_ = std::make_unique<QualityMonitor>(quality_config);
        ASSERT_TRUE(quality_monitor_->initialize());
    }
    
    void TearDown() override {
        // Print test summary
        std::cout << "Audio quality optimization test completed" << std::endl;
    }
    
    /**
     * @brief Generate broadcast-quality test audio
     */
    AudioTestData CreateBroadcastQualityAudio(double duration_seconds = 1.0,
                                             double target_lufs = -23.0,
                                             double frequency = 1000.0) {
        AudioTestData audio;
        audio.duration_seconds = duration_seconds;
        audio.sample_rate = 48000.0;
        audio.channels = 2;
        
        size_t num_samples = static_cast<size_t>(duration_seconds * audio.sample_rate * audio.channels);
        audio.samples.resize(num_samples);
        
        // Calculate RMS level for target LUFS
        // Simplified conversion: LUFS ≈ RMS(dB) - 0.691
        double target_rms_db = target_lufs + 0.691;
        double target_rms_linear = std::pow(10.0, target_rms_db / 20.0);
        
        // Generate stereo sine wave
        for (size_t i = 0; i < num_samples; i += 2) {
            double t = (i / 2) / audio.sample_rate;
            float sample = static_cast<float>(target_rms_linear * std::sin(2.0 * M_PI * frequency * t));
            
            audio.samples[i] = sample;     // Left channel
            audio.samples[i + 1] = sample; // Right channel
        }
        
        // Calculate actual metrics
        calculate_audio_metrics(audio);
        
        return audio;
    }
    
    /**
     * @brief Generate audio with specific characteristics for testing
     */
    AudioTestData CreateTestAudio(double lufs_level,
                                 double peak_level,
                                 double silence_duration = 0.0,
                                 bool add_clipping = false) {
        AudioTestData audio;
        audio.duration_seconds = 3.0; // 3 second test samples
        audio.sample_rate = 48000.0;
        audio.channels = 2;
        
        size_t num_samples = static_cast<size_t>(audio.duration_seconds * audio.sample_rate * audio.channels);
        audio.samples.resize(num_samples);
        
        // Calculate RMS level for target LUFS
        double target_rms_db = lufs_level + 0.691;
        double target_rms_linear = std::pow(10.0, target_rms_db / 20.0);
        
        // Apply peak scaling
        double peak_scale = peak_level;
        if (target_rms_linear > peak_level) {
            target_rms_linear = peak_level * 0.7; // Maintain some headroom
        }
        
        // Generate audio content
        size_t silence_samples = static_cast<size_t>(silence_duration * audio.sample_rate * audio.channels);
        
        for (size_t i = 0; i < num_samples; i += 2) {
            double t = (i / 2) / audio.sample_rate;
            float sample = 0.0f;
            
            if (i < silence_samples) {
                // Silence period
                sample = 0.0f;
            } else {
                // Audio content
                sample = static_cast<float>(target_rms_linear * std::sin(2.0 * M_PI * 1000.0 * t));
                
                // Apply peak scaling
                sample *= static_cast<float>(peak_scale);
                
                // Add clipping if requested
                if (add_clipping && std::abs(sample) > 0.95f) {
                    sample = (sample > 0) ? 1.0f : -1.0f;
                }
            }
            
            audio.samples[i] = sample;     // Left channel
            audio.samples[i + 1] = sample; // Right channel
        }
        
        calculate_audio_metrics(audio);
        
        return audio;
    }
    
    /**
     * @brief Calculate comprehensive audio metrics
     */
    void calculate_audio_metrics(AudioTestData& audio) {
        if (audio.samples.empty()) return;
        
        // Analyze with our audio analyser
        auto analysis_result = audio_analyser_->analyze(audio.samples, 
                                                       audio.sample_rate, 
                                                       audio.channels);
        
        audio.lufs_level = analysis_result.loudness_lufs;
        audio.peak_level = analysis_result.peak_level;
        audio.rms_level = analysis_result.rms_level;
        audio.dynamic_range = analysis_result.dynamic_range;
        audio.stereo_correlation = analysis_result.stereo_correlation;
        audio.silence_duration = analysis_result.silence_duration_seconds;
        audio.thd_percent = analysis_result.thd_percent;
        
        audio.has_clipping = analysis_result.has_clipping;
        audio.has_overmodulation = analysis_result.has_overmodulation;
        audio.has_dc_offset = analysis_result.has_dc_offset;
        audio.has_phase_issues = analysis_result.has_phase_issues;
    }
    
    std::unique_ptr<EtsiAlertSystem> alert_system_;
    std::unique_ptr<AudioAnalyser> audio_analyser_;
    std::unique_ptr<LoudnessMeter> loudness_meter_;
    std::unique_ptr<QualityMonitor> quality_monitor_;
};

/**
 * @brief Test broadcast-quality audio passes without warnings
 */
TEST_F(AudioQualityOptimizationTest, testBroadcastQualityAudioNoWarnings) {
    // Create perfect broadcast audio at EBU R128 target
    auto perfect_audio = CreateBroadcastQualityAudio(3.0, -23.0, 1000.0);
    
    // Analyze audio quality
    auto alerts = alert_system_->check_audio_quality(perfect_audio);
    
    // VALIDATE: No warnings or errors for perfect broadcast audio
    for (const auto& alert : alerts) {
        if (alert.category == AlertCategory::AUDIO_QUALITY) {
            EXPECT_LE(static_cast<int>(alert.severity), static_cast<int>(AlertSeverity::INFO))
                << "Unexpected alert for perfect broadcast audio: " << alert.message;
        }
    }
    
    // VALIDATE: Audio metrics within broadcast standards
    EXPECT_NEAR(perfect_audio.lufs_level, -23.0, 0.5); // ±0.5 LUFS accuracy
    EXPECT_LT(perfect_audio.peak_level, 0.9);           // Good headroom
    EXPECT_GT(perfect_audio.dynamic_range, 10.0);       // Good dynamic range
    EXPECT_FALSE(perfect_audio.has_clipping);
    EXPECT_FALSE(perfect_audio.has_overmodulation);
    
    std::cout << "Perfect Broadcast Audio Metrics:" << std::endl;
    std::cout << "  LUFS: " << perfect_audio.lufs_level << " LUFS" << std::endl;
    std::cout << "  Peak: " << (20 * std::log10(perfect_audio.peak_level)) << " dBFS" << std::endl;
    std::cout << "  Dynamic Range: " << perfect_audio.dynamic_range << " dB" << std::endl;
    std::cout << "  Alerts generated: " << alerts.size() << std::endl;
}

/**
 * @brief Test optimized LUFS tolerance prevents false positives
 */
TEST_F(AudioQualityOptimizationTest, testOptimizedLUFSTolerancePrevention) {
    struct LUFSTestCase {
        double lufs_level;
        std::string description;
        bool should_warn;
    };
    
    std::vector<LUFSTestCase> test_cases = {
        {-20.0, "3 dB above target (within optimized tolerance)", false},
        {-26.0, "3 dB below target (within optimized tolerance)", false},
        {-19.5, "3.5 dB above target (within broadcast tolerance)", false},
        {-26.5, "3.5 dB below target (within broadcast tolerance)", false},
        {-18.0, "5 dB above target (should warn)", true},
        {-28.0, "5 dB below target (should warn)", true},
        {-16.0, "7 dB above target (should be critical)", true},
        {-30.0, "7 dB below target (should be critical)", true}
    };
    
    for (const auto& test_case : test_cases) {
        auto test_audio = CreateTestAudio(test_case.lufs_level, 0.8); // Good peak level
        auto alerts = alert_system_->check_audio_quality(test_audio);
        
        // Check for LUFS-related alerts
        bool has_lufs_warning = false;
        bool has_lufs_critical = false;
        
        for (const auto& alert : alerts) {
            if (alert.category == AlertCategory::AUDIO_QUALITY && 
                alert.message.find("LUFS") != std::string::npos) {
                if (alert.severity == AlertSeverity::WARNING) has_lufs_warning = true;
                if (alert.severity == AlertSeverity::CRITICAL) has_lufs_critical = true;
            }
        }
        
        bool has_significant_alert = has_lufs_warning || has_lufs_critical;
        
        if (test_case.should_warn) {
            EXPECT_TRUE(has_significant_alert) 
                << "Expected warning for " << test_case.description 
                << " (LUFS: " << test_case.lufs_level << ")";
        } else {
            EXPECT_FALSE(has_significant_alert) 
                << "Unexpected warning for " << test_case.description 
                << " (LUFS: " << test_case.lufs_level << ")";
        }
        
        std::cout << test_case.description << " - LUFS: " << test_case.lufs_level 
                  << " - Alerts: " << (has_significant_alert ? "YES" : "NO") << std::endl;
    }
}

/**
 * @brief Test optimized peak level tolerance
 */
TEST_F(AudioQualityOptimizationTest, testOptimizedPeakLevelTolerance) {
    struct PeakTestCase {
        double peak_level;
        std::string description;
        bool should_warn;
    };
    
    std::vector<PeakTestCase> test_cases = {
        {0.8, "Good headroom (-1.9 dBFS)", false},
        {0.89, "Close to warning threshold (-1.0 dBFS)", false},
        {0.95, "Within tolerance range (-0.4 dBFS)", false},
        {0.98, "Very close to clipping (-0.2 dBFS)", true},
        {1.0, "At clipping threshold (0 dBFS)", true},
        {1.05, "Over clipping (+0.4 dBFS)", true}
    };
    
    for (const auto& test_case : test_cases) {
        auto test_audio = CreateTestAudio(-23.0, test_case.peak_level); // Good LUFS level
        auto alerts = alert_system_->check_audio_quality(test_audio);
        
        // Check for peak-related alerts
        bool has_peak_warning = false;
        bool has_peak_critical = false;
        
        for (const auto& alert : alerts) {
            if (alert.category == AlertCategory::AUDIO_QUALITY && 
                (alert.message.find("peak") != std::string::npos || 
                 alert.message.find("clipping") != std::string::npos)) {
                if (alert.severity == AlertSeverity::WARNING) has_peak_warning = true;
                if (alert.severity == AlertSeverity::CRITICAL) has_peak_critical = true;
            }
        }
        
        bool has_significant_alert = has_peak_warning || has_peak_critical;
        
        if (test_case.should_warn) {
            EXPECT_TRUE(has_significant_alert) 
                << "Expected warning for " << test_case.description 
                << " (Peak: " << test_case.peak_level << ")";
        } else {
            EXPECT_FALSE(has_significant_alert) 
                << "Unexpected warning for " << test_case.description 
                << " (Peak: " << test_case.peak_level << ")";
        }
        
        double peak_db = (test_case.peak_level > 0) ? 20 * std::log10(test_case.peak_level) : -60.0;
        std::cout << test_case.description << " - Peak: " << peak_db << " dBFS"
                  << " - Alerts: " << (has_significant_alert ? "YES" : "NO") << std::endl;
    }
}

/**
 * @brief Test optimized silence detection tolerance
 */
TEST_F(AudioQualityOptimizationTest, testOptimizedSilenceDetectionTolerance) {
    struct SilenceTestCase {
        double silence_duration;
        std::string description;
        bool should_warn;
    };
    
    std::vector<SilenceTestCase> test_cases = {
        {0.0, "No silence", false},
        {2.0, "Short silence (2s)", false},
        {4.8, "Just under warning threshold (4.8s)", false},
        {5.2, "Just over warning threshold (5.2s)", true},
        {7.0, "Moderate silence (7s)", true},
        {9.8, "Just under critical threshold (9.8s)", true},
        {10.2, "Just over critical threshold (10.2s)", true},
        {15.0, "Long silence (15s)", true}
    };
    
    for (const auto& test_case : test_cases) {
        auto test_audio = CreateTestAudio(-23.0, 0.8, test_case.silence_duration);
        auto alerts = alert_system_->check_audio_quality(test_audio);
        
        // Check for silence-related alerts
        bool has_silence_warning = false;
        bool has_silence_critical = false;
        
        for (const auto& alert : alerts) {
            if (alert.category == AlertCategory::AUDIO_QUALITY && 
                alert.message.find("silence") != std::string::npos) {
                if (alert.severity == AlertSeverity::WARNING) has_silence_warning = true;
                if (alert.severity == AlertSeverity::CRITICAL) has_silence_critical = true;
            }
        }
        
        bool has_significant_alert = has_silence_warning || has_silence_critical;
        
        if (test_case.should_warn) {
            EXPECT_TRUE(has_significant_alert) 
                << "Expected warning for " << test_case.description 
                << " (Silence: " << test_case.silence_duration << "s)";
        } else {
            EXPECT_FALSE(has_significant_alert) 
                << "Unexpected warning for " << test_case.description 
                << " (Silence: " << test_case.silence_duration << "s)";
        }
        
        std::cout << test_case.description << " - Duration: " << test_case.silence_duration << "s"
                  << " - Alerts: " << (has_significant_alert ? "YES" : "NO") << std::endl;
    }
}

/**
 * @brief Test context-aware audio quality assessment
 */
TEST_F(AudioQualityOptimizationTest, testContextAwareQualityAssessment) {
    // Test different audio content types with context awareness
    struct ContextTest {
        std::string content_type;
        double lufs_level;
        double expected_tolerance;
    };
    
    std::vector<ContextTest> context_tests = {
        {"music", -23.0, 3.0},        // Standard tolerance for music
        {"speech", -20.0, 4.0},       // Higher tolerance for speech content
        {"commercial", -18.0, 2.0},   // Stricter tolerance for commercials
        {"classical", -26.0, 5.0},    // Higher tolerance for classical music
        {"live_event", -21.0, 4.0}    // Higher tolerance for live events
    };
    
    for (const auto& test : context_tests) {
        // Set content context
        alert_system_->set_content_context(test.content_type);
        
        auto test_audio = CreateTestAudio(test.lufs_level, 0.8);
        auto alerts = alert_system_->check_audio_quality(test_audio);
        
        // Calculate deviation from -23 LUFS target
        double lufs_deviation = std::abs(test.lufs_level - (-23.0));
        
        bool has_lufs_warning = false;
        for (const auto& alert : alerts) {
            if (alert.category == AlertCategory::AUDIO_QUALITY && 
                alert.message.find("LUFS") != std::string::npos &&
                alert.severity >= AlertSeverity::WARNING) {
                has_lufs_warning = true;
                break;
            }
        }
        
        // Should not warn if deviation is within context-specific tolerance
        if (lufs_deviation <= test.expected_tolerance) {
            EXPECT_FALSE(has_lufs_warning) 
                << "Unexpected warning for " << test.content_type 
                << " content (deviation: " << lufs_deviation << " LUFS)";
        }
        
        std::cout << "Context: " << test.content_type 
                  << " - LUFS: " << test.lufs_level 
                  << " - Deviation: " << lufs_deviation 
                  << " - Tolerance: " << test.expected_tolerance
                  << " - Warning: " << (has_lufs_warning ? "YES" : "NO") << std::endl;
    }
    
    // Reset context
    alert_system_->clear_content_context();
}

/**
 * @brief Test real-world audio scenarios
 */
TEST_F(AudioQualityOptimizationTest, testRealWorldAudioScenarios) {
    struct RealWorldScenario {
        std::string name;
        double lufs_level;
        double peak_level;
        double silence_duration;
        bool add_clipping;
        bool should_be_acceptable;
    };
    
    std::vector<RealWorldScenario> scenarios = {
        {"Typical broadcast music", -22.5, 0.85, 0.0, false, true},
        {"Quiet classical piece", -25.5, 0.7, 0.0, false, true},
        {"Loud commercial", -18.5, 0.92, 0.0, false, true},  // Within broadcast tolerance
        {"Speech with pauses", -21.0, 0.8, 3.0, false, true},
        {"Live sports commentary", -20.0, 0.88, 1.0, false, true},
        {"Over-compressed pop", -16.0, 0.95, 0.0, false, false}, // Too loud
        {"Badly mastered track", -15.0, 1.05, 0.0, true, false},  // Clipping
        {"Technical difficulties", -10.0, 0.3, 8.0, false, false} // Too quiet + long silence
    };
    
    for (const auto& scenario : scenarios) {
        auto test_audio = CreateTestAudio(scenario.lufs_level, 
                                        scenario.peak_level,
                                        scenario.silence_duration,
                                        scenario.add_clipping);
        
        auto alerts = alert_system_->check_audio_quality(test_audio);
        
        // Count significant alerts (warning or critical)
        int significant_alerts = 0;
        for (const auto& alert : alerts) {
            if (alert.category == AlertCategory::AUDIO_QUALITY && 
                alert.severity >= AlertSeverity::WARNING) {
                significant_alerts++;
            }
        }
        
        if (scenario.should_be_acceptable) {
            EXPECT_EQ(significant_alerts, 0) 
                << "Unexpected alerts for acceptable scenario: " << scenario.name;
        } else {
            EXPECT_GT(significant_alerts, 0) 
                << "Expected alerts for problematic scenario: " << scenario.name;
        }
        
        std::cout << "Scenario: " << scenario.name 
                  << " - LUFS: " << scenario.lufs_level
                  << " - Peak: " << (20 * std::log10(scenario.peak_level)) << " dBFS"
                  << " - Alerts: " << significant_alerts
                  << " - " << (scenario.should_be_acceptable ? "ACCEPTABLE" : "PROBLEMATIC") << std::endl;
    }
}

/**
 * @brief Test adaptive threshold adjustment
 */
TEST_F(AudioQualityOptimizationTest, testAdaptiveThresholdAdjustment) {
    // Test adaptive thresholds based on content analysis
    alert_system_->enable_adaptive_thresholds(true);
    
    // Feed system with various audio samples to establish baseline
    std::vector<AudioTestData> training_samples;
    for (int i = 0; i < 50; ++i) {
        double lufs = -23.0 + (static_cast<double>(rand()) / RAND_MAX - 0.5) * 4.0; // ±2 LUFS variation
        double peak = 0.7 + (static_cast<double>(rand()) / RAND_MAX) * 0.2; // 0.7-0.9 peak range
        
        auto sample = CreateTestAudio(lufs, peak);
        training_samples.push_back(sample);
        
        // Feed to alert system for learning
        alert_system_->process_audio_for_adaptation(sample);
    }
    
    // Allow adaptation to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test that thresholds have adapted
    auto adapted_thresholds = alert_system_->get_current_thresholds();
    auto original_thresholds = alert_system_->get_default_thresholds();
    
    // Thresholds should have adapted based on training data
    EXPECT_NE(adapted_thresholds.lufs_warning_deviation, original_thresholds.lufs_warning_deviation);
    
    // Test audio that would have triggered alerts before adaptation
    auto test_audio = CreateTestAudio(-25.0, 0.85); // Slightly quiet but within training range
    auto alerts_after_adaptation = alert_system_->check_audio_quality(test_audio);
    
    // Should have fewer alerts after adaptation
    int warnings_after = 0;
    for (const auto& alert : alerts_after_adaptation) {
        if (alert.severity >= AlertSeverity::WARNING) warnings_after++;
    }
    
    // Disable adaptation and test same audio
    alert_system_->enable_adaptive_thresholds(false);
    alert_system_->reset_to_default_thresholds();
    
    auto alerts_without_adaptation = alert_system_->check_audio_quality(test_audio);
    int warnings_without = 0;
    for (const auto& alert : alerts_without_adaptation) {
        if (alert.severity >= AlertSeverity::WARNING) warnings_without++;
    }
    
    // Adaptation should reduce false positives
    EXPECT_LE(warnings_after, warnings_without) 
        << "Adaptation should reduce false positive alerts";
    
    std::cout << "Adaptive Thresholds Test:" << std::endl;
    std::cout << "  Warnings without adaptation: " << warnings_without << std::endl;
    std::cout << "  Warnings with adaptation: " << warnings_after << std::endl;
    std::cout << "  Improvement: " << (warnings_without - warnings_after) << " fewer warnings" << std::endl;
}

/**
 * @brief Test broadcast standards compliance validation
 */
TEST_F(AudioQualityOptimizationTest, testBroadcastStandardsCompliance) {
    // Test compliance with various broadcast standards
    std::vector<std::string> standards = {"EBU_R128", "ATSC_A85", "ITU_BS1770"};
    
    for (const auto& standard : standards) {
        alert_system_->set_broadcast_standard(standard);
        
        // Test audio that should be compliant with all standards
        auto compliant_audio = CreateBroadcastQualityAudio(5.0, -23.0, 1000.0);
        auto alerts = alert_system_->check_audio_quality(compliant_audio);
        
        // Should have no warnings for compliant audio regardless of standard
        int warnings = 0;
        for (const auto& alert : alerts) {
            if (alert.severity >= AlertSeverity::WARNING) warnings++;
        }
        
        EXPECT_EQ(warnings, 0) 
            << "Compliant audio should not trigger warnings for standard: " << standard;
        
        std::cout << "Standard: " << standard 
                  << " - Compliant audio warnings: " << warnings << std::endl;
    }
}

/**
 * @brief Main test execution
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "\n🎵 AUDIO QUALITY OPTIMIZATION VALIDATION TESTS" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << "Testing optimized audio thresholds preventing false positives..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    int result = RUN_ALL_TESTS();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n📊 AUDIO OPTIMIZATION TEST SUMMARY" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "Total execution time: " << total_time.count() << " ms" << std::endl;
    
    if (result == 0) {
        std::cout << "\n✅ ALL AUDIO TESTS PASSED - OPTIMIZATION VALIDATED" << std::endl;
        std::cout << "🎯 BROADCAST TOLERANCE STANDARDS MAINTAINED" << std::endl;
    } else {
        std::cout << "\n❌ AUDIO OPTIMIZATION TESTS FAILED - REQUIRES ATTENTION" << std::endl;
    }
    
    return result;
}