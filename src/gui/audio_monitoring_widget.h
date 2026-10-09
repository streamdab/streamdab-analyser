#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QColor>
#include <QIcon>
#include <QFont>
#include <QDateTime>
#include <memory>

// Forward declarations
class EtiProcessor;  // Qt camelCase for GUI layer
class ServiceBrowser;

/**
 * @class AudioMonitoringWidget
 * @brief Professional DAB+ Audio Monitoring Widget for Broadcasting Industry
 * 
 * This widget provides comprehensive real-time audio monitoring capabilities
 * specifically designed for DAB+ services with ETSI 300 799 compliance:
 * 
 * Features:
 * - Real-time audio level meters per DAB+ service
 * - HE-AAC v2 codec information display
 * - Audio quality indicators based on signal strength and bitrate
 * - Service names with professional broadcast formatting
 * - Ensemble statistics with DAB+ compliance metrics
 * - Color-coded quality visualization
 * - Professional broadcast industry workflow integration
 * - Memory-efficient real-time updates (60 FPS capability)
 * - ETSI 300 799 compliance indicators
 */
class AudioMonitoringWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Audio quality enumeration based on broadcast standards
     */
    enum class AudioQuality {
        Unknown = 0,
        Poor = 1,       ///< Signal issues, potential dropouts
        Fair = 2,       ///< Acceptable quality with minor issues
        Good = 3,       ///< Professional broadcast quality
        Excellent = 4   ///< Optimal DAB+ quality
    };

    /**
     * @brief Audio codec enumeration for DAB+
     */
    enum class AudioCodec {
        Unknown = 0,
        DAB = 1,         ///< Legacy DAB MPEG-1 Layer II
        DABPlus = 2,     ///< DAB+ HE-AAC v2
        DABPlusSBR = 3,  ///< DAB+ HE-AAC v2 with SBR
        DABPlusPS = 4    ///< DAB+ HE-AAC v2 with PS
    };

    /**
     * @brief Service audio information structure
     */
    struct ServiceAudioInfo {
        quint32 serviceId;
        QString serviceName;
        QString ensembleName;
        AudioCodec audioCodec;
        quint32 bitRate;           // kbps
        AudioQuality quality;
        int audioLevel;            // 0-100%
        double signalStrength;     // dB
        double errorRate;          // 0.0-100.0%
        bool isActive;
        bool isDabPlus;
        QString codecDetails;
        QDateTime lastUpdate;
        
        ServiceAudioInfo() : serviceId(0), audioCodec(AudioCodec::Unknown),
                           bitRate(0), quality(AudioQuality::Unknown),
                           audioLevel(0), signalStrength(0.0), errorRate(0.0),
                           isActive(false), isDabPlus(false) {}
    };

    /**
     * @brief Ensemble statistics structure
     */
    struct EnsembleStatistics {
        QString ensembleName;
        quint32 ensembleId;
        int totalServices;
        int dabPlusServices;
        int activeServices;
        double totalBitrate;       // Mbps
        double etsiComplianceScore; // 0.0-100.0%
        QDateTime lastUpdate;
        
        EnsembleStatistics() : ensembleId(0), totalServices(0),
                             dabPlusServices(0), activeServices(0),
                             totalBitrate(0.0), etsiComplianceScore(0.0) {}
    };

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit AudioMonitoringWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~AudioMonitoringWidget();

    /**
     * @brief Initialize widget with ETI processor
     * @param processor ETI processor instance
     * @return true if successful
     */
    bool initialize(EtiProcessor *processor = nullptr);

    /**
     * @brief Set ETI processor for audio data
     * @param processor ETI processor instance
     */
    void setEtiProcessor(EtiProcessor *processor);

    /**
     * @brief Set service browser for integration
     * @param serviceBrowser Service browser instance
     */
    void setServiceBrowser(ServiceBrowser *serviceBrowser);

    /**
     * @brief Update audio levels for all services
     * @param levels Map of service ID to audio level (0-100)
     */
    void updateAudioLevels(const QMap<QString, int>& levels);

    /**
     * @brief Update service information
     * @param services List of service audio information
     */
    void updateServiceInfo(const QList<ServiceAudioInfo>& services);

    /**
     * @brief Update ensemble statistics
     * @param statistics Ensemble statistics
     */
    void updateEnsembleStatistics(const EnsembleStatistics& statistics);

    /**
     * @brief Update ETSI compliance metrics
     * @param complianceScore Overall compliance score (0.0-100.0)
     * @param detailedMetrics Detailed compliance metrics
     */
    void updateComplianceMetrics(double complianceScore, const QVariantMap& detailedMetrics);

    /**
     * @brief Get service audio information
     * @param serviceId Service ID
     * @return Service audio information
     */
    ServiceAudioInfo getServiceAudioInfo(quint32 serviceId) const;

    /**
     * @brief Get all monitored services
     * @return List of service IDs
     */
    QList<quint32> getMonitoredServices() const;

    /**
     * @brief Get current ensemble statistics
     * @return Ensemble statistics
     */
    EnsembleStatistics getEnsembleStatistics() const;

    /**
     * @brief Enable/disable real-time monitoring
     * @param enabled Update status
     */
    void setRealTimeEnabled(bool enabled);

    /**
     * @brief Check if real-time monitoring is enabled
     * @return true if enabled
     */
    bool isRealTimeEnabled() const { return m_realTimeEnabled; }

    /**
     * @brief Set update rate in FPS
     * @param fps Update rate (1-60 FPS)
     */
    void setUpdateRate(int fps);

    /**
     * @brief Get current update rate
     * @return Update rate in FPS
     */
    int getUpdateRate() const { return m_updateRate; }

    /**
     * @brief Clear all audio monitoring data
     */
    void clearAudioData();

    /**
     * @brief Get DAB+ service count
     * @return Number of DAB+ services
     */
    int getDabPlusServiceCount() const;

    /**
     * @brief Get overall audio quality score
     * @return Quality score (0.0-100.0)
     */
    double getOverallQualityScore() const;

public slots:
    /**
     * @brief Refresh audio monitoring display
     */
    void refreshAudioLevels();

    /**
     * @brief Select service for detailed monitoring
     * @param serviceId Service ID to select
     */
    void selectService(quint32 serviceId);

    /**
     * @brief Update compliance statistics
     */
    void updateComplianceStatistics();

    /**
     * @brief Expand all service groups
     */
    void expandAll();

    /**
     * @brief Collapse all service groups
     */
    void collapseAll();

signals:
    /**
     * @brief Emitted when service selection changes
     * @param serviceId Selected service ID
     */
    void serviceSelected(quint32 serviceId);

    /**
     * @brief Emitted when audio level changes significantly
     * @param serviceId Service ID
     * @param level New audio level (0-100)
     */
    void audioLevelChanged(quint32 serviceId, int level);

    /**
     * @brief Emitted when audio quality changes
     * @param serviceId Service ID
     * @param quality New audio quality
     */
    void audioQualityChanged(quint32 serviceId, AudioQuality quality);

    /**
     * @brief Emitted when DAB+ service detected
     * @param serviceId Service ID
     * @param codecInfo Codec information
     */
    void dabPlusServiceDetected(quint32 serviceId, const QString& codecInfo);

    /**
     * @brief Emitted when ETSI compliance changes
     * @param complianceScore New compliance score
     */
    void etsiComplianceChanged(double complianceScore);

    /**
     * @brief Emitted when audio monitoring error occurs
     * @param serviceId Service ID
     * @param errorMessage Error description
     */
    void audioMonitoringError(quint32 serviceId, const QString& errorMessage);

private slots:
    /**
     * @brief Handle real-time audio update timer
     */
    void handleAudioUpdate();

    /**
     * @brief Handle service selection from UI
     */
    void handleServiceSelectionChanged();

    /**
     * @brief Handle audio meter click
     * @param serviceId Service ID
     */
    void handleAudioMeterClicked(quint32 serviceId);

    /**
     * @brief Handle codec display refresh
     */
    void refreshCodecDisplay();

private:
    /**
     * @brief Create user interface
     */
    void createUI();

    /**
     * @brief Create header section with title and controls
     */
    void createHeaderSection();

    /**
     * @brief Create service monitoring section
     */
    void createServiceMonitoringSection();

    /**
     * @brief Create ensemble statistics section
     */
    void createEnsembleStatisticsSection();

    /**
     * @brief Create ETSI compliance section
     */
    void createComplianceSection();

    /**
     * @brief Setup audio level meters
     */
    void setupAudioLevelMeters();

    /**
     * @brief Create service display for specific service
     * @param serviceInfo Service audio information
     * @return Widget for service display
     */
    QWidget* createServiceDisplay(const ServiceAudioInfo& serviceInfo);

    /**
     * @brief Update service display widget
     * @param serviceId Service ID
     * @param serviceInfo Updated service information
     */
    void updateServiceDisplay(quint32 serviceId, const ServiceAudioInfo& serviceInfo);

    /**
     * @brief Add service to monitoring
     * @param serviceInfo Service audio information
     */
    void addServiceToMonitoring(const ServiceAudioInfo& serviceInfo);

    /**
     * @brief Remove service from monitoring
     * @param serviceId Service ID
     */
    void removeServiceFromMonitoring(quint32 serviceId);

    /**
     * @brief Get audio quality color
     * @param quality Audio quality
     * @return Color for display
     */
    QColor getAudioQualityColor(AudioQuality quality) const;

    /**
     * @brief Get codec icon
     * @param codec Audio codec
     * @return Icon for codec display
     */
    QIcon getCodecIcon(AudioCodec codec) const;

    /**
     * @brief Get codec description
     * @param codec Audio codec
     * @return Human-readable codec description
     */
    QString getCodecDescription(AudioCodec codec) const;

    /**
     * @brief Get quality description
     * @param quality Audio quality
     * @return Human-readable quality description
     */
    QString getQualityDescription(AudioQuality quality) const;

    /**
     * @brief Format bitrate display
     * @param bitrate Bitrate in kbps
     * @return Formatted bitrate string
     */
    QString formatBitrate(quint32 bitrate) const;

    /**
     * @brief Format compliance score
     * @param score Compliance score (0.0-100.0)
     * @return Formatted compliance string
     */
    QString formatComplianceScore(double score) const;

    /**
     * @brief Calculate audio quality from metrics
     * @param signalStrength Signal strength in dB
     * @param errorRate Error rate percentage
     * @param bitrate Bitrate in kbps
     * @return Calculated audio quality
     */
    AudioQuality calculateAudioQuality(double signalStrength, double errorRate, quint32 bitrate) const;

    /**
     * @brief Update statistics labels
     */
    void updateStatisticsLabels();

    /**
     * @brief Validate widget state
     * @return true if state is valid
     */
    bool validateState() const;

    /**
     * @brief Apply professional broadcast styling
     */
    void applyBroadcastStyling();

    /**
     * @brief Setup signal connections
     */
    void setupSignalConnections();

    // Core components
    EtiProcessor *m_etiProcessor;
    ServiceBrowser *m_serviceBrowser;

    // Main layout
    QVBoxLayout *m_mainLayout;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QVBoxLayout *m_scrollLayout;

    // Header section
    QGroupBox *m_headerGroup;
    QHBoxLayout *m_headerLayout;
    QLabel *m_titleLabel;
    QPushButton *m_refreshButton;
    QPushButton *m_expandAllButton;
    QPushButton *m_collapseAllButton;
    QComboBox *m_displayModeCombo;

    // Service monitoring section
    QGroupBox *m_serviceGroup;
    QVBoxLayout *m_serviceLayout;
    QWidget *m_serviceContainer;
    QVBoxLayout *m_serviceContainerLayout;

    // Individual service widgets
    QMap<quint32, QWidget*> m_serviceWidgets;
    QMap<quint32, QProgressBar*> m_audioMeters;
    QMap<quint32, QLabel*> m_serviceLabels;
    QMap<quint32, QLabel*> m_codecLabels;
    QMap<quint32, QLabel*> m_qualityLabels;
    QMap<quint32, QLabel*> m_bitrateLabels;

    // Ensemble statistics section
    QGroupBox *m_statisticsGroup;
    QGridLayout *m_statisticsLayout;
    QLabel *m_totalServicesLabel;
    QLabel *m_dabPlusServicesLabel;
    QLabel *m_activeServicesLabel;
    QLabel *m_totalBitrateLabel;

    // ETSI compliance section
    QGroupBox *m_complianceGroup;
    QVBoxLayout *m_complianceLayout;
    QProgressBar *m_complianceProgress;
    QLabel *m_complianceLabel;
    QLabel *m_complianceDetailsLabel;

    // Data storage
    QHash<quint32, ServiceAudioInfo> m_serviceAudioInfo;
    EnsembleStatistics m_ensembleStatistics;
    quint32 m_selectedServiceId;
    double m_overallComplianceScore;
    QVariantMap m_complianceMetrics;

    // Update system
    QTimer *m_updateTimer;
    bool m_realTimeEnabled;
    int m_updateRate;  // FPS
    mutable QMutex m_dataMutex;

    // Statistics
    int m_totalServices;
    int m_dabPlusServices;
    int m_activeServices;
    double m_averageQuality;

    // Professional styling
    QFont m_headerFont;
    QFont m_serviceFont;
    QFont m_statisticsFont;

    // Constants
    static constexpr int DEFAULT_UPDATE_RATE = 30;  // 30 FPS for audio monitoring
    static constexpr int MIN_UPDATE_RATE = 1;       // Minimum 1 FPS
    static constexpr int MAX_UPDATE_RATE = 60;      // Maximum 60 FPS
    static constexpr int AUDIO_METER_HEIGHT = 20;   // Audio meter height in pixels
    static constexpr int SERVICE_WIDGET_HEIGHT = 80; // Service widget height
    static constexpr double QUALITY_THRESHOLD_POOR = 30.0;     // Below 30% is poor
    static constexpr double QUALITY_THRESHOLD_FAIR = 60.0;     // 30-60% is fair
    static constexpr double QUALITY_THRESHOLD_GOOD = 85.0;     // 60-85% is good
    static constexpr double QUALITY_THRESHOLD_EXCELLENT = 95.0; // Above 85% is excellent
};

Q_DECLARE_METATYPE(AudioMonitoringWidget::AudioQuality)
Q_DECLARE_METATYPE(AudioMonitoringWidget::AudioCodec)
Q_DECLARE_METATYPE(AudioMonitoringWidget::ServiceAudioInfo)
Q_DECLARE_METATYPE(AudioMonitoringWidget::EnsembleStatistics)