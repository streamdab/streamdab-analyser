#include <QApplication>
#include "gui/main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    Phase14DABAnalyzerWindow window;
    window.show();
    
    return app.exec();
}