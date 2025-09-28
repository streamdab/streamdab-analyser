#pragma once

#include <QObject>
#include <QProgressBar>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <memory>
#include <unordered_map>
#include <string>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

/**
 * @class ProgressManager
 * @brief Professional progress indication system with memory efficiency and thread safety
 * 
 * This class replaces the minimal GREEN phase progress bar operations with a 
 * professional, memory-efficient, and thread-safe progress management system
 * designed for broadcast industry standards.
 * 
 * Features:
 * - Thread-safe progress updates via Qt::QueuedConnection
 * - Memory-efficient operation tracking with smart pointers
 * - Professional progress stages with broadcast industry terminology
 * - Performance-optimized batch updates for >900 FPS processing
 * - ETSI compliance integration for frame processing milestones
 * - Cross-platform compatibility with native progress indicators
 */
class ProgressManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Progress operation stages for broadcast analysis
     */
    enum class ProgressStage {
        Initialization,     ///< System initialization and setup
        FileLoading,       ///< ETI file loading and validation
        FrameParsing,      ///< ETI frame structure parsing
        ServiceDiscovery,  ///< DAB service enumeration
        ComplianceCheck,   ///< ETSI compliance validation
        AudioAnalysis,     ///< Audio stream analysis
        ReportGeneration,  ///< Results compilation and export
        Completed          ///< Operation completed successfully
    };

    /**
     * @brief Progress operation information
     */
    struct ProgressOperation {
        QString name;              ///< Human-readable operation name
        ProgressStage currentStage; ///< Current processing stage
        int progress;              ///< Current progress (0-100)
        int totalFrames;           ///< Total frames to process (for ETI operations)
        int processedFrames;       ///< Frames processed so far
        qint64 startTime;          ///< Operation start timestamp
        bool isActive;             ///< Whether operation is currently active
        
        ProgressOperation() 
            : currentStage(ProgressStage::Initialization)
            , progress(0)
            , totalFrames(0) 
            , processedFrames(0)
            , startTime(0)
            , isActive(false) {}
    };

    /**
     * @brief Constructor
     * @param parent Parent widget for progress display
     */
    explicit ProgressManager(QWidget* parent = nullptr);

    /**
     * @brief Destructor with proper cleanup
     */
    ~ProgressManager() override;

    /**
     * @brief Start a new progress operation
     * @param operationName Human-readable operation name
     * @param totalSteps Total number of steps or frames
     * @return Operation ID for tracking updates
     */
    QString startOperation(const QString& operationName, int totalSteps = 100);

    /**
     * @brief Update progress for specific operation
     * @param operationId Operation identifier from startOperation
     * @param currentStep Current progress step
     * @param stage Optional stage update
     */
    void updateProgress(const QString& operationId, int currentStep, 
                       ProgressStage stage = ProgressStage::FrameParsing);

    /**
     * @brief Complete operation and cleanup resources
     * @param operationId Operation identifier
     */
    void completeOperation(const QString& operationId);

    /**
     * @brief Set progress bar widget for visual updates
     * @param progressBar Progress bar widget to control
     */
    void setProgressBar(QProgressBar* progressBar);

    /**
     * @brief Get current operation progress (thread-safe)
     * @param operationId Operation identifier
     * @return Progress percentage (0-100) or -1 if operation not found
     */
    int getProgress(const QString& operationId) const;

    /**
     * @brief Check if any operations are active
     * @return true if any operation is currently running
     */
    bool hasActiveOperations() const;

    /**
     * @brief Get performance statistics for monitoring
     * @return Performance metrics for >900 FPS validation
     */
    struct PerformanceMetrics {
        int operationsPerSecond;  ///< Operations processed per second
        double averageLatency;    ///< Average update latency in milliseconds
        int memoryUsage;          ///< Current memory usage in KB
        int activeOperations;     ///< Number of active operations
    };
    PerformanceMetrics getPerformanceMetrics() const;

public slots:
    /**
     * @brief Thread-safe slot for ETI frame processing updates
     * @param frameNumber Current frame being processed
     * @param totalFrames Total frames in ETI stream
     */
    void onFrameProcessed(quint64 frameNumber, quint64 totalFrames);

    /**
     * @brief Thread-safe slot for real-time mode progress
     * @param enabled Real-time mode state
     */
    void onRealTimeModeChanged(bool enabled);

    /**
     * @brief Thread-safe slot for ETSI compliance progress
     * @param stage Current compliance checking stage
     * @param progress Progress percentage
     */
    void onComplianceProgress(ProgressStage stage, int progress);

signals:
    /**
     * @brief Signal emitted when progress changes (thread-safe)
     * @param operationId Operation identifier
     * @param progress Current progress (0-100)
     * @param stage Current processing stage
     */
    void progressChanged(const QString& operationId, int progress, ProgressStage stage);

    /**
     * @brief Signal emitted when operation completes
     * @param operationId Operation identifier
     * @param success Whether operation completed successfully
     */
    void operationCompleted(const QString& operationId, bool success);

    /**
     * @brief Signal for performance monitoring
     * @param metrics Current performance statistics
     */
    void performanceUpdate(const PerformanceMetrics& metrics);

private slots:
    /**
     * @brief Internal timer for batch progress updates
     */
    void processBatchUpdates();

    /**
     * @brief Update progress bar widget (UI thread only)
     */
    void updateProgressBarSafely();

private:
    // Thread-safe operation tracking
    mutable QMutex m_operationsMutex;
    std::unordered_map<std::string, std::unique_ptr<ProgressOperation>> m_operations;
    
    // UI components (safe for UI thread access only)
    QProgressBar* m_progressBar;
    QWidget* m_parentWidget;
    
    // Performance optimization
    QTimer* m_batchUpdateTimer;
    QString m_currentActiveOperation;
    
    // Performance tracking
    mutable QMutex m_performanceMutex;
    PerformanceMetrics m_performanceMetrics;
    qint64 m_lastUpdateTime;
    int m_updateCounter;
    
    // Memory efficiency helpers
    void cleanupCompletedOperations();
    QString generateOperationId() const;
    void updatePerformanceMetrics();
    
    // Thread safety validation
    bool isUIThread() const;
    void ensureUIThread(const QString& functionName) const;
};

/**
 * @brief Helper function to convert ProgressStage to human-readable string
 * @param stage Progress stage enumeration
 * @return Human-readable stage description
 */
QString progressStageToString(ProgressManager::ProgressStage stage);