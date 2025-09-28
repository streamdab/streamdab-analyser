/**
 * @file configuration_manager.cpp
 * @brief Implementation of Configuration Management System
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#include "configuration_manager.h"

#include <QApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

namespace streamdab::config {

// ApplicationConfig implementation
ConfigValidationResult ApplicationConfig::validate() const {
    ConfigValidationResult result;
    result.valid = true;
    
    // Validate window geometry
    if (window_geometry.width() < 200 || window_geometry.height() < 150) {
        result.errors.append("Invalid window geometry: too small");
        result.valid = false;
    }
    
    if (window_geometry.width() > 10000 || window_geometry.height() > 10000) {
        result.warnings.append("Window geometry is very large");
    }
    
    // Validate update interval
    if (update_interval_ms < 100 || update_interval_ms > 10000) {
        result.errors.append("Update interval must be between 100ms and 10000ms");
        result.valid = false;
    }
    
    // Validate buffer size
    if (max_buffer_size <= 0 || max_buffer_size > 100000) {
        result.errors.append("Buffer size must be between 1 and 100000");
        result.valid = false;
    }
    
    // Validate target FPS
    if (target_fps <= 0.0 || target_fps > 10000.0) {
        result.errors.append("Target FPS must be between 0.1 and 10000");
        result.valid = false;
    }
    
    // Validate recent files count
    if (max_recent_files < 0 || max_recent_files > 50) {
        result.warnings.append("Recent files count should be between 0 and 50");
    }
    
    // Validate log file path if logging to file is enabled
    if (log_to_file && log_file_path.isEmpty()) {
        result.warnings.append("Log file path is empty but file logging is enabled");
    }
    
    result.summary = QString("Configuration validation: %1 errors, %2 warnings")
                    .arg(result.errors.size())
                    .arg(result.warnings.size());
    
    return result;
}

void ApplicationConfig::resetToDefaults() {
    window_geometry = QRect(100, 100, 1200, 800);
    window_maximized = false;
    window_state.clear();
    
    theme = "default";
    dark_mode = false;
    language = "en";
    update_interval_ms = 500;
    auto_save_enabled = true;
    
    max_buffer_size = 1000;
    target_fps = 1000.0;
    enable_performance_monitoring = true;
    enable_gpu_acceleration = false;
    
    log_level = "INFO";
    log_file_path.clear();
    log_to_file = true;
    log_to_console = false;
    max_log_files = 10;
    
    recent_files.clear();
    max_recent_files = 10;
}

QJsonObject ApplicationConfig::toJson() const {
    QJsonObject json;
    
    // Window settings
    QJsonObject window_obj;
    window_obj["x"] = window_geometry.x();
    window_obj["y"] = window_geometry.y();
    window_obj["width"] = window_geometry.width();
    window_obj["height"] = window_geometry.height();
    window_obj["maximized"] = window_maximized;
    window_obj["state"] = QString(window_state.toBase64());
    json["window"] = window_obj;
    
    // UI preferences
    QJsonObject ui_obj;
    ui_obj["theme"] = theme;
    ui_obj["dark_mode"] = dark_mode;
    ui_obj["language"] = language;
    ui_obj["update_interval_ms"] = update_interval_ms;
    ui_obj["auto_save_enabled"] = auto_save_enabled;
    json["ui"] = ui_obj;
    
    // Performance settings
    QJsonObject perf_obj;
    perf_obj["max_buffer_size"] = max_buffer_size;
    perf_obj["target_fps"] = target_fps;
    perf_obj["enable_performance_monitoring"] = enable_performance_monitoring;
    perf_obj["enable_gpu_acceleration"] = enable_gpu_acceleration;
    json["performance"] = perf_obj;
    
    // Logging settings
    QJsonObject log_obj;
    log_obj["log_level"] = log_level;
    log_obj["log_file_path"] = log_file_path;
    log_obj["log_to_file"] = log_to_file;
    log_obj["log_to_console"] = log_to_console;
    log_obj["max_log_files"] = max_log_files;
    json["logging"] = log_obj;
    
    // Recent files
    QJsonArray recent_array;
    for (const auto& file : recent_files) {
        recent_array.append(file);
    }
    json["recent_files"] = recent_array;
    json["max_recent_files"] = max_recent_files;
    
    return json;
}

bool ApplicationConfig::fromJson(const QJsonObject& json) {
    try {
        // Window settings
        if (json.contains("window")) {
            QJsonObject window_obj = json["window"].toObject();
            window_geometry = QRect(
                window_obj["x"].toInt(100),
                window_obj["y"].toInt(100),
                window_obj["width"].toInt(1200),
                window_obj["height"].toInt(800)
            );
            window_maximized = window_obj["maximized"].toBool(false);
            window_state = QByteArray::fromBase64(window_obj["state"].toString().toUtf8());
        }
        
        // UI preferences
        if (json.contains("ui")) {
            QJsonObject ui_obj = json["ui"].toObject();
            theme = ui_obj["theme"].toString("default");
            dark_mode = ui_obj["dark_mode"].toBool(false);
            language = ui_obj["language"].toString("en");
            update_interval_ms = ui_obj["update_interval_ms"].toInt(500);
            auto_save_enabled = ui_obj["auto_save_enabled"].toBool(true);
        }
        
        // Performance settings
        if (json.contains("performance")) {
            QJsonObject perf_obj = json["performance"].toObject();
            max_buffer_size = perf_obj["max_buffer_size"].toInt(1000);
            target_fps = perf_obj["target_fps"].toDouble(1000.0);
            enable_performance_monitoring = perf_obj["enable_performance_monitoring"].toBool(true);
            enable_gpu_acceleration = perf_obj["enable_gpu_acceleration"].toBool(false);
        }
        
        // Logging settings
        if (json.contains("logging")) {
            QJsonObject log_obj = json["logging"].toObject();
            log_level = log_obj["log_level"].toString("INFO");
            log_file_path = log_obj["log_file_path"].toString();
            log_to_file = log_obj["log_to_file"].toBool(true);
            log_to_console = log_obj["log_to_console"].toBool(false);
            max_log_files = log_obj["max_log_files"].toInt(10);
        }
        
        // Recent files
        recent_files.clear();
        if (json.contains("recent_files")) {
            QJsonArray recent_array = json["recent_files"].toArray();
            for (const auto& value : recent_array) {
                recent_files.append(value.toString());
            }
        }
        max_recent_files = json["max_recent_files"].toInt(10);
        
        return true;
    } catch (...) {
        return false;
    }
}

// StreamingConfig implementation
ConfigValidationResult StreamingConfig::validate() const {
    ConfigValidationResult result;
    result.valid = true;
    
    // Validate connection configuration
    auto connection_validation = ui::connection_utils::validateConfig(connection);
    if (!connection_validation.valid) {
        result.valid = false;
        result.errors.append("Invalid connection configuration");
        for (const auto& error : connection_validation.errors) {
            result.errors.append(QString("Connection: %1").arg(error));
        }
    }
    
    // Validate quality thresholds
    if (min_signal_quality < 0.0 || min_signal_quality > 100.0) {
        result.errors.append("Signal quality threshold must be between 0 and 100");
        result.valid = false;
    }
    
    if (min_etsi_compliance < 0.0 || min_etsi_compliance > 100.0) {
        result.errors.append("ETSI compliance threshold must be between 0 and 100");
        result.valid = false;
    }
    
    // Validate buffer settings
    if (buffer_size == 0 || buffer_size > 100000) {
        result.errors.append("Buffer size must be between 1 and 100000");
        result.valid = false;
    }
    
    if (max_buffer_overflows == 0) {
        result.warnings.append("Buffer overflow threshold is set to 0");
    }
    
    // Validate monitoring settings
    if (monitoring_update_interval < 100 || monitoring_update_interval > 10000) {
        result.warnings.append("Monitoring update interval should be between 100ms and 10000ms");
    }
    
    // Validate statistics file path if saving is enabled
    if (save_statistics && statistics_file_path.isEmpty()) {
        result.warnings.append("Statistics file path is empty but statistics saving is enabled");
    }
    
    result.summary = QString("Streaming configuration validation: %1 errors, %2 warnings")
                    .arg(result.errors.size())
                    .arg(result.warnings.size());
    
    return result;
}

void StreamingConfig::resetToDefaults() {
    connection = ui::connection_utils::createDefaultConfig();
    
    min_signal_quality = 80.0;
    min_etsi_compliance = 85.0;
    enable_quality_alerts = true;
    
    buffer_size = 1000;
    adaptive_buffering = true;
    max_buffer_overflows = 100;
    
    enable_real_time_monitoring = true;
    monitoring_update_interval = 500;
    save_statistics = true;
    statistics_file_path.clear();
}

QJsonObject StreamingConfig::toJson() const {
    QJsonObject json;
    
    // Connection settings
    QJsonObject conn_obj;
    conn_obj["protocol"] = connection.protocol;
    conn_obj["host"] = connection.host;
    conn_obj["port"] = connection.port;
    conn_obj["full_url"] = connection.full_url;
    conn_obj["enable_tist"] = connection.enable_tist;
    conn_obj["enable_metadata"] = connection.enable_metadata;
    conn_obj["buffer_frames"] = static_cast<int>(connection.buffer_frames);
    conn_obj["timeout_ms"] = static_cast<int>(connection.timeout_ms);
    conn_obj["auto_reconnect"] = connection.auto_reconnect;
    conn_obj["reconnect_interval_ms"] = static_cast<int>(connection.reconnect_interval_ms);
    conn_obj["min_signal_quality"] = connection.min_signal_quality;
    conn_obj["enable_etsi_compliance"] = connection.enable_etsi_compliance;
    conn_obj["enable_error_correction"] = connection.enable_error_correction;
    conn_obj["target_fps"] = connection.target_fps;
    conn_obj["adaptive_buffering"] = connection.adaptive_buffering;
    conn_obj["performance_monitoring"] = connection.performance_monitoring;
    json["connection"] = conn_obj;
    
    // Quality settings
    QJsonObject quality_obj;
    quality_obj["min_signal_quality"] = min_signal_quality;
    quality_obj["min_etsi_compliance"] = min_etsi_compliance;
    quality_obj["enable_quality_alerts"] = enable_quality_alerts;
    json["quality"] = quality_obj;
    
    // Buffer settings
    QJsonObject buffer_obj;
    buffer_obj["buffer_size"] = static_cast<int>(buffer_size);
    buffer_obj["adaptive_buffering"] = adaptive_buffering;
    buffer_obj["max_buffer_overflows"] = static_cast<int>(max_buffer_overflows);
    json["buffer"] = buffer_obj;
    
    // Monitoring settings
    QJsonObject monitor_obj;
    monitor_obj["enable_real_time_monitoring"] = enable_real_time_monitoring;
    monitor_obj["monitoring_update_interval"] = monitoring_update_interval;
    monitor_obj["save_statistics"] = save_statistics;
    monitor_obj["statistics_file_path"] = statistics_file_path;
    json["monitoring"] = monitor_obj;
    
    return json;
}

bool StreamingConfig::fromJson(const QJsonObject& json) {
    try {
        // Connection settings
        if (json.contains("connection")) {
            QJsonObject conn_obj = json["connection"].toObject();
            connection.protocol = conn_obj["protocol"].toString("tcp");
            connection.host = conn_obj["host"].toString("127.0.0.1");
            connection.port = static_cast<quint16>(conn_obj["port"].toInt(9200));
            connection.full_url = conn_obj["full_url"].toString();
            connection.enable_tist = conn_obj["enable_tist"].toBool(true);
            connection.enable_metadata = conn_obj["enable_metadata"].toBool(true);
            connection.buffer_frames = static_cast<uint32_t>(conn_obj["buffer_frames"].toInt(1000));
            connection.timeout_ms = static_cast<uint32_t>(conn_obj["timeout_ms"].toInt(5000));
            connection.auto_reconnect = conn_obj["auto_reconnect"].toBool(true);
            connection.reconnect_interval_ms = static_cast<uint32_t>(conn_obj["reconnect_interval_ms"].toInt(5000));
            connection.min_signal_quality = conn_obj["min_signal_quality"].toDouble(80.0);
            connection.enable_etsi_compliance = conn_obj["enable_etsi_compliance"].toBool(true);
            connection.enable_error_correction = conn_obj["enable_error_correction"].toBool(true);
            connection.target_fps = conn_obj["target_fps"].toDouble(1000.0);
            connection.adaptive_buffering = conn_obj["adaptive_buffering"].toBool(true);
            connection.performance_monitoring = conn_obj["performance_monitoring"].toBool(true);
            
            // Update full URL
            connection.updateFullURL();
        }
        
        // Quality settings
        if (json.contains("quality")) {
            QJsonObject quality_obj = json["quality"].toObject();
            min_signal_quality = quality_obj["min_signal_quality"].toDouble(80.0);
            min_etsi_compliance = quality_obj["min_etsi_compliance"].toDouble(85.0);
            enable_quality_alerts = quality_obj["enable_quality_alerts"].toBool(true);
        }
        
        // Buffer settings
        if (json.contains("buffer")) {
            QJsonObject buffer_obj = json["buffer"].toObject();
            buffer_size = static_cast<uint32_t>(buffer_obj["buffer_size"].toInt(1000));
            adaptive_buffering = buffer_obj["adaptive_buffering"].toBool(true);
            max_buffer_overflows = static_cast<uint32_t>(buffer_obj["max_buffer_overflows"].toInt(100));
        }
        
        // Monitoring settings
        if (json.contains("monitoring")) {
            QJsonObject monitor_obj = json["monitoring"].toObject();
            enable_real_time_monitoring = monitor_obj["enable_real_time_monitoring"].toBool(true);
            monitoring_update_interval = monitor_obj["monitoring_update_interval"].toInt(500);
            save_statistics = monitor_obj["save_statistics"].toBool(true);
            statistics_file_path = monitor_obj["statistics_file_path"].toString();
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

// ConfigurationManager implementation
ConfigurationManager::ConfigurationManager(QObject* parent)
    : QObject(parent)
    , auto_save_timer_(new QTimer(this))
{
    // Setup auto-save timer
    auto_save_timer_->setSingleShot(false);
    auto_save_timer_->setInterval(auto_save_interval_);
    connect(auto_save_timer_, &QTimer::timeout, this, &ConfigurationManager::autoSave);
    
    // Initialize default configurations
    app_config_.resetToDefaults();
    streaming_config_.resetToDefaults();
}

ConfigurationManager::~ConfigurationManager() {
    if (auto_save_enabled_ && config_dirty_) {
        saveApplicationSettings();
    }
}

bool ConfigurationManager::initialize(const QString& organization, const QString& application) {
    organization_ = organization;
    application_ = application;
    
    // Create application settings
    app_settings_ = std::make_unique<QSettings>(organization_, application_);
    
    // Setup configuration directories
    setupConfigDirectories();
    
    // Load existing configuration
    loadApplicationSettings();
    
    // Start auto-save timer if enabled
    if (auto_save_enabled_) {
        auto_save_timer_->start();
    }
    
    initialized_ = true;
    return true;
}

bool ConfigurationManager::saveConfigurationFile(const QString& filename, ConfigFormat format) {
    if (!initialized_) {
        emit configurationError("Configuration manager not initialized");
        return false;
    }
    
    QFileInfo file_info(filename);
    if (!ensureDirectoryExists(file_info.absolutePath())) {
        emit configurationError(QString("Cannot create directory: %1").arg(file_info.absolutePath()));
        return false;
    }
    
    bool success = false;
    
    try {
        switch (format) {
            case ConfigFormat::INI: {
                QSettings file_settings(filename, QSettings::IniFormat);
                saveToQSettings(file_settings);
                file_settings.sync();
                success = (file_settings.status() == QSettings::NoError);
                break;
            }
            
            case ConfigFormat::JSON: {
                QJsonDocument json_doc = saveToJson(true); // Include sensitive data for save
                QFile file(filename);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(json_doc.toJson());
                    success = true;
                }
                break;
            }
            
            case ConfigFormat::XML:
                // XML format implementation would go here
                emit configurationError("XML format not yet implemented");
                return false;
        }
        
        if (success) {
            last_save_time_ = QDateTime::currentDateTime();
            config_dirty_ = false;
            emit configurationSaved(filename);
        } else {
            emit configurationError(QString("Failed to save configuration to %1").arg(filename));
        }
        
    } catch (const std::exception& e) {
        emit configurationError(QString("Exception while saving configuration: %1").arg(e.what()));
        success = false;
    }
    
    return success;
}

bool ConfigurationManager::loadConfigurationFile(const QString& filename) {
    if (!initialized_) {
        emit configurationError("Configuration manager not initialized");
        return false;
    }
    
    if (!QFile::exists(filename)) {
        emit configurationError(QString("Configuration file does not exist: %1").arg(filename));
        return false;
    }
    
    ConfigFormat format = detectFileFormat(filename);
    bool success = false;
    
    try {
        switch (format) {
            case ConfigFormat::INI: {
                QSettings file_settings(filename, QSettings::IniFormat);
                if (file_settings.status() == QSettings::NoError) {
                    loadFromQSettings(file_settings);
                    success = true;
                }
                break;
            }
            
            case ConfigFormat::JSON: {
                QFile file(filename);
                if (file.open(QIODevice::ReadOnly)) {
                    QJsonParseError parse_error;
                    QJsonDocument json_doc = QJsonDocument::fromJson(file.readAll(), &parse_error);
                    if (parse_error.error == QJsonParseError::NoError) {
                        success = loadFromJson(json_doc);
                    } else {
                        emit configurationError(QString("JSON parse error: %1").arg(parse_error.errorString()));
                    }
                }
                break;
            }
            
            case ConfigFormat::XML:
                emit configurationError("XML format not yet implemented");
                return false;
        }
        
        if (success) {
            last_load_time_ = QDateTime::currentDateTime();
            config_dirty_ = false;
            emit configurationLoaded(filename);
        } else {
            emit configurationError(QString("Failed to load configuration from %1").arg(filename));
        }
        
    } catch (const std::exception& e) {
        emit configurationError(QString("Exception while loading configuration: %1").arg(e.what()));
        success = false;
    }
    
    return success;
}

bool ConfigurationManager::exportConfiguration(const QString& filename, ConfigFormat format, bool include_sensitive) {
    if (!initialized_) {
        emit configurationError("Configuration manager not initialized");
        return false;
    }
    
    QFileInfo file_info(filename);
    if (!ensureDirectoryExists(file_info.absolutePath())) {
        emit configurationError(QString("Cannot create directory: %1").arg(file_info.absolutePath()));
        return false;
    }
    
    bool success = false;
    
    try {
        switch (format) {
            case ConfigFormat::JSON: {
                QJsonDocument json_doc = saveToJson(include_sensitive);
                
                // Add export metadata
                QJsonObject root = json_doc.object();
                QJsonObject export_info;
                export_info["export_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
                export_info["export_version"] = getConfigVersion();
                export_info["export_application"] = QString("%1 %2").arg(organization_, application_);
                export_info["include_sensitive"] = include_sensitive;
                root["export_info"] = export_info;
                
                QFile file(filename);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(QJsonDocument(root).toJson());
                    success = true;
                }
                break;
            }
            
            case ConfigFormat::INI: {
                // For INI export, use QSettings
                QSettings export_settings(filename, QSettings::IniFormat);
                saveToQSettings(export_settings);
                
                // Add export metadata
                export_settings.beginGroup("ExportInfo");
                export_settings.setValue("export_timestamp", QDateTime::currentDateTime());
                export_settings.setValue("export_version", getConfigVersion());
                export_settings.setValue("export_application", QString("%1 %2").arg(organization_, application_));
                export_settings.setValue("include_sensitive", include_sensitive);
                export_settings.endGroup();
                
                export_settings.sync();
                success = (export_settings.status() == QSettings::NoError);
                break;
            }
            
            case ConfigFormat::XML:
                emit configurationError("XML export format not yet implemented");
                return false;
        }
        
        emit configurationExported(filename, success);
        
    } catch (const std::exception& e) {
        emit configurationError(QString("Exception while exporting configuration: %1").arg(e.what()));
        success = false;
    }
    
    return success;
}

bool ConfigurationManager::importConfiguration(const QString& filename, bool merge_with_existing) {
    if (!initialized_) {
        emit configurationError("Configuration manager not initialized");
        return false;
    }
    
    if (!QFile::exists(filename)) {
        emit configurationError(QString("Import file does not exist: %1").arg(filename));
        return false;
    }
    
    // Backup current configuration before import
    QString backup_description = QString("Pre-import backup (%1)").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    if (!createBackup(backup_description)) {
        qWarning() << "Failed to create backup before import";
    }
    
    ApplicationConfig backup_app_config = app_config_;
    StreamingConfig backup_streaming_config = streaming_config_;
    
    bool success = false;
    
    if (!merge_with_existing) {
        // Reset to defaults before importing
        app_config_.resetToDefaults();
        streaming_config_.resetToDefaults();
    }
    
    // Load the configuration
    success = loadConfigurationFile(filename);
    
    if (!success) {
        // Restore backup on failure
        app_config_ = backup_app_config;
        streaming_config_ = backup_streaming_config;
        emit configurationError("Import failed, configuration restored");
    } else {
        config_dirty_ = true;
        emit configurationChanged();
        emit configurationImported(filename, true);
    }
    
    return success;
}

void ConfigurationManager::saveApplicationSettings() {
    if (!initialized_ || !app_settings_) {
        return;
    }
    
    saveToQSettings(*app_settings_);
    app_settings_->sync();
    
    config_dirty_ = false;
    last_save_time_ = QDateTime::currentDateTime();
    
    emit configurationSaved(getDefaultConfigPath());
}

void ConfigurationManager::loadApplicationSettings() {
    if (!initialized_ || !app_settings_) {
        return;
    }
    
    // Check if configuration exists and migrate if needed
    if (app_settings_->allKeys().isEmpty()) {
        // First run, use defaults
        app_config_.resetToDefaults();
        streaming_config_.resetToDefaults();
    } else {
        // Load existing configuration
        if (!migrateConfiguration(*app_settings_)) {
            qWarning() << "Configuration migration failed, using defaults";
            app_config_.resetToDefaults();
            streaming_config_.resetToDefaults();
        } else {
            loadFromQSettings(*app_settings_);
        }
    }
    
    last_load_time_ = QDateTime::currentDateTime();
    config_dirty_ = false;
}

QString ConfigurationManager::getDefaultConfigPath(ConfigScope scope) const {
    switch (scope) {
        case ConfigScope::APPLICATION:
        case ConfigScope::USER:
            return user_config_path_;
        case ConfigScope::SESSION:
            return config_utils::getTempConfigDirectory() + "/session_config.ini";
        case ConfigScope::PROJECT:
            return QDir::currentPath() + "/project_config.ini";
        default:
            return user_config_path_;
    }
}

QString ConfigurationManager::getConfigDirectory(ConfigScope scope) const {
    switch (scope) {
        case ConfigScope::APPLICATION:
        case ConfigScope::USER:
            return config_dir_;
        case ConfigScope::SESSION:
            return config_utils::getTempConfigDirectory();
        case ConfigScope::PROJECT:
            return QDir::currentPath();
        default:
            return config_dir_;
    }
}

void ConfigurationManager::setApplicationConfig(const ApplicationConfig& config) {
    app_config_ = config;
    config_dirty_ = true;
    emit configurationChanged();
}

void ConfigurationManager::setStreamingConfig(const StreamingConfig& config) {
    streaming_config_ = config;
    config_dirty_ = true;
    emit configurationChanged();
}

bool ConfigurationManager::createBackup(const QString& description) {
    if (!initialized_) {
        return false;
    }
    
    QString backup_filename = generateBackupFilename(description);
    QString backup_path = backup_dir_ + "/" + backup_filename;
    
    if (!ensureDirectoryExists(backup_dir_)) {
        return false;
    }
    
    bool success = saveConfigurationFile(backup_path, ConfigFormat::INI);
    
    if (success) {
        ConfigBackup backup_info;
        backup_info.filename = backup_path;
        backup_info.description = description.isEmpty() ? "Automatic backup" : description;
        backup_info.timestamp = QDateTime::currentDateTime();
        backup_info.file_size = QFileInfo(backup_path).size();
        backup_info.format = ConfigFormat::INI;
        
        emit backupCreated(backup_info);
    }
    
    return success;
}

bool ConfigurationManager::restoreFromBackup(const QString& backup_filename) {
    if (!QFile::exists(backup_filename)) {
        emit configurationError(QString("Backup file does not exist: %1").arg(backup_filename));
        return false;
    }
    
    // Create a backup of current state before restore
    if (!createBackup("Pre-restore backup")) {
        qWarning() << "Failed to create pre-restore backup";
    }
    
    bool success = loadConfigurationFile(backup_filename);
    
    if (success) {
        config_dirty_ = true;
        emit configurationChanged();
        emit backupRestored(backup_filename);
    }
    
    return success;
}

QList<ConfigBackup> ConfigurationManager::getAvailableBackups() const {
    QList<ConfigBackup> backups;
    
    if (!QDir(backup_dir_).exists()) {
        return backups;
    }
    
    QDir backup_directory(backup_dir_);
    QStringList filters{"backup_*.ini"};
    QFileInfoList backup_files = backup_directory.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    
    for (const auto& file_info : backup_files) {
        ConfigBackup backup;
        backup.filename = file_info.absoluteFilePath();
        backup.timestamp = file_info.lastModified();
        backup.file_size = file_info.size();
        backup.format = ConfigFormat::INI;
        
        // Extract description from filename if possible
        QString basename = file_info.baseName();
        QRegularExpression desc_regex(R"(backup_(.+)_\d{8}_\d{6})");
        QRegularExpressionMatch match = desc_regex.match(basename);
        if (match.hasMatch()) {
            backup.description = match.captured(1).replace('_', ' ');
        } else {
            backup.description = "Backup";
        }
        
        backups.append(backup);
    }
    
    return backups;
}

int ConfigurationManager::cleanOldBackups(int max_age_days, int max_count) {
    QList<ConfigBackup> backups = getAvailableBackups();
    int removed_count = 0;
    
    // Remove backups older than max_age_days
    QDateTime cutoff_date = QDateTime::currentDateTime().addDays(-max_age_days);
    
    for (const auto& backup : backups) {
        bool should_remove = false;
        
        if (backup.timestamp < cutoff_date) {
            should_remove = true;
        }
        
        if (should_remove) {
            if (QFile::remove(backup.filename)) {
                removed_count++;
            }
        }
    }
    
    // Remove excess backups beyond max_count
    backups = getAvailableBackups(); // Refresh list after deletions
    if (backups.size() > max_count) {
        // Sort by timestamp (oldest first) and remove excess
        std::sort(backups.begin(), backups.end(), [](const ConfigBackup& a, const ConfigBackup& b) {
            return a.timestamp < b.timestamp;
        });
        
        int excess_count = backups.size() - max_count;
        for (int i = 0; i < excess_count; ++i) {
            if (QFile::remove(backups[i].filename)) {
                removed_count++;
            }
        }
    }
    
    return removed_count;
}

ConfigValidationResult ConfigurationManager::validateConfiguration() const {
    ConfigValidationResult app_result = app_config_.validate();
    ConfigValidationResult streaming_result = streaming_config_.validate();
    
    ConfigValidationResult combined_result;
    combined_result.valid = app_result.valid && streaming_result.valid;
    
    // Combine errors and warnings
    for (const auto& error : app_result.errors) {
        combined_result.errors.append(QString("Application: %1").arg(error));
    }
    for (const auto& error : streaming_result.errors) {
        combined_result.errors.append(QString("Streaming: %1").arg(error));
    }
    
    for (const auto& warning : app_result.warnings) {
        combined_result.warnings.append(QString("Application: %1").arg(warning));
    }
    for (const auto& warning : streaming_result.warnings) {
        combined_result.warnings.append(QString("Streaming: %1").arg(warning));
    }
    
    combined_result.summary = QString("Overall validation: %1 errors, %2 warnings")
                             .arg(combined_result.errors.size())
                             .arg(combined_result.warnings.size());
    
    return combined_result;
}

void ConfigurationManager::resetToDefaults(ConfigScope scope) {
    switch (scope) {
        case ConfigScope::APPLICATION:
            app_config_.resetToDefaults();
            streaming_config_.resetToDefaults();
            break;
        case ConfigScope::USER:
            app_config_.resetToDefaults();
            break;
        case ConfigScope::SESSION:
        case ConfigScope::PROJECT:
            // These scopes don't have defaults to reset
            break;
    }
    
    config_dirty_ = true;
    emit configurationChanged();
}

void ConfigurationManager::addRecentFile(const QString& filename) {
    recent_files_.removeAll(filename); // Remove if already exists
    recent_files_.prepend(filename);   // Add to front
    
    // Limit to max count
    while (recent_files_.size() > max_recent_files_) {
        recent_files_.removeLast();
    }
    
    app_config_.recent_files = recent_files_;
    config_dirty_ = true;
    
    emit recentFilesChanged(recent_files_);
}

void ConfigurationManager::removeRecentFile(const QString& filename) {
    if (recent_files_.removeAll(filename) > 0) {
        app_config_.recent_files = recent_files_;
        config_dirty_ = true;
        emit recentFilesChanged(recent_files_);
    }
}

QStringList ConfigurationManager::getRecentFiles() const {
    return recent_files_;
}

void ConfigurationManager::clearRecentFiles() {
    if (!recent_files_.isEmpty()) {
        recent_files_.clear();
        app_config_.recent_files = recent_files_;
        config_dirty_ = true;
        emit recentFilesChanged(recent_files_);
    }
}

ConfigFormat ConfigurationManager::detectFileFormat(const QString& filename) {
    QFileInfo file_info(filename);
    QString extension = file_info.suffix().toLower();
    
    if (extension == "ini" || extension == "conf") {
        return ConfigFormat::INI;
    } else if (extension == "json") {
        return ConfigFormat::JSON;
    } else if (extension == "xml") {
        return ConfigFormat::XML;
    } else {
        // Try to detect from content
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray content = file.read(1024); // Read first 1KB
            if (content.startsWith('{') || content.startsWith('[')) {
                return ConfigFormat::JSON;
            } else if (content.contains("<?xml")) {
                return ConfigFormat::XML;
            }
        }
        
        // Default to INI
        return ConfigFormat::INI;
    }
}

bool ConfigurationManager::isValidConfigFile(const QString& filename) {
    if (!QFile::exists(filename)) {
        return false;
    }
    
    ConfigFormat format = detectFileFormat(filename);
    
    switch (format) {
        case ConfigFormat::INI: {
            QSettings settings(filename, QSettings::IniFormat);
            return settings.status() == QSettings::NoError && !settings.allKeys().isEmpty();
        }
        
        case ConfigFormat::JSON: {
            QFile file(filename);
            if (file.open(QIODevice::ReadOnly)) {
                QJsonParseError error;
                QJsonDocument::fromJson(file.readAll(), &error);
                return error.error == QJsonParseError::NoError;
            }
            return false;
        }
        
        case ConfigFormat::XML:
            // XML validation would go here
            return false;
    }
    
    return false;
}

void ConfigurationManager::autoSave() {
    if (config_dirty_ && auto_save_enabled_) {
        saveApplicationSettings();
    }
}

void ConfigurationManager::reloadConfiguration() {
    loadApplicationSettings();
    emit configurationChanged();
}

void ConfigurationManager::updateSetting(const QString& key, const QVariant& value) {
    if (!app_settings_) {
        return;
    }
    
    app_settings_->setValue(key, value);
    config_dirty_ = true;
    emit configurationChanged();
}

void ConfigurationManager::setupConfigDirectories() {
    config_dir_ = config_utils::getUserConfigDirectory();
    backup_dir_ = config_dir_ + "/backups";
    user_config_path_ = config_dir_ + "/config.ini";
    system_config_path_ = config_utils::getSystemConfigDirectory() + "/config.ini";
    
    ensureDirectoryExists(config_dir_);
    ensureDirectoryExists(backup_dir_);
}

void ConfigurationManager::loadFromQSettings(QSettings& settings) {
    // Load application config
    settings.beginGroup("Application");
    
    // Window settings
    app_config_.window_geometry = settings.value("window_geometry", QRect(100, 100, 1200, 800)).toRect();
    app_config_.window_maximized = settings.value("window_maximized", false).toBool();
    app_config_.window_state = settings.value("window_state").toByteArray();
    
    // UI preferences
    app_config_.theme = settings.value("theme", "default").toString();
    app_config_.dark_mode = settings.value("dark_mode", false).toBool();
    app_config_.language = settings.value("language", "en").toString();
    app_config_.update_interval_ms = settings.value("update_interval_ms", 500).toInt();
    app_config_.auto_save_enabled = settings.value("auto_save_enabled", true).toBool();
    
    // Performance settings
    app_config_.max_buffer_size = settings.value("max_buffer_size", 1000).toInt();
    app_config_.target_fps = settings.value("target_fps", 1000.0).toDouble();
    app_config_.enable_performance_monitoring = settings.value("enable_performance_monitoring", true).toBool();
    app_config_.enable_gpu_acceleration = settings.value("enable_gpu_acceleration", false).toBool();
    
    // Logging settings
    app_config_.log_level = settings.value("log_level", "INFO").toString();
    app_config_.log_file_path = settings.value("log_file_path").toString();
    app_config_.log_to_file = settings.value("log_to_file", true).toBool();
    app_config_.log_to_console = settings.value("log_to_console", false).toBool();
    app_config_.max_log_files = settings.value("max_log_files", 10).toInt();
    
    // Recent files
    app_config_.recent_files = settings.value("recent_files").toStringList();
    app_config_.max_recent_files = settings.value("max_recent_files", 10).toInt();
    
    settings.endGroup();
    
    // Load streaming config
    settings.beginGroup("Streaming");
    
    // Connection settings
    streaming_config_.connection.protocol = settings.value("connection_protocol", "tcp").toString();
    streaming_config_.connection.host = settings.value("connection_host", "127.0.0.1").toString();
    streaming_config_.connection.port = static_cast<quint16>(settings.value("connection_port", 9200).toUInt());
    streaming_config_.connection.enable_tist = settings.value("enable_tist", true).toBool();
    streaming_config_.connection.enable_metadata = settings.value("enable_metadata", true).toBool();
    streaming_config_.connection.buffer_frames = static_cast<uint32_t>(settings.value("buffer_frames", 1000).toUInt());
    streaming_config_.connection.timeout_ms = static_cast<uint32_t>(settings.value("timeout_ms", 5000).toUInt());
    streaming_config_.connection.auto_reconnect = settings.value("auto_reconnect", true).toBool();
    streaming_config_.connection.reconnect_interval_ms = static_cast<uint32_t>(settings.value("reconnect_interval_ms", 5000).toUInt());
    streaming_config_.connection.enable_etsi_compliance = settings.value("enable_etsi_compliance", true).toBool();
    streaming_config_.connection.enable_error_correction = settings.value("enable_error_correction", true).toBool();
    streaming_config_.connection.target_fps = settings.value("target_fps", 1000.0).toDouble();
    streaming_config_.connection.adaptive_buffering = settings.value("adaptive_buffering", true).toBool();
    streaming_config_.connection.performance_monitoring = settings.value("performance_monitoring", true).toBool();
    
    // Update connection full URL
    streaming_config_.connection.updateFullURL();
    
    // Quality settings
    streaming_config_.min_signal_quality = settings.value("min_signal_quality", 80.0).toDouble();
    streaming_config_.min_etsi_compliance = settings.value("min_etsi_compliance", 85.0).toDouble();
    streaming_config_.enable_quality_alerts = settings.value("enable_quality_alerts", true).toBool();
    
    // Buffer settings
    streaming_config_.buffer_size = static_cast<uint32_t>(settings.value("buffer_size", 1000).toUInt());
    streaming_config_.adaptive_buffering = settings.value("adaptive_buffering", true).toBool();
    streaming_config_.max_buffer_overflows = static_cast<uint32_t>(settings.value("max_buffer_overflows", 100).toUInt());
    
    // Monitoring settings
    streaming_config_.enable_real_time_monitoring = settings.value("enable_real_time_monitoring", true).toBool();
    streaming_config_.monitoring_update_interval = settings.value("monitoring_update_interval", 500).toInt();
    streaming_config_.save_statistics = settings.value("save_statistics", true).toBool();
    streaming_config_.statistics_file_path = settings.value("statistics_file_path").toString();
    
    settings.endGroup();
    
    // Update recent files list
    recent_files_ = app_config_.recent_files;
}

void ConfigurationManager::saveToQSettings(QSettings& settings) {
    // Save application config
    settings.beginGroup("Application");
    
    // Window settings
    settings.setValue("window_geometry", app_config_.window_geometry);
    settings.setValue("window_maximized", app_config_.window_maximized);
    settings.setValue("window_state", app_config_.window_state);
    
    // UI preferences
    settings.setValue("theme", app_config_.theme);
    settings.setValue("dark_mode", app_config_.dark_mode);
    settings.setValue("language", app_config_.language);
    settings.setValue("update_interval_ms", app_config_.update_interval_ms);
    settings.setValue("auto_save_enabled", app_config_.auto_save_enabled);
    
    // Performance settings
    settings.setValue("max_buffer_size", app_config_.max_buffer_size);
    settings.setValue("target_fps", app_config_.target_fps);
    settings.setValue("enable_performance_monitoring", app_config_.enable_performance_monitoring);
    settings.setValue("enable_gpu_acceleration", app_config_.enable_gpu_acceleration);
    
    // Logging settings
    settings.setValue("log_level", app_config_.log_level);
    settings.setValue("log_file_path", app_config_.log_file_path);
    settings.setValue("log_to_file", app_config_.log_to_file);
    settings.setValue("log_to_console", app_config_.log_to_console);
    settings.setValue("max_log_files", app_config_.max_log_files);
    
    // Recent files
    settings.setValue("recent_files", app_config_.recent_files);
    settings.setValue("max_recent_files", app_config_.max_recent_files);
    
    settings.endGroup();
    
    // Save streaming config
    settings.beginGroup("Streaming");
    
    // Connection settings
    settings.setValue("connection_protocol", streaming_config_.connection.protocol);
    settings.setValue("connection_host", streaming_config_.connection.host);
    settings.setValue("connection_port", streaming_config_.connection.port);
    settings.setValue("enable_tist", streaming_config_.connection.enable_tist);
    settings.setValue("enable_metadata", streaming_config_.connection.enable_metadata);
    settings.setValue("buffer_frames", streaming_config_.connection.buffer_frames);
    settings.setValue("timeout_ms", streaming_config_.connection.timeout_ms);
    settings.setValue("auto_reconnect", streaming_config_.connection.auto_reconnect);
    settings.setValue("reconnect_interval_ms", streaming_config_.connection.reconnect_interval_ms);
    settings.setValue("enable_etsi_compliance", streaming_config_.connection.enable_etsi_compliance);
    settings.setValue("enable_error_correction", streaming_config_.connection.enable_error_correction);
    settings.setValue("target_fps", streaming_config_.connection.target_fps);
    settings.setValue("adaptive_buffering", streaming_config_.connection.adaptive_buffering);
    settings.setValue("performance_monitoring", streaming_config_.connection.performance_monitoring);
    
    // Quality settings
    settings.setValue("min_signal_quality", streaming_config_.min_signal_quality);
    settings.setValue("min_etsi_compliance", streaming_config_.min_etsi_compliance);
    settings.setValue("enable_quality_alerts", streaming_config_.enable_quality_alerts);
    
    // Buffer settings
    settings.setValue("buffer_size", streaming_config_.buffer_size);
    settings.setValue("adaptive_buffering", streaming_config_.adaptive_buffering);
    settings.setValue("max_buffer_overflows", streaming_config_.max_buffer_overflows);
    
    // Monitoring settings
    settings.setValue("enable_real_time_monitoring", streaming_config_.enable_real_time_monitoring);
    settings.setValue("monitoring_update_interval", streaming_config_.monitoring_update_interval);
    settings.setValue("save_statistics", streaming_config_.save_statistics);
    settings.setValue("statistics_file_path", streaming_config_.statistics_file_path);
    
    settings.endGroup();
    
    // Save configuration version
    settings.setValue("config_version", getConfigVersion());
}

bool ConfigurationManager::loadFromJson(const QJsonDocument& json_doc) {
    QJsonObject root = json_doc.object();
    
    // Load application config
    if (root.contains("application")) {
        if (!app_config_.fromJson(root["application"].toObject())) {
            return false;
        }
    }
    
    // Load streaming config
    if (root.contains("streaming")) {
        if (!streaming_config_.fromJson(root["streaming"].toObject())) {
            return false;
        }
    }
    
    // Update recent files list
    recent_files_ = app_config_.recent_files;
    
    return true;
}

QJsonDocument ConfigurationManager::saveToJson(bool include_sensitive) const {
    Q_UNUSED(include_sensitive) // For future use with sensitive data filtering
    
    QJsonObject root;
    
    // Add application config
    root["application"] = app_config_.toJson();
    
    // Add streaming config
    root["streaming"] = streaming_config_.toJson();
    
    // Add metadata
    QJsonObject metadata;
    metadata["config_version"] = getConfigVersion();
    metadata["save_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    metadata["application_name"] = QString("%1 %2").arg(organization_, application_);
    root["metadata"] = metadata;
    
    return QJsonDocument(root);
}

bool ConfigurationManager::ensureDirectoryExists(const QString& path) const {
    QDir dir;
    return dir.mkpath(path);
}

QString ConfigurationManager::generateBackupFilename(const QString& description) const {
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString clean_description = description;
    clean_description.replace(' ', '_');
    clean_description.replace(QRegularExpression("[^a-zA-Z0-9_-]"), "");
    
    if (clean_description.isEmpty()) {
        return QString("backup_%1.ini").arg(timestamp);
    } else {
        return QString("backup_%1_%2.ini").arg(clean_description, timestamp);
    }
}

bool ConfigurationManager::validateFileFormat(const QString& filename, ConfigFormat expected_format) const {
    return detectFileFormat(filename) == expected_format;
}

bool ConfigurationManager::migrateConfiguration(QSettings& settings) {
    QString version = settings.value("config_version", "0.0").toString();
    
    // For now, no migration needed as this is the first version
    Q_UNUSED(version)
    
    return true;
}

QString ConfigurationManager::getConfigVersion() const {
    return QString("%1.%2").arg(CONFIG_VERSION_MAJOR).arg(CONFIG_VERSION_MINOR);
}

// Utility functions implementation
namespace config_utils {

QString getSystemConfigDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString getUserConfigDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString getTempConfigDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/StreamDAB";
}

QString formatToExtension(ConfigFormat format) {
    switch (format) {
        case ConfigFormat::INI:
            return "ini";
        case ConfigFormat::JSON:
            return "json";
        case ConfigFormat::XML:
            return "xml";
        default:
            return "ini";
    }
}

ConfigFormat extensionToFormat(const QString& extension) {
    QString ext = extension.toLower();
    if (ext == "json") {
        return ConfigFormat::JSON;
    } else if (ext == "xml") {
        return ConfigFormat::XML;
    } else {
        return ConfigFormat::INI;
    }
}

QString getFormatDescription(ConfigFormat format) {
    switch (format) {
        case ConfigFormat::INI:
            return "INI Configuration File";
        case ConfigFormat::JSON:
            return "JSON Configuration File";
        case ConfigFormat::XML:
            return "XML Configuration File";
        default:
            return "Configuration File";
    }
}

bool isValidConfigKey(const QString& key) {
    QRegularExpression key_regex(R"(^[a-zA-Z][a-zA-Z0-9_]*(/[a-zA-Z][a-zA-Z0-9_]*)*$)");
    return key_regex.match(key).hasMatch();
}

QVariant sanitizeConfigValue(const QVariant& value) {
    // Basic sanitization - remove null bytes and limit string length
    if (value.type() == QVariant::String) {
        QString str = value.toString();
        str.remove(QChar('\0'));
        if (str.length() > 10000) {
            str.truncate(10000);
        }
        return str;
    }
    
    return value;
}

ApplicationConfig createDefaultApplicationConfig() {
    ApplicationConfig config;
    config.resetToDefaults();
    return config;
}

StreamingConfig createDefaultStreamingConfig() {
    StreamingConfig config;
    config.resetToDefaults();
    return config;
}

} // namespace config_utils

} // namespace streamdab::config