#include <QtWidgets/QApplication>
#include <QtWidgets/QSplashScreen>
#include <QtGui/QPixmap>
#include <QtCore/QTimer>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>

#include "gui/dab_analyzer_main_window.h"

// Enable Qt logging for development
Q_LOGGING_CATEGORY(dabAnalyzer, "dab.analyzer")

/**
 * @brief StreamDAB Stream Analyzer - DAB Analyzer Professional Interface
 *
 * Phase 1.3 Implementation Features:
 * - DAB Analyzer compliant 4-panel layout
 * - Ensemble Tree Explorer with checkbox selection
 * - Frame Navigator with selectable frame list (24ms precision)
 * - Service Details panel with signal quality metrics
 * - FIC Analysis Hub with FIC-Analyzer/FIC-Extractor tabs
 * - Enhanced ETI processing with Bangkok ETI-LI format support
 * - Professional DAB broadcast industry styling
 */

void setupApplication()
{
    QApplication::setApplicationName("StreamDAB Stream Analyzer");
    QApplication::setApplicationVersion("1.4.0");
    QApplication::setOrganizationName("StreamDAB Project");
    QApplication::setOrganizationDomain("streamdab.org");

    // Set application properties (Qt6 compatible)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
}

QSplashScreen* createSplashScreen()
{
    // Create a simple splash screen for professional appearance
    QPixmap splashPixmap(400, 200);
    splashPixmap.fill(QColor(0x00, 0x66, 0xCC));  // DAB Blue background

    QSplashScreen* splash = new QSplashScreen(splashPixmap);
    splash->show();

    // Show loading messages
    splash->showMessage("StreamDAB Stream Analyzer v1.4",
                       Qt::AlignCenter | Qt::AlignBottom, Qt::white);
    QApplication::processEvents();

    QTimer::singleShot(500, [splash]() {
        splash->showMessage("Initializing DAB Analyzer Interface...",
                           Qt::AlignCenter | Qt::AlignBottom, Qt::white);
        QApplication::processEvents();
    });

    QTimer::singleShot(1000, [splash]() {
        splash->showMessage("Loading Advanced FIG Analyzer...",
                           Qt::AlignCenter | Qt::AlignBottom, Qt::white);
        QApplication::processEvents();
    });

    QTimer::singleShot(1500, [splash]() {
        splash->showMessage("Ready for Advanced DAB Analysis",
                           Qt::AlignCenter | Qt::AlignBottom, Qt::white);
        QApplication::processEvents();
    });

    return splash;
}

void setupLogging()
{
    // Configure Qt logging for development and debugging
    QLoggingCategory::setFilterRules("dab.analyzer.debug=true");

    // Log application startup
    qCDebug(dabAnalyzer) << "StreamDAB Stream Analyzer Phase 1.4 starting...";
    qCDebug(dabAnalyzer) << "Qt version:" << qVersion();
    qCDebug(dabAnalyzer) << "DAB Analyzer interface enabled";
    qCDebug(dabAnalyzer) << "Advanced FIG analyzer with Bangkok support ready";
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Setup application properties
    setupApplication();
    setupLogging();

    // Create and show splash screen
    QSplashScreen* splash = createSplashScreen();

    // Create main window
    DABAnalyzerMainWindow window;

    // Setup window with professional appearance
    window.setWindowIcon(QIcon());  // Could add icon later

    // Close splash screen after 2 seconds and show main window
    QTimer::singleShot(2000, [&]() {
        splash->finish(&window);
        splash->deleteLater();

        window.show();

        // Log successful startup
        qCDebug(dabAnalyzer) << "DAB Analyzer main window displayed successfully";
        qCDebug(dabAnalyzer) << "Phase 1.4 initialization complete";
    });

    // Handle command line ETI file argument
    QStringList args = app.arguments();
    if (args.size() > 1) {
        QString etiFile = args[1];
        if (QFile::exists(etiFile)) {
            QTimer::singleShot(2500, [&window, etiFile]() {
                qCDebug(dabAnalyzer) << "Loading ETI file from command line:" << etiFile;
                window.openETIFile(etiFile);
            });
        }
    }

    qCDebug(dabAnalyzer) << "Entering Qt event loop...";

    return app.exec();
}
