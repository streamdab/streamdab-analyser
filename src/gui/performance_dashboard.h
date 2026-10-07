#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QTimer>
#include <QFrame>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QColor>
#include <QMutex>
#include <QDateTime>
#include <QComboBox>
#include <deque>

// Forward declarations
class EtiProcessor;  // Qt camelCase for GUI layer

/**
 * @struct PerformanceStats
 * @brief Performance statistics structure for real-time monitoring
 */
struct PerformanceStats {
    double frameRate;           // FPS
    double memoryUsage;         // MB
    quint64 frameCount;         // Total frames processed
    double cpuUsage;            // CPU percentage
    double averageLatency;      // ms
    QDateTime timestamp;
    
    PerformanceStats() : frameRate(0.0), memoryUsage(0.0), frameCount(0), 
                        cpuUsage(0.0), averageLatency(0.0), timestamp(QDateTime::currentDateTime()) {}
};

/**
 * @struct ComplianceResult
 * @brief ETSI compliance result structure
 */
struct ComplianceResult {
    double compliancePercentage;    // 0-100%
    int violationCount;
    QString complianceLevel;        // "EXCELLENT", "GOOD", "FAIR", "POOR"
    QStringList activeViolations;
    QDateTime lastUpdate;
    
    ComplianceResult() : compliancePercentage(100.0), violationCount(0), 
                        complianceLevel("EXCELLENT"), lastUpdate(QDateTime::currentDateTime()) {}
};

/**
 * @class PerformanceDashboard
 * @brief Professional real-time performance monitoring dashboard
 * 
 * This widget provides comprehensive real-time performance monitoring
 * for the StreamDAB Analyser with broadcast industry standards:
 * - Live FPS monitoring with 900+ FPS target validation
 * - Memory usage tracking with professional thresholds
 * - ETSI compliance monitoring and violation alerts
 * - CPU usage and processing latency metrics
 * - Historical trend visualization
 * - Professional broadcast industry styling
 */
class PerformanceDashboard : public QWidget
{
    Q_OBJECT
    
public:
    explicit PerformanceDashboard(QWidget *parent = nullptr);
    ~PerformanceDashboard();
    
    // Professional broadcast industry colors (public for chart widgets)
    static const QColor BACKGROUND_DARK;     // #2D2D30
    static const QColor BACKGROUND_MEDIUM;   // #3E3E42
    static const QColor ACCENT_BLUE;         // #0078D4
    static const QColor STATUS_EXCELLENT;    // #00AA00 (Green)
    static const QColor STATUS_GOOD;         // #4CAF50 (Light Green) 
    static const QColor STATUS_FAIR;         // #FFC107 (Amber)
    static const QColor STATUS_POOR;         // #FF9800 (Orange)
    static const QColor STATUS_CRITICAL;     // #DC3545 (Red)
    static const QColor TEXT_PRIMARY;        // #FFFFFF
    static const QColor TEXT_SECONDARY;      // #CCCCCC
    
    /**
     * @brief Connect to ETI processor for performance data
     * @param processor ETI processor instance
     */
    void connectEtiProcessor(EtiProcessor* processor);
    
    /**
     * @brief Start real-time monitoring
     */
    void startMonitoring();
    
    /**
     * @brief Stop real-time monitoring
     */
    void stopMonitoring();
    
    /**
     * @brief Check if monitoring is active
     * @return true if monitoring is running
     */
    bool isMonitoring() const { return m_monitoringActive; }
    
    /**
     * @brief Get current performance statistics
     * @return Current performance stats
     */
    PerformanceStats getCurrentStats() const { return m_currentStats; }
    
    /**
     * @brief Apply professional broadcasting theme
     */
    void applyBroadcastingTheme();
    
    /**
     * @brief Get target FPS for chart scaling
     * @return Target FPS value
     */
    double getTargetFPS() const { return m_targetFPS; }
    
    /**
     * @brief Get target memory MB for chart scaling
     * @return Target memory MB value
     */
    double getTargetMemoryMB() const { return m_targetMemoryMB; }
    
    /**
     * @brief Get target latency MS for chart scaling
     * @return Target latency MS value
     */
    double getTargetLatencyMS() const { return m_targetLatencyMS; }
    
public slots:
    /**
     * @brief Update performance metrics from external source
     * @param stats Performance statistics
     */
    void updatePerformanceMetrics(const PerformanceStats& stats);
    
    /**
     * @brief Update ETSI compliance status
     * @param compliance Compliance result
     */
    void updateComplianceStatus(const ComplianceResult& compliance);
    
    /**
     * @brief Reset performance statistics
     */
    void resetStatistics();
    
    /**
     * @brief Update target thresholds
     * @param targetFPS Target FPS (default: 900)
     * @param targetMemoryMB Target memory in MB (default: 100)
     */
    void updateTargets(double targetFPS = 900.0, double targetMemoryMB = 100.0);
    
signals:
    /**
     * @brief Emitted when performance threshold is exceeded
     * @param metric Metric name
     * @param value Current value
     * @param threshold Threshold value
     */
    void performanceThresholdExceeded(const QString& metric, double value, double threshold);
    
    /**
     * @brief Emitted when compliance violation occurs
     * @param violationType Type of violation
     * @param severity Severity level
     */
    void complianceViolationDetected(const QString& violationType, const QString& severity);
    
private slots:
    /**
     * @brief Handle real-time update timer
     */
    void onUpdateTimer();
    
    /**
     * @brief Handle ETI processor performance update
     * @param frameRate Current frame rate
     * @param memoryMB Current memory usage
     */
    void onEtiPerformanceUpdate(double frameRate, double memoryMB);
    
    /**
     * @brief Handle time range selection change
     * @param index Selected time range index
     */
    void onTimeRangeChanged(int index);
    
private:
    /**
     * @brief Setup dashboard UI components
     */
    void setupUI();
    
    /**
     * @brief Setup performance indicators
     */
    void setupPerformanceIndicators();
    
    /**
     * @brief Setup compliance monitor
     */
    void setupComplianceMonitor();
    
    /**
     * @brief Setup trend charts
     */
    void setupTrendCharts();
    
    /**
     * @brief Update FPS display
     * @param fps Current FPS
     */
    void updateFPSDisplay(double fps);
    
    /**
     * @brief Update memory display
     * @param memoryMB Memory usage in MB
     */
    void updateMemoryDisplay(double memoryMB);
    
    /**
     * @brief Update compliance display
     * @param compliancePercentage Compliance percentage
     */
    void updateComplianceDisplay(double compliancePercentage);
    
    /**
     * @brief Update trend data
     */
    void updateTrendData();
    
    /**
     * @brief Get performance status color
     * @param value Current value
     * @param threshold Threshold value
     * @param invert true if lower values are better
     * @return Status color
     */
    QColor getPerformanceStatusColor(double value, double threshold, bool invert = false) const;
    
    /**
     * @brief Format performance value for display
     * @param value Value to format
     * @param unit Unit string
     * @param precision Decimal precision
     * @return Formatted string
     */
    QString formatPerformanceValue(double value, const QString& unit, int precision = 1) const;
    
    /**
     * @brief Check performance thresholds
     */
    void checkPerformanceThresholds();
    
    /**
     * @brief Update historical data
     */
    void updateHistoricalData();
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QGridLayout* m_metricsLayout;
    QHBoxLayout* m_complianceLayout;
    
    // Performance indicators
    QFrame* m_fpsFrame;
    QLabel* m_fpsLabel;
    QLabel* m_fpsValueLabel;
    QProgressBar* m_fpsProgressBar;
    QLabel* m_fpsStatusLabel;
    
    QFrame* m_memoryFrame;
    QLabel* m_memoryLabel;
    QLabel* m_memoryValueLabel;
    QProgressBar* m_memoryProgressBar;
    QLabel* m_memoryStatusLabel;
    
    QFrame* m_cpuFrame;
    QLabel* m_cpuLabel;
    QLabel* m_cpuValueLabel;
    QProgressBar* m_cpuProgressBar;
    QLabel* m_cpuStatusLabel;
    
    QFrame* m_latencyFrame;
    QLabel* m_latencyLabel;
    QLabel* m_latencyValueLabel;
    QProgressBar* m_latencyProgressBar;
    QLabel* m_latencyStatusLabel;
    
    // Compliance monitor
    QFrame* m_complianceFrame;
    QLabel* m_complianceLabel;
    QLabel* m_complianceScoreLabel;
    QProgressBar* m_complianceProgressBar;
    QTextEdit* m_violationsDisplay;
    
    // Trend charts components
    QFrame* m_trendChartsFrame;
    QWidget* m_chartDisplayArea;
    QComboBox* m_timeRangeCombo;
    
    // Data and state
    EtiProcessor* m_etiProcessor;
    QTimer* m_updateTimer;
    bool m_monitoringActive;
    
    PerformanceStats m_currentStats;
    ComplianceResult m_currentCompliance;
    
    // Performance targets
    double m_targetFPS;
    double m_targetMemoryMB;
    double m_targetCPU;
    double m_targetLatencyMS;
    
    // Historical data for trends
    std::deque<PerformanceStats> m_historicalStats;
    // GUI-thread only: the dashboard is driven by QTimer/Qt signals on the
    // main thread; non-mutable (no const method locks it).
    QMutex m_dataMutex;
    
    // Professional styling
    QFont m_headerFont;
    QFont m_valueFont;
    QFont m_statusFont;
    
    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 1000;  // 1 second updates
    static constexpr int MAX_HISTORICAL_POINTS = 300; // 5 minutes at 1s intervals
    static constexpr double DEFAULT_TARGET_FPS = 900.0;
    static constexpr double DEFAULT_TARGET_MEMORY_MB = 100.0;
    static constexpr double DEFAULT_TARGET_CPU = 50.0;
    static constexpr double DEFAULT_TARGET_LATENCY_MS = 50.0;
};