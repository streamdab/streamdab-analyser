#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "utils/logger.h"

// Test simple ETI functionality without full processor class
void testETIBasics() {
    Logger::instance().log(Logger::Info, "ETI", "Testing basic ETI concepts...");
    
    // Simple ETI frame structure test
    const size_t ETI_FRAME_SIZE = 6144;
    std::vector<uint8_t> test_frame(ETI_FRAME_SIZE, 0);
    
    // ETI sync pattern: 0x49, 0x93, 0x1E, 0x03
    test_frame[0] = 0x49;
    test_frame[1] = 0x93;
    test_frame[2] = 0x1E;
    test_frame[3] = 0x03;
    
    Logger::instance().log(Logger::Info, "ETI", 
        QString("Created test ETI frame of %1 bytes").arg(test_frame.size()));
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    Logger::instance().log(Logger::Info, "Application", "StreamDAB ETI Test starting...");
    
    testETIBasics();
    
    QWidget window;
    QVBoxLayout* layout = new QVBoxLayout(&window);
    QLabel* label = new QLabel("StreamDAB Analyzer - ETI Test");
    layout->addWidget(label);
    
    window.setWindowTitle("StreamDAB ETI Test");
    window.show();
    
    Logger::instance().log(Logger::Info, "Application", "ETI Test initialized");
    
    return app.exec();
}