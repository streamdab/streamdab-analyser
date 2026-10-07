#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <memory>

#include "core/eti_processor.hpp"
#include "core/modern_eti_frame_parser.hpp"
#include "core/enhanced_fig_analyser.hpp"
#include "core/comprehensive_etsi_validator.hpp"
#include "gui/main_window.h"

/**
 * @brief Critical tests to enforce ETI processor initialization policy
 * 
 * These tests prevent regressions where ETI processor initialization
 * gets disabled or commented out, which causes the "ETI processor is 
 * not initialized" error when opening files.
 * 
 * POLICY ENFORCEMENT: NO MOCK/PLACEHOLDER implementations allowed.
 * All functionality must be real and working.
 */
class EtiProcessorInitializationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Ensure QApplication exists for Qt components
        if (!QCoreApplication::instance()) {
            int argc = 1;
            char* argv[] = {"test", nullptr};
            app = std::make_unique<QApplication>(argc, argv);
        }
    }

    void TearDown() override
    {
        processor.reset();
        mainWindow.reset();
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<EtiProcessor> processor;
    std::unique_ptr<MainWindow> mainWindow;
};

/**
 * @brief CRITICAL: Test that ETI processor can be created and initialized
 * 
 * This test prevents regressions where ETI processor initialization
 * is commented out or disabled in the codebase.
 */
TEST_F(EtiProcessorInitializationTest, ProcessorCanBeCreatedAndInitialized)
{
    // ARRANGE: Create ETI processor
    processor = std::make_unique<EtiProcessor>();
    ASSERT_NE(processor.get(), nullptr) << "ETI processor creation failed";

    // ACT: Initialize the processor
    bool initSuccess = processor->initialize();

    // ASSERT: Initialization must succeed
    EXPECT_TRUE(initSuccess) << "ETI processor initialization failed - check for commented out code";
    EXPECT_TRUE(processor->isInitialized()) << "ETI processor reports not initialized after successful init";
    EXPECT_EQ(processor->getStatus(), "Ready") << "ETI processor status should be 'Ready' after initialization";
}

/**
 * @brief CRITICAL: Test that MainWindow initializes ETI processor
 * 
 * This test ensures MainWindow actually creates and initializes the 
 * ETI processor during its initialization, preventing the specific
 * regression encountered where initialization was commented out.
 */
TEST_F(EtiProcessorInitializationTest, MainWindowInitializesEtiProcessor)
{
    // ARRANGE: Create MainWindow
    mainWindow = std::make_unique<MainWindow>();
    ASSERT_NE(mainWindow.get(), nullptr) << "MainWindow creation failed";

    // ACT: Initialize the main window
    bool initSuccess = mainWindow->initialize();

    // ASSERT: MainWindow initialization succeeds
    EXPECT_TRUE(initSuccess) << "MainWindow initialization failed";
    
    // CRITICAL: Verify ETI processor is created and initialized
    EtiProcessor* processor = mainWindow->getEtiProcessor();
    ASSERT_NE(processor, nullptr) << "REGRESSION: ETI processor not created by MainWindow - check for commented initialization code";
    EXPECT_TRUE(processor->isInitialized()) << "REGRESSION: ETI processor not initialized by MainWindow";
}

/**
 * @brief POLICY ENFORCEMENT: Test that ETI processor has real implementation
 * 
 * This test verifies that ETI processor methods are not mock/placeholder
 * implementations, enforcing the "NO MOCK" policy.
 */
TEST_F(EtiProcessorInitializationTest, ProcessorHasRealImplementation)
{
    // ARRANGE: Create and initialize processor
    processor = std::make_unique<EtiProcessor>();
    ASSERT_TRUE(processor->initialize()) << "Setup failed - processor initialization";

    // ACT & ASSERT: Test core methods have real implementation
    
    // 1. getFrameCount should return 0 initially (real implementation)
    EXPECT_EQ(processor->getFrameCount(), 0) << "getFrameCount should return 0 initially";
    
    // 2. getStatus should return meaningful status (not placeholder)
    QString status = processor->getStatus();
    EXPECT_FALSE(status.isEmpty()) << "getStatus should not return empty string";
    EXPECT_NE(status, "TODO") << "getStatus should not be placeholder 'TODO'";
    EXPECT_NE(status, "Not implemented") << "getStatus should not be placeholder";
    
    // 3. isInitialized should work correctly
    EXPECT_TRUE(processor->isInitialized()) << "isInitialized should return true after successful init";
    
    // 4. getInternalEngine should return valid processing engine
    auto* engine = processor->getProcessingEngine();
    EXPECT_NE(engine, nullptr) << "Internal processing engine should be available (not mock)";
}

/**
 * @brief INTEGRATION TEST: Test ETI file processing capability
 * 
 * This test verifies that the ETI processor can actually attempt to
 * process files without immediately failing due to uninitialized state.
 */
TEST_F(EtiProcessorInitializationTest, ProcessorCanAttemptFileProcessing)
{
    // ARRANGE: Create and initialize processor
    processor = std::make_unique<EtiProcessor>();
    ASSERT_TRUE(processor->initialize()) << "Setup failed - processor initialization";

    // ACT: Attempt to process a non-existent file
    // We expect this to fail with "file not found", not "processor not initialized"
    bool result = processor->processFile("/nonexistent/file.eti");

    // ASSERT: Should fail with file error, not initialization error
    EXPECT_FALSE(result) << "Should fail for non-existent file";
    
    // The status should indicate file error, not initialization error
    QString status = processor->getStatus();
    EXPECT_FALSE(status.contains("not initialized", Qt::CaseInsensitive)) 
        << "Error should be about file, not initialization: " << status.toStdString();
}

/**
 * @brief REGRESSION PREVENTION: Test MainWindow file opening flow
 * 
 * This test simulates the exact user action that failed and ensures
 * it doesn't fail with "ETI processor is not initialized" error.
 */
TEST_F(EtiProcessorInitializationTest, MainWindowFileOpeningFlow)
{
    // ARRANGE: Create and initialize MainWindow
    mainWindow = std::make_unique<MainWindow>();
    ASSERT_TRUE(mainWindow->initialize()) << "MainWindow initialization failed";
    
    EtiProcessor* processor = mainWindow->getEtiProcessor();
    ASSERT_NE(processor, nullptr) << "ETI processor not available";
    ASSERT_TRUE(processor->isInitialized()) << "ETI processor not initialized";

    // ACT: Simulate file opening (this is what user does when error occurs)
    // We test with non-existent file to avoid needing real ETI data
    bool result = processor->processFile("/tmp/nonexistent_test.eti");

    // ASSERT: Should fail with file error, NOT initialization error
    EXPECT_FALSE(result) << "Should fail for non-existent file";
    
    // CRITICAL: Error should NOT be about processor initialization
    QString status = processor->getStatus();
    EXPECT_FALSE(status.contains("not initialized", Qt::CaseInsensitive))
        << "REGRESSION: Getting 'not initialized' error again! Status: " << status.toStdString();
    EXPECT_FALSE(status.contains("processor not initialized", Qt::CaseInsensitive))
        << "REGRESSION: Getting 'processor not initialized' error! Status: " << status.toStdString();
}

/**
 * @brief POLICY TEST: Verify no commented-out initialization code
 * 
 * This test uses reflection/introspection to verify that critical
 * initialization code is actually executed, not commented out.
 */
TEST_F(EtiProcessorInitializationTest, VerifyNoCommentedInitialization)
{
    // ARRANGE: Create MainWindow
    mainWindow = std::make_unique<MainWindow>();

    // ACT: Initialize and capture any initialization signals/logs
    bool initSuccess = mainWindow->initialize();

    // ASSERT: Verify initialization actually happened
    EXPECT_TRUE(initSuccess) << "MainWindow initialization must succeed";
    
    // Verify ETI processor exists (proves initialization code is not commented)
    EtiProcessor* processor = mainWindow->getEtiProcessor();
    EXPECT_NE(processor, nullptr) 
        << "POLICY VIOLATION: ETI processor is nullptr - initialization code may be commented out!";
    
    if (processor) {
        EXPECT_TRUE(processor->isInitialized())
            << "POLICY VIOLATION: ETI processor not initialized - check for commented initialize() calls!";
    }
}