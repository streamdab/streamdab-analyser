/**
 * @file main.cpp
 * @brief Main entry point for ETI Stream Analyzer
 *
 * This file contains the main application entry point with Qt6 initialization,
 * platform-specific setup, and application lifecycle management.
 *
 * @author ETI Stream Analyzer Team
 * @date 2024
 * @copyright Copyright (c) 2024 ETI Stream Analyzer Team
 */

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QMessageBox>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <QCommandLineOption>

#ifdef Q_OS_WIN
#include <QSettings>
#include <windows.h>
#endif

#ifdef Q_OS_MACOS
#include <QMenuBar>
#endif

// Application headers (will be created in next phase)
// #include "gui/main_window.h"
// #include "utils/config_manager.h"
// #include "utils/logger.h"

// Temporary placeholder widget for initial testing
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QStatusBar>

// Application constants
static const QString APP_NAME = "ETI Stream Analyzer";
static const QString APP_VERSION = "1.0.0";
static const QString APP_ORGANIZATION = "ETI Analyzer Team";
static const QString APP_DOMAIN = "etianalyzer.com";

// Logging category
Q_LOGGING_CATEGORY(main, "eti.main")

/**
 * @brief Temporary main window for testing the build system
 *
 * This is a placeholder implementation that will be replaced with
 * the actual MainWindow class in the next development phase.
 */
class TemporaryMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TemporaryMainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        setupUI();
        setWindowTitle(QString("%1 v%2").arg(APP_NAME, APP_VERSION));
        resize(800, 600);
    }

private slots:
    void showAbout()
    {
        QMessageBox::about(this, "About " + APP_NAME,
            QString("<h2>%1</h2>"
                   "<p>Version: %2</p>"
                   "<p>Professional DAB/DAB+ ETI Stream Analysis Tool</p>"
                   "<p>Built with Qt %3</p>"
                   "<p>Build Configuration Test: <b>SUCCESS</b></p>")
            .arg(APP_NAME, APP_VERSION, QT_VERSION_STR));
    }

private:
    void setupUI()
    {
        // Central widget
        auto *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // Layout
        auto *layout = new QVBoxLayout(centralWidget);

        // Welcome label
        auto *welcomeLabel = new QLabel(QString(
            "<h1>Welcome to %1</h1>"
            "<p>Professional ETI Stream Analyzer</p>"
            "<p><b>Build System Test - Phase 1 Complete</b></p>"
        ).arg(APP_NAME));
        welcomeLabel->setAlignment(Qt::AlignCenter);
        welcomeLabel->setStyleSheet("QLabel { padding: 20px; }");

        // Feature status
        auto *statusLabel = new QLabel(
            "<h3>Build Configuration Status:</h3>"
            "<ul>"
            "<li>✓ Qt6 Integration: Ready</li>"
            "<li>✓ Cross-platform CMake: Configured</li>"
            "<li>✓ Dependency Management: Set up</li>"
            "<li>✓ Build System: Operational</li>"
            "</ul>"
            "<p><i>Ready for ETISnoop integration (Phase 2)</i></p>"
        );
        statusLabel->setStyleSheet("QLabel { padding: 10px; }");

        // Test button
        auto *aboutButton = new QPushButton("About / Build Info");
        connect(aboutButton, &QPushButton::clicked, this, &TemporaryMainWindow::showAbout);

        // Add to layout
        layout->addWidget(welcomeLabel);
        layout->addWidget(statusLabel);
        layout->addWidget(aboutButton);
        layout->addStretch();

        // Status bar
        statusBar()->showMessage("ETI Stream Analyzer - Build System Ready");
    }
};

/**
 * @brief Initialize application directories and settings
 */
void initializeApplicationDirectories()
{
    // Create application data directories
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    QDir().mkpath(appDataPath + "/logs");
    QDir().mkpath(appDataPath + "/config");
    QDir().mkpath(appDataPath + "/temp");

    qCDebug(main) << "Application data directory:" << appDataPath;
}

/**
 * @brief Set up application styling and theme
 */
void setupApplicationStyle(QApplication &app)
{
    // Set application style
    QStringList availableStyles = QStyleFactory::keys();
    qCDebug(main) << "Available styles:" << availableStyles;

#ifdef Q_OS_WIN
    if (availableStyles.contains("WindowsVista")) {
        app.setStyle("WindowsVista");
    }
#elif defined(Q_OS_MACOS)
    if (availableStyles.contains("macOS")) {
        app.setStyle("macOS");
    }
#else
    if (availableStyles.contains("Fusion")) {
        app.setStyle("Fusion");
    }
#endif

    // Set application icon (placeholder for now)
    // app.setWindowIcon(QIcon(":/icons/app_icon.png"));
}

/**
 * @brief Parse command line arguments
 */
void parseCommandLineArguments(QApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("ETI Stream Analyzer - Professional DAB/DAB+ Analysis Tool");
    parser.addHelpOption();
    parser.addVersionOption();

    // Debug option
    QCommandLineOption debugOption(QStringList() << "d" << "debug",
        "Enable debug output");
    parser.addOption(debugOption);

    // Config file option
    QCommandLineOption configOption(QStringList() << "c" << "config",
        "Configuration file path", "file");
    parser.addOption(configOption);

    parser.process(app);

    if (parser.isSet(debugOption)) {
        QLoggingCategory::setFilterRules("*.debug=true");
        qCDebug(main) << "Debug mode enabled";
    }

    if (parser.isSet(configOption)) {
        QString configFile = parser.value(configOption);
        qCDebug(main) << "Using config file:" << configFile;
        // TODO: Pass to ConfigManager in next phase
    }
}

/**
 * @brief Platform-specific initialization
 */
void platformSpecificInit()
{
#ifdef Q_OS_WIN
    // Windows-specific initialization
    // Enable high DPI support
    SetProcessDPIAware();
#endif

#ifdef Q_OS_MACOS
    // macOS-specific initialization
    // Configure dock and menu bar behavior
#endif

#ifdef Q_OS_LINUX
    // Linux-specific initialization
    // Set up any required environment variables
#endif
}

/**
 * @brief Main application entry point
 *
 * Initializes Qt application, sets up platform-specific configurations,
 * and launches the main application window.
 *
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @return Application exit code
 */
int main(int argc, char *argv[])
{
    // Enable high DPI support (Qt 6)
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Create Qt application
    QApplication app(argc, argv);

    // Set application metadata
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName(APP_ORGANIZATION);
    app.setOrganizationDomain(APP_DOMAIN);

    // Platform-specific initialization
    platformSpecificInit();

    // Parse command line arguments
    parseCommandLineArguments(app);

    // Initialize application directories
    initializeApplicationDirectories();

    // Set up application styling
    setupApplicationStyle(app);

    // Log startup information
    qCInfo(main) << "Starting" << APP_NAME << "version" << APP_VERSION;
    qCInfo(main) << "Qt version:" << QT_VERSION_STR;
    qCInfo(main) << "Platform:" << QGuiApplication::platformName();

    try {
        // Initialize core components (placeholder for next phase)
        // ConfigManager::initialize();
        // Logger::initialize();

        // Create and show main window
        TemporaryMainWindow mainWindow;
        mainWindow.show();

        qCInfo(main) << "Application initialized successfully";

        // Run application event loop
        int result = app.exec();

        qCInfo(main) << "Application shutting down with code:" << result;
        return result;

    } catch (const std::exception &e) {
        qCCritical(main) << "Fatal error during application startup:" << e.what();
        QMessageBox::critical(nullptr, "Fatal Error",
            QString("Application failed to start:\n%1").arg(e.what()));
        return -1;
    }
}

// Include MOC file for temporary main window
#include "main.moc"
