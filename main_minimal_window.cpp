#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include "utils/logger.h"

// Minimal MainWindow without complex dependencies
class MinimalMainWindow : public QMainWindow {
    Q_OBJECT

public:
    MinimalMainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        auto* layout = new QVBoxLayout(centralWidget);
        auto* label = new QLabel("StreamDAB Analyzer - Minimal Window");
        layout->addWidget(label);
        
        setWindowTitle("StreamDAB Minimal Test");
        Logger::instance().log(Logger::Info, "MainWindow", "Minimal window created");
    }
};

#include "main_minimal_window.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    Logger::instance().log(Logger::Info, "Application", "Testing minimal MainWindow...");
    
    MinimalMainWindow window;
    window.show();
    
    return app.exec();
}