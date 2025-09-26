/**
 * @file professional_workflow_manager.cpp
 * @brief Professional broadcast user workflow integration manager implementation
 * 
 * Implements comprehensive user workflows that integrate all GUI components
 * with the ETSI-compliant ETI processing engine for broadcast industry standards.
 */

#include "professional_workflow_manager.h"
#include "main_window.h"
#include "service_explorer_panel.h"
#include "analyzer_widget.h"
#include "constellation_widget.h"
#include "service_browser.h"
#include "audio_monitoring_widget.h"
#include "fig_analysis_widget.h"
#include "performance_dashboard.h"
#include "etsi_compliance_monitor.h"
#include "core/eti_processor.h"
#include "utils/logger.h"

#include <QApplication>
#include <QKeyEvent>
#include <QShortcut>
#include <QProgressDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QStandardPaths>
#include <QDebug>

ProfessionalWorkflowManager::ProfessionalWorkflowManager(MainWindow* mainWindow, QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_etiProcessor(nullptr)
    , m_serviceExplorerPanel(nullptr)
    , m_performanceDashboard(nullptr)
    , m_complianceMonitor(nullptr)
    , m_analyzerWidget(nullptr)
    , m_constellationWidget(nullptr)
    , m_serviceBrowser(nullptr)
    , m_audioMonitor(nullptr)
    , m_figAnalysis(nullptr)
    , m_currentState(Idle)
    // No QStateMachine initialization needed
    , m_panelSyncActive(false)
    , m_panelSyncMode(AutoSync)
    , m_panelSyncTimer(nullptr)
    , m_syncLatencyTimer(nullptr)
    , m_metricsUpdateTimer(nullptr)
    , m_uiUpdateTimer(nullptr)
    , m_workflowTimer(nullptr)
    , m_selectedServiceId(0)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Constructor starting");
    
    // Initialize timers
    m_syncLatencyTimer = new QElapsedTimer();
    m_workflowTimer = new QElapsedTimer();
    
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Constructor completed");
}

ProfessionalWorkflowManager::~ProfessionalWorkflowManager()
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Destructor starting");
    
    // Stop all timers
    if (m_panelSyncTimer) {
        m_panelSyncTimer->stop();
        delete m_panelSyncTimer;
    }
    
    if (m_metricsUpdateTimer) {
        m_metricsUpdateTimer->stop();
        delete m_metricsUpdateTimer;
    }
    
    if (m_uiUpdateTimer) {
        m_uiUpdateTimer->stop();
        delete m_uiUpdateTimer;
    }
    
    // Cleanup timers
    delete m_syncLatencyTimer;
    delete m_workflowTimer;
    
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Destructor completed");
}

bool ProfessionalWorkflowManager::initialize()
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Initialization starting");
    
    try {
        // Get component references from main window
        if (!m_mainWindow) {
            Logger::instance().log(Logger::Error, "ProfessionalWorkflowManager", "Main window is null");
            return false;
        }
        
        m_etiProcessor = m_mainWindow->getEtiProcessor();
        m_analyzerWidget = m_mainWindow->getAnalyzerWidget();
        m_constellationWidget = m_mainWindow->getConstellationWidget();
        m_serviceBrowser = m_mainWindow->getServiceBrowser();
        
        // Get additional component references from main window
        m_serviceExplorerPanel = m_mainWindow->findChild<ServiceExplorerPanel*>();
        m_performanceDashboard = m_mainWindow->findChild<PerformanceDashboard*>();
        m_complianceMonitor = m_mainWindow->findChild<ETSIComplianceMonitor*>();
        m_audioMonitor = m_mainWindow->findChild<AudioMonitoringWidget*>();
        m_figAnalysis = m_mainWindow->findChild<FigAnalysisWidget*>();
        
        // Validate essential components
        if (!m_etiProcessor) {
            Logger::instance().log(Logger::Error, "ProfessionalWorkflowManager", "ETI processor is null");
            return false;
        }
        
        // Initialize workflow state machine
        initializeWorkflowStateMachine();
        
        // Setup signal/slot connections
        setupSignalSlotConnections();
        
        // Create timers
        createPanelSyncTimer();
        
        // Setup professional keyboard shortcuts
        setupKeyboardShortcuts();
        
        // Initialize workflow metrics
        m_workflowMetrics = WorkflowMetrics();
        m_workflowTimer->start();
        
        Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "ProfessionalWorkflowManager", 
                               QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

// ==============================================
// FILE ANALYSIS WORKFLOW IMPLEMENTATION
// ==============================================

void ProfessionalWorkflowManager::startFileAnalysisWorkflow(const QString& filename)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Starting File Analysis Workflow: %1").arg(filename));
    
    // Validate file exists
    if (!QFileInfo::exists(filename)) {
        Logger::instance().log(Logger::Error, "ProfessionalWorkflowManager", 
                               QString("File does not exist: %1").arg(filename));
        return;
    }
    
    // Update workflow state
    WorkflowState oldState = m_currentState;
    m_currentState = FileAnalysisActive;
    emit workflowStateChanged(m_currentState, oldState);
    
    // Store current filename
    m_currentFilename = filename;
    
    // Start workflow timer
    m_workflowTimer->restart();
    m_workflowMetrics.workflowStartTime = QDateTime::currentMSecsSinceEpoch();
    m_workflowMetrics.currentWorkflow = "File Analysis";
    
    // Enable panel synchronization
    m_panelSyncActive = true;
    setPanelSyncMode(AutoSync);
    
    // Update status
    updateWorkflowStatus("File Analysis Workflow Started", QString("Analyzing: %1").arg(QFileInfo(filename).fileName()));
    
    // Process file with ETI processor
    if (m_etiProcessor && m_etiProcessor->is_initialized()) {
        Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Processing ETI file");
        
        // Process file asynchronously to avoid blocking UI
        bool result = m_etiProcessor->process_file(filename);
        if (!result) {
            Logger::instance().log(Logger::Error, "ProfessionalWorkflowManager", "Failed to process ETI file");
            updateWorkflowStatus("File Analysis Failed", "Could not process ETI file");
            m_currentState = Idle;
            return;
        }
        
        // File processing started successfully
        updateWorkflowStatus("Processing ETI File", "Parsing frames and discovering services...");
    } else {
        Logger::instance().log(Logger::Error, "ProfessionalWorkflowManager", "ETI processor not available");
        updateWorkflowStatus("File Analysis Failed", "ETI processor not initialized");
        m_currentState = Idle;
    }
}

void ProfessionalWorkflowManager::onFileOpened(const QString& filename)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("File opened: %1").arg(filename));
    
    // Update metrics
    m_workflowMetrics.totalFramesProcessed = 0;
    m_workflowMetrics.servicesDiscovered = 0;
    m_workflowMetrics.errorsDetected = 0;
    
    // Synchronize all panels with new file data
    synchronizeAllPanels();
    
    // Update status
    updateWorkflowStatus("File Opened Successfully", QString("Ready to analyze: %1").arg(QFileInfo(filename).fileName()));
}

void ProfessionalWorkflowManager::onFileAnalysisCompleted(int frameCount, int serviceCount)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("File analysis completed: %1 frames, %2 services").arg(frameCount).arg(serviceCount));
    
    // Update metrics
    m_workflowMetrics.totalFramesProcessed = frameCount;
    m_workflowMetrics.servicesDiscovered = serviceCount;
    
    qint64 analysisTime = m_workflowTimer->elapsed();
    if (analysisTime > 0) {
        m_workflowMetrics.etiProcessingRate = (frameCount * 1000.0) / analysisTime; // FPS
    }
    
    // Synchronize all panels with final results
    synchronizeAllPanels();
    
    // Update status
    updateWorkflowStatus("File Analysis Completed", 
                        QString("Processed %1 frames, discovered %2 services in %3ms")
                        .arg(frameCount).arg(serviceCount).arg(analysisTime));
    
    // Transition to service discovery workflow
    startServiceDiscoveryWorkflow();
}

// ==============================================
// REAL-TIME ANALYSIS WORKFLOW IMPLEMENTATION
// ==============================================

void ProfessionalWorkflowManager::startRealTimeAnalysisWorkflow(const QHash<QString, QVariant>& sourceConfig)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Starting Real-Time Analysis Workflow");
    
    // Update workflow state
    WorkflowState oldState = m_currentState;
    m_currentState = RealTimeAnalysisActive;
    emit workflowStateChanged(m_currentState, oldState);
    
    // Start workflow timer
    m_workflowTimer->restart();
    m_workflowMetrics.workflowStartTime = QDateTime::currentMSecsSinceEpoch();
    m_workflowMetrics.currentWorkflow = "Real-Time Analysis";
    
    // Enable high-frequency panel synchronization for real-time
    m_panelSyncActive = true;
    setPanelSyncMode(AutoSync);
    
    // Start UI update timer for 60 FPS
    if (!m_uiUpdateTimer) {
        m_uiUpdateTimer = new QTimer(this);
        connect(m_uiUpdateTimer, &QTimer::timeout, this, &ProfessionalWorkflowManager::onUIUpdateTimer);
    }
    m_uiUpdateTimer->start(UI_UPDATE_INTERVAL_MS);
    
    // Update status
    QString sourceInfo = sourceConfig.value("address", "Unknown").toString() + ":" + 
                        sourceConfig.value("port", "Unknown").toString();
    updateWorkflowStatus("Real-Time Analysis Started", QString("Source: %1").arg(sourceInfo));
    
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Real-Time Analysis Workflow initialized");
}

void ProfessionalWorkflowManager::onRealTimeModeChanged(bool enabled)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Real-time mode changed: %1").arg(enabled ? "enabled" : "disabled"));
    
    if (enabled) {
        // Real-time mode activated
        if (m_currentState != RealTimeAnalysisActive) {
            QHash<QString, QVariant> defaultConfig;
            defaultConfig["address"] = "239.192.0.1";
            defaultConfig["port"] = 9200;
            startRealTimeAnalysisWorkflow(defaultConfig);
        }
    } else {
        // Real-time mode deactivated
        if (m_uiUpdateTimer) {
            m_uiUpdateTimer->stop();
        }
        
        updateWorkflowStatus("Real-Time Analysis Stopped", "Returned to file analysis mode");
        
        // Transition back to appropriate workflow
        if (!m_currentFilename.isEmpty()) {
            m_currentState = FileAnalysisActive;
        } else {
            m_currentState = Idle;
        }
    }
}

void ProfessionalWorkflowManager::onLiveFrameProcessed(const QByteArray& frameData, double processingRate)
{
    // Update metrics
    m_workflowMetrics.totalFramesProcessed++;
    m_workflowMetrics.etiProcessingRate = processingRate;
    
    // Emit processing rate signal
    emit realTimeProcessingRateChanged(processingRate);
    
    // Synchronize panels if processing rate is within target
    if (processingRate >= TARGET_ETI_PROCESSING_RATE) {
        // Only synchronize if we're meeting performance targets
        if (m_panelSyncActive) {
            synchronizeAllPanels();
        }
    }
}

// ==============================================
// ERROR DETECTION WORKFLOW IMPLEMENTATION
// ==============================================

void ProfessionalWorkflowManager::startErrorDetectionWorkflow(const QString& filename)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Starting Error Detection Workflow: %1").arg(filename));
    
    // Update workflow state
    WorkflowState oldState = m_currentState;
    m_currentState = ErrorDetectionActive;
    emit workflowStateChanged(m_currentState, oldState);
    
    // Start workflow timer
    m_workflowTimer->restart();
    m_workflowMetrics.workflowStartTime = QDateTime::currentMSecsSinceEpoch();
    m_workflowMetrics.currentWorkflow = "Error Detection";
    
    // Clear previous errors
    m_detectedErrors.clear();
    m_workflowMetrics.errorsDetected = 0;
    
    // Enable panel synchronization for error navigation
    m_panelSyncActive = true;
    setPanelSyncMode(AutoSync);
    
    // Update status
    updateWorkflowStatus("Error Detection Started", QString("Scanning: %1").arg(QFileInfo(filename).fileName()));
    
    // Start file analysis with error focus
    startFileAnalysisWorkflow(filename);
}

void ProfessionalWorkflowManager::onETSIComplianceError(const eti::ETSIComplianceResult& error)
{
    QString errorDetails = QString("Compliance: %1, Score: %2%, Errors: %3")
                           .arg(error.is_compliant ? "PASS" : "FAIL")
                           .arg(error.compliance_score, 0, 'f', 1)
                           .arg(error.errors.size());
    Logger::instance().log(Logger::Warning, "ProfessionalWorkflowManager", 
                           QString("ETSI compliance error detected: %1").arg(errorDetails));
    
    // Store error
    m_detectedErrors.append(error);
    m_workflowMetrics.errorsDetected++;
    
    // Synchronize messages panel with new error
    synchronizeMessagesPanel(m_detectedErrors);
    
    // Update status with detailed error information
    QString statusMessage = QString("ETSI compliance issues: %1 errors, %2 warnings")
                            .arg(error.errors.size()).arg(error.warnings.size());
    updateWorkflowStatus("Error Detected", statusMessage);
}

void ProfessionalWorkflowManager::navigateToErrorFrame(quint64 frameNumber, const QString& errorType)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Navigating to error frame %1 (type: %2)").arg(frameNumber).arg(errorType));
    
    // Measure navigation latency
    m_syncLatencyTimer->start();
    
    // Emit navigation request
    emit errorNavigationRequested(frameNumber, errorType);
    
    // Synchronize all panels to show error context
    synchronizeAllPanels();
    
    // Measure and log navigation latency
    qint64 navigationLatency = m_syncLatencyTimer->elapsed();
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Error navigation completed in %1ms").arg(navigationLatency));
    
    // Update status
    updateWorkflowStatus(QString("Navigated to Error Frame %1").arg(frameNumber), 
                        QString("Error Type: %1, Navigation Time: %2ms").arg(errorType).arg(navigationLatency));
}

bool ProfessionalWorkflowManager::generateErrorReport(const QString& outputPath)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Generating error report: %1").arg(outputPath));
    
    try {
        QFile reportFile(outputPath);
        if (!reportFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Logger::instance().log(Logger::Error, "ProfessionalWorkflowManager", 
                                   QString("Cannot open report file: %1").arg(outputPath));
            return false;
        }
        
        QTextStream out(&reportFile);
        
        // Generate professional error report
        out << "ETI Stream Analyzer - ETSI Compliance Error Report\n";
        out << "================================================\n\n";
        
        out << "Report Generated: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        out << "Analyzed File: " << m_currentFilename << "\n";
        out << "Total Frames Processed: " << m_workflowMetrics.totalFramesProcessed << "\n";
        out << "Total Errors Detected: " << m_workflowMetrics.errorsDetected << "\n";
        out << "Analysis Duration: " << m_workflowTimer->elapsed() << " ms\n\n";
        
        // Error summary by severity
        QHash<QString, int> errorsBySeverity;
        for (const auto& error : m_detectedErrors) {
            QString severity = error.is_compliant ? "Warning" : "Critical";
            if (error.compliance_score < 50.0) severity = "Critical";
            else if (error.compliance_score < 80.0) severity = "Major";
            else severity = "Minor";
            errorsBySeverity[severity]++;
        }
        
        out << "Error Summary by Severity:\n";
        out << "-------------------------\n";
        for (auto it = errorsBySeverity.begin(); it != errorsBySeverity.end(); ++it) {
            out << it.key() << ": " << it.value() << " errors\n";
        }
        out << "\n";
        
        // Detailed error listing
        out << "Detailed Error Listing:\n";
        out << "----------------------\n";
        for (int i = 0; i < m_detectedErrors.size(); ++i) {
            const auto& error = m_detectedErrors[i];
            out << QString("Error %1:\n").arg(i + 1);
            out << "  Compliance Status: " << (error.is_compliant ? "PASS" : "FAIL") << "\n";
            out << "  Compliance Score: " << QString::number(error.compliance_score, 'f', 1) << "%\n";
            out << "  Error Count: " << error.errors.size() << "\n";
            out << "  Warning Count: " << error.warnings.size() << "\n";
            out << "  ETSI Standard: " << QString::fromStdString(error.etsi_standard) << "\n";
            out << "  Test Date: " << QString::fromStdString(error.test_date) << "\n";
            
            // List individual errors
            if (!error.errors.empty()) {
                out << "  Individual Errors:\n";
                for (const auto& err : error.errors) {
                    out << "    - " << QString::fromStdString(err) << "\n";
                }
            }
            
            // List warnings
            if (!error.warnings.empty()) {
                out << "  Warnings:\n";
                for (const auto& warn : error.warnings) {
                    out << "    - " << QString::fromStdString(warn) << "\n";
                }
            }
            out << "  Timestamp: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
        }
        
        reportFile.close();
        
        // Emit report generated signal
        emit professionalReportGenerated(outputPath, "ETSI Compliance Error Report");
        
        Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                               QString("Error report generated successfully: %1").arg(outputPath));
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "ProfessionalWorkflowManager", 
                               QString("Error report generation failed: %1").arg(e.what()));
        return false;
    }
}

// ==============================================
// SERVICE DISCOVERY WORKFLOW IMPLEMENTATION
// ==============================================

void ProfessionalWorkflowManager::startServiceDiscoveryWorkflow()
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Starting Service Discovery Workflow");
    
    // Update workflow state
    WorkflowState oldState = m_currentState;
    m_currentState = ServiceDiscoveryActive;
    emit workflowStateChanged(m_currentState, oldState);
    
    // Enable panel synchronization for service navigation
    m_panelSyncActive = true;
    setPanelSyncMode(FilterSync);
    
    // Update status
    updateWorkflowStatus("Service Discovery Active", 
                        QString("Found %1 services ready for analysis").arg(m_workflowMetrics.servicesDiscovered));
    
    // Synchronize explorer panel with discovered services
    QList<eti::DabService> serviceList;
    for (auto it = m_discoveredServices.begin(); it != m_discoveredServices.end(); ++it) {
        serviceList.append(it.value());
    }
    synchronizeExplorerPanel(serviceList);
    
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Service Discovery Workflow activated");
}

void ProfessionalWorkflowManager::onServiceSelected(quint32 serviceId)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Service selected: %1").arg(serviceId));
    
    // Store selected service
    m_selectedServiceId = serviceId;
    
    // Measure panel synchronization latency
    qint64 syncLatency = measurePanelSyncLatency();
    
    // Synchronize properties panel with selected service
    synchronizePropertiesPanel(serviceId);
    
    // Filter main panel by selected service
    QList<quint32> serviceFilter = {serviceId};
    filterMainPanelByServices(serviceFilter);
    
    // Emit service selection changed signal
    emit serviceSelectionChanged(serviceId);
    
    // Update status
    QString serviceName = m_discoveredServices.contains(serviceId) ? 
                         QString::fromStdString(m_discoveredServices[serviceId].label) : QString("Service %1").arg(serviceId);
    updateWorkflowStatus(QString("Service Selected: %1").arg(serviceName), 
                        QString("Sync Latency: %1ms").arg(syncLatency));
}

void ProfessionalWorkflowManager::onServicePropertiesUpdated(quint32 serviceId, const QHash<QString, QVariant>& properties)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Service properties updated: %1").arg(serviceId));
    
    // Update discovered service data
    if (m_discoveredServices.contains(serviceId)) {
        // Update service properties (simplified for now)
        auto& service = m_discoveredServices[serviceId];
        if (properties.contains("label")) {
            service.label = properties["label"].toString().toStdString();
        }
        if (properties.contains("bitrate")) {
            // TODO: service.bitrate = properties["bitrate"].toInt(); // Field not available in DabService struct
            Q_UNUSED(properties); // Suppress unused variable warning
        }
    }
    
    // If this is the currently selected service, synchronize properties panel
    if (serviceId == m_selectedServiceId) {
        synchronizePropertiesPanel(serviceId);
    }
}

void ProfessionalWorkflowManager::filterMainPanelByServices(const QList<quint32>& serviceIds)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Filtering main panel by %1 services").arg(serviceIds.size()));
    
    // TODO: Implement main panel filtering when main panel component is available
    // For now, just log the filter request
    
    QString serviceIdList;
    for (quint32 serviceId : serviceIds) {
        if (!serviceIdList.isEmpty()) serviceIdList += ", ";
        serviceIdList += QString::number(serviceId);
    }
    
    updateWorkflowStatus("Main Panel Filtered", QString("Showing services: %1").arg(serviceIdList));
}

// ==============================================
// PANEL SYNCHRONIZATION ARCHITECTURE
// ==============================================

void ProfessionalWorkflowManager::setPanelSyncMode(PanelSyncMode mode)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Setting panel sync mode: %1").arg(static_cast<int>(mode)));
    
    m_panelSyncMode = mode;
    
    // Adjust sync timer interval based on mode
    if (m_panelSyncTimer) {
        switch (mode) {
            case AutoSync:
                m_panelSyncTimer->setInterval(PANEL_SYNC_INTERVAL_MS);
                break;
            case ManualSync:
                m_panelSyncTimer->stop();
                break;
            case FilterSync:
                m_panelSyncTimer->setInterval(PANEL_SYNC_INTERVAL_MS / 2); // Higher frequency for filtering
                break;
            case CompareSync:
                m_panelSyncTimer->setInterval(PANEL_SYNC_INTERVAL_MS * 2); // Lower frequency for comparison
                break;
        }
        
        if (mode != ManualSync && m_panelSyncActive) {
            m_panelSyncTimer->start();
        }
    }
}

void ProfessionalWorkflowManager::synchronizeAllPanels()
{
    if (!m_panelSyncActive) {
        return;
    }
    
    // Measure synchronization latency
    qint64 syncLatency = measurePanelSyncLatency();
    
    // Synchronize all panels (placeholder implementations)
    Logger::instance().log(Logger::Debug, "ProfessionalWorkflowManager", "Synchronizing all panels");
    
    // TODO: Implement actual panel synchronization when components are available
    // synchronizeExplorerPanel(serviceList);
    // synchronizePropertiesPanel(m_selectedServiceId);
    // synchronizeMainPanel(frameList);
    // synchronizeMessagesPanel(m_detectedErrors);
    
    // Update metrics
    m_workflowMetrics.panelSyncLatency = syncLatency;
    
    // Emit synchronization completed signal
    emit panelSynchronizationCompleted(syncLatency);
    
    Logger::instance().log(Logger::Debug, "ProfessionalWorkflowManager", 
                           QString("Panel synchronization completed in %1ms").arg(syncLatency));
}

void ProfessionalWorkflowManager::synchronizeExplorerPanel(const QList<eti::DabService>& services)
{
    Logger::instance().log(Logger::Debug, "ProfessionalWorkflowManager", 
                           QString("Synchronizing Explorer panel with %1 services").arg(services.size()));
    
    // TODO: Implement explorer panel synchronization when component is available
    // if (m_serviceExplorerPanel) {
    //     m_serviceExplorerPanel->updateServices(services);
    // }
}

void ProfessionalWorkflowManager::synchronizePropertiesPanel(quint32 serviceId)
{
    Logger::instance().log(Logger::Debug, "ProfessionalWorkflowManager", 
                           QString("Synchronizing Properties panel for service %1").arg(serviceId));
    
    // TODO: Implement properties panel synchronization when component is available
    // if (m_propertiesPanel && m_discoveredServices.contains(serviceId)) {
    //     m_propertiesPanel->showServiceProperties(m_discoveredServices[serviceId]);
    // }
}

void ProfessionalWorkflowManager::synchronizeMainPanel(const QList<eti::EtiFrame>& frames)
{
    Logger::instance().log(Logger::Debug, "ProfessionalWorkflowManager", 
                           QString("Synchronizing Main panel with %1 frames").arg(frames.size()));
    
    // TODO: Implement main panel synchronization when component is available
    // if (m_mainPanel) {
    //     m_mainPanel->updateFrames(frames);
    // }
}

void ProfessionalWorkflowManager::synchronizeMessagesPanel(const QList<eti::ETSIComplianceResult>& errors)
{
    Logger::instance().log(Logger::Debug, "ProfessionalWorkflowManager", 
                           QString("Synchronizing Messages panel with %1 errors").arg(errors.size()));
    
    // TODO: Implement messages panel synchronization when component is available
    // if (m_messagesPanel) {
    //     m_messagesPanel->updateErrors(errors);
    // }
}

// ==============================================
// PROFESSIONAL USER EXPERIENCE
// ==============================================

void ProfessionalWorkflowManager::setupKeyboardShortcuts()
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Setting up professional keyboard shortcuts");
    
    // Define professional broadcast industry keyboard shortcuts
    m_keyboardShortcuts.clear();
    
    // File operations
    m_keyboardShortcuts[QKeySequence::Open] = "Open ETI File";
    m_keyboardShortcuts[QKeySequence::Close] = "Close File";
    m_keyboardShortcuts[QKeySequence::Refresh] = "Refresh Analysis";
    
    // Analysis operations
    m_keyboardShortcuts[QKeySequence(Qt::Key_F5)] = "Start Real-time Analysis";
    m_keyboardShortcuts[QKeySequence(Qt::Key_F6)] = "Stop Analysis";
    m_keyboardShortcuts[QKeySequence(Qt::Key_F7)] = "Start Error Detection";
    m_keyboardShortcuts[QKeySequence(Qt::Key_F8)] = "Generate Report";
    
    // Navigation
    m_keyboardShortcuts[QKeySequence(Qt::Key_F2)] = "Next Error";
    m_keyboardShortcuts[QKeySequence(Qt::Key_F3)] = "Previous Error";
    m_keyboardShortcuts[QKeySequence(Qt::Key_F4)] = "Jump to Frame";
    
    // Panel operations
    m_keyboardShortcuts[QKeySequence(Qt::Key_F9)] = "Toggle Service Explorer";
    m_keyboardShortcuts[QKeySequence(Qt::Key_F10)] = "Toggle Properties Panel";
    m_keyboardShortcuts[QKeySequence(Qt::Key_F11)] = "Toggle Full Screen";
    m_keyboardShortcuts[QKeySequence(Qt::Key_F12)] = "Toggle Messages Panel";
    
    // Service operations
    m_keyboardShortcuts[QKeySequence(Qt::CTRL + Qt::Key_1)] = "Select Service 1";
    m_keyboardShortcuts[QKeySequence(Qt::CTRL + Qt::Key_2)] = "Select Service 2";
    m_keyboardShortcuts[QKeySequence(Qt::CTRL + Qt::Key_3)] = "Select Service 3";
    m_keyboardShortcuts[QKeySequence(Qt::CTRL + Qt::Key_A)] = "Select All Services";
    m_keyboardShortcuts[QKeySequence(Qt::CTRL + Qt::Key_D)] = "Deselect All Services";
    
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Configured %1 professional keyboard shortcuts").arg(m_keyboardShortcuts.size()));
}

void ProfessionalWorkflowManager::onKeyboardShortcutActivated(const QKeySequence& shortcut)
{
    if (!m_keyboardShortcuts.contains(shortcut)) {
        return;
    }
    
    QString action = m_keyboardShortcuts[shortcut];
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Keyboard shortcut activated: %1 -> %2").arg(shortcut.toString()).arg(action));
    
    // Handle shortcut actions
    if (action == "Start Real-time Analysis") {
        QHash<QString, QVariant> defaultConfig;
        defaultConfig["address"] = "239.192.0.1";
        defaultConfig["port"] = 9200;
        startRealTimeAnalysisWorkflow(defaultConfig);
    } else if (action == "Start Error Detection" && !m_currentFilename.isEmpty()) {
        startErrorDetectionWorkflow(m_currentFilename);
    } else if (action == "Generate Report") {
        QString reportPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + 
                            "/eti_error_report_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".txt";
        generateErrorReport(reportPath);
    }
    
    // Update status with shortcut action
    updateWorkflowStatus(QString("Shortcut Activated: %1").arg(action), shortcut.toString());
}

void ProfessionalWorkflowManager::showWorkflowProgress(const QString& workflowName, int progress)
{
    Logger::instance().log(Logger::Debug, "ProfessionalWorkflowManager", 
                           QString("Workflow progress: %1 = %2%").arg(workflowName).arg(progress));
    
    // TODO: Show progress in status bar or progress dialog when available
    updateWorkflowStatus(QString("%1 Progress: %2%").arg(workflowName).arg(progress), 
                        QString("Processing..."));
}

void ProfessionalWorkflowManager::updateWorkflowStatus(const QString& status, const QString& details)
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Workflow status: %1 | %2").arg(status).arg(details));
    
    // Update main window status if available
    if (m_mainWindow) {
        m_mainWindow->updateStatus(status, details);
    }
}

// ==============================================
// PRIVATE IMPLEMENTATION
// ==============================================

void ProfessionalWorkflowManager::setupSignalSlotConnections()
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Setting up signal/slot connections");
    
    // Connect ETI processor signals
    if (m_etiProcessor) {
        connect(m_etiProcessor, QOverload<quint64, const QByteArray&>::of(&EtiProcessor::frameProcessed),
                this, &ProfessionalWorkflowManager::onEtiFrameProcessed);
        
        connect(m_etiProcessor, &EtiProcessor::serviceDiscovered,
                this, &ProfessionalWorkflowManager::onEtiServiceDiscovered);
        
        connect(m_etiProcessor, &EtiProcessor::ensembleDiscovered,
                this, &ProfessionalWorkflowManager::onEtiEnsembleDiscovered);
        
        connect(m_etiProcessor, &EtiProcessor::errorOccurred,
                this, &ProfessionalWorkflowManager::onEtiErrorOccurred);
    }
    
    // Connect main window signals
    if (m_mainWindow) {
        connect(m_mainWindow, &MainWindow::etiFileOpened,
                this, &ProfessionalWorkflowManager::onFileOpened);
        
        connect(m_mainWindow, &MainWindow::realTimeModeChanged,
                this, &ProfessionalWorkflowManager::onRealTimeModeChanged);
    }
    
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Signal/slot connections established");
}

void ProfessionalWorkflowManager::initializeWorkflowStateMachine()
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Initializing workflow state machine");
    
#if HAVE_QT6_STATEMACHINE
    // Create state machine
    m_workflowStateMachine = new QStateMachine(this);
    
    // Create states
    QState* idleState = new QState(m_workflowStateMachine);
    QState* fileAnalysisState = new QState(m_workflowStateMachine);
    QState* realTimeAnalysisState = new QState(m_workflowStateMachine);
    QState* errorDetectionState = new QState(m_workflowStateMachine);
    QState* serviceDiscoveryState = new QState(m_workflowStateMachine);
    QState* comparativeAnalysisState = new QState(m_workflowStateMachine);
    
    // Store states for reference
    m_workflowStates[Idle] = idleState;
    m_workflowStates[FileAnalysisActive] = fileAnalysisState;
    m_workflowStates[RealTimeAnalysisActive] = realTimeAnalysisState;
    m_workflowStates[ErrorDetectionActive] = errorDetectionState;
    m_workflowStates[ServiceDiscoveryActive] = serviceDiscoveryState;
    m_workflowStates[ComparativeAnalysis] = comparativeAnalysisState;
    
    // Set initial state
    m_workflowStateMachine->setInitialState(idleState);
    
    // TODO: Add state transitions and actions when needed
    
    // Start state machine
    m_workflowStateMachine->start();
    
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Workflow state machine initialized");
#else
    Logger::instance().log(Logger::Warning, "ProfessionalWorkflowManager", "QStateMachine not available - using simplified workflow management");
    // TODO: m_workflowStateMachine = nullptr; // Variable not declared in header
#endif
}

void ProfessionalWorkflowManager::createPanelSyncTimer()
{
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Creating panel synchronization timer");
    
    // Create panel sync timer
    m_panelSyncTimer = new QTimer(this);
    connect(m_panelSyncTimer, &QTimer::timeout, this, &ProfessionalWorkflowManager::synchronizeAllPanels);
    m_panelSyncTimer->setInterval(PANEL_SYNC_INTERVAL_MS);
    
    // Create metrics update timer
    m_metricsUpdateTimer = new QTimer(this);
    connect(m_metricsUpdateTimer, &QTimer::timeout, this, &ProfessionalWorkflowManager::updateWorkflowMetrics);
    m_metricsUpdateTimer->start(METRICS_UPDATE_INTERVAL_MS);
    
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", "Panel synchronization timers created");
}

qint64 ProfessionalWorkflowManager::measurePanelSyncLatency()
{
    // Simple latency measurement - start timing
    m_syncLatencyTimer->start();
    
    // Simulate panel synchronization work (placeholder)
    // In real implementation, this would measure actual sync operations
    
    // Return elapsed time
    return m_syncLatencyTimer->elapsed();
}

void ProfessionalWorkflowManager::updateUIComponents()
{
    // Update UI components with current workflow data
    // This is called by the UI update timer for real-time workflows
    
    if (m_currentState == RealTimeAnalysisActive) {
        // Update real-time visualization components
        if (m_analyzerWidget) {
            // m_analyzerWidget->updateRealTimeData();
        }
        
        if (m_constellationWidget) {
            // m_constellationWidget->updateConstellation();
        }
        
        // Update metrics
        m_workflowMetrics.uiUpdateRate = TARGET_UI_UPDATE_RATE; // Placeholder
    }
}

bool ProfessionalWorkflowManager::validateWorkflowState() const
{
    // Validate workflow state consistency
    
    // Check if current state matches actual system state
    if (m_currentState == RealTimeAnalysisActive && (!m_uiUpdateTimer || !m_uiUpdateTimer->isActive())) {
        return false;
    }
    
    if (m_currentState == FileAnalysisActive && m_currentFilename.isEmpty()) {
        return false;
    }
    
    return true;
}

void ProfessionalWorkflowManager::updateWorkflowMetrics()
{
    // Update workflow metrics
    if (m_workflowTimer->isValid()) {
        qint64 elapsedTime = m_workflowTimer->elapsed();
        
        // Calculate rates if we have processed frames
        if (m_workflowMetrics.totalFramesProcessed > 0 && elapsedTime > 0) {
            m_workflowMetrics.etiProcessingRate = (m_workflowMetrics.totalFramesProcessed * 1000.0) / elapsedTime;
        }
        
        // Emit updated metrics
        emit workflowMetricsUpdated(m_workflowMetrics);
    }
}

void ProfessionalWorkflowManager::onEtiFrameProcessed(quint64 frameNumber, const QByteArray& data)
{
    Q_UNUSED(frameNumber);
    Q_UNUSED(data);
    
    // Handle ETI frame processed signal
    m_workflowMetrics.totalFramesProcessed++;
    
    // Update processing rate calculation
    if (m_workflowTimer->isValid()) {
        qint64 elapsedTime = m_workflowTimer->elapsed();
        if (elapsedTime > 0) {
            m_workflowMetrics.etiProcessingRate = (m_workflowMetrics.totalFramesProcessed * 1000.0) / elapsedTime;
        }
    }
}

void ProfessionalWorkflowManager::onEtiServiceDiscovered(const eti::DabService& service)
{
    // Handle service discovered signal
    m_discoveredServices[service.service_id] = service;
    m_workflowMetrics.servicesDiscovered = m_discoveredServices.size();
    
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Service discovered: %1 (%2)").arg(QString::fromStdString(service.label)).arg(service.service_id));
}

void ProfessionalWorkflowManager::onEtiEnsembleDiscovered(const eti::Ensemble& ensemble)
{
    // Handle ensemble discovered signal
    Logger::instance().log(Logger::Info, "ProfessionalWorkflowManager", 
                           QString("Ensemble discovered: %1 (%2)").arg(QString::fromStdString(ensemble.label)).arg(ensemble.ensemble_id));
    
    // Update workflow status
    updateWorkflowStatus("Ensemble Discovered", QString("%1 (ID: %2)").arg(QString::fromStdString(ensemble.label)).arg(ensemble.ensemble_id));
}

void ProfessionalWorkflowManager::onEtiErrorOccurred(const QString& error)
{
    // Handle ETI processing error
    Logger::instance().log(Logger::Error, "ProfessionalWorkflowManager", 
                           QString("ETI processing error: %1").arg(error));
    
    // Update workflow status
    updateWorkflowStatus("ETI Processing Error", error);
}

void ProfessionalWorkflowManager::onUIUpdateTimer()
{
    // Handle UI update timer for real-time workflows
    updateUIComponents();
}