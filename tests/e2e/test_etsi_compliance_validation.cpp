/**
 * @file test_etsi_compliance_validation.cpp
 * @brief ETSI Compliance Testing for Phase 5.3 Professional UI/UX Validation
 * 
 * Tests ETSI compliance integration with the professional UI:
 * - ETI frame processing (6144-byte frames, 24ms precision)
 * - FIG analysis validation against ETSI standards
 * - Service tree model accuracy for DAB hierarchy
 * - Error detection and professional alert system
 * - Broadcasting industry standards compliance
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include <QTest>
#include <QTimer>
#include <QSignalSpy>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QDir>
#include <QByteArray>
#include <QRandomGenerator>
#include <memory>

// Core includes
#include "gui/main_window.h"
#include "gui/analyser_widget.h"
#include "gui/service_browser.h"
#include "gui/constellation_widget.h"
#include "gui/eti_service_tree_model.h"
#include "gui/eti_frame_list_model.h"
#include "gui/fig_analysis_widget.h"
#include "core/eti_processor.hpp"
#include "core/etsi/compliance_engine.h"
#include "core/etsi/alert_system.h"
#include "core/etsi/standard_validators.h"
#include "tests/fixtures/test_data_generators.h"
#include "tests/fixtures/etsi_test_data/etsi_reference_data.h"
#include "tests/mocks/mock_eti_processor.h"

using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::InSequence;

/**
 * @class ETSIComplianceValidationTest
 * @brief Comprehensive ETSI compliance testing with professional UI integration
 */
class ETSIComplianceValidationTest : public ::testing::Test
{
protected:
    void SetUp() override {
        // Initialize Qt application for GUI testing
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = std::make_unique<QApplication>(argc, argv);
        }

        // Create main window
        mainWindow = std::make_unique<MainWindow>();
        ASSERT_TRUE(mainWindow->initialize());

        // Setup mock ETI processor
        mockEtiProcessor = std::make_unique<MockEtiProcessor>();
        
        // Create test data generators
        testDataGenerator = std::make_unique<TestDataGenerators>();
        etsiTestData = std::make_unique<ETSIReferenceData>();
        
        // Initialize compliance monitoring
        complianceMonitor = std::make_unique<ComplianceMonitor>();
        
        // Show main window for testing
        mainWindow->show();
        QTest::qWaitForWindowExposed(mainWindow.get());
        
        // Allow UI to stabilize
        QTest::qWait(200);
    }

    void TearDown() override {
        if (mainWindow) {
            mainWindow->close();
            mainWindow.reset();
        }
        mockEtiProcessor.reset();
        testDataGenerator.reset();
        etsiTestData.reset();
        complianceMonitor.reset();
    }

    /**
     * @brief ETSI compliance monitoring utility
     */
    class ComplianceMonitor {
    public:
        struct ComplianceResult {
            QString standard;
            QString section;
            bool compliant;
            QString issue;
            QString severity;
            QDateTime timestamp;
            
            ComplianceResult() : compliant(false) {}
        };
        
        void recordCompliance(const QString& standard, const QString& section, 
                            bool compliant, const QString& issue = QString(), 
                            const QString& severity = "INFO") {
            ComplianceResult result;
            result.standard = standard;
            result.section = section;
            result.compliant = compliant;
            result.issue = issue;
            result.severity = severity;
            result.timestamp = QDateTime::currentDateTime();
            
            results.append(result);
            
            if (compliant) {
                passedTests++;
            } else {
                failedTests++;
                if (severity == "CRITICAL") criticalIssues++;
                else if (severity == "WARNING") warningIssues++;
            }
        }
        
        double getComplianceScore() const {
            int totalTests = passedTests + failedTests;
            if (totalTests == 0) return 0.0;
            return (double)passedTests / totalTests * 100.0;
        }
        
        QList<ComplianceResult> getResults() const { return results; }
        int getPassedTests() const { return passedTests; }
        int getFailedTests() const { return failedTests; }
        int getCriticalIssues() const { return criticalIssues; }
        int getWarningIssues() const { return warningIssues; }
        
        void reset() {
            results.clear();
            passedTests = 0;
            failedTests = 0;
            criticalIssues = 0;
            warningIssues = 0;
        }
        
    private:
        QList<ComplianceResult> results;
        int passedTests = 0;
        int failedTests = 0;
        int criticalIssues = 0;
        int warningIssues = 0;
    };

    /**
     * @brief Generate ETSI-compliant ETI frame
     */
    QByteArray generateETSICompliantFrame() {
        QByteArray frame;
        frame.resize(6144); // Standard ETI frame size
        
        // ETI Frame Header (ETSI EN 300 799)
        frame[0] = 0xFF; // SYNC
        frame[1] = 0x1F; // ERR/FIC_LEN
        frame[2] = 0x05; // FICF/NST
        frame[3] = 0x00; // FP
        
        // Add FIC data (Fast Information Channel)
        int ficStart = 4;
        for (int i = 0; i < 96; ++i) { // 96 bytes FIC
            frame[ficStart + i] = etsiTestData->generateFICByte(i);
        }
        
        // Add subchannel data
        int subchannelStart = 100;
        QByteArray subchannelData = etsiTestData->generateSubchannelData();
        for (int i = 0; i < subchannelData.size() && (subchannelStart + i) < 6144; ++i) {
            frame[subchannelStart + i] = subchannelData[i];
        }
        
        // Add CRC (last 4 bytes)
        quint32 crc = etsiTestData->calculateCRC(frame.mid(0, 6140));
        frame[6140] = (crc >> 24) & 0xFF;
        frame[6141] = (crc >> 16) & 0xFF;
        frame[6142] = (crc >> 8) & 0xFF;
        frame[6143] = crc & 0xFF;
        
        return frame;
    }

    /**
     * @brief Generate ETI frame with ETSI compliance issues
     */
    QByteArray generateNonCompliantFrame() {
        QByteArray frame = generateETSICompliantFrame();
        
        // Introduce compliance issues
        frame[1] = 0x00; // Invalid ERR/FIC_LEN
        frame[2] = 0xFF; // Invalid FICF/NST
        
        // Corrupt FIC data
        for (int i = 4; i < 20; ++i) {
            frame[i] = 0x00; // Invalid FIC data
        }
        
        // Invalid CRC
        frame[6140] = 0x00;
        frame[6141] = 0x00;
        frame[6142] = 0x00;
        frame[6143] = 0x00;
        
        return frame;
    }

    /**
     * @brief Validate ETI frame structure against ETSI EN 300 799
     */
    bool validateETIFrameStructure(const QByteArray& frame) {
        if (frame.size() != 6144) {
            complianceMonitor->recordCompliance("ETSI EN 300 799", "Frame Size", false, 
                                              QString("Frame size %1 != 6144 bytes").arg(frame.size()), "CRITICAL");
            return false;
        }
        
        // Check SYNC pattern
        if ((quint8)frame[0] != 0xFF) {
            complianceMonitor->recordCompliance("ETSI EN 300 799", "SYNC Pattern", false, 
                                              QString("Invalid SYNC byte: 0x%1").arg((quint8)frame[0], 2, 16, QChar('0')), "CRITICAL");
            return false;
        }
        
        complianceMonitor->recordCompliance("ETSI EN 300 799", "Frame Size", true);
        complianceMonitor->recordCompliance("ETSI EN 300 799", "SYNC Pattern", true);
        
        // Validate FIC length
        quint8 ficLen = frame[1] & 0x1F;
        if (ficLen > 30) {
            complianceMonitor->recordCompliance("ETSI EN 300 799", "FIC Length", false, 
                                              QString("FIC length %1 > 30").arg(ficLen), "WARNING");
            return false;
        }
        
        complianceMonitor->recordCompliance("ETSI EN 300 799", "FIC Length", true);
        
        // Validate NST (Number of SubchannelS in Transport)
        quint8 nst = frame[2] & 0x3F;
        if (nst > 64) {
            complianceMonitor->recordCompliance("ETSI EN 300 799", "NST Value", false, 
                                              QString("NST %1 > 64").arg(nst), "WARNING");
            return false;
        }
        
        complianceMonitor->recordCompliance("ETSI EN 300 799", "NST Value", true);
        
        return true;
    }

    /**
     * @brief Validate FIG (Fast Information Group) compliance
     */
    bool validateFIGCompliance(quint8 figType, const QVariantMap& figData) {
        QString figStandard = QString("ETSI EN 300 401 FIG %1").arg(figType);
        
        switch (figType) {
            case 0: // FIG 0 - MCI and basic SI
                if (!figData.contains("extension")) {
                    complianceMonitor->recordCompliance(figStandard, "Extension Field", false, 
                                                      "Missing extension field", "CRITICAL");
                    return false;
                }
                complianceMonitor->recordCompliance(figStandard, "Extension Field", true);
                break;
                
            case 1: // FIG 1 - SI labels
                if (!figData.contains("charset") || !figData.contains("label")) {
                    complianceMonitor->recordCompliance(figStandard, "Label Data", false, 
                                                      "Missing charset or label field", "CRITICAL");
                    return false;
                }
                complianceMonitor->recordCompliance(figStandard, "Label Data", true);
                break;
                
            case 2: // FIG 2 - Dynamic labels
                if (!figData.contains("segment")) {
                    complianceMonitor->recordCompliance(figStandard, "Segment Field", false, 
                                                      "Missing segment field", "WARNING");
                    return false;
                }
                complianceMonitor->recordCompliance(figStandard, "Segment Field", true);
                break;
                
            default:
                complianceMonitor->recordCompliance(figStandard, "Unknown FIG", true, 
                                                  "FIG type handled", "INFO");
                break;
        }
        
        return true;
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<MockEtiProcessor> mockEtiProcessor;
    std::unique_ptr<TestDataGenerators> testDataGenerator;
    std::unique_ptr<ETSIReferenceData> etsiTestData;
    std::unique_ptr<ComplianceMonitor> complianceMonitor;
};

/**
 * @brief Test Case 1: ETI Frame Processing Compliance (6144-byte frames, 24ms precision)
 */
TEST_F(ETSIComplianceValidationTest, ETIFrameProcessingCompliance) {
    ASSERT_TRUE(mainWindow);
    
    // Test ETSI EN 300 799 compliance
    
    // Test 1: Compliant ETI frame processing
    QByteArray compliantFrame = generateETSICompliantFrame();
    EXPECT_EQ(compliantFrame.size(), 6144) << "ETI frame must be exactly 6144 bytes";
    EXPECT_TRUE(validateETIFrameStructure(compliantFrame)) << "Compliant frame should pass validation";
    
    // Test 2: Non-compliant frame detection
    QByteArray nonCompliantFrame = generateNonCompliantFrame();
    EXPECT_FALSE(validateETIFrameStructure(nonCompliantFrame)) << "Non-compliant frame should be detected";
    
    // Test 3: Frame timing precision (24ms)
    auto* analyserWidget = mainWindow->getAnalyserWidget();
    ASSERT_TRUE(analyserWidget);
    
    auto* frameTable = analyserWidget->findChild<QTableWidget*>("frameListTable");
    if (frameTable) {
        // Simulate frame processing with 24ms timing
        QElapsedTimer frameTiming;
        std::vector<qint64> frameIntervals;
        
        for (int i = 0; i < 50; ++i) {
            frameTiming.start();
            
            // Process compliant frame
            testDataGenerator->processETIFrame(compliantFrame);
            QApplication::processEvents();
            
            qint64 processingTime = frameTiming.elapsed();
            frameIntervals.push_back(processingTime);
            
            // Verify processing time is reasonable for real-time (should be much less than 24ms)
            EXPECT_LT(processingTime, 20) << QString("Frame processing should be <20ms for real-time, got %1ms").arg(processingTime);
            
            QTest::qWait(1); // Small delay between frames
        }
        
        // Calculate average processing time
        double avgProcessingTime = std::accumulate(frameIntervals.begin(), frameIntervals.end(), 0.0) / frameIntervals.size();
        EXPECT_LT(avgProcessingTime, 10.0) << QString("Average frame processing should be <10ms, got %1ms").arg(avgProcessingTime);
        
        // Update frame list
        testDataGenerator->populateFrameList(frameTable);
        QTest::qWait(200);
        
        EXPECT_GT(frameTable->rowCount(), 0) << "Frame table should contain processed frames";
    }
    
    // Test 4: Frame rate compliance (>900 fps capability)
    QElapsedTimer fpsTest;
    fpsTest.start();
    
    int frameCount = 0;
    while (fpsTest.elapsed() < 1000 && frameCount < 1000) { // 1 second test
        testDataGenerator->processETIFrame(compliantFrame);
        frameCount++;
        
        if (frameCount % 100 == 0) {
            QApplication::processEvents();
        }
    }
    
    qint64 testDuration = fpsTest.elapsed();
    double achievedFps = (double)frameCount * 1000.0 / testDuration;
    
    EXPECT_GE(achievedFps, 900.0) << QString("Should achieve >900 FPS, got %1 FPS").arg(achievedFps);
    
    complianceMonitor->recordCompliance("ETSI EN 300 799", "Frame Rate", achievedFps >= 900.0, 
                                      QString("Achieved %1 FPS").arg(achievedFps));
}

/**
 * @brief Test Case 2: FIG Analysis Validation Against ETSI Standards
 */
TEST_F(ETSIComplianceValidationTest, FIGAnalysisValidation) {
    ASSERT_TRUE(mainWindow);
    
    // Get FIG analysis widget
    auto* figAnalysisWidget = mainWindow->findChild<FigAnalysisWidget*>();
    if (!figAnalysisWidget) {
        // FIG widget might be in bottom tabs
        auto* bottomTabs = mainWindow->findChild<QTabWidget*>("bottomToolTabs");
        if (bottomTabs) {
            for (int i = 0; i < bottomTabs->count(); ++i) {
                if (bottomTabs->tabText(i).contains("FIG", Qt::CaseInsensitive)) {
                    bottomTabs->setCurrentIndex(i);
                    figAnalysisWidget = bottomTabs->widget(i)->findChild<FigAnalysisWidget*>();
                    break;
                }
            }
        }
    }
    
    if (figAnalysisWidget) {
        // Test FIG 0 compliance (ETSI EN 300 401)
        QVariantMap fig0Data;
        fig0Data["extension"] = 0;
        fig0Data["ensembleId"] = 0x1234;
        fig0Data["ensembleName"] = "Test Ensemble";
        
        FigAnalysisWidget::FigAnalysis fig0Analysis;
        fig0Analysis.figType = 0;
        fig0Analysis.figExtension = 0;
        fig0Analysis.figName = "MCI and SI";
        fig0Analysis.figData = fig0Data;
        fig0Analysis.complianceStatus = FigAnalysisWidget::ComplianceStatus::Compliant;
        
        figAnalysisWidget->updateFigData(fig0Analysis);
        QTest::qWait(100);
        
        EXPECT_TRUE(validateFIGCompliance(0, fig0Data)) << "FIG 0 should be compliant";
        
        // Test FIG 1 compliance (Service labels)
        QVariantMap fig1Data;
        fig1Data["charset"] = 0; // EBU Latin
        fig1Data["label"] = "Test Service";
        fig1Data["serviceId"] = 0x5678;
        
        FigAnalysisWidget::FigAnalysis fig1Analysis;
        fig1Analysis.figType = 1;
        fig1Analysis.figExtension = 0;
        fig1Analysis.figName = "Service Labels";
        fig1Analysis.figData = fig1Data;
        fig1Analysis.complianceStatus = FigAnalysisWidget::ComplianceStatus::Compliant;
        
        figAnalysisWidget->updateFigData(fig1Analysis);
        QTest::qWait(100);
        
        EXPECT_TRUE(validateFIGCompliance(1, fig1Data)) << "FIG 1 should be compliant";
        
        // Test non-compliant FIG data
        QVariantMap invalidFig1Data;
        // Missing required fields
        
        EXPECT_FALSE(validateFIGCompliance(1, invalidFig1Data)) << "Invalid FIG 1 should be detected";
        
        // Verify overall compliance score
        double complianceScore = figAnalysisWidget->getOverallComplianceScore();
        EXPECT_GE(complianceScore, 0.0) << "Compliance score should be valid";
        EXPECT_LE(complianceScore, 100.0) << "Compliance score should be within range";
        
        // Test detected FIG types
        QList<quint8> detectedFigs = figAnalysisWidget->getDetectedFigTypes();
        EXPECT_TRUE(detectedFigs.contains(0)) << "FIG 0 should be detected";
        EXPECT_TRUE(detectedFigs.contains(1)) << "FIG 1 should be detected";
        
        complianceMonitor->recordCompliance("ETSI EN 300 401", "FIG Analysis", true, 
                                          QString("Compliance score: %1%").arg(complianceScore));
    }
}

/**
 * @brief Test Case 3: Service Tree Model Accuracy for DAB Hierarchy
 */
TEST_F(ETSIComplianceValidationTest, ServiceTreeModelAccuracy) {
    ASSERT_TRUE(mainWindow);
    
    auto* serviceBrowser = mainWindow->getServiceBrowser();
    ASSERT_TRUE(serviceBrowser);
    
    auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
    ASSERT_TRUE(serviceTreeModel);
    
    // Test ETSI DAB hierarchy compliance
    
    // Create ETSI-compliant ensemble
    EtiServiceTreeModel::EnsembleInfo ensemble;
    ensemble.ensembleId = 0x1234;
    ensemble.ensembleName = "Test Ensemble";
    ensemble.country = "Germany"; // ETSI standard country
    ensemble.status = EtiServiceTreeModel::ServiceStatus::Compliant;
    
    serviceTreeModel->updateFromEnsemble(ensemble);
    QTest::qWait(100);
    
    // Verify ensemble in model
    QModelIndex ensembleIndex = serviceTreeModel->getEnsembleIndex();
    EXPECT_TRUE(ensembleIndex.isValid()) << "Ensemble should be present in model";
    
    complianceMonitor->recordCompliance("ETSI EN 300 401", "Ensemble Structure", ensembleIndex.isValid());
    
    // Add ETSI-compliant services
    std::vector<EtiServiceTreeModel::ServiceInfo> testServices = {
        {0x1001, 1, "Radio Service 1", "Radio Service 1 Thai", "News", "German", "DAB+", 
         EtiServiceTreeModel::ServiceStatus::Active, true, 95.5, 128, "3A"},
        {0x1002, 2, "Radio Service 2", "Radio Service 2 Thai", "Music", "German", "DAB+", 
         EtiServiceTreeModel::ServiceStatus::Active, true, 98.2, 192, "2A"},
        {0x2001, 3, "Data Service 1", "Data Service 1 Thai", "EPG", "German", "Data", 
         EtiServiceTreeModel::ServiceStatus::Active, true, 89.1, 64, "4A"}
    };
    
    for (const auto& service : testServices) {
        serviceTreeModel->addService(service);
        QTest::qWait(50);
        
        // Verify service in model
        QModelIndex serviceIndex = serviceTreeModel->getServiceIndex(service.serviceId);
        EXPECT_TRUE(serviceIndex.isValid()) << QString("Service 0x%1 should be in model").arg(service.serviceId, 4, 16, QChar('0'));
        
        // Verify service data
        EtiServiceTreeModel::ServiceInfo retrievedService = serviceTreeModel->getServiceInfo(service.serviceId);
        EXPECT_EQ(retrievedService.serviceId, service.serviceId) << "Service ID should match";
        EXPECT_EQ(retrievedService.serviceName, service.serviceName) << "Service name should match";
        EXPECT_EQ(retrievedService.bitRate, service.bitRate) << "Bit rate should match";
        
        complianceMonitor->recordCompliance("ETSI EN 300 401", QString("Service 0x%1").arg(service.serviceId, 4, 16), true);
    }
    
    // Test service hierarchy navigation
    QList<quint32> allServices = serviceTreeModel->getAllServices();
    EXPECT_EQ(allServices.size(), testServices.size()) << "All services should be retrievable";
    
    QList<quint32> activeServices = serviceTreeModel->getActiveServices();
    EXPECT_EQ(activeServices.size(), testServices.size()) << "All test services should be active";
    
    // Test service statistics
    QVariantMap stats = serviceTreeModel->getServiceStatistics();
    EXPECT_EQ(stats["totalServices"].toInt(), testServices.size()) << "Statistics should reflect service count";
    EXPECT_EQ(stats["activeServices"].toInt(), testServices.size()) << "Statistics should reflect active services";
    
    complianceMonitor->recordCompliance("ETSI EN 300 401", "Service Hierarchy", true, 
                                      QString("Services: %1 total, %2 active").arg(allServices.size()).arg(activeServices.size()));
    
    // Test subchannel structure
    for (size_t i = 0; i < testServices.size(); ++i) {
        EtiServiceTreeModel::SubchannelInfo subchannel;
        subchannel.subchannelId = testServices[i].subChannelId;
        subchannel.name = QString("Subchannel %1").arg(subchannel.subchannelId);
        subchannel.startAddress = i * 64; // Standard subchannel addressing
        subchannel.subchannelSize = testServices[i].bitRate / 8; // Size based on bit rate
        subchannel.protectionLevel = testServices[i].protectionLevel;
        subchannel.status = EtiServiceTreeModel::ServiceStatus::Active;
        
        serviceTreeModel->addSubchannel(subchannel);
        QTest::qWait(50);
        
        QModelIndex subchannelIndex = serviceTreeModel->getSubchannelIndex(subchannel.subchannelId);
        EXPECT_TRUE(subchannelIndex.isValid()) << QString("Subchannel %1 should be in model").arg(subchannel.subchannelId);
        
        complianceMonitor->recordCompliance("ETSI EN 300 401", QString("Subchannel %1").arg(subchannel.subchannelId), true);
    }
}

/**
 * @brief Test Case 4: Error Detection and Professional Alert System
 */
TEST_F(ETSIComplianceValidationTest, ErrorDetectionAndAlertSystem) {
    ASSERT_TRUE(mainWindow);
    
    // Test alert system integration
    auto* alertSystem = mainWindow->findChild<QWidget*>("alertSystem");
    
    // Test ETSI compliance engine
    auto* complianceEngine = mainWindow->findChild<QObject*>("complianceEngine");
    
    // Generate and process frames with errors
    QByteArray errorFrame = generateNonCompliantFrame();
    
    // Process error frame and monitor alerts
    QSignalSpy errorSpy(mainWindow.get(), SIGNAL(etiError(QString)));
    
    testDataGenerator->processETIFrame(errorFrame);
    QTest::qWait(200);
    
    // Verify error detection
    EXPECT_FALSE(validateETIFrameStructure(errorFrame)) << "Error frame should be detected";
    
    // Test frame error highlighting in UI
    auto* analyserWidget = mainWindow->getAnalyserWidget();
    if (analyserWidget) {
        auto* frameTable = analyserWidget->findChild<QTableWidget*>("frameListTable");
        if (frameTable) {
            testDataGenerator->populateFrameList(frameTable);
            QTest::qWait(200);
            
            // Look for error highlighting
            bool errorHighlightFound = false;
            for (int row = 0; row < frameTable->rowCount(); ++row) {
                auto* item = frameTable->item(row, 0);
                if (item) {
                    QColor backgroundColor = item->background().color();
                    if (backgroundColor.red() > 200 && backgroundColor.green() < 100) {
                        errorHighlightFound = true;
                        break;
                    }
                }
            }
            
            // Note: Error highlighting may not be implemented yet
            complianceMonitor->recordCompliance("UI Error Indication", "Frame Highlighting", errorHighlightFound, 
                                              errorHighlightFound ? "Error frames highlighted" : "Error highlighting not yet implemented");
        }
    }
    
    // Test alert generation for various ETSI compliance issues
    std::vector<QString> testAlerts = {
        "ETSI EN 300 799 Frame Size Error",
        "ETSI EN 300 401 FIG Compliance Issue", 
        "ETSI EN 302 077 Signal Quality Warning",
        "ETSI TS 102 563 Audio Coding Error"
    };
    
    for (const QString& alertMessage : testAlerts) {
        // Simulate alert generation
        testDataGenerator->generateTestAlert(alertMessage, "WARNING");
        QTest::qWait(50);
        
        complianceMonitor->recordCompliance("Alert System", alertMessage, true, "Alert generated successfully");
    }
    
    // Test alert response time
    QElapsedTimer alertTimer;
    alertTimer.start();
    
    testDataGenerator->generateTestAlert("Critical ETSI Compliance Error", "CRITICAL");
    QApplication::processEvents();
    
    qint64 alertResponseTime = alertTimer.elapsed();
    EXPECT_LT(alertResponseTime, 17) << QString("Alert response should be <17ms (target), got %1ms").arg(alertResponseTime);
    
    complianceMonitor->recordCompliance("Alert System", "Response Time", alertResponseTime < 17, 
                                      QString("Response time: %1ms").arg(alertResponseTime));
}

/**
 * @brief Test Case 5: Broadcasting Industry Standards Compliance
 */
TEST_F(ETSIComplianceValidationTest, BroadcastingIndustryStandards) {
    ASSERT_TRUE(mainWindow);
    
    // Test ETSI TS 102 563 (DAB+ Audio Coding)
    QVariantMap audioStandards;
    audioStandards["codec"] = "HE-AACv2";
    audioStandards["sampleRate"] = 48000;
    audioStandards["bitRate"] = 128;
    audioStandards["channels"] = 2;
    
    bool audioCompliant = (audioStandards["codec"].toString() == "HE-AACv2" &&
                          audioStandards["sampleRate"].toInt() == 48000 &&
                          audioStandards["bitRate"].toInt() >= 32 &&
                          audioStandards["channels"].toInt() <= 2);
    
    complianceMonitor->recordCompliance("ETSI TS 102 563", "Audio Coding", audioCompliant, 
                                      audioCompliant ? "HE-AACv2 compliant" : "Audio coding non-compliant");
    
    // Test ETSI EN 302 077 (Transmitting Equipment)
    QVariantMap transmissionStandards;
    transmissionStandards["mode"] = "I"; // Transmission Mode I
    transmissionStandards["band"] = "VHF III"; // Band III (174-240 MHz)
    transmissionStandards["powerLevel"] = "Class 1"; // Transmitter class
    
    bool transmissionCompliant = (transmissionStandards["mode"].toString() == "I" &&
                                transmissionStandards["band"].toString() == "VHF III");
    
    complianceMonitor->recordCompliance("ETSI EN 302 077", "Transmission", transmissionCompliant, 
                                      transmissionCompliant ? "Transmission parameters compliant" : "Transmission non-compliant");
    
    // Test professional UI color compliance (broadcasting standards)
    QPalette palette = mainWindow->palette();
    QColor backgroundColor = palette.color(QPalette::Window);
    QColor textColor = palette.color(QPalette::WindowText);
    
    // Professional broadcast colors should be dark background with light text
    bool colorCompliant = (backgroundColor.value() < 128 && textColor.value() > 128);
    
    complianceMonitor->recordCompliance("Broadcasting UI", "Color Scheme", colorCompliant, 
                                      colorCompliant ? "Professional dark theme" : "Color scheme non-compliant");
    
    // Test update rate compliance (60 FPS professional standard)
    auto* realTimeTimer = mainWindow->findChild<QTimer*>("realTimeTimer");
    bool updateRateCompliant = false;
    if (realTimeTimer) {
        updateRateCompliant = (realTimeTimer->interval() <= 17); // 60 FPS = 16.67ms
    }
    
    complianceMonitor->recordCompliance("Broadcasting UI", "Update Rate", updateRateCompliant, 
                                      updateRateCompliant ? "60 FPS update rate" : "Update rate insufficient");
    
    // Test layout compliance (professional three-panel layout)
    auto* horizontalSplitter = mainWindow->findChild<QSplitter*>("horizontalSplitter");
    bool layoutCompliant = false;
    if (horizontalSplitter) {
        QList<int> sizes = horizontalSplitter->sizes();
        if (sizes.size() == 3) {
            int totalWidth = sizes[0] + sizes[1] + sizes[2];
            double leftPercent = (double)sizes[0] / totalWidth * 100.0;
            double centerPercent = (double)sizes[1] / totalWidth * 100.0;
            double rightPercent = (double)sizes[2] / totalWidth * 100.0;
            
            layoutCompliant = (leftPercent >= 20.0 && leftPercent <= 30.0 &&
                             centerPercent >= 45.0 && centerPercent <= 55.0 &&
                             rightPercent >= 20.0 && rightPercent <= 30.0);
        }
    }
    
    complianceMonitor->recordCompliance("Broadcasting UI", "Professional Layout", layoutCompliant, 
                                      layoutCompliant ? "25%-50%-25% layout compliant" : "Layout proportions non-compliant");
}

/**
 * @brief Test Case 6: Overall ETSI Compliance Score and Reporting
 */
TEST_F(ETSIComplianceValidationTest, OverallComplianceScoreAndReporting) {
    ASSERT_TRUE(mainWindow);
    
    // Run all compliance tests and generate comprehensive report
    
    // Process multiple compliant frames
    for (int i = 0; i < 10; ++i) {
        QByteArray compliantFrame = generateETSICompliantFrame();
        validateETIFrameStructure(compliantFrame);
        QTest::qWait(10);
    }
    
    // Process some non-compliant frames  
    for (int i = 0; i < 3; ++i) {
        QByteArray nonCompliantFrame = generateNonCompliantFrame();
        validateETIFrameStructure(nonCompliantFrame);
        QTest::qWait(10);
    }
    
    // Test various FIG types
    for (quint8 figType = 0; figType <= 2; ++figType) {
        QVariantMap figData;
        switch (figType) {
            case 0:
                figData["extension"] = 0;
                figData["ensembleId"] = 0x1234;
                break;
            case 1:
                figData["charset"] = 0;
                figData["label"] = "Test";
                break;
            case 2:
                figData["segment"] = 1;
                break;
        }
        validateFIGCompliance(figType, figData);
    }
    
    // Calculate overall compliance score
    double overallScore = complianceMonitor->getComplianceScore();
    
    EXPECT_GE(overallScore, 70.0) << QString("Overall ETSI compliance should be ≥70%, got %1%").arg(overallScore);
    EXPECT_LE(overallScore, 100.0) << "Compliance score should not exceed 100%";
    
    // Generate compliance report
    QList<ComplianceMonitor::ComplianceResult> results = complianceMonitor->getResults();
    
    qDebug() << "=== ETSI COMPLIANCE REPORT ===";
    qDebug() << QString("Overall Score: %1%").arg(overallScore, 0, 'f', 1);
    qDebug() << QString("Passed Tests: %1").arg(complianceMonitor->getPassedTests());
    qDebug() << QString("Failed Tests: %1").arg(complianceMonitor->getFailedTests());
    qDebug() << QString("Critical Issues: %1").arg(complianceMonitor->getCriticalIssues());
    qDebug() << QString("Warning Issues: %1").arg(complianceMonitor->getWarningIssues());
    qDebug() << "";
    
    // Report by standard
    QMap<QString, QList<ComplianceMonitor::ComplianceResult>> resultsByStandard;
    for (const auto& result : results) {
        resultsByStandard[result.standard].append(result);
    }
    
    for (auto it = resultsByStandard.begin(); it != resultsByStandard.end(); ++it) {
        const QString& standard = it.key();
        const QList<ComplianceMonitor::ComplianceResult>& standardResults = it.value();
        
        int passed = 0, failed = 0;
        for (const auto& result : standardResults) {
            if (result.compliant) passed++; else failed++;
        }
        
        double standardScore = (double)passed / (passed + failed) * 100.0;
        qDebug() << QString("%1: %2% (%3 passed, %4 failed)")
                    .arg(standard)
                    .arg(standardScore, 0, 'f', 1)
                    .arg(passed)
                    .arg(failed);
        
        // Key standards should have high compliance
        if (standard.contains("ETSI EN 300 799") || standard.contains("ETSI EN 300 401")) {
            EXPECT_GE(standardScore, 80.0) << QString("Core standard %1 should have ≥80% compliance").arg(standard);
        }
    }
    
    qDebug() << "=== END COMPLIANCE REPORT ===";
    
    // Verify critical standards compliance
    EXPECT_EQ(complianceMonitor->getCriticalIssues(), 0) << "Should have no critical compliance issues";
    EXPECT_LT(complianceMonitor->getWarningIssues(), 5) << "Should have minimal warning issues";
}

// Test Suite Entry Point
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Qt Test framework
    QApplication app(argc, argv);
    
    // Run all tests
    return RUN_ALL_TESTS();
}