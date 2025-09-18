/**
 * @file stream_recording_dialog.cpp
 * @brief Professional Stream Recording Configuration Dialog Implementation
 * 
 * Implements comprehensive GUI for stream recording configuration with
 * real-time monitoring, professional workflows, and broadcast-standard controls.
 * 
 * @author UI/UX Agent - Phase 4 Integration
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#include "stream_recording_dialog.h"
#include "utils/logger_compat.h"

#include <QApplication>
#include <QScreen>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QSplitter>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QFrame>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QDir>
#include <QDebug>

StreamRecordingDialog::StreamRecordingDialog(eti::StreamRecordingManager *recordingManager, QWidget *parent)
    : QDialog(parent)
    , m_recordingManager(recordingManager)
    , m_mainLayout(nullptr)
    , m_tabWidget(nullptr)
    , m_recordingTab(nullptr)
    , m_outputGroup(nullptr)
    , m_formatGroup(nullptr)
    , m_outputDirectoryEdit(nullptr)
    , m_browseDirectoryButton(nullptr)
    , m_diskSpaceLabel(nullptr)
    , m_diskUsageBar(nullptr)
    , m_outputFormatCombo(nullptr)
    , m_formatDescriptionLabel(nullptr)
    , m_enableCompressionCheck(nullptr)
    , m_compressionLevelSpin(nullptr)
    , m_segmentationTab(nullptr)
    , m_segmentGroup(nullptr)
    , m_rotationGroup(nullptr)
    , m_cleanupGroup(nullptr)
    , m_segmentDurationSpin(nullptr)
    , m_segmentDurationUnit(nullptr)
    , m_maxSegmentSizeSpin(nullptr)
    , m_maxArchiveSegmentsSpin(nullptr)
    , m_filenamePatternEdit(nullptr)
    , m_segmentPreviewLabel(nullptr)
    , m_enableDailyRotationCheck(nullptr)
    , m_dailyRotationHourSpin(nullptr)
    , m_enableAutoArchiveCheck(nullptr)
    , m_enableAutoCleanupCheck(nullptr)
    , m_retentionPeriodSpin(nullptr)
    , m_retentionPeriodUnit(nullptr)
    , m_maxDiskUsageSpin(nullptr)
    , m_formatTab(nullptr)
    , m_qualityGroup(nullptr)
    , m_metadataGroup(nullptr)
    , m_preserveTimestampsCheck(nullptr)
    , m_enableIntegrityCheckingCheck(nullptr)
    , m_qualitySlider(nullptr)
    , m_qualityLabel(nullptr)
    , m_embedMetadataCheck(nullptr)
    , m_embedEnsembleInfoCheck(nullptr)
    , m_embedServiceInfoCheck(nullptr)
    , m_customMetadataEdit(nullptr)
    , m_monitoringTab(nullptr)
    , m_monitoringSplitter(nullptr)
    , m_sessionsGroup(nullptr)
    , m_statisticsGroup(nullptr)
    , m_sessionsTable(nullptr)
    , m_refreshSessionsButton(nullptr)
    , m_clearSessionsButton(nullptr)
    , m_currentSessionLabel(nullptr)
    , m_recordingProgressBar(nullptr)
    , m_framesRecordedLabel(nullptr)
    , m_bytesWrittenLabel(nullptr)
    , m_segmentsCreatedLabel(nullptr)
    , m_errorsEncounteredLabel(nullptr)
    , m_averageFrameRateLabel(nullptr)
    , m_currentLatencyLabel(nullptr)
    , m_recordingControlsLayout(nullptr)
    , m_startRecordingButton(nullptr)
    , m_stopRecordingButton(nullptr)
    , m_pauseRecordingButton(nullptr)
    , m_testSettingsButton(nullptr)
    , m_buttonLayout(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_applyButton(nullptr)
    , m_resetButton(nullptr)
    , m_loadButton(nullptr)
    , m_saveButton(nullptr)
    , m_statusUpdateTimer(nullptr)
    , m_diskSpaceUpdateTimer(nullptr)
    , m_directoryWatcher(nullptr)
    , m_initialized(false)
    , m_configurationChanged(false)
    , m_currentFormat(eti::RecordingFormat::ETI_NI)
{
    setWindowTitle(tr("Stream Recording Configuration - ETI Stream Analyzer"));
    setWindowIcon(QIcon(":/icons/record_config.png"));
    setModal(false); // Allow interaction with main window
    resize(900, 700);
    
    // Center dialog on screen
    if (parent) {
        QPoint parentCenter = parent->frameGeometry().center();
        move(parentCenter - frameGeometry().center());
    } else {
        QScreen *screen = QApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.center() - frameGeometry().center());
    }

    // Initialize default configuration
    m_currentConfig.segment_duration = std::chrono::minutes(DEFAULT_SEGMENT_DURATION_MINUTES);
    m_currentConfig.enable_compression = false;
    m_currentConfig.preserve_timestamps = true;
    m_currentConfig.max_segment_size_mb = DEFAULT_MAX_SEGMENT_SIZE_MB;
    m_currentConfig.retention_period = std::chrono::hours(DEFAULT_RETENTION_HOURS);
    m_currentConfig.max_disk_usage_percent = DEFAULT_DISK_USAGE_PERCENT;
    m_currentConfig.filename_pattern = "eti_%Y%m%d_%H%M%S";
}

StreamRecordingDialog::~StreamRecordingDialog()
{
    if (m_statusUpdateTimer) {
        m_statusUpdateTimer->stop();
    }
    if (m_diskSpaceUpdateTimer) {
        m_diskSpaceUpdateTimer->stop();
    }
}

bool StreamRecordingDialog::initialize()
{
    if (!m_recordingManager) {
        qWarning() << "StreamRecordingDialog: No recording manager provided";
        return false;
    }

    if (m_initialized) {
        return true;
    }

    try {
        createUI();
        setupRecordingManagerConnections();
        loadCurrentConfiguration();
        applyBroadcastStyling();
        
        // Setup update timers
        m_statusUpdateTimer = new QTimer(this);
        m_statusUpdateTimer->setInterval(STATUS_UPDATE_INTERVAL_MS);
        connect(m_statusUpdateTimer, &QTimer::timeout, this, &StreamRecordingDialog::updateRecordingStatus);
        m_statusUpdateTimer->start();
        
        m_diskSpaceUpdateTimer = new QTimer(this);
        m_diskSpaceUpdateTimer->setInterval(DISK_SPACE_UPDATE_INTERVAL_MS);
        connect(m_diskSpaceUpdateTimer, &QTimer::timeout, this, &StreamRecordingDialog::updateDiskSpaceDisplay);
        m_diskSpaceUpdateTimer->start();

        // Setup directory watcher
        m_directoryWatcher = new QFileSystemWatcher(this);
        connect(m_directoryWatcher, &QFileSystemWatcher::directoryChanged,
                this, &StreamRecordingDialog::updateDiskSpaceDisplay);
        
        m_initialized = true;
        qInfo() << "StreamRecordingDialog initialized successfully";
        return true;
    }
    catch (const std::exception& e) {
        qCritical() << "StreamRecordingDialog initialization failed:" << e.what();
        return false;
    }
}

void StreamRecordingDialog::createUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(8);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);

    // Create main tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setUsesScrollButtons(true);
    m_tabWidget->setElideMode(Qt::ElideRight);

    // Create tabs
    createRecordingTab();
    createSegmentationTab();
    createFormatTab();
    createMonitoringTab();

    // Add tabs to widget
    m_tabWidget->addTab(m_recordingTab, QIcon(":/icons/recording.png"), tr("&Recording"));
    m_tabWidget->addTab(m_segmentationTab, QIcon(":/icons/segmentation.png"), tr("&Segmentation"));
    m_tabWidget->addTab(m_formatTab, QIcon(":/icons/format.png"), tr("&Format"));
    m_tabWidget->addTab(m_monitoringTab, QIcon(":/icons/monitoring.png"), tr("&Monitoring"));

    m_mainLayout->addWidget(m_tabWidget);

    // Create recording controls
    m_recordingControlsLayout = new QHBoxLayout();
    
    m_startRecordingButton = new QPushButton(tr("&Start Recording"));
    m_startRecordingButton->setIcon(QIcon(":/icons/record_start.png"));
    m_startRecordingButton->setMinimumHeight(32);
    m_recordingControlsLayout->addWidget(m_startRecordingButton);

    m_stopRecordingButton = new QPushButton(tr("S&top Recording"));
    m_stopRecordingButton->setIcon(QIcon(":/icons/record_stop.png"));
    m_stopRecordingButton->setMinimumHeight(32);
    m_stopRecordingButton->setEnabled(false);
    m_recordingControlsLayout->addWidget(m_stopRecordingButton);

    m_pauseRecordingButton = new QPushButton(tr("&Pause Recording"));
    m_pauseRecordingButton->setIcon(QIcon(":/icons/record_pause.png"));
    m_pauseRecordingButton->setMinimumHeight(32);
    m_pauseRecordingButton->setEnabled(false);
    m_recordingControlsLayout->addWidget(m_pauseRecordingButton);

    m_recordingControlsLayout->addStretch();

    m_testSettingsButton = new QPushButton(tr("&Test Settings"));
    m_testSettingsButton->setIcon(QIcon(":/icons/test.png"));
    m_testSettingsButton->setMinimumHeight(32);
    m_recordingControlsLayout->addWidget(m_testSettingsButton);

    m_mainLayout->addLayout(m_recordingControlsLayout);

    // Create button box
    createButtonBox();
    m_mainLayout->addLayout(m_buttonLayout);

    // Connect recording control signals
    connect(m_startRecordingButton, &QPushButton::clicked,
            this, &StreamRecordingDialog::startRecordingSession);
    connect(m_stopRecordingButton, &QPushButton::clicked,
            this, &StreamRecordingDialog::stopRecordingSession);
    connect(m_pauseRecordingButton, &QPushButton::clicked,
            this, &StreamRecordingDialog::pauseRecordingSession);
    connect(m_testSettingsButton, &QPushButton::clicked,
            this, &StreamRecordingDialog::testRecordingSettings);
}

void StreamRecordingDialog::createRecordingTab()
{
    m_recordingTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_recordingTab);
    layout->setSpacing(12);
    layout->setContentsMargins(12, 12, 12, 12);

    // Output Directory Group
    m_outputGroup = new QGroupBox(tr("Output Directory"));
    QGridLayout *outputLayout = new QGridLayout(m_outputGroup);

    outputLayout->addWidget(new QLabel(tr("Directory:")), 0, 0);
    m_outputDirectoryEdit = new QLineEdit();
    m_outputDirectoryEdit->setPlaceholderText(tr("Select output directory for recordings"));
    outputLayout->addWidget(m_outputDirectoryEdit, 0, 1);

    m_browseDirectoryButton = new QPushButton(tr("Browse..."));
    outputLayout->addWidget(m_browseDirectoryButton, 0, 2);

    m_diskSpaceLabel = new QLabel(tr("Available: Calculating..."));
    outputLayout->addWidget(m_diskSpaceLabel, 1, 0, 1, 2);

    m_diskUsageBar = new QProgressBar();
    m_diskUsageBar->setRange(0, 100);
    m_diskUsageBar->setValue(0);
    m_diskUsageBar->setFormat(tr("Disk Usage: %p%"));
    outputLayout->addWidget(m_diskUsageBar, 1, 2);

    layout->addWidget(m_outputGroup);

    // Format Group
    m_formatGroup = new QGroupBox(tr("Recording Format"));
    QVBoxLayout *formatLayout = new QVBoxLayout(m_formatGroup);

    QHBoxLayout *formatSelectLayout = new QHBoxLayout();
    formatSelectLayout->addWidget(new QLabel(tr("Format:")));
    
    m_outputFormatCombo = new QComboBox();
    m_outputFormatCombo->addItem(tr("ETI-NI (Native Interface)"), static_cast<int>(eti::RecordingFormat::ETI_NI));
    m_outputFormatCombo->addItem(tr("ETI-LI (Linear Interface)"), static_cast<int>(eti::RecordingFormat::ETI_LI));
    m_outputFormatCombo->addItem(tr("EDI (Encapsulated DAB Interface)"), static_cast<int>(eti::RecordingFormat::EDI));
    m_outputFormatCombo->addItem(tr("Raw Binary"), static_cast<int>(eti::RecordingFormat::RAW_BINARY));
    m_outputFormatCombo->addItem(tr("Compressed ETI"), static_cast<int>(eti::RecordingFormat::COMPRESSED_ETI));
    m_outputFormatCombo->addItem(tr("Compressed Archive"), static_cast<int>(eti::RecordingFormat::COMPRESSED_ARCHIVE));
    formatSelectLayout->addWidget(m_outputFormatCombo);
    formatSelectLayout->addStretch();
    formatLayout->addLayout(formatSelectLayout);

    m_formatDescriptionLabel = new QLabel();
    m_formatDescriptionLabel->setWordWrap(true);
    m_formatDescriptionLabel->setStyleSheet("QLabel { padding: 8px; background-color: #f0f0f0; border: 1px solid #cccccc; border-radius: 4px; }");
    formatLayout->addWidget(m_formatDescriptionLabel);

    // Compression options
    QHBoxLayout *compressionLayout = new QHBoxLayout();
    m_enableCompressionCheck = new QCheckBox(tr("Enable compression"));
    compressionLayout->addWidget(m_enableCompressionCheck);

    compressionLayout->addWidget(new QLabel(tr("Level:")));
    m_compressionLevelSpin = new QSpinBox();
    m_compressionLevelSpin->setRange(MIN_COMPRESSION_LEVEL, MAX_COMPRESSION_LEVEL);
    m_compressionLevelSpin->setValue(DEFAULT_COMPRESSION_LEVEL);
    m_compressionLevelSpin->setEnabled(false);
    compressionLayout->addWidget(m_compressionLevelSpin);
    compressionLayout->addStretch();
    formatLayout->addLayout(compressionLayout);

    layout->addWidget(m_formatGroup);

    layout->addStretch();

    // Connect signals
    connect(m_browseDirectoryButton, &QPushButton::clicked,
            this, &StreamRecordingDialog::browseOutputDirectory);
    connect(m_outputDirectoryEdit, &QLineEdit::textChanged,
            this, &StreamRecordingDialog::handleOutputDirectoryChanged);
    connect(m_outputFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StreamRecordingDialog::handleOutputFormatChanged);
    connect(m_enableCompressionCheck, &QCheckBox::toggled,
            this, &StreamRecordingDialog::handleCompressionChanged);
    connect(m_compressionLevelSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &StreamRecordingDialog::handleCompressionChanged);
}

void StreamRecordingDialog::createSegmentationTab()
{
    m_segmentationTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_segmentationTab);
    layout->setSpacing(12);
    layout->setContentsMargins(12, 12, 12, 12);

    // Segmentation Group
    m_segmentGroup = new QGroupBox(tr("Time-Based Segmentation"));
    QGridLayout *segmentLayout = new QGridLayout(m_segmentGroup);

    segmentLayout->addWidget(new QLabel(tr("Segment duration:")), 0, 0);
    m_segmentDurationSpin = new QSpinBox();
    m_segmentDurationSpin->setRange(MIN_SEGMENT_DURATION_MINUTES, MAX_SEGMENT_DURATION_MINUTES);
    m_segmentDurationSpin->setValue(DEFAULT_SEGMENT_DURATION_MINUTES);
    segmentLayout->addWidget(m_segmentDurationSpin, 0, 1);

    m_segmentDurationUnit = new QComboBox();
    m_segmentDurationUnit->addItems({tr("Minutes"), tr("Hours")});
    segmentLayout->addWidget(m_segmentDurationUnit, 0, 2);

    segmentLayout->addWidget(new QLabel(tr("Max segment size:")), 1, 0);
    m_maxSegmentSizeSpin = new QSpinBox();
    m_maxSegmentSizeSpin->setRange(MIN_MAX_SEGMENT_SIZE_MB, MAX_MAX_SEGMENT_SIZE_MB);
    m_maxSegmentSizeSpin->setValue(DEFAULT_MAX_SEGMENT_SIZE_MB);
    m_maxSegmentSizeSpin->setSuffix(" MB");
    segmentLayout->addWidget(m_maxSegmentSizeSpin, 1, 1);

    segmentLayout->addWidget(new QLabel(tr("Max archive segments:")), 2, 0);
    m_maxArchiveSegmentsSpin = new QSpinBox();
    m_maxArchiveSegmentsSpin->setRange(1, 1000);
    m_maxArchiveSegmentsSpin->setValue(100);
    segmentLayout->addWidget(m_maxArchiveSegmentsSpin, 2, 1);

    segmentLayout->addWidget(new QLabel(tr("Filename pattern:")), 3, 0);
    m_filenamePatternEdit = new QLineEdit();
    m_filenamePatternEdit->setText("eti_%Y%m%d_%H%M%S");
    m_filenamePatternEdit->setToolTip(tr("strftime format: %Y=year, %m=month, %d=day, %H=hour, %M=minute, %S=second"));
    segmentLayout->addWidget(m_filenamePatternEdit, 3, 1, 1, 2);

    segmentLayout->addWidget(new QLabel(tr("Preview:")), 4, 0);
    m_segmentPreviewLabel = new QLabel();
    m_segmentPreviewLabel->setStyleSheet("QLabel { padding: 4px; background-color: #f8f8f8; border: 1px solid #cccccc; }");
    segmentLayout->addWidget(m_segmentPreviewLabel, 4, 1, 1, 2);

    layout->addWidget(m_segmentGroup);

    // Daily Rotation Group
    m_rotationGroup = new QGroupBox(tr("Daily Rotation"));
    QVBoxLayout *rotationLayout = new QVBoxLayout(m_rotationGroup);

    m_enableDailyRotationCheck = new QCheckBox(tr("Enable daily rotation"));
    rotationLayout->addWidget(m_enableDailyRotationCheck);

    QHBoxLayout *rotationTimeLayout = new QHBoxLayout();
    rotationTimeLayout->addWidget(new QLabel(tr("Rotation hour:")));
    m_dailyRotationHourSpin = new QSpinBox();
    m_dailyRotationHourSpin->setRange(0, 23);
    m_dailyRotationHourSpin->setValue(0);
    m_dailyRotationHourSpin->setSuffix(" hour");
    m_dailyRotationHourSpin->setEnabled(false);
    rotationTimeLayout->addWidget(m_dailyRotationHourSpin);
    rotationTimeLayout->addStretch();
    rotationLayout->addLayout(rotationTimeLayout);

    m_enableAutoArchiveCheck = new QCheckBox(tr("Auto-archive completed segments"));
    rotationLayout->addWidget(m_enableAutoArchiveCheck);

    layout->addWidget(m_rotationGroup);

    // Cleanup Group
    m_cleanupGroup = new QGroupBox(tr("Automatic Cleanup"));
    QGridLayout *cleanupLayout = new QGridLayout(m_cleanupGroup);

    m_enableAutoCleanupCheck = new QCheckBox(tr("Enable automatic cleanup"));
    m_enableAutoCleanupCheck->setChecked(true);
    cleanupLayout->addWidget(m_enableAutoCleanupCheck, 0, 0, 1, 3);

    cleanupLayout->addWidget(new QLabel(tr("Retention period:")), 1, 0);
    m_retentionPeriodSpin = new QSpinBox();
    m_retentionPeriodSpin->setRange(MIN_RETENTION_HOURS, MAX_RETENTION_HOURS);
    m_retentionPeriodSpin->setValue(DEFAULT_RETENTION_HOURS);
    cleanupLayout->addWidget(m_retentionPeriodSpin, 1, 1);

    m_retentionPeriodUnit = new QComboBox();
    m_retentionPeriodUnit->addItems({tr("Hours"), tr("Days"), tr("Weeks")});
    m_retentionPeriodUnit->setCurrentIndex(2); // Weeks
    cleanupLayout->addWidget(m_retentionPeriodUnit, 1, 2);

    cleanupLayout->addWidget(new QLabel(tr("Max disk usage:")), 2, 0);
    m_maxDiskUsageSpin = new QDoubleSpinBox();
    m_maxDiskUsageSpin->setRange(MIN_DISK_USAGE_PERCENT, MAX_DISK_USAGE_PERCENT);
    m_maxDiskUsageSpin->setValue(DEFAULT_DISK_USAGE_PERCENT);
    m_maxDiskUsageSpin->setSuffix(" %");
    m_maxDiskUsageSpin->setDecimals(1);
    cleanupLayout->addWidget(m_maxDiskUsageSpin, 2, 1);

    layout->addWidget(m_cleanupGroup);

    layout->addStretch();

    // Connect signals
    connect(m_segmentDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &StreamRecordingDialog::handleSegmentationChanged);
    connect(m_segmentDurationUnit, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StreamRecordingDialog::handleSegmentationChanged);
    connect(m_filenamePatternEdit, &QLineEdit::textChanged,
            this, &StreamRecordingDialog::handleSegmentationChanged);
    connect(m_enableDailyRotationCheck, &QCheckBox::toggled,
            m_dailyRotationHourSpin, &QSpinBox::setEnabled);
}

void StreamRecordingDialog::createFormatTab()
{
    m_formatTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_formatTab);
    layout->setSpacing(12);
    layout->setContentsMargins(12, 12, 12, 12);

    // Quality Group
    m_qualityGroup = new QGroupBox(tr("Recording Quality"));
    QVBoxLayout *qualityLayout = new QVBoxLayout(m_qualityGroup);

    m_preserveTimestampsCheck = new QCheckBox(tr("Preserve original timestamps"));
    m_preserveTimestampsCheck->setChecked(true);
    qualityLayout->addWidget(m_preserveTimestampsCheck);

    m_enableIntegrityCheckingCheck = new QCheckBox(tr("Enable integrity checking"));
    m_enableIntegrityCheckingCheck->setChecked(true);
    qualityLayout->addWidget(m_enableIntegrityCheckingCheck);

    QHBoxLayout *qualitySliderLayout = new QHBoxLayout();
    qualitySliderLayout->addWidget(new QLabel(tr("Quality:")));
    
    m_qualitySlider = new QSlider(Qt::Horizontal);
    m_qualitySlider->setRange(1, 10);
    m_qualitySlider->setValue(8);
    m_qualitySlider->setTickPosition(QSlider::TicksBelow);
    m_qualitySlider->setTickInterval(1);
    qualitySliderLayout->addWidget(m_qualitySlider);

    m_qualityLabel = new QLabel(tr("High"));
    m_qualityLabel->setMinimumWidth(60);
    qualitySliderLayout->addWidget(m_qualityLabel);
    qualityLayout->addLayout(qualitySliderLayout);

    layout->addWidget(m_qualityGroup);

    // Metadata Group
    m_metadataGroup = new QGroupBox(tr("Metadata Embedding"));
    QVBoxLayout *metadataLayout = new QVBoxLayout(m_metadataGroup);

    m_embedMetadataCheck = new QCheckBox(tr("Embed stream metadata"));
    m_embedMetadataCheck->setChecked(true);
    metadataLayout->addWidget(m_embedMetadataCheck);

    m_embedEnsembleInfoCheck = new QCheckBox(tr("Embed ensemble information"));
    m_embedEnsembleInfoCheck->setChecked(true);
    metadataLayout->addWidget(m_embedEnsembleInfoCheck);

    m_embedServiceInfoCheck = new QCheckBox(tr("Embed service information"));
    m_embedServiceInfoCheck->setChecked(true);
    metadataLayout->addWidget(m_embedServiceInfoCheck);

    metadataLayout->addWidget(new QLabel(tr("Custom metadata:")));
    m_customMetadataEdit = new QTextEdit();
    m_customMetadataEdit->setMaximumHeight(80);
    m_customMetadataEdit->setPlaceholderText(tr("Enter custom metadata (JSON format)"));
    metadataLayout->addWidget(m_customMetadataEdit);

    layout->addWidget(m_metadataGroup);

    layout->addStretch();

    // Connect quality slider
    connect(m_qualitySlider, &QSlider::valueChanged, this, [this](int value) {
        QString qualityText;
        if (value <= 3) qualityText = tr("Low");
        else if (value <= 6) qualityText = tr("Medium");
        else if (value <= 8) qualityText = tr("High");
        else qualityText = tr("Maximum");
        m_qualityLabel->setText(qualityText);
    });
}

void StreamRecordingDialog::createMonitoringTab()
{
    m_monitoringTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_monitoringTab);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    // Create splitter for sessions and statistics
    m_monitoringSplitter = new QSplitter(Qt::Vertical);

    // Sessions Group
    m_sessionsGroup = new QGroupBox(tr("Recording Sessions"));
    QVBoxLayout *sessionsLayout = new QVBoxLayout(m_sessionsGroup);

    // Sessions table
    m_sessionsTable = new QTableWidget();
    m_sessionsTable->setColumnCount(7);
    m_sessionsTable->setHorizontalHeaderLabels({
        tr("Session ID"), tr("Status"), tr("Start Time"), tr("Duration"), 
        tr("Frames"), tr("Size"), tr("Format")
    });
    m_sessionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sessionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sessionsTable->setAlternatingRowColors(true);
    m_sessionsTable->horizontalHeader()->setStretchLastSection(true);
    m_sessionsTable->verticalHeader()->setVisible(false);
    m_sessionsTable->setSortingEnabled(true);
    sessionsLayout->addWidget(m_sessionsTable);

    // Sessions buttons
    QHBoxLayout *sessionsButtonLayout = new QHBoxLayout();
    m_refreshSessionsButton = new QPushButton(tr("Refresh"));
    m_refreshSessionsButton->setIcon(QIcon(":/icons/refresh.png"));
    sessionsButtonLayout->addWidget(m_refreshSessionsButton);

    m_clearSessionsButton = new QPushButton(tr("Clear Completed"));
    m_clearSessionsButton->setIcon(QIcon(":/icons/clear.png"));
    sessionsButtonLayout->addWidget(m_clearSessionsButton);

    sessionsButtonLayout->addStretch();
    sessionsLayout->addLayout(sessionsButtonLayout);

    m_monitoringSplitter->addWidget(m_sessionsGroup);

    // Statistics Group
    m_statisticsGroup = new QGroupBox(tr("Current Session Statistics"));
    QGridLayout *statsLayout = new QGridLayout(m_statisticsGroup);

    m_currentSessionLabel = new QLabel(tr("No active session"));
    m_currentSessionLabel->setStyleSheet("QLabel { font-weight: bold; }");
    statsLayout->addWidget(m_currentSessionLabel, 0, 0, 1, 2);

    m_recordingProgressBar = new QProgressBar();
    m_recordingProgressBar->setRange(0, 100);
    m_recordingProgressBar->setValue(0);
    m_recordingProgressBar->setFormat(tr("Recording Progress: %p%"));
    statsLayout->addWidget(m_recordingProgressBar, 1, 0, 1, 2);

    statsLayout->addWidget(new QLabel(tr("Frames Recorded:")), 2, 0);
    m_framesRecordedLabel = new QLabel("0");
    statsLayout->addWidget(m_framesRecordedLabel, 2, 1);

    statsLayout->addWidget(new QLabel(tr("Bytes Written:")), 3, 0);
    m_bytesWrittenLabel = new QLabel("0 bytes");
    statsLayout->addWidget(m_bytesWrittenLabel, 3, 1);

    statsLayout->addWidget(new QLabel(tr("Segments Created:")), 4, 0);
    m_segmentsCreatedLabel = new QLabel("0");
    statsLayout->addWidget(m_segmentsCreatedLabel, 4, 1);

    statsLayout->addWidget(new QLabel(tr("Errors Encountered:")), 5, 0);
    m_errorsEncounteredLabel = new QLabel("0");
    statsLayout->addWidget(m_errorsEncounteredLabel, 5, 1);

    statsLayout->addWidget(new QLabel(tr("Average Frame Rate:")), 6, 0);
    m_averageFrameRateLabel = new QLabel("0.0 fps");
    statsLayout->addWidget(m_averageFrameRateLabel, 6, 1);

    statsLayout->addWidget(new QLabel(tr("Current Latency:")), 7, 0);
    m_currentLatencyLabel = new QLabel("0 ms");
    statsLayout->addWidget(m_currentLatencyLabel, 7, 1);

    m_monitoringSplitter->addWidget(m_statisticsGroup);
    m_monitoringSplitter->setSizes({400, 300});

    layout->addWidget(m_monitoringSplitter);

    // Connect monitoring signals
    connect(m_refreshSessionsButton, &QPushButton::clicked,
            this, &StreamRecordingDialog::updateSessionList);
    connect(m_clearSessionsButton, &QPushButton::clicked, this, [this]() {
        // TODO: Implement clear completed sessions
        QMessageBox::information(this, tr("Clear Sessions"), 
                               tr("Clear completed sessions functionality will be implemented."));
    });
    connect(m_sessionsTable, &QTableWidget::itemSelectionChanged,
            this, &StreamRecordingDialog::handleSessionSelectionChanged);
}

void StreamRecordingDialog::createButtonBox()
{
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(8);

    // Load/Save buttons
    m_loadButton = new QPushButton(tr("&Load"));
    m_loadButton->setIcon(QIcon(":/icons/load.png"));
    m_loadButton->setToolTip(tr("Load configuration from file"));
    m_buttonLayout->addWidget(m_loadButton);

    m_saveButton = new QPushButton(tr("&Save"));
    m_saveButton->setIcon(QIcon(":/icons/save.png"));
    m_saveButton->setToolTip(tr("Save configuration to file"));
    m_buttonLayout->addWidget(m_saveButton);

    // Spacer
    m_buttonLayout->addStretch();

    // Reset button
    m_resetButton = new QPushButton(tr("&Reset"));
    m_resetButton->setIcon(QIcon(":/icons/reset.png"));
    m_resetButton->setToolTip(tr("Reset to default values"));
    m_buttonLayout->addWidget(m_resetButton);

    // Standard buttons
    m_okButton = new QPushButton(tr("&OK"));
    m_okButton->setIcon(QIcon(":/icons/ok.png"));
    m_okButton->setDefault(true);
    m_buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton(tr("&Cancel"));
    m_cancelButton->setIcon(QIcon(":/icons/cancel.png"));
    m_buttonLayout->addWidget(m_cancelButton);

    m_applyButton = new QPushButton(tr("&Apply"));
    m_applyButton->setIcon(QIcon(":/icons/apply.png"));
    m_applyButton->setToolTip(tr("Apply changes without closing dialog"));
    m_buttonLayout->addWidget(m_applyButton);

    // Connect button signals
    connect(m_okButton, &QPushButton::clicked, this, &StreamRecordingDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &StreamRecordingDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &StreamRecordingDialog::applyConfiguration);
    connect(m_resetButton, &QPushButton::clicked, this, &StreamRecordingDialog::resetToDefaults);
    connect(m_loadButton, &QPushButton::clicked, this, &StreamRecordingDialog::loadConfiguration);
    connect(m_saveButton, &QPushButton::clicked, this, &StreamRecordingDialog::saveConfiguration);
}

void StreamRecordingDialog::setupRecordingManagerConnections()
{
    if (!m_recordingManager) return;

    // Connect recording manager signals
    connect(m_recordingManager, &eti::StreamRecordingManager::recording_session_started,
            this, &StreamRecordingDialog::handleRecordingSessionStarted);
    connect(m_recordingManager, &eti::StreamRecordingManager::recording_session_stopped,
            this, &StreamRecordingDialog::handleRecordingSessionStopped);
    connect(m_recordingManager, &eti::StreamRecordingManager::session_statistics_updated,
            this, &StreamRecordingDialog::handleSessionStatisticsUpdate);
    connect(m_recordingManager, &eti::StreamRecordingManager::recording_error_occurred,
            this, &StreamRecordingDialog::handleRecordingError);
}

void StreamRecordingDialog::loadCurrentConfiguration()
{
    if (!m_recordingManager) return;

    // Load default configuration
    m_currentConfig = m_recordingManager->get_default_segmentation_config();

    // Set output directory to default documents location
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ETI_Recordings";
    m_outputDirectoryEdit->setText(defaultDir);

    // Set initial format
    m_currentFormat = eti::RecordingFormat::ETI_NI;
    m_outputFormatCombo->setCurrentIndex(0);

    // Update UI with current configuration
    updateFormatDescription();
    updateSegmentationPreview();
    updateDiskSpaceDisplay();

    m_configurationChanged = false;
}

void StreamRecordingDialog::applyConfiguration()
{
    if (!validateConfiguration()) {
        return;
    }

    applyUISettings();
    m_configurationChanged = false;
    emit configurationChanged();

    QMessageBox::information(this, tr("Configuration Applied"), 
                           tr("Recording configuration has been successfully applied."));
}

void StreamRecordingDialog::applyUISettings()
{
    if (!m_recordingManager) return;

    // Update segmentation configuration
    int duration = m_segmentDurationSpin->value();
    if (m_segmentDurationUnit->currentIndex() == 1) { // Hours
        duration *= 60; // Convert to minutes
    }
    m_currentConfig.segment_duration = std::chrono::minutes(duration);

    m_currentConfig.max_segment_size_mb = m_maxSegmentSizeSpin->value();
    m_currentConfig.max_archive_segments = m_maxArchiveSegmentsSpin->value();
    m_currentConfig.filename_pattern = m_filenamePatternEdit->text();

    m_currentConfig.enable_compression = m_enableCompressionCheck->isChecked();
    m_currentConfig.preserve_timestamps = m_preserveTimestampsCheck->isChecked();

    // Update retention settings
    int retentionValue = m_retentionPeriodSpin->value();
    switch (m_retentionPeriodUnit->currentIndex()) {
        case 0: // Hours
            m_currentConfig.retention_period = std::chrono::hours(retentionValue);
            break;
        case 1: // Days
            m_currentConfig.retention_period = std::chrono::hours(retentionValue * 24);
            break;
        case 2: // Weeks
            m_currentConfig.retention_period = std::chrono::hours(retentionValue * 24 * 7);
            break;
    }

    m_currentConfig.auto_cleanup = m_enableAutoCleanupCheck->isChecked();
    m_currentConfig.max_disk_usage_percent = m_maxDiskUsageSpin->value();

    // Apply to recording manager
    m_recordingManager->set_default_segmentation_config(m_currentConfig);
}

bool StreamRecordingDialog::validateConfiguration() const
{
    // Validate output directory
    QString outputDir = m_outputDirectoryEdit->text();
    if (outputDir.isEmpty()) {
        QMessageBox::warning(const_cast<StreamRecordingDialog*>(this), 
                           tr("Invalid Configuration"), 
                           tr("Output directory cannot be empty."));
        return false;
    }

    QDir dir(outputDir);
    if (!dir.exists()) {
        int ret = QMessageBox::question(const_cast<StreamRecordingDialog*>(this), 
                                      tr("Create Directory"), 
                                      tr("Output directory does not exist. Create it?"));
        if (ret == QMessageBox::Yes) {
            if (!dir.mkpath(outputDir)) {
                QMessageBox::critical(const_cast<StreamRecordingDialog*>(this), 
                                    tr("Directory Creation Failed"), 
                                    tr("Failed to create output directory."));
                return false;
            }
        } else {
            return false;
        }
    }

    // Validate filename pattern
    QString pattern = m_filenamePatternEdit->text();
    if (pattern.isEmpty()) {
        QMessageBox::warning(const_cast<StreamRecordingDialog*>(this), 
                           tr("Invalid Configuration"), 
                           tr("Filename pattern cannot be empty."));
        return false;
    }

    return true;
}

void StreamRecordingDialog::applyBroadcastStyling()
{
    // Apply professional broadcast styling
    setStyleSheet(R"(
        QDialog {
            background-color: #2b2b2b;
            color: #ffffff;
        }
        
        QTabWidget::pane {
            border: 1px solid #555555;
            background-color: #3a3a3a;
        }
        
        QTabBar::tab {
            background-color: #404040;
            color: #ffffff;
            padding: 8px 16px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        
        QTabBar::tab:selected {
            background-color: #3a3a3a;
            border-bottom: 2px solid #0078d4;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 2px solid #555555;
            border-radius: 5px;
            margin-top: 1ex;
            padding-top: 8px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #404040;
            border: 1px solid #666666;
            border-radius: 3px;
            padding: 4px;
            selection-background-color: #0078d4;
        }
        
        QTextEdit {
            background-color: #404040;
            border: 1px solid #666666;
            border-radius: 3px;
        }
        
        QTableWidget {
            background-color: #404040;
            alternate-background-color: #4a4a4a;
            border: 1px solid #666666;
            border-radius: 3px;
            gridline-color: #555555;
        }
        
        QTableWidget::item {
            border: none;
            padding: 4px;
        }
        
        QHeaderView::section {
            background-color: #505050;
            border: 1px solid #666666;
            padding: 4px;
        }
        
        QPushButton {
            background-color: #505050;
            border: 1px solid #666666;
            border-radius: 4px;
            padding: 6px 12px;
            min-width: 70px;
        }
        
        QPushButton:hover {
            background-color: #606060;
        }
        
        QPushButton:pressed {
            background-color: #404040;
        }
        
        QPushButton:default {
            border: 2px solid #0078d4;
        }
        
        QProgressBar {
            border: 1px solid #666666;
            border-radius: 3px;
            text-align: center;
        }
        
        QProgressBar::chunk {
            background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0,
                                            stop: 0 #00aa00, stop: 0.7 #aaaa00, stop: 1.0 #aa0000);
            border-radius: 2px;
        }
        
        QSlider::groove:horizontal {
            border: 1px solid #666666;
            height: 8px;
            background: #404040;
            border-radius: 4px;
        }
        
        QSlider::handle:horizontal {
            background: #0078d4;
            border: 1px solid #0056a0;
            width: 18px;
            margin: -2px 0;
            border-radius: 3px;
        }
    )");
}

// Slot implementations
void StreamRecordingDialog::startRecordingSession()
{
    if (!validateConfiguration()) {
        return;
    }

    applyUISettings();

    QString outputDir = m_outputDirectoryEdit->text();
    m_currentSessionId = m_recordingManager->start_recording_session(outputDir, m_currentFormat, m_currentConfig);
    
    if (!m_currentSessionId.isEmpty()) {
        m_startRecordingButton->setEnabled(false);
        m_stopRecordingButton->setEnabled(true);
        m_pauseRecordingButton->setEnabled(true);
        
        qInfo() << "Recording session started:" << m_currentSessionId;
    } else {
        QMessageBox::critical(this, tr("Recording Error"), 
                            tr("Failed to start recording session."));
    }
}

void StreamRecordingDialog::stopRecordingSession()
{
    if (!m_currentSessionId.isEmpty()) {
        m_recordingManager->stop_recording_session(m_currentSessionId);
        
        m_startRecordingButton->setEnabled(true);
        m_stopRecordingButton->setEnabled(false);
        m_pauseRecordingButton->setEnabled(false);
        
        m_currentSessionId.clear();
        qInfo() << "Recording session stopped";
    }
}

void StreamRecordingDialog::pauseRecordingSession()
{
    if (!m_currentSessionId.isEmpty()) {
        if (m_pauseRecordingButton->text() == tr("&Pause Recording")) {
            m_recordingManager->pause_recording_session(m_currentSessionId);
            m_pauseRecordingButton->setText(tr("&Resume Recording"));
            m_pauseRecordingButton->setIcon(QIcon(":/icons/record_resume.png"));
        } else {
            m_recordingManager->resume_recording_session(m_currentSessionId);
            m_pauseRecordingButton->setText(tr("&Pause Recording"));
            m_pauseRecordingButton->setIcon(QIcon(":/icons/record_pause.png"));
        }
    }
}

void StreamRecordingDialog::resumeRecordingSession()
{
    // Implementation handled in pauseRecordingSession
    pauseRecordingSession();
}

void StreamRecordingDialog::browseOutputDirectory()
{
    QString currentDir = m_outputDirectoryEdit->text();
    if (currentDir.isEmpty()) {
        currentDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString selectedDir = QFileDialog::getExistingDirectory(this, 
        tr("Select Output Directory"), 
        currentDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!selectedDir.isEmpty()) {
        m_outputDirectoryEdit->setText(selectedDir);
    }
}

void StreamRecordingDialog::testRecordingSettings()
{
    if (!validateConfiguration()) {
        return;
    }

    // TODO: Implement settings test
    QMessageBox::information(this, tr("Test Settings"), 
                           tr("Settings test functionality will be implemented.\n\n"
                              "Current settings appear to be valid for recording."));
}

void StreamRecordingDialog::resetToDefaults()
{
    // Reset to default values
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ETI_Recordings";
    m_outputDirectoryEdit->setText(defaultDir);
    
    m_outputFormatCombo->setCurrentIndex(0); // ETI-NI
    m_enableCompressionCheck->setChecked(false);
    m_compressionLevelSpin->setValue(DEFAULT_COMPRESSION_LEVEL);
    
    m_segmentDurationSpin->setValue(DEFAULT_SEGMENT_DURATION_MINUTES);
    m_segmentDurationUnit->setCurrentIndex(0); // Minutes
    m_maxSegmentSizeSpin->setValue(DEFAULT_MAX_SEGMENT_SIZE_MB);
    m_maxArchiveSegmentsSpin->setValue(100);
    m_filenamePatternEdit->setText("eti_%Y%m%d_%H%M%S");
    
    m_enableDailyRotationCheck->setChecked(false);
    m_dailyRotationHourSpin->setValue(0);
    m_enableAutoArchiveCheck->setChecked(false);
    
    m_enableAutoCleanupCheck->setChecked(true);
    m_retentionPeriodSpin->setValue(DEFAULT_RETENTION_HOURS);
    m_retentionPeriodUnit->setCurrentIndex(2); // Weeks
    m_maxDiskUsageSpin->setValue(DEFAULT_DISK_USAGE_PERCENT);
    
    m_preserveTimestampsCheck->setChecked(true);
    m_enableIntegrityCheckingCheck->setChecked(true);
    m_qualitySlider->setValue(8);
    
    m_embedMetadataCheck->setChecked(true);
    m_embedEnsembleInfoCheck->setChecked(true);
    m_embedServiceInfoCheck->setChecked(true);
    m_customMetadataEdit->clear();
    
    updateFormatDescription();
    updateSegmentationPreview();
    
    m_configurationChanged = true;
}

// Update slot implementations
void StreamRecordingDialog::updateFormatDescription()
{
    QString description;
    switch (static_cast<eti::RecordingFormat>(m_outputFormatCombo->currentData().toInt())) {
        case eti::RecordingFormat::ETI_NI:
            description = tr("ETI Native Interface format (6144 bytes per frame, 24ms audio). "
                           "Standard format for DAB transmission systems.");
            break;
        case eti::RecordingFormat::ETI_LI:
            description = tr("ETI Linear Interface format for direct processing. "
                           "Optimized for real-time analysis applications.");
            break;
        case eti::RecordingFormat::EDI:
            description = tr("Encapsulated DAB Interface for IP distribution. "
                           "Network-friendly format with error correction.");
            break;
        case eti::RecordingFormat::RAW_BINARY:
            description = tr("Unprocessed stream data. Preserves all original data "
                           "including synchronization and error information.");
            break;
        case eti::RecordingFormat::COMPRESSED_ETI:
            description = tr("ZIP compressed ETI format with embedded metadata. "
                           "Reduces file size while maintaining data integrity.");
            break;
        case eti::RecordingFormat::COMPRESSED_ARCHIVE:
            description = tr("TAR/ZIP archive with metadata for long-term storage. "
                           "Includes comprehensive documentation and checksums.");
            break;
        default:
            description = tr("Unknown format selected.");
            break;
    }
    
    m_formatDescriptionLabel->setText(description);
}

void StreamRecordingDialog::updateSegmentationPreview()
{
    QString pattern = m_filenamePatternEdit->text();
    QDateTime now = QDateTime::currentDateTime();
    QString preview = now.toString(pattern.replace("%Y", "yyyy")
                                          .replace("%m", "MM")
                                          .replace("%d", "dd")
                                          .replace("%H", "hh")
                                          .replace("%M", "mm")
                                          .replace("%S", "ss")) + ".eti";
    m_segmentPreviewLabel->setText(preview);
}

void StreamRecordingDialog::updateRecordingStatus()
{
    if (m_currentSessionId.isEmpty()) {
        m_currentSessionLabel->setText(tr("No active session"));
        m_recordingProgressBar->setValue(0);
        return;
    }

    // Get current session info
    if (m_recordingManager && m_recordingManager->is_session_active(m_currentSessionId)) {
        auto session = m_recordingManager->get_session_info(m_currentSessionId);
        
        m_currentSessionLabel->setText(tr("Session: %1").arg(m_currentSessionId));
        
        // Update statistics (example values for demonstration)
        m_framesRecordedLabel->setText(QString::number(session.frames_recorded.load()));
        m_bytesWrittenLabel->setText(formatFileSize(session.bytes_written.load()));
        m_segmentsCreatedLabel->setText(QString::number(session.segments_created.load()));
        m_errorsEncounteredLabel->setText(QString::number(session.errors_encountered.load()));
        m_averageFrameRateLabel->setText(QString("%1 fps").arg(session.average_frame_rate, 0, 'f', 1));
        
        auto latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(session.average_latency);
        m_currentLatencyLabel->setText(QString("%1 ms").arg(latencyMs.count()));
    }
}

void StreamRecordingDialog::updateDiskSpaceDisplay()
{
    QString outputDir = m_outputDirectoryEdit->text();
    if (outputDir.isEmpty()) return;

    QStorageInfo storage(outputDir);
    if (storage.isValid()) {
        qint64 totalBytes = storage.bytesTotal();
        qint64 availableBytes = storage.bytesAvailable();
        qint64 usedBytes = totalBytes - availableBytes;
        
        double usagePercent = (double)usedBytes / totalBytes * 100.0;
        
        m_diskSpaceLabel->setText(tr("Available: %1 / %2")
                                 .arg(formatFileSize(availableBytes))
                                 .arg(formatFileSize(totalBytes)));
        
        m_diskUsageBar->setValue(static_cast<int>(usagePercent));
        
        // Update directory watcher
        if (!m_directoryWatcher->directories().contains(outputDir)) {
            m_directoryWatcher->addPath(outputDir);
        }
    }
}

void StreamRecordingDialog::updateSessionList()
{
    if (!m_recordingManager) return;

    QStringList sessionIds = m_recordingManager->get_active_sessions();
    
    m_sessionsTable->setRowCount(sessionIds.size());
    
    for (int i = 0; i < sessionIds.size(); ++i) {
        const QString& sessionId = sessionIds[i];
        auto session = m_recordingManager->get_session_info(sessionId);
        
        m_sessionsTable->setItem(i, 0, new QTableWidgetItem(sessionId));
        m_sessionsTable->setItem(i, 1, new QTableWidgetItem(getSessionStatusText(session)));
        m_sessionsTable->setItem(i, 2, new QTableWidgetItem(session.start_time.toString("yyyy-MM-dd hh:mm:ss")));
        
        qint64 duration = QDateTime::currentDateTime().msecsTo(session.start_time);
        if (duration < 0) duration = -duration;
        m_sessionsTable->setItem(i, 3, new QTableWidgetItem(formatDuration(duration)));
        
        m_sessionsTable->setItem(i, 4, new QTableWidgetItem(QString::number(session.frames_recorded.load())));
        m_sessionsTable->setItem(i, 5, new QTableWidgetItem(formatFileSize(session.bytes_written.load())));
        m_sessionsTable->setItem(i, 6, new QTableWidgetItem(getFormatDisplayName(session.format)));
    }
    
    m_sessionsTable->resizeColumnsToContents();
}

// Helper methods
QString StreamRecordingDialog::getFormatDisplayName(eti::RecordingFormat format) const
{
    switch (format) {
        case eti::RecordingFormat::ETI_NI: return tr("ETI-NI");
        case eti::RecordingFormat::ETI_LI: return tr("ETI-LI");
        case eti::RecordingFormat::EDI: return tr("EDI");
        case eti::RecordingFormat::RAW_BINARY: return tr("Raw");
        case eti::RecordingFormat::COMPRESSED_ETI: return tr("Compressed ETI");
        case eti::RecordingFormat::COMPRESSED_ARCHIVE: return tr("Archive");
        default: return tr("Unknown");
    }
}

QString StreamRecordingDialog::getSessionStatusText(const eti::RecordingSession& session) const
{
    if (session.is_active) {
        if (session.is_paused) {
            return tr("Paused");
        } else {
            return tr("Recording");
        }
    } else {
        return tr("Stopped");
    }
}

QString StreamRecordingDialog::formatFileSize(qint64 bytes) const
{
    if (bytes < 1024) return tr("%1 bytes").arg(bytes);
    if (bytes < 1024 * 1024) return tr("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024 * 1024 * 1024) return tr("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    return tr("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

QString StreamRecordingDialog::formatDuration(qint64 milliseconds) const
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    if (hours > 0) {
        return tr("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    } else {
        return tr("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
    }
}

// Event handlers
void StreamRecordingDialog::handleOutputFormatChanged()
{
    m_currentFormat = static_cast<eti::RecordingFormat>(m_outputFormatCombo->currentData().toInt());
    updateFormatDescription();
    m_configurationChanged = true;
}

void StreamRecordingDialog::handleSegmentationChanged()
{
    updateSegmentationPreview();
    m_configurationChanged = true;
}

void StreamRecordingDialog::handleCompressionChanged()
{
    bool compressionEnabled = m_enableCompressionCheck->isChecked();
    m_compressionLevelSpin->setEnabled(compressionEnabled);
    m_configurationChanged = true;
}

void StreamRecordingDialog::handleOutputDirectoryChanged()
{
    updateDiskSpaceDisplay();
    m_configurationChanged = true;
}

void StreamRecordingDialog::handleSessionSelectionChanged()
{
    int currentRow = m_sessionsTable->currentRow();
    if (currentRow >= 0) {
        QTableWidgetItem *sessionItem = m_sessionsTable->item(currentRow, 0);
        if (sessionItem) {
            QString sessionId = sessionItem->text();
            // TODO: Show session details
            qInfo() << "Selected session:" << sessionId;
        }
    }
}

void StreamRecordingDialog::handleRecordingSessionStarted(const QString& sessionId)
{
    updateSessionList();
    emit recordingSessionStarted(sessionId);
}

void StreamRecordingDialog::handleRecordingSessionStopped(const QString& sessionId)
{
    updateSessionList();
    emit recordingSessionStopped(sessionId);
}

void StreamRecordingDialog::handleSessionStatisticsUpdate(const QString& sessionId, 
                                                         const eti::RecordingSession& session)
{
    if (sessionId == m_currentSessionId) {
        updateRecordingStatus();
    }
}

void StreamRecordingDialog::handleRecordingError(const QString& sessionId, const QString& error)
{
    QMessageBox::warning(this, tr("Recording Error"), 
                        tr("Session %1: %2").arg(sessionId, error));
}

void StreamRecordingDialog::loadConfiguration()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Recording Configuration"),
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
        tr("Configuration Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        // TODO: Implement configuration loading
        QMessageBox::information(this, tr("Load Configuration"), 
                               tr("Configuration loading will be implemented."));
    }
}

void StreamRecordingDialog::saveConfiguration()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Recording Configuration"),
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/recording_config.json",
        tr("Configuration Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        // TODO: Implement configuration saving
        QMessageBox::information(this, tr("Save Configuration"), 
                               tr("Configuration saving will be implemented."));
    }
}

void StreamRecordingDialog::accept()
{
    if (m_configurationChanged) {
        applyConfiguration();
    }
    QDialog::accept();
}

void StreamRecordingDialog::reject()
{
    if (m_configurationChanged) {
        int ret = QMessageBox::question(this, tr("Unsaved Changes"),
                                      tr("You have unsaved changes. Do you want to save them?"),
                                      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save) {
            applyConfiguration();
            QDialog::accept();
        } else if (ret == QMessageBox::Cancel) {
            return; // Don't close dialog
        }
        // For Discard, continue with reject
    }
    
    QDialog::reject();
}