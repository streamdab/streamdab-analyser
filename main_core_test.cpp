#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "utils/logger.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Test logger
    Logger::instance().log(Logger::Info, "Application", "StreamDAB Analyzer starting...");
    
    QWidget window;
    QVBoxLayout* layout = new QVBoxLayout(&window);
    QLabel* label = new QLabel("StreamDAB Analyzer - Core Test");
    layout->addWidget(label);
    
    window.setWindowTitle("StreamDAB Core Test");
    window.show();
    
    Logger::instance().log(Logger::Info, "Application", "Qt GUI initialized");
    
    return app.exec();
}