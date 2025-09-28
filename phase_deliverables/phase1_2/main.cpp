#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QScreen>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Application properties
    app.setApplicationName("StreamDAB Analyzer Phase 1.2");
    app.setApplicationVersion("1.2.0");
    app.setOrganizationName("StreamDAB Project");
    app.setOrganizationDomain("streamdab.org");
    
    // Set application icon (if available)
    // app.setWindowIcon(QIcon(":/icons/streamdab.png"));
    
    // Check Qt version
    qDebug() << "StreamDAB Analyzer Phase 1.2 starting...";
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "Application Version:" << app.applicationVersion();
    
    // Set fusion style for professional appearance
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Create splash screen
    QPixmap splash(400, 300);
    splash.fill(QColor(45, 45, 48)); // Professional dark background
    
    QSplashScreen splashScreen(splash);
    splashScreen.setStyleSheet(
        "QSplashScreen {"
        "    color: white;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
    );
    
    splashScreen.show();
    splashScreen.showMessage("StreamDAB Analyzer Phase 1.2\n\n"
                           "Qt GUI Integration with ETI-LI Support\n"
                           "Loading...", 
                           Qt::AlignCenter | Qt::AlignBottom, Qt::white);
    
    app.processEvents();
    
    // Initialize application (simulate loading time)
    QTimer::singleShot(1000, [&]() {
        splashScreen.showMessage("StreamDAB Analyzer Phase 1.2\n\n"
                               "Qt GUI Integration with ETI-LI Support\n"
                               "Ready!", 
                               Qt::AlignCenter | Qt::AlignBottom, Qt::white);
    });
    
    QTimer::singleShot(1500, [&]() {
        splashScreen.finish(nullptr);
    });
    
    // Create and show main window
    MainWindow window;
    
    // Position window in center of screen
    window.show();
    window.move(QApplication::primaryScreen()->geometry().center() - window.rect().center());
    
    // Show welcome message
    QTimer::singleShot(2000, [&window]() {
        QMessageBox::information(&window, 
            "Welcome to StreamDAB Analyzer Phase 1.2",
            "Phase 1.2: Qt GUI Integration Complete!\n\n"
            "Features:\n"
            "✅ Professional Qt6 GUI interface\n"
            "✅ Bangkok ETI-LI file support\n"
            "✅ Real-time processing display\n"
            "✅ Multi-format ETI detection\n"
            "✅ ETSI EN 300 799 compliance\n\n"
            "Ready to process Bangkok DAB files!\n"
            "Click 'Open ETI File' to begin analysis.");
    });
    
    qDebug() << "StreamDAB Analyzer Phase 1.2 initialized successfully";
    qDebug() << "Ready for Bangkok ETI-LI file processing";
    
    return app.exec();
}