/**
 * @file etsi_integration_manager.h
 * @brief ETSI Standards Compliance Integration Manager
 * 
 * Central integration manager that coordinates all ETSI compliance components
 * including compliance engine, alert system, logging framework, reporting system,
 * broadcast standards framework, and real-time monitoring for seamless
 * professional broadcast operations.
 * 
 * Features:
 * - Centralized ETSI compliance system coordination
 * - Component lifecycle management and initialization
 * - Configuration management across all components
 * - Performance monitoring and optimization
 * - Error handling and recovery mechanisms
 * - Professional broadcast workflow integration
 * - Cross-component data flow orchestration
 * - System health monitoring and diagnostics
 * 
 * Integration Capabilities:
 * - Real-time compliance validation with alerting
 * - Automated report generation and distribution
 * - Audit trail logging with regulatory compliance
 * - Performance monitoring with optimization
 * - Multi-stream concurrent processing
 * - Emergency alert system integration
 * - Broadcast standards compliance validation
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#ifndef ETSI_INTEGRATION_MANAGER_H
#define ETSI_INTEGRATION_MANAGER_H

#include "compliance_engine.h"
#include "alert_system.h"
#include "compliance_logger.h"
#include "compliance_reporter.h"
#include "broadcast_standards.h"
#include "realtime_monitor.h"
#include "../eti_types.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

namespace etsi {
namespace integration {

/**
 * @brief Integration system status
 */
enum class SystemStatus : uint8_t {
    UNINITIALIZED = 0,      // System not initialized
    INITIALIZING = 1,       // System initialization in progress
    READY = 2,              // System ready for operation
    RUNNING = 3,            // System actively running
    DEGRADED = 4,           // System running with degraded performance
    ERROR = 5,              // System error state
    SHUTTING_DOWN = 6,      // System shutdown in progress
    SHUTDOWN = 7            // System shutdown complete
};

/**
 * @brief Component health status
 */
enum class ComponentHealth : uint8_t {
    HEALTHY = 1,            // Component operating normally
    WARNING = 2,            // Component has warnings
    DEGRADED = 3,           // Component performance degraded
    CRITICAL = 4,           // Component critical issues
    FAILED = 5,             // Component failed
    UNAVAILABLE = 6         // Component unavailable
};

/**
 * @brief System component identifiers
 */
enum class SystemComponent : uint8_t {
    COMPLIANCE_ENGINE = 1,
    ALERT_SYSTEM = 2,
    LOGGER = 3,
    REPORTER = 4,
    BROADCAST_FRAMEWORK = 5,
    REALTIME_MONITOR = 6,
    INTEGRATION_MANAGER = 7
};

/**
 * @brief Component health information
 */
struct ComponentHealthInfo {
    SystemComponent component;
    ComponentHealth health;
    std::string status_message;
    std::chrono::system_clock::time_point last_update;
    std::map<std::string, std::string> diagnostics;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    
    ComponentHealthInfo(SystemComponent comp) 
        : component(comp), health(ComponentHealth::UNAVAILABLE),
          last_update(std::chrono::system_clock::now()) {}
};

/**
 * @brief System configuration for all components
 */
struct EtsiSystemConfig {
    // Compliance engine configuration
    compliance::EtsiComplianceEngine::Config compliance_config;
    
    // Alert system configuration
    alerts::AlertSystemConfig alert_config;
    
    // Logging configuration
    logging::AuditTrailConfig logging_config;
    
    // Reporting configuration
    reporting::EtsiComplianceReporter::Config reporting_config;
    
    // Broadcast standards configuration
    broadcast::BroadcastStandardsFramework::Config broadcast_config;
    
    // Real-time monitoring configuration
    realtime::EtsiRealtimeMonitor::Config realtime_config;
    
    // Integration manager settings
    bool enable_auto_recovery = true;
    bool enable_performance_optimization = true;
    bool enable_health_monitoring = true;
    std::chrono::seconds health_check_interval{30};
    std::chrono::minutes auto_report_interval{60};
    
    EtsiSystemConfig() = default;
};

/**
 * @brief System performance summary
 */
struct SystemPerformanceSummary {
    // Overall system metrics
    std::chrono::microseconds avg_processing_latency{0};
    std::chrono::microseconds max_processing_latency{0};
    double cpu_usage_percentage = 0.0;
    double memory_usage_mb = 0.0;
    uint32_t active_streams = 0;
    
    // Component-specific metrics
    compliance::EtsiComplianceEngine::PerformanceMetrics compliance_metrics;
    alerts::EtsiAlertSystem::AlertStatistics alert_statistics;
    logging::ComplianceLoggingStats logging_statistics;
    reporting::EtsiComplianceReporter::ReporterStatistics reporting_statistics;
    broadcast::BroadcastStandardsFramework::PerformanceMetrics broadcast_metrics;
    realtime::RealtimePerformanceMetrics realtime_metrics;
    
    // System health indicators
    std::map<SystemComponent, ComponentHealth> component_health;
    uint32_t total_violations_detected = 0;
    uint32_t total_alerts_generated = 0;
    double overall_compliance_rate = 0.0;
    
    std::chrono::system_clock::time_point measurement_time;
    
    SystemPerformanceSummary() : measurement_time(std::chrono::system_clock::now()) {}
};

/**
 * @brief Workflow automation configuration
 */
struct WorkflowAutomation {
    // Automatic reporting
    bool enable_daily_reports = true;
    bool enable_weekly_reports = true;
    bool enable_monthly_reports = true;
    std::chrono::hours daily_report_time{6};    // 6 AM
    
    // Alert automation
    bool enable_alert_escalation = true;
    std::chrono::minutes alert_escalation_timeout{15};
    bool enable_auto_acknowledgment = false;
    std::chrono::minutes auto_ack_timeout{30};
    
    // Performance optimization
    bool enable_auto_tuning = true;
    std::chrono::minutes optimization_interval{60};
    double performance_threshold = 0.8;        // 80% efficiency threshold
    
    // Recovery automation
    bool enable_auto_restart = true;
    uint32_t max_restart_attempts = 3;
    std::chrono::seconds restart_delay{30};
    
    WorkflowAutomation() = default;
};

// Forward declarations
class SystemHealthMonitor;
class PerformanceOptimizer;
class WorkflowManager;

/**
 * @brief ETSI Standards Compliance Integration Manager
 * 
 * Central coordinator for all ETSI compliance components providing
 * unified system management, performance monitoring, automated workflows,
 * and professional broadcast operations integration.
 */
class EtsiIntegrationManager {
public:
    explicit EtsiIntegrationManager(const EtsiSystemConfig& config = EtsiSystemConfig{});
    ~EtsiIntegrationManager();
    
    // Disable copy/move for singleton pattern
    EtsiIntegrationManager(const EtsiIntegrationManager&) = delete;
    EtsiIntegrationManager& operator=(const EtsiIntegrationManager&) = delete;
    EtsiIntegrationManager(EtsiIntegrationManager&&) = delete;
    EtsiIntegrationManager& operator=(EtsiIntegrationManager&&) = delete;
    
    /**
     * @brief Initialize the entire ETSI compliance system
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown the entire ETSI compliance system
     */
    void shutdown();
    
    /**
     * @brief Start system operations
     * @return true if started successfully
     */
    bool start();
    
    /**
     * @brief Stop system operations
     * @return true if stopped successfully
     */
    bool stop();
    
    /**
     * @brief Get current system status
     * @return Current system status
     */
    SystemStatus get_system_status() const { return system_status_.load(); }
    
    /**
     * @brief Check if system is ready for operations
     * @return true if system is ready
     */
    bool is_ready() const { return system_status_.load() >= SystemStatus::READY; }
    
    /**
     * @brief Check if system is running
     * @return true if system is running
     */
    bool is_running() const { return system_status_.load() == SystemStatus::RUNNING; }
    
    /**
     * @brief Process ETI frame through complete compliance pipeline
     * @param frame ETI frame to process
     * @param stream_id Associated stream ID
     * @return Comprehensive compliance result
     */
    compliance::ComplianceResult process_eti_frame(const EtiFrame& frame, uint32_t stream_id);
    
    /**
     * @brief Process audio data through quality monitoring pipeline
     * @param audio_data Raw audio samples
     * @param sample_rate Audio sample rate
     * @param channels Number of channels
     * @param service_id Associated service ID
     * @return Audio quality compliance result
     */
    broadcast::BroadcastComplianceResult process_audio_data(
        const std::vector<float>& audio_data,
        uint32_t sample_rate,
        uint32_t channels,
        uint32_t service_id
    );
    
    /**
     * @brief Add stream source for monitoring
     * @param source Stream source configuration
     * @return true if added successfully
     */
    bool add_stream_source(const realtime::StreamSource& source);
    
    /**
     * @brief Remove stream source
     * @param stream_id Stream ID to remove
     * @return true if removed successfully
     */
    bool remove_stream_source(uint32_t stream_id);
    
    /**
     * @brief Generate comprehensive system report
     * @param scope Report scope configuration
     * @param format Report format
     * @return Report generation result
     */
    reporting::ReportGenerationResult generate_system_report(
        const reporting::ReportScope& scope,
        reporting::ReportFormat format = reporting::ReportFormat::HTML
    );
    
    /**
     * @brief Get system performance summary
     * @return Current system performance metrics
     */
    SystemPerformanceSummary get_performance_summary() const;
    
    /**
     * @brief Get component health information
     * @param component Component to check
     * @return Component health information
     */
    ComponentHealthInfo get_component_health(SystemComponent component) const;
    
    /**
     * @brief Get all component health information
     * @return Vector of all component health info
     */
    std::vector<ComponentHealthInfo> get_all_component_health() const;
    
    /**
     * @brief Update system configuration
     * @param config New system configuration
     * @return true if update successful
     */
    bool update_configuration(const EtsiSystemConfig& config);
    
    /**
     * @brief Get current system configuration
     * @return Current configuration
     */
    const EtsiSystemConfig& get_configuration() const { return config_; }
    
    /**
     * @brief Set workflow automation configuration
     * @param automation Automation configuration
     */
    void set_workflow_automation(const WorkflowAutomation& automation);
    
    /**
     * @brief Enable/disable automatic optimization
     * @param enabled true to enable automatic optimization
     */
    void enable_auto_optimization(bool enabled);
    
    /**
     * @brief Trigger manual system optimization
     * @return true if optimization successful
     */
    bool optimize_system_performance();
    
    /**
     * @brief Get individual component interfaces
     */
    std::shared_ptr<compliance::EtsiComplianceEngine> get_compliance_engine() const {
        return compliance_engine_;
    }
    
    std::shared_ptr<alerts::EtsiAlertSystem> get_alert_system() const {
        return alert_system_;
    }
    
    std::shared_ptr<logging::EtsiComplianceLogger> get_logger() const {
        return logger_;
    }
    
    std::shared_ptr<reporting::EtsiComplianceReporter> get_reporter() const {
        return reporter_;
    }
    
    std::shared_ptr<broadcast::BroadcastStandardsFramework> get_broadcast_framework() const {
        return broadcast_framework_;
    }
    
    std::shared_ptr<realtime::EtsiRealtimeMonitor> get_realtime_monitor() const {
        return realtime_monitor_;
    }
    
    /**
     * @brief Register system event callbacks
     */
    using SystemStatusCallback = std::function<void(SystemStatus, SystemStatus)>; // old, new
    using ComponentHealthCallback = std::function<void(SystemComponent, ComponentHealth)>;
    using PerformanceCallback = std::function<void(const SystemPerformanceSummary&)>;
    using AlertCallback = std::function<void(const alerts::Alert&)>;
    
    void register_status_callback(SystemStatusCallback callback);
    void register_health_callback(ComponentHealthCallback callback);
    void register_performance_callback(PerformanceCallback callback);
    void register_alert_callback(AlertCallback callback);
    
    /**
     * @brief Unregister all callbacks
     */
    void unregister_callbacks();
    
    /**
     * @brief Get system diagnostics
     * @return Comprehensive system diagnostics
     */
    std::map<std::string, std::string> get_system_diagnostics() const;
    
    /**
     * @brief Export system configuration
     * @param format Export format ("json", "yaml", "xml")
     * @return Exported configuration string
     */
    std::string export_configuration(const std::string& format = "json") const;
    
    /**
     * @brief Import system configuration
     * @param config_data Configuration data string
     * @param format Import format ("json", "yaml", "xml")
     * @return true if import successful
     */
    bool import_configuration(const std::string& config_data, const std::string& format = "json");
    
    /**
     * @brief Validate system configuration
     * @param config Configuration to validate
     * @return Validation result with errors/warnings
     */
    std::pair<bool, std::vector<std::string>> validate_configuration(const EtsiSystemConfig& config) const;
    
    /**
     * @brief Get system uptime
     * @return System uptime duration
     */
    std::chrono::seconds get_uptime() const;
    
    /**
     * @brief Get system version information
     * @return System version string
     */
    std::string get_version_info() const;

private:
    EtsiSystemConfig config_;
    std::atomic<SystemStatus> system_status_{SystemStatus::UNINITIALIZED};
    std::chrono::system_clock::time_point startup_time_;
    
    // Core components
    std::shared_ptr<compliance::EtsiComplianceEngine> compliance_engine_;
    std::shared_ptr<alerts::EtsiAlertSystem> alert_system_;
    std::shared_ptr<logging::EtsiComplianceLogger> logger_;
    std::shared_ptr<reporting::EtsiComplianceReporter> reporter_;
    std::shared_ptr<broadcast::BroadcastStandardsFramework> broadcast_framework_;
    std::shared_ptr<realtime::EtsiRealtimeMonitor> realtime_monitor_;
    
    // Management components
    std::unique_ptr<SystemHealthMonitor> health_monitor_;
    std::unique_ptr<PerformanceOptimizer> performance_optimizer_;
    std::unique_ptr<WorkflowManager> workflow_manager_;
    
    // Component health tracking
    mutable std::mutex health_mutex_;
    std::map<SystemComponent, ComponentHealthInfo> component_health_;
    
    // Background threads
    std::unique_ptr<std::thread> health_monitoring_thread_;
    std::unique_ptr<std::thread> performance_monitoring_thread_;
    std::unique_ptr<std::thread> workflow_thread_;
    
    // Callback handling
    std::mutex callback_mutex_;
    SystemStatusCallback status_callback_;
    ComponentHealthCallback health_callback_;
    PerformanceCallback performance_callback_;
    AlertCallback alert_callback_;
    
    // Workflow automation
    WorkflowAutomation workflow_automation_;
    
    // Synchronization
    std::atomic<bool> shutdown_requested_{false};
    std::condition_variable status_condition_;
    std::mutex status_mutex_;
    
    // Internal methods
    bool initialize_components();
    bool start_components();
    void stop_components();
    void shutdown_components();
    
    void setup_component_interconnections();
    void start_background_threads();
    void stop_background_threads();
    
    void health_monitoring_loop();
    void performance_monitoring_loop();
    void workflow_management_loop();
    
    void update_component_health(SystemComponent component, ComponentHealth health,
                               const std::string& message = "");
    void set_system_status(SystemStatus status);
    
    void handle_component_failure(SystemComponent component, const std::string& error);
    bool attempt_component_recovery(SystemComponent component);
    
    void notify_status_change(SystemStatus old_status, SystemStatus new_status);
    void notify_health_change(SystemComponent component, ComponentHealth health);
    void notify_performance_update(const SystemPerformanceSummary& summary);
    void notify_alert_generated(const alerts::Alert& alert);
    
    // Utility methods
    static std::string get_system_status_name(SystemStatus status);
    static std::string get_component_health_name(ComponentHealth health);
    static std::string get_component_name(SystemComponent component);
    
    bool validate_component_config(SystemComponent component) const;
    std::string generate_system_info() const;
};

/**
 * @brief System health monitor for component monitoring
 */
class SystemHealthMonitor {
public:
    explicit SystemHealthMonitor(EtsiIntegrationManager* manager) : manager_(manager) {}
    
    ComponentHealth check_component_health(SystemComponent component);
    std::map<SystemComponent, ComponentHealth> check_all_components();
    
private:
    EtsiIntegrationManager* manager_;
    
    ComponentHealth check_compliance_engine_health();
    ComponentHealth check_alert_system_health();
    ComponentHealth check_logger_health();
    ComponentHealth check_reporter_health();
    ComponentHealth check_broadcast_framework_health();
    ComponentHealth check_realtime_monitor_health();
};

/**
 * @brief Performance optimizer for system tuning
 */
class PerformanceOptimizer {
public:
    explicit PerformanceOptimizer(EtsiIntegrationManager* manager) : manager_(manager) {}
    
    bool optimize_system(const SystemPerformanceSummary& current_performance);
    
private:
    EtsiIntegrationManager* manager_;
    
    bool optimize_compliance_engine(const compliance::EtsiComplianceEngine::PerformanceMetrics& metrics);
    bool optimize_realtime_monitor(const realtime::RealtimePerformanceMetrics& metrics);
    bool optimize_memory_usage();
    bool optimize_thread_allocation();
};

/**
 * @brief Workflow manager for automation
 */
class WorkflowManager {
public:
    explicit WorkflowManager(EtsiIntegrationManager* manager) : manager_(manager) {}
    
    void execute_workflows(const WorkflowAutomation& automation);
    
private:
    EtsiIntegrationManager* manager_;
    
    void handle_daily_reporting();
    void handle_alert_escalation();
    void handle_performance_optimization();
    void handle_component_recovery();
};

/**
 * @brief Utility functions for integration management
 */
namespace utils {
    /**
     * @brief Generate system status dashboard
     * @param manager Integration manager instance
     * @return HTML dashboard content
     */
    std::string generate_system_dashboard(const EtsiIntegrationManager& manager);
    
    /**
     * @brief Export system metrics to monitoring format
     * @param performance System performance summary
     * @param format Export format ("prometheus", "influxdb", "json")
     * @return Formatted metrics data
     */
    std::string export_metrics(const SystemPerformanceSummary& performance, 
                              const std::string& format = "json");
    
    /**
     * @brief Create system backup configuration
     * @param manager Integration manager instance
     * @return Backup configuration data
     */
    std::string create_system_backup(const EtsiIntegrationManager& manager);
    
    /**
     * @brief Restore system from backup
     * @param backup_data Backup configuration data
     * @param manager Integration manager instance
     * @return true if restore successful
     */
    bool restore_system_backup(const std::string& backup_data, EtsiIntegrationManager& manager);
    
    /**
     * @brief Validate system readiness for production
     * @param manager Integration manager instance
     * @return Readiness check result with recommendations
     */
    std::pair<bool, std::vector<std::string>> validate_production_readiness(
        const EtsiIntegrationManager& manager
    );
}

} // namespace integration
} // namespace etsi

#endif // ETSI_INTEGRATION_MANAGER_H