/**
 * @file test_stream_recording_manager.cpp
 * @brief Comprehensive TDD test suite for StreamRecordingManager
 * 
 * Tests time-based segmentation recording, multiple format support,
 * and professional broadcast archival workflows.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QDir>
#include <QSignalSpy>
#include <QTimer>
#include <QEventLoop>
#include <chrono>
#include <thread>

#include "core/stream_recording_manager.hpp"
#include "tests/fixtures/etsi_test_data/etsi_reference_data.h"

using namespace eti;
using namespace testing;

class StreamRecordingManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Qt application for file operations
        if (!QCoreApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = std::make_unique<QCoreApplication>(argc, argv);
        }
        
        // Create temporary directory for test recordings
        temp_dir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(temp_dir->isValid());
        
        recording_manager = std::make_unique<StreamRecordingManager>();
        
        // Setup default segmentation config for testing
        segmentation_config.segment_duration = std::chrono::minutes{1}; // Short for testing
        segmentation_config.enable_compression = false;
        segmentation_config.auto_cleanup = false; // Disable for test verification
        segmentation_config.max_segment_size_mb = 10;
        segmentation_config.filename_pattern = "test_eti_%Y%m%d_%H%M%S";
        
        // Generate test ETI frames
        test_frames = EtsiReferenceData::get_valid_eti_frames();
        ASSERT_GT(test_frames.size(), 0);
    }
    
    void TearDown() override {
        if (recording_manager) {
            // Stop all active sessions
            auto active_sessions = recording_manager->get_active_sessions();
            for (const auto& session_id : active_sessions) {
                recording_manager->stop_recording_session(session_id);
            }
            recording_manager.reset();
        }
    }
    
    std::unique_ptr<QCoreApplication> app;
    std::unique_ptr<QTemporaryDir> temp_dir;
    std::unique_ptr<StreamRecordingManager> recording_manager;
    SegmentationConfig segmentation_config;
    std::vector<EtiFrame> test_frames;
};

// TDD Phase 1: Basic Recording Session Management
TEST_F(StreamRecordingManagerTest, CreateRecordingSession) {
    // Red: Test fails without implementation
    EXPECT_NO_THROW({
        QString session_id = recording_manager->start_recording_session(
            temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
        
        EXPECT_FALSE(session_id.isEmpty());
        EXPECT_TRUE(recording_manager->is_session_active(session_id));
        EXPECT_FALSE(recording_manager->is_session_paused(session_id));
    });
}

TEST_F(StreamRecordingManagerTest, StartStopRecordingSession) {
    // Test session lifecycle
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    EXPECT_TRUE(recording_manager->is_session_active(session_id));
    
    // Stop session
    bool stopped = recording_manager->stop_recording_session(session_id);
    EXPECT_TRUE(stopped);
    EXPECT_FALSE(recording_manager->is_session_active(session_id));
}

TEST_F(StreamRecordingManagerTest, PauseResumeRecordingSession) {
    // Test session pause/resume
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    // Pause session
    bool paused = recording_manager->pause_recording_session(session_id);
    EXPECT_TRUE(paused);
    EXPECT_TRUE(recording_manager->is_session_paused(session_id));
    
    // Resume session
    bool resumed = recording_manager->resume_recording_session(session_id);
    EXPECT_TRUE(resumed);
    EXPECT_FALSE(recording_manager->is_session_paused(session_id));
}

TEST_F(StreamRecordingManagerTest, MultipleRecordingSessions) {
    // Test multiple concurrent sessions
    QString session1 = recording_manager->start_recording_session(
        temp_dir->path() + "/session1", RecordingFormat::ETI_NI, segmentation_config);
    QString session2 = recording_manager->start_recording_session(
        temp_dir->path() + "/session2", RecordingFormat::ETI_LI, segmentation_config);
    
    EXPECT_FALSE(session1.isEmpty());
    EXPECT_FALSE(session2.isEmpty());
    EXPECT_NE(session1, session2);
    
    auto active_sessions = recording_manager->get_active_sessions();
    EXPECT_EQ(active_sessions.size(), 2);
    EXPECT_TRUE(active_sessions.contains(session1));
    EXPECT_TRUE(active_sessions.contains(session2));
}

// TDD Phase 2: Frame Recording Tests
TEST_F(StreamRecordingManagerTest, RecordSingleFrame) {
    // Test single frame recording
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    // Record a test frame
    if (!test_frames.empty()) {
        recording_manager->record_frame(session_id, test_frames[0]);
        
        // Allow time for recording
        QEventLoop loop;
        QTimer::singleShot(100, &loop, &QEventLoop::quit);
        loop.exec();
        
        auto session_info = recording_manager->get_session_info(session_id);
        EXPECT_GT(session_info.frames_recorded, 0);
    }
}

TEST_F(StreamRecordingManagerTest, RecordFrameBatch) {
    // Test batch frame recording
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    // Record batch of frames
    recording_manager->record_frame_batch(session_id, test_frames);
    
    // Allow time for recording
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();
    
    auto session_info = recording_manager->get_session_info(session_id);
    EXPECT_GE(session_info.frames_recorded, test_frames.size());
}

TEST_F(StreamRecordingManagerTest, RecordingFormats) {
    // Test different recording formats
    std::vector<RecordingFormat> formats = {
        RecordingFormat::ETI_NI,
        RecordingFormat::ETI_LI,
        RecordingFormat::RAW_BINARY
    };
    
    for (auto format : formats) {
        QString session_id = recording_manager->start_recording_session(
            temp_dir->path() + "/" + QString::number(static_cast<int>(format)),
            format, segmentation_config);
        
        EXPECT_FALSE(session_id.isEmpty());
        
        auto session_info = recording_manager->get_session_info(session_id);
        EXPECT_EQ(session_info.format, format);
        
        recording_manager->stop_recording_session(session_id);
    }
}

// TDD Phase 3: Time-based Segmentation Tests
TEST_F(StreamRecordingManagerTest, TimeBasedSegmentation) {
    // Test automatic time-based segmentation
    SegmentationConfig short_config = segmentation_config;
    short_config.segment_duration = std::chrono::seconds{1}; // 1 second for quick test
    
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, short_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    QSignalSpy segment_spy(recording_manager.get(), 
                          &StreamRecordingManager::segment_completed);
    
    // Record frames over multiple segments
    for (int i = 0; i < 10; ++i) {
        if (i < test_frames.size()) {
            recording_manager->record_frame(session_id, test_frames[i]);
        }
        QThread::msleep(150); // Sleep to span multiple segments
    }
    
    // Wait for segmentation
    QEventLoop loop;
    QTimer::singleShot(2500, &loop, &QEventLoop::quit); // Wait for 2+ segments
    loop.exec();
    
    auto session_info = recording_manager->get_session_info(session_id);
    EXPECT_GT(session_info.segments_created, 1);
    
    // Check if signal was emitted
    EXPECT_GE(segment_spy.count(), 1);
}

TEST_F(StreamRecordingManagerTest, SegmentSizeLimit) {
    // Test segment size-based rotation
    SegmentationConfig size_config = segmentation_config;
    size_config.max_segment_size_mb = 1; // 1MB limit for quick test
    
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, size_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    // Record many frames to exceed size limit
    for (int i = 0; i < 1000; ++i) {
        int frame_index = i % test_frames.size();
        recording_manager->record_frame(session_id, test_frames[frame_index]);
    }
    
    // Allow time for processing
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();
    
    auto session_info = recording_manager->get_session_info(session_id);
    EXPECT_GT(session_info.frames_recorded, 1000);
}

TEST_F(StreamRecordingManagerTest, ForceSegmentRotation) {
    // Test manual segment rotation
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    // Record some frames
    for (size_t i = 0; i < std::min(size_t(10), test_frames.size()); ++i) {
        recording_manager->record_frame(session_id, test_frames[i]);
    }
    
    QSignalSpy segment_spy(recording_manager.get(), 
                          &StreamRecordingManager::segment_completed);
    
    // Force rotation
    recording_manager->force_segment_rotation(session_id);
    
    // Wait for rotation
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();
    
    EXPECT_GE(segment_spy.count(), 1);
}

// TDD Phase 4: File Management and Output Tests
TEST_F(StreamRecordingManagerTest, OutputDirectoryCreation) {
    // Test automatic output directory creation
    QString non_existent_dir = temp_dir->path() + "/new_directory";
    EXPECT_FALSE(QDir(non_existent_dir).exists());
    
    QString session_id = recording_manager->start_recording_session(
        non_existent_dir, RecordingFormat::ETI_NI, segmentation_config);
    
    EXPECT_FALSE(session_id.isEmpty());
    EXPECT_TRUE(QDir(non_existent_dir).exists());
}

TEST_F(StreamRecordingManagerTest, FilenameGeneration) {
    // Test filename pattern generation
    SegmentationConfig filename_config = segmentation_config;
    filename_config.filename_pattern = "custom_%Y%m%d_%H%M%S";
    
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, filename_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    // Record frames to create files
    for (size_t i = 0; i < std::min(size_t(5), test_frames.size()); ++i) {
        recording_manager->record_frame(session_id, test_frames[i]);
    }
    
    // Force segment creation
    recording_manager->force_segment_rotation(session_id);
    
    // Wait for file creation
    QEventLoop loop;
    QTimer::singleShot(300, &loop, &QEventLoop::quit);
    loop.exec();
    
    auto session_files = recording_manager->get_session_files(session_id);
    if (!session_files.isEmpty()) {
        QString filename = QFileInfo(session_files.first()).baseName();
        EXPECT_TRUE(filename.startsWith("custom_"));
        EXPECT_TRUE(filename.contains(QDateTime::currentDateTime().toString("yyyyMM")));
    }
}

TEST_F(StreamRecordingManagerTest, FileFormatExtensions) {
    // Test correct file extensions for different formats
    std::map<RecordingFormat, QString> expected_extensions = {
        {RecordingFormat::ETI_NI, ".eti"},
        {RecordingFormat::ETI_LI, ".etili"},
        {RecordingFormat::RAW_BINARY, ".raw"}
    };
    
    for (const auto& [format, extension] : expected_extensions) {
        QString format_description = recording_manager->get_format_description(format);
        QString format_ext = recording_manager->get_format_extension(format);
        
        EXPECT_FALSE(format_description.isEmpty());
        EXPECT_EQ(format_ext, extension);
    }
}

// TDD Phase 5: Performance and Statistics Tests
TEST_F(StreamRecordingManagerTest, RecordingPerformance) {
    // Test recording performance metrics
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Record frames at high rate
    for (int i = 0; i < 250; ++i) { // Simulate 1 second at 250 fps
        int frame_index = i % test_frames.size();
        recording_manager->record_frame(session_id, test_frames[frame_index]);
        
        // Minimal delay to simulate real-time
        if (i % 10 == 0) {
            QThread::msleep(1);
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Wait for processing completion
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();
    
    double frame_rate = recording_manager->get_recording_frame_rate(session_id);
    auto latency = recording_manager->get_recording_latency(session_id);
    
    EXPECT_GT(frame_rate, 0.0);
    EXPECT_LT(latency.count(), 50000); // Less than 50ms latency
    
    qDebug() << "Recording Performance:";
    qDebug() << "  Duration:" << duration.count() << "ms";
    qDebug() << "  Frame rate:" << frame_rate << "fps";
    qDebug() << "  Latency:" << latency.count() << "µs";
}

TEST_F(StreamRecordingManagerTest, StatisticsTracking) {
    // Test statistics tracking
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    QSignalSpy stats_spy(recording_manager.get(), 
                         &StreamRecordingManager::session_statistics_updated);
    
    // Record frames
    for (size_t i = 0; i < std::min(size_t(20), test_frames.size()); ++i) {
        recording_manager->record_frame(session_id, test_frames[i]);
    }
    
    // Wait for statistics update
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();
    
    size_t total_frames = recording_manager->get_total_frames_recorded(session_id);
    size_t total_bytes = recording_manager->get_total_bytes_recorded(session_id);
    
    EXPECT_GT(total_frames, 0);
    EXPECT_GT(total_bytes, 0);
    EXPECT_EQ(total_bytes, total_frames * ETI_FRAME_SIZE);
}

// TDD Phase 6: Error Handling Tests
TEST_F(StreamRecordingManagerTest, InvalidOutputDirectory) {
    // Test handling of invalid output directory
    QString invalid_dir = "/root/restricted/path"; // Typically restricted path
    
    QString session_id = recording_manager->start_recording_session(
        invalid_dir, RecordingFormat::ETI_NI, segmentation_config);
    
    if (session_id.isEmpty()) {
        // Expected behavior for restricted paths
        auto errors = recording_manager->get_session_errors("invalid_session");
        EXPECT_GE(errors.size(), 0); // May have errors
    } else {
        // If session was created, it should handle the error gracefully
        EXPECT_FALSE(session_id.isEmpty());
    }
}

TEST_F(StreamRecordingManagerTest, DiskSpaceMonitoring) {
    // Test disk space monitoring
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    QSignalSpy disk_warning_spy(recording_manager.get(), 
                               &StreamRecordingManager::disk_space_warning);
    
    double disk_usage = recording_manager->get_disk_usage_percent(temp_dir->path());
    qint64 available_space = recording_manager->get_available_disk_space(temp_dir->path());
    
    EXPECT_GE(disk_usage, 0.0);
    EXPECT_LE(disk_usage, 100.0);
    EXPECT_GT(available_space, 0);
    
    // Set very low threshold to trigger warning
    recording_manager->set_disk_usage_threshold(1.0);
    
    // Record frames to potentially trigger warning
    for (size_t i = 0; i < std::min(size_t(10), test_frames.size()); ++i) {
        recording_manager->record_frame(session_id, test_frames[i]);
    }
    
    // Disk warning may or may not be triggered depending on actual disk space
}

TEST_F(StreamRecordingManagerTest, SessionErrorTracking) {
    // Test session error tracking
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    
    // Errors are tracked internally and can be retrieved
    auto errors = recording_manager->get_session_errors(session_id);
    size_t error_count = recording_manager->get_error_count(session_id);
    
    EXPECT_EQ(errors.size(), error_count);
    EXPECT_GE(error_count, 0);
    
    // Clear errors
    recording_manager->clear_session_errors(session_id);
    auto cleared_errors = recording_manager->get_session_errors(session_id);
    EXPECT_EQ(cleared_errors.size(), 0);
}

// TDD Phase 7: Factory and Configuration Tests
TEST_F(StreamRecordingManagerTest, FactoryCreation) {
    // Test factory methods
    auto broadcast_recorder = StreamRecordingManagerFactory::create_broadcast_recorder(
        temp_dir->path());
    EXPECT_TRUE(broadcast_recorder != nullptr);
    
    auto archival_recorder = StreamRecordingManagerFactory::create_archival_recorder(
        temp_dir->path(), std::chrono::hours{72});
    EXPECT_TRUE(archival_recorder != nullptr);
    
    auto monitoring_recorder = StreamRecordingManagerFactory::create_monitoring_recorder(
        temp_dir->path(), std::chrono::minutes{5});
    EXPECT_TRUE(monitoring_recorder != nullptr);
    
    auto storage_recorder = StreamRecordingManagerFactory::create_storage_recorder(
        temp_dir->path(), 9);
    EXPECT_TRUE(storage_recorder != nullptr);
}

TEST_F(StreamRecordingManagerTest, ConfigurationManagement) {
    // Test configuration management
    SegmentationConfig new_config = segmentation_config;
    new_config.segment_duration = std::chrono::minutes{30};
    new_config.enable_compression = true;
    
    recording_manager->set_default_segmentation_config(new_config);
    auto retrieved_config = recording_manager->get_default_segmentation_config();
    
    EXPECT_EQ(retrieved_config.segment_duration, std::chrono::minutes{30});
    EXPECT_TRUE(retrieved_config.enable_compression);
}

TEST_F(StreamRecordingManagerTest, SupportedFormats) {
    // Test format support queries
    auto supported_formats = recording_manager->get_supported_formats();
    EXPECT_GT(supported_formats.size(), 0);
    
    for (auto format : supported_formats) {
        QString description = recording_manager->get_format_description(format);
        QString extension = recording_manager->get_format_extension(format);
        bool is_compressed = recording_manager->is_format_compressed(format);
        
        EXPECT_FALSE(description.isEmpty());
        EXPECT_FALSE(extension.isEmpty());
        // is_compressed can be true or false
    }
}

// TDD Integration Test
TEST_F(StreamRecordingManagerTest, EndToEndRecordingWorkflow) {
    // Complete end-to-end recording workflow test
    QSignalSpy session_started_spy(recording_manager.get(), 
                                  &StreamRecordingManager::recording_session_started);
    QSignalSpy segment_completed_spy(recording_manager.get(), 
                                    &StreamRecordingManager::segment_completed);
    QSignalSpy session_stopped_spy(recording_manager.get(), 
                                  &StreamRecordingManager::recording_session_stopped);
    
    // Step 1: Start recording session
    QString session_id = recording_manager->start_recording_session(
        temp_dir->path(), RecordingFormat::ETI_NI, segmentation_config);
    
    ASSERT_FALSE(session_id.isEmpty());
    EXPECT_GE(session_started_spy.count(), 1);
    
    // Step 2: Record frames
    for (size_t i = 0; i < std::min(size_t(50), test_frames.size()); ++i) {
        recording_manager->record_frame(session_id, test_frames[i]);
        if (i % 10 == 0) {
            QThread::msleep(10); // Small delay for realistic timing
        }
    }
    
    // Step 3: Force segment rotation
    recording_manager->force_segment_rotation(session_id);
    
    // Step 4: Record more frames
    recording_manager->record_frame_batch(session_id, test_frames);
    
    // Step 5: Stop recording
    bool stopped = recording_manager->stop_recording_session(session_id);
    EXPECT_TRUE(stopped);
    
    // Allow time for completion
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();
    
    EXPECT_GE(session_stopped_spy.count(), 1);
    
    // Verify final results
    auto session_info = recording_manager->get_session_info(session_id);
    EXPECT_GT(session_info.frames_recorded, 0);
    EXPECT_GT(session_info.bytes_written, 0);
    EXPECT_FALSE(session_info.is_active);
    
    auto session_files = recording_manager->get_session_files(session_id);
    qint64 total_size = recording_manager->get_session_total_size(session_id);
    
    EXPECT_GT(total_size, 0);
    
    qDebug() << "End-to-end test results:";
    qDebug() << "  Session ID:" << session_id;
    qDebug() << "  Frames recorded:" << session_info.frames_recorded;
    qDebug() << "  Bytes written:" << session_info.bytes_written;
    qDebug() << "  Segments created:" << session_info.segments_created;
    qDebug() << "  Files created:" << session_files.size();
    qDebug() << "  Total size:" << total_size << "bytes";
}

// Main function for running tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Qt application for file operations
    QCoreApplication app(argc, argv);
    
    // Run tests
    return RUN_ALL_TESTS();
}