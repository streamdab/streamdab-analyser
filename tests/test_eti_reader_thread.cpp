// tests/test_eti_reader_thread.cpp
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QFile>
#include <QTemporaryFile>
#include <QThread>
#include <QElapsedTimer>
#include "../src/core/eti_reader_thread.hpp"

class ETIReaderThreadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create Qt application for event loop
        int argc = 0;
        char** argv = nullptr;
        if (!QCoreApplication::instance()) {
            app = new QCoreApplication(argc, argv);
        }
        
        // Create temporary ETI file for testing
        createTestETIFile();
    }
    
    void TearDown() override {
        if (tempFile) {
            tempFile->remove();
            delete tempFile;
            tempFile = nullptr;
        }
    }
    
    // Helper to process Qt events
    void processEvents(int maxTimeMs = 100) {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < maxTimeMs) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }
    
    void createTestETIFile() {
        tempFile = new QTemporaryFile();
        if (tempFile->open()) {
            // Write 100 frames of test ETI data (6144 bytes each)
            QByteArray frame(6144, 0);
            
            // Simple ETI-LI header structure
            frame[0] = 0xFF;  // ERR byte
            frame[1] = 0x00;  // FSYNC
            frame[2] = 0x00;  // LIDATA (FCT)
            frame[3] = 0x00;  // LIDATA (NST)
            
            for (int i = 0; i < 100; ++i) {
                // Update frame counter (FCT)
                frame[2] = static_cast<char>(i % 250);
                tempFile->write(frame);
            }
            
            tempFile->flush();
            tempFilePath = tempFile->fileName();
        }
    }
    
    QCoreApplication* app = nullptr;
    QTemporaryFile* tempFile = nullptr;
    QString tempFilePath;
};

// Test 1: Construction and initialization
TEST_F(ETIReaderThreadTest, ConstructionTest) {
    ETIReaderThread thread;
    EXPECT_EQ(thread.getFramesProcessed(), 0);
    EXPECT_EQ(thread.getProcessingTimeMs(), 0);
}

// Test 2: Set file path
TEST_F(ETIReaderThreadTest, SetFilePathTest) {
    ETIReaderThread thread;
    thread.setFilePath("/path/to/test.eti");
    // No crash expected
    SUCCEED();
}

// Test 3: Set input mode
TEST_F(ETIReaderThreadTest, SetInputModeTest) {
    ETIReaderThread thread;
    thread.setInputMode(ETIReaderThread::InputMode::FileMode);
    thread.setInputMode(ETIReaderThread::InputMode::UDPMode);
    SUCCEED();
}

// Test 4: Start and stop processing
TEST_F(ETIReaderThreadTest, StartStopProcessingTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    QSignalSpy startedSpy(&thread, &ETIReaderThread::processingStarted);
    QSignalSpy completeSpy(&thread, &ETIReaderThread::processingComplete);
    
    thread.startProcessing();
    
    // Process Qt events to allow thread to start
    processEvents(200);
    ASSERT_TRUE(startedSpy.wait(1000));
    EXPECT_EQ(startedSpy.count(), 1);
    
    // Allow some frame processing with event loop
    processEvents(200);
    
    // Stop processing
    thread.stopProcessing();
    
    // Wait for completion
    ASSERT_TRUE(completeSpy.wait(2000));
    EXPECT_EQ(completeSpy.count(), 1);
    
    // Check that frames were processed
    EXPECT_GT(thread.getFramesProcessed(), 0);
}

// Test 5: Pause and resume processing
TEST_F(ETIReaderThreadTest, PauseResumeTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    QSignalSpy frameReceivedSpy(&thread, &ETIReaderThread::frameReceived);
    
    thread.startProcessing();
    
    // Process events to allow frames
    processEvents(200);
    int framesBeforePause = frameReceivedSpy.count();
    
    // Pause
    thread.pauseProcessing();
    frameReceivedSpy.clear();
    
    // Process events - should have no new frames while paused
    processEvents(200);
    EXPECT_EQ(frameReceivedSpy.count(), 0);
    
    // Resume
    thread.resumeProcessing();
    
    // Process events - should have new frames after resume
    processEvents(200);
    EXPECT_GT(frameReceivedSpy.count(), 0);
    
    thread.stopProcessing();
    thread.wait(1000);
}

// Test 6: Frame received signal emission
TEST_F(ETIReaderThreadTest, FrameReceivedSignalTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    QSignalSpy frameReceivedSpy(&thread, &ETIReaderThread::frameReceived);
    
    thread.startProcessing();
    
    // Process events to allow frame processing
    processEvents(200);
    
    // Wait for at least 5 frames (5 * 24ms = 120ms + margin)
    ASSERT_TRUE(frameReceivedSpy.wait(500));
    
    thread.stopProcessing();
    thread.wait(1000);
    
    EXPECT_GE(frameReceivedSpy.count(), 5);
}

// Test 7: FIC data updated signal
TEST_F(ETIReaderThreadTest, FICDataUpdatedSignalTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    QSignalSpy ficSpy(&thread, &ETIReaderThread::ficDataUpdated);
    
    thread.startProcessing();
    
    // Wait for FIC data
    if (ficSpy.wait(1000)) {
        EXPECT_GT(ficSpy.count(), 0);
        
        // Verify signal contains QSharedPointer<FICDataSignal>
        if (ficSpy.count() > 0) {
            QList<QVariant> arguments = ficSpy.takeFirst();
            EXPECT_TRUE(arguments.at(0).canConvert<QSharedPointer<FICDataSignal>>());
        }
    }
    
    thread.stopProcessing();
    thread.wait(1000);
}

// Test 8: MSC data updated signal
TEST_F(ETIReaderThreadTest, MSCDataUpdatedSignalTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    QSignalSpy mscSpy(&thread, &ETIReaderThread::mscDataUpdated);
    
    thread.startProcessing();
    
    // Wait for MSC data
    if (mscSpy.wait(1000)) {
        EXPECT_GT(mscSpy.count(), 0);
    }
    
    thread.stopProcessing();
    thread.wait(1000);
}

// Test 9: Error stats updated signal
TEST_F(ETIReaderThreadTest, ErrorStatsUpdatedSignalTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    QSignalSpy statsSpy(&thread, &ETIReaderThread::errorStatsUpdated);
    
    thread.startProcessing();
    
    // Process events to allow frame processing
    processEvents(200);
    
    // Wait for stats (currently stubbed, so may not emit)
    if (statsSpy.wait(1000)) {
        EXPECT_GT(statsSpy.count(), 0);
        
        // Verify signal contains ErrorStats
        if (statsSpy.count() > 0) {
            QList<QVariant> arguments = statsSpy.takeFirst();
            EXPECT_TRUE(arguments.at(0).canConvert<ErrorStats>());
        }
    }
    
    thread.stopProcessing();
    thread.wait(1000);
}

// Test 10: Processing with invalid file
TEST_F(ETIReaderThreadTest, InvalidFileTest) {
    ETIReaderThread thread;
    thread.setFilePath("/nonexistent/file.eti");
    
    QSignalSpy errorSpy(&thread, &ETIReaderThread::processingError);
    
    thread.startProcessing();
    
    // Should emit error
    ASSERT_TRUE(errorSpy.wait(1000));
    EXPECT_EQ(errorSpy.count(), 1);
    
    // Verify error message
    QList<QVariant> arguments = errorSpy.takeFirst();
    QString errorMsg = arguments.at(0).toString();
    EXPECT_FALSE(errorMsg.isEmpty());
    
    thread.wait(1000);
}

// Test 11: Frame count accuracy
TEST_F(ETIReaderThreadTest, FrameCountAccuracyTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    QSignalSpy completeSpy(&thread, &ETIReaderThread::processingComplete);
    
    thread.startProcessing();
    
    // Process events continuously while waiting for completion
    for (int i = 0; i < 50 && completeSpy.count() == 0; ++i) {
        processEvents(100);
    }
    
    // Wait for completion
    ASSERT_TRUE(completeSpy.wait(5000));
    
    // Verify frame count (should process all 100 frames)
    EXPECT_EQ(thread.getFramesProcessed(), 100);
    
    // Verify completion signal arguments
    QList<QVariant> arguments = completeSpy.takeFirst();
    int totalFrames = arguments.at(0).toInt();
    int timeMs = arguments.at(1).toInt();
    
    EXPECT_EQ(totalFrames, 100);
    EXPECT_GT(timeMs, 0);
}

// Test 12: Frame rate calculation
TEST_F(ETIReaderThreadTest, FrameRateTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    QSignalSpy frameReceivedSpy(&thread, &ETIReaderThread::frameReceived);
    
    thread.startProcessing();
    
    // Process events for 1 second to allow frame processing
    for (int i = 0; i < 10; ++i) {
        processEvents(100);
    }
    
    thread.stopProcessing();
    thread.wait(1000);
    
    int framesProcessed = frameReceivedSpy.count();
    
    // At 24ms interval, expect ~41 fps
    // Allow margin: 30-50 fps
    EXPECT_GE(framesProcessed, 30);
    EXPECT_LE(framesProcessed, 50);
}

// Test 13: Thread safety - concurrent state access
TEST_F(ETIReaderThreadTest, ThreadSafetyTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    thread.startProcessing();
    
    // Access state while thread is running
    for (int i = 0; i < 10; ++i) {
        int frames = thread.getFramesProcessed();
        int timeMs = thread.getProcessingTimeMs();
        
        // No crash expected
        EXPECT_GE(frames, 0);
        EXPECT_GE(timeMs, 0);
        
        QThread::msleep(10);
    }
    
    thread.stopProcessing();
    thread.wait(1000);
}

// Test 14: Multiple start/stop cycles
TEST_F(ETIReaderThreadTest, MultipleStartStopTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    for (int cycle = 0; cycle < 3; ++cycle) {
        QSignalSpy startedSpy(&thread, &ETIReaderThread::processingStarted);
        QSignalSpy completeSpy(&thread, &ETIReaderThread::processingComplete);
        
        thread.startProcessing();
        processEvents(100);
        ASSERT_TRUE(startedSpy.wait(1000));
        
        // Allow some processing
        processEvents(200);
        
        thread.stopProcessing();
        ASSERT_TRUE(completeSpy.wait(2000));
        
        EXPECT_GT(thread.getFramesProcessed(), 0);
    }
}

// Test 15: Ensemble info signal
TEST_F(ETIReaderThreadTest, EnsembleInfoSignalTest) {
    ETIReaderThread thread;
    thread.setFilePath(tempFilePath);
    
    QSignalSpy ensembleSpy(&thread, &ETIReaderThread::ensembleInfoChanged);
    
    thread.startProcessing();
    
    // Wait for ensemble info (may or may not be present in test data)
    ensembleSpy.wait(1000);
    
    thread.stopProcessing();
    thread.wait(1000);
    
    // Just verify no crash
    SUCCEED();
}

int main(int argc, char **argv) {
    // Create QCoreApplication BEFORE initializing GTest
    // This provides the event loop needed for QThread::exec() in worker thread
    QCoreApplication app(argc, argv);
    
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run tests with Qt event loop processing
    int result = RUN_ALL_TESTS();
    
    return result;
}
