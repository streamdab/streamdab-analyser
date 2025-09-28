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
#include "core/headless_eti_processor.hpp"
#include "utils/config_manager.h"
#include "utils/logger.h"

// Application constants
const QString APP_NAME = "StreamDAB Stream Analyzer";
const QString APP_VERSION = "1.0.0";
const QString APP_ORGANIZATION = "ETI Analyzer Team";
const QString APP_DOMAIN = "eti-analyzer.com";

// Logging category
Q_LOGGING_CATEGORY(etiMain, "eti.main")

/**
 * @brief Parse command line arguments
 */
bool parseCommandLineArguments(const QApplication& app, QString& inputFile, QString& outputFile, bool& headlessMode)
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

    // Headless mode options
    QCommandLineOption headlessOption(QStringList() << "headless" << "batch",
                                     "Run in headless mode without GUI");
    parser.addOption(headlessOption);

    QCommandLineOption inputOption(QStringList() << "i" << "input",
                                  "Input ETI file to process", "file");
    parser.addOption(inputOption);

    QCommandLineOption outputOption(QStringList() << "o" << "output",
                                   "Output analysis file (YAML/JSON)", "file");
    parser.addOption(outputOption);

    parser.process(app);

    if (parser.isSet(debugOption)) {
        QLoggingCategory::setFilterRules("*.debug=true");
        qCDebug(etiMain) << "Debug mode enabled";
    }

    if (parser.isSet(configOption)) {
        QString configFile = parser.value(configOption);
        qCDebug(etiMain) << "Using config file:" << configFile;
    }

    // Check for headless mode
    headlessMode = parser.isSet(headlessOption);
    if (headlessMode) {
        qCInfo(etiMain) << "Headless mode enabled";
        
        if (!parser.isSet(inputOption)) {
            qCCritical(etiMain) << "Headless mode requires input file (-i/--input)";
            parser.showHelp(1);
            return false;
        }
        
        inputFile = parser.value(inputOption);
        outputFile = parser.value(outputOption);
        
        if (outputFile.isEmpty()) {
            // Default output file based on input file
            QFileInfo inputInfo(inputFile);
            outputFile = inputInfo.path() + "/" + inputInfo.baseName() + "_analysis.yaml";
        }
        
        qCInfo(etiMain) << "Input file:" << inputFile;
        qCInfo(etiMain) << "Output file:" << outputFile;
    }

    return true;
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
    // Aggressive Qt platform optimization for file dialog performance
    QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
    if (desktop.contains("KDE", Qt::CaseInsensitive)) {
        // Disable all platform integrations that can cause slow file dialogs
        qputenv("QT_QPA_PLATFORMTHEME", "");        // No platform theme
        qputenv("QT_STYLE_OVERRIDE", "fusion");     // Fast Fusion style
        qputenv("QT_XDG_DESKTOP_PORTAL", "0");      // Disable xdg-desktop-portal
        qputenv("QT_QPA_NO_NATIVE_DIALOGS", "1");   // Force non-native dialogs
        qDebug() << "KDE detected: Applied aggressive platform optimizations for file dialog performance";
    }
    
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
    QString inputFile, outputFile;
    bool headlessMode = false;
    
    if (!parseCommandLineArguments(app, inputFile, outputFile, headlessMode)) {
        return -1; // Parsing failed or help was shown
    }
    
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
        
        // Check if running in headless mode
        if (headlessMode) {
            qCInfo(etiMain) << "Running in headless mode";
            qCInfo(etiMain) << "Input file:" << inputFile;
            qCInfo(etiMain) << "Output file:" << outputFile;
            
            // Create headless processor
            HeadlessETIProcessor processor;
            
            // Connect progress signals for console output
            QObject::connect(&processor, &HeadlessETIProcessor::status_changed,
                           [](const QString& status) {
                               qCInfo(etiMain) << "Status:" << status;
                           });
                           
            QObject::connect(&processor, &HeadlessETIProcessor::progress_changed,
                           [](double progress) {
                               qCInfo(etiMain) << "Progress:" << QString::number(progress * 100, 'f', 1) << "%";
                           });
            
            // Process the file
            qCInfo(etiMain) << "Starting headless ETI processing...";
            auto result = processor.process_file(inputFile, outputFile);
            
            if (result.success) {
                qCInfo(etiMain) << "Processing completed successfully";
                qCInfo(etiMain) << "Frames processed:" << result.processedFrames << "/" << result.totalFrames;
                qCInfo(etiMain) << "Processing time:" << result.processingTimeMs << "ms";
                qCInfo(etiMain) << "Average FPS:" << QString::number(result.averageFPS, 'f', 2);
                qCInfo(etiMain) << "Services found:" << result.servicesFound;
                qCInfo(etiMain) << "ETSI compliant:" << (result.etsiCompliant ? "Yes" : "No");
                qCInfo(etiMain) << "Results saved to:" << outputFile;
                return 0;
            } else {
                qCCritical(etiMain) << "Processing failed:" << result.errorMessage;
                return -1;
            }
        } else {
            // GUI mode
            qCInfo(etiMain) << "Running in GUI mode";
            
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
        }
        
    } catch (const std::exception &e) {
        qCCritical(etiMain) << "Fatal error during application startup:" << e.what();
        QMessageBox::critical(nullptr, "Fatal Error",
            QString("Application failed to start:\n%1").arg(e.what()));
        return -1;
    }
}