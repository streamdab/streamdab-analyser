/**
 * @file test_gui_stability.cpp
 * @brief GUI stability tests for ETI Stream Analyser
 */

#include <QApplication>
#include <QTest>
#include <QTimer>
#include <memory>

#include "gui/main_window.h"
#include "utils/logger.h"

class TestGUIStability : public QObject
{
    Q_OBJECT

public:
    TestGUIStability() = default;

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testMainWindowInitialization();
    void testWindowClose();

private:
    std::unique_ptr<MainWindow> m_mainWindow;
};

void TestGUIStability::initTestCase()
{
    Logger::instance().setLogLevel(Logger::Warning);
}

void TestGUIStability::cleanupTestCase()
{
    m_mainWindow.reset();
}

void TestGUIStability::testMainWindowInitialization()
{
    // Test that MainWindow can be created and initialized without crashing
    m_mainWindow = std::make_unique<MainWindow>();
    QVERIFY(m_mainWindow != nullptr);
    
    // Initialize the window (this was causing segmentation fault)
    bool initResult = m_mainWindow->initialize();
    QVERIFY(initResult == true);
    
    // Verify the window is in a valid state
    QVERIFY(m_mainWindow->isVisible() == false); // Window not shown yet
}

void TestGUIStability::testWindowClose()
{
    if (m_mainWindow) {
        m_mainWindow->close();
        // Should not crash during close
        QVERIFY(true);
    }
}

QTEST_MAIN(TestGUIStability)
#include "test_gui_stability.moc"