#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QWidget window;
    QVBoxLayout* layout = new QVBoxLayout(&window);
    QLabel* label = new QLabel("StreamDAB Analyzer - Qt Test");
    layout->addWidget(label);
    
    window.setWindowTitle("StreamDAB Test");
    window.show();
    
    return app.exec();
}