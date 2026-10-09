#ifndef GUI_TEST_FRAMEWORK_H
#define GUI_TEST_FRAMEWORK_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <memory>

/**
 * @brief Professional GUI testing framework for ETI Stream Analyser
 * 
 * Provides comprehensive testing capabilities for Qt GUI components
 * with focus on broadcast industry requirements and 60 FPS performance.
 */
class GuiTestFramework : public QObject
{
    Q_OBJECT

public:
    explicit GuiTestFramework(QObject* parent = nullptr);
    ~GuiTestFramework();

    /**
     * @brief Initialize the test environment
     * @return true if initialization successful
     */
    bool initializeTestEnvironment();

    /**
     * @brief Clean up test environment
     */
    void cleanupTestEnvironment();

    /**
     * @brief Create a test widget for testing
     * @return Pointer to test widget
     */
    QWidget* createTestWidget();

    /**
     * @brief Validate 60 FPS GUI performance
     * @return true if GUI maintains 60 FPS
     */
    bool isGuiResponseTime60FPS();

private:
    bool m_initialized;
    std::unique_ptr<QWidget> m_testWidget;
    QTimer* m_performanceTimer;
};

#endif // GUI_TEST_FRAMEWORK_H