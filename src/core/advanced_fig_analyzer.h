#ifndef ADVANCED_FIG_ANALYZER_H
#define ADVANCED_FIG_ANALYZER_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QDateTime>
#include <cstdint>
#include "eti_types.h"

/**
 * @brief Advanced FIG (Fast Information Group) Analyzer
 * 
 * Phase 1.4 implementation providing comprehensive FIG analysis
 * for DAB-XPlorer FIC-XPlorer and FIC-XTractor functionality.
 * 
 * Supports complete ETSI EN 300 799 FIG processing:
 * - FIG 0/0: Ensemble Organization
 * - FIG 0/1: Subchannel Organization  
 * - FIG 0/2: Service Organization
 * - FIG 1/0: Programme Service Labels
 * - FIG 1/1: Programme Service Data
 * - FIG 1/4: Service Component Labels
 */
class AdvancedFIGAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit AdvancedFIGAnalyzer(QObject *parent = nullptr);
    
    // Main analysis methods
    void analyzeFICData(const QByteArray& ficData);
    void reset();
    
    // FIG processing control
    void enableFIGType(uint8_t type, uint8_t extension, bool enabled);
    bool isFIGTypeEnabled(uint8_t type, uint8_t extension) const;
    
    // Data accessors
    EnsembleInfo getCurrentEnsemble() const { return m_currentEnsemble; }
    ServiceInfoList getDiscoveredServices() const { return m_discoveredServices; }
    QStringList getServiceLabels() const;
    QStringList getProgrammeInfo() const;
    
    // Analysis results
    int getTotalFIGsProcessed() const { return m_totalFIGsProcessed; }
    int getValidFIGsCount() const { return m_validFIGsCount; }
    QString getLastError() const { return m_lastError; }

signals:
    // Service discovery signals
    void serviceDiscovered(const ServiceInfo& service);
    void ensembleUpdated(const EnsembleInfo& ensemble);
    void serviceLabelUpdated(uint32_t serviceId, const QString& label);
    
    // FIG analysis signals
    void figProcessed(const FIGData& figData);
    void figError(uint8_t figType, uint8_t extension, const QString& error);
    
    // Programme information signals
    void programmeTypeUpdated(uint32_t serviceId, uint8_t ptyCode);
    void timeInfoUpdated(const QDateTime& currentTime);

private:
    // FIG type processors
    void processFIG0(const QByteArray& figData);
    void processFIG1(const QByteArray& figData);
    
    // FIG 0 extension processors  
    void processFIG0_0(const QByteArray& data); // Ensemble organization
    void processFIG0_1(const QByteArray& data); // Subchannel organization
    void processFIG0_2(const QByteArray& data); // Service organization
    void processFIG0_3(const QByteArray& data); // Service component
    void processFIG0_8(const QByteArray& data); // Service component global definition
    void processFIG0_13(const QByteArray& data); // User application information
    
    // FIG 1 extension processors
    void processFIG1_0(const QByteArray& data); // Programme service labels
    void processFIG1_1(const QByteArray& data); // Programme service data
    void processFIG1_4(const QByteArray& data); // Service component labels
    void processFIG1_5(const QByteArray& data); // Data service labels
    
    // Helper methods
    bool validateFIGHeader(const QByteArray& data, uint8_t& type, uint8_t& length);
    QString extractDABLabel(const QByteArray& data, int offset, int length);
    uint16_t extractUInt16(const QByteArray& data, int offset);
    uint32_t extractUInt32(const QByteArray& data, int offset);
    
    // Service management
    ServiceInfo* findOrCreateService(uint32_t serviceId);
    void updateServiceInformation(uint32_t serviceId, const ServiceInfo& updates);
    
    // ETSI compliance checking
    bool validateETSICompliance(uint8_t figType, uint8_t extension, const QByteArray& data);
    void reportComplianceError(const QString& standard, const QString& error);

private:
    // Current analysis state
    EnsembleInfo m_currentEnsemble;
    ServiceInfoList m_discoveredServices;
    QMap<uint32_t, QString> m_serviceLabels;
    QMap<uint32_t, QString> m_componentLabels;
    QMap<uint32_t, uint8_t> m_programmeTypes;
    
    // FIG processing control
    QMap<QString, bool> m_enabledFIGTypes; // "type.extension" -> enabled
    
    // Statistics
    int m_totalFIGsProcessed;
    int m_validFIGsCount;
    QString m_lastError;
    
    // Processing state
    QByteArray m_currentFICData;
    bool m_ensembleValid;
    uint16_t m_ensembleId;
    uint8_t m_changeFlags;
    
    // Time information
    QDateTime m_currentDABTime;
    bool m_timeValid;
    
    // Default enabled FIG types
    void initializeDefaultFIGTypes();
};

/**
 * @brief FIG parsing utilities
 * 
 * Static utility functions for FIG data parsing
 * and ETSI standard compliance validation
 */
class FIGParsingUtils
{
public:
    // Character set conversion for DAB labels
    static QString convertDABCharset(const QByteArray& data, int offset, int length);
    
    // Country and language code utilities
    static QString countryCodeToString(uint8_t countryCode);
    static QString languageCodeToString(uint8_t languageCode);
    
    // Service type utilities
    static QString serviceTypeToString(uint8_t serviceType);
    static bool isAudioService(uint8_t serviceType);
    static bool isDataService(uint8_t serviceType);
    
    // Protection level utilities
    static QString protectionLevelToString(uint8_t protectionLevel);
    static int protectionLevelToBitrate(uint8_t protectionLevel);
    
    // Programme type utilities
    static QString programmeTypeToString(uint8_t ptyCode);
    static QString programmeTypeToDescription(uint8_t ptyCode);
    
    // Time and date utilities
    static QDateTime convertDABTime(uint32_t dabTime);
    static QString formatDABTime(const QDateTime& time);
    
    // CRC validation
    static bool validateFIGCRC(const QByteArray& figData);
    static uint16_t calculateFIGCRC(const QByteArray& data);
};

#endif // ADVANCED_FIG_ANALYZER_H