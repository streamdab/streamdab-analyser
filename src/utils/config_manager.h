#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QSettings>
#include <memory>

/**
 * @class ConfigManager
 * @brief Configuration management for ETI Stream Analyser
 * 
 * Handles application settings, user preferences, and configuration
 * persistence using Qt's QSettings framework.
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit ConfigManager(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~ConfigManager();

    /**
     * @brief Initialize configuration manager
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Get configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Configuration value
     */
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /**
     * @brief Set configuration value
     * @param key Configuration key
     * @param value Value to set
     */
    void setValue(const QString& key, const QVariant& value);

    /**
     * @brief Check if key exists
     * @param key Configuration key
     * @return true if key exists
     */
    bool contains(const QString& key) const;

    /**
     * @brief Remove configuration key
     * @param key Key to remove
     */
    void remove(const QString& key);

    /**
     * @brief Clear all configuration
     */
    void clear();

    /**
     * @brief Sync configuration to disk
     */
    void sync();

    /**
     * @brief Check if configuration is initialized
     * @return true if ready
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Get settings file path
     * @return Path to settings file
     */
    QString getSettingsPath() const;

    // Common configuration keys
    static const QString WINDOW_GEOMETRY;
    static const QString WINDOW_STATE;
    static const QString LAST_FILE_PATH;
    static const QString UPDATE_RATE;
    static const QString REAL_TIME_MODE;
    static const QString AUDIO_MONITORING;
    static const QString ERROR_LOGGING;

signals:
    /**
     * @brief Emitted when configuration changes
     * @param key Changed key
     * @param value New value
     */
    void configurationChanged(const QString& key, const QVariant& value);

    /**
     * @brief Emitted when configuration is saved
     */
    void configurationSaved();

private:
    std::unique_ptr<QSettings> m_settings;
    bool m_initialized;
    QString m_configPath;

    /**
     * @brief Setup default configuration values
     */
    void setupDefaults();
};