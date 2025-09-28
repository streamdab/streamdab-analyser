/**
 * @file stream_recording_dialog.h
 * @brief Professional Stream Recording Configuration Dialog
 * 
 * Comprehensive GUI for configuring ETI stream recording with time-based
 * segmentation, format selection, and professional broadcast workflows.
 * 
 * @author UI/UX Agent - Phase 4 Integration
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSlider>
#include <QProgressBar>
#include <QTextEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QSplitter>
#include <QTimer>
#include <QMutex>
#include <QFileSystemWatcher>
#include <QStorageInfo>
#include <memory>

#include "core/stream_recording_manager.hpp"

/**
 * @class StreamRecordingDialog
 * @brief Professional stream recording configuration interface
 * 
 * Features:
 * - Multi-format recording configuration (ETI-NI, ETI-LI, EDI, compressed)
 * - Time-based segmentation settings with live preview
 * - Disk space monitoring and automatic cleanup configuration
 * - Real-time recording status and statistics display
 * - Professional broadcast workflow integration
 * - Recording session management and history
 * - Archive creation and compression settings
 */
class StreamRecordingDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param recordingManager Pointer to the stream recording manager
     * @param parent Parent widget
     */
    explicit StreamRecordingDialog(eti::StreamRecordingManager *recordingManager, QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~StreamRecordingDialog();

    /**
     * @brief Initialize the dialog
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Get the recording manager
     * @return Pointer to recording manager
     */
    eti::StreamRecordingManager* getRecordingManager() const { return m_recordingManager; }

public slots:
    /**
     * @brief Start new recording session
     */
    void startRecordingSession();

    /**
     * @brief Stop active recording session
     */
    void stopRecordingSession();

    /**
     * @brief Pause active recording session
     */
    void pauseRecordingSession();

    /**
     * @brief Resume paused recording session
     */
    void resumeRecordingSession();

    /**
     * @brief Apply recording configuration
     */
    void applyConfiguration();

    /**
     * @brief Reset configuration to defaults
     */
    void resetToDefaults();

    /**
     * @brief Load configuration from file
     */
    void loadConfiguration();

    /**
     * @brief Save configuration to file
     */
    void saveConfiguration();

    /**
     * @brief Browse for output directory
     */
    void browseOutputDirectory();

    /**
     * @brief Test recording settings
     */
    void testRecordingSettings();

signals:
    /**
     * @brief Emitted when recording session starts
     * @param sessionId Session identifier
     */
    void recordingSessionStarted(const QString& sessionId);

    /**
     * @brief Emitted when recording session stops
     * @param sessionId Session identifier
     */
    void recordingSessionStopped(const QString& sessionId);

    /**
     * @brief Emitted when configuration changes
     */
    void configurationChanged();

protected:
    /**
     * @brief Handle accept (OK button)
     */
    void accept() override;

    /**
     * @brief Handle reject (Cancel button)
     */
    void reject() override;

private slots:
    /**
     * @brief Handle output format change
     */
    void handleOutputFormatChanged();

    /**
     * @brief Handle segmentation settings change
     */
    void handleSegmentationChanged();

    /**
     * @brief Handle compression settings change
     */
    void handleCompressionChanged();

    /**
     * @brief Handle output directory change
     */
    void handleOutputDirectoryChanged();

    /**
     * @brief Handle recording session selection
     */
    void handleSessionSelectionChanged();

    /**
     * @brief Update recording status display
     */
    void updateRecordingStatus();

    /**
     * @brief Update disk space display
     */
    void updateDiskSpaceDisplay();

    /**
     * @brief Update session list
     */
    void updateSessionList();

    /**
     * @brief Handle recording session started
     * @param sessionId Session identifier
     */
    void handleRecordingSessionStarted(const QString& sessionId);

    /**
     * @brief Handle recording session stopped
     * @param sessionId Session identifier
     */
    void handleRecordingSessionStopped(const QString& sessionId);

    /**
     * @brief Handle session statistics update
     * @param sessionId Session identifier
     * @param session Session data
     */
    void handleSessionStatisticsUpdate(const QString& sessionId, const eti::RecordingSession& session);

    /**
     * @brief Handle recording error
     * @param sessionId Session identifier
     * @param error Error message
     */
    void handleRecordingError(const QString& sessionId, const QString& error);

private:
    /**
     * @brief Create the dialog UI
     */
    void createUI();

    /**
     * @brief Create recording configuration tab
     */
    void createRecordingTab();

    /**
     * @brief Create segmentation configuration tab
     */
    void createSegmentationTab();

    /**
     * @brief Create format and compression tab
     */
    void createFormatTab();

    /**
     * @brief Create session monitoring tab
     */
    void createMonitoringTab();

    /**
     * @brief Create button box
     */
    void createButtonBox();

    /**
     * @brief Setup recording manager connections
     */
    void setupRecordingManagerConnections();

    /**
     * @brief Load current configuration
     */
    void loadCurrentConfiguration();

    /**
     * @brief Apply UI settings to recording manager
     */
    void applyUISettings();

    /**
     * @brief Update format description
     */
    void updateFormatDescription();

    /**
     * @brief Update segmentation preview
     */
    void updateSegmentationPreview();

    /**
     * @brief Update compression preview
     */
    void updateCompressionPreview();

    /**
     * @brief Validate configuration settings
     * @return true if configuration is valid
     */
    bool validateConfiguration() const;

    /**
     * @brief Apply professional broadcast styling
     */
    void applyBroadcastStyling();

    /**
     * @brief Get format display name
     * @param format Recording format
     * @return Display name for format
     */
    QString getFormatDisplayName(eti::RecordingFormat format) const;

    /**
     * @brief Get session status text
     * @param session Recording session
     * @return Status text
     */
    QString getSessionStatusText(const eti::RecordingSession& session) const;

    /**
     * @brief Format file size
     * @param bytes Size in bytes
     * @return Formatted size string
     */
    QString formatFileSize(qint64 bytes) const;

    /**
     * @brief Format duration
     * @param milliseconds Duration in milliseconds
     * @return Formatted duration string
     */
    QString formatDuration(qint64 milliseconds) const;

    // UI components
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;
    
    // Recording settings tab
    QWidget *m_recordingTab;
    QGroupBox *m_formatGroup;
    QGroupBox *m_segmentationGroup;
    QComboBox *m_formatCombo;
    QLineEdit *m_outputPathEdit;
    QPushButton *m_browseButton;
    QCheckBox *m_enableSegmentationCheck;
    QSpinBox *m_segmentDurationSpin;
    QComboBox *m_segmentUnitCombo;
    
    // Quality settings tab
    QWidget *m_qualityTab;
    QGroupBox *m_compressionGroup;
    QGroupBox *m_metadataGroup;
    QComboBox *m_compressionCombo;
    QSlider *m_qualitySlider;
    QCheckBox *m_includeMetadataCheck;
    
    // Advanced settings tab
    QWidget *m_advancedTab;
    QGroupBox *m_bufferGroup;
    QSpinBox *m_bufferSizeSpin;
    QCheckBox *m_enablePrebufferCheck;
    
    // Status tab
    QWidget *m_statusTab;
    QProgressBar *m_recordingProgress;
    QLabel *m_statusLabel;
    QLabel *m_diskSpaceLabel;
    QPushButton *m_startStopButton;

    // Core components
    eti::StreamRecordingManager *m_recordingManager;

    // Update timers
    QTimer *m_statusUpdateTimer;
    QTimer *m_diskSpaceUpdateTimer;
    QFileSystemWatcher *m_directoryWatcher;

    // State tracking
    bool m_initialized;
    bool m_configurationChanged;
    QString m_currentSessionId;
    eti::RecordingFormat m_currentFormat;
    eti::SegmentationConfig m_currentConfig;

    // Constants for configuration
    static constexpr int MIN_SEGMENT_DURATION_MINUTES = 1;
    static constexpr int MAX_SEGMENT_DURATION_MINUTES = 1440; // 24 hours
    static constexpr int DEFAULT_SEGMENT_DURATION_MINUTES = 60;
    static constexpr int MIN_MAX_SEGMENT_SIZE_MB = 1;
    static constexpr int MAX_MAX_SEGMENT_SIZE_MB = 10240; // 10 GB
    static constexpr int DEFAULT_MAX_SEGMENT_SIZE_MB = 1024; // 1 GB
    static constexpr int MIN_RETENTION_HOURS = 1;
    static constexpr int MAX_RETENTION_HOURS = 8760; // 1 year
    static constexpr int DEFAULT_RETENTION_HOURS = 168; // 1 week
    static constexpr int MIN_COMPRESSION_LEVEL = 1;
    static constexpr int MAX_COMPRESSION_LEVEL = 9;
    static constexpr int DEFAULT_COMPRESSION_LEVEL = 6;
    static constexpr double MIN_DISK_USAGE_PERCENT = 50.0;
    static constexpr double MAX_DISK_USAGE_PERCENT = 95.0;
    static constexpr double DEFAULT_DISK_USAGE_PERCENT = 85.0;

    static constexpr int STATUS_UPDATE_INTERVAL_MS = 1000; // 1 second
    static constexpr int DISK_SPACE_UPDATE_INTERVAL_MS = 5000; // 5 seconds
};