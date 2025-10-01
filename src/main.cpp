#include <QApplication>
#include "gui/dab_analyzer_main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    DABAnalyzerMainWindow window;
    window.show();
    
    return app.exec();
}