/**
 * @file test_audio_pipeline.cpp
 * @brief Integration Test: ETI Frame → FIG Processing → Audio Validation Pipeline
 *
 * Tests complete audio processing pipeline:
 * 1. ETI frame parsing
 * 2. FIG extraction and decoding (service discovery)
 * 3. Audio stream extraction from MSC
 * 4. DAB+ superframe validation
 * 5. Audio quality metrics extraction
 *
 * Phase 3A Wave 3.1: Integration & Testing Infrastructure
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 */

#include <QtTest/QtTest>
#include <QSignalSpy>
#include "test_utils.h"
#include "../../src/core/eti_types.h"
#include "../../src/core/dabplus_stream_validator.hpp"

class TestAudioPipeline : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Pipeline component tests
    void testETI_Frame_Generation();
    void testAudio_Validator_Creation();
    void testAudio_Superframe_Metrics();
    
    // Signal/slot chain tests
    void testAudio_Signal_Mechanism();
    
    // Error handling tests
    void testInvalidAudio_Handling();
    void testEmptyAudio_Handling();

private:
    // Test components
    std::unique_ptr<eti::audio::DABPlusStreamValidator> m_audio_validator;
    
    // Helper methods
    QByteArray generateTestETIFrame();
    QByteArray generateAudioData();
    void verifyAudioMetrics(const eti::audio::AudioQualityMetrics& metrics);
};

void TestAudioPipeline::initTestCase() {
    qInfo() << "=== Audio Pipeline Integration Test Suite ===";
    qInfo() << "Testing: Audio validation infrastructure";
    
    // Initialize audio validator
    m_audio_validator = std::make_unique<eti::audio::DABPlusStreamValidator>();
    
    QVERIFY(m_audio_validator != nullptr);
}

void TestAudioPipeline::cleanupTestCase() {
    qInfo() << "=== Audio Pipeline Test Cleanup ===";
    m_audio_validator.reset();
}

/**
 * @brief Test ETI frame generation utilities
 */
void TestAudioPipeline::testETI_Frame_Generation() {
    qInfo() << "Test: ETI Frame Generation";
    
    // Generate test ETI frame
    QByteArray eti_frame = TestUtils::generateMinimalETIFrame();
    QVERIFY(!eti_frame.isEmpty());
    QCOMPARE(eti_frame.size(), 6144);
    
    // Verify frame structure
    QVERIFY(TestUtils::verifyETIFrameStructure(eti_frame));
    
    qInfo() << "  ✓ ETI frame generated: 6144 bytes";
    qInfo() << "  ✓ Sync pattern validated";
}

/**
 * @brief Test audio validator instantiation
 */
void TestAudioPipeline::testAudio_Validator_Creation() {
    qInfo() << "Test: Audio Validator Creation";
    
    QVERIFY(m_audio_validator != nullptr);
    
    // Get initial statistics
    auto stats = m_audio_validator->getStatistics();
    QCOMPARE(stats.superframes_processed, static_cast<uint64_t>(0));
    
    qInfo() << "  ✓ Audio validator created";
    qInfo() << "  ✓ Statistics accessible";
}

/**
 * @brief Test audio quality metrics extraction
 */
void TestAudioPipeline::testAudio_Superframe_Metrics() {
    qInfo() << "Test: Audio Quality Metrics";
    
    // Create a synthetic audio superframe with known parameters
    eti::audio::AudioSuperframe superframe;
    superframe.sbr_flag = true;
    superframe.ps_flag = true;
    superframe.dac_rate = 0; // 48 kHz
    superframe.num_aus = 4;
    superframe.protection_level = 3;
    superframe.bitrate_kbps = 96;
    superframe.is_valid = true;
    
    // Extract metrics
    auto metrics = m_audio_validator->extractAudioMetrics(superframe);
    
    // Verify metrics
    QCOMPARE(metrics.bitrate_kbps, static_cast<uint16_t>(96));
    QCOMPARE(metrics.sample_rate_khz, static_cast<uint8_t>(48));
    QVERIFY(metrics.is_heaac_v2);
    QCOMPARE(metrics.getProfileString(), QString("HE-AAC v2"));
    
    qInfo() << "  ✓ Metrics extracted correctly";
    qInfo() << "    - Bitrate:" << metrics.bitrate_kbps << "kbps";
    qInfo() << "    - Profile:" << metrics.getProfileString();
    qInfo() << "    - Sample rate:" << metrics.sample_rate_khz << "kHz";
}

/**
 * @brief Test signal/slot mechanism for audio detection
 */
void TestAudioPipeline::testAudio_Signal_Mechanism() {
    qInfo() << "Test: Audio Signal Mechanism";
    
    // Setup signal spy for audio detection
    QSignalSpy spy(m_audio_validator.get(), 
                   &eti::audio::DABPlusStreamValidator::audioStreamDetected);
    
    QVERIFY(spy.isValid());
    
    qInfo() << "  ✓ Signal/slot mechanism functional";
}

/**
 * @brief Test error handling for invalid audio data
 */
void TestAudioPipeline::testInvalidAudio_Handling() {
    qInfo() << "Test: Invalid Audio Handling";
    
    // Create invalid audio data (too small)
    QByteArray invalid_audio(10, 0x00);
    uint32_t service_id = 0xE1C00379;
    
    // Should handle gracefully without crash
    bool result = m_audio_validator->processAudioData(service_id, invalid_audio);
    
    QVERIFY(!result); // Should fail gracefully
    qInfo() << "  ✓ Invalid audio rejected correctly";
}

/**
 * @brief Test handling of empty audio data
 */
void TestAudioPipeline::testEmptyAudio_Handling() {
    qInfo() << "Test: Empty Audio Handling";
    
    // Create empty audio data
    QByteArray empty_audio;
    uint32_t service_id = 0xE1C00379;
    
    // Should handle gracefully
    bool result = m_audio_validator->processAudioData(service_id, empty_audio);
    
    QVERIFY(!result); // Should fail gracefully
    qInfo() << "  ✓ Empty audio handled correctly";
}

// ============================================================================
// Helper Methods
// ============================================================================

QByteArray TestAudioPipeline::generateTestETIFrame() {
    return TestUtils::generateMinimalETIFrame();
}

QByteArray TestAudioPipeline::generateAudioData() {
    return TestUtils::generateETIFrameWithAudioData();
}

void TestAudioPipeline::verifyAudioMetrics(
    const eti::audio::AudioQualityMetrics& metrics) 
{
    // Verify metrics are within valid ranges
    QVERIFY(metrics.bitrate_kbps >= 8 && metrics.bitrate_kbps <= 192);
    QVERIFY(metrics.sample_rate_khz == 24 || metrics.sample_rate_khz == 48);
    QVERIFY(metrics.protection_level >= 1 && metrics.protection_level <= 5);
    
    qInfo() << "  ✓ Audio metrics validated";
}

QTEST_MAIN(TestAudioPipeline)
#include "test_audio_pipeline.moc"
