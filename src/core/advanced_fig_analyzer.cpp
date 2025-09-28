#include "advanced_fig_analyzer.h"
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <algorithm>

AdvancedFIGAnalyzer::AdvancedFIGAnalyzer(QObject *parent)
    : QObject(parent)
    , m_totalFIGsProcessed(0)
    , m_validFIGsCount(0)
    , m_ensembleValid(false)
    , m_ensembleId(0)
    , m_changeFlags(0)
    , m_timeValid(false)
{
    initializeDefaultFIGTypes();
}

void AdvancedFIGAnalyzer::initializeDefaultFIGTypes()
{
    // Enable essential FIG types by default for DAB-XPlorer compatibility
    m_enabledFIGTypes["0.0"] = true;   // Ensemble organization
    m_enabledFIGTypes["0.1"] = true;   // Subchannel organization  
    m_enabledFIGTypes["0.2"] = true;   // Service organization
    m_enabledFIGTypes["0.3"] = false;  // Service component (optional)
    m_enabledFIGTypes["0.8"] = false;  // Service component global (optional)
    m_enabledFIGTypes["0.13"] = false; // User application (optional)
    m_enabledFIGTypes["1.0"] = true;   // Programme service labels
    m_enabledFIGTypes["1.1"] = false;  // Programme service data (optional)
    m_enabledFIGTypes["1.4"] = false;  // Service component labels (optional)
    m_enabledFIGTypes["1.5"] = false;  // Data service labels (optional)
}

void AdvancedFIGAnalyzer::analyzeFICData(const QByteArray& ficData)
{
    if (ficData.size() < 4) {
        m_lastError = "FIC data too short";
        return;
    }
    
    m_currentFICData = ficData;
    int offset = 0;
    
    // Process all FIGs in the FIC data block
    while (offset < ficData.size() - 1) {
        uint8_t figType, figLength;
        
        if (!validateFIGHeader(ficData, figType, figLength)) {
            m_lastError = QString("Invalid FIG header at offset %1").arg(offset);
            break;
        }
        
        if (offset + figLength > ficData.size()) {
            m_lastError = QString("FIG length exceeds data size at offset %1").arg(offset);
            break;
        }
        
        // Extract FIG data
        QByteArray figData = ficData.mid(offset + 2, figLength);
        
        // Process based on FIG type
        switch (figType) {
            case 0:
                processFIG0(figData);
                break;
            case 1:
                processFIG1(figData);
                break;
            default:
                qDebug() << "Unsupported FIG type:" << figType;
                break;
        }
        
        m_totalFIGsProcessed++;
        offset += figLength + 2; // +2 for header
    }
}

void AdvancedFIGAnalyzer::processFIG0(const QByteArray& figData)
{
    if (figData.size() < 1) return;
    
    uint8_t extension = (figData[0] >> 3) & 0x1F;
    
    QString figKey = QString("0.%1").arg(extension);
    if (!m_enabledFIGTypes.value(figKey, false)) {
        return; // FIG type disabled
    }
    
    // Process based on extension
    switch (extension) {
        case 0:
            processFIG0_0(figData);
            break;
        case 1:
            processFIG0_1(figData);
            break;
        case 2:
            processFIG0_2(figData);
            break;
        case 3:
            processFIG0_3(figData);
            break;
        case 8:
            processFIG0_8(figData);
            break;
        case 13:
            processFIG0_13(figData);
            break;
        default:
            qDebug() << "Unsupported FIG 0 extension:" << extension;
            break;
    }
}

void AdvancedFIGAnalyzer::processFIG1(const QByteArray& figData)
{
    if (figData.size() < 1) return;
    
    uint8_t extension = (figData[0] >> 3) & 0x1F;
    
    QString figKey = QString("1.%1").arg(extension);
    if (!m_enabledFIGTypes.value(figKey, false)) {
        return; // FIG type disabled
    }
    
    // Process based on extension
    switch (extension) {
        case 0:
            processFIG1_0(figData);
            break;
        case 1:
            processFIG1_1(figData);
            break;
        case 4:
            processFIG1_4(figData);
            break;
        case 5:
            processFIG1_5(figData);
            break;
        default:
            qDebug() << "Unsupported FIG 1 extension:" << extension;
            break;
    }
}

void AdvancedFIGAnalyzer::processFIG0_0(const QByteArray& data)
{
    // FIG 0/0: Ensemble organization
    if (data.size() < 6) return;
    
    // Extract ensemble information
    uint16_t ensembleId = extractUInt16(data, 2);
    uint8_t changeFlags = data[4];
    uint8_t alarmFlag = data[5];
    
    // Update ensemble information
    m_currentEnsemble.ensembleId = ensembleId;
    m_ensembleId = ensembleId;
    m_changeFlags = changeFlags;
    m_ensembleValid = true;
    
    // Extract country code and ECC if available
    if (data.size() >= 8) {
        uint8_t countryCode = data[6];
        m_currentEnsemble.countryCode = FIGParsingUtils::countryCodeToString(countryCode);
    }
    
    // Create FIG data for signal emission
    FIGData figData;
    figData.figType = 0;
    figData.figExtension = 0;
    figData.rawData = data;
    figData.isValid = true;
    figData.description = QString("Ensemble Organization - EID: 0x%1")
        .arg(ensembleId, 4, 16, QChar('0')).toUpper();
    figData.decodedContent = QString("Ensemble ID: 0x%1\nChange Flags: 0x%2\nAlarm: %3\nCountry: %4")
        .arg(ensembleId, 4, 16, QChar('0'))
        .arg(changeFlags, 2, 16, QChar('0'))
        .arg(alarmFlag ? "Yes" : "No")
        .arg(m_currentEnsemble.countryCode);
    
    emit figProcessed(figData);
    emit ensembleUpdated(m_currentEnsemble);
    
    m_validFIGsCount++;
}

void AdvancedFIGAnalyzer::processFIG0_1(const QByteArray& data)
{
    // FIG 0/1: Subchannel organization
    if (data.size() < 4) return;
    
    int offset = 1; // Skip header byte
    
    while (offset + 3 < data.size()) {
        uint8_t subchannelId = data[offset] & 0x3F;
        uint16_t startAddress = extractUInt16(data, offset + 1) & 0x3FF;
        
        // Extract protection and size information
        bool shortForm = (data[offset + 2] & 0x80) != 0;
        uint8_t protectionLevel;
        uint16_t subchannelSize;
        
        if (shortForm) {
            protectionLevel = (data[offset + 2] >> 2) & 0x03;
            subchannelSize = ((data[offset + 2] & 0x03) << 8) | data[offset + 3];
            offset += 4;
        } else {
            protectionLevel = (data[offset + 3] >> 2) & 0x03;
            subchannelSize = ((data[offset + 3] & 0x03) << 8) | data[offset + 4];
            offset += 5;
        }
        
        // Create FIG data for signal emission
        FIGData figData;
        figData.figType = 0;
        figData.figExtension = 1;
        figData.rawData = data;
        figData.isValid = true;
        figData.description = QString("Subchannel Organization - SubCh: %1").arg(subchannelId);
        figData.decodedContent = QString("Subchannel ID: %1\nStart Address: %2\nProtection: %3\nSize: %4")
            .arg(subchannelId)
            .arg(startAddress)
            .arg(FIGParsingUtils::protectionLevelToString(protectionLevel))
            .arg(subchannelSize);
        
        emit figProcessed(figData);
    }
    
    m_validFIGsCount++;
}

void AdvancedFIGAnalyzer::processFIG0_2(const QByteArray& data)
{
    // FIG 0/2: Service organization
    if (data.size() < 5) return;
    
    int offset = 1; // Skip header byte
    
    while (offset + 4 < data.size()) {
        uint32_t serviceId = extractUInt32(data, offset);
        uint8_t serviceType = data[offset + 3] & 0x1F;
        
        // Create or update service information
        ServiceInfo service;
        service.serviceId = serviceId;
        service.ensembleId = m_ensembleId;
        service.type = FIGParsingUtils::serviceTypeToString(serviceType);
        service.isAudio = FIGParsingUtils::isAudioService(serviceType);
        service.isData = FIGParsingUtils::isDataService(serviceType);
        
        // Add to discovered services if not already present
        bool found = false;
        for (auto& existingService : m_discoveredServices) {
            if (existingService.serviceId == serviceId) {
                // Update existing service
                existingService.type = service.type;
                existingService.isAudio = service.isAudio;
                existingService.isData = service.isData;
                found = true;
                break;
            }
        }
        
        if (!found) {
            m_discoveredServices.append(service);
            emit serviceDiscovered(service);
        }
        
        // Create FIG data for signal emission
        FIGData figData;
        figData.figType = 0;
        figData.figExtension = 2;
        figData.rawData = data;
        figData.isValid = true;
        figData.description = QString("Service Organization - SID: 0x%1")
            .arg(serviceId, 8, 16, QChar('0')).toUpper();
        figData.decodedContent = QString("Service ID: 0x%1\nType: %2\nAudio: %3\nData: %4")
            .arg(serviceId, 8, 16, QChar('0'))
            .arg(service.type)
            .arg(service.isAudio ? "Yes" : "No")
            .arg(service.isData ? "Yes" : "No");
        
        emit figProcessed(figData);
        
        offset += 5; // Move to next service
    }
    
    m_validFIGsCount++;
}

void AdvancedFIGAnalyzer::processFIG1_0(const QByteArray& data)
{
    // FIG 1/0: Programme service labels
    if (data.size() < 19) return; // Minimum size for label
    
    int offset = 1; // Skip header byte
    
    while (offset + 18 < data.size()) {
        uint32_t serviceId = extractUInt32(data, offset);
        QString label = FIGParsingUtils::convertDABCharset(data, offset + 4, 16);
        
        // Update service label
        m_serviceLabels[serviceId] = label;
        
        // Update service in discovered services list
        for (auto& service : m_discoveredServices) {
            if (service.serviceId == serviceId) {
                service.label = label;
                emit serviceLabelUpdated(serviceId, label);
                break;
            }
        }
        
        // Create FIG data for signal emission
        FIGData figData;
        figData.figType = 1;
        figData.figExtension = 0;
        figData.rawData = data;
        figData.isValid = true;
        figData.description = QString("Programme Service Label - SID: 0x%1")
            .arg(serviceId, 8, 16, QChar('0')).toUpper();
        figData.decodedContent = QString("Service ID: 0x%1\nLabel: \"%2\"")
            .arg(serviceId, 8, 16, QChar('0'))
            .arg(label);
        
        emit figProcessed(figData);
        
        offset += 20; // Move to next label (4 + 16 bytes)
    }
    
    m_validFIGsCount++;
}

// Placeholder implementations for other FIG processors
void AdvancedFIGAnalyzer::processFIG0_3(const QByteArray& data)
{
    Q_UNUSED(data)
    // FIG 0/3: Service component - to be implemented in future phases
}

void AdvancedFIGAnalyzer::processFIG0_8(const QByteArray& data)
{
    Q_UNUSED(data)
    // FIG 0/8: Service component global definition - to be implemented
}

void AdvancedFIGAnalyzer::processFIG0_13(const QByteArray& data)
{
    Q_UNUSED(data)
    // FIG 0/13: User application information - to be implemented
}

void AdvancedFIGAnalyzer::processFIG1_1(const QByteArray& data)
{
    Q_UNUSED(data)
    // FIG 1/1: Programme service data - to be implemented
}

void AdvancedFIGAnalyzer::processFIG1_4(const QByteArray& data)
{
    Q_UNUSED(data)
    // FIG 1/4: Service component labels - to be implemented
}

void AdvancedFIGAnalyzer::processFIG1_5(const QByteArray& data)
{
    Q_UNUSED(data)
    // FIG 1/5: Data service labels - to be implemented
}

// Control methods
void AdvancedFIGAnalyzer::enableFIGType(uint8_t type, uint8_t extension, bool enabled)
{
    QString figKey = QString("%1.%2").arg(type).arg(extension);
    m_enabledFIGTypes[figKey] = enabled;
}

bool AdvancedFIGAnalyzer::isFIGTypeEnabled(uint8_t type, uint8_t extension) const
{
    QString figKey = QString("%1.%2").arg(type).arg(extension);
    return m_enabledFIGTypes.value(figKey, false);
}

void AdvancedFIGAnalyzer::reset()
{
    m_currentEnsemble = EnsembleInfo();
    m_discoveredServices.clear();
    m_serviceLabels.clear();
    m_componentLabels.clear();
    m_programmeTypes.clear();
    m_totalFIGsProcessed = 0;
    m_validFIGsCount = 0;
    m_lastError.clear();
    m_ensembleValid = false;
    m_timeValid = false;
}

// Data accessors
QStringList AdvancedFIGAnalyzer::getServiceLabels() const
{
    QStringList labels;
    for (auto it = m_serviceLabels.begin(); it != m_serviceLabels.end(); ++it) {
        labels.append(QString("SID 0x%1: %2")
                     .arg(it.key(), 8, 16, QChar('0'))
                     .arg(it.value()));
    }
    return labels;
}

QStringList AdvancedFIGAnalyzer::getProgrammeInfo() const
{
    QStringList info;
    
    // Add time information if available
    if (m_timeValid) {
        info.append(QString("Time: %1").arg(FIGParsingUtils::formatDABTime(m_currentDABTime)));
    }
    
    // Add programme type information
    for (auto it = m_programmeTypes.begin(); it != m_programmeTypes.end(); ++it) {
        info.append(QString("SID 0x%1: %2")
                   .arg(it.key(), 8, 16, QChar('0'))
                   .arg(FIGParsingUtils::programmeTypeToString(it.value())));
    }
    
    return info;
}

// Helper methods
bool AdvancedFIGAnalyzer::validateFIGHeader(const QByteArray& data, uint8_t& type, uint8_t& length)
{
    if (data.size() < 2) return false;
    
    type = (data[0] >> 5) & 0x07;
    length = data[1];
    
    return (length > 0 && length <= 30); // Valid FIG length range
}

uint16_t AdvancedFIGAnalyzer::extractUInt16(const QByteArray& data, int offset)
{
    if (offset + 1 >= data.size()) return 0;
    return (static_cast<uint16_t>(data[offset]) << 8) | 
           static_cast<uint16_t>(data[offset + 1]);
}

uint32_t AdvancedFIGAnalyzer::extractUInt32(const QByteArray& data, int offset)
{
    if (offset + 3 >= data.size()) return 0;
    return (static_cast<uint32_t>(data[offset]) << 24) |
           (static_cast<uint32_t>(data[offset + 1]) << 16) |
           (static_cast<uint32_t>(data[offset + 2]) << 8) |
           static_cast<uint32_t>(data[offset + 3]);
}

// FIG Parsing Utilities Implementation
QString FIGParsingUtils::convertDABCharset(const QByteArray& data, int offset, int length)
{
    if (offset + length > data.size()) return QString();
    
    // Simple ASCII conversion for now - can be enhanced for full EBU character set
    QString result;
    for (int i = 0; i < length; ++i) {
        char c = data[offset + i];
        if (c >= 0x20 && c <= 0x7E) { // Printable ASCII
            result.append(c);
        } else if (c == 0x00) {
            break; // Null terminator
        }
    }
    
    return result.trimmed();
}

QString FIGParsingUtils::countryCodeToString(uint8_t countryCode)
{
    // ECC country codes - partial implementation
    switch (countryCode) {
        case 0xE1: return "TH"; // Thailand
        case 0xE0: return "DE"; // Germany
        case 0xE2: return "UK"; // United Kingdom
        case 0xE3: return "FR"; // France
        default: return QString("0x%1").arg(countryCode, 2, 16, QChar('0'));
    }
}

QString FIGParsingUtils::serviceTypeToString(uint8_t serviceType)
{
    switch (serviceType) {
        case 0x00: return "DAB Audio";
        case 0x01: return "DAB+ Audio";
        case 0x02: return "Data Service";
        case 0x03: return "Packet Data";
        case 0x18: return "DAB+ Enhanced";
        default: return QString("Type %1").arg(serviceType);
    }
}

bool FIGParsingUtils::isAudioService(uint8_t serviceType)
{
    return (serviceType == 0x00 || serviceType == 0x01 || serviceType == 0x18);
}

bool FIGParsingUtils::isDataService(uint8_t serviceType)
{
    return (serviceType == 0x02 || serviceType == 0x03);
}

QString FIGParsingUtils::protectionLevelToString(uint8_t protectionLevel)
{
    switch (protectionLevel) {
        case 0: return "UEP-1";
        case 1: return "UEP-2";
        case 2: return "UEP-3";
        case 3: return "UEP-4";
        default: return QString("UEP-%1").arg(protectionLevel);
    }
}

QString FIGParsingUtils::programmeTypeToString(uint8_t ptyCode)
{
    // International programme type codes
    static const char* ptyNames[] = {
        "No programme type", "News", "Current Affairs", "Information",
        "Sport", "Education", "Drama", "Culture", "Science", "Varied",
        "Pop Music", "Rock Music", "Easy Listening", "Light Classical",
        "Serious Classical", "Other Music", "Weather/Meteorology", "Finance/Business",
        "Children's programmes", "Social Affairs", "Religion", "Phone In",
        "Travel", "Leisure", "Jazz Music", "Country Music", "National Music",
        "Oldies Music", "Folk Music", "Documentary", "Alarm Test", "Alarm"
    };
    
    if (ptyCode < 32) {
        return QString(ptyNames[ptyCode]);
    } else {
        return QString("PTy %1").arg(ptyCode);
    }
}

QDateTime FIGParsingUtils::convertDABTime(uint32_t dabTime)
{
    // DAB time conversion - simplified implementation
    // Real implementation would handle Modified Julian Date properly
    return QDateTime::currentDateTime();
}

QString FIGParsingUtils::formatDABTime(const QDateTime& time)
{
    return time.toString("hh:mm:ss UTC");
}