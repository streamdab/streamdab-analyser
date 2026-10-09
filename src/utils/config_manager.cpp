#include "config_manager.h"
#include "logger.h"

#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

// Configuration keys
const QString ConfigManager::WINDOW_GEOMETRY = "window/geometry";
const QString ConfigManager::WINDOW_STATE = "window/state";
const QString ConfigManager::LAST_FILE_PATH = "files/lastPath";
const QString ConfigManager::UPDATE_RATE = "display/updateRate";
const QString ConfigManager::REAL_TIME_MODE = "processing/realTimeMode";
const QString ConfigManager::AUDIO_MONITORING = "audio/monitoring";
const QString ConfigManager::ERROR_LOGGING = "logging/errorLevel";

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
}

ConfigManager::~ConfigManager()
{
    if (m_settings) {
        m_settings->sync();
    }
    Logger::instance().log(Logger::Info, "ConfigManager", "Destructor called");
}

bool ConfigManager::initialize()
{
    Logger::instance().log(Logger::Info, "ConfigManager", "Initializing configuration manager");
    
    try {
        // Setup application info for QSettings
        QCoreApplication::setOrganizationName("ETI Stream Analyser");
        QCoreApplication::setApplicationName("ETI Stream Analyser");
        QCoreApplication::setApplicationVersion("1.0.0");
        
        // Create settings instance
        m_settings = std::make_unique<QSettings>();
        
        // Get settings file path
        m_configPath = m_settings->fileName();
        
        // Ensure settings directory exists
        QFileInfo settingsInfo(m_configPath);
        QDir settingsDir = settingsInfo.dir();
        if (!settingsDir.exists()) {
            settingsDir.mkpath(".");
        }
        
        // Setup default values
        setupDefaults();
        
        m_initialized = true;
        
        Logger::instance().log(Logger::Info, "ConfigManager", 
                              QString("Configuration manager initialized. Settings file: %1")
                              .arg(m_configPath));
        return true;
    }
    catch (const std::exception& e) {
        QString error = QString("Failed to initialize configuration manager: %1").arg(e.what());
        Logger::instance().log(Logger::Error, "ConfigManager", error);
        return false;
    }
}

QVariant ConfigManager::getValue(const QString& key, const QVariant& defaultValue) const
{
    if (!m_initialized || !m_settings) {
        Logger::instance().log(Logger::Warning, "ConfigManager", "Configuration not initialized");
        return defaultValue;
    }
    
    return m_settings->value(key, defaultValue);
}

void ConfigManager::setValue(const QString& key, const QVariant& value)
{
    if (!m_initialized || !m_settings) {
        Logger::instance().log(Logger::Warning, "ConfigManager", "Configuration not initialized");
        return;
    }
    
    QVariant oldValue = m_settings->value(key);
    m_settings->setValue(key, value);
    
    if (oldValue != value) {
        emit configurationChanged(key, value);
        Logger::instance().log(Logger::Debug, "ConfigManager", 
                              QString("Configuration changed: %1 = %2").arg(key, value.toString()));
    }
}

bool ConfigManager::contains(const QString& key) const
{
    if (!m_initialized || !m_settings) {
        return false;
    }
    
    return m_settings->contains(key);
}

void ConfigManager::remove(const QString& key)
{
    if (!m_initialized || !m_settings) {
        Logger::instance().log(Logger::Warning, "ConfigManager", "Configuration not initialized");
        return;
    }
    
    m_settings->remove(key);
    Logger::instance().log(Logger::Debug, "ConfigManager", QString("Configuration key removed: %1").arg(key));
}

void ConfigManager::clear()
{
    if (!m_initialized || !m_settings) {
        Logger::instance().log(Logger::Warning, "ConfigManager", "Configuration not initialized");
        return;
    }
    
    m_settings->clear();
    setupDefaults();
    Logger::instance().log(Logger::Info, "ConfigManager", "Configuration cleared and defaults restored");
}

void ConfigManager::sync()
{
    if (!m_initialized || !m_settings) {
        Logger::instance().log(Logger::Warning, "ConfigManager", "Configuration not initialized");
        return;
    }
    
    m_settings->sync();
    emit configurationSaved();
    Logger::instance().log(Logger::Debug, "ConfigManager", "Configuration synchronized to disk");
}

QString ConfigManager::getSettingsPath() const
{
    return m_configPath;
}

void ConfigManager::setupDefaults()
{
    if (!m_settings) return;
    
    // Set default values if they don't exist
    if (!m_settings->contains(UPDATE_RATE)) {
        m_settings->setValue(UPDATE_RATE, 60); // 60 FPS
    }
    
    if (!m_settings->contains(REAL_TIME_MODE)) {
        m_settings->setValue(REAL_TIME_MODE, false);
    }
    
    if (!m_settings->contains(AUDIO_MONITORING)) {
        m_settings->setValue(AUDIO_MONITORING, true);
    }
    
    if (!m_settings->contains(ERROR_LOGGING)) {
        m_settings->setValue(ERROR_LOGGING, "Info");
    }
    
    if (!m_settings->contains(LAST_FILE_PATH)) {
        QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        m_settings->setValue(LAST_FILE_PATH, documentsPath);
    }
    
    Logger::instance().log(Logger::Debug, "ConfigManager", "Default configuration values set");
}