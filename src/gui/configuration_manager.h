/**
 * @file configuration_manager.h
 * @brief Configuration Management System for ETI Stream Analyser
 * 
 * Professional configuration management system with .ini file support,
 * import/export functionality, cross-platform configuration paths,
 * and comprehensive validation for broadcast industry applications.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include <QObject>
#include <QString>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariant>
#include <QStringList>
#include <QDateTime>
#include <memory>

// Connection configuration
#include "connection_config_dialog.h"

namespace streamdab::config {

/**
 * @brief Configuration file format enumeration
 */
enum class ConfigFormat {
    INI = 0,        // QSettings INI format
    JSON = 1,       // JSON configuration
    XML = 2         // XML configuration (future)
};

/**
 * @brief Configuration scope enumeration
 */
enum class ConfigScope {
    APPLICATION = 0,    // Application-wide settings
    USER = 1,           // User-specific settings
    SESSION = 2,        // Session-specific settings
    PROJECT = 3         // Project-specific settings
};

/**
 * @brief Configuration validation result
 */
struct ConfigValidationResult {
    bool valid{false};
    QStringList errors;
    QStringList warnings;
    QString summary;
    
    /**
     * @brief Check if configuration has critical errors
     */
    [[nodiscard]] bool hasCriticalErrors() const {
        return !valid || !errors.isEmpty();
    }
    
    /**
     * @brief Get validation score (0-100)
     */
    [[nodiscard]] int getValidationScore() const {
        if (!valid) return 0;
        
        int score = 100;
        score -= errors.size() * 20;        // Critical errors
        score -= warnings.size() * 5;      // Warnings
        
        return std::max(0, score);
    }
};

/**
 * @brief Configuration backup information
 */
struct ConfigBackup {
    QString filename;
    QString description;
    QDateTime timestamp;
    qint64 file_size{0};
    ConfigFormat format{ConfigFormat::INI};
    
    /**
     * @brief Check if backup is valid
     */
    [[nodiscard]] bool isValid() const {
        return !filename.isEmpty() && 
               timestamp.isValid() && 
               file_size > 0;
    }
    
    /**
     * @brief Get backup age in days
     */
    [[nodiscard]] int getAgeInDays() const {
        return timestamp.daysTo(QDateTime::currentDateTime());
    }
};

/**
 * @brief Application configuration structure
 */
struct ApplicationConfig {
    // Window settings
    QRect window_geometry{100, 100, 1200, 800};
    bool window_maximized{false};
    QByteArray window_state;
    
    // UI preferences
    QString theme{"default"};
    bool dark_mode{false};
    QString language{"en"};
    int update_interval_ms{500};
    bool auto_save_enabled{true};
    
    // Performance settings
    int max_buffer_size{1000};
    double target_fps{1000.0};
    bool enable_performance_monitoring{true};
    bool enable_gpu_acceleration{false};
    
    // Logging settings
    QString log_level{"INFO"};
    QString log_file_path;
    bool log_to_file{true};
    bool log_to_console{false};
    int max_log_files{10};
    
    // Recent files
    QStringList recent_files;
    int max_recent_files{10};
    
    /**
     * @brief Validate configuration
     */
    [[nodiscard]] ConfigValidationResult validate() const;
    
    /**
     * @brief Reset to default values
     */
    void resetToDefaults();
    
    /**
     * @brief Convert to JSON object
     */
    [[nodiscard]] QJsonObject toJson() const;
    
    /**
     * @brief Load from JSON object
     */
    bool fromJson(const QJsonObject& json);
};

/**
 * @brief Streaming configuration structure
 */
struct StreamingConfig {
    // Connection settings
    ui::ConnectionConfig connection;
    
    // Quality settings
    double min_signal_quality{80.0};
    double min_etsi_compliance{85.0};
    bool enable_quality_alerts{true};
    
    // Buffer settings
    uint32_t buffer_size{1000};
    bool adaptive_buffering{true};
    uint32_t max_buffer_overflows{100};
    
    // Monitoring settings
    bool enable_real_time_monitoring{true};
    int monitoring_update_interval{500};
    bool save_statistics{true};
    QString statistics_file_path;
    
    /**
     * @brief Validate streaming configuration
     */
    [[nodiscard]] ConfigValidationResult validate() const;
    
    /**
     * @brief Reset to default values
     */
    void resetToDefaults();
    
    /**
     * @brief Convert to JSON object
     */
    [[nodiscard]] QJsonObject toJson() const;
    
    /**
     * @brief Load from JSON object
     */
    bool fromJson(const QJsonObject& json);
};

/**
 * @brief Professional Configuration Manager
 * 
 * Comprehensive configuration management system providing .ini file
 * operations, import/export functionality, cross-platform support,
 * and validation for professional broadcast applications.
 */
class ConfigurationManager : public QObject {
    Q_OBJECT
    
public:
    explicit ConfigurationManager(QObject* parent = nullptr);
    ~ConfigurationManager() override;
    
    // Disable copy/move for proper resource management
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;
    ConfigurationManager(ConfigurationManager&&) = delete;
    ConfigurationManager& operator=(ConfigurationManager&&) = delete;
    
    /**
     * @brief Initialize configuration manager
     * @param organization Organization name for settings
     * @param application Application name for settings
     * @return true if initialization successful
     */
    bool initialize(const QString& organization = "StreamDAB Technologies",
                   const QString& application = "ETI Stream Analyser");
    
    /**
     * @brief Check if manager is initialized
     */
    [[nodiscard]] bool isInitialized() const noexcept {
        return initialized_;
    }
    
    // Configuration file operations
    /**
     * @brief Save configuration to file
     * @param filename Configuration file path
     * @param format File format (default: INI)
     * @return true if save successful
     */
    bool saveConfigurationFile(const QString& filename, ConfigFormat format = ConfigFormat::INI);
    
    /**
     * @brief Load configuration from file
     * @param filename Configuration file path
     * @return true if load successful
     */
    bool loadConfigurationFile(const QString& filename);
    
    /**
     * @brief Export configuration for sharing/backup
     * @param filename Export file path
     * @param format Export format
     * @param include_sensitive Include sensitive data (passwords, etc.)
     * @return true if export successful
     */
    bool exportConfiguration(const QString& filename, 
                           ConfigFormat format = ConfigFormat::JSON,
                           bool include_sensitive = false);
    
    /**
     * @brief Import configuration from file
     * @param filename Import file path
     * @param merge_with_existing Merge with existing config or replace
     * @return true if import successful
     */
    bool importConfiguration(const QString& filename, bool merge_with_existing = true);
    
    // Application settings management
    /**
     * @brief Save application settings
     */
    void saveApplicationSettings();
    
    /**
     * @brief Load application settings
     */
    void loadApplicationSettings();
    
    /**
     * @brief Get default configuration path
     * @param scope Configuration scope
     * @return Default configuration file path
     */
    [[nodiscard]] QString getDefaultConfigPath(ConfigScope scope = ConfigScope::USER) const;
    
    /**
     * @brief Get configuration directory
     * @param scope Configuration scope
     * @return Configuration directory path
     */
    [[nodiscard]] QString getConfigDirectory(ConfigScope scope = ConfigScope::USER) const;
    
    // Configuration access
    /**
     * @brief Get application configuration
     */
    [[nodiscard]] ApplicationConfig getApplicationConfig() const {
        return app_config_;
    }
    
    /**
     * @brief Set application configuration
     * @param config New application configuration
     */
    void setApplicationConfig(const ApplicationConfig& config);
    
    /**
     * @brief Get streaming configuration
     */
    [[nodiscard]] StreamingConfig getStreamingConfig() const {
        return streaming_config_;
    }
    
    /**
     * @brief Set streaming configuration
     * @param config New streaming configuration
     */
    void setStreamingConfig(const StreamingConfig& config);
    
    // Backup and restore
    /**
     * @brief Create configuration backup
     * @param description Backup description
     * @return true if backup created successfully
     */
    bool createBackup(const QString& description = QString());
    
    /**
     * @brief Restore configuration from backup
     * @param backup_filename Backup file to restore
     * @return true if restore successful
     */
    bool restoreFromBackup(const QString& backup_filename);
    
    /**
     * @brief Get available backups
     * @return List of available configuration backups
     */
    [[nodiscard]] QList<ConfigBackup> getAvailableBackups() const;
    
    /**
     * @brief Clean old backups
     * @param max_age_days Maximum age in days to keep
     * @param max_count Maximum number of backups to keep
     * @return Number of backups removed
     */
    int cleanOldBackups(int max_age_days = 30, int max_count = 10);
    
    // Validation and verification
    /**
     * @brief Validate current configuration
     * @return Validation result
     */
    [[nodiscard]] ConfigValidationResult validateConfiguration() const;
    
    /**
     * @brief Reset configuration to defaults
     * @param scope Scope to reset (all if not specified)
     */
    void resetToDefaults(ConfigScope scope = ConfigScope::APPLICATION);
    
    // Recent files management
    /**
     * @brief Add file to recent files list
     * @param filename File to add
     */
    void addRecentFile(const QString& filename);
    
    /**
     * @brief Remove file from recent files list
     * @param filename File to remove
     */
    void removeRecentFile(const QString& filename);
    
    /**
     * @brief Get recent files list
     * @return List of recent files
     */
    [[nodiscard]] QStringList getRecentFiles() const;
    
    /**
     * @brief Clear recent files list
     */
    void clearRecentFiles();
    
    // Configuration format detection
    /**
     * @brief Detect configuration file format
     * @param filename Configuration file path
     * @return Detected format
     */
    [[nodiscard]] static ConfigFormat detectFileFormat(const QString& filename);
    
    /**
     * @brief Check if configuration file is valid
     * @param filename Configuration file path
     * @return true if file is valid configuration
     */
    [[nodiscard]] static bool isValidConfigFile(const QString& filename);

public slots:
    /**
     * @brief Auto-save current configuration
     */
    void autoSave();
    
    /**
     * @brief Reload configuration from disk
     */
    void reloadConfiguration();
    
    /**
     * @brief Update setting value
     * @param key Setting key
     * @param value Setting value
     */
    void updateSetting(const QString& key, const QVariant& value);

signals:
    /**
     * @brief Emitted when configuration is saved
     * @param filename Saved configuration file
     */
    void configurationSaved(const QString& filename);
    
    /**
     * @brief Emitted when configuration is loaded
     * @param filename Loaded configuration file
     */
    void configurationLoaded(const QString& filename);
    
    /**
     * @brief Emitted when configuration export completes
     * @param filename Exported file
     * @param success Export success status
     */
    void configurationExported(const QString& filename, bool success);
    
    /**
     * @brief Emitted when configuration import completes
     * @param filename Imported file
     * @param success Import success status
     */
    void configurationImported(const QString& filename, bool success);
    
    /**
     * @brief Emitted when configuration error occurs
     * @param error Error description
     */
    void configurationError(const QString& error);
    
    /**
     * @brief Emitted when configuration changes
     */
    void configurationChanged();
    
    /**
     * @brief Emitted when backup is created
     * @param backup_info Backup information
     */
    void backupCreated(const ConfigBackup& backup_info);
    
    /**
     * @brief Emitted when backup is restored
     * @param backup_filename Restored backup file
     */
    void backupRestored(const QString& backup_filename);
    
    /**
     * @brief Emitted when recent files list changes
     * @param recent_files Updated recent files list
     */
    void recentFilesChanged(const QStringList& recent_files);

private:
    /**
     * @brief Setup default configuration directories
     */
    void setupConfigDirectories();
    
    /**
     * @brief Load configuration from QSettings
     * @param settings QSettings instance
     */
    void loadFromQSettings(QSettings& settings);
    
    /**
     * @brief Save configuration to QSettings
     * @param settings QSettings instance
     */
    void saveToQSettings(QSettings& settings);
    
    /**
     * @brief Load configuration from JSON
     * @param json_doc JSON document
     * @return true if load successful
     */
    bool loadFromJson(const QJsonDocument& json_doc);
    
    /**
     * @brief Save configuration to JSON
     * @param include_sensitive Include sensitive data
     * @return JSON document
     */
    QJsonDocument saveToJson(bool include_sensitive = false) const;
    
    /**
     * @brief Ensure configuration directory exists
     * @param path Directory path
     * @return true if directory exists or was created
     */
    bool ensureDirectoryExists(const QString& path) const;
    
    /**
     * @brief Generate backup filename
     * @param description Backup description
     * @return Generated backup filename
     */
    QString generateBackupFilename(const QString& description = QString()) const;
    
    /**
     * @brief Validate file format
     * @param filename File to validate
     * @param expected_format Expected format
     * @return true if format matches
     */
    bool validateFileFormat(const QString& filename, ConfigFormat expected_format) const;
    
    /**
     * @brief Migrate configuration from older versions
     * @param settings QSettings instance
     * @return true if migration successful
     */
    bool migrateConfiguration(QSettings& settings);
    
    /**
     * @brief Get configuration version
     * @return Current configuration version
     */
    QString getConfigVersion() const;

    // Core settings
    std::unique_ptr<QSettings> app_settings_;
    QString organization_;
    QString application_;
    bool initialized_{false};
    
    // Configuration data
    ApplicationConfig app_config_;
    StreamingConfig streaming_config_;
    
    // Auto-save timer
    QTimer* auto_save_timer_{nullptr};
    bool auto_save_enabled_{true};
    int auto_save_interval_{30000}; // 30 seconds
    
    // Configuration paths
    QString config_dir_;
    QString backup_dir_;
    QString user_config_path_;
    QString system_config_path_;
    
    // Configuration tracking
    bool config_dirty_{false};
    QDateTime last_save_time_;
    QDateTime last_load_time_;
    
    // Recent files
    QStringList recent_files_;
    int max_recent_files_{10};
    
    // Constants
    static constexpr int CONFIG_VERSION_MAJOR = 1;
    static constexpr int CONFIG_VERSION_MINOR = 0;
    static constexpr int MAX_BACKUP_AGE_DAYS = 30;
    static constexpr int MAX_BACKUP_COUNT = 10;
};

/**
 * @brief Configuration utility functions
 */
namespace config_utils {
    /**
     * @brief Get system-wide configuration directory
     */
    [[nodiscard]] QString getSystemConfigDirectory();
    
    /**
     * @brief Get user-specific configuration directory
     */
    [[nodiscard]] QString getUserConfigDirectory();
    
    /**
     * @brief Get temporary configuration directory
     */
    [[nodiscard]] QString getTempConfigDirectory();
    
    /**
     * @brief Convert ConfigFormat to file extension
     */
    [[nodiscard]] QString formatToExtension(ConfigFormat format);
    
    /**
     * @brief Convert file extension to ConfigFormat
     */
    [[nodiscard]] ConfigFormat extensionToFormat(const QString& extension);
    
    /**
     * @brief Get configuration format description
     */
    [[nodiscard]] QString getFormatDescription(ConfigFormat format);
    
    /**
     * @brief Validate configuration key
     */
    [[nodiscard]] bool isValidConfigKey(const QString& key);
    
    /**
     * @brief Sanitize configuration value
     */
    [[nodiscard]] QVariant sanitizeConfigValue(const QVariant& value);
    
    /**
     * @brief Create default application configuration
     */
    [[nodiscard]] ApplicationConfig createDefaultApplicationConfig();
    
    /**
     * @brief Create default streaming configuration
     */
    [[nodiscard]] StreamingConfig createDefaultStreamingConfig();
}

} // namespace streamdab::config