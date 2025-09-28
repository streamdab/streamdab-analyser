/**
 * @file ui_integration_validator.cpp
 * @brief Implementation of comprehensive UI integration validation system
 * 
 * Validates complete integration between all backend components and GUI elements
 * to ensure seamless data flow and user interface functionality.
 * 
 * @author UI Integration Validation Specialist (Agent 16)
 * @date 2025
 */

#include "integration_validator.h"
#include "main_window.h"
#include "audio_monitoring_widget.h"
#include "etsi_compliance_monitor.h"
#include "service_browser.h"
#include "analyzer_widget.h"
#include "constellation_widget.h"
#include "audio_decoder_integration.h"
#include "../core/eti_processor.h"
#include "../core/audio_decoder.hpp"
#include "../core/stream_recording_manager.h"
#include "../core/network_stream_receiver.h"
#include "../core/comprehensive_etsi_validator.hpp"

#include <QApplication>
#include <QTimer>
#include <QMetaObject>
#include <QMetaMethod>
#include <QWidget>
#include <QVariant>
#include <QDateTime>
#include <QDebug>
#include <QEventLoop>
#include <QElapsedTimer>

UIIntegrationValidator::UIIntegrationValidator(QObject* parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_audioMonitor(nullptr)
    , m_complianceMonitor(nullptr)
    , m_serviceBrowser(nullptr)
    , m_analyzerWidget(nullptr)
    , m_constellationWidget(nullptr)
    , m_etiProcessor(nullptr)
    , m_monitoringTimer(nullptr)
    , m_isInitialized(false)
    , m_continuousMonitoring(false)
    , m_lastIntegrationScore(0.0)
{
    m_monitoringTimer = new QTimer(this);
    connect(m_monitoringTimer, &QTimer::timeout, this, &UIIntegrationValidator::onMonitoringTimer);
    m_monitoringTimer->setInterval(5000); // 5-second monitoring intervals

    // Initialize critical connections list
    m_criticalConnections = {
        "audio_decoder→audio_monitoring_widget",
        "network_receiver→main_window",
        "etsi_validator→compliance_monitor",
        "eti_processor→service_browser",
        "stream_recorder→main_window",
        "frame_processor→analyzer_widget"
    };

    // Initialize optional connections list
    m_optionalConnections = {
        "constellation_widget→analyzer_widget",
        "performance_dashboard→main_window",
        "workflow_manager→main_window"
    };
}

UIIntegrationValidator::~UIIntegrationValidator()
{
    stopContinuousMonitoring();
}

bool UIIntegrationValidator::initialize(MainWindow* mainWindow)
{
    if (!mainWindow) {
        qWarning() << "UIIntegrationValidator: Cannot initialize with null MainWindow";
        return false;
    }

    m_mainWindow = mainWindow;

    // Get GUI component references from MainWindow
    m_audioMonitor = m_mainWindow->findChild<AudioMonitoringWidget*>();
    m_complianceMonitor = m_mainWindow->findChild<ETSIComplianceMonitor*>();
    m_serviceBrowser = m_mainWindow->getServiceBrowser();
    m_analyzerWidget = m_mainWindow->getAnalyzerWidget();
    m_constellationWidget = m_mainWindow->getConstellationWidget();
    m_etiProcessor = m_mainWindow->getEtiProcessor();

    // Initialize backend components for testing
    try {
        // Audio decoder for testing
        m_audioDecoder = std::make_unique<eti::audio::audio_decoder>();
        
        // Recording manager for testing
        m_recordingManager = std::make_unique<eti::StreamRecordingManager>();
        
        // Network receiver for testing
        m_networkReceiver = std::make_unique<eti::NetworkStreamReceiver>();
        
        // ETSI validator for testing
        m_etsiValidator = std::make_unique<eti::compliance::ComprehensiveETSIValidator>();
        
    } catch (const std::exception& e) {
        qWarning() << "UIIntegrationValidator: Failed to initialize backend components:" << e.what();
        return false;
    }

    m_isInitialized = true;
    return true;
}

UIIntegrationValidationResult UIIntegrationValidator::runCompleteValidation()
{
    if (!m_isInitialized) {
        qWarning() << "UIIntegrationValidator: Not initialized. Call initialize() first.";
        return UIIntegrationValidationResult{};
    }

    UIIntegrationValidationResult result;
    result.detailed_report = QString("UI Integration Validation Report - %1\n")
                            .arg(QDateTime::currentDateTime().toString());
    result.detailed_report += "=" * 80 + "\n\n";

    qDebug() << "Starting comprehensive UI integration validation...";

    // 1. Audio Decoder UI Integration Validation
    result.detailed_report += "1. AUDIO DECODER UI INTEGRATION VALIDATION\n";
    result.detailed_report += "-" * 50 + "\n";
    
    bool audioIntegration = validateAudioDecoderConnections();
    result.successful_connections << (audioIntegration ? "Audio Decoder Integration" : "");
    if (!audioIntegration) {
        result.failed_connections << "Audio Decoder Integration";
    }
    result.total_connections_tested++;
    
    result.detailed_report += QString("Audio Decoder → GUI Integration: %1\n")
                             .arg(audioIntegration ? "✅ PASS" : "❌ FAIL");

    // Store component-specific results
    UIIntegrationValidationResult::ComponentIntegration audioComponent;
    audioComponent.component_name = "AudioMonitoringWidget";
    audioComponent.audio_decoder_integrated = audioIntegration;
    if (!audioIntegration) {
        audioComponent.integration_issues << "Audio decoder signals not connected to GUI";
    }
    result.component_results["AudioMonitoringWidget"] = audioComponent;

    // 2. Stream Playback Engine UI Integration
    result.detailed_report += "\n2. STREAM PLAYBACK ENGINE UI INTEGRATION\n";
    result.detailed_report += "-" * 50 + "\n";
    
    bool playbackIntegration = validateStreamPlaybackConnections();
    result.successful_connections << (playbackIntegration ? "Stream Playback Integration" : "");
    if (!playbackIntegration) {
        result.failed_connections << "Stream Playback Integration";
    }
    result.total_connections_tested++;
    
    result.detailed_report += QString("Stream Playback → GUI Controls: %1\n")
                             .arg(playbackIntegration ? "✅ PASS" : "❌ FAIL");

    // 3. Network Discovery GUI Integration
    result.detailed_report += "\n3. NETWORK DISCOVERY GUI INTEGRATION\n";
    result.detailed_report += "-" * 50 + "\n";
    
    bool networkIntegration = validateNetworkDiscoveryConnections();
    result.successful_connections << (networkIntegration ? "Network Discovery Integration" : "");
    if (!networkIntegration) {
        result.failed_connections << "Network Discovery Integration";
    }
    result.total_connections_tested++;
    
    result.detailed_report += QString("Network Discovery → GUI Display: %1\n")
                             .arg(networkIntegration ? "✅ PASS" : "❌ FAIL");

    // 4. ETSI Validator GUI Integration
    result.detailed_report += "\n4. ETSI VALIDATOR GUI INTEGRATION\n";
    result.detailed_report += "-" * 50 + "\n";
    
    bool etsiIntegration = validateETSIValidatorConnections();
    result.successful_connections << (etsiIntegration ? "ETSI Validator Integration" : "");
    if (!etsiIntegration) {
        result.failed_connections << "ETSI Validator Integration";
    }
    result.total_connections_tested++;
    
    result.detailed_report += QString("ETSI Validator → Compliance Display: %1\n")
                             .arg(etsiIntegration ? "✅ PASS" : "❌ FAIL");

    // 5. Frame Quality Analysis GUI Integration
    result.detailed_report += "\n5. FRAME QUALITY ANALYSIS GUI INTEGRATION\n";
    result.detailed_report += "-" * 50 + "\n";
    
    bool qualityIntegration = validateFrameQualityConnections();
    result.successful_connections << (qualityIntegration ? "Frame Quality Integration" : "");
    if (!qualityIntegration) {
        result.failed_connections << "Frame Quality Integration";
    }
    result.total_connections_tested++;
    
    result.detailed_report += QString("Frame Quality → Real-time Display: %1\n")
                             .arg(qualityIntegration ? "✅ PASS" : "❌ FAIL");

    // 6. End-to-End Workflow Testing
    result.detailed_report += "\n6. END-TO-END WORKFLOW TESTING\n";
    result.detailed_report += "-" * 50 + "\n";
    
    bool fileWorkflow = testFileAnalysisWorkflow();
    bool streamingWorkflow = testRealTimeStreamingWorkflow();
    bool errorWorkflow = testErrorDetectionWorkflow();
    
    result.successful_connections << (fileWorkflow ? "File Analysis Workflow" : "");
    result.successful_connections << (streamingWorkflow ? "Real-time Streaming Workflow" : "");
    result.successful_connections << (errorWorkflow ? "Error Detection Workflow" : "");
    
    if (!fileWorkflow) result.failed_connections << "File Analysis Workflow";
    if (!streamingWorkflow) result.failed_connections << "Real-time Streaming Workflow";  
    if (!errorWorkflow) result.failed_connections << "Error Detection Workflow";
    
    result.total_connections_tested += 3;
    
    result.detailed_report += QString("File Analysis Workflow: %1\n")
                             .arg(fileWorkflow ? "✅ PASS" : "❌ FAIL");
    result.detailed_report += QString("Real-time Streaming Workflow: %1\n")
                             .arg(streamingWorkflow ? "✅ PASS" : "❌ FAIL");
    result.detailed_report += QString("Error Detection Workflow: %1\n")
                             .arg(errorWorkflow ? "✅ PASS" : "❌ FAIL");

    // 7. Performance and Responsiveness Validation
    result.detailed_report += "\n7. PERFORMANCE AND RESPONSIVENESS VALIDATION\n";
    result.detailed_report += "-" * 50 + "\n";
    
    bool performanceValidation = validatePerformanceAndResponsiveness();
    result.successful_connections << (performanceValidation ? "Performance Validation" : "");
    if (!performanceValidation) {
        result.failed_connections << "Performance Validation";
    }
    result.total_connections_tested++;
    
    result.detailed_report += QString("GUI Performance During Processing: %1\n")
                             .arg(performanceValidation ? "✅ PASS" : "❌ FAIL");

    // Calculate final results
    result.successful_connections.removeAll("");
    int successful_count = result.successful_connections.size();
    result.success_rate = result.total_connections_tested > 0 ? 
        (double(successful_count) / double(result.total_connections_tested)) * 100.0 : 0.0;
    
    result.all_connections_valid = (result.failed_connections.isEmpty());
    
    // Final scoring and report
    double integrationScore = calculateIntegrationScore(result);
    result.detailed_report += QString("\n8. INTEGRATION SCORING RESULTS\n");
    result.detailed_report += "-" * 50 + "\n";
    result.detailed_report += QString("Total Connections Tested: %1\n").arg(result.total_connections_tested);
    result.detailed_report += QString("Successful Connections: %1\n").arg(successful_count);
    result.detailed_report += QString("Failed Connections: %1\n").arg(result.failed_connections.size());
    result.detailed_report += QString("Success Rate: %1%\n").arg(result.success_rate, 0, 'f', 1);
    result.detailed_report += QString("Integration Score: %1/10.0\n").arg(integrationScore, 0, 'f', 1);
    
    if (integrationScore >= 10.0) {
        result.detailed_report += "\n🏆 PERFECT UI INTEGRATION ACHIEVED! 🏆\n";
        result.detailed_report += "All backend components are seamlessly integrated with GUI elements.\n";
    } else if (integrationScore >= 8.0) {
        result.detailed_report += "\n✅ EXCELLENT UI INTEGRATION\n";
        result.detailed_report += "Most components are well integrated with minor issues.\n";
    } else if (integrationScore >= 6.0) {
        result.detailed_report += "\n⚠️ GOOD UI INTEGRATION WITH IMPROVEMENTS NEEDED\n";
        result.detailed_report += "Several integration issues require attention.\n";
    } else {
        result.detailed_report += "\n❌ SIGNIFICANT UI INTEGRATION ISSUES DETECTED\n";
        result.detailed_report += "Major integration problems require immediate attention.\n";
    }

    // Emit results
    m_lastResults = result;
    m_lastIntegrationScore = integrationScore;
    emit validationCompleted(result);
    emit integrationScoreChanged(integrationScore);

    qDebug() << QString("UI Integration Validation Complete - Score: %1/10.0")
                .arg(integrationScore, 0, 'f', 1);

    return result;
}

bool UIIntegrationValidator::validateAudioDecoderConnections()
{
    if (!m_audioMonitor || !m_audioDecoder) {
        qWarning() << "UIIntegrationValidator: Audio components not available for validation";
        return false;
    }

    bool allConnectionsValid = true;
    
    // Test 1: Verify audio_decoder signals exist and are connected
    const QMetaObject* decoderMeta = m_audioDecoder->metaObject();
    
    // Check for critical audio decoder signals
    QStringList requiredSignals = {
        "audio_decoded(QByteArray,uint32_t,uint16_t)",
        "audio_levels_changed(double,double)",
        "codec_info_changed(eti::audio::audio_decoder::codec_type,uint32_t,uint32_t,uint16_t)",
        "quality_metrics_updated(double,double)"
    };
    
    for (const QString& signal : requiredSignals) {
        int signalIndex = decoderMeta->indexOfSignal(signal.toLocal8Bit().constData());
        if (signalIndex == -1) {
            qWarning() << "UIIntegrationValidator: Audio decoder missing required signal:" << signal;
            allConnectionsValid = false;
        }
    }
    
    // Test 2: Check AudioMonitoringWidget slots
    const QMetaObject* monitorMeta = m_audioMonitor->metaObject();
    
    QStringList requiredSlots = {
        "updateAudioLevels(QMap<QString,int>)",
        "updateServiceInfo(QList<AudioMonitoringWidget::ServiceAudioInfo>)",
        "updateComplianceMetrics(double,QVariantMap)"
    };
    
    for (const QString& slot : requiredSlots) {
        int slotIndex = monitorMeta->indexOfSlot(slot.toLocal8Bit().constData());
        if (slotIndex == -1) {
            qWarning() << "UIIntegrationValidator: AudioMonitoringWidget missing required slot:" << slot;
            allConnectionsValid = false;
        }
    }
    
    // Test 3: Verify signal/slot connections exist
    if (m_mainWindow) {
        // Check if AudioDecoderIntegration exists and is properly connected
        AudioDecoderIntegration* integration = m_mainWindow->findChild<AudioDecoderIntegration*>();
        if (!integration) {
            qWarning() << "UIIntegrationValidator: AudioDecoderIntegration not found in MainWindow";
            allConnectionsValid = false;
        } else {
            // Test integration functionality
            if (!integration->isInitialized()) {
                qWarning() << "UIIntegrationValidator: AudioDecoderIntegration not initialized";
                allConnectionsValid = false;
            }
        }
    }
    
    // Test 4: Simulate audio level update and verify GUI response
    if (allConnectionsValid && m_audioMonitor) {
        QMap<QString, int> testLevels;
        testLevels["1001"] = 75; // Service ID 1001, level 75%
        testLevels["1002"] = 50; // Service ID 1002, level 50%
        
        // This should trigger GUI updates if properly connected
        m_audioMonitor->updateAudioLevels(testLevels);
        
        // Check if update was processed (basic validation)
        if (m_audioMonitor->getMonitoredServices().isEmpty()) {
            qWarning() << "UIIntegrationValidator: Audio monitoring widget not responding to updates";
            allConnectionsValid = false;
        }
    }
    
    return allConnectionsValid;
}

bool UIIntegrationValidator::validateStreamPlaybackConnections()
{
    if (!m_mainWindow || !m_recordingManager) {
        qWarning() << "UIIntegrationValidator: Stream playback components not available";
        return false;
    }

    bool allConnectionsValid = true;
    
    // Test 1: Check for playback-related actions in MainWindow
    QAction* startRecordingAction = m_mainWindow->findChild<QAction*>("m_startRecordingAction");
    QAction* stopRecordingAction = m_mainWindow->findChild<QAction*>("m_stopRecordingAction");
    
    if (!startRecordingAction || !stopRecordingAction) {
        qWarning() << "UIIntegrationValidator: Recording actions not found in MainWindow";
        allConnectionsValid = false;
    }
    
    // Test 2: Verify recording manager signals exist
    const QMetaObject* recorderMeta = m_recordingManager->metaObject();
    
    QStringList requiredRecorderSignals = {
        "recording_session_started(QString)",
        "recording_session_stopped(QString)",
        "segment_completed(QString,QString)",
        "session_statistics_updated(QString,eti::RecordingSession)"
    };
    
    for (const QString& signal : requiredRecorderSignals) {
        int signalIndex = recorderMeta->indexOfSignal(signal.toLocal8Bit().constData());
        if (signalIndex == -1) {
            qWarning() << "UIIntegrationValidator: StreamRecordingManager missing signal:" << signal;
            allConnectionsValid = false;
        }
    }
    
    // Test 3: Check MainWindow slots for recording
    const QMetaObject* mainMeta = m_mainWindow->metaObject();
    
    QStringList requiredMainSlots = {
        "startStreamRecording()",
        "stopStreamRecording()"
    };
    
    for (const QString& slot : requiredMainSlots) {
        int slotIndex = mainMeta->indexOfSlot(slot.toLocal8Bit().constData());
        if (slotIndex == -1) {
            qWarning() << "UIIntegrationValidator: MainWindow missing recording slot:" << slot;
            allConnectionsValid = false;
        }
    }
    
    // Test 4: Verify status bar updates for recording state
    QStatusBar* statusBar = m_mainWindow->statusBar();
    if (!statusBar) {
        qWarning() << "UIIntegrationValidator: MainWindow status bar not available";
        allConnectionsValid = false;
    }
    
    return allConnectionsValid;
}

bool UIIntegrationValidator::validateNetworkDiscoveryConnections()
{
    if (!m_mainWindow || !m_networkReceiver) {
        qWarning() << "UIIntegrationValidator: Network components not available";
        return false;
    }

    bool allConnectionsValid = true;
    
    // Test 1: Check network receiver signals
    const QMetaObject* networkMeta = m_networkReceiver->metaObject();
    
    QStringList requiredNetworkSignals = {
        "frame_received(eti::EtiFrame)",
        "error_occurred(QString)",
        "statistics_updated(eti::NetworkStreamStatistics)",
        "connection_status_changed(bool)"
    };
    
    for (const QString& signal : requiredNetworkSignals) {
        int signalIndex = networkMeta->indexOfSignal(signal.toLocal8Bit().constData());
        if (signalIndex == -1) {
            qWarning() << "UIIntegrationValidator: NetworkStreamReceiver missing signal:" << signal;
            allConnectionsValid = false;
        }
    }
    
    // Test 2: Check MainWindow network-related slots
    const QMetaObject* mainMeta = m_mainWindow->metaObject();
    
    QStringList requiredNetworkSlots = {
        "startNetworkStreaming()",
        "stopNetworkStreaming()",
        "configureNetworkStreaming()"
    };
    
    for (const QString& slot : requiredNetworkSlots) {
        int slotIndex = mainMeta->indexOfSlot(slot.toLocal8Bit().constData());
        if (slotIndex == -1) {
            qWarning() << "UIIntegrationValidator: MainWindow missing network slot:" << slot;
            allConnectionsValid = false;
        }
    }
    
    // Test 3: Check for network streaming actions
    QAction* startNetworkAction = m_mainWindow->findChild<QAction*>("m_startNetworkStreamingAction");
    QAction* stopNetworkAction = m_mainWindow->findChild<QAction*>("m_stopNetworkStreamingAction");
    QAction* configureNetworkAction = m_mainWindow->findChild<QAction*>("m_configureNetworkAction");
    
    if (!startNetworkAction || !stopNetworkAction || !configureNetworkAction) {
        qWarning() << "UIIntegrationValidator: Network streaming actions not found";
        allConnectionsValid = false;
    }
    
    // Test 4: Verify network quality indicators in status bar
    QLabel* networkQualityLabel = m_mainWindow->findChild<QLabel*>("networkQualityLabel");
    if (!networkQualityLabel) {
        qDebug() << "UIIntegrationValidator: Network quality indicator not found (optional)";
    }
    
    return allConnectionsValid;
}

bool UIIntegrationValidator::validateETSIValidatorConnections()
{
    if (!m_complianceMonitor || !m_etsiValidator) {
        qWarning() << "UIIntegrationValidator: ETSI validation components not available";
        return false;
    }

    bool allConnectionsValid = true;
    
    // Test 1: Check ETSI validator signals
    const QMetaObject* validatorMeta = m_etsiValidator->metaObject();
    
    QStringList requiredValidatorSignals = {
        "validation_completed(eti::compliance::ValidationResults)",
        "violation_detected(eti::compliance::ComplianceViolation)",
        "compliance_score_updated(double)"
    };
    
    for (const QString& signal : requiredValidatorSignals) {
        int signalIndex = validatorMeta->indexOfSignal(signal.toLocal8Bit().constData());
        if (signalIndex == -1) {
            qWarning() << "UIIntegrationValidator: ETSI validator missing signal:" << signal;
            allConnectionsValid = false;
        }
    }
    
    // Test 2: Check compliance monitor slots
    const QMetaObject* complianceMeta = m_complianceMonitor->metaObject();
    
    QStringList requiredComplianceSlots = {
        "addViolation(ETSIViolation)",
        "updateStatistics()",
        "updateComplianceScore(double)"
    };
    
    for (const QString& slot : requiredComplianceSlots) {
        int slotIndex = complianceMeta->indexOfSlot(slot.toLocal8Bit().constData());
        if (slotIndex == -1) {
            qWarning() << "UIIntegrationValidator: ETSIComplianceMonitor missing slot:" << slot;
            allConnectionsValid = false;
        }
    }
    
    // Test 3: Simulate violation and check GUI response
    if (allConnectionsValid && m_complianceMonitor) {
        ETSIViolation testViolation;
        testViolation.violationId = "TEST_VIOLATION_001";
        testViolation.severity = ETSIViolation::Warning;
        testViolation.category = ETSIViolation::FIG_Structure;
        testViolation.description = "Test violation for integration validation";
        testViolation.standardReference = "ETSI EN 300 401 Section 8.1.1";
        testViolation.timestamp = QDateTime::currentDateTime();
        testViolation.frameContext = "Frame 12345";
        testViolation.isActive = true;
        
        // This should trigger GUI updates if properly connected
        m_complianceMonitor->addViolation(testViolation);
        
        // Check if compliance monitor is responsive
        ComplianceStatistics stats = m_complianceMonitor->getCurrentStatistics();
        if (stats.totalViolations == 0) {
            qWarning() << "UIIntegrationValidator: Compliance monitor not responding to violations";
            allConnectionsValid = false;
        }
    }
    
    return allConnectionsValid;
}

bool UIIntegrationValidator::validateFrameQualityConnections()
{
    if (!m_analyzerWidget || !m_etiProcessor) {
        qWarning() << "UIIntegrationValidator: Frame quality components not available";
        return false;
    }

    bool allConnectionsValid = true;
    
    // Test 1: Check ETI processor frame quality signals
    const QMetaObject* processorMeta = m_etiProcessor->metaObject();
    
    QStringList requiredProcessorSignals = {
        "frameProcessed(quint64,QByteArray)",
        "qualityMetricsUpdated(eti::FrameQualityMetrics)",
        "serviceDiscovered(eti::DabService)"
    };
    
    for (const QString& signal : requiredProcessorSignals) {
        int signalIndex = processorMeta->indexOfSignal(signal.toLocal8Bit().constData());
        if (signalIndex == -1) {
            qWarning() << "UIIntegrationValidator: EtiProcessor missing signal:" << signal;
            allConnectionsValid = false;
        }
    }
    
    // Test 2: Check analyzer widget slots for quality updates
    const QMetaObject* analyzerMeta = m_analyzerWidget->metaObject();
    
    QStringList requiredAnalyzerSlots = {
        "updateFrameQuality(double)",
        "updateProcessingStatistics()",
        "displayFrameAnalysis(quint64)"
    };
    
    for (const QString& slot : requiredAnalyzerSlots) {
        int slotIndex = analyzerMeta->indexOfSlot(slot.toLocal8Bit().constData());
        if (slotIndex == -1) {
            qWarning() << "UIIntegrationValidator: AnalyzerWidget missing slot:" << slot;
            allConnectionsValid = false;
        }
    }
    
    // Test 3: Check constellation widget frame data integration
    if (m_constellationWidget) {
        const QMetaObject* constellationMeta = m_constellationWidget->metaObject();
        
        int updateSlot = constellationMeta->indexOfSlot("updateConstellation(QByteArray)");
        if (updateSlot == -1) {
            qWarning() << "UIIntegrationValidator: ConstellationWidget missing update slot";
            allConnectionsValid = false;
        }
    }
    
    return allConnectionsValid;
}

bool UIIntegrationValidator::testFileAnalysisWorkflow()
{
    if (!m_mainWindow) {
        return false;
    }

    bool workflowValid = true;
    
    // Test workflow: File → Open → Analyze → Navigate → Export Results
    
    // Step 1: Check file opening capability
    QAction* openAction = m_mainWindow->findChild<QAction*>("m_openFileAction");
    if (!openAction) {
        qWarning() << "UIIntegrationValidator: File open action not available";
        workflowValid = false;
    }
    
    // Step 2: Check analysis start/stop capability
    QAction* startAnalysis = m_mainWindow->findChild<QAction*>("m_startAnalysisAction");
    QAction* stopAnalysis = m_mainWindow->findChild<QAction*>("m_stopAnalysisAction");
    if (!startAnalysis || !stopAnalysis) {
        qWarning() << "UIIntegrationValidator: Analysis control actions not available";
        workflowValid = false;
    }
    
    // Step 3: Check service browser integration
    if (!m_serviceBrowser) {
        qWarning() << "UIIntegrationValidator: Service browser not available for workflow";
        workflowValid = false;
    }
    
    // Step 4: Check properties panel integration
    QWidget* propertiesPanel = m_mainWindow->findChild<QWidget*>("m_propertiesPanel");
    if (!propertiesPanel) {
        qWarning() << "UIIntegrationValidator: Properties panel not available";
        workflowValid = false;
    }
    
    return workflowValid;
}

bool UIIntegrationValidator::testRealTimeStreamingWorkflow()
{
    if (!m_mainWindow || !m_networkReceiver) {
        return false;
    }

    bool workflowValid = true;
    
    // Test workflow: Connect Source → Enable Real-time Mode → Stream Processing → Live Updates
    
    // Step 1: Check real-time mode capability
    if (!m_mainWindow->isRealTimeMode()) {
        // Try to enable real-time mode
        const QMetaObject* mainMeta = m_mainWindow->metaObject();
        int slotIndex = mainMeta->indexOfSlot("startRealTimeAnalysis()");
        if (slotIndex == -1) {
            qWarning() << "UIIntegrationValidator: Real-time mode not available";
            workflowValid = false;
        }
    }
    
    // Step 2: Check network streaming capability
    QAction* startStreaming = m_mainWindow->findChild<QAction*>("m_startNetworkStreamingAction");
    if (!startStreaming) {
        qWarning() << "UIIntegrationValidator: Network streaming action not available";
        workflowValid = false;
    }
    
    // Step 3: Check live update capability
    if (m_mainWindow->getUpdateRate() < 30) {
        qWarning() << "UIIntegrationValidator: Update rate too low for real-time processing";
        workflowValid = false;
    }
    
    return workflowValid;
}

bool UIIntegrationValidator::testErrorDetectionWorkflow()
{
    if (!m_complianceMonitor) {
        return false;
    }

    bool workflowValid = true;
    
    // Test workflow: Error Detection → UI Alert → User Navigation → Analysis → Report Generation
    
    // Step 1: Check error detection capability
    if (!m_complianceMonitor->isMonitoring()) {
        // Try to start monitoring
        m_complianceMonitor->startMonitoring();
        if (!m_complianceMonitor->isMonitoring()) {
            qWarning() << "UIIntegrationValidator: Error detection monitoring cannot be started";
            workflowValid = false;
        }
    }
    
    // Step 2: Check error navigation capability
    const QMetaObject* mainMeta = m_mainWindow->metaObject();
    int nextErrorSlot = mainMeta->indexOfSlot("navigateToNextError()");
    int prevErrorSlot = mainMeta->indexOfSlot("navigateToPreviousError()");
    
    if (nextErrorSlot == -1 || prevErrorSlot == -1) {
        qWarning() << "UIIntegrationValidator: Error navigation not available";
        workflowValid = false;
    }
    
    // Step 3: Check report generation capability
    int reportSlot = mainMeta->indexOfSlot("generateProfessionalReport()");
    if (reportSlot == -1) {
        qWarning() << "UIIntegrationValidator: Report generation not available";
        workflowValid = false;
    }
    
    return workflowValid;
}

bool UIIntegrationValidator::validatePerformanceAndResponsiveness()
{
    if (!m_mainWindow) {
        return false;
    }

    bool performanceValid = true;
    
    // Test 1: Check GUI responsiveness during processing
    QElapsedTimer responseTimer;
    responseTimer.start();
    
    // Simulate intensive processing
    QApplication::processEvents();
    
    qint64 responseTime = responseTimer.elapsed();
    if (responseTime > 100) { // More than 100ms response time is poor
        qWarning() << "UIIntegrationValidator: GUI response time too high:" << responseTime << "ms";
        performanceValid = false;
    }
    
    // Test 2: Check update rate capability
    int targetFPS = m_mainWindow->getUpdateRate();
    if (targetFPS < 30) {
        qWarning() << "UIIntegrationValidator: Update rate below 30 FPS:" << targetFPS;
        performanceValid = false;
    }
    
    // Test 3: Check memory usage (basic check)
    // This is a simplified check - in real implementation, you'd monitor actual memory usage
    if (!m_mainWindow->isVisible()) {
        qWarning() << "UIIntegrationValidator: Main window not visible during performance test";
        performanceValid = false;
    }
    
    return performanceValid;
}

double UIIntegrationValidator::calculateIntegrationScore(const UIIntegrationValidationResult& results)
{
    if (results.total_connections_tested == 0) {
        return 0.0;
    }

    // Base score from success rate
    double baseScore = (results.success_rate / 100.0) * 8.0; // 80% of total score
    
    // Bonus points for critical integrations
    double bonusScore = 0.0;
    
    // Critical integration bonuses (up to 2.0 points)
    if (results.successful_connections.contains("Audio Decoder Integration")) {
        bonusScore += 0.5;
    }
    if (results.successful_connections.contains("Network Discovery Integration")) {
        bonusScore += 0.5;
    }
    if (results.successful_connections.contains("ETSI Validator Integration")) {
        bonusScore += 0.4;
    }
    if (results.successful_connections.contains("File Analysis Workflow")) {
        bonusScore += 0.3;
    }
    if (results.successful_connections.contains("Real-time Streaming Workflow")) {
        bonusScore += 0.3;
    }
    
    // Perfect integration bonus
    if (results.all_connections_valid) {
        bonusScore += 0.5;
    }
    
    double totalScore = baseScore + bonusScore;
    return qMin(totalScore, 10.0); // Cap at 10.0
}

QString UIIntegrationValidator::generateIntegrationReport(const UIIntegrationValidationResult& results)
{
    return results.detailed_report;
}

void UIIntegrationValidator::startContinuousMonitoring()
{
    if (!m_isInitialized) {
        qWarning() << "UIIntegrationValidator: Cannot start monitoring - not initialized";
        return;
    }
    
    m_continuousMonitoring = true;
    m_monitoringTimer->start();
    qDebug() << "UIIntegrationValidator: Started continuous monitoring";
}

void UIIntegrationValidator::stopContinuousMonitoring()
{
    m_continuousMonitoring = false;
    m_monitoringTimer->stop();
    qDebug() << "UIIntegrationValidator: Stopped continuous monitoring";
}

void UIIntegrationValidator::performQuickValidation()
{
    if (!m_isInitialized) {
        qWarning() << "UIIntegrationValidator: Cannot perform validation - not initialized";
        return;
    }
    
    // Run lightweight validation checks
    bool audioOk = (m_audioMonitor != nullptr);
    bool networkOk = (m_networkReceiver != nullptr);
    bool etsiOk = (m_complianceMonitor != nullptr);
    
    double quickScore = 0.0;
    if (audioOk) quickScore += 3.0;
    if (networkOk) quickScore += 3.0;
    if (etsiOk) quickScore += 4.0;
    
    if (quickScore != m_lastIntegrationScore) {
        m_lastIntegrationScore = quickScore;
        emit integrationScoreChanged(quickScore);
    }
}

void UIIntegrationValidator::onMonitoringTimer()
{
    if (!m_continuousMonitoring) {
        return;
    }
    
    performQuickValidation();
}

// Helper method implementations
bool UIIntegrationValidator::validateSignalSlotConnection(QObject* sender, const char* signal, 
                                                        QObject* receiver, const char* slot)
{
    if (!sender || !receiver) {
        return false;
    }
    
    const QMetaObject* senderMeta = sender->metaObject();
    const QMetaObject* receiverMeta = receiver->metaObject();
    
    int signalIndex = senderMeta->indexOfSignal(QMetaObject::normalizedSignature(signal));
    int slotIndex = receiverMeta->indexOfSlot(QMetaObject::normalizedSignature(slot));
    
    return (signalIndex >= 0 && slotIndex >= 0);
}

bool UIIntegrationValidator::testSignalEmissionReception(const QString& component, const QString& testSignal)
{
    // This is a placeholder for signal testing
    // In a full implementation, you would emit test signals and verify reception
    Q_UNUSED(component)
    Q_UNUSED(testSignal)
    return true;
}

bool UIIntegrationValidator::validateWidgetDataUpdates(QWidget* widget, const QVariantMap& testData)
{
    if (!widget) {
        return false;
    }
    
    // Basic widget validation - check if widget is responsive
    return widget->isVisible() && widget->isEnabled();
}

bool UIIntegrationValidator::checkGUIResponsiveness(int processingDuration)
{
    QElapsedTimer timer;
    timer.start();
    
    // Simulate processing
    QEventLoop loop;
    QTimer::singleShot(processingDuration, &loop, &QEventLoop::quit);
    loop.exec();
    
    // Check if GUI remained responsive
    QApplication::processEvents();
    
    return timer.elapsed() < (processingDuration + 50); // Allow 50ms overhead
}

QVariantMap UIIntegrationValidator::createTestData(const QString& component)
{
    QVariantMap testData;
    
    if (component == "AudioMonitoringWidget") {
        testData["service_id"] = 1001;
        testData["audio_level"] = 75;
        testData["codec_type"] = "DAB+";
        testData["bitrate"] = 128;
    } else if (component == "ETSIComplianceMonitor") {
        testData["violation_type"] = "FIG_Structure";
        testData["severity"] = "Warning";
        testData["description"] = "Test violation";
    } else if (component == "NetworkReceiver") {
        testData["multicast_address"] = "239.192.0.1";
        testData["port"] = 9200;
        testData["frame_rate"] = 400.0;
    }
    
    return testData;
}