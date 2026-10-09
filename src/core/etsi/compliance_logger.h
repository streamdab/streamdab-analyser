/**
 * @file compliance_logger.h
 * @brief ETSI Standards Compliance Logging Framework
 * 
 * Professional logging framework for ETSI standards compliance monitoring
 * with structured audit trails, regulatory documentation, and real-time
 * violation tracking for broadcast industry applications.
 * 
 * Features:
 * - Structured compliance logging with ETSI standard validation
 * - Professional audit trail generation for regulatory compliance
 * - Real-time violation detection and documentation
 * - Standards deviation identification and tracking
 * - Cross-platform compliance documentation export
 * - Integration with professional broadcast monitoring systems
 * 
 * Standards Supported:
 * - ETSI EN 300 401 (DAB Radio Broadcasting)
 * - ETSI EN 302 077 (DAB+ Audio Coding)
 * - ETSI EN 300 799 (ETI Distribution Interface)
 * - ETSI TS 102 563 (DAB+ Audio Encoding Guidelines)
 * - ETSI TS 101 756 (Registered Tables for DAB)
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#ifndef ETSI_COMPLIANCE_LOGGER_H
#define ETSI_COMPLIANCE_LOGGER_H

#include "compliance_engine.h"
#include "alert_system.h"
#include "../eti_types.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <fstream>
#include <queue>
#include <thread>

namespace etsi {
namespace logging {

/**
 * @brief Compliance log severity levels for professional documentation
 */
enum class LogSeverity : uint8_t {
    TRACE = 0,      // Detailed execution tracing
    DEBUG = 1,      // Debug information
    INFO = 2,       // General information
    NOTICE = 3,     // Notice - significant events
    WARNING = 4,    // Warning - potential issues
    ERROR = 5,      // Error - compliance violations
    CRITICAL = 6,   // Critical - service-affecting violations
    EMERGENCY = 7   // Emergency - immediate action required
};

/**
 * @brief Compliance logging categories for professional audit trails
 */
enum class LogCategory : uint8_t {
    ETSI_VALIDATION = 1,        // ETSI standard validation results
    FRAME_ANALYSIS = 2,         // ETI frame analysis logging
    FIG_PROCESSING = 3,         // FIG analysis and validation
    AUDIO_QUALITY = 4,          // Audio quality monitoring
    SERVICE_MONITORING = 5,     // Service availability tracking
    ERROR_CORRECTION = 6,       // Reed-Solomon error correction
    SYNCHRONIZATION = 7,        // Time synchronization events
    PERFORMANCE = 8,            // System performance metrics
    SECURITY = 9,               // Security-related events
    CONFIGURATION = 10,         // Configuration changes
    REGULATORY = 11,            // Regulatory compliance events
    EMERGENCY = 12             // Emergency alert system events
};

/**
 * @brief Individual compliance log entry
 */
struct ComplianceLogEntry {
    uint64_t log_id;
    LogSeverity severity;
    LogCategory category;
    compliance::EtsiStandard related_standard;
    std::string message;
    std::string details;
    std::string source_location;    // File, function, line information
    std::string entity_id;          // Service ID, Ensemble ID, Frame number
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::microseconds processing_time;
    
    // Audit trail information
    std::string session_id;
    std::string user_context;
    std::string operation_context;
    
    ComplianceLogEntry(LogSeverity sev, LogCategory cat, const std::string& msg,
                      compliance::EtsiStandard std = compliance::EtsiStandard::EN_300_401)
        : log_id(generate_log_id()), severity(sev), category(cat), 
          related_standard(std), message(msg),
          timestamp(std::chrono::system_clock::now()),
          processing_time(0) {}
    
    std::string get_severity_string() const {
        return get_severity_name(severity);
    }
    
    std::string get_category_string() const {
        return get_category_name(category);
    }
    
    std::string get_standard_string() const {
        return get_standard_name(related_standard);
    }
    
private:
    static uint64_t generate_log_id() {
        static std::atomic<uint64_t> counter{1};
        return counter.fetch_add(1);
    }
    
    static std::string get_severity_name(LogSeverity severity);
    static std::string get_category_name(LogCategory category);
    static std::string get_standard_name(compliance::EtsiStandard standard);
};

/**
 * @brief Compliance audit trail configuration
 */
struct AuditTrailConfig {
    bool enable_audit_trail = true;
    bool enable_regulatory_documentation = true;
    bool enable_performance_logging = true;
    bool enable_security_logging = true;
    
    std::string audit_file_prefix = "etsi_compliance_audit";
    std::string log_directory = "./logs";
    std::string archive_directory = "./logs/archive";
    
    // Log rotation settings
    size_t max_log_file_size_mb = 100;      // 100 MB per file
    uint32_t max_log_files = 50;            // Keep 50 files
    std::chrono::hours log_rotation_interval{24}; // Daily rotation
    
    // Retention policy
    std::chrono::days log_retention_days{365};     // 1 year retention
    std::chrono::days archive_retention_days{2555}; // 7 years for regulatory
    
    // Real-time logging
    bool enable_real_time_logging = true;
    std::chrono::milliseconds flush_interval{1000}; // 1 second flush
    size_t buffer_size = 10000;             // Log entries buffer
    
    AuditTrailConfig() = default;
};

/**
 * @brief Log filtering and query configuration
 */
struct LogFilterConfig {
    std::vector<LogSeverity> enabled_severities = {
        LogSeverity::INFO, LogSeverity::NOTICE, LogSeverity::WARNING,
        LogSeverity::ERROR, LogSeverity::CRITICAL, LogSeverity::EMERGENCY
    };
    
    std::vector<LogCategory> enabled_categories = {
        LogCategory::ETSI_VALIDATION, LogCategory::FRAME_ANALYSIS,
        LogCategory::FIG_PROCESSING, LogCategory::AUDIO_QUALITY,
        LogCategory::SERVICE_MONITORING, LogCategory::ERROR_CORRECTION,
        LogCategory::REGULATORY, LogCategory::EMERGENCY
    };
    
    std::vector<compliance::EtsiStandard> enabled_standards = {
        compliance::EtsiStandard::EN_300_401,
        compliance::EtsiStandard::EN_302_077,
        compliance::EtsiStandard::EN_300_799,
        compliance::EtsiStandard::TS_102_563,
        compliance::EtsiStandard::TS_101_756
    };
    
    // Time filtering
    std::chrono::system_clock::time_point start_time = 
        std::chrono::system_clock::time_point::min();
    std::chrono::system_clock::time_point end_time = 
        std::chrono::system_clock::time_point::max();
    
    // Entity filtering
    std::vector<std::string> entity_filter;    // Service IDs, Ensemble IDs, etc.
    std::vector<std::string> session_filter;   // Session IDs
    
    // Content filtering
    std::string message_filter;               // Substring match in message
    std::string details_filter;               // Substring match in details
    
    LogFilterConfig() = default;
};

/**
 * @brief Compliance logging statistics
 */
struct ComplianceLoggingStats {
    uint64_t total_entries_logged = 0;
    uint64_t entries_by_severity[8] = {0}; // One for each LogSeverity
    uint64_t entries_by_category[13] = {0}; // One for each LogCategory
    
    uint64_t compliance_violations = 0;
    uint64_t critical_issues = 0;
    uint64_t warnings_generated = 0;
    
    std::chrono::system_clock::time_point first_log_time;
    std::chrono::system_clock::time_point last_log_time;
    
    uint64_t total_bytes_logged = 0;
    uint64_t files_created = 0;
    uint64_t files_archived = 0;
    
    std::chrono::microseconds avg_logging_time{0};
    std::chrono::microseconds max_logging_time{0};
    
    ComplianceLoggingStats() {
        first_log_time = std::chrono::system_clock::now();
        last_log_time = first_log_time;
    }
};

// Forward declarations
class LogFormatter;
class LogWriter;
class LogArchiver;
class LogQueryEngine;

/**
 * @brief Professional ETSI Standards Compliance Logger
 * 
 * Comprehensive logging framework for ETSI compliance monitoring with
 * structured audit trails, regulatory documentation, and professional
 * reporting capabilities for broadcast industry applications.
 */
class EtsiComplianceLogger {
public:
    explicit EtsiComplianceLogger(const AuditTrailConfig& config = AuditTrailConfig{});
    ~EtsiComplianceLogger();
    
    // Disable copy/move for thread safety
    EtsiComplianceLogger(const EtsiComplianceLogger&) = delete;
    EtsiComplianceLogger& operator=(const EtsiComplianceLogger&) = delete;
    EtsiComplianceLogger(EtsiComplianceLogger&&) = delete;
    EtsiComplianceLogger& operator=(EtsiComplianceLogger&&) = delete;
    
    /**
     * @brief Initialize the compliance logger
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown the compliance logger
     */
    void shutdown();
    
    /**
     * @brief Log compliance validation result
     * @param result Compliance validation result to log
     * @param context Additional context information
     */
    void log_compliance_result(const compliance::ComplianceResult& result,
                             const std::string& context = "");
    
    /**
     * @brief Log ETI frame analysis
     * @param frame ETI frame that was analyzed
     * @param analysis_result Analysis results
     * @param processing_time Time taken for analysis
     */
    void log_eti_frame_analysis(const EtiFrame& frame,
                               const std::string& analysis_result,
                               std::chrono::microseconds processing_time);
    
    /**
     * @brief Log FIG processing result
     * @param fig_type FIG type that was processed
     * @param fig_data FIG data processed
     * @param result Processing result
     * @param compliance_issues Any compliance issues found
     */
    void log_fig_processing(uint8_t fig_type,
                          const std::vector<uint8_t>& fig_data,
                          const std::string& result,
                          const std::vector<std::string>& compliance_issues = {});
    
    /**
     * @brief Log audio quality monitoring result
     * @param service_id Service ID being monitored
     * @param quality_metrics Audio quality metrics
     * @param compliance_status Compliance status
     */
    void log_audio_quality(uint32_t service_id,
                         const std::map<std::string, double>& quality_metrics,
                         bool compliance_status);
    
    /**
     * @brief Log service availability event
     * @param service_id Service ID
     * @param event_type Event type (start, stop, reconfiguration, etc.)
     * @param details Event details
     * @param impact Service impact assessment
     */
    void log_service_event(uint32_t service_id,
                         const std::string& event_type,
                         const std::string& details,
                         const std::string& impact = "");
    
    /**
     * @brief Log alert generated by alert system
     * @param alert Alert that was generated
     */
    void log_alert(const alerts::Alert& alert);
    
    /**
     * @brief Log performance metrics
     * @param metrics Performance metrics map
     * @param component Component name generating metrics
     */
    void log_performance_metrics(const std::map<std::string, double>& metrics,
                                const std::string& component);
    
    /**
     * @brief Log configuration change
     * @param component Component that changed
     * @param old_config Previous configuration (JSON/YAML string)
     * @param new_config New configuration (JSON/YAML string)
     * @param changed_by User or system that made the change
     */
    void log_configuration_change(const std::string& component,
                                const std::string& old_config,
                                const std::string& new_config,
                                const std::string& changed_by);
    
    /**
     * @brief Log regulatory compliance event
     * @param regulation Regulation name (e.g., "FCC Part 73", "Ofcom 1998/2070")
     * @param compliance_check Compliance check performed
     * @param result Check result
     * @param corrective_action Required corrective action (if any)
     */
    void log_regulatory_compliance(const std::string& regulation,
                                 const std::string& compliance_check,
                                 const std::string& result,
                                 const std::string& corrective_action = "");
    
    /**
     * @brief Generic logging method for custom entries
     * @param severity Log severity level
     * @param category Log category
     * @param message Log message
     * @param details Additional details
     * @param standard Related ETSI standard
     * @param metadata Additional metadata map
     */
    void log(LogSeverity severity,
            LogCategory category,
            const std::string& message,
            const std::string& details = "",
            compliance::EtsiStandard standard = compliance::EtsiStandard::EN_300_401,
            const std::map<std::string, std::string>& metadata = {});
    
    /**
     * @brief Query log entries with filtering
     * @param filter Filter configuration
     * @param max_entries Maximum entries to return (0 = no limit)
     * @return Vector of matching log entries
     */
    std::vector<ComplianceLogEntry> query_logs(const LogFilterConfig& filter,
                                              size_t max_entries = 1000) const;
    
    /**
     * @brief Get recent log entries
     * @param duration Duration to look back
     * @param severity_filter Minimum severity level
     * @return Vector of recent log entries
     */
    std::vector<ComplianceLogEntry> get_recent_logs(
        std::chrono::minutes duration = std::chrono::minutes{60},
        LogSeverity severity_filter = LogSeverity::INFO
    ) const;
    
    /**
     * @brief Get compliance violations in timeframe
     * @param duration Duration to look back
     * @return Vector of compliance violation entries
     */
    std::vector<ComplianceLogEntry> get_compliance_violations(
        std::chrono::hours duration = std::chrono::hours{24}
    ) const;
    
    /**
     * @brief Generate compliance audit report
     * @param start_time Report start time
     * @param end_time Report end time
     * @param format Report format ("text", "html", "pdf", "json")
     * @return Formatted audit report
     */
    std::string generate_audit_report(
        std::chrono::system_clock::time_point start_time,
        std::chrono::system_clock::time_point end_time,
        const std::string& format = "text"
    ) const;
    
    /**
     * @brief Generate regulatory compliance documentation
     * @param regulation_name Regulation name
     * @param timeframe Documentation timeframe
     * @return Regulatory compliance documentation
     */
    std::string generate_regulatory_documentation(
        const std::string& regulation_name,
        std::chrono::hours timeframe = std::chrono::hours{24 * 30} // 30 days
    ) const;
    
    /**
     * @brief Export logs to external format
     * @param filter Filter configuration
     * @param format Export format ("csv", "json", "xml", "syslog")
     * @param filename Output filename
     * @return true if export successful
     */
    bool export_logs(const LogFilterConfig& filter,
                    const std::string& format,
                    const std::string& filename) const;
    
    /**
     * @brief Get current logging statistics
     * @return Current statistics
     */
    ComplianceLoggingStats get_statistics() const;
    
    /**
     * @brief Reset logging statistics
     */
    void reset_statistics();
    
    /**
     * @brief Update logging configuration
     * @param config New configuration
     */
    void update_configuration(const AuditTrailConfig& config);
    
    /**
     * @brief Force log flush to disk
     */
    void flush_logs();
    
    /**
     * @brief Trigger log rotation
     */
    void rotate_logs();
    
    /**
     * @brief Archive old logs
     * @param older_than Archive logs older than this duration
     */
    void archive_logs(std::chrono::days older_than = std::chrono::days{30});
    
    /**
     * @brief Set session context for audit trail
     * @param session_id Unique session identifier
     * @param user_context User or system context
     */
    void set_session_context(const std::string& session_id,
                           const std::string& user_context);
    
    /**
     * @brief Set operation context for current operations
     * @param operation_context Current operation description
     */
    void set_operation_context(const std::string& operation_context);
    
    /**
     * @brief Enable/disable log category
     * @param category Category to enable/disable
     * @param enabled true to enable, false to disable
     */
    void enable_log_category(LogCategory category, bool enabled);
    
    /**
     * @brief Set minimum log severity level
     * @param min_severity Minimum severity to log
     */
    void set_minimum_severity(LogSeverity min_severity);

private:
    AuditTrailConfig config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Logging components
    std::unique_ptr<LogFormatter> formatter_;
    std::unique_ptr<LogWriter> writer_;
    std::unique_ptr<LogArchiver> archiver_;
    std::unique_ptr<LogQueryEngine> query_engine_;
    
    // Background processing
    std::unique_ptr<std::thread> logging_thread_;
    std::queue<ComplianceLogEntry> log_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    
    // Session and context tracking
    std::string current_session_id_;
    std::string current_user_context_;
    std::string current_operation_context_;
    mutable std::mutex context_mutex_;
    
    // Statistics tracking
    mutable std::mutex stats_mutex_;
    ComplianceLoggingStats statistics_;
    
    // Configuration and filtering
    LogFilterConfig filter_config_;
    LogSeverity minimum_severity_ = LogSeverity::INFO;
    
    // Internal methods
    void logging_loop();
    void process_log_entry(const ComplianceLogEntry& entry);
    void update_statistics(const ComplianceLogEntry& entry);
    bool should_log_entry(const ComplianceLogEntry& entry) const;
    void add_to_queue(ComplianceLogEntry&& entry);
    void ensure_log_directory_exists();
    void cleanup_old_logs();
    
    // Utility methods
    std::string format_timestamp(std::chrono::system_clock::time_point timestamp) const;
    std::string generate_log_filename() const;
    static std::string get_severity_color_code(LogSeverity severity);
    static std::string get_category_icon(LogCategory category);
};

/**
 * @brief Convenience macros for compliance logging
 */
#define ETSI_LOG_TRACE(logger, category, message, ...) \
    logger->log(etsi::logging::LogSeverity::TRACE, category, message, ##__VA_ARGS__)

#define ETSI_LOG_DEBUG(logger, category, message, ...) \
    logger->log(etsi::logging::LogSeverity::DEBUG, category, message, ##__VA_ARGS__)

#define ETSI_LOG_INFO(logger, category, message, ...) \
    logger->log(etsi::logging::LogSeverity::INFO, category, message, ##__VA_ARGS__)

#define ETSI_LOG_NOTICE(logger, category, message, ...) \
    logger->log(etsi::logging::LogSeverity::NOTICE, category, message, ##__VA_ARGS__)

#define ETSI_LOG_WARNING(logger, category, message, ...) \
    logger->log(etsi::logging::LogSeverity::WARNING, category, message, ##__VA_ARGS__)

#define ETSI_LOG_ERROR(logger, category, message, ...) \
    logger->log(etsi::logging::LogSeverity::ERROR, category, message, ##__VA_ARGS__)

#define ETSI_LOG_CRITICAL(logger, category, message, ...) \
    logger->log(etsi::logging::LogSeverity::CRITICAL, category, message, ##__VA_ARGS__)

#define ETSI_LOG_EMERGENCY(logger, category, message, ...) \
    logger->log(etsi::logging::LogSeverity::EMERGENCY, category, message, ##__VA_ARGS__)

/**
 * @brief Utility functions for compliance logging
 */
namespace utils {
    /**
     * @brief Convert log entry to human-readable string
     * @param entry Log entry to format
     * @param include_metadata Include metadata in output
     * @return Formatted string representation
     */
    std::string format_log_entry(const ComplianceLogEntry& entry, bool include_metadata = true);
    
    /**
     * @brief Generate log summary for timeframe
     * @param entries Log entries to summarize
     * @return Summary statistics string
     */
    std::string generate_log_summary(const std::vector<ComplianceLogEntry>& entries);
    
    /**
     * @brief Check if log severity requires immediate notification
     * @param severity Log severity to check
     * @return true if immediate notification required
     */
    bool requires_immediate_notification(LogSeverity severity);
    
    /**
     * @brief Calculate log entry priority for processing
     * @param entry Log entry to score
     * @return Priority score (higher = more urgent)
     */
    uint32_t calculate_log_priority(const ComplianceLogEntry& entry);
    
    /**
     * @brief Convert logs to regulatory compliance format
     * @param entries Log entries to convert
     * @param regulation_standard Regulation standard format
     * @return Formatted compliance documentation
     */
    std::string convert_to_regulatory_format(const std::vector<ComplianceLogEntry>& entries,
                                           const std::string& regulation_standard);
}

} // namespace logging
} // namespace etsi

#endif // ETSI_COMPLIANCE_LOGGER_H