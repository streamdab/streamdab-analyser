/**
 * @file compliance_reporter.h
 * @brief Professional ETSI Compliance Reporting System
 * 
 * Comprehensive compliance reporting framework for ETSI standards validation
 * with professional report generation, service quality assessment, regulatory
 * documentation, and audit trail capabilities for broadcast industry applications.
 * 
 * Features:
 * - Professional PDF/HTML/XML report generation
 * - ETSI standards compliance validation summaries
 * - Service quality assessment documentation
 * - Regulatory compliance audit trails
 * - Real-time monitoring dashboards
 * - Cross-platform compliance export formats
 * - Integration with professional broadcast monitoring
 * 
 * Report Types:
 * - Daily Operations Reports
 * - Compliance Audit Reports
 * - Service Quality Reports
 * - Emergency Alert Documentation
 * - Regulatory Filing Reports
 * - Performance Analysis Reports
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#ifndef ETSI_COMPLIANCE_REPORTER_H
#define ETSI_COMPLIANCE_REPORTER_H

#include "compliance_engine.h"
#include "compliance_logger.h"
#include "alert_system.h"
#include "broadcast_standards.h"
#include "../eti_types.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <sstream>

namespace etsi {
namespace reporting {

/**
 * @brief Professional report format types
 */
enum class ReportFormat : uint8_t {
    TEXT = 1,           // Plain text format
    HTML = 2,           // HTML web format with CSS styling
    PDF = 3,            // Professional PDF report
    XML = 4,            // Structured XML format
    JSON = 5,           // JSON data format
    CSV = 6,            // Comma-separated values
    EXCEL = 7,          // Microsoft Excel format
    REGULATORY = 8      // Regulatory-specific format
};

/**
 * @brief Report categories for different use cases
 */
enum class ReportCategory : uint8_t {
    DAILY_OPERATIONS = 1,       // Daily operations summary
    COMPLIANCE_AUDIT = 2,       // Compliance audit report
    SERVICE_QUALITY = 3,        // Service quality assessment
    EMERGENCY_ALERTS = 4,       // Emergency alert documentation
    REGULATORY_FILING = 5,      // Regulatory filing report
    PERFORMANCE_ANALYSIS = 6,   // Performance analysis report
    VIOLATION_SUMMARY = 7,      // Compliance violations summary
    MAINTENANCE_LOG = 8,        // Maintenance activities log
    CONFIGURATION_AUDIT = 9,    // Configuration audit trail
    INCIDENT_REPORT = 10        // Incident investigation report
};

/**
 * @brief Report scope and timeframe configuration
 */
struct ReportScope {
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    
    // Entity filtering
    std::vector<uint32_t> service_ids;      // Specific services to include
    std::vector<uint32_t> ensemble_ids;     // Specific ensembles to include
    std::vector<std::string> session_ids;   // Specific sessions to include
    
    // Standards filtering
    std::vector<compliance::EtsiStandard> included_standards;
    std::vector<alerts::AlertCategory> included_alert_categories;
    std::vector<logging::LogCategory> included_log_categories;
    
    // Severity filtering
    alerts::AlertSeverity min_alert_severity = alerts::AlertSeverity::INFO;
    logging::LogSeverity min_log_severity = logging::LogSeverity::INFO;
    
    // Content filtering
    bool include_compliance_details = true;
    bool include_performance_metrics = true;
    bool include_audio_quality_data = true;
    bool include_service_availability = true;
    bool include_error_statistics = true;
    bool include_configuration_changes = false;
    bool include_debug_information = false;
    
    ReportScope() {
        // Default to last 24 hours
        end_time = std::chrono::system_clock::now();
        start_time = end_time - std::chrono::hours{24};
        
        // Include all standards by default
        included_standards = {
            compliance::EtsiStandard::EN_300_401,
            compliance::EtsiStandard::EN_302_077,
            compliance::EtsiStandard::EN_300_799,
            compliance::EtsiStandard::TS_102_563,
            compliance::EtsiStandard::TS_101_756
        };
    }
};

/**
 * @brief Report template configuration
 */
struct ReportTemplate {
    std::string template_name;
    std::string organization_name;
    std::string organization_logo_path;
    std::string report_title_template;
    std::string header_template;
    std::string footer_template;
    
    // Styling options
    std::string css_style_path;             // For HTML reports
    std::string pdf_template_path;          // For PDF reports
    std::map<std::string, std::string> custom_fields;
    
    // Regulatory requirements
    std::string regulatory_authority;       // e.g., "FCC", "Ofcom", "CRTC"
    std::string license_number;
    std::string facility_id;
    std::string contact_information;
    
    ReportTemplate() = default;
};

/**
 * @brief Compliance metrics summary for reporting
 */
struct ComplianceMetricsSummary {
    // Overall compliance statistics
    double overall_compliance_percentage = 0.0;
    uint32_t total_frames_analyzed = 0;
    uint32_t compliant_frames = 0;
    uint32_t violation_frames = 0;
    
    // Standards-specific compliance
    std::map<compliance::EtsiStandard, double> standard_compliance_percentages;
    std::map<compliance::EtsiStandard, uint32_t> standard_violation_counts;
    
    // Service quality metrics
    struct ServiceQuality {
        double availability_percentage = 0.0;
        double audio_quality_score = 0.0;
        uint32_t service_interruptions = 0;
        std::chrono::seconds total_downtime{0};
        double average_lufs = 0.0;
        double peak_level_max = 0.0;
    };
    std::map<uint32_t, ServiceQuality> service_quality_metrics;
    
    // Error correction statistics
    struct ErrorStats {
        uint32_t fic_errors = 0;
        uint32_t msc_errors = 0;
        uint32_t reed_solomon_failures = 0;
        uint32_t sync_losses = 0;
        double error_rate_percentage = 0.0;
    };
    ErrorStats error_statistics;
    
    // Alert statistics
    struct AlertStats {
        uint32_t total_alerts = 0;
        uint32_t critical_alerts = 0;
        uint32_t emergency_alerts = 0;
        uint32_t resolved_alerts = 0;
        std::chrono::seconds average_resolution_time{0};
    };
    AlertStats alert_statistics;
    
    // Performance metrics
    struct PerformanceStats {
        std::chrono::microseconds average_processing_time{0};
        std::chrono::microseconds max_processing_time{0};
        double cpu_usage_average = 0.0;
        double memory_usage_average_mb = 0.0;
        uint32_t processing_errors = 0;
    };
    PerformanceStats performance_statistics;
    
    std::chrono::system_clock::time_point measurement_start;
    std::chrono::system_clock::time_point measurement_end;
    std::chrono::seconds measurement_duration{0};
    
    ComplianceMetricsSummary() {
        measurement_start = std::chrono::system_clock::now();
        measurement_end = measurement_start;
    }
};

/**
 * @brief Individual report generation result
 */
struct ReportGenerationResult {
    bool success = false;
    std::string report_content;
    std::string report_filename;
    ReportFormat format;
    ReportCategory category;
    size_t report_size_bytes = 0;
    std::chrono::milliseconds generation_time{0};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    
    std::chrono::system_clock::time_point generation_timestamp;
    std::string generator_version;
    std::string checksum;
    
    ReportGenerationResult() : generation_timestamp(std::chrono::system_clock::now()) {}
};

// Forward declarations
class ReportGenerator;
class PdfGenerator;
class HtmlGenerator;
class XmlGenerator;
class RegulatoryFormatter;

/**
 * @brief Professional ETSI Compliance Reporting System
 * 
 * Comprehensive reporting framework for generating professional compliance
 * reports, audit documentation, and regulatory filing materials with full
 * ETSI standards validation and broadcast industry compliance.
 */
class EtsiComplianceReporter {
public:
    /**
     * @brief Reporter configuration
     */
    struct Config {
        std::string reports_directory = "./reports";
        std::string templates_directory = "./templates";
        std::string temp_directory = "./temp";
        
        ReportTemplate default_template;
        
        // Report generation settings
        bool enable_auto_generation = true;
        std::chrono::hours daily_report_time{6};        // 6 AM daily reports
        std::chrono::minutes report_cache_duration{30}; // 30 minute cache
        size_t max_report_history = 1000;               // Keep 1000 reports
        
        // Performance settings
        size_t max_concurrent_generations = 3;
        std::chrono::seconds generation_timeout{300};   // 5 minute timeout
        bool enable_report_compression = true;
        
        Config() = default;
    };
    
    explicit EtsiComplianceReporter(const Config& config = Config{});
    ~EtsiComplianceReporter();
    
    // Disable copy/move for thread safety
    EtsiComplianceReporter(const EtsiComplianceReporter&) = delete;
    EtsiComplianceReporter& operator=(const EtsiComplianceReporter&) = delete;
    EtsiComplianceReporter(EtsiComplianceReporter&&) = delete;
    EtsiComplianceReporter& operator=(EtsiComplianceReporter&&) = delete;
    
    /**
     * @brief Initialize the compliance reporter
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown the compliance reporter
     */
    void shutdown();
    
    /**
     * @brief Generate comprehensive compliance report
     * @param scope Report scope and filtering configuration
     * @param format Desired report format
     * @param template_name Report template to use
     * @return Report generation result
     */
    ReportGenerationResult generate_compliance_report(
        const ReportScope& scope,
        ReportFormat format = ReportFormat::HTML,
        const std::string& template_name = "default"
    );
    
    /**
     * @brief Generate daily operations report
     * @param date Date for the daily report
     * @param format Report format
     * @return Report generation result
     */
    ReportGenerationResult generate_daily_operations_report(
        const std::chrono::system_clock::time_point& date,
        ReportFormat format = ReportFormat::PDF
    );
    
    /**
     * @brief Generate service quality assessment report
     * @param service_ids Service IDs to include in report
     * @param timeframe Assessment timeframe
     * @param format Report format
     * @return Report generation result
     */
    ReportGenerationResult generate_service_quality_report(
        const std::vector<uint32_t>& service_ids,
        std::chrono::hours timeframe = std::chrono::hours{24},
        ReportFormat format = ReportFormat::HTML
    );
    
    /**
     * @brief Generate compliance audit report
     * @param audit_period Audit period timeframe
     * @param standards Standards to audit
     * @param format Report format
     * @return Report generation result
     */
    ReportGenerationResult generate_audit_report(
        std::chrono::hours audit_period = std::chrono::hours{24 * 30}, // 30 days
        const std::vector<compliance::EtsiStandard>& standards = {},
        ReportFormat format = ReportFormat::PDF
    );
    
    /**
     * @brief Generate emergency alerts documentation
     * @param timeframe Documentation timeframe
     * @param format Report format
     * @return Report generation result
     */
    ReportGenerationResult generate_emergency_alerts_report(
        std::chrono::hours timeframe = std::chrono::hours{24 * 7}, // 7 days
        ReportFormat format = ReportFormat::PDF
    );
    
    /**
     * @brief Generate regulatory filing report
     * @param regulation_name Regulation name
     * @param filing_period Filing period
     * @param format Report format
     * @return Report generation result
     */
    ReportGenerationResult generate_regulatory_filing_report(
        const std::string& regulation_name,
        std::chrono::hours filing_period = std::chrono::hours{24 * 30}, // 30 days
        ReportFormat format = ReportFormat::REGULATORY
    );
    
    /**
     * @brief Generate performance analysis report
     * @param analysis_period Analysis period
     * @param include_benchmarks Include performance benchmarks
     * @param format Report format
     * @return Report generation result
     */
    ReportGenerationResult generate_performance_report(
        std::chrono::hours analysis_period = std::chrono::hours{24 * 7}, // 7 days
        bool include_benchmarks = true,
        ReportFormat format = ReportFormat::HTML
    );
    
    /**
     * @brief Generate compliance violations summary
     * @param summary_period Summary period
     * @param min_severity Minimum violation severity
     * @param format Report format
     * @return Report generation result
     */
    ReportGenerationResult generate_violations_summary(
        std::chrono::hours summary_period = std::chrono::hours{24},
        alerts::AlertSeverity min_severity = alerts::AlertSeverity::WARNING,
        ReportFormat format = ReportFormat::HTML
    );
    
    /**
     * @brief Collect compliance metrics for reporting
     * @param scope Data collection scope
     * @return Compliance metrics summary
     */
    ComplianceMetricsSummary collect_compliance_metrics(const ReportScope& scope);
    
    /**
     * @brief Register custom report template
     * @param template_config Template configuration
     * @return true if template registered successfully
     */
    bool register_report_template(const ReportTemplate& template_config);
    
    /**
     * @brief Get available report templates
     * @return Vector of available template names
     */
    std::vector<std::string> get_available_templates() const;
    
    /**
     * @brief Schedule automatic report generation
     * @param category Report category to schedule
     * @param schedule Cron-style schedule string
     * @param format Report format
     * @param template_name Template to use
     * @return true if scheduled successfully
     */
    bool schedule_automatic_report(
        ReportCategory category,
        const std::string& schedule,
        ReportFormat format = ReportFormat::PDF,
        const std::string& template_name = "default"
    );
    
    /**
     * @brief Cancel automatic report generation
     * @param category Report category to cancel
     * @return true if cancelled successfully
     */
    bool cancel_automatic_report(ReportCategory category);
    
    /**
     * @brief Get report generation history
     * @param max_reports Maximum number of reports to return
     * @return Vector of recent report generation results
     */
    std::vector<ReportGenerationResult> get_report_history(size_t max_reports = 100) const;
    
    /**
     * @brief Export report to file
     * @param result Report generation result
     * @param filename Output filename
     * @return true if export successful
     */
    bool export_report_to_file(const ReportGenerationResult& result,
                              const std::string& filename) const;
    
    /**
     * @brief Get report by filename
     * @param filename Report filename
     * @return Report content or empty string if not found
     */
    std::string get_report_by_filename(const std::string& filename) const;
    
    /**
     * @brief Delete old reports
     * @param older_than Delete reports older than this duration
     * @return Number of reports deleted
     */
    uint32_t cleanup_old_reports(std::chrono::days older_than = std::chrono::days{90});
    
    /**
     * @brief Set data sources for report generation
     * @param compliance_engine Compliance engine for validation data
     * @param logger Compliance logger for audit trail data
     * @param alert_system Alert system for alert data
     * @param broadcast_framework Broadcast standards framework for quality data
     */
    void set_data_sources(
        std::shared_ptr<compliance::EtsiComplianceEngine> compliance_engine,
        std::shared_ptr<logging::EtsiComplianceLogger> logger,
        std::shared_ptr<alerts::EtsiAlertSystem> alert_system,
        std::shared_ptr<broadcast::BroadcastStandardsFramework> broadcast_framework
    );
    
    /**
     * @brief Update reporter configuration
     * @param config New configuration
     */
    void update_configuration(const Config& config);
    
    /**
     * @brief Get reporter statistics
     */
    struct ReporterStatistics {
        uint32_t total_reports_generated = 0;
        uint32_t successful_reports = 0;
        uint32_t failed_reports = 0;
        std::map<ReportFormat, uint32_t> reports_by_format;
        std::map<ReportCategory, uint32_t> reports_by_category;
        std::chrono::microseconds average_generation_time{0};
        std::chrono::microseconds max_generation_time{0};
        uint64_t total_report_size_bytes = 0;
        std::chrono::system_clock::time_point last_report_time;
    };
    
    ReporterStatistics get_statistics() const;
    
    /**
     * @brief Reset reporter statistics
     */
    void reset_statistics();

private:
    Config config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Data sources
    std::shared_ptr<compliance::EtsiComplianceEngine> compliance_engine_;
    std::shared_ptr<logging::EtsiComplianceLogger> logger_;
    std::shared_ptr<alerts::EtsiAlertSystem> alert_system_;
    std::shared_ptr<broadcast::BroadcastStandardsFramework> broadcast_framework_;
    
    // Report generators
    std::unique_ptr<ReportGenerator> text_generator_;
    std::unique_ptr<HtmlGenerator> html_generator_;
    std::unique_ptr<PdfGenerator> pdf_generator_;
    std::unique_ptr<XmlGenerator> xml_generator_;
    std::unique_ptr<RegulatoryFormatter> regulatory_formatter_;
    
    // Template management
    std::map<std::string, ReportTemplate> templates_;
    mutable std::mutex templates_mutex_;
    
    // Report history and caching
    mutable std::mutex history_mutex_;
    std::vector<ReportGenerationResult> report_history_;
    std::map<std::string, std::pair<ReportGenerationResult, std::chrono::system_clock::time_point>> report_cache_;
    
    // Statistics tracking
    mutable std::mutex stats_mutex_;
    ReporterStatistics statistics_;
    
    // Automatic reporting
    std::map<ReportCategory, std::string> scheduled_reports_;
    std::unique_ptr<std::thread> scheduler_thread_;
    
    // Internal methods
    void initialize_generators();
    void ensure_directories_exist();
    void update_statistics(const ReportGenerationResult& result);
    void add_to_history(const ReportGenerationResult& result);
    void cleanup_cache();
    void scheduler_loop();
    
    // Report generation helper methods
    std::string generate_report_header(const ReportTemplate& template,
                                     const ReportScope& scope,
                                     ReportCategory category);
    std::string generate_report_footer(const ReportTemplate& template);
    std::string generate_compliance_summary_section(const ComplianceMetricsSummary& metrics);
    std::string generate_service_quality_section(const ComplianceMetricsSummary& metrics);
    std::string generate_alerts_section(const ReportScope& scope);
    std::string generate_performance_section(const ComplianceMetricsSummary& metrics);
    
    // Data collection helper methods
    std::vector<compliance::ComplianceResult> collect_compliance_data(const ReportScope& scope);
    std::vector<logging::ComplianceLogEntry> collect_log_data(const ReportScope& scope);
    std::vector<alerts::Alert> collect_alert_data(const ReportScope& scope);
    std::vector<broadcast::BroadcastComplianceResult> collect_broadcast_data(const ReportScope& scope);
    
    // Utility methods
    std::string format_duration(std::chrono::seconds duration) const;
    std::string format_percentage(double percentage, int decimal_places = 2) const;
    std::string format_file_size(size_t size_bytes) const;
    std::string generate_report_filename(ReportCategory category, ReportFormat format) const;
    std::string calculate_checksum(const std::string& content) const;
    static std::string get_format_extension(ReportFormat format);
    static std::string get_category_name(ReportCategory category);
    static std::string get_format_name(ReportFormat format);
};

/**
 * @brief Utility functions for compliance reporting
 */
namespace utils {
    /**
     * @brief Convert compliance metrics to tabular format
     * @param metrics Compliance metrics to format
     * @param format Output format ("csv", "html", "text")
     * @return Formatted table string
     */
    std::string format_metrics_table(const ComplianceMetricsSummary& metrics,
                                   const std::string& format = "html");
    
    /**
     * @brief Generate compliance trend analysis
     * @param historical_metrics Vector of historical metrics
     * @return Trend analysis string
     */
    std::string generate_trend_analysis(const std::vector<ComplianceMetricsSummary>& historical_metrics);
    
    /**
     * @brief Create executive summary from detailed metrics
     * @param metrics Detailed compliance metrics
     * @param max_length Maximum summary length
     * @return Executive summary string
     */
    std::string create_executive_summary(const ComplianceMetricsSummary& metrics,
                                       size_t max_length = 500);
    
    /**
     * @brief Generate compliance scorecard
     * @param metrics Compliance metrics
     * @return Scorecard with grades and ratings
     */
    std::string generate_compliance_scorecard(const ComplianceMetricsSummary& metrics);
    
    /**
     * @brief Convert report to regulatory format
     * @param report_content Original report content
     * @param regulation_standard Regulation standard format
     * @return Regulatory formatted report
     */
    std::string convert_to_regulatory_format(const std::string& report_content,
                                           const std::string& regulation_standard);
    
    /**
     * @brief Validate report completeness
     * @param result Report generation result
     * @param required_sections Required sections list
     * @return Validation result with missing sections
     */
    std::pair<bool, std::vector<std::string>> validate_report_completeness(
        const ReportGenerationResult& result,
        const std::vector<std::string>& required_sections
    );
}

} // namespace reporting
} // namespace etsi

#endif // ETSI_COMPLIANCE_REPORTER_H