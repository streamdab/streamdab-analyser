#include <QApplication>
#include "utils/logger.h"
#include "gui/main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    Logger::instance().log(Logger::Info, "Application", "Testing real MainWindow...");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}