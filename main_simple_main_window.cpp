#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include "utils/logger.h"

// Simplified MainWindow without complex dependencies
class SimpleMainWindow : public QMainWindow {
    Q_OBJECT

public:
    SimpleMainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        Logger::instance().log(Logger::Info, "MainWindow", "Simple MainWindow created");
    }

private:
    void setupUI() {
        // Create central widget
        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        // Create layout
        auto* layout = new QVBoxLayout(centralWidget);
        auto* label = new QLabel("StreamDAB Analyzer - Simple MainWindow");
        layout->addWidget(label);
        
        // Create menu bar
        auto* fileMenu = menuBar()->addMenu("File");
        fileMenu->addAction("Open", [this]() {
            Logger::instance().log(Logger::Info, "Menu", "File->Open clicked");
        });
        
        // Create status bar
        statusBar()->showMessage("Ready");
        
        setWindowTitle("StreamDAB Stream Analyzer");
        resize(800, 600);
    }
};

#include "main_simple_main_window.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    Logger::instance().log(Logger::Info, "Application", "Starting StreamDAB with simple MainWindow...");
    
    SimpleMainWindow window;
    window.show();
    
    return app.exec();
}