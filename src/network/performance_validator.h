/**
 * @file performance_validator.h
 * @brief Performance Target Validation and ETSI Compliance Engine
 * 
 * Implements comprehensive validation framework for:
 * - Real-time performance target validation (>7,482 FPS target)
 * - ETSI EN 300 799/401 compliance verification
 * - Network streaming quality assurance
 * - Professional broadcast standards compliance
 * - Automated performance testing and benchmarking
 * - Continuous compliance monitoring
 * 
 * @author Network/Stream Agent
 * @date 2025-09-21
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#ifndef PERFORMANCE_VALIDATOR_H
#define PERFORMANCE_VALIDATOR_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <unordered_map>

#include "../core/eti_types.h"
#include "eti_network_integration.h"

namespace eti_network {

/**
 * @brief Performance target definitions for validation
 */
struct PerformanceTargets {
    // Processing performance targets
    double target_fps = 7482.0;                       // Target processing FPS (>7,482 actual)
    std::chrono::milliseconds max_latency{50};        // Maximum processing latency
    double min_efficiency = 0.95;                     // Minimum processing efficiency
    size_t max_frame_drops = 10;                      // Maximum consecutive frame drops
    
    // Memory and resource targets
    double max_memory_mb = 100.0;                     // Maximum memory usage (4MB actual)
    double max_cpu_usage = 0.8;                       // Maximum CPU usage (80%)
    double max_disk_io_mbps = 50.0;                   // Maximum disk I/O rate
    double max_network_bandwidth_mbps = 100.0;        // Maximum network bandwidth
    
    // Quality targets
    double min_signal_quality = 0.8;                  // Minimum signal quality
    double max_error_rate = 0.001;                    // Maximum error rate (0.1%)
    double min_etsi_compliance_score = 0.95;          // Minimum ETSI compliance score
    size_t max_compliance_violations = 5;             // Maximum compliance violations
    
    // Network streaming targets
    double max_packet_loss = 0.01;                    // Maximum packet loss (1%)
    std::chrono::milliseconds max_jitter{10};         // Maximum network jitter
    double min_network_reliability = 0.99;           // Minimum network reliability
    size_t max_buffer_overruns = 3;                   // Maximum buffer overruns
    
    // Real-time constraints
    std::chrono::microseconds max_frame_processing_time{1000}; // Max frame processing time
    std::chrono::milliseconds max_startup_time{2000}; // Maximum startup time
    std::chrono::milliseconds max_recovery_time{5000}; // Maximum error recovery time
    double min_real_time_ratio = 0.98;                // Minimum real-time processing ratio
    
    QJsonObject to_json() const {
        QJsonObject obj;
        obj["target_fps"] = target_fps;
        obj["max_latency_ms"] = static_cast<int>(max_latency.count());
        obj["min_efficiency"] = min_efficiency;
        obj["max_frame_drops"] = static_cast<int>(max_frame_drops);
        obj["max_memory_mb"] = max_memory_mb;
        obj["max_cpu_usage"] = max_cpu_usage;
        obj["max_disk_io_mbps"] = max_disk_io_mbps;
        obj["max_network_bandwidth_mbps"] = max_network_bandwidth_mbps;
        obj["min_signal_quality"] = min_signal_quality;
        obj["max_error_rate"] = max_error_rate;
        obj["min_etsi_compliance_score"] = min_etsi_compliance_score;
        obj["max_compliance_violations"] = static_cast<int>(max_compliance_violations);
        return obj;
    }
};

/**
 * @brief ETSI compliance validation rules
 */
struct ETSIComplianceRules {
    // ETSI EN 300 799 rules (ETI specification)
    bool validate_eti_sync_pattern = true;            // Validate ETI sync pattern
    bool validate_eti_frame_size = true;              // Validate 6144-byte frame size
    bool validate_eti_lidata_field = true;            // Validate LIDATA field structure
    bool validate_eti_crc = true;                     // Validate ETI CRC
    bool validate_eti_timing = true;                  // Validate ETI timing (24ms frames)
    
    // ETSI EN 300 401 rules (DAB specification)
    bool validate_fic_structure = true;               // Validate FIC structure
    bool validate_fig_blocks = true;                  // Validate FIG block structure
    bool validate_ensemble_info = true;               // Validate ensemble information
    bool validate_service_organization = true;        // Validate service organization
    bool validate_subchannel_organization = true;     // Validate subchannel organization
    
    // DAB+ specific rules
    bool validate_audio_encoding = true;              // Validate audio encoding parameters
    bool validate_protection_levels = true;           // Validate protection levels
    bool validate_capacity_allocation = true;         // Validate capacity allocation
    bool validate_service_linking = true;             // Validate service linking
    
    // Network streaming rules
    bool validate_rtp_headers = true;                 // Validate RTP headers (if applicable)
    bool validate_multicast_addressing = true;        // Validate multicast addressing
    bool validate_timing_synchronization = true;      // Validate timing synchronization
    bool validate_redundancy_handling = true;         // Validate redundancy handling
    
    // Quality thresholds for compliance
    double min_fic_success_rate = 0.95;               // Minimum FIC decode success rate
    double min_msc_success_rate = 0.98;               // Minimum MSC decode success rate
    size_t max_continuity_errors = 5;                 // Maximum continuity errors
    size_t max_crc_errors = 10;                       // Maximum CRC errors
    
    QJsonObject to_json() const {
        QJsonObject obj;
        obj["validate_eti_sync_pattern"] = validate_eti_sync_pattern;
        obj["validate_eti_frame_size"] = validate_eti_frame_size;
        obj["validate_eti_lidata_field"] = validate_eti_lidata_field;
        obj["validate_eti_crc"] = validate_eti_crc;
        obj["validate_eti_timing"] = validate_eti_timing;
        obj["validate_fic_structure"] = validate_fic_structure;
        obj["validate_fig_blocks"] = validate_fig_blocks;
        obj["validate_ensemble_info"] = validate_ensemble_info;
        obj["validate_service_organization"] = validate_service_organization;
        obj["validate_subchannel_organization"] = validate_subchannel_organization;
        obj["min_fic_success_rate"] = min_fic_success_rate;
        obj["min_msc_success_rate"] = min_msc_success_rate;
        obj["max_continuity_errors"] = static_cast<int>(max_continuity_errors);
        obj["max_crc_errors"] = static_cast<int>(max_crc_errors);
        return obj;
    }
};

/**
 * @brief Validation result for a specific test
 */
struct ValidationResult {
    QString test_name;                                 // Test identifier
    QString test_category;                             // Test category
    bool passed = false;                               // Test passed status
    double measured_value = 0.0;                      // Measured value
    double target_value = 0.0;                        // Target value
    double deviation_percent = 0.0;                   // Deviation percentage
    QString error_message;                             // Error message if failed
    QDateTime test_timestamp;                          // Test execution timestamp
    std::chrono::milliseconds test_duration{0};       // Test execution duration
    QJsonObject additional_data;                       // Additional test data
    
    double get_performance_ratio() const {
        if (target_value == 0.0) return 1.0;
        return measured_value / target_value;
    }
    
    bool meets_threshold(double threshold_ratio = 0.9) const {
        return get_performance_ratio() >= threshold_ratio;
    }
    
    QJsonObject to_json() const {
        QJsonObject obj;
        obj["test_name"] = test_name;
        obj["test_category"] = test_category;
        obj["passed"] = passed;
        obj["measured_value"] = measured_value;
        obj["target_value"] = target_value;
        obj["deviation_percent"] = deviation_percent;
        obj["error_message"] = error_message;
        obj["test_timestamp"] = test_timestamp.toString(Qt::ISODate);
        obj["test_duration_ms"] = static_cast<int>(test_duration.count());
        obj["additional_data"] = additional_data;
        return obj;
    }
};

/**
 * @brief Comprehensive validation report
 */
struct ValidationReport {
    QString report_id;                                 // Unique report identifier
    QDateTime report_timestamp;                        // Report generation timestamp
    std::chrono::milliseconds validation_duration{0}; // Total validation duration
    
    // Overall results
    size_t total_tests = 0;                           // Total number of tests
    size_t passed_tests = 0;                          // Number of passed tests
    size_t failed_tests = 0;                          // Number of failed tests
    double overall_pass_rate = 0.0;                   // Overall pass rate
    
    // Performance validation results
    std::vector<ValidationResult> performance_results; // Performance test results
    bool performance_targets_met = false;              // Performance targets status
    double performance_score = 0.0;                   // Performance score (0.0-1.0)
    
    // ETSI compliance results
    std::vector<ValidationResult> compliance_results;  // Compliance test results
    bool etsi_compliant = false;                       // ETSI compliance status
    double compliance_score = 0.0;                    // Compliance score (0.0-1.0)
    QStringList compliance_violations;                // List of compliance violations
    
    // Network streaming results
    std::vector<ValidationResult> network_results;     // Network test results
    bool network_requirements_met = false;            // Network requirements status
    double network_quality_score = 0.0;               // Network quality score (0.0-1.0)
    
    // Resource utilization results
    std::vector<ValidationResult> resource_results;    // Resource test results
    bool resource_constraints_met = false;            // Resource constraints status
    double resource_efficiency = 0.0;                 // Resource efficiency (0.0-1.0)
    
    // Recommendations and actions
    QStringList recommendations;                       // Performance recommendations
    QStringList required_actions;                     // Required corrective actions
    QStringList optimization_suggestions;             // Optimization suggestions
    
    bool is_production_ready() const {
        return performance_targets_met && 
               etsi_compliant && 
               network_requirements_met && 
               resource_constraints_met &&
               overall_pass_rate >= 0.95;
    }
    
    QString get_summary() const {
        return QString("Validation Report: %1/%2 tests passed (%.1f%%), "
                      "Performance: %3, ETSI: %4, Network: %5, Resources: %6")
               .arg(passed_tests).arg(total_tests).arg(overall_pass_rate * 100)
               .arg(performance_targets_met ? "PASS" : "FAIL")
               .arg(etsi_compliant ? "COMPLIANT" : "NON-COMPLIANT")
               .arg(network_requirements_met ? "PASS" : "FAIL")
               .arg(resource_constraints_met ? "PASS" : "FAIL");
    }
    
    QJsonObject to_json() const {
        QJsonObject obj;
        obj["report_id"] = report_id;
        obj["report_timestamp"] = report_timestamp.toString(Qt::ISODate);
        obj["validation_duration_ms"] = static_cast<int>(validation_duration.count());
        obj["total_tests"] = static_cast<int>(total_tests);
        obj["passed_tests"] = static_cast<int>(passed_tests);
        obj["failed_tests"] = static_cast<int>(failed_tests);
        obj["overall_pass_rate"] = overall_pass_rate;
        obj["performance_targets_met"] = performance_targets_met;
        obj["performance_score"] = performance_score;
        obj["etsi_compliant"] = etsi_compliant;
        obj["compliance_score"] = compliance_score;
        obj["network_requirements_met"] = network_requirements_met;
        obj["network_quality_score"] = network_quality_score;
        obj["resource_constraints_met"] = resource_constraints_met;
        obj["resource_efficiency"] = resource_efficiency;
        obj["is_production_ready"] = is_production_ready();
        obj["summary"] = get_summary();
        return obj;
    }
};

/**
 * @brief Performance benchmark executor for systematic testing
 */
class PerformanceBenchmark : public QObject {
    Q_OBJECT
    
public:
    explicit PerformanceBenchmark(QObject* parent = nullptr);
    ~PerformanceBenchmark();
    
    // Benchmark configuration
    void setPerformanceTargets(const PerformanceTargets& targets);
    PerformanceTargets getPerformanceTargets() const;
    void setBenchmarkDuration(std::chrono::seconds duration);
    void setWarmupDuration(std::chrono::seconds duration);
    
    // Benchmark execution
    bool runPerformanceBenchmark();
    bool runLatencyBenchmark();
    bool runThroughputBenchmark();
    bool runMemoryBenchmark();
    bool runStressBenchmark();
    
    // Results access
    std::vector<ValidationResult> getBenchmarkResults() const;
    ValidationResult getLatestResult(const QString& testName) const;
    bool areTargetsMet() const;
    double getOverallPerformanceScore() const;
    
signals:
    void benchmarkStarted(const QString& testName);
    void benchmarkCompleted(const QString& testName, const ValidationResult& result);
    void benchmarkFailed(const QString& testName, const QString& error);
    void benchmarkProgress(const QString& testName, int percentage);
    
private slots:
    void updateBenchmarkProgress();
    
private:
    void initializeBenchmark();
    ValidationResult measureFrameProcessingRate();
    ValidationResult measureProcessingLatency();
    ValidationResult measureMemoryUsage();
    ValidationResult measureResourceUtilization();
    ValidationResult runStressTest();
    
    PerformanceTargets m_targets;
    std::vector<ValidationResult> m_results;
    std::chrono::seconds m_benchmarkDuration{30};
    std::chrono::seconds m_warmupDuration{5};
    
    std::unique_ptr<QTimer> m_progressTimer;
    std::atomic<bool> m_benchmarkRunning{false};
    
    static constexpr std::chrono::milliseconds PROGRESS_UPDATE_INTERVAL{1000};
};

/**
 * @brief ETSI compliance validator for standards verification
 */
class ETSIComplianceValidator : public QObject {
    Q_OBJECT
    
public:
    explicit ETSIComplianceValidator(QObject* parent = nullptr);
    ~ETSIComplianceValidator();
    
    // Compliance configuration
    void setComplianceRules(const ETSIComplianceRules& rules);
    ETSIComplianceRules getComplianceRules() const;
    void enableStandardValidation(const QString& standard, bool enabled);
    
    // Frame-level validation
    ValidationResult validateETIFrame(const eti::EtiFrame& frame);
    ValidationResult validateFICStructure(const eti::EtiFicField& fic);
    ValidationResult validateMSCStructure(const eti::EtiMscField& msc);
    ValidationResult validateEnsembleInfo(const eti::Ensemble& ensemble);
    ValidationResult validateServiceInfo(const eti::DabService& service);
    
    // Stream-level validation
    bool runComplianceValidation(const QString& streamSource);
    std::vector<ValidationResult> validateStreamCompliance(const QByteArray& streamData);
    ValidationResult validateStreamContinuity(const std::vector<eti::EtiFrame>& frames);
    ValidationResult validateTimingCompliance(const std::vector<eti::EtiFrame>& frames);
    
    // Compliance reporting
    std::vector<ValidationResult> getComplianceResults() const;
    QStringList getComplianceViolations() const;
    double getComplianceScore() const;
    bool isETSICompliant() const;
    
signals:
    void complianceValidationStarted();
    void complianceValidationCompleted(double complianceScore);
    void complianceViolationDetected(const QString& violation, const QString& standard);
    void frameValidationCompleted(const ValidationResult& result);
    
private:
    // EN 300 799 validation methods
    bool validateETISyncPattern(const eti::EtiFrame& frame);
    bool validateETIFrameSize(const eti::EtiFrame& frame);
    bool validateLIDATAField(const eti::EtiLidataField& lidata);
    bool validateETICRC(const eti::EtiFrame& frame);
    bool validateETITiming(const eti::EtiFrame& frame, const eti::EtiFrame& previousFrame);
    
    // EN 300 401 validation methods
    bool validateFICStructure(const eti::EtiFicField& fic);
    bool validateFIGBlocks(const std::vector<eti::FigBlock>& figBlocks);
    bool validateEnsembleConfiguration(const eti::Ensemble& ensemble);
    bool validateServiceConfiguration(const eti::DabService& service);
    bool validateSubchannelConfiguration(const eti::SubChannelInfo& subchannel);
    
    // Advanced validation methods
    bool validateProtectionLevels(const eti::DabService& service);
    bool validateCapacityAllocation(const eti::Ensemble& ensemble);
    bool validateServiceLinking(const std::vector<eti::DabService>& services);
    
    ETSIComplianceRules m_rules;
    std::vector<ValidationResult> m_complianceResults;
    QStringList m_complianceViolations;
    
    mutable QMutex m_validationMutex;
    std::atomic<double> m_complianceScore{1.0};
    std::atomic<bool> m_etsiCompliant{true};
};

/**
 * @brief Professional Performance Validator
 * 
 * Comprehensive validation framework implementing:
 * - Real-time performance target validation (>7,482 FPS)
 * - ETSI EN 300 799/401 compliance verification
 * - Network streaming quality assurance
 * - Professional broadcast standards compliance
 * - Automated benchmarking and continuous monitoring
 * - Production readiness assessment
 */
class PerformanceValidator : public QObject {
    Q_OBJECT
    
public:
    explicit PerformanceValidator(QObject* parent = nullptr);
    explicit PerformanceValidator(const PerformanceTargets& targets, 
                                const ETSIComplianceRules& rules, 
                                QObject* parent = nullptr);
    ~PerformanceValidator();
    
    // Configuration management
    void setPerformanceTargets(const PerformanceTargets& targets);
    PerformanceTargets getPerformanceTargets() const;
    void setComplianceRules(const ETSIComplianceRules& rules);
    ETSIComplianceRules getComplianceRules() const;
    
    // Validation execution
    bool runFullValidation();
    bool runPerformanceValidation();
    bool runComplianceValidation();
    bool runNetworkValidation();
    bool runResourceValidation();
    
    // Continuous monitoring
    void startContinuousValidation();
    void stopContinuousValidation();
    bool isContinuousValidationActive() const;
    void setContinuousValidationInterval(std::chrono::seconds interval);
    
    // Integration validation
    bool validateIntegrationPerformance(EtiNetworkIntegration* integration);
    bool validateStreamingConfiguration(const IntegrationConfig& config);
    bool validateNetworkConfiguration(const ETIStreamInfo& streamInfo);
    bool validateEquipmentConfiguration(const EquipmentConfig& equipmentConfig);
    
    // Results and reporting
    ValidationReport getLatestReport() const;
    std::vector<ValidationReport> getValidationHistory() const;
    bool isProductionReady() const;
    double getOverallScore() const;
    QStringList getRecommendations() const;
    
    // Real-time monitoring
    void monitorPerformance(const IntegrationStatistics& stats);
    void monitorCompliance(const eti::EtiFrame& frame);
    void monitorNetworkQuality(const NetworkQualityMetrics& metrics);
    void monitorResourceUsage(double cpu, double memory, double diskIO, double networkBW);
    
    // Export and reporting
    void exportReport(const QString& filename, const QString& format = "JSON") const;
    void exportBenchmarkData(const QString& filename) const;
    QString generateExecutiveSummary() const;
    QString generateTechnicalReport() const;
    QJsonObject generateComplianceReport() const;
    
    // Threshold management
    void setValidationThresholds(double performanceThreshold, double complianceThreshold, 
                               double networkThreshold, double resourceThreshold);
    void enableAdaptiveThresholds(bool enabled);
    void resetValidationCounters();

signals:
    void validationStarted(const QString& validationType);
    void validationCompleted(const ValidationReport& report);
    void validationFailed(const QString& validationType, const QString& error);
    void performanceTargetViolated(const QString& target, double currentValue, double targetValue);
    void complianceViolationDetected(const QString& violation, const QString& standard);
    void networkQualityDegraded(double qualityScore, const QString& reason);
    void resourceConstraintViolated(const QString& resource, double usage, double limit);
    void productionReadinessChanged(bool ready);
    void validationReportGenerated(const ValidationReport& report);
    void continuousValidationStatusChanged(bool active);

private slots:
    void performContinuousValidation();
    void handleBenchmarkCompleted(const QString& testName, const ValidationResult& result);
    void handleComplianceValidationCompleted(double complianceScore);
    void updateValidationStatistics();

private:
    // Initialization and setup
    void initializeValidator();
    void setupValidationComponents();
    void cleanupValidator();
    
    // Validation execution methods
    ValidationReport executeFullValidation();
    std::vector<ValidationResult> executePerformanceTests();
    std::vector<ValidationResult> executeComplianceTests();
    std::vector<ValidationResult> executeNetworkTests();
    std::vector<ValidationResult> executeResourceTests();
    
    // Analysis and scoring
    double calculateOverallScore(const ValidationReport& report);
    void generateRecommendations(ValidationReport& report);
    void assessProductionReadiness(ValidationReport& report);
    void updateValidationTrends();
    
    // Threshold management
    void checkPerformanceThresholds(const ValidationResult& result);
    void checkComplianceThresholds(const ValidationResult& result);
    void adaptThresholds();
    
    // Core validation components
    std::unique_ptr<PerformanceBenchmark> m_performanceBenchmark;
    std::unique_ptr<ETSIComplianceValidator> m_complianceValidator;
    
    // Configuration
    PerformanceTargets m_performanceTargets;
    ETSIComplianceRules m_complianceRules;
    
    // Validation state
    ValidationReport m_latestReport;
    std::vector<ValidationReport> m_validationHistory;
    std::atomic<bool> m_continuousValidationActive{false};
    std::chrono::seconds m_continuousValidationInterval{60};
    
    // Thresholds
    double m_performanceThreshold = 0.9;
    double m_complianceThreshold = 0.95;
    double m_networkThreshold = 0.8;
    double m_resourceThreshold = 0.8;
    bool m_adaptiveThresholds = false;
    
    // Monitoring and timers
    std::unique_ptr<QTimer> m_continuousTimer;
    std::unique_ptr<QTimer> m_statisticsTimer;
    
    // Statistics and trends
    mutable QMutex m_validationMutex;
    std::atomic<bool> m_productionReady{false};
    std::atomic<double> m_overallScore{0.0};
    
    static constexpr size_t MAX_VALIDATION_HISTORY = 100;
    static constexpr std::chrono::milliseconds STATISTICS_UPDATE_INTERVAL{5000};
};

} // namespace eti_network

#endif // PERFORMANCE_VALIDATOR_H