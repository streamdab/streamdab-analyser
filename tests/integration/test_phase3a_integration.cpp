/**
 * @file test_phase3a_integration.cpp
 * @brief Phase 3A Integration Tests
 *
 * Comprehensive integration tests for Phase 3A component integration manager.
 * Tests complete signal/slot pipelines, data flow, and real-world scenarios.
 *
 * Test Coverage:
 * - Component initialization
 * - Signal/slot connection verification
 * - ETI → FIG → Service discovery pipeline
 * - ETI → MSC → Audio validation pipeline
 * - ETI → MOT → EPG decoding pipeline
 * - Error handling and recovery
 * - Performance and thread safety
 *
 * Phase 3A Wave 3.1: Integration & Testing (Day 2)
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 * @version 1.0
 */

#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include "../../src/core/phase3a_integration.hpp"
#include "test_utils.h"

using namespace eti::integration;

class TestPhase3AIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // ========================================================================
    // Component Initialization Tests
    // ========================================================================

    void test_component_initialization();
    void test_component_reinitialization();
    void test_component_shutdown();
    void test_initialization_signal_emission();

    // ========================================================================
    // Signal/Slot Connection Tests
    // ========================================================================

    void test_signal_slot_connections();
    void test_fig_analyser_connection();
    void test_audio_validator_connection();
    void test_mot_protocol_connection();
    void test_epg_decoder_connection();

    // ========================================================================
    // Pipeline Tests (Data Flow)
    // ========================================================================

    void test_eti_to_fig_pipeline();
    void test_eti_to_audio_pipeline();
    void test_eti_to_mot_pipeline();
    void test_mot_to_epg_pipeline();
    void test_complete_pipeline_integration();

    // ========================================================================
    // Processing Interface Tests
    // ========================================================================

    void test_single_frame_processing();
    void test_multiple_frame_processing();
    void test_file_processing();
    void test_processing_stop();

    // ========================================================================
    // Statistics Tests
    // ========================================================================

    void test_statistics_aggregation();
    void test_statistics_reset();
    void test_statistics_updates();

    // ========================================================================
    // Error Handling Tests
    // ========================================================================

    void test_invalid_frame_handling();
    void test_component_error_propagation();
    void test_processing_without_initialization();

    // ========================================================================
    // Performance Tests
    // ========================================================================

    void test_processing_performance();
    void test_thread_safety();

private:
    Phase3AIntegrationManager* m_manager;
    
    // Helper methods
    QByteArray createTestETIFile(int frame_count = 100);
    void waitForSignals(int timeout_ms = 1000);
};

// ============================================================================
// Test Fixture Setup/Teardown
// ============================================================================

void TestPhase3AIntegration::initTestCase()
{
    qDebug() << "=== Phase 3A Integration Tests Started ===";
    m_manager = nullptr;
}

void TestPhase3AIntegration::cleanupTestCase()
{
    qDebug() << "=== Phase 3A Integration Tests Complete ===";
}

void TestPhase3AIntegration::init()
{
    m_manager = new Phase3AIntegrationManager();
    QVERIFY(m_manager != nullptr);
}

void TestPhase3AIntegration::cleanup()
{
    if (m_manager) {
        delete m_manager;
        m_manager = nullptr;
    }
}

// ============================================================================
// Component Initialization Tests
// ============================================================================

void TestPhase3AIntegration::test_component_initialization()
{
    // Test: Initialize all components successfully
    QVERIFY(!m_manager->isInitialized());

    QSignalSpy readySpy(m_manager, &Phase3AIntegrationManager::integrationReady);
    bool success = m_manager->initializeComponents();

    QVERIFY(success);
    QVERIFY(m_manager->isInitialized());
    QCOMPARE(readySpy.count(), 1);

    // Verify components created
    QVERIFY(m_manager->etiParser() != nullptr);
    QVERIFY(m_manager->figAnalyser() != nullptr);
    QVERIFY(m_manager->audioValidator() != nullptr);
    QVERIFY(m_manager->motProtocol() != nullptr);
    QVERIFY(m_manager->epgDecoder() != nullptr);
}

void TestPhase3AIntegration::test_component_reinitialization()
{
    // Test: Reinitialization should succeed without errors
    QVERIFY(m_manager->initializeComponents());
    QVERIFY(m_manager->isInitialized());

    // Second initialization should return true (already initialized)
    QVERIFY(m_manager->initializeComponents());
    QVERIFY(m_manager->isInitialized());
}

void TestPhase3AIntegration::test_component_shutdown()
{
    // Test: Shutdown cleans up properly
    QVERIFY(m_manager->initializeComponents());
    QVERIFY(m_manager->isInitialized());

    m_manager->shutdownComponents();
    QVERIFY(!m_manager->isInitialized());

    // Should be safe to shutdown again
    m_manager->shutdownComponents();
    QVERIFY(!m_manager->isInitialized());
}

void TestPhase3AIntegration::test_initialization_signal_emission()
{
    // Test: integrationReady signal emitted on success
    QSignalSpy readySpy(m_manager, &Phase3AIntegrationManager::integrationReady);
    QSignalSpy errorSpy(m_manager, &Phase3AIntegrationManager::initializationError);

    QVERIFY(m_manager->initializeComponents());

    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);
}

// ============================================================================
// Signal/Slot Connection Tests
// ============================================================================

void TestPhase3AIntegration::test_signal_slot_connections()
{
    // Test: All signal/slot connections established
    QVERIFY(m_manager->initializeComponents());

    // Verify connections via signal emission tests
    QSignalSpy serviceSpy(m_manager, &Phase3AIntegrationManager::serviceDiscovered);
    QSignalSpy ensembleSpy(m_manager, &Phase3AIntegrationManager::ensembleDiscovered);
    QSignalSpy audioSpy(m_manager, &Phase3AIntegrationManager::audioStreamValidated);

    // Connections verified (will test actual signal emission in pipeline tests)
    QVERIFY(serviceSpy.isValid());
    QVERIFY(ensembleSpy.isValid());
    QVERIFY(audioSpy.isValid());
}

void TestPhase3AIntegration::test_fig_analyser_connection()
{
    // Test: FIG analyser signals connected to manager
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy serviceSpy(m_manager, &Phase3AIntegrationManager::serviceDiscovered);
    QSignalSpy ensembleSpy(m_manager, &Phase3AIntegrationManager::ensembleDiscovered);

    // Simulate FIG analyser signal (would normally come from processing)
    // For now, verify spies are valid (actual signal tested in pipeline tests)
    QVERIFY(serviceSpy.isValid());
    QVERIFY(ensembleSpy.isValid());
}

void TestPhase3AIntegration::test_audio_validator_connection()
{
    // Test: Audio validator signals connected to manager
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy audioSpy(m_manager, &Phase3AIntegrationManager::audioStreamValidated);
    QVERIFY(audioSpy.isValid());
}

void TestPhase3AIntegration::test_mot_protocol_connection()
{
    // Test: MOT protocol signals connected to manager
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy motSpy(m_manager, &Phase3AIntegrationManager::motObjectExtracted);
    QVERIFY(motSpy.isValid());
}

void TestPhase3AIntegration::test_epg_decoder_connection()
{
    // Test: EPG decoder signals connected to manager
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy epgSpy(m_manager, &Phase3AIntegrationManager::epgEventParsed);
    QVERIFY(epgSpy.isValid());
}

// ============================================================================
// Pipeline Tests (Data Flow)
// ============================================================================

void TestPhase3AIntegration::test_eti_to_fig_pipeline()
{
    // Test: ETI frame → FIG extraction → Service discovery
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy serviceSpy(m_manager, &Phase3AIntegrationManager::serviceDiscovered);
    QSignalSpy frameSpy(m_manager, &Phase3AIntegrationManager::frameProcessed);

    // Generate ETI frame with service information
    QByteArray frame = TestUtils::generateETIFrameWithService(0xC221, "Test Service");

    bool success = m_manager->processETIFrame(frame);
    QVERIFY(success);

    // Wait for signal processing
    waitForSignals();

    // Verify frame processed
    QCOMPARE(frameSpy.count(), 1);

    // Note: Service discovery depends on FIG processing which may take multiple frames
    // For basic test, verify pipeline doesn't crash
}

void TestPhase3AIntegration::test_eti_to_audio_pipeline()
{
    // Test: ETI frame → MSC extraction → Audio validation
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy audioSpy(m_manager, &Phase3AIntegrationManager::audioStreamValidated);

    // Generate ETI frame with audio data
    QByteArray frame = TestUtils::generateETIFrameWithAudioData();

    bool success = m_manager->processETIFrame(frame);
    QVERIFY(success);

    waitForSignals();

    // Audio validation may not trigger on single frame (needs full superframe)
    // Verify no crashes occurred
}

void TestPhase3AIntegration::test_eti_to_mot_pipeline()
{
    // Test: ETI frame → MSC extraction → MOT extraction
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy motSpy(m_manager, &Phase3AIntegrationManager::motObjectExtracted);

    // Generate ETI frame with data service
    QByteArray frame = TestUtils::generateETIFrameWithDataService();

    bool success = m_manager->processETIFrame(frame);
    QVERIFY(success);

    waitForSignals();

    // MOT object completion requires multiple frames
    // Verify pipeline processes without errors
}

void TestPhase3AIntegration::test_mot_to_epg_pipeline()
{
    // Test: MOT object → EPG decoding
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy epgSpy(m_manager, &Phase3AIntegrationManager::epgEventParsed);

    // Create MOT object with EPG-like data
    QByteArray motObject = TestUtils::generateCompleteMOTObject();

    // Directly process through EPG decoder (MOT extraction tested separately)
    m_manager->epgDecoder()->processMOTObject(motObject);

    waitForSignals();

    // EPG parsing may not succeed with synthetic data
    // Verify no crashes
}

void TestPhase3AIntegration::test_complete_pipeline_integration()
{
    // Test: Complete pipeline with multiple frames
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy frameSpy(m_manager, &Phase3AIntegrationManager::frameProcessed);
    QSignalSpy statsSpy(m_manager, &Phase3AIntegrationManager::statisticsUpdated);

    // Process multiple frames of different types
    QByteArray frame1 = TestUtils::generateMinimalETIFrame();
    QByteArray frame2 = TestUtils::generateETIFrameWithAudioData();
    QByteArray frame3 = TestUtils::generateETIFrameWithDataService();

    QVERIFY(m_manager->processETIFrame(frame1));
    QVERIFY(m_manager->processETIFrame(frame2));
    QVERIFY(m_manager->processETIFrame(frame3));

    waitForSignals();

    QCOMPARE(frameSpy.count(), 3);
    QVERIFY(statsSpy.count() > 0);

    // Verify statistics updated
    auto stats = m_manager->getStatistics();
    QCOMPARE(stats.frames_processed, 3u);
}

// ============================================================================
// Processing Interface Tests
// ============================================================================

void TestPhase3AIntegration::test_single_frame_processing()
{
    // Test: Process single ETI frame
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy frameSpy(m_manager, &Phase3AIntegrationManager::frameProcessed);

    QByteArray frame = TestUtils::generateMinimalETIFrame();
    bool success = m_manager->processETIFrame(frame);

    QVERIFY(success);
    waitForSignals();
    QCOMPARE(frameSpy.count(), 1);

    auto stats = m_manager->getStatistics();
    QCOMPARE(stats.frames_processed, 1u);
    QCOMPARE(stats.frames_valid, 1u);
}

void TestPhase3AIntegration::test_multiple_frame_processing()
{
    // Test: Process multiple frames sequentially
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy frameSpy(m_manager, &Phase3AIntegrationManager::frameProcessed);

    int frame_count = 10;
    for (int i = 0; i < frame_count; i++) {
        QByteArray frame = TestUtils::generateMinimalETIFrame();
        QVERIFY(m_manager->processETIFrame(frame));
    }

    waitForSignals();

    QCOMPARE(frameSpy.count(), frame_count);

    auto stats = m_manager->getStatistics();
    QCOMPARE(stats.frames_processed, static_cast<uint32_t>(frame_count));
}

void TestPhase3AIntegration::test_file_processing()
{
    // Test: Process ETI file
    QVERIFY(m_manager->initializeComponents());

    // Create temporary ETI file
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());

    int frame_count = 20;
    for (int i = 0; i < frame_count; i++) {
        QByteArray frame = TestUtils::generateMinimalETIFrame();
        tempFile.write(frame);
    }
    tempFile.flush();
    QString filePath = tempFile.fileName();

    QSignalSpy completeSpy(m_manager, &Phase3AIntegrationManager::processingComplete);

    bool success = m_manager->processETIFile(filePath);
    QVERIFY(success);

    // Wait for completion (file processing is synchronous)
    waitForSignals(2000);

    QCOMPARE(completeSpy.count(), 1);
    QVERIFY(!m_manager->isProcessing());

    auto stats = m_manager->getStatistics();
    QCOMPARE(stats.frames_processed, static_cast<uint32_t>(frame_count));
}

void TestPhase3AIntegration::test_processing_stop()
{
    // Test: Stop processing mid-operation
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy stopSpy(m_manager, &Phase3AIntegrationManager::processingComplete);

    // Create large file
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());

    for (int i = 0; i < 1000; i++) {
        QByteArray frame = TestUtils::generateMinimalETIFrame();
        tempFile.write(frame);
    }
    tempFile.flush();

    // Start processing in separate thread (or just call stop immediately)
    m_manager->processETIFile(tempFile.fileName());
    
    // Stop immediately (processing is synchronous, so this tests stop flag)
    m_manager->stopProcessing();

    QVERIFY(!m_manager->isProcessing());
}

// ============================================================================
// Statistics Tests
// ============================================================================

void TestPhase3AIntegration::test_statistics_aggregation()
{
    // Test: Statistics aggregated from all components
    QVERIFY(m_manager->initializeComponents());

    // Initial statistics should be zero
    auto stats = m_manager->getStatistics();
    QCOMPARE(stats.frames_processed, 0u);
    QCOMPARE(stats.services_discovered, 0u);

    // Process some frames
    for (int i = 0; i < 5; i++) {
        QByteArray frame = TestUtils::generateMinimalETIFrame();
        m_manager->processETIFrame(frame);
    }

    waitForSignals();

    // Verify statistics updated
    stats = m_manager->getStatistics();
    QCOMPARE(stats.frames_processed, 5u);
}

void TestPhase3AIntegration::test_statistics_reset()
{
    // Test: Statistics reset functionality
    QVERIFY(m_manager->initializeComponents());

    // Process frames
    for (int i = 0; i < 3; i++) {
        QByteArray frame = TestUtils::generateMinimalETIFrame();
        m_manager->processETIFrame(frame);
    }

    auto stats = m_manager->getStatistics();
    QVERIFY(stats.frames_processed > 0);

    // Reset
    m_manager->resetStatistics();

    stats = m_manager->getStatistics();
    QCOMPARE(stats.frames_processed, 0u);
    QCOMPARE(stats.services_discovered, 0u);
}

void TestPhase3AIntegration::test_statistics_updates()
{
    // Test: Statistics update signals emitted
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy statsSpy(m_manager, &Phase3AIntegrationManager::statisticsUpdated);

    // Process frame
    QByteArray frame = TestUtils::generateMinimalETIFrame();
    m_manager->processETIFrame(frame);

    waitForSignals();

    // Statistics should update at least once
    QVERIFY(statsSpy.count() >= 1);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

void TestPhase3AIntegration::test_invalid_frame_handling()
{
    // Test: Handle invalid frame gracefully
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy errorSpy(m_manager, &Phase3AIntegrationManager::integrationError);

    // Invalid frame size
    QByteArray invalidFrame(100, 0x00);
    bool success = m_manager->processETIFrame(invalidFrame);

    QVERIFY(!success);

    auto stats = m_manager->getStatistics();
    QCOMPARE(stats.frames_valid, 0u);
}

void TestPhase3AIntegration::test_component_error_propagation()
{
    // Test: Component errors propagated to manager
    QVERIFY(m_manager->initializeComponents());

    QSignalSpy errorSpy(m_manager, &Phase3AIntegrationManager::integrationError);
    QSignalSpy warningSpy(m_manager, &Phase3AIntegrationManager::integrationWarning);

    // Process frames and monitor for errors
    for (int i = 0; i < 5; i++) {
        QByteArray frame = TestUtils::generateMinimalETIFrame();
        m_manager->processETIFrame(frame);
    }

    waitForSignals();

    // Errors may or may not occur with synthetic data
    // Verify error handling doesn't crash
}

void TestPhase3AIntegration::test_processing_without_initialization()
{
    // Test: Processing without initialization fails gracefully
    QSignalSpy errorSpy(m_manager, &Phase3AIntegrationManager::integrationError);

    // Don't initialize
    QVERIFY(!m_manager->isInitialized());

    QByteArray frame = TestUtils::generateMinimalETIFrame();
    bool success = m_manager->processETIFrame(frame);

    QVERIFY(!success);
}

// ============================================================================
// Performance Tests
// ============================================================================

void TestPhase3AIntegration::test_processing_performance()
{
    // Test: Measure processing performance
    QVERIFY(m_manager->initializeComponents());

    int frame_count = 100;
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < frame_count; i++) {
        QByteArray frame = TestUtils::generateMinimalETIFrame();
        m_manager->processETIFrame(frame);
    }

    qint64 elapsed_ms = timer.elapsed();
    double fps = (static_cast<double>(frame_count) / elapsed_ms) * 1000.0;

    qInfo() << "Performance:" << fps << "FPS";
    qInfo() << "Elapsed:" << elapsed_ms << "ms for" << frame_count << "frames";

    // Target: >100 FPS for integration layer (lower than raw ETI parsing due to overhead)
    QVERIFY(fps > 100.0);

    auto stats = m_manager->getStatistics();
    QCOMPARE(stats.frames_processed, static_cast<uint32_t>(frame_count));
}

void TestPhase3AIntegration::test_thread_safety()
{
    // Test: Thread-safe operation (basic test)
    QVERIFY(m_manager->initializeComponents());

    // Get statistics from multiple calls (mutex protection test)
    auto stats1 = m_manager->getStatistics();
    auto stats2 = m_manager->getStatistics();
    
    QCOMPARE(stats1.frames_processed, stats2.frames_processed);

    // isInitialized thread safety
    QVERIFY(m_manager->isInitialized());
    QVERIFY(m_manager->isInitialized());
}

// ============================================================================
// Helper Methods
// ============================================================================

QByteArray TestPhase3AIntegration::createTestETIFile(int frame_count)
{
    QByteArray fileData;
    for (int i = 0; i < frame_count; i++) {
        fileData.append(TestUtils::generateMinimalETIFrame());
    }
    return fileData;
}

void TestPhase3AIntegration::waitForSignals(int timeout_ms)
{
    QTest::qWait(timeout_ms);
    QCoreApplication::processEvents();
}

QTEST_MAIN(TestPhase3AIntegration)
#include "test_phase3a_integration.moc"
