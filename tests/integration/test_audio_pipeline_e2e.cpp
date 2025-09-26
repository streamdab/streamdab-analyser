/**
 * @file test_audio_pipeline_e2e.cpp
 * @brief End-to-End validation test for complete audio decoding pipeline
 * 
 * Professional integration test demonstrating the complete audio system working
 * from ETI frame input through audio decoding to GUI display updates, with
 * full DAB/DAB+ codec support and real-time performance validation.
 * 
 * @author Audio Implementation Specialist Agent 14
 * @date 2025-09-26
 */

#include <QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTimer>
#include <QThread>
#include <memory>
#include <chrono>
#include <random>

#include "../../src/core/audio_decoder.hpp"
#include "../../src/core/dab_audio_decoder.h"
#include "../../src/core/audio_extraction_pipeline.h"
#include "../../src/gui/audio_monitoring_widget.h"
#include "../../src/gui/audio_decoder_integration.h"

/**
 * @brief Comprehensive End-to-End Audio Pipeline Test
 * 
 * This test validates the complete audio processing chain from ETI frames
 * to GUI display, ensuring all components work together seamlessly with
 * professional broadcast industry requirements.
 */
class TestAudioPipelineE2E : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Core pipeline validation tests
    void test_complete_pipeline_initialization_e2e();
    void test_dab_mpeg1_layer2_pipeline_e2e();
    void test_dab_plus_aac_pipeline_e2e();
    void test_multi_service_processing_e2e();
    void test_real_time_processing_performance_e2e();
    
    // GUI integration validation tests
    void test_gui_audio_monitoring_integration_e2e();
    void test_audio_level_monitoring_updates_e2e();
    void test_quality_metrics_gui_updates_e2e();
    void test_service_selection_workflow_e2e();
    
    // Professional broadcast requirements tests
    void test_broadcast_latency_requirements_e2e();
    void test_professional_audio_quality_validation_e2e();
    void test_error_recovery_and_resilience_e2e();
    void test_memory_efficiency_and_stability_e2e();
    
    // Performance and reliability tests
    void test_sustained_real_time_processing_e2e();
    void test_codec_switching_performance_e2e();
    void test_concurrent_service_processing_e2e();
    void test_long_duration_stability_e2e();

private:
    // Test environment setup
    std::unique_ptr<QApplication> test_app;
    std::unique_ptr<eti::audio::audio_decoder> mpeg_decoder;
    std::unique_ptr<eti::audio::audio_decoder> aac_decoder;
    std::unique_ptr<DabAudioDecoder> dab_decoder;
    std::unique_ptr<eti::AudioExtractionPipeline> extraction_pipeline;
    std::unique_ptr<AudioMonitoringWidget> monitoring_widget;
    std::unique_ptr<AudioDecoderIntegration> decoder_integration;
    
    // Test data generators
    QByteArray generateValidEtiFrame(uint32_t service_id, AudioCodecType codec);
    QByteArray generateDabMpegFrame(int bitrate = 128, int duration_ms = 24);
    QByteArray generateDabPlusAacFrame(int bitrate = 64, int duration_ms = 24);
    std::vector<QByteArray> generateFrameSequence(AudioCodecType codec, int frame_count);
    
    // Test validation helpers
    bool validateAudioOutput(const QByteArray& pcm_data, AudioCodecType expected_codec);
    bool validateProcessingLatency(std::chrono::microseconds latency, bool real_time_required);
    bool validateAudioQuality(const AudioQualityAssessment& assessment, AudioQualityLevel min_quality);
    bool validateMemoryUsage(size_t max_memory_mb);
    
    // Performance measurement utilities
    struct PerformanceMetrics {
        std::chrono::microseconds average_latency{0};
        double processing_rate_fps = 0.0;
        size_t memory_usage_mb = 0;
        double cpu_usage_percent = 0.0;
        uint64_t frames_processed = 0;
        uint64_t frames_with_errors = 0;
    };
    
    PerformanceMetrics measureProcessingPerformance(int test_duration_seconds);
    void logPerformanceResults(const QString& test_name, const PerformanceMetrics& metrics);
};

void TestAudioPipelineE2E::initTestCase() {
    // Create QApplication if not already created
    if (!QApplication::instance()) {
        int argc = 0;
        char** argv = nullptr;
        test_app = std::make_unique<QApplication>(argc, argv);
    }
    
    // Initialize all audio system components
    mpeg_decoder = std::make_unique<eti::audio::audio_decoder>();
    aac_decoder = std::make_unique<eti::audio::audio_decoder>();
    dab_decoder = std::make_unique<DabAudioDecoder>();
    extraction_pipeline = std::make_unique<eti::AudioExtractionPipeline>();
    monitoring_widget = std::make_unique<AudioMonitoringWidget>();
    decoder_integration = std::make_unique<AudioDecoderIntegration>();
    
    qDebug() << "E2E Test Environment initialized successfully";
}

void TestAudioPipelineE2E::cleanupTestCase() {
    // Clean shutdown of all components
    if (decoder_integration) {
        decoder_integration->stopRealTimeProcessing();
    }
    
    if (extraction_pipeline) {
        extraction_pipeline->stop_extraction();
        extraction_pipeline->remove_all_services();
    }
    
    // Reset all components
    decoder_integration.reset();
    monitoring_widget.reset();
    extraction_pipeline.reset();
    dab_decoder.reset();
    aac_decoder.reset();
    mpeg_decoder.reset();
    test_app.reset();
    
    qDebug() << "E2E Test Environment cleaned up successfully";
}

void TestAudioPipelineE2E::test_complete_pipeline_initialization_e2e() {
    // ARRANGE - Prepare complete pipeline initialization
    QVERIFY2(decoder_integration != nullptr, "Decoder integration must be available");
    QVERIFY2(monitoring_widget != nullptr, "Monitoring widget must be available");
    
    // ACT - Initialize complete pipeline with GUI integration
    bool integration_init = decoder_integration->initialize();
    decoder_integration->setMonitoringWidget(monitoring_widget.get());
    
    // Initialize monitoring widget with null processor (standalone mode)
    bool widget_init = monitoring_widget->initialize(nullptr);
    
    // ASSERT - Verify complete pipeline initialization
    QVERIFY2(integration_init, "Audio decoder integration must initialize successfully");
    QVERIFY2(widget_init, "Audio monitoring widget must initialize successfully");
    QVERIFY2(decoder_integration->isInitialized(), "Integration should report initialized state");
    
    // Verify component interconnection
    QVERIFY2(!decoder_integration->isRealTimeProcessingActive(), "Real-time processing should not be active initially");
    QCOMPARE(decoder_integration->getCurrentServiceId(), static_cast<uint32_t>(0));
    QCOMPARE(decoder_integration->getTotalFramesProcessed(), static_cast<uint64_t>(0));
    
    qDebug() << "Complete pipeline initialization validated successfully";
}

void TestAudioPipelineE2E::test_dab_mpeg1_layer2_pipeline_e2e() {
    // ARRANGE - Initialize pipeline for MPEG-1 Layer 2 processing
    QVERIFY2(decoder_integration->initialize(), "Integration must be initialized for MPEG test");
    decoder_integration->setMonitoringWidget(monitoring_widget.get());
    
    const uint32_t test_service_id = 0x1234;
    QSignalSpy frame_decoded_spy(decoder_integration.get(), &AudioDecoderIntegration::audio_frame_decoded);
    QSignalSpy quality_changed_spy(decoder_integration.get(), &AudioDecoderIntegration::audio_quality_changed);
    
    // Generate test MPEG-1 Layer 2 frames
    std::vector<QByteArray> mpeg_frames = generateFrameSequence(AudioCodecType::DAB_MPEG1_Layer2, 10);
    
    // ACT - Process MPEG-1 Layer 2 frames through complete pipeline
    int successful_decodes = 0;
    for (const auto& frame : mpeg_frames) {
        bool decode_result = decoder_integration->decodeAudioFrame(
            test_service_id, frame, AudioCodecType::DAB_MPEG1_Layer2);
        if (decode_result) {
            successful_decodes++;
        }
        
        // Allow GUI event processing
        QApplication::processEvents();
        QTest::qWait(5);
    }
    
    // Allow signal processing to complete
    QTest::qWait(100);
    
    // ASSERT - Verify MPEG-1 Layer 2 pipeline functionality
    QVERIFY2(successful_decodes > 0, "At least some MPEG frames should decode successfully");
    QVERIFY2(frame_decoded_spy.count() > 0, "Frame decoded signals should be emitted");
    QVERIFY2(decoder_integration->getTotalFramesProcessed() > 0, "Frame processing counter should be updated");
    
    // Verify audio quality assessment
    AudioQualityAssessment quality = decoder_integration->getAudioQuality(test_service_id);
    QCOMPARE(quality.service_id, test_service_id);
    QVERIFY2(quality.overall_quality != AudioQualityLevel::Unknown, "Audio quality should be assessed");
    
    // Verify processing performance for MPEG-1 Layer 2
    double processing_rate = decoder_integration->getProcessingRate();
    QVERIFY2(processing_rate > 40.0, "MPEG-1 Layer 2 processing should exceed real-time (41.67 fps)");
    
    auto average_latency = decoder_integration->getAverageLatency();
    QVERIFY2(average_latency.count() < 20000, "Average latency should be under 20ms for broadcast");
    
    qDebug() << QString("MPEG-1 Layer 2 Pipeline: %1 frames processed, %2 fps, %3 μs avg latency")
                .arg(successful_decodes)
                .arg(processing_rate, 0, 'f', 1)
                .arg(average_latency.count());
}

void TestAudioPipelineE2E::test_dab_plus_aac_pipeline_e2e() {
    // ARRANGE - Initialize pipeline for DAB+ AAC processing
    QVERIFY2(decoder_integration->initialize(), "Integration must be initialized for AAC test");
    decoder_integration->setMonitoringWidget(monitoring_widget.get());
    
    const uint32_t test_service_id = 0x5678;
    QSignalSpy frame_decoded_spy(decoder_integration.get(), &AudioDecoderIntegration::audio_frame_decoded);
    QSignalSpy audio_levels_spy(decoder_integration.get(), &AudioDecoderIntegration::audio_levels_updated);
    
    // Generate test HE-AAC v2 frames
    std::vector<QByteArray> aac_frames = generateFrameSequence(AudioCodecType::DABPlus_HE_AAC_v2, 15);
    
    // ACT - Process HE-AAC v2 frames through complete pipeline
    int successful_decodes = 0;
    for (const auto& frame : aac_frames) {
        bool decode_result = decoder_integration->decodeAudioFrame(
            test_service_id, frame, AudioCodecType::DABPlus_HE_AAC_v2);
        if (decode_result) {
            successful_decodes++;
        }
        
        QApplication::processEvents();
        QTest::qWait(3);
    }
    
    QTest::qWait(150); // Allow signals to propagate
    
    // ASSERT - Verify DAB+ AAC pipeline functionality
    QVERIFY2(successful_decodes > 0, "HE-AAC v2 frames should decode successfully");
    QVERIFY2(frame_decoded_spy.count() > 0, "Decoded frame signals should be emitted");
    QVERIFY2(audio_levels_spy.count() > 0, "Audio level updates should be emitted to GUI");
    
    // Verify DAB+ specific quality metrics
    AudioQualityAssessment quality = decoder_integration->getAudioQuality(test_service_id);
    QCOMPARE(quality.service_id, test_service_id);
    QVERIFY2(quality.overall_quality >= AudioQualityLevel::Good, "DAB+ should provide good quality");
    QVERIFY2(quality.peak_level_db > -60.0, "Audio levels should be detected");
    QVERIFY2(quality.codec_type == AudioCodecType::DABPlus_HE_AAC_v2, "Codec type should be detected correctly");
    
    // Verify enhanced processing performance for DAB+
    double processing_rate = decoder_integration->getProcessingRate();
    QVERIFY2(processing_rate > 40.0, "DAB+ processing should exceed real-time requirements");
    
    qDebug() << QString("DAB+ HE-AAC v2 Pipeline: %1 frames processed, %2 fps, Quality: %3")
                .arg(successful_decodes)
                .arg(processing_rate, 0, 'f', 1)
                .arg(static_cast<int>(quality.overall_quality));
}

void TestAudioPipelineE2E::test_real_time_processing_performance_e2e() {
    // ARRANGE - Initialize for real-time performance testing
    QVERIFY2(decoder_integration->initialize(), "Integration must be initialized for real-time test");
    decoder_integration->setMonitoringWidget(monitoring_widget.get());
    
    const uint32_t test_service_id = 0x9ABC;
    QSignalSpy processing_started_spy(decoder_integration.get(), &AudioDecoderIntegration::real_time_processing_started);
    QSignalSpy performance_metrics_spy(decoder_integration.get(), &AudioDecoderIntegration::performance_metrics_updated);
    
    // ACT - Start real-time processing and measure performance
    bool real_time_started = decoder_integration->startRealTimeProcessing(test_service_id);
    QVERIFY2(real_time_started, "Real-time processing must start successfully");
    
    // Wait for processing start confirmation
    QTest::qWait(50);
    
    // Simulate real-time frame processing
    auto start_time = std::chrono::steady_clock::now();
    const int test_duration_ms = 1000; // 1 second test
    const int frame_interval_ms = 24;   // DAB frame interval
    int frames_sent = 0;
    
    QTimer frame_timer;
    connect(&frame_timer, &QTimer::timeout, [&]() {
        QByteArray test_frame = generateDabPlusAacFrame(64, 24);
        decoder_integration->decodeAudioFrame(test_service_id, test_frame, AudioCodecType::DABPlus_AAC);
        frames_sent++;
        
        QApplication::processEvents();
    });
    
    frame_timer.start(frame_interval_ms);
    QTest::qWait(test_duration_ms);
    frame_timer.stop();
    
    auto end_time = std::chrono::steady_clock::now();
    auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Stop real-time processing
    decoder_integration->stopRealTimeProcessing();
    
    // ASSERT - Verify real-time processing performance
    QVERIFY2(processing_started_spy.count() > 0, "Real-time processing start signal should be emitted");
    QVERIFY2(frames_sent > 0, "Test frames should be sent to decoder");
    
    // Verify processing rate meets broadcast requirements
    double processing_rate = decoder_integration->getProcessingRate();
    QVERIFY2(processing_rate >= 35.0, "Real-time processing must maintain at least 35 fps");
    
    // Verify low latency requirements for broadcasting
    auto average_latency = decoder_integration->getAverageLatency();
    QVERIFY2(average_latency.count() < 50000, "Real-time latency must be under 50ms");
    
    // Verify frame processing efficiency
    uint64_t processed_frames = decoder_integration->getTotalFramesProcessed();
    double processing_efficiency = static_cast<double>(processed_frames) / frames_sent * 100.0;
    QVERIFY2(processing_efficiency >= 80.0, "Processing efficiency should be at least 80%");
    
    qDebug() << QString("Real-time Performance: %1 fps, %2 μs latency, %3% efficiency")
                .arg(processing_rate, 0, 'f', 1)
                .arg(average_latency.count())
                .arg(processing_efficiency, 0, 'f', 1);
}

void TestAudioPipelineE2E::test_gui_audio_monitoring_integration_e2e() {
    // ARRANGE - Initialize GUI integration components
    QVERIFY2(decoder_integration->initialize(), "Integration must be initialized for GUI test");
    decoder_integration->setMonitoringWidget(monitoring_widget.get());
    
    const uint32_t test_service_id = 0xDEF0;
    QSignalSpy audio_levels_spy(decoder_integration.get(), &AudioDecoderIntegration::audio_levels_updated);
    QSignalSpy quality_changed_spy(decoder_integration.get(), &AudioDecoderIntegration::audio_quality_changed);
    QSignalSpy service_selected_spy(monitoring_widget.get(), &AudioMonitoringWidget::serviceSelected);
    
    // ACT - Process audio frames and verify GUI updates
    std::vector<QByteArray> test_frames = generateFrameSequence(AudioCodecType::DABPlus_AAC, 5);
    
    for (const auto& frame : test_frames) {
        bool decode_result = decoder_integration->decodeAudioFrame(
            test_service_id, frame, AudioCodecType::DABPlus_AAC);
        QVERIFY2(decode_result, "Frame decoding should succeed for GUI integration test");
        
        QApplication::processEvents();
        QTest::qWait(10);
    }
    
    // Simulate service selection from GUI
    monitoring_widget->selectService(test_service_id);
    QApplication::processEvents();
    QTest::qWait(50);
    
    // ASSERT - Verify GUI integration functionality
    QVERIFY2(audio_levels_spy.count() > 0, "Audio level updates should reach monitoring widget");
    QVERIFY2(quality_changed_spy.count() > 0, "Quality changes should be signaled to GUI");
    
    // Verify GUI service selection workflow
    QVERIFY2(service_selected_spy.count() > 0, "Service selection should generate signals");
    
    // Verify monitoring widget state
    auto monitored_services = monitoring_widget->getMonitoredServices();
    QVERIFY2(monitored_services.contains(test_service_id), "Service should be registered in monitoring widget");
    
    auto service_audio_info = monitoring_widget->getServiceAudioInfo(test_service_id);
    QCOMPARE(service_audio_info.serviceId, test_service_id);
    QVERIFY2(service_audio_info.isActive, "Service should be marked as active");
    QVERIFY2(service_audio_info.isDabPlus, "DAB+ codec should be detected");
    
    // Verify overall quality metrics in GUI
    double overall_quality = monitoring_widget->getOverallQualityScore();
    QVERIFY2(overall_quality >= 0.0 && overall_quality <= 100.0, "Overall quality should be in valid range");
    
    qDebug() << QString("GUI Integration: %1 services monitored, %2% overall quality")
                .arg(monitored_services.size())
                .arg(overall_quality, 0, 'f', 1);
}

void TestAudioPipelineE2E::test_broadcast_latency_requirements_e2e() {
    // ARRANGE - Initialize for broadcast latency validation
    QVERIFY2(decoder_integration->initialize(), "Integration must be initialized for latency test");
    
    const uint32_t test_service_id = 0x1111;
    const int latency_test_frames = 50;
    std::vector<std::chrono::microseconds> frame_latencies;
    
    // ACT - Measure per-frame processing latency
    std::vector<QByteArray> test_frames = generateFrameSequence(AudioCodecType::DABPlus_HE_AAC_v1, latency_test_frames);
    
    for (const auto& frame : test_frames) {
        auto frame_start = std::chrono::high_resolution_clock::now();
        
        bool decode_result = decoder_integration->decodeAudioFrame(
            test_service_id, frame, AudioCodecType::DABPlus_HE_AAC_v1);
        
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_latency = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);
        
        if (decode_result) {
            frame_latencies.push_back(frame_latency);
        }
        
        QApplication::processEvents();
    }
    
    // Calculate latency statistics
    if (!frame_latencies.empty()) {
        auto min_latency = *std::min_element(frame_latencies.begin(), frame_latencies.end());
        auto max_latency = *std::max_element(frame_latencies.begin(), frame_latencies.end());
        
        auto sum_latency = std::accumulate(frame_latencies.begin(), frame_latencies.end(), 
                                          std::chrono::microseconds{0});
        auto avg_latency = sum_latency / frame_latencies.size();
        
        // ASSERT - Verify broadcast industry latency requirements
        QVERIFY2(avg_latency.count() < 20000, "Average latency must be under 20ms for broadcast");
        QVERIFY2(max_latency.count() < 50000, "Maximum latency must be under 50ms for broadcast");
        QVERIFY2(min_latency.count() < 10000, "Minimum latency should be under 10ms for efficiency");
        
        // Calculate latency consistency (95th percentile)
        std::sort(frame_latencies.begin(), frame_latencies.end());
        size_t p95_index = static_cast<size_t>(frame_latencies.size() * 0.95);
        auto p95_latency = frame_latencies[p95_index];
        QVERIFY2(p95_latency.count() < 30000, "95th percentile latency must be under 30ms");
        
        qDebug() << QString("Broadcast Latency: Avg %1 μs, Max %2 μs, P95 %3 μs")
                    .arg(avg_latency.count())
                    .arg(max_latency.count())
                    .arg(p95_latency.count());
    } else {
        QFAIL("No successful frame decodes for latency measurement");
    }
}

// Helper method implementations
QByteArray TestAudioPipelineE2E::generateValidEtiFrame(uint32_t service_id, AudioCodecType codec) {
    Q_UNUSED(service_id) // Service ID would be embedded in real ETI frame
    
    switch (codec) {
        case AudioCodecType::DAB_MPEG1_Layer2:
            return generateDabMpegFrame(128, 24);
        case AudioCodecType::DABPlus_AAC:
        case AudioCodecType::DABPlus_HE_AAC_v1:
        case AudioCodecType::DABPlus_HE_AAC_v2:
            return generateDabPlusAacFrame(64, 24);
        default:
            return QByteArray();
    }
}

QByteArray TestAudioPipelineE2E::generateDabMpegFrame(int bitrate, int duration_ms) {
    Q_UNUSED(bitrate)
    Q_UNUSED(duration_ms)
    
    // Generate simulated MPEG-1 Layer 2 frame
    QByteArray mpeg_frame(1152, 0); // Typical MPEG-1 L2 frame size
    
    // MPEG header simulation
    mpeg_frame[0] = 0xFF; // Sync word
    mpeg_frame[1] = 0xF6; // MPEG-1 Layer 2
    mpeg_frame[2] = 0x40; // Bitrate index
    mpeg_frame[3] = 0x00; // Sample rate and other flags
    
    // Fill with pseudo-random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    
    for (int i = 4; i < mpeg_frame.size(); ++i) {
        mpeg_frame[i] = dis(gen);
    }
    
    return mpeg_frame;
}

QByteArray TestAudioPipelineE2E::generateDabPlusAacFrame(int bitrate, int duration_ms) {
    Q_UNUSED(bitrate)
    Q_UNUSED(duration_ms)
    
    // Generate simulated HE-AAC frame with ADTS header
    QByteArray aac_frame(512, 0); // Typical AAC frame size
    
    // ADTS header simulation (7 bytes)
    aac_frame[0] = 0xFF; // Sync word
    aac_frame[1] = 0xF1; // MPEG-4, no CRC
    aac_frame[2] = 0x40; // AAC-LC profile, sample rate index
    aac_frame[3] = 0x40; // Channel config, frame length MSB
    aac_frame[4] = 0x20; // Frame length LSB, buffer fullness MSB
    aac_frame[5] = 0x1F; // Buffer fullness LSB, number of blocks
    aac_frame[6] = 0xFC; // Number of blocks continued
    
    // Fill payload with pseudo-random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    
    for (int i = 7; i < aac_frame.size(); ++i) {
        aac_frame[i] = dis(gen);
    }
    
    return aac_frame;
}

std::vector<QByteArray> TestAudioPipelineE2E::generateFrameSequence(AudioCodecType codec, int frame_count) {
    std::vector<QByteArray> frames;
    frames.reserve(frame_count);
    
    for (int i = 0; i < frame_count; ++i) {
        frames.push_back(generateValidEtiFrame(0x1234, codec));
    }
    
    return frames;
}

bool TestAudioPipelineE2E::validateAudioOutput(const QByteArray& pcm_data, AudioCodecType expected_codec) {
    Q_UNUSED(expected_codec)
    
    if (pcm_data.isEmpty()) {
        return false;
    }
    
    // Basic PCM validation
    if (pcm_data.size() % sizeof(int16_t) != 0) {
        return false; // Should be 16-bit aligned
    }
    
    // Check for some audio variation (not all zeros/silence)
    const int16_t* samples = reinterpret_cast<const int16_t*>(pcm_data.constData());
    size_t sample_count = pcm_data.size() / sizeof(int16_t);
    
    if (sample_count < 10) {
        return false; // Too few samples
    }
    
    // Look for variation in samples
    int16_t first_sample = samples[0];
    for (size_t i = 1; i < std::min(sample_count, size_t(100)); ++i) {
        if (samples[i] != first_sample) {
            return true; // Found variation - looks like valid audio
        }
    }
    
    return false; // All samples identical - likely not valid audio
}

QTEST_APPLESS_MAIN(TestAudioPipelineE2E)
#include "test_audio_pipeline_e2e.moc"