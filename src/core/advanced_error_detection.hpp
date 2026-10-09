#pragma once

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <QString>
#include <QDateTime>
#include <QDebug>
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <chrono>

namespace error_detection {

// Forward declarations
class ErrorDetectionWorker;

// Error severity levels
enum class ErrorSeverity {
    CRITICAL,    // System-breaking errors requiring immediate attention
    HIGH,        // Major functionality issues
    MEDIUM,      // Quality degradation issues
    LOW,         // Minor issues or warnings
    INFO         // Informational messages
};

// Error categories for classification
enum class ErrorCategory {
    ETI_SYNC,           // ETI synchronization errors
    FRAME_STRUCTURE,    // Frame structure validation errors
    FIC_DECODING,      // FIC decoding errors
    SERVICE_DISCOVERY, // Service discovery issues
    AUDIO_QUALITY,     // Audio quality problems
    NETWORK_STREAM,    // Network streaming issues
    FILE_IO,           // File input/output errors
    MEMORY_MANAGEMENT, // Memory allocation/deallocation issues
    PERFORMANCE,       // Performance degradation
    CONFIGURATION      // Configuration validation errors
};

// Error recovery strategies
enum class RecoveryStrategy {
    NONE,              // No automatic recovery
    RETRY,             // Retry the failed operation
    SKIP_FRAME,        // Skip problematic frame and continue
    RESET_DECODER,     // Reset decoder state
    FALLBACK_METHOD,   // Use alternative processing method
    NOTIFY_USER,       // Notify user for manual intervention
    RESTART_STREAM     // Restart stream processing
};

// Individual error report structure
struct ErrorReport {
    QString error_id;
    ErrorSeverity severity;
    ErrorCategory category;
    QString description;
    QString technical_details;
    QDateTime timestamp;
    uint32_t frame_number;
    QString stream_id;
    RecoveryStrategy suggested_recovery;
    bool auto_recovery_attempted;
    bool recovery_successful;
    std::chrono::milliseconds detection_latency;
    
    ErrorReport() : 
        severity(ErrorSeverity::INFO),
        category(ErrorCategory::ETI_SYNC),
        frame_number(0),
        suggested_recovery(RecoveryStrategy::NONE),
        auto_recovery_attempted(false),
        recovery_successful(false),
        detection_latency(0) {}
};

// Error pattern detection for trend analysis
struct ErrorPattern {
    QString pattern_id;
    ErrorCategory category;
    uint32_t occurrence_count;
    QDateTime first_occurrence;
    QDateTime last_occurrence;
    std::vector<uint32_t> frame_numbers;
    double frequency_per_minute;
    QString pattern_description;
    bool is_critical_pattern;
    
    ErrorPattern() : 
        occurrence_count(0),
        frequency_per_minute(0.0),
        is_critical_pattern(false) {}
};

// System health metrics
struct SystemHealthMetrics {
    double overall_health_score;        // 0-100 scale
    double error_rate_per_minute;
    double critical_error_rate;
    uint32_t total_errors_detected;
    uint32_t successful_recoveries;
    uint32_t failed_recoveries;
    QDateTime last_critical_error;
    std::vector<ErrorPattern> active_patterns;
    
    SystemHealthMetrics() : 
        overall_health_score(100.0),
        error_rate_per_minute(0.0),
        critical_error_rate(0.0),
        total_errors_detected(0),
        successful_recoveries(0),
        failed_recoveries(0) {}
};

// QThread Worker for background error detection processing
class ErrorDetectionWorker : public QObject {
    Q_OBJECT

public:
    explicit ErrorDetectionWorker(QObject* parent = nullptr);
    ~ErrorDetectionWorker() = default;

public slots:
    void startMonitoring();
    void stopMonitoring();
    void performMonitoringCycle();
    void performPatternAnalysis();
    void updateHealthMetrics();

signals:
    void monitoringCycleCompleted();
    void patternAnalysisCompleted();
    void healthMetricsUpdated();
    void errorDetected(const ErrorReport& error);
    void criticalErrorDetected(const ErrorReport& error);

private:
    std::atomic<bool> m_monitoring_active{false};
    QTimer* m_monitoring_timer{nullptr};
    QTimer* m_pattern_timer{nullptr};
    QTimer* m_health_timer{nullptr};
};

// Advanced Error Detection Engine
class AdvancedErrorDetector : public QObject {
    Q_OBJECT

public:
    explicit AdvancedErrorDetector(QObject* parent = nullptr);
    ~AdvancedErrorDetector();

    // Error detection configuration
    void setErrorDetectionEnabled(bool enabled);
    void setRealTimeMonitoringEnabled(bool enabled);
    void setAutoRecoveryEnabled(bool enabled);
    void setDetectionSensitivity(double sensitivity); // 0.0 - 1.0
    
    // Error threshold configuration
    void setCriticalErrorThreshold(uint32_t errors_per_minute);
    void setPerformanceThreshold(double min_fps, double max_memory_mb);
    void setQualityThreshold(double min_quality_score);
    
    // Error detection methods
    bool detectETISyncErrors(const std::vector<uint8_t>& frame_data, uint32_t frame_number);
    bool detectFrameStructureErrors(const std::vector<uint8_t>& frame_data, uint32_t frame_number);
    bool detectFICDecodingErrors(const std::vector<uint8_t>& fic_data, uint32_t frame_number);
    bool detectServiceDiscoveryErrors(const QString& stream_id, uint32_t expected_services, uint32_t found_services);
    bool detectAudioQualityErrors(const QString& stream_id, double quality_score, uint32_t frame_number);
    bool detectNetworkStreamErrors(const QString& stream_url, const QString& error_description);
    bool detectPerformanceErrors(double current_fps, double memory_usage_mb);
    
    // Error recovery methods
    bool attemptAutoRecovery(const ErrorReport& error);
    bool executeRecoveryStrategy(RecoveryStrategy strategy, const ErrorReport& error);
    void resetErrorState(const QString& stream_id = "");
    
    // Error pattern analysis
    void analyzeErrorPatterns();
    std::vector<ErrorPattern> getActiveErrorPatterns() const;
    bool isPatternsDetected() const;
    
    // System health monitoring
    SystemHealthMetrics getSystemHealth() const;
    double calculateHealthScore() const;
    void updateHealthMetrics();
    
    // Error reporting
    std::vector<ErrorReport> getErrorHistory(const QDateTime& since = QDateTime()) const;
    std::vector<ErrorReport> getErrorsByCategory(ErrorCategory category) const;
    std::vector<ErrorReport> getErrorsBySeverity(ErrorSeverity severity) const;
    std::vector<ErrorReport> getCriticalErrors() const;
    
    // Error export
    QString generateErrorReport(const QDateTime& from, const QDateTime& to) const;
    QString generateHealthReport() const;
    bool exportErrorsToFile(const QString& file_path, const QDateTime& from, const QDateTime& to) const;
    
    // Statistics
    uint32_t getTotalErrorCount() const;
    uint32_t getErrorCount(ErrorCategory category) const;
    uint32_t getErrorCount(ErrorSeverity severity) const;
    double getAverageRecoveryTime() const;
    
signals:
    void errorDetected(const ErrorReport& error);
    void criticalErrorDetected(const ErrorReport& error);
    void errorPatternDetected(const ErrorPattern& pattern);
    void autoRecoveryAttempted(const ErrorReport& error, bool successful);
    void systemHealthChanged(const SystemHealthMetrics& health);
    void errorThresholdExceeded(ErrorCategory category, uint32_t count);
    
private slots:
    void onMonitoringTimerTimeout();
    void onPatternAnalysisTimerTimeout();
    void onHealthUpdateTimerTimeout();
    
private:
    // Configuration
    std::atomic<bool> m_error_detection_enabled;
    std::atomic<bool> m_real_time_monitoring_enabled;
    std::atomic<bool> m_auto_recovery_enabled;
    std::atomic<double> m_detection_sensitivity;
    
    // Thresholds
    std::atomic<uint32_t> m_critical_error_threshold;
    std::atomic<double> m_min_fps_threshold;
    std::atomic<double> m_max_memory_threshold;
    std::atomic<double> m_min_quality_threshold;
    
    // Error storage
    mutable QMutex m_errors_mutex;
    std::vector<ErrorReport> m_error_history;
    std::unordered_map<QString, std::vector<ErrorReport>> m_errors_by_stream;
    
    // Pattern detection
    mutable QMutex m_patterns_mutex;
    std::vector<ErrorPattern> m_detected_patterns;
    std::unordered_map<ErrorCategory, ErrorPattern> m_category_patterns;
    
    // System health
    mutable QMutex m_health_mutex;
    SystemHealthMetrics m_system_health;
    
    // QThread-based background processing (replaces blocking timers)
    QThread* m_worker_thread;
    ErrorDetectionWorker* m_worker;
    
    // Internal methods
    QString generateErrorId() const;
    void addErrorToHistory(const ErrorReport& error);
    void initializeWorkerIfNeeded();
    void updateErrorPattern(const ErrorReport& error);
    bool isFrameCorrupted(const std::vector<uint8_t>& frame_data) const;
    bool validateETIFrameStructure(const std::vector<uint8_t>& frame_data) const;
    double calculateRecoverySuccessRate() const;
    void cleanupOldErrors();
    void notifySystemHealth();
    
    // Recovery implementations
    bool retryOperation(const ErrorReport& error);
    bool skipFrame(const ErrorReport& error);
    bool resetDecoder(const ErrorReport& error);
    bool useFallbackMethod(const ErrorReport& error);
    bool restartStream(const ErrorReport& error);
    
    // Pattern detection algorithms
    bool detectRepeatingErrors(ErrorCategory category);
    bool detectErrorBursts(const std::vector<ErrorReport>& errors);
    bool detectDegradationTrend(const std::vector<ErrorReport>& errors);
};

} // namespace error_detection