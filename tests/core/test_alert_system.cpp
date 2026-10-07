#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QSignalSpy>
#include <QApplication>
#include <QTime>
#include "../../src/core/etsi/alert_system.h"
#include "../../src/core/eti_types.h"

using namespace testing;

/**
 * @brief Test Alert System TDD Implementation
 * 
 * Professional Alert System for ETI Stream Analyser following TDD methodology.
 * Tests written BEFORE implementation (RED phase).
 * 
 * Features to test:
 * - Configurable alert thresholds
 * - Real-time monitoring with threshold detection
 * - Alert dispatch system for performance
 * - Visual/audio notifications
 * - Professional broadcast alert standards compliance
 */
class AlertSystemTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        // Create QApplication if needed for Qt tests
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = std::make_unique<QApplication>(argc, argv);
        }
        
        // Initialize alert system
        alertSystem = std::make_unique<AlertSystem>();
    }

    void TearDown() override 
    {
        alertSystem.reset();
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<AlertSystem> alertSystem;
};

// ============================================================================
// BASIC ALERT SYSTEM FUNCTIONALITY TESTS
// ============================================================================

TEST_F(AlertSystemTest, ConstructorInitializesCorrectly)
{
    // ARRANGE & ACT - Constructor called in SetUp
    
    // ASSERT
    EXPECT_TRUE(alertSystem != nullptr);
    EXPECT_TRUE(alertSystem->isInitialized());
    EXPECT_FALSE(alertSystem->isMonitoringEnabled());
    EXPECT_EQ(alertSystem->getActiveAlertCount(), 0);
}

TEST_F(AlertSystemTest, EnableMonitoringStartsAlertProcessing)
{
    // ARRANGE
    QSignalSpy monitoringStartedSpy(alertSystem.get(), &AlertSystem::monitoringStarted);
    
    // ACT
    bool result = alertSystem->enableMonitoring(true);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(alertSystem->isMonitoringEnabled());
    EXPECT_EQ(monitoringStartedSpy.count(), 1);
}

TEST_F(AlertSystemTest, DisableMonitoringStopsAlertProcessing)
{
    // ARRANGE
    alertSystem->enableMonitoring(true);
    QSignalSpy monitoringStoppedSpy(alertSystem.get(), &AlertSystem::monitoringStopped);
    
    // ACT
    bool result = alertSystem->enableMonitoring(false);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_FALSE(alertSystem->isMonitoringEnabled());
    EXPECT_EQ(monitoringStoppedSpy.count(), 1);
}

// ============================================================================
// ALERT THRESHOLD CONFIGURATION TESTS
// ============================================================================

TEST_F(AlertSystemTest, SetSignalQualityThresholdConfiguresCorrectly)
{
    // ARRANGE
    const double lowThreshold = 15.0;  // dB SNR
    const double criticalThreshold = 10.0;  // dB SNR
    
    // ACT
    bool result = alertSystem->setSignalQualityThreshold(
        AlertSystem::AlertLevel::Warning, lowThreshold);
    bool result2 = alertSystem->setSignalQualityThreshold(
        AlertSystem::AlertLevel::Critical, criticalThreshold);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(result2);
    EXPECT_EQ(alertSystem->getSignalQualityThreshold(AlertSystem::AlertLevel::Warning), lowThreshold);
    EXPECT_EQ(alertSystem->getSignalQualityThreshold(AlertSystem::AlertLevel::Critical), criticalThreshold);
}

TEST_F(AlertSystemTest, SetErrorRateThresholdConfiguresCorrectly)
{
    // ARRANGE
    const double warningRate = 0.01;   // 1% error rate
    const double criticalRate = 0.05;  // 5% error rate
    
    // ACT
    bool result1 = alertSystem->setErrorRateThreshold(
        AlertSystem::AlertLevel::Warning, warningRate);
    bool result2 = alertSystem->setErrorRateThreshold(
        AlertSystem::AlertLevel::Critical, criticalRate);
    
    // ASSERT
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_EQ(alertSystem->getErrorRateThreshold(AlertSystem::AlertLevel::Warning), warningRate);
    EXPECT_EQ(alertSystem->getErrorRateThreshold(AlertSystem::AlertLevel::Critical), criticalRate);
}

TEST_F(AlertSystemTest, SetAudioLevelThresholdConfiguresCorrectly)
{
    // ARRANGE
    const double silenceThreshold = -60.0;  // dBFS
    const double overloadThreshold = -3.0;   // dBFS
    
    // ACT
    bool result1 = alertSystem->setAudioLevelThreshold(
        AlertSystem::AlertLevel::Warning, silenceThreshold);
    bool result2 = alertSystem->setAudioLevelThreshold(
        AlertSystem::AlertLevel::Critical, overloadThreshold);
    
    // ASSERT
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_EQ(alertSystem->getAudioLevelThreshold(AlertSystem::AlertLevel::Warning), silenceThreshold);
    EXPECT_EQ(alertSystem->getAudioLevelThreshold(AlertSystem::AlertLevel::Critical), overloadThreshold);
}

TEST_F(AlertSystemTest, InvalidThresholdValuesAreRejected)
{
    // ARRANGE & ACT & ASSERT
    EXPECT_FALSE(alertSystem->setSignalQualityThreshold(AlertSystem::AlertLevel::Warning, -10.0));  // Negative SNR
    EXPECT_FALSE(alertSystem->setErrorRateThreshold(AlertSystem::AlertLevel::Warning, 1.5));        // >100% error rate
    EXPECT_FALSE(alertSystem->setAudioLevelThreshold(AlertSystem::AlertLevel::Warning, 10.0));      // >0 dBFS
}

// ============================================================================
// REAL-TIME THRESHOLD DETECTION TESTS
// ============================================================================

TEST_F(AlertSystemTest, ProcessEtiFrameTriggersSignalQualityAlert)
{
    // ARRANGE
    alertSystem->setSignalQualityThreshold(AlertSystem::AlertLevel::Warning, 15.0);
    alertSystem->enableMonitoring(true);
    
    QSignalSpy alertTriggeredSpy(alertSystem.get(), &AlertSystem::alertTriggered);
    
    EtiFrameData frame;
    frame.signalQuality.snr = 12.0;  // Below 15.0 threshold
    frame.signalQuality.ber = 0.001;
    frame.timestamp = QTime::currentTime();
    
    // ACT
    alertSystem->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_EQ(alertTriggeredSpy.count(), 1);
    
    auto alertData = alertTriggeredSpy.at(0);
    EXPECT_EQ(alertData.at(0).value<AlertSystem::AlertLevel>(), AlertSystem::AlertLevel::Warning);
    EXPECT_EQ(alertData.at(1).value<AlertSystem::AlertType>(), AlertSystem::AlertType::SignalQuality);
}

TEST_F(AlertSystemTest, ProcessEtiFrameTriggersErrorRateAlert)
{
    // ARRANGE
    alertSystem->setErrorRateThreshold(AlertSystem::AlertLevel::Critical, 0.02);
    alertSystem->enableMonitoring(true);
    
    QSignalSpy alertTriggeredSpy(alertSystem.get(), &AlertSystem::alertTriggered);
    
    EtiFrameData frame;
    frame.signalQuality.ber = 0.025;  // Above 0.02 threshold
    frame.signalQuality.snr = 20.0;
    frame.timestamp = QTime::currentTime();
    
    // ACT
    alertSystem->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_EQ(alertTriggeredSpy.count(), 1);
    
    auto alertData = alertTriggeredSpy.at(0);
    EXPECT_EQ(alertData.at(0).value<AlertSystem::AlertLevel>(), AlertSystem::AlertLevel::Critical);
    EXPECT_EQ(alertData.at(1).value<AlertSystem::AlertType>(), AlertSystem::AlertType::ErrorRate);
}

TEST_F(AlertSystemTest, ProcessAudioDataTriggersAudioLevelAlert)
{
    // ARRANGE
    alertSystem->setAudioLevelThreshold(AlertSystem::AlertLevel::Warning, -60.0);
    alertSystem->enableMonitoring(true);
    
    QSignalSpy alertTriggeredSpy(alertSystem.get(), &AlertSystem::alertTriggered);
    
    AudioLevelData audioData;
    audioData.peakLevel = -65.0;  // Below -60.0 threshold (silence)
    audioData.rmsLevel = -70.0;
    audioData.timestamp = QTime::currentTime();
    audioData.serviceId = 0x1001;
    
    // ACT
    alertSystem->processAudioData(audioData);
    
    // ASSERT
    EXPECT_EQ(alertTriggeredSpy.count(), 1);
    
    auto alertData = alertTriggeredSpy.at(0);
    EXPECT_EQ(alertData.at(0).value<AlertSystem::AlertLevel>(), AlertSystem::AlertLevel::Warning);
    EXPECT_EQ(alertData.at(1).value<AlertSystem::AlertType>(), AlertSystem::AlertType::AudioLevel);
}

TEST_F(AlertSystemTest, FrameProcessingWithinThresholdsDoesNotTriggerAlerts)
{
    // ARRANGE
    alertSystem->setSignalQualityThreshold(AlertSystem::AlertLevel::Warning, 15.0);
    alertSystem->setErrorRateThreshold(AlertSystem::AlertLevel::Warning, 0.01);
    alertSystem->enableMonitoring(true);
    
    QSignalSpy alertTriggeredSpy(alertSystem.get(), &AlertSystem::alertTriggered);
    
    EtiFrameData frame;
    frame.signalQuality.snr = 18.0;   // Above threshold
    frame.signalQuality.ber = 0.005;  // Below threshold
    frame.timestamp = QTime::currentTime();
    
    // ACT
    alertSystem->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_EQ(alertTriggeredSpy.count(), 0);
}

// ============================================================================
// ALERT DISPATCH SYSTEM TESTS
// ============================================================================

TEST_F(AlertSystemTest, AlertDispatchSystemHandlesMultipleAlerts)
{
    // ARRANGE
    alertSystem->setSignalQualityThreshold(AlertSystem::AlertLevel::Warning, 15.0);
    alertSystem->setErrorRateThreshold(AlertSystem::AlertLevel::Critical, 0.02);
    alertSystem->enableMonitoring(true);
    
    QSignalSpy alertTriggeredSpy(alertSystem.get(), &AlertSystem::alertTriggered);
    
    EtiFrameData frame;
    frame.signalQuality.snr = 12.0;   // Below threshold -> Warning
    frame.signalQuality.ber = 0.025;  // Above threshold -> Critical
    frame.timestamp = QTime::currentTime();
    
    // ACT
    alertSystem->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_EQ(alertTriggeredSpy.count(), 2);
    
    // Should have both Warning and Critical alerts
    bool hasWarning = false, hasCritical = false;
    for (int i = 0; i < alertTriggeredSpy.count(); ++i) {
        auto level = alertTriggeredSpy.at(i).at(0).value<AlertSystem::AlertLevel>();
        if (level == AlertSystem::AlertLevel::Warning) hasWarning = true;
        if (level == AlertSystem::AlertLevel::Critical) hasCritical = true;
    }
    EXPECT_TRUE(hasWarning);
    EXPECT_TRUE(hasCritical);
}

TEST_F(AlertSystemTest, AlertCooldownPreventsDuplicateAlerts)
{
    // ARRANGE
    alertSystem->setSignalQualityThreshold(AlertSystem::AlertLevel::Warning, 15.0);
    alertSystem->setAlertCooldownPeriod(1000);  // 1 second cooldown
    alertSystem->enableMonitoring(true);
    
    QSignalSpy alertTriggeredSpy(alertSystem.get(), &AlertSystem::alertTriggered);
    
    EtiFrameData frame;
    frame.signalQuality.snr = 12.0;  // Below threshold
    frame.signalQuality.ber = 0.001;
    frame.timestamp = QTime::currentTime();
    
    // ACT
    alertSystem->processEtiFrame(frame);  // Should trigger alert
    alertSystem->processEtiFrame(frame);  // Should NOT trigger (cooldown)
    
    // ASSERT
    EXPECT_EQ(alertTriggeredSpy.count(), 1);  // Only one alert
}

TEST_F(AlertSystemTest, AlertHistoryMaintainsRecentAlerts)
{
    // ARRANGE
    alertSystem->setSignalQualityThreshold(AlertSystem::AlertLevel::Warning, 15.0);
    alertSystem->enableMonitoring(true);
    
    EtiFrameData frame;
    frame.signalQuality.snr = 12.0;
    frame.signalQuality.ber = 0.001;
    frame.timestamp = QTime::currentTime();
    
    // ACT
    alertSystem->processEtiFrame(frame);
    
    // ASSERT
    auto history = alertSystem->getAlertHistory();
    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history.first().level, AlertSystem::AlertLevel::Warning);
    EXPECT_EQ(history.first().type, AlertSystem::AlertType::SignalQuality);
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

TEST_F(AlertSystemTest, HighFrequencyProcessingMaintainsPerformance)
{
    // ARRANGE
    alertSystem->setSignalQualityThreshold(AlertSystem::AlertLevel::Warning, 15.0);
    alertSystem->enableMonitoring(true);
    
    const int numFrames = 1000;
    EtiFrameData frame;
    frame.signalQuality.snr = 20.0;  // Good signal, no alerts
    frame.signalQuality.ber = 0.001;
    
    // ACT
    QTime startTime = QTime::currentTime();
    for (int i = 0; i < numFrames; ++i) {
        frame.timestamp = QTime::currentTime();
        alertSystem->processEtiFrame(frame);
    }
    int elapsedMs = startTime.msecsTo(QTime::currentTime());
    
    // ASSERT
    // Should process 1000 frames in less than 100ms (requirement: <100ms latency)
    EXPECT_LT(elapsedMs, 100);
    
    // Memory usage should remain stable
    EXPECT_LT(alertSystem->getMemoryUsage(), 10 * 1024 * 1024);  // <10MB
}

// ============================================================================
// BROADCAST STANDARDS COMPLIANCE TESTS
// ============================================================================

TEST_F(AlertSystemTest, AlertLevelsFollowBroadcastStandards)
{
    // ARRANGE & ACT & ASSERT
    
    // Standard broadcast alert levels
    EXPECT_TRUE(alertSystem->isValidAlertLevel(AlertSystem::AlertLevel::Info));
    EXPECT_TRUE(alertSystem->isValidAlertLevel(AlertSystem::AlertLevel::Warning));
    EXPECT_TRUE(alertSystem->isValidAlertLevel(AlertSystem::AlertLevel::Critical));
    EXPECT_TRUE(alertSystem->isValidAlertLevel(AlertSystem::AlertLevel::Emergency));
}

TEST_F(AlertSystemTest, AlertTypesFollowBroadcastStandards)
{
    // ARRANGE & ACT & ASSERT
    
    // Standard broadcast alert types
    EXPECT_TRUE(alertSystem->isValidAlertType(AlertSystem::AlertType::SignalQuality));
    EXPECT_TRUE(alertSystem->isValidAlertType(AlertSystem::AlertType::ErrorRate));
    EXPECT_TRUE(alertSystem->isValidAlertType(AlertSystem::AlertType::AudioLevel));
    EXPECT_TRUE(alertSystem->isValidAlertType(AlertSystem::AlertType::ServiceLoss));
    EXPECT_TRUE(alertSystem->isValidAlertType(AlertSystem::AlertType::SystemError));
}

TEST_F(AlertSystemTest, AlertMessagesFollowBroadcastFormat)
{
    // ARRANGE
    alertSystem->setSignalQualityThreshold(AlertSystem::AlertLevel::Warning, 15.0);
    alertSystem->enableMonitoring(true);
    
    QSignalSpy alertTriggeredSpy(alertSystem.get(), &AlertSystem::alertTriggered);
    
    EtiFrameData frame;
    frame.signalQuality.snr = 12.0;
    frame.signalQuality.ber = 0.001;
    frame.timestamp = QTime::currentTime();
    
    // ACT
    alertSystem->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_EQ(alertTriggeredSpy.count(), 1);
    
    auto alertData = alertTriggeredSpy.at(0);
    QString message = alertData.at(2).toString();
    
    // Message should contain timestamp, level, type, and value
    EXPECT_TRUE(message.contains("WARNING"));
    EXPECT_TRUE(message.contains("Signal Quality"));
    EXPECT_TRUE(message.contains("SNR"));
    EXPECT_TRUE(message.contains("12.0"));
}

// ============================================================================
// CONFIGURATION PERSISTENCE TESTS
// ============================================================================

TEST_F(AlertSystemTest, AlertConfigurationCanBeSavedAndLoaded)
{
    // ARRANGE
    alertSystem->setSignalQualityThreshold(AlertSystem::AlertLevel::Warning, 15.0);
    alertSystem->setErrorRateThreshold(AlertSystem::AlertLevel::Critical, 0.02);
    alertSystem->setAudioLevelThreshold(AlertSystem::AlertLevel::Warning, -60.0);
    
    // ACT
    bool saveResult = alertSystem->saveConfiguration("test_config.yaml");
    auto newAlertSystem = std::make_unique<AlertSystem>();
    bool loadResult = newAlertSystem->loadConfiguration("test_config.yaml");
    
    // ASSERT
    EXPECT_TRUE(saveResult);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(newAlertSystem->getSignalQualityThreshold(AlertSystem::AlertLevel::Warning), 15.0);
    EXPECT_EQ(newAlertSystem->getErrorRateThreshold(AlertSystem::AlertLevel::Critical), 0.02);
    EXPECT_EQ(newAlertSystem->getAudioLevelThreshold(AlertSystem::AlertLevel::Warning), -60.0);
}

// ============================================================================
// INTEGRATION WITH GUI TESTS
// ============================================================================

TEST_F(AlertSystemTest, AlertSystemIntegratesWithMainWindow)
{
    // ARRANGE
    // This test will verify integration after GUI implementation
    
    // ACT & ASSERT
    EXPECT_TRUE(alertSystem->supportsGuiIntegration());
    EXPECT_TRUE(alertSystem->canConnectToMainWindow());
}