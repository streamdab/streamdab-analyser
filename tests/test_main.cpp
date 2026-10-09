/**
 * @file test_main.cpp
 * @brief Main test runner for ETI Stream Analyser unit tests
 * 
 * This file contains the main test runner that executes all unit tests
 * for the ETI Stream Analyser project using the Qt Test framework.
 * 
 * @author ETI Stream Analyser Team
 * @date 2024
 * @copyright Copyright (c) 2024 ETI Stream Analyser Team
 */

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>

// Test includes (will be implemented in next phase)
// #include "core/test_eti_processor.h"
// #include "core/test_dab_decoder.h"
// #include "core/test_fic_decoder.h"
// #include "gui/test_main_window.h"
// #include "utils/test_config_manager.h"
// #include "utils/test_file_utils.h"
// #include "utils/test_signal_processing.h"

// Temporary test class for build verification
class BuildSystemTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qDebug() << "ETI Stream Analyser Test Suite";
        qDebug() << "Qt Version:" << QT_VERSION_STR;
        qDebug() << "Build Type:" << 
#ifdef DEBUG_BUILD
            "Debug";
#else
            "Release";
#endif
    }

    void testQtFramework()
    {
        // Test Qt basic functionality
        QVERIFY(!QCoreApplication::applicationName().isEmpty());
        QCOMPARE(QCoreApplication::applicationName(), QString("ETI Stream Analyser Tests"));
    }

    void testApplicationDirectories()
    {
        // Test that standard paths are accessible
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QVERIFY(!appDataPath.isEmpty());
        
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QVERIFY(!tempPath.isEmpty());
        QVERIFY(QDir(tempPath).exists());
    }

    void testFileSystem()
    {
        // Test basic file system operations
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString testDir = tempDir + "/eti_analyser_test";
        
        QDir dir;
        QVERIFY(dir.mkpath(testDir));
        QVERIFY(QDir(testDir).exists());
        
        // Clean up
        QVERIFY(dir.rmpath(testDir));
    }

    void testBuildConfiguration()
    {
        // Verify build configuration
        qDebug() << "Testing build configuration...";
        
        // Test that we can create Qt objects
        QObject testObject;
        QVERIFY(testObject.metaObject() != nullptr);
        
        // Test string handling
        QString testString = "ETI Stream Analyser";
        QCOMPARE(testString.length(), 19);
        QVERIFY(testString.contains("ETI"));
        
        qDebug() << "Build configuration test passed";
    }

    void cleanupTestCase()
    {
        qDebug() << "Test suite completed successfully";
    }
};

/**
 * @brief Test runner registry
 * 
 * This class manages the registration and execution of all test classes.
 */
class TestRunner
{
public:
    static TestRunner& instance()
    {
        static TestRunner instance;
        return instance;
    }
    
    void registerTest(QObject* test, const QString& name)
    {
        m_tests.append(qMakePair(test, name));
    }
    
    int runAllTests(int argc, char* argv[])
    {
        int totalFailures = 0;
        
        qDebug() << "Running" << m_tests.size() << "test suites...";
        
        for (const auto& testPair : m_tests) {
            QObject* test = testPair.first;
            const QString& name = testPair.second;
            
            qDebug() << "\n=== Running test suite:" << name << "===";
            
            int result = QTest::qExec(test, argc, argv);
            if (result != 0) {
                totalFailures++;
                qWarning() << "Test suite" << name << "failed with code:" << result;
            } else {
                qDebug() << "Test suite" << name << "passed";
            }
        }
        
        qDebug() << "\n=== Test Summary ===";
        qDebug() << "Total test suites:" << m_tests.size();
        qDebug() << "Failed test suites:" << totalFailures;
        qDebug() << "Success rate:" << 
            QString::number((double)(m_tests.size() - totalFailures) / m_tests.size() * 100, 'f', 1) + "%";
        
        return totalFailures;
    }
    
private:
    QList<QPair<QObject*, QString>> m_tests;
};

/**
 * @brief Macro for easy test registration
 */
#define REGISTER_TEST(TestClass) \
    do { \
        TestClass* test = new TestClass(); \
        TestRunner::instance().registerTest(test, #TestClass); \
    } while(0)

/**
 * @brief Main test application entry point
 * 
 * Sets up the test environment and runs all registered test suites.
 * 
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @return Exit code (0 for success, non-zero for failures)
 */
int main(int argc, char *argv[])
{
    // Create Qt Core application for unit tests (no GUI needed)
    QCoreApplication app(argc, argv);
    
    // Set application properties for testing
    app.setApplicationName("ETI Stream Analyser Tests");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ETI Analyser Team");
    
    // Configure logging for tests
    QLoggingCategory::setFilterRules("*.debug=true");
    
    // Register test suites
    REGISTER_TEST(BuildSystemTest);
    
    // TODO: Register actual test classes when implemented
    // REGISTER_TEST(EtiProcessorTest);
    // REGISTER_TEST(DabDecoderTest);
    // REGISTER_TEST(FicDecoderTest);
    // REGISTER_TEST(MainWindowTest);
    // REGISTER_TEST(ConfigManagerTest);
    // REGISTER_TEST(FileUtilsTest);
    // REGISTER_TEST(SignalProcessingTest);
    
    // Run all tests
    int result = TestRunner::instance().runAllTests(argc, argv);
    
    if (result == 0) {
        qDebug() << "\nAll tests passed successfully!";
    } else {
        qWarning() << "\nSome tests failed. Check output above for details.";
    }
    
    return result;
}

// Include MOC file
#include "test_main.moc"