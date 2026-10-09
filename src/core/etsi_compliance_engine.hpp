#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <vector>
#include <memory>
#include <array>
#include <cstdint>

namespace etsi {

// ETSI EN 300 799 Validation Levels
enum class ComplianceLevel {
    CRITICAL,    // Must pass for broadcast compliance
    WARNING,     // Should be addressed for optimal performance
    INFO,        // Informational notices
    PASS         // Validation successful
};

// ETSI Validation Error Types
enum class ValidationErrorType {
    ETI_SYNC_ERROR,          // ETI sync bytes incorrect
    FRAME_COUNTER_ERROR,     // FC sequence error
    FIC_CRC_ERROR,           // FIC CRC validation failed
    MSC_CRC_ERROR,           // MSC CRC validation failed
    FRAME_SIZE_ERROR,        // Invalid frame size
    FIG_TYPE_ERROR,          // Invalid FIG type
    SERVICE_ID_ERROR,        // Invalid Service ID
    ENSEMBLE_ID_ERROR,       // Invalid Ensemble ID
    TIMESTAMP_ERROR,         // TIST timestamp error
    PROTECTION_LEVEL_ERROR   // Invalid protection level
};

// Validation Result Structure
struct ValidationResult {
    ValidationErrorType error_type;
    ComplianceLevel level;
    QString description;
    QString etsi_reference;      // ETSI standard reference
    uint32_t frame_number;
    uint16_t byte_offset;
    QDateTime timestamp;
    QString suggested_fix;
    
    ValidationResult(ValidationErrorType type, ComplianceLevel lvl, 
                    const QString& desc, const QString& ref,
                    uint32_t frame = 0, uint16_t offset = 0)
        : error_type(type), level(lvl), description(desc), 
          etsi_reference(ref), frame_number(frame), byte_offset(offset),
          timestamp(QDateTime::currentDateTime()) {}
};

// ETSI Compliance Report
struct ComplianceReport {
    QString stream_name;
    QDateTime analysis_time;
    uint32_t total_frames_analyzed;
    uint32_t critical_errors;
    uint32_t warnings;
    uint32_t info_notices;
    uint32_t passed_validations;
    double compliance_score;     // 0.0-100.0%
    std::vector<ValidationResult> results;
    
    ComplianceReport() : total_frames_analyzed(0), critical_errors(0), 
                        warnings(0), info_notices(0), passed_validations(0),
                        compliance_score(0.0) {}
};

// ETI Frame Structure for Validation
struct ETIFrame {
    std::array<uint8_t, 6144> raw_data;
    uint32_t frame_number;
    
    // ETI Header fields
    uint32_t sync_bytes;         // Bytes 0-3
    uint32_t frame_counter;      // Bytes 4-6 (24-bit)
    uint8_t transmission_mode;   // TM field
    uint8_t frame_phase;         // FP field
    
    // FIC section
    std::vector<uint8_t> fic_data;
    uint16_t fic_crc;
    
    // MSC section  
    std::vector<uint8_t> msc_data;
    uint16_t msc_crc;
    
    ETIFrame() : frame_number(0), sync_bytes(0), frame_counter(0),
                transmission_mode(0), frame_phase(0), fic_crc(0), msc_crc(0) {}
};

// ETSI Compliance Engine Class
class ETSIComplianceEngine : public QObject {
    Q_OBJECT
    
public:
    explicit ETSIComplianceEngine(QObject* parent = nullptr);
    virtual ~ETSIComplianceEngine() = default;
    
    // Main validation methods
    ValidationResult validateETIFrame(const ETIFrame& frame);
    ComplianceReport analyzeETIStream(const std::vector<ETIFrame>& frames);
    
    // Individual validation methods
    ValidationResult validateETISync(const ETIFrame& frame);
    ValidationResult validateFrameCounter(const ETIFrame& frame, uint32_t expected_fc);
    ValidationResult validateFICSection(const ETIFrame& frame);
    ValidationResult validateMSCSection(const ETIFrame& frame);
    ValidationResult validateFIGTypes(const ETIFrame& frame);
    ValidationResult validateServiceIDs(const ETIFrame& frame);
    
    // CRC validation
    bool validateFICCRC(const std::vector<uint8_t>& fic_data, uint16_t crc);
    bool validateMSCCRC(const std::vector<uint8_t>& msc_data, uint16_t crc);
    uint16_t calculateCRC16(const std::vector<uint8_t>& data);
    
    // Configuration
    void setStrictMode(bool strict);
    void setValidationLevel(ComplianceLevel min_level);
    
    // Report generation
    QString generateTextReport(const ComplianceReport& report);
    QString generateHTMLReport(const ComplianceReport& report);
    bool saveReportToFile(const ComplianceReport& report, const QString& filename);
    
signals:
    void validationComplete(const ValidationResult& result);
    void complianceReportReady(const ComplianceReport& report);
    void criticalErrorDetected(const ValidationResult& result);
    
private:
    bool m_strict_mode;
    ComplianceLevel m_min_validation_level;
    uint32_t m_last_frame_counter;
    
    // Internal validation helpers
    bool isValidETISync(uint32_t sync_bytes);
    bool isValidTransmissionMode(uint8_t tm);
    bool isValidFramePhase(uint8_t fp);
    bool isValidFIGType(uint8_t fig_type);
    
    // ETSI standard references
    QString getETSIReference(ValidationErrorType error_type);
    QString getSuggestedFix(ValidationErrorType error_type);
    
    // Statistics tracking
    void updateComplianceStatistics(ComplianceReport& report, const ValidationResult& result);
    double calculateComplianceScore(const ComplianceReport& report);
};

} // namespace etsi