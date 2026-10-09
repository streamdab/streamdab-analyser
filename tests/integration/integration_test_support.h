#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <chrono>

/**
 * @file integration_test_support.h
 * @brief Integration test support for UI/UX Agent and Build Manager validation
 * 
 * Provides comprehensive test data and validation utilities for:
 * - UI/UX Agent GUI component testing
 * - Build Manager Agent integration validation
 * - Standards Compliance Agent ETSI validation
 * - Cross-agent integration scenarios
 */

namespace IntegrationTestSupport {

// Forward declarations
class EtsiEtiFrame;
struct TestScenario;
struct ValidationResults;

/**
 * @brief UI/UX Agent GUI Testing Support
 */
namespace GuiTestSupport {
    
    /**
     * @brief GUI test data configuration
     */
    struct GuiTestConfig {
        // Visual testing scenarios
        bool include_ensemble_display_test = true;
        bool include_service_list_test = true;
        bool include_audio_visualization_test = true;
        bool include_signal_quality_test = true;
        bool include_error_display_test = true;
        
        // User interaction scenarios
        bool include_service_selection_test = true;
        bool include_settings_dialog_test = true;
        bool include_file_operations_test = true;
        bool include_network_config_test = true;
        
        // Real-time display testing
        bool include_realtime_updates = true;
        bool include_performance_display = true;
        bool include_status_indicators = true;
        
        // Error handling scenarios
        bool include_error_recovery_test = true;
        bool include_invalid_data_handling = true;
        bool include_connection_failure_test = true;
    };
    
    /**
     * @brief Generate test data for GUI ensemble display testing
     * @param config GUI test configuration
     * @return GUI-optimized test scenarios
     */
    struct GuiEnsembleTestData {
        // Ensemble information for display
        struct EnsembleDisplayData {
            uint16_t ensemble_id;
            std::string ensemble_label;
            std::string country;
            uint8_t service_count;
            std::vector<std::string> service_labels;
            std::vector<bool> service_active_states;
            std::vector<double> service_signal_qualities;
        };
        
        // Multiple ensembles for testing
        std::vector<EnsembleDisplayData> ensembles;
        
        // Test scenarios
        std::vector<EtsiEtiFrame> normal_operation_frames;
        std::vector<EtsiEtiFrame> service_switching_frames;
        std::vector<EtsiEtiFrame> signal_quality_change_frames;
        std::vector<EtsiEtiFrame> error_recovery_frames;
    };
    
    GuiEnsembleTestData generate_gui_ensemble_test_data(const GuiTestConfig& config = GuiTestConfig{});
    
    /**
     * @brief Generate test data for service browser widget
     * @return Service browser test scenarios
     */
    struct ServiceBrowserTestData {
        struct ServiceInfo {
            uint16_t service_id;
            std::string service_label;
            std::string service_type; // "Audio", "Data", "Programme"
            uint8_t sub_channel_id;
            uint16_t bit_rate;
            bool is_dab_plus;
            double signal_quality;
            bool is_available;
        };
        
        std::vector<ServiceInfo> services;
        std::vector<EtsiEtiFrame> test_frames;
        
        // Dynamic scenarios
        std::vector<ServiceInfo> services_appearing; // Services coming online
        std::vector<ServiceInfo> services_disappearing; // Services going offline
        std::vector<ServiceInfo> services_quality_changing; // Quality fluctuations
    };
    
    ServiceBrowserTestData generate_service_browser_test_data();
    
    /**
     * @brief Generate test data for constellation display widget
     * @return Constellation visualization test data
     */
    struct ConstellationTestData {
        struct ConstellationPoint {
            double i_value; // In-phase component
            double q_value; // Quadrature component
            double magnitude;
            double phase;
        };
        
        // Constellation patterns for different scenarios
        std::vector<ConstellationPoint> clean_signal_constellation;
        std::vector<ConstellationPoint> noisy_signal_constellation;
        std::vector<ConstellationPoint> interference_constellation;
        std::vector<ConstellationPoint> multipath_constellation;
        
        // Dynamic constellation data
        std::vector<std::vector<ConstellationPoint>> constellation_sequence;
        
        // Associated ETI frames
        std::vector<EtsiEtiFrame> corresponding_frames;
    };
    
    ConstellationTestData generate_constellation_test_data();
    
    /**
     * @brief Generate test data for audio analyser widget
     * @return Audio analysis test data
     */
    struct AudioAnalyserTestData {
        struct AudioSpectrum {
            std::vector<double> frequencies; // Hz
            std::vector<double> magnitudes;  // dB
            double peak_frequency;
            double total_power;
            double snr_db;
        };
        
        // Different audio content spectrums
        AudioSpectrum music_spectrum;
        AudioSpectrum speech_spectrum;
        AudioSpectrum silence_spectrum;
        AudioSpectrum noise_spectrum;
        
        // Time-based audio data
        std::vector<AudioSpectrum> spectrum_sequence;
        
        // Audio quality metrics
        struct AudioQualityMetrics {
            double bit_rate;
            double sample_rate;
            int channels;
            double dynamic_range;
            double thd_plus_n; // Total Harmonic Distortion + Noise
            std::string codec_type; // "DAB", "DAB+"
        };
        
        std::vector<AudioQualityMetrics> quality_metrics;
        
        // Associated test frames
        std::vector<EtsiEtiFrame> audio_test_frames;
    };
    
    AudioAnalyserTestData generate_audio_analyser_test_data();
    
    /**
     * @brief Generate test scenarios for user interactions
     * @return User interaction test scenarios
     */
    struct UserInteractionTestData {
        // File operation scenarios
        std::vector<std::string> test_eti_file_paths;
        std::vector<std::string> invalid_file_paths;
        std::vector<std::string> large_file_paths;
        
        // Network configuration scenarios
        struct NetworkConfigScenario {
            std::string scenario_name;
            std::string multicast_address;
            uint16_t port;
            bool should_succeed;
            std::string expected_error;
        };
        
        std::vector<NetworkConfigScenario> network_scenarios;
        
        // Settings dialog scenarios
        struct SettingsScenario {
            std::string scenario_name;
            std::map<std::string, std::string> settings;
            bool should_apply_successfully;
            std::vector<std::string> expected_warnings;
        };
        
        std::vector<SettingsScenario> settings_scenarios;
    };
    
    UserInteractionTestData generate_user_interaction_test_data();
    
    /**
     * @brief GUI performance test data
     * @return Performance-focused GUI test data
     */
    struct GuiPerformanceTestData {
        // High-frequency update scenarios
        std::vector<EtsiEtiFrame> rapid_update_frames; // For testing UI refresh rates
        
        // Large dataset scenarios
        std::vector<EtsiEtiFrame> large_ensemble_frames; // Many services
        
        // Memory usage scenarios
        std::vector<EtsiEtiFrame> memory_stress_frames; // Long-running display
        
        // Performance metrics
        struct GuiPerformanceExpectations {
            std::chrono::milliseconds max_update_latency{50}; // 50ms max UI update
            double max_cpu_usage = 15.0; // 15% CPU for GUI updates
            size_t max_memory_mb = 128;   // 128MB for GUI components
            double min_refresh_rate = 30.0; // 30 FPS minimum for smooth display
        } expectations;
    };
    
    GuiPerformanceTestData generate_gui_performance_test_data();
    
} // namespace GuiTestSupport

/**
 * @brief Build Manager Integration Test Support
 */
namespace BuildIntegrationSupport {
    
    /**
     * @brief Build system test configuration
     */
    struct BuildTestConfig {
        // Build target testing
        bool test_debug_build = true;
        bool test_release_build = true;
        bool test_coverage_build = true;
        bool test_sanitizer_builds = true;
        
        // Platform testing
        bool test_linux_build = true;
        bool test_windows_build = true;
        bool test_macos_build = true;
        
        // Dependency testing
        bool test_external_dependencies = true;
        bool test_static_linking = true;
        bool test_dynamic_linking = true;
        
        // Quality assurance
        bool run_static_analysis = true;
        bool run_memory_leak_tests = true;
        bool run_performance_regression = true;
        bool run_compliance_validation = true;
    };
    
    /**
     * @brief Generate test data for build validation
     * @param config Build test configuration
     * @return Build validation test data
     */
    struct BuildValidationTestData {
        // Unit test data for build verification
        std::vector<EtsiEtiFrame> unit_test_frames;
        
        // Integration test data
        std::vector<EtsiEtiFrame> integration_test_frames;
        
        // Performance test data for regression detection
        std::vector<EtsiEtiFrame> performance_regression_frames;
        
        // Compliance test data
        std::vector<EtsiEtiFrame> etsi_compliance_frames;
        
        // Memory leak detection data
        std::vector<EtsiEtiFrame> memory_leak_test_frames;
        
        // Expected test results
        struct ExpectedResults {
            size_t expected_test_count;
            size_t expected_pass_count;
            double expected_coverage_percentage;
            std::chrono::seconds max_build_time{300}; // 5 minutes max
            std::chrono::seconds max_test_time{600};  // 10 minutes max
        } expected_results;
    };
    
    BuildValidationTestData generate_build_validation_data(const BuildTestConfig& config = BuildTestConfig{});
    
    /**
     * @brief Generate CI/CD pipeline test data
     * @return CI/CD integration test data
     */
    struct CiCdTestData {
        // Automated test scenarios
        std::vector<EtsiEtiFrame> smoke_test_frames;     // Quick validation
        std::vector<EtsiEtiFrame> regression_test_frames; // Full regression suite
        std::vector<EtsiEtiFrame> nightly_test_frames;   // Extended nightly tests
        
        // Build matrix scenarios
        struct BuildMatrix {
            std::string platform;
            std::string compiler;
            std::string build_type;
            std::vector<std::string> cmake_options;
            bool should_succeed;
            std::vector<std::string> expected_warnings;
        };
        
        std::vector<BuildMatrix> build_matrix_scenarios;
        
        // Performance benchmarks for regression detection
        struct PerformanceBenchmark {
            std::string benchmark_name;
            double baseline_fps;
            double acceptable_regression_percentage;
            std::vector<EtsiEtiFrame> benchmark_frames;
        };
        
        std::vector<PerformanceBenchmark> performance_benchmarks;
    };
    
    CiCdTestData generate_cicd_test_data();
    
    /**
     * @brief Generate test data for code coverage validation
     * @return Coverage test data
     */
    struct CoverageTestData {
        // Test scenarios designed to exercise specific code paths
        std::vector<EtsiEtiFrame> core_functionality_frames;
        std::vector<EtsiEtiFrame> error_handling_frames;
        std::vector<EtsiEtiFrame> edge_case_frames;
        std::vector<EtsiEtiFrame> integration_path_frames;
        
        // Expected coverage targets
        struct CoverageTargets {
            double line_coverage_target = 85.0;     // 85% line coverage
            double function_coverage_target = 90.0; // 90% function coverage
            double branch_coverage_target = 80.0;   // 80% branch coverage
            
            // Critical components requiring higher coverage
            std::map<std::string, double> component_coverage_targets = {
                {"eti_processor", 95.0},
                {"fic_decoder", 90.0},
                {"audio_decoder", 85.0},
                {"network_receiver", 85.0}
            };
        } targets;
    };
    
    CoverageTestData generate_coverage_test_data();
    
} // namespace BuildIntegrationSupport

/**
 * @brief Standards Compliance Test Support
 */
namespace ComplianceTestSupport {
    
    /**
     * @brief ETSI compliance test configuration
     */
    struct ComplianceTestConfig {
        // Standard versions to test
        bool test_etsi_en_300_799_v1_2_1 = true;
        bool test_etsi_en_300_401_v2_1_1 = true;
        bool test_etsi_ts_102_563_v1_2_1 = true;
        
        // Compliance test levels
        enum class ComplianceLevel {
            BASIC_STRUCTURE,     // Basic frame structure compliance
            PROTOCOL_COMPLIANCE, // Full protocol compliance
            BROADCAST_QUALITY   // Broadcast industry quality standards
        } compliance_level = ComplianceLevel::BROADCAST_QUALITY;
        
        // Test coverage
        bool test_frame_structure = true;
        bool test_fig_compliance = true;
        bool test_timing_compliance = true;
        bool test_error_handling = true;
        bool test_audio_quality = true;
    };
    
    /**
     * @brief Generate ETSI compliance test suite
     * @param config Compliance test configuration
     * @return Comprehensive compliance test data
     */
    struct EtsiComplianceTestData {
        // Frame structure compliance tests
        std::vector<EtsiEtiFrame> valid_structure_frames;
        std::vector<EtsiEtiFrame> invalid_structure_frames;
        
        // FIG compliance tests
        std::vector<EtsiEtiFrame> fig_compliance_frames;
        std::vector<EtsiEtiFrame> fig_error_frames;
        
        // Timing compliance tests
        std::vector<EtsiEtiFrame> timing_compliant_frames;
        std::vector<EtsiEtiFrame> timing_violation_frames;
        
        // Audio quality compliance
        std::vector<EtsiEtiFrame> audio_quality_frames;
        
        // Expected compliance results
        struct ComplianceExpectations {
            double expected_compliance_score = 0.98; // 98% compliance
            size_t max_acceptable_violations = 5;
            std::vector<std::string> acceptable_warnings = {
                "Minor FIC padding optimization",
                "Non-critical timing jitter"
            };
        } expectations;
    };
    
    EtsiComplianceTestData generate_etsi_compliance_test_data(
        const ComplianceTestConfig& config = ComplianceTestConfig{}
    );
    
    /**
     * @brief Generate broadcast industry compliance test data
     * @return Industry standard compliance tests
     */
    struct BroadcastComplianceTestData {
        // Professional broadcast requirements
        std::vector<EtsiEtiFrame> professional_quality_frames;
        
        // Reliability requirements
        std::vector<EtsiEtiFrame> reliability_test_frames;
        
        // Interoperability tests
        std::vector<EtsiEtiFrame> interoperability_frames;
        
        // Real-world scenario tests
        std::vector<EtsiEtiFrame> field_condition_frames;
        
        // Industry benchmarks
        struct IndustryBenchmarks {
            double min_availability = 99.9;        // 99.9% availability
            double max_error_rate = 0.001;         // 0.1% error rate
            std::chrono::hours max_recovery_time{1}; // 1 hour max recovery
            double min_audio_quality = 4.5;        // ITU-R BS.1116 scale
        } benchmarks;
    };
    
    BroadcastComplianceTestData generate_broadcast_compliance_test_data();
    
} // namespace ComplianceTestSupport

/**
 * @brief Cross-Agent Integration Test Support
 */
namespace CrossAgentIntegration {
    
    /**
     * @brief Integration scenario configuration
     */
    struct IntegrationScenarioConfig {
        // Agent combinations to test
        bool test_tester_build_integration = true;
        bool test_ui_network_integration = true;
        bool test_compliance_all_agents = true;
        bool test_performance_all_agents = true;
        
        // Workflow scenarios
        bool test_development_workflow = true;
        bool test_deployment_workflow = true;
        bool test_maintenance_workflow = true;
        bool test_emergency_workflow = true;
    };
    
    /**
     * @brief Generate end-to-end integration test scenarios
     * @param config Integration scenario configuration
     * @return Complete integration test suite
     */
    struct EndToEndTestSuite {
        // Development workflow tests
        struct DevelopmentWorkflow {
            std::vector<EtsiEtiFrame> tdd_cycle_frames;      // Tester Agent
            std::vector<EtsiEtiFrame> build_validation_frames; // Build Manager
            std::vector<EtsiEtiFrame> gui_integration_frames;  // UI/UX Agent
            std::vector<EtsiEtiFrame> compliance_check_frames; // Standards Agent
            std::vector<EtsiEtiFrame> stream_processing_frames; // Network Agent
        } development_workflow;
        
        // Production deployment tests
        struct ProductionDeployment {
            std::vector<EtsiEtiFrame> system_validation_frames;
            std::vector<EtsiEtiFrame> performance_verification_frames;
            std::vector<EtsiEtiFrame> compliance_certification_frames;
            std::vector<EtsiEtiFrame> user_acceptance_frames;
        } production_deployment;
        
        // Maintenance and monitoring tests
        struct MaintenanceWorkflow {
            std::vector<EtsiEtiFrame> health_monitoring_frames;
            std::vector<EtsiEtiFrame> performance_monitoring_frames;
            std::vector<EtsiEtiFrame> error_detection_frames;
            std::vector<EtsiEtiFrame> system_update_frames;
        } maintenance_workflow;
        
        // Emergency response tests
        struct EmergencyResponse {
            std::vector<EtsiEtiFrame> failure_detection_frames;
            std::vector<EtsiEtiFrame> error_recovery_frames;
            std::vector<EtsiEtiFrame> backup_system_frames;
            std::vector<EtsiEtiFrame> service_restoration_frames;
        } emergency_response;
    };
    
    EndToEndTestSuite generate_end_to_end_test_suite(
        const IntegrationScenarioConfig& config = IntegrationScenarioConfig{}
    );
    
    /**
     * @brief Generate agent handoff test scenarios
     * @return Agent communication test data
     */
    struct AgentHandoffTestData {
        // Data flow between agents
        struct DataFlow {
            std::string source_agent;
            std::string target_agent;
            std::vector<EtsiEtiFrame> handoff_frames;
            std::string data_format;
            bool should_succeed;
        };
        
        std::vector<DataFlow> agent_data_flows;
        
        // Synchronization tests
        std::vector<EtsiEtiFrame> synchronization_test_frames;
        
        // Communication protocol tests
        std::vector<EtsiEtiFrame> protocol_validation_frames;
    };
    
    AgentHandoffTestData generate_agent_handoff_test_data();
    
} // namespace CrossAgentIntegration

/**
 * @brief Integration test validation utilities
 */
namespace IntegrationValidation {
    
    /**
     * @brief Validation result structure
     */
    struct ValidationResults {
        bool overall_success = false;
        double success_percentage = 0.0;
        
        // Component-specific results
        std::map<std::string, bool> component_results;
        std::map<std::string, std::vector<std::string>> component_errors;
        
        // Performance validation
        bool meets_performance_requirements = false;
        std::map<std::string, double> performance_metrics;
        
        // Compliance validation
        bool meets_compliance_requirements = false;
        std::vector<std::string> compliance_violations;
        
        // Integration health
        bool agent_communication_healthy = false;
        bool data_flow_intact = false;
        bool error_handling_functional = false;
        
        // Recommendations
        std::vector<std::string> improvement_recommendations;
        std::vector<std::string> critical_issues;
        
        /**
         * @brief Generate comprehensive validation report
         * @return Formatted validation report
         */
        std::vector<std::string> generate_comprehensive_report() const;
    };
    
    /**
     * @brief Run complete integration validation
     * @param gui_data GUI test data
     * @param build_data Build integration data
     * @param compliance_data Compliance test data
     * @param integration_data Cross-agent integration data
     * @return Comprehensive validation results
     */
    ValidationResults run_complete_validation(
        const GuiTestSupport::GuiEnsembleTestData& gui_data,
        const BuildIntegrationSupport::BuildValidationTestData& build_data,
        const ComplianceTestSupport::EtsiComplianceTestData& compliance_data,
        const CrossAgentIntegration::EndToEndTestSuite& integration_data
    );
    
    /**
     * @brief Quick integration health check
     * @return Basic integration status
     */
    bool quick_integration_health_check();
    
    /**
     * @brief Generate integration test summary for project management
     * @param results Validation results
     * @return Executive summary
     */
    std::vector<std::string> generate_executive_summary(const ValidationResults& results);
    
} // namespace IntegrationValidation

} // namespace IntegrationTestSupport