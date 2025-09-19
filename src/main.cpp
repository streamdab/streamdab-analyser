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

// Application headers
#include "gui/main_window.h"
#include "utils/config_manager.h"
#include "utils/logger.h"

// Application constants
const QString APP_NAME = "ETI Stream Analyzer";
const QString APP_VERSION = "1.0.0";
const QString APP_ORGANIZATION = "ETI Analyzer Team";
const QString APP_DOMAIN = "eti-analyzer.com";

// Logging category
Q_LOGGING_CATEGORY(etiMain, "eti.main")

/**
 * @brief Parse command line arguments
 */
void parseCommandLineArguments(const QApplication& app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("ETI Stream Analyzer - Professional DAB/DAB+ Analysis Tool");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption debugOption(QStringList() << "d" << "debug",
                                   "Enable debug output");
    parser.addOption(debugOption);

    QCommandLineOption configOption(QStringList() << "c" << "config",
                                    "Configuration file path", "file");
    parser.addOption(configOption);

    parser.process(app);

    if (parser.isSet(debugOption)) {
        QLoggingCategory::setFilterRules("*.debug=true");
        qCDebug(etiMain) << "Debug mode enabled";
    }

    if (parser.isSet(configOption)) {
        QString configFile = parser.value(configOption);
        qCDebug(etiMain) << "Using config file:" << configFile;
    }
}

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

    qCDebug(etiMain) << "Application data directory:" << appDataPath;
}

/**
 * @brief Set up application styling and theme
 */
void setupApplicationStyle(QApplication& app)
{
    // Set application style
    QStringList availableStyles = QStyleFactory::keys();
    qCDebug(etiMain) << "Available styles:" << availableStyles;

    // Use Fusion style for consistent cross-platform appearance
    if (availableStyles.contains("Fusion")) {
        app.setStyle("Fusion");
    }

    // Set professional color scheme
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    app.setPalette(darkPalette);
}

/**
 * @brief Platform-specific initialization
 */
void platformSpecificInit()
{
#ifdef Q_OS_WIN
    // Windows-specific initialization
    SetProcessDPIAware();
#endif

#ifdef Q_OS_MACOS
    // macOS-specific initialization
    // Configure dock and menu bar behavior
#endif
}

/**
 * @brief Main application entry point
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
    qCInfo(etiMain) << "Starting" << APP_NAME << "version" << APP_VERSION;
    qCInfo(etiMain) << "Qt version:" << QT_VERSION_STR;
    qCInfo(etiMain) << "Platform:" << QGuiApplication::platformName();
    
    try {
        // Initialize Logger system with hang protection
        qCInfo(etiMain) << "Initializing Logger system with hang protection...";
        Logger::instance().enableLogging(true);
        Logger::instance().initialize();
        
        // Test with actual MainWindow and initialization
        qCInfo(etiMain) << "Creating MainWindow...";
        MainWindow mainWindow;
        qCInfo(etiMain) << "MainWindow created, calling initialize()...";
        
        if (!mainWindow.initialize()) {
            qCCritical(etiMain) << "Failed to initialize main window";
            QMessageBox::critical(nullptr, "Initialization Error",
                "Failed to initialize the main window. Please check the logs for details.");
            return -1;
        }
        
        qCInfo(etiMain) << "MainWindow initialized successfully, showing...";
        mainWindow.show();
        qCInfo(etiMain) << "MainWindow shown with full professional interface";
        
        // Run application event loop
        int result = app.exec();
        qCInfo(etiMain) << "Application shutting down with code:" << result;
        return result;
        
    } catch (const std::exception &e) {
        qCCritical(etiMain) << "Fatal error during application startup:" << e.what();
        QMessageBox::critical(nullptr, "Fatal Error",
            QString("Application failed to start:\n%1").arg(e.what()));
        return -1;
    }
}

#include "main.moc"