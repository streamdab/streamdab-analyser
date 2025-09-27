/**
 * @file simple_gui_test.cpp
 * @brief Simple GUI test to verify segmentation fault fixes
 */

#include <QApplication>
#include <QMainWindow>
#include <QDebug>
#include <memory>

// Include the fixed MainWindow
#include "src/gui/main_window.h"
#include "src/utils/logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set up basic logging
    Logger::instance().setLogLevel(Logger::Info);
    
    qDebug() << "Creating MainWindow instance...";
    
    try {
        // Test MainWindow creation and initialization
        std::unique_ptr<MainWindow> mainWindow = std::make_unique<MainWindow>();
        
        qDebug() << "MainWindow created successfully";
        
        // Test initialization (this was causing segmentation fault)
        bool initResult = mainWindow->initialize();
        
        if (initResult) {
            qDebug() << "MainWindow initialization successful!";
            qDebug() << "Segmentation fault fix confirmed - GUI components initialized properly";
            
            // Show the window briefly to confirm it works
            mainWindow->show();
            
            // Process events for 2 seconds then close
            QTimer::singleShot(2000, [&]() {
                qDebug() << "Test completed - closing application";
                app.quit();
            });
            
            return app.exec();
            
        } else {
            qCritical() << "MainWindow initialization failed!";
            return -1;
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Exception during MainWindow test:" << e.what();
        return -2;
    }
}