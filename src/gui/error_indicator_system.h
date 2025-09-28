#pragma once

#include <QObject>
#include <QLabel>
#include <QProgressBar>
#include <QWidget>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <memory>

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
class QGraphicsEffect;
QT_END_NAMESPACE

/**
 * @class ErrorIndicatorSystem
 * @brief Professional error visualization system for broadcast industry standards
 * 
 * This class replaces the minimal GREEN phase error display with a sophisticated
 * error indication system that provides immediate visual feedback, categorized
 * error levels, and professional error recovery guidance.
 * 
 * Features:
 * - Professional error categorization (Info, Warning, Error, Critical, ETSI)
 * - Thread-safe error propagation throughout UI components
 * - Memory-efficient error history with automatic cleanup
 * - Visual effects for error attention (animations, color coding)
 * - ETSI compliance error integration with standard references
 * - Performance-optimized error batching for >900 FPS processing
 * - Professional error recovery suggestions and actions
 */
class ErrorIndicatorSystem : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Error severity levels for broadcast analysis
     */
    enum class ErrorSeverity {
        Info,           ///< Informational messages (blue)
        Warning,        ///< Warning conditions (yellow) 
        Error,          ///< Error conditions (orange)
        Critical,       ///< Critical errors (red)
        ETSICompliance, ///< ETSI standard violations (purple)
        Fatal           ///< System fatal errors (dark red, flashing)
    };

    /**
     * @brief Error indicator types for different UI components
     */
    enum class IndicatorType {
        StatusLabel,    ///< Status bar label indication
        ProgressBar,    ///< Progress bar color indication
        Widget,         ///< Widget border/background indication
        Icon,           ///< Icon-based indication
        Animation       ///< Animated visual effect
    };

    /**
     * @brief Error information structure
     */
    struct ErrorInfo {
        QString message;           ///< Error description
        ErrorSeverity severity;    ///< Error severity level
        QString context;           ///< Error context/source
        QString etsiReference;     ///< ETSI standard reference (if applicable)
        QString recoveryAction;    ///< Suggested recovery action
        qint64 timestamp;          ///< Error occurrence time
        bool requiresAttention;    ///< Whether error requires user attention
        bool autoRecover;          ///< Whether system can auto-recover
        
        ErrorInfo()
            : severity(ErrorSeverity::Info)
            , timestamp(0)
            , requiresAttention(false)
            , autoRecover(false) {}
            
        ErrorInfo(const QString& msg, ErrorSeverity sev, const QString& ctx = QString())
            : message(msg), severity(sev), context(ctx), timestamp(QDateTime::currentMSecsSinceEpoch())
            , requiresAttention(sev >= ErrorSeverity::Error)
            , autoRecover(sev <= ErrorSeverity::Warning) {}
    };

    /**
     * @brief Constructor
     * @param parent Parent widget for error indicators
     */
    explicit ErrorIndicatorSystem(QWidget* parent = nullptr);

    /**
     * @brief Destructor with proper cleanup
     */
    ~ErrorIndicatorSystem() override;

    /**
     * @brief Register UI component for error indication
     * @param component UI component to indicate errors on
     * @param type Type of error indication to use
     * @param name Component identifier name
     */
    void registerComponent(QWidget* component, IndicatorType type, const QString& name);

    /**
     * @brief Register status label for error display
     * @param statusLabel Status label widget
     */
    void registerStatusLabel(QLabel* statusLabel);

    /**
     * @brief Register progress bar for error indication
     * @param progressBar Progress bar widget
     */
    void registerProgressBar(QProgressBar* progressBar);

    /**
     * @brief Show error with professional indication (thread-safe)
     * @param errorInfo Complete error information
     */
    void showError(const ErrorInfo& errorInfo);

    /**
     * @brief Show error with basic parameters (thread-safe)
     * @param message Error message
     * @param severity Error severity level
     * @param context Optional error context
     */
    void showError(const QString& message, 
                   ErrorSeverity severity = ErrorSeverity::Error,
                   const QString& context = QString());

    /**
     * @brief Show ETSI compliance error with standard reference
     * @param message Error message
     * @param etsiStandard ETSI standard reference (e.g., "ETSI EN 300 799")
     * @param section Standard section reference
     * @param recoveryAction Suggested recovery action
     */
    void showETSIError(const QString& message, 
                       const QString& etsiStandard,
                       const QString& section = QString(),
                       const QString& recoveryAction = QString());

    /**
     * @brief Clear all error indicators
     */
    void clearAllErrors();

    /**
     * @brief Clear errors by severity level
     * @param severity Severity level to clear
     */
    void clearErrorsBySeverity(ErrorSeverity severity);

    /**
     * @brief Clear errors by context
     * @param context Context/source to clear
     */
    void clearErrorsByContext(const QString& context);

    /**
     * @brief Get current highest severity error
     * @return Most severe active error or empty if none
     */
    ErrorInfo getCurrentError() const;

    /**
     * @brief Check if any errors are active
     * @return true if any errors are currently displayed
     */
    bool hasActiveErrors() const;

    /**
     * @brief Get error count by severity
     * @param severity Severity level to count
     * @return Number of active errors at specified severity
     */
    int getErrorCount(ErrorSeverity severity) const;

    /**
     * @brief Get performance metrics for error system
     */
    struct ErrorPerformanceMetrics {
        int errorsPerSecond;      ///< Errors processed per second
        int activeErrors;         ///< Currently active errors
        double averageLatency;    ///< Average indication latency in milliseconds
        int componentsManaged;    ///< Number of managed UI components
    };
    ErrorPerformanceMetrics getPerformanceMetrics() const;

public slots:
    /**
     * @brief Thread-safe slot for error propagation
     * @param errorMessage Error description
     */
    void onErrorOccurred(const QString& errorMessage);

    /**
     * @brief Thread-safe slot for ETSI compliance errors
     * @param complianceError ETSI compliance error details
     * @param standardReference ETSI standard reference
     */
    void onETSIComplianceError(const QString& complianceError, 
                               const QString& standardReference);

    /**
     * @brief Thread-safe slot for warning conditions
     * @param warningMessage Warning description
     * @param context Warning context
     */
    void onWarningOccurred(const QString& warningMessage, 
                           const QString& context = QString());

    /**
     * @brief Thread-safe slot for critical errors
     * @param criticalError Critical error description
     */
    void onCriticalError(const QString& criticalError);

signals:
    /**
     * @brief Signal emitted when error state changes
     * @param hasErrors Whether any errors are currently active
     * @param highestSeverity Highest severity of active errors
     */
    void errorStateChanged(bool hasErrors, ErrorSeverity highestSeverity);

    /**
     * @brief Signal for critical errors requiring immediate attention
     * @param errorInfo Critical error information
     */
    void criticalErrorRequiresAttention(const ErrorInfo& errorInfo);

    /**
     * @brief Signal for ETSI compliance violations
     * @param errorInfo ETSI compliance error information
     */
    void etsiComplianceViolation(const ErrorInfo& errorInfo);

    /**
     * @brief Signal for performance monitoring
     * @param metrics Current error system performance
     */
    void performanceUpdate(const ErrorPerformanceMetrics& metrics);

private slots:
    /**
     * @brief Process queued error indications
     */
    void processErrorQueue();

    /**
     * @brief Update UI component indicators (UI thread only)
     */
    void updateComponentIndicators();

    /**
     * @brief Clean up expired error indicators
     */
    void cleanupExpiredErrors();

    /**
     * @brief Handle animation completion
     */
    void onAnimationFinished();

private:
    // Thread-safe error tracking
    mutable QMutex m_errorMutex;
    QQueue<ErrorInfo> m_errorQueue;
    QList<ErrorInfo> m_activeErrors;
    
    // UI component management
    struct ComponentInfo {
        QWidget* widget;
        IndicatorType type;
        QString name;
        QString originalStyleSheet;
        QPropertyAnimation* animation;
        
        ComponentInfo() : widget(nullptr), animation(nullptr) {}
    };
    QList<ComponentInfo> m_managedComponents;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    
    // Performance optimization
    QTimer* m_processTimer;
    QTimer* m_cleanupTimer;
    
    // Performance tracking
    mutable QMutex m_performanceMutex;
    ErrorPerformanceMetrics m_performanceMetrics;
    qint64 m_lastUpdateTime;
    int m_errorCounter;
    
    // Configuration
    static const int PROCESS_INTERVAL_MS = 100;   // 10 FPS error updates
    static const int CLEANUP_INTERVAL_MS = 5000;  // 5 second cleanup
    static const int MAX_ACTIVE_ERRORS = 50;      // Memory limit
    
    // Helper methods
    void enqueueError(const ErrorInfo& errorInfo);
    void applyErrorIndication(const ComponentInfo& component, const ErrorInfo& error);
    void clearErrorIndication(const ComponentInfo& component);
    QString getStyleSheetForSeverity(ErrorSeverity severity) const;
    QColor getColorForSeverity(ErrorSeverity severity) const;
    void startErrorAnimation(QWidget* widget, ErrorSeverity severity);
    void updatePerformanceMetrics();
    bool isUIThread() const;
    void ensureUIThread(const QString& functionName) const;
    ErrorSeverity getHighestSeverity() const;
    void removeExpiredErrors();
};

/**
 * @brief Helper function to convert ErrorSeverity to human-readable string
 * @param severity Error severity enumeration
 * @return Human-readable severity description
 */
QString errorSeverityToString(ErrorIndicatorSystem::ErrorSeverity severity);

/**
 * @brief Helper function to get ETSI standard URL for reference
 * @param standard ETSI standard identifier (e.g., "EN 300 799")
 * @return URL to ETSI standard documentation
 */
QString getETSIStandardURL(const QString& standard);