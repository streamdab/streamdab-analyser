#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QTimer>
#include <QDebug>
#include <iostream>

// Simple include for MainWindow
#include "src/gui/main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    std::cout << "Creating MainWindow..." << std::endl;
    MainWindow window;
    
    std::cout << "Showing window..." << std::endl;
    window.show();
    
    // Test MenuBar accessibility
    QMenuBar* menuBar = window.findChild<QMenuBar*>();
    if (menuBar) {
        std::cout << "✅ SUCCESS: MenuBar found via findChild<QMenuBar*>()" << std::endl;
        std::cout << "   Object name: " << menuBar->objectName().toStdString() << std::endl;
        
        // Test File menu accessibility
        QMenu* fileMenu = window.findChild<QMenu*>("FileMenu");
        if (fileMenu) {
            std::cout << "✅ SUCCESS: File menu found with object name 'FileMenu'" << std::endl;
            std::cout << "   Menu text: " << fileMenu->title().toStdString() << std::endl;
            
            // Test actions
            QList<QAction*> actions = fileMenu->actions();
            std::cout << "   File menu actions (" << actions.size() << "):" << std::endl;
            for (QAction* action : actions) {
                if (!action->isSeparator()) {
                    std::cout << "     - " << action->text().toStdString() << std::endl;
                }
            }
            
            // Test Open ETI File action
            QAction* openAction = window.findChild<QAction*>("OpenETIFileAction");
            if (openAction) {
                std::cout << "✅ SUCCESS: Open ETI File action found" << std::endl;
                std::cout << "   Action text: " << openAction->text().toStdString() << std::endl;
                std::cout << "   Shortcut: " << openAction->shortcut().toString().toStdString() << std::endl;
            } else {
                std::cout << "❌ FAILED: Open ETI File action not found" << std::endl;
            }
        } else {
            std::cout << "❌ FAILED: File menu not found with object name 'FileMenu'" << std::endl;
        }
        
        // Test all menus
        QList<QMenu*> menus = menuBar->findChildren<QMenu*>();
        std::cout << "   Total menus found: " << menus.size() << std::endl;
        for (QMenu* menu : menus) {
            std::cout << "     - " << menu->title().toStdString() 
                      << " (object name: " << menu->objectName().toStdString() << ")" << std::endl;
        }
        
    } else {
        std::cout << "❌ FAILED: MenuBar not found via findChild<QMenuBar*>()" << std::endl;
        
        // Try alternative access
        QMenuBar* directMenuBar = window.menuBar();
        if (directMenuBar) {
            std::cout << "✅ INFO: MenuBar accessible via direct menuBar() call" << std::endl;
        } else {
            std::cout << "❌ CRITICAL: MenuBar not accessible at all" << std::endl;
        }
    }
    
    // Exit after test
    QTimer::singleShot(100, &app, &QApplication::quit);
    
    return app.exec();
}