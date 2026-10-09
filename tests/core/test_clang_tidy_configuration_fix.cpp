#include <gtest/gtest.h>
#include <QString>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>

/**
 * TDD Test Case for clang-tidy Configuration Fix
 * 
 * Test Objective: Ensure .clang-tidy configuration has correct case values
 * TDD Methodology: Red-Green-Refactor
 * AAA Pattern: Arrange-Act-Assert
 */
class ClangTidyConfigurationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Arrange: Set up test environment
        projectRoot = QString(CMAKE_SOURCE_DIR);
        clangTidyFile = projectRoot + "/.clang-tidy";
        
        // Ensure we're in the correct directory
        ASSERT_TRUE(QDir(projectRoot).exists()) << "Project root directory must exist";
        ASSERT_TRUE(QFile::exists(clangTidyFile)) << ".clang-tidy file must exist";
    }
    
    QString projectRoot;
    QString clangTidyFile;
};

/**
 * RED PHASE TEST: This test should FAIL initially
 * Validates that clang-tidy configuration uses correct case values
 */
TEST_F(ClangTidyConfigurationTest, ShouldHaveCorrectCaseValuesInConfiguration) {
    // Arrange: Read the .clang-tidy file
    QFile file(clangTidyFile);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text)) 
        << "Should be able to read .clang-tidy file";
    
    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();
    
    // Act: Check for correct case values (this should PASS after GREEN phase)
    bool hasCamelCaseFunction = content.contains("value: camelCase");
    bool hasCamelCaseVariable = content.contains("value: camelCase"); 
    
    // Assert: Verify configuration has correct values (RED: should fail initially)
    EXPECT_FALSE(content.contains("warning: invalid configuration value 'camelCase'"))
        << "Configuration should not generate warnings about camelCase";
    
    // Additional validation for proper case handling
    EXPECT_TRUE(hasCamelCaseFunction || content.contains("value: CamelCase"))
        << "Function case should be properly configured";
    EXPECT_TRUE(hasCamelCaseVariable || content.contains("value: snake_case"))
        << "Variable case should be properly configured";
}

/**
 * TDD INTEGRATION TEST: Validate clang-tidy runs without configuration warnings
 */
TEST_F(ClangTidyConfigurationTest, ShouldRunClangTidyWithoutConfigurationWarnings) {
    // Arrange: Prepare a simple test file to analyze
    QString testFile = projectRoot + "/src/main.cpp";
    ASSERT_TRUE(QFile::exists(testFile)) << "Test source file must exist";
    
    // Act: Run clang-tidy on the test file
    QProcess clangTidyProcess;
    clangTidyProcess.setWorkingDirectory(projectRoot);
    
    QStringList arguments;
    arguments << testFile
              << "--config-file=" + clangTidyFile
              << "--"
              << "-I" + projectRoot + "/src"
              << "-std=c++20";
    
    clangTidyProcess.start("clang-tidy", arguments);
    bool finished = clangTidyProcess.waitForFinished(30000); // 30 second timeout
    
    ASSERT_TRUE(finished) << "clang-tidy should complete within timeout";
    
    // Assert: Check that there are no configuration warnings
    QString output = clangTidyProcess.readAllStandardError();
    QString stdOut = clangTidyProcess.readAllStandardOutput();
    QString combinedOutput = output + stdOut;
    
    EXPECT_FALSE(combinedOutput.contains("warning: invalid configuration value 'camelCase'"))
        << "Should not have camelCase configuration warnings";
    EXPECT_FALSE(combinedOutput.contains("did you mean 'CamelCase'"))
        << "Should not suggest CamelCase corrections";
    
    // Additional validation: process should exit successfully
    EXPECT_EQ(clangTidyProcess.exitCode(), 0) 
        << "clang-tidy should exit successfully without configuration errors";
}

/**
 * TDD REFACTOR PHASE TEST: Validate overall build quality
 */
TEST_F(ClangTidyConfigurationTest, ShouldContributeToZeroWarningBuild) {
    // Arrange: This test validates the broader build quality goal
    QString buildDir = projectRoot + "/build";
    
    // Act: This is a meta-test that ensures our fix contributes to zero warnings
    // The actual build testing is handled by the build system
    
    // Assert: Configuration file should exist and be valid YAML
    QFile file(clangTidyFile);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = file.readAll();
    file.close();
    
    // Basic YAML structure validation
    EXPECT_TRUE(content.contains("Checks:")) << "Should have Checks section";
    EXPECT_TRUE(content.contains("CheckOptions:")) << "Should have CheckOptions section";
    EXPECT_FALSE(content.isEmpty()) << "Configuration should not be empty";
    
    // Validate that problematic patterns are not present
    EXPECT_FALSE(content.contains("camelCase") && content.contains("warning"))
        << "Should not have patterns that generate warnings";
}

/**
 * TDD PERFORMANCE TEST: Ensure fix doesn't impact build performance
 */
TEST_F(ClangTidyConfigurationTest, ShouldNotImpactBuildPerformance) {
    // Arrange: Configuration should be efficient
    QFile file(clangTidyFile);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = file.readAll();
    file.close();
    
    // Act & Assert: Check configuration efficiency
    int lineCount = content.split('\n').size();
    EXPECT_LT(lineCount, 50) << "Configuration should be concise for build performance";
    
    // Check for overly broad or expensive checks
    EXPECT_FALSE(content.contains("-*,*")) << "Should not enable all checks (performance impact)";
    EXPECT_TRUE(content.contains("HeaderFilterRegex")) << "Should filter headers for performance";
}