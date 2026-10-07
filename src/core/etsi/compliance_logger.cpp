/**
 * @file compliance_logger.cpp
 * @brief ETSI Standards Compliance Logging Framework Implementation
 * 
 * Implementation of professional logging framework for ETSI compliance
 * monitoring with structured audit trails and regulatory documentation.
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#include "compliance_logger.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <ctime>

namespace etsi {
namespace logging {

// Helper classes for logging functionality
class LogFormatter {
public:
    std::string format_entry(const ComplianceLogEntry& entry, const std::string& format = "text") const {
        if (format == "json") {
            return format_json(entry);
        } else if (format == "csv") {
            return format_csv(entry);
        } else if (format == "xml") {
            return format_xml(entry);
        } else {
            return format_text(entry);
        }
    }
    
private:
    std::string format_text(const ComplianceLogEntry& entry) const {
        std::ostringstream oss;
        
        auto time_t_timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
        oss << "[" << std::put_time(std::localtime(&time_t_timestamp), "%Y-%m-%d %H:%M:%S") << "] "
            << "[" << entry.get_severity_string() << "] "
            << "[" << entry.get_category_string() << "] "
            << "[" << entry.get_standard_string() << "] "
            << entry.message;
            
        if (!entry.details.empty()) {
            oss << " - " << entry.details;
        }
        
        if (!entry.entity_id.empty()) {
            oss << " (Entity: " << entry.entity_id << ")";
        }
        
        if (entry.processing_time.count() > 0) {
            oss << " [" << entry.processing_time.count() << "μs]";
        }
        
        if (!entry.metadata.empty()) {
            oss << " {";
            bool first = true;
            for (const auto& pair : entry.metadata) {
                if (!first) oss << ", ";
                oss << pair.first << ":" << pair.second;
                first = false;
            }
            oss << "}";
        }
        
        return oss.str();
    }
    
    std::string format_json(const ComplianceLogEntry& entry) const {
        std::ostringstream oss;
        auto time_t_timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
        
        oss << "{\n"
            << "  \"id\": " << entry.log_id << ",\n"
            << "  \"timestamp\": \"" << std::put_time(std::localtime(&time_t_timestamp), "%Y-%m-%d %H:%M:%S") << "\",\n"
            << "  \"severity\": \"" << entry.get_severity_string() << "\",\n"
            << "  \"category\": \"" << entry.get_category_string() << "\",\n"
            << "  \"standard\": \"" << entry.get_standard_string() << "\",\n"
            << "  \"message\": \"" << escape_json_string(entry.message) << "\",\n"
            << "  \"details\": \"" << escape_json_string(entry.details) << "\",\n"
            << "  \"entity_id\": \"" << entry.entity_id << "\",\n"
            << "  \"processing_time_us\": " << entry.processing_time.count();
            
        if (!entry.session_id.empty()) {
            oss << ",\n  \"session_id\": \"" << entry.session_id << "\"";
        }
        
        if (!entry.metadata.empty()) {
            oss << ",\n  \"metadata\": {\n";
            bool first = true;
            for (const auto& pair : entry.metadata) {
                if (!first) oss << ",\n";
                oss << "    \"" << escape_json_string(pair.first) << "\": \"" 
                    << escape_json_string(pair.second) << "\"";
                first = false;
            }
            oss << "\n  }";
        }
        
        oss << "\n}";
        return oss.str();
    }
    
    std::string format_csv(const ComplianceLogEntry& entry) const {
        std::ostringstream oss;
        auto time_t_timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
        
        oss << entry.log_id << ","
            << std::put_time(std::localtime(&time_t_timestamp), "%Y-%m-%d %H:%M:%S") << ","
            << entry.get_severity_string() << ","
            << entry.get_category_string() << ","
            << entry.get_standard_string() << ","
            << "\"" << escape_csv_string(entry.message) << "\","
            << "\"" << escape_csv_string(entry.details) << "\","
            << entry.entity_id << ","
            << entry.processing_time.count() << ","
            << entry.session_id;
            
        return oss.str();
    }
    
    std::string format_xml(const ComplianceLogEntry& entry) const {
        std::ostringstream oss;
        auto time_t_timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
        
        oss << "<log_entry id=\"" << entry.log_id << "\">\n"
            << "  <timestamp>" << std::put_time(std::localtime(&time_t_timestamp), "%Y-%m-%d %H:%M:%S") << "</timestamp>\n"
            << "  <severity>" << entry.get_severity_string() << "</severity>\n"
            << "  <category>" << entry.get_category_string() << "</category>\n"
            << "  <standard>" << entry.get_standard_string() << "</standard>\n"
            << "  <message><![CDATA[" << entry.message << "]]></message>\n"
            << "  <details><![CDATA[" << entry.details << "]]></details>\n"
            << "  <entity_id>" << entry.entity_id << "</entity_id>\n"
            << "  <processing_time_us>" << entry.processing_time.count() << "</processing_time_us>\n";
            
        if (!entry.session_id.empty()) {
            oss << "  <session_id>" << entry.session_id << "</session_id>\n";
        }
        
        if (!entry.metadata.empty()) {
            oss << "  <metadata>\n";
            for (const auto& pair : entry.metadata) {
                oss << "    <" << pair.first << "><![CDATA[" << pair.second << "]]></" << pair.first << ">\n";
            }
            oss << "  </metadata>\n";
        }
        
        oss << "</log_entry>";
        return oss.str();
    }
    
    std::string escape_json_string(const std::string& str) const {
        std::string escaped = str;
        size_t pos = 0;
        while ((pos = escaped.find("\"", pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\\\"");
            pos += 2;
        }
        pos = 0;
        while ((pos = escaped.find("\n", pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\\n");
            pos += 2;
        }
        return escaped;
    }
    
    std::string escape_csv_string(const std::string& str) const {
        std::string escaped = str;
        size_t pos = 0;
        while ((pos = escaped.find("\"", pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\"\"");
            pos += 2;
        }
        return escaped;
    }
};

class LogWriter {
public:
    explicit LogWriter(const AuditTrailConfig& config) : config_(config) {}
    
    bool initialize() {
        try {
            ensure_directory_exists(config_.log_directory);
            ensure_directory_exists(config_.archive_directory);
            
            current_log_file_ = generate_log_filename();
            log_stream_.open(current_log_file_, std::ios::app);
            
            return log_stream_.is_open();
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    void shutdown() {
        if (log_stream_.is_open()) {
            log_stream_.flush();
            log_stream_.close();
        }
    }
    
    bool write_entry(const std::string& formatted_entry) {
        if (!log_stream_.is_open()) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(write_mutex_);
        log_stream_ << formatted_entry << std::endl;
        
        current_file_size_ += formatted_entry.length() + 1;
        
        // Check if rotation is needed
        if (current_file_size_ > config_.max_log_file_size_mb * 1024 * 1024) {
            rotate_log_file();
        }
        
        return true;
    }
    
    void flush() {
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (log_stream_.is_open()) {
            log_stream_.flush();
        }
    }
    
    void rotate_log_file() {
        if (log_stream_.is_open()) {
            log_stream_.flush();
            log_stream_.close();
        }
        
        // Generate new log filename
        current_log_file_ = generate_log_filename();
        log_stream_.open(current_log_file_, std::ios::app);
        current_file_size_ = 0;
    }
    
private:
    const AuditTrailConfig& config_;
    std::ofstream log_stream_;
    std::string current_log_file_;
    size_t current_file_size_ = 0;
    std::mutex write_mutex_;
    
    void ensure_directory_exists(const std::string& directory) {
        std::filesystem::create_directories(directory);
    }
    
    std::string generate_log_filename() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << config_.log_directory << "/"
            << config_.audit_file_prefix << "_"
            << std::put_time(std::localtime(&time_t_now), "%Y%m%d_%H%M%S")
            << ".log";
            
        return oss.str();
    }
};

class LogArchiver {
public:
    explicit LogArchiver(const AuditTrailConfig& config) : config_(config) {}
    
    void archive_old_logs(std::chrono::days older_than) {
        auto cutoff_time = std::chrono::system_clock::now() - older_than;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(config_.log_directory)) {
                if (!entry.is_regular_file()) continue;
                
                auto file_time = std::chrono::file_clock::to_sys(entry.last_write_time());
                if (file_time < cutoff_time) {
                    std::string archive_path = config_.archive_directory + "/" + entry.path().filename().string();
                    std::filesystem::rename(entry.path(), archive_path);
                }
            }
        } catch (const std::exception& e) {
            // Handle archiving errors
        }
    }
    
    void cleanup_archived_logs(std::chrono::days retention_period) {
        auto cutoff_time = std::chrono::system_clock::now() - retention_period;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(config_.archive_directory)) {
                if (!entry.is_regular_file()) continue;
                
                auto file_time = std::chrono::file_clock::to_sys(entry.last_write_time());
                if (file_time < cutoff_time) {
                    std::filesystem::remove(entry.path());
                }
            }
        } catch (const std::exception& e) {
            // Handle cleanup errors
        }
    }
    
private:
    const AuditTrailConfig& config_;
};

class LogQueryEngine {
public:
    std::vector<ComplianceLogEntry> query(const LogFilterConfig& filter,
                                        const std::vector<ComplianceLogEntry>& entries,
                                        size_t max_entries = 1000) const {
        std::vector<ComplianceLogEntry> filtered_entries;
        
        for (const auto& entry : entries) {
            if (matches_filter(entry, filter)) {
                filtered_entries.push_back(entry);
                
                if (max_entries > 0 && filtered_entries.size() >= max_entries) {
                    break;
                }
            }
        }
        
        // Sort by timestamp (newest first)
        std::sort(filtered_entries.begin(), filtered_entries.end(),
                 [](const ComplianceLogEntry& a, const ComplianceLogEntry& b) {
                     return a.timestamp > b.timestamp;
                 });
        
        return filtered_entries;
    }
    
private:
    bool matches_filter(const ComplianceLogEntry& entry, const LogFilterConfig& filter) const {
        // Check severity
        if (std::find(filter.enabled_severities.begin(), filter.enabled_severities.end(), 
                     entry.severity) == filter.enabled_severities.end()) {
            return false;
        }
        
        // Check category
        if (std::find(filter.enabled_categories.begin(), filter.enabled_categories.end(),
                     entry.category) == filter.enabled_categories.end()) {
            return false;
        }
        
        // Check standard
        if (std::find(filter.enabled_standards.begin(), filter.enabled_standards.end(),
                     entry.related_standard) == filter.enabled_standards.end()) {
            return false;
        }
        
        // Check time range
        if (entry.timestamp < filter.start_time || entry.timestamp > filter.end_time) {
            return false;
        }
        
        // Check entity filter
        if (!filter.entity_filter.empty()) {
            bool found = false;
            for (const auto& entity : filter.entity_filter) {
                if (entry.entity_id.find(entity) != std::string::npos) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        
        // Check session filter
        if (!filter.session_filter.empty()) {
            bool found = std::find(filter.session_filter.begin(), filter.session_filter.end(),
                                 entry.session_id) != filter.session_filter.end();
            if (!found) return false;
        }
        
        // Check message filter
        if (!filter.message_filter.empty()) {
            if (entry.message.find(filter.message_filter) == std::string::npos) {
                return false;
            }
        }
        
        // Check details filter
        if (!filter.details_filter.empty()) {
            if (entry.details.find(filter.details_filter) == std::string::npos) {
                return false;
            }
        }
        
        return true;
    }
};

// ComplianceLogEntry static method implementations
std::string ComplianceLogEntry::get_severity_name(LogSeverity severity) {
    switch (severity) {
        case LogSeverity::TRACE: return "TRACE";
        case LogSeverity::DEBUG: return "DEBUG";
        case LogSeverity::INFO: return "INFO";
        case LogSeverity::NOTICE: return "NOTICE";
        case LogSeverity::WARNING: return "WARNING";
        case LogSeverity::ERROR: return "ERROR";
        case LogSeverity::CRITICAL: return "CRITICAL";
        case LogSeverity::EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

std::string ComplianceLogEntry::get_category_name(LogCategory category) {
    switch (category) {
        case LogCategory::ETSI_VALIDATION: return "ETSI_VALIDATION";
        case LogCategory::FRAME_ANALYSIS: return "FRAME_ANALYSIS";
        case LogCategory::FIG_PROCESSING: return "FIG_PROCESSING";
        case LogCategory::AUDIO_QUALITY: return "AUDIO_QUALITY";
        case LogCategory::SERVICE_MONITORING: return "SERVICE_MONITORING";
        case LogCategory::ERROR_CORRECTION: return "ERROR_CORRECTION";
        case LogCategory::SYNCHRONIZATION: return "SYNCHRONIZATION";
        case LogCategory::PERFORMANCE: return "PERFORMANCE";
        case LogCategory::SECURITY: return "SECURITY";
        case LogCategory::CONFIGURATION: return "CONFIGURATION";
        case LogCategory::REGULATORY: return "REGULATORY";
        case LogCategory::EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

std::string ComplianceLogEntry::get_standard_name(compliance::EtsiStandard standard) {
    switch (standard) {
        case compliance::EtsiStandard::EN_300_401: return "EN_300_401";
        case compliance::EtsiStandard::EN_302_077: return "EN_302_077";
        case compliance::EtsiStandard::EN_300_799: return "EN_300_799";
        case compliance::EtsiStandard::TS_102_563: return "TS_102_563";
        case compliance::EtsiStandard::TS_101_756: return "TS_101_756";
        default: return "UNKNOWN";
    }
}

// EtsiComplianceLogger implementation
EtsiComplianceLogger::EtsiComplianceLogger(const AuditTrailConfig& config)
    : config_(config) {}

EtsiComplianceLogger::~EtsiComplianceLogger() {
    shutdown();
}

bool EtsiComplianceLogger::initialize() {
    if (initialized_.load()) {
        return true;
    }
    
    try {
        // Initialize logging components
        formatter_ = std::make_unique<LogFormatter>();
        writer_ = std::make_unique<LogWriter>(config_);
        archiver_ = std::make_unique<LogArchiver>(config_);
        query_engine_ = std::make_unique<LogQueryEngine>();
        
        if (!writer_->initialize()) {
            return false;
        }
        
        // Start background logging thread
        logging_thread_ = std::make_unique<std::thread>(&EtsiComplianceLogger::logging_loop, this);
        
        initialized_.store(true);
        
        // Log initialization
        log(LogSeverity::INFO, LogCategory::CONFIGURATION,
            "ETSI Compliance Logger initialized",
            "Logger initialized with audit trail configuration");
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void EtsiComplianceLogger::shutdown() {
    if (!initialized_.load()) {
        return;
    }
    
    // Log shutdown
    log(LogSeverity::INFO, LogCategory::CONFIGURATION,
        "ETSI Compliance Logger shutting down",
        "Logger shutdown initiated");
    
    shutdown_requested_.store(true);
    
    // Notify logging thread to wake up and exit
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_condition_.notify_all();
    }
    
    // Wait for logging thread to finish
    if (logging_thread_ && logging_thread_->joinable()) {
        logging_thread_->join();
    }
    
    // Final flush and shutdown
    if (writer_) {
        writer_->shutdown();
    }
    
    initialized_.store(false);
}

void EtsiComplianceLogger::log_compliance_result(const compliance::ComplianceResult& result,
                                                const std::string& context) {
    LogSeverity severity = result.is_compliant ? LogSeverity::INFO : LogSeverity::ERROR;
    
    std::ostringstream message;
    message << "Compliance validation ";
    message << (result.is_compliant ? "passed" : "failed");
    message << " - Level: " << static_cast<int>(result.achieved_level);
    message << ", Overall: " << std::fixed << std::setprecision(1) << result.get_overall_compliance() << "%";
    
    std::ostringstream details;
    details << "Violations: " << result.violations.size();
    details << ", Warnings: " << result.warnings.size();
    details << ", Frames: " << result.frames_analyzed;
    details << ", Time: " << result.validation_time.count() << "μs";
    if (!context.empty()) {
        details << ", Context: " << context;
    }
    
    ComplianceLogEntry entry(severity, LogCategory::ETSI_VALIDATION, message.str());
    entry.details = details.str();
    entry.processing_time = result.validation_time;
    
    // Add standard-specific compliance percentages
    for (const auto& pair : result.compliance_percentages) {
        std::string key = "compliance_" + ComplianceLogEntry::get_standard_name(pair.first);
        entry.metadata[key] = std::to_string(pair.second);
    }
    
    add_to_queue(std::move(entry));
    
    // Log individual violations
    for (const auto& violation : result.violations) {
        ComplianceLogEntry violation_entry(LogSeverity::ERROR, LogCategory::ETSI_VALIDATION,
                                          "Compliance violation detected");
        violation_entry.details = violation.description;
        violation_entry.related_standard = violation.standard;
        violation_entry.entity_id = "Frame " + std::to_string(violation.frame_number);
        violation_entry.metadata["location"] = violation.location;
        violation_entry.metadata["recommendation"] = violation.recommendation;
        
        add_to_queue(std::move(violation_entry));
    }
}

void EtsiComplianceLogger::log_eti_frame_analysis(const EtiFrame& frame,
                                                 const std::string& analysis_result,
                                                 std::chrono::microseconds processing_time) {
    ComplianceLogEntry entry(LogSeverity::DEBUG, LogCategory::FRAME_ANALYSIS,
                           "ETI frame analysis completed");
    entry.details = analysis_result;
    entry.processing_time = processing_time;
    
    auto lidata = frame.get_lidata_field();
    entry.entity_id = "Frame " + std::to_string(lidata.fc);
    entry.metadata["frame_count"] = std::to_string(lidata.fc);
    entry.metadata["mode_id"] = std::to_string(lidata.mid);
    entry.metadata["nst"] = std::to_string(lidata.nst);
    entry.metadata["frame_valid"] = frame.is_valid() ? "true" : "false";
    
    add_to_queue(std::move(entry));
}

void EtsiComplianceLogger::log_fig_processing(uint8_t fig_type,
                                            const std::vector<uint8_t>& fig_data,
                                            const std::string& result,
                                            const std::vector<std::string>& compliance_issues) {
    LogSeverity severity = compliance_issues.empty() ? LogSeverity::DEBUG : LogSeverity::WARNING;
    
    ComplianceLogEntry entry(severity, LogCategory::FIG_PROCESSING,
                           "FIG processing completed");
    entry.details = result;
    entry.metadata["fig_type"] = std::to_string(fig_type);
    entry.metadata["fig_size"] = std::to_string(fig_data.size());
    entry.metadata["issues_count"] = std::to_string(compliance_issues.size());
    
    if (!compliance_issues.empty()) {
        entry.metadata["issues"] = std::accumulate(compliance_issues.begin(), compliance_issues.end(),
                                                  std::string{},
                                                  [](const std::string& a, const std::string& b) {
                                                      return a.empty() ? b : a + "; " + b;
                                                  });
    }
    
    add_to_queue(std::move(entry));
}

void EtsiComplianceLogger::log_audio_quality(uint32_t service_id,
                                            const std::map<std::string, double>& quality_metrics,
                                            bool compliance_status) {
    LogSeverity severity = compliance_status ? LogSeverity::INFO : LogSeverity::WARNING;
    
    ComplianceLogEntry entry(severity, LogCategory::AUDIO_QUALITY,
                           compliance_status ? "Audio quality compliant" : "Audio quality issues detected");
    entry.entity_id = "Service " + std::to_string(service_id);
    entry.related_standard = compliance::EtsiStandard::EN_302_077;
    
    // Add quality metrics to metadata
    for (const auto& metric : quality_metrics) {
        entry.metadata[metric.first] = std::to_string(metric.second);
    }
    
    add_to_queue(std::move(entry));
}

void EtsiComplianceLogger::log_service_event(uint32_t service_id,
                                            const std::string& event_type,
                                            const std::string& details,
                                            const std::string& impact) {
    LogSeverity severity = LogSeverity::INFO;
    if (event_type.find("error") != std::string::npos || 
        event_type.find("failure") != std::string::npos) {
        severity = LogSeverity::ERROR;
    } else if (event_type.find("warning") != std::string::npos) {
        severity = LogSeverity::WARNING;
    }
    
    ComplianceLogEntry entry(severity, LogCategory::SERVICE_MONITORING,
                           "Service event: " + event_type);
    entry.details = details;
    entry.entity_id = "Service " + std::to_string(service_id);
    entry.metadata["event_type"] = event_type;
    
    if (!impact.empty()) {
        entry.metadata["impact"] = impact;
    }
    
    add_to_queue(std::move(entry));
}

void EtsiComplianceLogger::log_alert(const alerts::Alert& alert) {
    LogSeverity severity = LogSeverity::INFO;
    switch (alert.severity) {
        case alerts::AlertSeverity::EMERGENCY:
            severity = LogSeverity::EMERGENCY;
            break;
        case alerts::AlertSeverity::CRITICAL:
            severity = LogSeverity::CRITICAL;
            break;
        case alerts::AlertSeverity::WARNING:
            severity = LogSeverity::WARNING;
            break;
        case alerts::AlertSeverity::NOTICE:
            severity = LogSeverity::NOTICE;
            break;
        case alerts::AlertSeverity::INFO:
            severity = LogSeverity::INFO;
            break;
    }
    
    ComplianceLogEntry entry(severity, LogCategory::REGULATORY,
                           "Alert generated: " + alert.title);
    entry.details = alert.description;
    entry.entity_id = alert.location;
    entry.related_standard = alert.related_standard;
    entry.metadata["alert_id"] = std::to_string(alert.alert_id);
    entry.metadata["alert_category"] = std::to_string(static_cast<int>(alert.category));
    
    // Copy alert metadata
    for (const auto& pair : alert.metadata) {
        entry.metadata["alert_" + pair.first] = pair.second;
    }
    
    add_to_queue(std::move(entry));
}

void EtsiComplianceLogger::log_performance_metrics(const std::map<std::string, double>& metrics,
                                                  const std::string& component) {
    ComplianceLogEntry entry(LogSeverity::DEBUG, LogCategory::PERFORMANCE,
                           "Performance metrics collected");
    entry.details = "Component: " + component;
    
    for (const auto& metric : metrics) {
        entry.metadata[metric.first] = std::to_string(metric.second);
    }
    
    add_to_queue(std::move(entry));
}

void EtsiComplianceLogger::log_configuration_change(const std::string& component,
                                                   const std::string& old_config,
                                                   const std::string& new_config,
                                                   const std::string& changed_by) {
    ComplianceLogEntry entry(LogSeverity::NOTICE, LogCategory::CONFIGURATION,
                           "Configuration changed: " + component);
    entry.details = "Changed by: " + changed_by;
    entry.metadata["component"] = component;
    entry.metadata["old_config"] = old_config;
    entry.metadata["new_config"] = new_config;
    entry.metadata["changed_by"] = changed_by;
    
    add_to_queue(std::move(entry));
}

void EtsiComplianceLogger::log_regulatory_compliance(const std::string& regulation,
                                                    const std::string& compliance_check,
                                                    const std::string& result,
                                                    const std::string& corrective_action) {
    LogSeverity severity = result.find("pass") != std::string::npos ? LogSeverity::INFO : LogSeverity::WARNING;
    
    ComplianceLogEntry entry(severity, LogCategory::REGULATORY,
                           "Regulatory compliance check: " + regulation);
    entry.details = compliance_check + " - " + result;
    entry.metadata["regulation"] = regulation;
    entry.metadata["check"] = compliance_check;
    entry.metadata["result"] = result;
    
    if (!corrective_action.empty()) {
        entry.metadata["corrective_action"] = corrective_action;
    }
    
    add_to_queue(std::move(entry));
}

void EtsiComplianceLogger::log(LogSeverity severity,
                              LogCategory category,
                              const std::string& message,
                              const std::string& details,
                              compliance::EtsiStandard standard,
                              const std::map<std::string, std::string>& metadata) {
    ComplianceLogEntry entry(severity, category, message, standard);
    entry.details = details;
    entry.metadata = metadata;
    
    // Add session context
    {
        std::lock_guard<std::mutex> lock(context_mutex_);
        entry.session_id = current_session_id_;
        entry.user_context = current_user_context_;
        entry.operation_context = current_operation_context_;
    }
    
    add_to_queue(std::move(entry));
}

void EtsiComplianceLogger::set_session_context(const std::string& session_id,
                                              const std::string& user_context) {
    std::lock_guard<std::mutex> lock(context_mutex_);
    current_session_id_ = session_id;
    current_user_context_ = user_context;
}

void EtsiComplianceLogger::set_operation_context(const std::string& operation_context) {
    std::lock_guard<std::mutex> lock(context_mutex_);
    current_operation_context_ = operation_context;
}

ComplianceLoggingStats EtsiComplianceLogger::get_statistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return statistics_;
}

void EtsiComplianceLogger::flush_logs() {
    if (writer_) {
        writer_->flush();
    }
}

// Private implementation methods
void EtsiComplianceLogger::logging_loop() {
    while (!shutdown_requested_.load()) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Wait for log entries or shutdown
        queue_condition_.wait_for(lock, config_.flush_interval, [this] {
            return !log_queue_.empty() || shutdown_requested_.load();
        });
        
        // Process all queued log entries
        while (!log_queue_.empty()) {
            auto entry = std::move(log_queue_.front());
            log_queue_.pop();
            lock.unlock();
            
            process_log_entry(entry);
            
            lock.lock();
        }
        
        // Periodic flush
        if (writer_) {
            writer_->flush();
        }
    }
}

void EtsiComplianceLogger::process_log_entry(const ComplianceLogEntry& entry) {
    if (!should_log_entry(entry)) {
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        std::string formatted_entry = formatter_->format_entry(entry, "text");
        
        if (writer_) {
            writer_->write_entry(formatted_entry);
        }
        
        update_statistics(entry);
        
    } catch (const std::exception& e) {
        // Handle logging errors
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Update average processing time
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (statistics_.avg_logging_time.count() == 0) {
        statistics_.avg_logging_time = processing_time;
    } else {
        statistics_.avg_logging_time = (statistics_.avg_logging_time + processing_time) / 2;
    }
    
    if (processing_time > statistics_.max_logging_time) {
        statistics_.max_logging_time = processing_time;
    }
}

void EtsiComplianceLogger::update_statistics(const ComplianceLogEntry& entry) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    statistics_.total_entries_logged++;
    statistics_.entries_by_severity[static_cast<int>(entry.severity)]++;
    statistics_.entries_by_category[static_cast<int>(entry.category)]++;
    
    if (entry.severity >= LogSeverity::ERROR) {
        statistics_.compliance_violations++;
    }
    if (entry.severity >= LogSeverity::CRITICAL) {
        statistics_.critical_issues++;
    }
    if (entry.severity == LogSeverity::WARNING) {
        statistics_.warnings_generated++;
    }
    
    statistics_.last_log_time = entry.timestamp;
    
    if (statistics_.total_entries_logged == 1) {
        statistics_.first_log_time = entry.timestamp;
    }
}

bool EtsiComplianceLogger::should_log_entry(const ComplianceLogEntry& entry) const {
    return entry.severity >= minimum_severity_;
}

void EtsiComplianceLogger::add_to_queue(ComplianceLogEntry&& entry) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        // Check buffer size limit
        if (log_queue_.size() >= config_.buffer_size) {
            log_queue_.pop(); // Remove oldest entry
        }
        
        log_queue_.push(std::move(entry));
    }
    
    queue_condition_.notify_one();
}

// Utility function implementations
namespace utils {

std::string format_log_entry(const ComplianceLogEntry& entry, bool include_metadata) {
    std::ostringstream oss;
    auto time_t_timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
    
    oss << "[" << std::put_time(std::localtime(&time_t_timestamp), "%Y-%m-%d %H:%M:%S") << "] "
        << "[" << entry.get_severity_string() << "] "
        << "[" << entry.get_category_string() << "] "
        << entry.message;
        
    if (!entry.details.empty()) {
        oss << "\n  Details: " << entry.details;
    }
    
    if (!entry.entity_id.empty()) {
        oss << "\n  Entity: " << entry.entity_id;
    }
    
    if (entry.processing_time.count() > 0) {
        oss << "\n  Processing Time: " << entry.processing_time.count() << "μs";
    }
    
    if (include_metadata && !entry.metadata.empty()) {
        oss << "\n  Metadata:";
        for (const auto& pair : entry.metadata) {
            oss << "\n    " << pair.first << ": " << pair.second;
        }
    }
    
    return oss.str();
}

std::string generate_log_summary(const std::vector<ComplianceLogEntry>& entries) {
    if (entries.empty()) {
        return "No log entries found";
    }
    
    std::ostringstream oss;
    std::map<LogSeverity, int> severity_counts;
    std::map<LogCategory, int> category_counts;
    
    for (const auto& entry : entries) {
        severity_counts[entry.severity]++;
        category_counts[entry.category]++;
    }
    
    oss << "=== LOG SUMMARY ===\n";
    oss << "Total Entries: " << entries.size() << "\n";
    oss << "Time Range: " << std::put_time(std::localtime(&(std::chrono::system_clock::to_time_t(entries.back().timestamp))), "%Y-%m-%d %H:%M:%S");
    oss << " to " << std::put_time(std::localtime(&(std::chrono::system_clock::to_time_t(entries.front().timestamp))), "%Y-%m-%d %H:%M:%S") << "\n\n";
    
    oss << "By Severity:\n";
    for (const auto& pair : severity_counts) {
        oss << "  " << ComplianceLogEntry::get_severity_name(pair.first) << ": " << pair.second << "\n";
    }
    
    oss << "\nBy Category:\n";
    for (const auto& pair : category_counts) {
        oss << "  " << ComplianceLogEntry::get_category_name(pair.first) << ": " << pair.second << "\n";
    }
    
    return oss.str();
}

bool requires_immediate_notification(LogSeverity severity) {
    return severity >= LogSeverity::CRITICAL;
}

uint32_t calculate_log_priority(const ComplianceLogEntry& entry) {
    return static_cast<uint32_t>(entry.severity) * 1000 + 
           static_cast<uint32_t>(entry.category) * 10;
}

std::string convert_to_regulatory_format(const std::vector<ComplianceLogEntry>& entries,
                                        const std::string& regulation_standard) {
    std::ostringstream oss;
    
    oss << "REGULATORY COMPLIANCE REPORT\n";
    oss << "Standard: " << regulation_standard << "\n";
    oss << "Generated: " << std::put_time(std::localtime(&(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))), "%Y-%m-%d %H:%M:%S") << "\n";
    oss << "Entry Count: " << entries.size() << "\n\n";
    
    for (const auto& entry : entries) {
        oss << "Entry ID: " << entry.log_id << "\n";
        oss << "Timestamp: " << std::put_time(std::localtime(&(std::chrono::system_clock::to_time_t(entry.timestamp))), "%Y-%m-%d %H:%M:%S") << "\n";
        oss << "Severity: " << entry.get_severity_string() << "\n";
        oss << "Message: " << entry.message << "\n";
        if (!entry.details.empty()) {
            oss << "Details: " << entry.details << "\n";
        }
        oss << "---\n";
    }
    
    return oss.str();
}

} // namespace utils

} // namespace logging
} // namespace etsi