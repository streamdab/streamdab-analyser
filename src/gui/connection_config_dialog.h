/**
 * @file connection_config_dialog.h
 * @brief Connection Configuration Dialog for ETI-over-IP Streaming
 * 
 * Professional dialog for configuring ZeroMQ ETI stream connections with
 * protocol selection, host/port configuration, URL preview, test connection,
 * and advanced settings for real-time streaming applications.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QTabWidget>
#include <QTimer>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QMovie>
#include <QStandardPaths>
#include <memory>

// Core components for connection testing
#include "../core/zeromq_eti_client.hpp"
#include "../core/modern_eti_controller.hpp"

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
QT_END_NAMESPACE

namespace streamdab::ui {

/**
 * @brief Connection configuration structure
 */
struct ConnectionConfig {
    // Basic connection settings
    QString protocol{"tcp"};              // "tcp", "zmq+tcp", "zmq+ipc"
    QString host{"127.0.0.1"};           // IP address or hostname
    quint16 port{9200};                  // Port number (1-65535)
    QString full_url;                    // Auto-generated complete URL
    
    // Advanced options
    bool enable_tist{true};              // Enable TIST timestamp processing
    bool enable_metadata{true};          // Enable metadata extraction
    uint32_t buffer_frames{1000};        // Buffer size in frames
    uint32_t timeout_ms{5000};           // Connection timeout
    bool auto_reconnect{true};           // Enable automatic reconnection
    uint32_t reconnect_interval_ms{5000}; // Reconnection interval
    
    // Quality settings
    double min_signal_quality{80.0};     // Minimum signal quality threshold
    bool enable_etsi_compliance{true};   // Enable ETSI compliance checking
    bool enable_error_correction{true};  // Enable Reed-Solomon error correction
    
    // Performance settings
    double target_fps{1000.0};           // Target processing FPS
    bool adaptive_buffering{true};       // Enable adaptive buffer management
    bool performance_monitoring{true};   // Enable performance monitoring
    
    /**
     * @brief Generate full URL from components
     */
    void updateFullURL() {
        if (protocol == "zmq+ipc") {
            full_url = QString("ipc:///tmp/eti_%1").arg(port);
        } else if (protocol == "zmq+tcp" || protocol == "tcp") {
            full_url = QString("tcp://%1:%2").arg(host).arg(port);
        } else {
            full_url = QString("%1://%2:%3").arg(protocol, host).arg(port);
        }
    }
    
    /**
     * @brief Validate configuration
     */
    [[nodiscard]] bool isValid() const {
        return !protocol.isEmpty() && 
               !host.isEmpty() && 
               port > 0 && port <= 65535 &&
               timeout_ms > 0 && timeout_ms <= 60000 &&
               buffer_frames > 0 && buffer_frames <= 10000 &&
               target_fps > 0.0 && target_fps <= 10000.0 &&
               min_signal_quality >= 0.0 && min_signal_quality <= 100.0;
    }
    
    /**
     * @brief Convert to processing config
     */
    [[nodiscard]] eti::controller::ProcessingConfig toProcessingConfig() const {
        eti::controller::ProcessingConfig config;
        
        // ZeroMQ configuration
        config.zmq_config.endpoint = full_url;
        config.zmq_config.timeout_ms = timeout_ms;
        config.auto_reconnect = auto_reconnect;
        config.reconnect_interval_ms = reconnect_interval_ms;
        
        // Performance settings
        config.target_fps = target_fps;
        config.max_frame_buffer_size = static_cast<int>(buffer_frames);
        config.enable_performance_monitoring = performance_monitoring;
        
        // Quality settings
        config.min_signal_quality = min_signal_quality;
        config.enable_etsi_compliance_checking = enable_etsi_compliance;
        config.enable_error_recovery = enable_error_correction;
        
        // Frame processing
        config.enable_eti_ni_processing = enable_tist;
        config.enable_reed_solomon_correction = enable_error_correction;
        
        return config;
    }
};

/**
 * @brief Connection test result
 */
struct ConnectionTestResult {
    bool success{false};
    QString message;
    QString details;
    double latency_ms{0.0};
    double signal_quality{0.0};
    uint32_t frames_received{0};
    std::chrono::milliseconds test_duration{0};
    
    [[nodiscard]] bool isGoodConnection() const {
        return success && 
               latency_ms < 100.0 && 
               signal_quality > 70.0 && 
               frames_received > 5;
    }
};

/**
 * @brief Professional Connection Configuration Dialog
 * 
 * Provides comprehensive configuration interface for ETI-over-IP streaming
 * connections with protocol selection, connection testing, and advanced
 * settings for professional broadcast monitoring applications.
 */
class ConnectionConfigDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit ConnectionConfigDialog(QWidget* parent = nullptr);
    ~ConnectionConfigDialog() override;
    
    // Disable copy/move for proper resource management
    ConnectionConfigDialog(const ConnectionConfigDialog&) = delete;
    ConnectionConfigDialog& operator=(const ConnectionConfigDialog&) = delete;
    ConnectionConfigDialog(ConnectionConfigDialog&&) = delete;
    ConnectionConfigDialog& operator=(ConnectionConfigDialog&&) = delete;
    
    /**
     * @brief Set initial configuration
     * @param config Initial connection configuration
     */
    void setConfiguration(const ConnectionConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current connection configuration
     */
    [[nodiscard]] ConnectionConfig getConfiguration() const;
    
    /**
     * @brief Check if configuration is valid
     */
    [[nodiscard]] bool isConfigurationValid() const;
    
    /**
     * @brief Get last connection test result
     */
    [[nodiscard]] ConnectionTestResult getLastTestResult() const {
        return last_test_result_;
    }

public slots:
    /**
     * @brief Test current connection configuration
     */
    void testConnection();
    
    /**
     * @brief Save configuration to file
     */
    void saveConfiguration();
    
    /**
     * @brief Load configuration from file
     */
    void loadConfiguration();
    
    /**
     * @brief Reset to default configuration
     */
    void resetToDefaults();
    
    /**
     * @brief Import configuration from file
     */
    void importConfiguration();
    
    /**
     * @brief Export configuration to file
     */
    void exportConfiguration();

signals:
    /**
     * @brief Emitted when configuration is accepted
     * @param config Accepted configuration
     */
    void configurationAccepted(const ConnectionConfig& config);
    
    /**
     * @brief Emitted when configuration changes
     * @param config Current configuration
     */
    void configurationChanged(const ConnectionConfig& config);
    
    /**
     * @brief Emitted when connection test completes
     * @param result Test result
     */
    void connectionTestCompleted(const ConnectionTestResult& result);
    
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

protected:
    /**
     * @brief Handle accept button click
     */
    void accept() override;
    
    /**
     * @brief Handle reject button click
     */
    void reject() override;

private slots:
    /**
     * @brief Handle protocol selection change
     */
    void onProtocolChanged(const QString& protocol);
    
    /**
     * @brief Handle host input change
     */
    void onHostChanged(const QString& host);
    
    /**
     * @brief Handle port input change
     */
    void onPortChanged(int port);
    
    /**
     * @brief Handle advanced settings change
     */
    void onAdvancedSettingsChanged();
    
    /**
     * @brief Handle connection test progress
     */
    void onConnectionTestProgress(int progress);
    
    /**
     * @brief Handle connection test completion
     */
    void onConnectionTestFinished();
    
    /**
     * @brief Handle connection test timeout
     */
    void onConnectionTestTimeout();
    
    /**
     * @brief Update URL preview
     */
    void updateURLPreview();
    
    /**
     * @brief Validate current configuration
     */
    void validateConfiguration();

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();
    
    /**
     * @brief Create basic connection tab
     */
    QWidget* createBasicConnectionTab();
    
    /**
     * @brief Create advanced settings tab
     */
    QWidget* createAdvancedSettingsTab();
    
    /**
     * @brief Create connection test tab
     */
    QWidget* createConnectionTestTab();
    
    /**
     * @brief Setup signal connections
     */
    void connectSignals();
    
    /**
     * @brief Apply professional styling
     */
    void applyProfessionalStyling();
    
    /**
     * @brief Load default settings
     */
    void loadDefaultSettings();
    
    /**
     * @brief Save current settings
     */
    void saveCurrentSettings();
    
    /**
     * @brief Get configuration file path
     * @param for_export true if for export, false for app config
     */
    QString getConfigFilePath(bool for_export = false) const;
    
    /**
     * @brief Start connection test with current configuration
     */
    void startConnectionTest();
    
    /**
     * @brief Stop active connection test
     */
    void stopConnectionTest();
    
    /**
     * @brief Update test progress display
     */
    void updateTestProgress(const QString& message, int progress);
    
    /**
     * @brief Create protocol-specific help text
     */
    QString getProtocolHelpText(const QString& protocol) const;
    
    /**
     * @brief Validate host input
     */
    bool validateHost(const QString& host) const;
    
    /**
     * @brief Validate port input
     */
    bool validatePort(int port) const;

    // Main layout
    QVBoxLayout* m_mainLayout{nullptr};
    QTabWidget* m_tabWidget{nullptr};
    
    // Basic Connection Tab
    QWidget* m_basicTab{nullptr};
    QFormLayout* m_basicLayout{nullptr};
    QComboBox* m_protocolCombo{nullptr};
    QLineEdit* m_hostEdit{nullptr};
    QSpinBox* m_portSpin{nullptr};
    QLabel* m_urlPreviewLabel{nullptr};
    QPushButton* m_testConnectionBtn{nullptr};
    QLabel* m_connectionStatusLabel{nullptr};
    
    // Advanced Settings Tab
    QWidget* m_advancedTab{nullptr};
    QVBoxLayout* m_advancedLayout{nullptr};
    
    // Processing Options Group
    QGroupBox* m_processingGroup{nullptr};
    QCheckBox* m_enableTISTCheck{nullptr};
    QCheckBox* m_enableMetadataCheck{nullptr};
    QCheckBox* m_enableETSIComplianceCheck{nullptr};
    QCheckBox* m_enableErrorCorrectionCheck{nullptr};
    
    // Buffer Settings Group
    QGroupBox* m_bufferGroup{nullptr};
    QSpinBox* m_bufferFramesSpin{nullptr};
    QSpinBox* m_timeoutSpin{nullptr};
    QCheckBox* m_autoReconnectCheck{nullptr};
    QSpinBox* m_reconnectIntervalSpin{nullptr};
    QCheckBox* m_adaptiveBufferingCheck{nullptr};
    
    // Quality Settings Group
    QGroupBox* m_qualityGroup{nullptr};
    QSpinBox* m_minSignalQualitySpin{nullptr};
    QSpinBox* m_targetFPSSpin{nullptr};
    QCheckBox* m_performanceMonitoringCheck{nullptr};
    
    // Connection Test Tab
    QWidget* m_testTab{nullptr};
    QVBoxLayout* m_testLayout{nullptr};
    QGroupBox* m_testControlGroup{nullptr};
    QPushButton* m_startTestBtn{nullptr};
    QPushButton* m_stopTestBtn{nullptr};
    QProgressBar* m_testProgressBar{nullptr};
    QLabel* m_testStatusLabel{nullptr};
    
    // Test Results Group
    QGroupBox* m_testResultsGroup{nullptr};
    QTextEdit* m_testResultsText{nullptr};
    QLabel* m_latencyLabel{nullptr};
    QLabel* m_signalQualityLabel{nullptr};
    QLabel* m_framesReceivedLabel{nullptr};
    
    // Button box
    QHBoxLayout* m_buttonLayout{nullptr};
    QPushButton* m_saveBtn{nullptr};
    QPushButton* m_loadBtn{nullptr};
    QPushButton* m_importBtn{nullptr};
    QPushButton* m_exportBtn{nullptr};
    QPushButton* m_resetBtn{nullptr};
    QPushButton* m_okBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    
    // Connection testing
    std::unique_ptr<eti::controller::ModernETIController> m_testController{nullptr};
    QTimer* m_testTimer{nullptr};
    QTimer* m_testTimeoutTimer{nullptr};
    bool m_testInProgress{false};
    ConnectionTestResult last_test_result_;
    std::chrono::system_clock::time_point test_start_time_;
    
    // Configuration management
    ConnectionConfig current_config_;
    QSettings* m_settings{nullptr};
    
    // UI state
    bool m_configurationValid{false};
    
    // Constants
    static constexpr int TEST_DURATION_MS = 10000;  // 10 second test
    static constexpr int TEST_TIMEOUT_MS = 15000;   // 15 second timeout
    static constexpr int MIN_TEST_FRAMES = 5;       // Minimum frames for success
    static constexpr double MAX_ACCEPTABLE_LATENCY = 100.0; // Maximum latency in ms
};

/**
 * @brief Utility functions for connection configuration
 */
namespace connection_utils {
    /**
     * @brief Create default connection configuration
     */
    [[nodiscard]] ConnectionConfig createDefaultConfig();
    
    /**
     * @brief Create ODR-DabMux default configuration
     */
    [[nodiscard]] ConnectionConfig createODRDabMuxConfig();
    
    /**
     * @brief Create high-performance configuration
     */
    [[nodiscard]] ConnectionConfig createHighPerformanceConfig();
    
    /**
     * @brief Validate connection configuration
     * @param config Configuration to validate
     * @return Validation result with error messages
     */
    struct ValidationResult {
        bool valid{false};
        std::vector<QString> errors;
        std::vector<QString> warnings;
    };
    
    [[nodiscard]] ValidationResult validateConfig(const ConnectionConfig& config);
    
    /**
     * @brief Get protocol display names
     */
    [[nodiscard]] QStringList getSupportedProtocols();
    
    /**
     * @brief Get protocol description
     */
    [[nodiscard]] QString getProtocolDescription(const QString& protocol);
    
    /**
     * @brief Get default port for protocol
     */
    [[nodiscard]] quint16 getDefaultPortForProtocol(const QString& protocol);
}

} // namespace streamdab::ui