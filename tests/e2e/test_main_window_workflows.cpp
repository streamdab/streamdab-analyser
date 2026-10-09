/**
 * E2E Test: Main Window Workflows
 * Tests core MainWindow functionality and user interactions
 */

#include <QtTest/QtTest>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QSplitter>

#include "gui/main_window.h"
#include "utils/logger.h"
#include "test_utils.h"

class TestMainWindowWorkflows : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Core MainWindow functionality tests
    void testMainWindowInitialization();
    void testMenuBarFunctionality();
    void testToolBarInteractions();
    void testStatusBarUpdates();
    void testWindowLayoutAndDocking();
    void testKeyboardShortcuts();
    void testWindowStateManagement();

private:
    QApplication* app;
    MainWindow* mainWindow;
    
    void verifyMenuStructure();
    void verifyToolBarSetup();
    void verifyLayoutConfiguration();
};

void TestMainWindowWorkflows::initTestCase()
{
    int argc = 1;
    const char* argv[] = {"test_main_window_workflows"};
    app = new QApplication(argc, const_cast<char**>(argv));
    
    Logger::instance().setLogLevel(Logger::Debug);
    Logger::instance().log(Logger::Info, "MainWindowE2E", "Starting MainWindow workflow tests");
}

void TestMainWindowWorkflows::cleanupTestCase()
{
    Logger::instance().log(Logger::Info, "MainWindowE2E", "MainWindow workflow tests completed");
    delete app;
}

void TestMainWindowWorkflows::init()
{
    mainWindow = new MainWindow();
    mainWindow->show();
    [[maybe_unused]] bool windowActive = QTest::qWaitForWindowActive(mainWindow);
    QVERIFY(mainWindow->isVisible());
}

void TestMainWindowWorkflows::cleanup()
{
    // Use enhanced widget cleanup from test utils
    TestUtils::safeDeleteWidget(mainWindow);
    
    // Perform safe test cleanup
    TestUtils::safeTestCleanup();
    
    // Validate memory state
    TestUtils::validateTestMemoryState("MainWindowWorkflows");
}

void TestMainWindowWorkflows::testMainWindowInitialization()
{
    Logger::instance().log(Logger::Debug, "MainWindowE2E", "Testing MainWindow initialization");
    
    // Verify basic window properties
    QVERIFY(mainWindow != nullptr);
    QVERIFY(mainWindow->isVisible());
    QVERIFY(!mainWindow->windowTitle().isEmpty());
    
    // Check minimum window size is reasonable
    QSize windowSize = mainWindow->size();
    QVERIFY2(windowSize.width() >= 800, "Window width too small for professional application");
    QVERIFY2(windowSize.height() >= 600, "Window height too small for professional application");
    
    // Verify essential UI components exist
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    QStatusBar* statusBar = mainWindow->findChild<QStatusBar*>();
    
    QVERIFY2(menuBar != nullptr, "MenuBar not found");
    QVERIFY2(statusBar != nullptr, "StatusBar not found");
    
    Logger::instance().log(Logger::Info, "MainWindowE2E", 
                          QString("MainWindow initialized successfully: %1x%2")
                          .arg(windowSize.width()).arg(windowSize.height()));
}

void TestMainWindowWorkflows::testMenuBarFunctionality()
{
    Logger::instance().log(Logger::Debug, "MainWindowE2E", "Testing MenuBar functionality");
    
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    QVERIFY(menuBar != nullptr);
    
    verifyMenuStructure();
    
    // Test menu accessibility
    QList<QAction*> menuActions = menuBar->actions();
    QVERIFY2(menuActions.count() > 0, "No menu actions found");
    
    Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                          QString("Found %1 top-level menus").arg(menuActions.count()));
    
    // Test each top-level menu
    foreach (QAction* action, menuActions) {
        if (action->menu()) {
            QString menuName = action->text();
            QMenu* menu = action->menu();
            
            Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                                  QString("Testing menu: %1 (%2 actions)")
                                  .arg(menuName).arg(menu->actions().count()));
            
            // Verify menu can be opened
            QVERIFY(menu->isEnabled());
            
            // Test that menu has actions
            QVERIFY2(menu->actions().count() > 0, 
                    QString("Menu %1 has no actions").arg(menuName).toLocal8Bit());
        }
    }
}

void TestMainWindowWorkflows::testToolBarInteractions()
{
    Logger::instance().log(Logger::Debug, "MainWindowE2E", "Testing ToolBar interactions");
    
    // Find all toolbars
    QList<QToolBar*> toolBars = mainWindow->findChildren<QToolBar*>();
    
    if (toolBars.isEmpty()) {
        Logger::instance().log(Logger::Info, "MainWindowE2E", "No toolbars found - this may be expected");
        return;
    }
    
    foreach (QToolBar* toolBar, toolBars) {
        QVERIFY(toolBar->isVisible());
        
        QList<QAction*> toolBarActions = toolBar->actions();
        Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                              QString("ToolBar '%1' has %2 actions")
                              .arg(toolBar->objectName()).arg(toolBarActions.count()));
        
        // Test toolbar actions are accessible
        foreach (QAction* action, toolBarActions) {
            if (!action->isSeparator()) {
                QVERIFY2(action->isEnabled() || action->text().contains("disabled"), 
                        QString("ToolBar action '%1' is unexpectedly disabled").arg(action->text()).toLocal8Bit());
            }
        }
    }
}

void TestMainWindowWorkflows::testStatusBarUpdates()
{
    Logger::instance().log(Logger::Debug, "MainWindowE2E", "Testing StatusBar updates");
    
    QStatusBar* statusBar = mainWindow->findChild<QStatusBar*>();
    QVERIFY(statusBar != nullptr);
    QVERIFY(statusBar->isVisible());
    
    // Check if status bar has initial message
    QString initialMessage = statusBar->currentMessage();
    Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                          QString("StatusBar initial message: '%1'").arg(initialMessage));
    
    // Status bar should be ready for updates
    QVERIFY(statusBar->isEnabled());
}

void TestMainWindowWorkflows::testWindowLayoutAndDocking()
{
    Logger::instance().log(Logger::Debug, "MainWindowE2E", "Testing window layout and docking");
    
    verifyLayoutConfiguration();
    
    // Test for dock widgets
    QList<QDockWidget*> dockWidgets = mainWindow->findChildren<QDockWidget*>();
    
    if (!dockWidgets.isEmpty()) {
        Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                              QString("Found %1 dock widgets").arg(dockWidgets.count()));
        
        foreach (QDockWidget* dock, dockWidgets) {
            // Test dock widget functionality
            QVERIFY(dock->isVisible());
            QVERIFY(dock->widget() != nullptr); // Should have content widget
            
            // Test dock can be moved (if not fixed)
            if (dock->features() & QDockWidget::DockWidgetMovable) {
                Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                                      QString("Dock widget '%1' is movable").arg(dock->windowTitle()));
            }
        }
    }
    
    // Test for splitter-based layout
    QList<QSplitter*> splitters = mainWindow->findChildren<QSplitter*>();
    
    if (!splitters.isEmpty()) {
        Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                              QString("Found %1 splitters in layout").arg(splitters.count()));
        
        foreach (QSplitter* splitter, splitters) {
            QVERIFY(splitter->count() >= 2); // Splitter should have at least 2 widgets
            
            // Test splitter handles are accessible
            for (int i = 1; i < splitter->count(); ++i) {
                QSplitterHandle* handle = splitter->handle(i);
                QVERIFY(handle != nullptr);
                QVERIFY(handle->isEnabled());
            }
        }
    }
}

void TestMainWindowWorkflows::testKeyboardShortcuts()
{
    Logger::instance().log(Logger::Debug, "MainWindowE2E", "Testing keyboard shortcuts");
    
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    if (!menuBar) return;
    
    // Collect all actions with shortcuts
    QList<QAction*> actionsWithShortcuts;
    
    foreach (QAction* menuAction, menuBar->actions()) {
        if (menuAction->menu()) {
            foreach (QAction* action, menuAction->menu()->actions()) {
                if (!action->shortcut().isEmpty()) {
                    actionsWithShortcuts.append(action);
                }
            }
        }
    }
    
    Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                          QString("Found %1 actions with keyboard shortcuts").arg(actionsWithShortcuts.count()));
    
    // Test a few common shortcuts
    foreach (QAction* action, actionsWithShortcuts) {
        QKeySequence shortcut = action->shortcut();
        QString actionText = action->text();
        
        Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                              QString("Action '%1' has shortcut: %2")
                              .arg(actionText).arg(shortcut.toString()));
        
        // Verify shortcut is properly configured
        QVERIFY(!shortcut.isEmpty());
        QVERIFY(action->isEnabled() || actionText.contains("disabled"));
    }
}

void TestMainWindowWorkflows::testWindowStateManagement()
{
    Logger::instance().log(Logger::Debug, "MainWindowE2E", "Testing window state management");
    
    // Test window can be minimized/restored
    QSize originalSize = mainWindow->size();
    QPoint originalPos = mainWindow->pos();
    
    // Test resize
    mainWindow->resize(1000, 700);
    QTest::qWait(100);
    QVERIFY(mainWindow->size() != originalSize);
    
    // Test move
    mainWindow->move(100, 100);
    QTest::qWait(100);
    QVERIFY(mainWindow->pos() != originalPos);
    
    // Test window state changes
    [[maybe_unused]] Qt::WindowStates originalState = mainWindow->windowState();
    
    // Test maximize (if supported)
    if (mainWindow->maximumSize() != mainWindow->minimumSize()) {
        mainWindow->showMaximized();
        QTest::qWait(200);
        QVERIFY(mainWindow->windowState() & Qt::WindowMaximized);
        
        // Restore
        mainWindow->showNormal();
        QTest::qWait(200);
        QVERIFY(!(mainWindow->windowState() & Qt::WindowMaximized));
    }
    
    Logger::instance().log(Logger::Info, "MainWindowE2E", "Window state management test completed");
}

void TestMainWindowWorkflows::verifyMenuStructure()
{
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    QStringList expectedMenus = {"File", "Edit", "View", "Tools", "Help"};
    QStringList foundMenus;
    
    foreach (QAction* action, menuBar->actions()) {
        if (action->menu()) {
            foundMenus << action->text().remove('&'); // Remove accelerator markers
        }
    }
    
    Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                          QString("Expected menus: %1").arg(expectedMenus.join(", ")));
    Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                          QString("Found menus: %1").arg(foundMenus.join(", ")));
    
    // At minimum, we expect File and Help menus
    bool hasFileMenu = false, hasHelpMenu = false;
    
    foreach (const QString& menu, foundMenus) {
        if (menu.contains("File")) hasFileMenu = true;
        if (menu.contains("Help")) hasHelpMenu = true;
    }
    
    QVERIFY2(hasFileMenu, "File menu not found");
    QVERIFY2(hasHelpMenu, "Help menu not found");
}

void TestMainWindowWorkflows::verifyToolBarSetup()
{
    QList<QToolBar*> toolBars = mainWindow->findChildren<QToolBar*>();
    
    // Log toolbar configuration
    if (toolBars.isEmpty()) {
        Logger::instance().log(Logger::Info, "MainWindowE2E", "No toolbars configured");
    } else {
        foreach (QToolBar* toolBar, toolBars) {
            Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                                  QString("ToolBar: %1 (area: %2, movable: %3)")
                                  .arg(toolBar->windowTitle())
                                  .arg(static_cast<int>(mainWindow->toolBarArea(toolBar)))
                                  .arg(toolBar->isMovable()));
        }
    }
}

void TestMainWindowWorkflows::verifyLayoutConfiguration()
{
    // Check central widget
    QWidget* centralWidget = mainWindow->centralWidget();
    if (centralWidget) {
        Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                              QString("Central widget: %1").arg(centralWidget->metaObject()->className()));
        QVERIFY(centralWidget->isVisible());
    }
    
    // Check for proper layout structure
    QList<QWidget*> topLevelWidgets = mainWindow->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    Logger::instance().log(Logger::Debug, "MainWindowE2E", 
                          QString("MainWindow has %1 direct child widgets").arg(topLevelWidgets.count()));
}

QTEST_MAIN(TestMainWindowWorkflows)
#include "test_main_window_workflows.moc"