#pragma once

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QHash>
#include <QMap>
#include <QString>
#include <QDateTime>
#include "eti_types.hpp"

// Forward declarations for ETI types

/**
 * @class fic_decoder
 * @brief Professional FIC (Fast Information Channel) decoder following ETSI EN 300 401
 * 
 * Decodes FIC data from ETI frames to extract service information, ensemble data,
 * and service component details according to ETSI specifications.
 * 
 * Features:
 * - ETSI EN 300 401 compliant FIC decoding
 * - FIG (Fast Information Group) parsing for all standard types
 * - Service and ensemble information extraction
 * - Real-time FIC processing with error detection
 * - Service label decoding (including extended character sets)
 * - Programme Type (PTy) classification
 */
class fic_decoder : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief FIG (Fast Information Group) types supported
     */
    enum class FigType {
        Unknown = -1,
        BasicServiceInfo = 0,      // FIG 0/1 - Basic service and service component definition
        ServiceLabel = 1,          // FIG 1/1 - Programme service label
        ServiceComponent = 2,      // FIG 0/2 - Basic service component definition
        PacketData = 3,           // FIG 0/3 - Service component with packet mode
        StreamData = 4,           // FIG 0/4 - Service component with stream mode
        LanguageInfo = 5,         // FIG 0/5 - Service component language
        ServiceLinking = 6,       // FIG 0/6 - Service linking information
        ConfigInfo = 7            // FIG 0/7 - Configuration information
    };
    Q_ENUM(FigType)

    /**
     * @brief Decoder processing status
     */
    enum class DecoderStatus {
        Idle = 0,
        Processing = 1,
        Error = 2,
        Ready = 3,
        Synchronized = 4
    };
    Q_ENUM(DecoderStatus)

    /**
     * @brief Service information extracted from FIC
     */
    struct ServiceInfo {
        quint32 serviceId;
        QString serviceLabel;
        QString shortLabel;
        quint8 programmeType;
        quint8 languageCode;
        bool isLocal;
        bool isDataService;
        QDateTime lastUpdated;
        
        ServiceInfo() : serviceId(0), programmeType(0), languageCode(0), 
                       isLocal(false), isDataService(false) {}
    };

    /**
     * @brief Ensemble information from FIC
     */
    struct EnsembleInfo {
        quint16 ensembleId;
        QString ensembleLabel;
        QString shortLabel;
        quint8 countryId;
        quint8 extendedCountryCode;
        quint8 cifCount;
        QString label;  // Test-compatible alias for ensembleLabel
        QDateTime lastUpdated;
        
        EnsembleInfo() : ensembleId(0), countryId(0), extendedCountryCode(0), cifCount(0) {}
    };

    /**
     * @brief Service component information
     */
    struct ComponentInfo {
        quint32 serviceId;
        quint8 subChannelId;
        quint16 startAddress;
        quint16 length;
        quint8 protectionLevel;
        bool isProtected;
        QString label;
        
        ComponentInfo() : serviceId(0), subChannelId(0), startAddress(0), 
                         length(0), protectionLevel(0), isProtected(false) {}
    };

    /**
     * @brief Service information structure (test-compatible)
     */
    struct Service {
        quint32 serviceId;
        QString label;
        quint8 nbServiceComp;
        QList<ComponentInfo> components;
        
        Service() : serviceId(0), nbServiceComp(0) {}
    };

    /**
     * @brief Sub-channel information structure (test-compatible)
     */
    struct SubChannel {
        quint8 subChannelId;
        quint16 startAddress;
        quint16 subChannelSize;
        quint8 protectionLevel;
        bool isValid;
        
        SubChannel() : subChannelId(0), startAddress(0), subChannelSize(0), 
                      protectionLevel(0), isValid(false) {}
    };

    /**
     * @brief Ensemble information structure (test-compatible)
     */
    struct EnsembleData {
        quint16 ensembleId;
        QString label;
        quint8 cifCount;
        
        EnsembleData() : ensembleId(0), cifCount(0) {}
    };

    explicit fic_decoder(QObject *parent = nullptr);
    virtual ~fic_decoder();

    // Core decoding operations
    bool initialize();
    void shutdown();
    bool is_initialized() const { return initialized_; }

    // FIC processing
    bool process_fic_data(const QByteArray& ficData);
    bool process_eti_frame(const eti::EtiFrameData& frame);
    bool process_fig_block(const eti::FigBlock& figBlock);
    
    // CamelCase versions for existing implementation compatibility
    bool processFicData(const QByteArray& ficData);
    bool processEtiFrame(const eti::EtiFrameData& frame);
    bool processFigBlock(const eti::FigBlock& figBlock);

    // Information retrieval
    QList<ServiceInfo> get_discovered_services() const;
    ServiceInfo get_service_info(quint32 serviceId) const;
    
    // CamelCase versions for existing implementation compatibility
    QList<ServiceInfo> getDiscoveredServices() const;
    ServiceInfo getServiceInfo(quint32 serviceId) const;
    EnsembleInfo get_ensemble_info() const;
    QList<ComponentInfo> get_service_components(quint32 serviceId) const;
    
    // CamelCase versions for existing implementation compatibility
    EnsembleInfo getEnsembleInfo() const;
    QList<ComponentInfo> getServiceComponents(quint32 serviceId) const;

    // Service management
    bool has_service(quint32 serviceId) const;
    bool has_ensemble_info() const;
    QString get_service_label(quint32 serviceId) const;
    QString get_ensemble_label() const;

    // Status and statistics
    DecoderStatus get_status() const { return status_; }
    quint32 get_processed_fic_count() const { return fic_count_; }
    double get_error_rate() const { return error_rate_; }
    quint32 get_fig_type_count(FigType type) const;
    
    // Test-compatible interface methods
    quint32 get_fib_count() const { return fic_count_; }
    quint32 get_error_count() const { return error_count_; }
    int get_sync_level() const { return sync_level_; }
    void reset();
    QList<Service> get_services() const;
    QList<SubChannel> get_sub_channels() const;
    EnsembleData get_ensemble_data() const;
    Service get_service_by_id(quint32 serviceId) const;
    SubChannel get_sub_channel_by_id(quint8 subChannelId) const;
    bool is_service_available(quint32 serviceId) const;
    bool is_sub_channel_configured(quint8 subChannelId) const;

    // Configuration
    void set_error_threshold(double threshold) { error_threshold_ = threshold; }
    double get_error_threshold() const { return error_threshold_; }

signals:
    /**
     * @brief Emitted when new services are discovered
     */
    void servicesUpdated(const QList<ServiceInfo>& services);

    /**
     * @brief Emitted when ensemble information is updated
     */
    void ensembleInfoUpdated(const EnsembleInfo& ensemble);

    /**
     * @brief Emitted when service components are discovered
     */
    void serviceComponentsUpdated(quint32 serviceId, const QList<ComponentInfo>& components);

    /**
     * @brief Emitted when decoder status changes
     */
    void statusChanged(DecoderStatus status);

    /**
     * @brief Emitted on FIC decoding errors
     */
    void ficError(const QString& error);

    /**
     * @brief Emitted when service label is updated
     */
    void serviceLabelUpdated(quint32 serviceId, const QString& label);

    /**
     * @brief Emitted when FIC data is processed
     */
    void ficDataProcessed();

    /**
     * @brief Emitted when service information is updated
     */
    void serviceInfoUpdated(quint32 serviceId);

    /**
     * @brief Emitted when sub-channel information is updated
     */
    void subchannelInfoUpdated(quint8 subChannelId);

private slots:
    void handle_fic_processed();
    void handle_fic_error(const QString& error);

private:
    // FIG processing methods
    bool process_fig0(const QByteArray& figData);
    bool process_fig1(const QByteArray& figData);
    bool process_fig0_type1(const QByteArray& data); // Service info
    bool process_fig0_type2(const QByteArray& data); // Service components
    bool process_fig1_type1(const QByteArray& data); // Service labels
    
    // CamelCase versions for existing implementation compatibility
    bool processFig0(const QByteArray& figData);
    bool processFig1(const QByteArray& figData);

    // Data extraction helpers
    QString extract_label(const QByteArray& data, int offset, int length) const;
    QString extract_short_label(const QString& fullLabel) const;
    bool validate_fig_header(const QByteArray& data) const;
    quint8 calculate_fig_length(const QByteArray& data) const;
    
    // CamelCase versions for existing implementation compatibility
    bool validateFigHeader(const QByteArray& data) const;
    quint8 calculateFigLength(const QByteArray& data) const;

    // Service management
    void add_or_update_service(const ServiceInfo& service);
    void add_or_update_component(const ComponentInfo& component);
    void update_ensemble_info(const EnsembleInfo& ensemble);

    // Error handling and statistics
    void update_error_statistics(bool hasError);
    void reset_decoder();
    
    // CamelCase versions for existing implementation compatibility
    void updateErrorStatistics(bool hasError);
    void resetDecoder();

    // Member variables
    bool initialized_;
    DecoderStatus status_;
    quint32 fic_count_;
    double error_rate_;
    double error_threshold_;
    quint32 error_count_;
    int sync_level_;

    // Information storage
    QMap<quint32, ServiceInfo> services_;
    QMap<quint32, QList<ComponentInfo>> service_components_;
    EnsembleInfo ensemble_info_;
    bool has_ensemble_info_;
    
    // Test-compatible data structures
    QMap<quint32, Service> test_services_;
    QMap<quint8, SubChannel> sub_channels_;
    EnsembleData ensemble_data_;

    // Statistics and monitoring
    QHash<FigType, quint32> fig_type_stats_;
    QDateTime last_process_time_;

    // Processing state
    QByteArray fic_buffer_;
    bool processing_active_;

    // Constants
    static constexpr double DEFAULT_ERROR_THRESHOLD = 0.1; // 10%
    static constexpr quint32 MAX_FIC_BUFFER_SIZE = 1024;
    static constexpr quint8 FIG_HEADER_SIZE = 1;
    static constexpr quint8 MAX_SERVICES_PER_ENSEMBLE = 64;
};