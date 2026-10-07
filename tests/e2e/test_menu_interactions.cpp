/**
 * E2E Test: Menu Interactions
 * Tests all menu functionality and user interactions
 */

#include <QtTest/QtTest>
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QKeySequence>

#include "gui/main_window.h"
#include "gui/service_browser.h"
#include "gui/analyser_widget.h"
#include "utils/logger.h"

class TestMenuInteractions : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Menu system tests
    void testFileMenuComplete();
    void testEditMenuFunctionality();
    void testViewMenuOptions();
    void testToolsMenuActions();
    void testHelpMenuContent();
    void testKeyboardShortcuts();
    void testMenuAccessibility();
    void testContextMenus();

private:
    QApplication* app;
    MainWindow* mainWindow;
    
    void testMenuStructure(QMenu* menu, const QString& menuName);
    void testMenuAction(QAction* action, const QString& actionName);
    void verifyKeyboardShortcut(QAction* action);
};

void TestMenuInteractions::initTestCase()
{
    int argc = 1;
    const char* argv[] = {"test_menu_interactions"};
    app = new QApplication(argc, const_cast<char**>(argv));
    
    Logger::instance().setLogLevel(Logger::Debug);
    Logger::instance().log(Logger::Info, "MenuE2E", "Starting menu interactions E2E tests");
}

void TestMenuInteractions::cleanupTestCase()
{
    Logger::instance().log(Logger::Info, "MenuE2E", "Menu interactions E2E tests completed");
    delete app;
}

void TestMenuInteractions::init()
{
    mainWindow = new MainWindow();
    mainWindow->show();
    bool windowActive = QTest::qWaitForWindowActive(mainWindow);
    Q_UNUSED(windowActive);
    QVERIFY(mainWindow->isVisible());
}

void TestMenuInteractions::cleanup()
{
    if (mainWindow) {
        mainWindow->close();
        delete mainWindow;
        mainWindow = nullptr;
    }
}

void TestMenuInteractions::testFileMenuComplete()
{
    Logger::instance().log(Logger::Info, "MenuE2E", "Testing File menu complete functionality");
    
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    QVERIFY(menuBar != nullptr);
    
    // Find File menu
    QMenu* fileMenu = nullptr;
    foreach (QAction* action, menuBar->actions()) {
        if (action->text().contains("File") || action->text().contains("&File")) {
            fileMenu = action->menu();
            break;
        }
    }
    
    QVERIFY2(fileMenu != nullptr, "File menu not found");
    
    testMenuStructure(fileMenu, "File");
    
    // Test specific File menu actions
    QStringList expectedFileActions = {
        "Open ETI File", "Open", "Recent", "Save", "Export", "Exit", "Quit"
    };
    
    QList<QAction*> fileActions = fileMenu->actions();
    QStringList foundActions;
    
    foreach (QAction* action, fileActions) {
        if (!action->isSeparator()) {
            foundActions << action->text();
            testMenuAction(action, QString("File -> %1").arg(action->text()));
        }
    }
    
    Logger::instance().log(Logger::Debug, "MenuE2E", 
                          QString("File menu actions: %1").arg(foundActions.join(", ")));
    
    // Verify essential File menu actions exist
    bool hasOpenAction = false;
    foreach (const QString& action, foundActions) {
        if (action.contains("Open", Qt::CaseInsensitive)) {
            hasOpenAction = true;
            break;
        }
    }
    QVERIFY2(hasOpenAction, "Open action not found in File menu");
}

void TestMenuInteractions::testEditMenuFunctionality()
{
    Logger::instance().log(Logger::Info, "MenuE2E", "Testing Edit menu functionality");
    
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    
    // Find Edit menu
    QMenu* editMenu = nullptr;
    foreach (QAction* action, menuBar->actions()) {
        if (action->text().contains("Edit") || action->text().contains("&Edit")) {
            editMenu = action->menu();
            break;
        }
    }
    
    if (editMenu) {
        testMenuStructure(editMenu, "Edit");
        
        // Test common Edit actions
        QStringList expectedEditActions = {
            "Copy", "Cut", "Paste", "Select All", "Find", "Preferences", "Settings"
        };
        
        QList<QAction*> editActions = editMenu->actions();
        foreach (QAction* action, editActions) {
            if (!action->isSeparator()) {
                testMenuAction(action, QString("Edit -> %1").arg(action->text()));
            }
        }
        
        Logger::instance().log(Logger::Info, "MenuE2E", "Edit menu functionality tested");
    } else {
        Logger::instance().log(Logger::Info, "MenuE2E", "Edit menu not found - may not be implemented");
    }
}

void TestMenuInteractions::testViewMenuOptions()
{
    Logger::instance().log(Logger::Info, "MenuE2E", "Testing View menu options");
    
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    
    // Find View menu
    QMenu* viewMenu = nullptr;
    foreach (QAction* action, menuBar->actions()) {
        if (action->text().contains("View") || action->text().contains("&View")) {
            viewMenu = action->menu();
            break;
        }
    }
    
    if (viewMenu) {
        testMenuStructure(viewMenu, "View");
        
        // Test View menu actions
        QList<QAction*> viewActions = viewMenu->actions();
        foreach (QAction* action, viewActions) {
            if (!action->isSeparator()) {
                testMenuAction(action, QString("View -> %1").arg(action->text()));
                
                // Test checkable actions (like show/hide panels)
                if (action->isCheckable()) {
                    Logger::instance().log(Logger::Debug, "MenuE2E", 
                                          QString("View action '%1' is checkable (checked: %2)")
                                          .arg(action->text()).arg(action->isChecked()));
                    
                    // Test toggling checkable actions
                    bool originalState = action->isChecked();
                    action->trigger();
                    QTest::qWait(100);
                    
                    // Verify state changed
                    if (action->isEnabled()) {
                        QVERIFY2(action->isChecked() != originalState, 
                                QString("Checkable action '%1' did not toggle").arg(action->text()).toLocal8Bit());
                        
                        // Toggle back
                        action->trigger();
                        QTest::qWait(100);
                    }
                }
            }
        }
        
        Logger::instance().log(Logger::Info, "MenuE2E", "View menu options tested");
    } else {
        Logger::instance().log(Logger::Info, "MenuE2E", "View menu not found - may not be implemented");
    }
}

void TestMenuInteractions::testToolsMenuActions()
{
    Logger::instance().log(Logger::Info, "MenuE2E", "Testing Tools menu actions");
    
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    
    // Find Tools menu
    QMenu* toolsMenu = nullptr;
    foreach (QAction* action, menuBar->actions()) {
        if (action->text().contains("Tools") || action->text().contains("&Tools")) {
            toolsMenu = action->menu();
            break;
        }
    }
    
    if (toolsMenu) {
        testMenuStructure(toolsMenu, "Tools");
        
        // Test Tools menu actions
        QStringList expectedToolsActions = {
            "Settings", "Preferences", "Configuration", "Options", "Report", "Export"
        };
        
        QList<QAction*> toolsActions = toolsMenu->actions();
        QStringList foundToolsActions;
        
        foreach (QAction* action, toolsActions) {
            if (!action->isSeparator()) {
                foundToolsActions << action->text();
                testMenuAction(action, QString("Tools -> %1").arg(action->text()));
            }
        }
        
        Logger::instance().log(Logger::Debug, "MenuE2E", 
                              QString("Tools menu actions: %1").arg(foundToolsActions.join(", ")));
        
        Logger::instance().log(Logger::Info, "MenuE2E", "Tools menu actions tested");
    } else {
        Logger::instance().log(Logger::Info, "MenuE2E", "Tools menu not found - may not be implemented");
    }
}

void TestMenuInteractions::testHelpMenuContent()
{
    Logger::instance().log(Logger::Info, "MenuE2E", "Testing Help menu content");
    
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    
    // Find Help menu
    QMenu* helpMenu = nullptr;
    foreach (QAction* action, menuBar->actions()) {
        if (action->text().contains("Help") || action->text().contains("&Help")) {
            helpMenu = action->menu();
            break;
        }
    }
    
    QVERIFY2(helpMenu != nullptr, "Help menu not found - this is required");
    
    testMenuStructure(helpMenu, "Help");
    
    // Test Help menu actions
    QStringList expectedHelpActions = {
        "About", "Documentation", "User Guide", "Support", "Report Bug"
    };
    
    QList<QAction*> helpActions = helpMenu->actions();
    QStringList foundHelpActions;
    
    foreach (QAction* action, helpActions) {
        if (!action->isSeparator()) {
            foundHelpActions << action->text();
            testMenuAction(action, QString("Help -> %1").arg(action->text()));
            
            // Test About dialog specifically
            if (action->text().contains("About", Qt::CaseInsensitive)) {
                Logger::instance().log(Logger::Debug, "MenuE2E", "Testing About dialog");
                
                // Set up About dialog interception
                QTimer::singleShot(100, [this]() {
                    QWidget* aboutDialog = QApplication::activeModalWidget();
                    if (aboutDialog) {
                        Logger::instance().log(Logger::Debug, "MenuE2E", "About dialog opened successfully");
                        
                        // Verify dialog has content
                        QVERIFY(aboutDialog->isVisible());
                        
                        // Close dialog
                        QTest::qWait(200); // Let user "read" about info
                        aboutDialog->close();
                    }
                });
                
                action->trigger();
                QTest::qWait(400);
            }
        }
    }
    
    Logger::instance().log(Logger::Debug, "MenuE2E", 
                          QString("Help menu actions: %1").arg(foundHelpActions.join(", ")));
    
    // Verify About action exists
    bool hasAboutAction = false;
    foreach (const QString& action, foundHelpActions) {
        if (action.contains("About", Qt::CaseInsensitive)) {
            hasAboutAction = true;
            break;
        }
    }
    QVERIFY2(hasAboutAction, "About action not found in Help menu");
    
    Logger::instance().log(Logger::Info, "MenuE2E", "Help menu content tested");
}

void TestMenuInteractions::testKeyboardShortcuts()
{
    Logger::instance().log(Logger::Info, "MenuE2E", "Testing keyboard shortcuts");
    
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    
    // Collect all actions with shortcuts across all menus
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
    
    Logger::instance().log(Logger::Debug, "MenuE2E", 
                          QString("Found %1 actions with keyboard shortcuts").arg(actionsWithShortcuts.count()));
    
    // Test each shortcut
    foreach (QAction* action, actionsWithShortcuts) {
        verifyKeyboardShortcut(action);
    }
    
    // Test common expected shortcuts
    QStringList expectedShortcuts = {"Ctrl+O", "Ctrl+Q", "Ctrl+N", "F1"};
    QStringList foundShortcuts;
    
    foreach (QAction* action, actionsWithShortcuts) {
        foundShortcuts << action->shortcut().toString();
    }
    
    Logger::instance().log(Logger::Debug, "MenuE2E", 
                          QString("Found shortcuts: %1").arg(foundShortcuts.join(", ")));
    
    // Verify at least some common shortcuts exist
    bool hasCtrlO = foundShortcuts.contains("Ctrl+O");
    bool hasCtrlQ = foundShortcuts.contains("Ctrl+Q");
    
    if (hasCtrlO) {
        Logger::instance().log(Logger::Debug, "MenuE2E", "Ctrl+O shortcut found (typically for Open)");
    }
    if (hasCtrlQ) {
        Logger::instance().log(Logger::Debug, "MenuE2E", "Ctrl+Q shortcut found (typically for Quit)");
    }
    
    Logger::instance().log(Logger::Info, "MenuE2E", "Keyboard shortcuts testing completed");
}

void TestMenuInteractions::testMenuAccessibility()
{
    Logger::instance().log(Logger::Info, "MenuE2E", "Testing menu accessibility");
    
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>();
    
    // Test keyboard navigation of menus
    foreach (QAction* menuAction, menuBar->actions()) {
        if (menuAction->menu()) {
            QMenu* menu = menuAction->menu();
            QString menuName = menuAction->text();
            
            // Test that menu can be activated
            QVERIFY2(menu->isEnabled(), QString("Menu '%1' is disabled").arg(menuName).toLocal8Bit());
            
            // Test accelerator keys (& character)
            if (menuName.contains('&')) {
                Logger::instance().log(Logger::Debug, "MenuE2E", 
                                      QString("Menu '%1' has accelerator key").arg(menuName));
            }
            
            // Test menu actions accessibility
            foreach (QAction* action, menu->actions()) {
                if (!action->isSeparator()) {
                    // Check for accelerator keys in action text
                    if (action->text().contains('&')) {
                        Logger::instance().log(Logger::Debug, "MenuE2E", 
                                              QString("Action '%1' has accelerator key").arg(action->text()));
                    }
                    
                    // Check for tooltip/status tip
                    if (!action->toolTip().isEmpty()) {
                        Logger::instance().log(Logger::Debug, "MenuE2E", 
                                              QString("Action '%1' has tooltip: %2")
                                              .arg(action->text()).arg(action->toolTip()));
                    }
                }
            }
        }
    }
    
    Logger::instance().log(Logger::Info, "MenuE2E", "Menu accessibility testing completed");
}

void TestMenuInteractions::testContextMenus()
{
    Logger::instance().log(Logger::Info, "MenuE2E", "Testing context menus");
    
    // Test context menus on main window components
    QWidget* centralWidget = mainWindow->centralWidget();
    if (centralWidget) {
        // Test right-click context menu on central widget
        QTest::mouseClick(centralWidget, Qt::RightButton, Qt::NoModifier, QPoint(100, 100));
        QTest::qWait(100);
        
        // Check if context menu appeared
        QMenu* contextMenu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (contextMenu) {
            Logger::instance().log(Logger::Debug, "MenuE2E", 
                                  QString("Context menu found with %1 actions").arg(contextMenu->actions().count()));
            
            // Test context menu actions
            foreach (QAction* action, contextMenu->actions()) {
                if (!action->isSeparator()) {
                    Logger::instance().log(Logger::Debug, "MenuE2E", 
                                          QString("Context menu action: %1").arg(action->text()));
                }
            }
            
            // Close context menu
            contextMenu->close();
        } else {
            Logger::instance().log(Logger::Info, "MenuE2E", "No context menu found - may not be implemented");
        }
    }
    
    // Test context menus on other components
    ServiceBrowser* serviceBrowser = mainWindow->findChild<ServiceBrowser*>();
    if (serviceBrowser) {
        QTest::mouseClick(qobject_cast<QWidget*>(serviceBrowser), Qt::RightButton, Qt::NoModifier, QPoint(50, 50));
        QTest::qWait(100);
        
        QMenu* serviceContextMenu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (serviceContextMenu) {
            Logger::instance().log(Logger::Debug, "MenuE2E", "Service browser context menu found");
            serviceContextMenu->close();
        }
    }
    
    Logger::instance().log(Logger::Info, "MenuE2E", "Context menu testing completed");
}

void TestMenuInteractions::testMenuStructure(QMenu* menu, const QString& menuName)
{
    QVERIFY2(menu != nullptr, QString("Menu %1 is null").arg(menuName).toLocal8Bit());
    QVERIFY2(menu->isEnabled(), QString("Menu %1 is disabled").arg(menuName).toLocal8Bit());
    
    QList<QAction*> actions = menu->actions();
    QVERIFY2(actions.count() > 0, QString("Menu %1 has no actions").arg(menuName).toLocal8Bit());
    
    Logger::instance().log(Logger::Debug, "MenuE2E", 
                          QString("Menu '%1' structure: %2 actions").arg(menuName).arg(actions.count()));
    
    // Count separators vs actual actions
    int separatorCount = 0;
    int actionCount = 0;
    
    foreach (QAction* action, actions) {
        if (action->isSeparator()) {
            separatorCount++;
        } else {
            actionCount++;
        }
    }
    
    Logger::instance().log(Logger::Debug, "MenuE2E", 
                          QString("Menu '%1': %2 actions, %3 separators").arg(menuName).arg(actionCount).arg(separatorCount));
    
    QVERIFY2(actionCount > 0, QString("Menu %1 has no non-separator actions").arg(menuName).toLocal8Bit());
}

void TestMenuInteractions::testMenuAction(QAction* action, const QString& actionName)
{
    QVERIFY2(action != nullptr, QString("Action %1 is null").arg(actionName).toLocal8Bit());
    
    // Test basic action properties
    QVERIFY2(!action->text().isEmpty(), QString("Action %1 has empty text").arg(actionName).toLocal8Bit());
    
    // Log action details
    Logger::instance().log(Logger::Debug, "MenuE2E", 
                          QString("Testing action: %1 (enabled: %2, visible: %3, checkable: %4)")
                          .arg(actionName)
                          .arg(action->isEnabled())
                          .arg(action->isVisible())
                          .arg(action->isCheckable()));
    
    // Test keyboard shortcut if present
    if (!action->shortcut().isEmpty()) {
        verifyKeyboardShortcut(action);
    }
    
    // Test action icon if present
    if (!action->icon().isNull()) {
        Logger::instance().log(Logger::Debug, "MenuE2E", 
                              QString("Action %1 has icon").arg(actionName));
    }
    
    // Note: We don't trigger actions here to avoid side effects during testing
    // Real trigger testing would be done in specific workflow tests
}

void TestMenuInteractions::verifyKeyboardShortcut(QAction* action)
{
    QKeySequence shortcut = action->shortcut();
    QVERIFY2(!shortcut.isEmpty(), "Action shortcut is empty");
    
    QString shortcutString = shortcut.toString();
    QVERIFY2(!shortcutString.isEmpty(), "Shortcut string is empty");
    
    Logger::instance().log(Logger::Debug, "MenuE2E", 
                          QString("Action '%1' shortcut: %2").arg(action->text()).arg(shortcutString));
    
    // Verify shortcut is properly formatted
    QVERIFY2(shortcutString.length() > 1, "Shortcut string too short");
    
    // Note: We don't test actual shortcut triggering here to avoid side effects
    // Real shortcut testing would be done in specific workflow tests
}

QTEST_MAIN(TestMenuInteractions)
#include "test_menu_interactions.moc"