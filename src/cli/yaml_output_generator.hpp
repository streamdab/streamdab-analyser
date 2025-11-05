/**
 * @file yaml_output_generator.hpp
 * @brief ETISnoop-Compatible YAML Output Generator for StreamDAB Analyzer
 *
 * Generates YAML formatted output compatible with ETISnoop for comprehensive
 * DAB/DAB+ stream analysis results. Supports Thai UTF-8 encoding and follows
 * ETSI EN 300 401 naming conventions.
 *
 * @author Agent 35 - YAML Output Generator
 * @date 2025-11-05
 * @copyright StreamDAB Analyzer Project - PDCA Week 8
 */

#ifndef YAML_OUTPUT_GENERATOR_HPP
#define YAML_OUTPUT_GENERATOR_HPP

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QList>
#include <QFile>
#include <QTextStream>
#include "../core/eti_types.hpp"
#include "../core/fig_parser.hpp"

namespace cli {

/**
 * @brief YAML Output Generator for StreamDAB Analyzer
 *
 * Generates ETISnoop-compatible YAML output containing:
 * - ETI stream metadata (format, frames, errors, timing)
 * - Ensemble information (ID, label, country, flags)
 * - Service listings (ID, label, type, components)
 * - Sub-channel organization (address, size, protection)
 * - FIG statistics (counts by type)
 * - ETSI compliance status
 * - Performance metrics
 *
 * Output format follows YAML 1.2 specification with:
 * - 2-space indentation
 * - Proper string escaping
 * - ISO 8601 timestamps
 * - Hexadecimal 0xNNNN notation
 * - Thai UTF-8 preservation
 */
class YAMLOutputGenerator {
public:
    /**
     * @brief Construct YAML output generator
     */
    YAMLOutputGenerator();

    /**
     * @brief Destructor
     */
    ~YAMLOutputGenerator() = default;

    // ========================================================================
    // Data Setters
    // ========================================================================

    /**
     * @brief Set input file name
     * @param filename Path to input ETI file
     */
    void setInputFile(const QString& filename);

    /**
     * @brief Set total processing time
     * @param ms Processing duration in milliseconds
     */
    void setProcessingTime(qint64 ms);

    /**
     * @brief Set ensemble information
     * @param ensemble Ensemble structure from ETI processing
     */
    void setEnsembleInfo(const eti::Ensemble& ensemble);

    /**
     * @brief Add service to output
     * @param service DAB service structure
     */
    void addService(const eti::DabService& service);

    /**
     * @brief Add sub-channel to output
     * @param subchannel Sub-channel information structure
     */
    void addSubchannel(const eti::fig::Fig01SubchannelInfo& subchannel);

    /**
     * @brief Set FIG type statistics
     * @param fig_counts Map of FIG type names to counts (e.g., "0/0" -> 4200)
     */
    void setFIGStatistics(const QMap<QString, int>& fig_counts);

    /**
     * @brief Set ETSI compliance status
     * @param compliant Overall compliance status
     * @param warnings List of compliance warning messages
     */
    void setETSICompliance(bool compliant, const QStringList& warnings);

    /**
     * @brief Set performance metrics
     * @param processing_time Total processing time in milliseconds
     * @param fps Frames processed per second
     */
    void setPerformanceMetrics(qint64 processing_time, double fps);

    /**
     * @brief Set ETI stream information
     * @param total_frames Total frames in stream
     * @param valid_frames Number of valid frames
     * @param error_frames Number of error frames
     * @param crc_errors Number of CRC errors detected
     */
    void setETIStreamInfo(int total_frames, int valid_frames, int error_frames, int crc_errors);

    // ========================================================================
    // Output Generation
    // ========================================================================

    /**
     * @brief Generate complete YAML output
     * @return YAML formatted string
     */
    QString generateYAML() const;

    /**
     * @brief Write YAML output to file
     * @param filename Output file path
     * @return true if write successful
     */
    bool writeToFile(const QString& filename);

    /**
     * @brief Write YAML output to stdout
     * @return true if write successful
     */
    bool writeToStdout();

private:
    // ========================================================================
    // Internal Data Structures
    // ========================================================================

    /**
     * @brief Complete YAML output data
     */
    struct YAMLData {
        QString version{"1.0"};
        QString generator{"StreamDAB Analyzer v1.1"};
        QDateTime timestamp;
        QString input_file;
        qint64 analysis_duration_ms{0};

        struct ETIStream {
            QString format{"ETI-LI"};
            int frame_size{6144};
            int total_frames{0};
            int valid_frames{0};
            int error_frames{0};
            double frame_rate{250.0};
            int crc_errors{0};
            double duration_seconds{0.0};
        } eti_stream;

        eti::Ensemble ensemble;
        QList<eti::DabService> services;
        QList<eti::fig::Fig01SubchannelInfo> subchannels;
        QMap<QString, int> fig_counts;

        bool etsi_compliant{true};
        QStringList etsi_warnings;

        qint64 processing_time_ms{0};
        double frames_per_second{0.0};
    } m_data;

    // ========================================================================
    // YAML Formatting Methods
    // ========================================================================

    /**
     * @brief Format ETI stream section
     * @return YAML formatted ETI stream info
     */
    QString formatETIStream() const;

    /**
     * @brief Format ensemble section
     * @return YAML formatted ensemble info
     */
    QString formatEnsemble() const;

    /**
     * @brief Format services section
     * @return YAML formatted service list
     */
    QString formatServices() const;

    /**
     * @brief Format sub-channels section
     * @return YAML formatted sub-channel list
     */
    QString formatSubchannels() const;

    /**
     * @brief Format FIG statistics section
     * @return YAML formatted FIG statistics
     */
    QString formatFIGStatistics() const;

    /**
     * @brief Format ETSI compliance section
     * @return YAML formatted compliance status
     */
    QString formatETSICompliance() const;

    /**
     * @brief Format performance section
     * @return YAML formatted performance metrics
     */
    QString formatPerformance() const;

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Escape YAML special characters in string
     * @param str Input string (may contain Thai UTF-8)
     * @return Escaped string safe for YAML
     */
    QString escapeYAMLString(const QString& str) const;

    /**
     * @brief Format 16-bit value as hexadecimal
     * @param value Value to format
     * @return Hexadecimal string (e.g., "0x1234")
     */
    QString formatHex(quint16 value) const;

    /**
     * @brief Format 32-bit value as hexadecimal
     * @param value Value to format
     * @return Hexadecimal string (e.g., "0x12345678")
     */
    QString formatHex(quint32 value) const;

    /**
     * @brief Format protection level description
     * @param subchannel Sub-channel info
     * @return Protection level string (e.g., "UEP-3", "EEP-2A")
     */
    QString formatProtectionLevel(const eti::fig::Fig01SubchannelInfo& subchannel) const;

    /**
     * @brief Get service type description
     * @param service DAB service
     * @return Service type string (e.g., "DAB+ Audio", "Data Service")
     */
    QString getServiceType(const eti::DabService& service) const;

    /**
     * @brief Write YAML to text stream
     * @param stream Output text stream
     * @param yaml YAML content
     * @return true if write successful
     */
    bool writeToStream(QTextStream& stream, const QString& yaml) const;
};

} // namespace cli

#endif // YAML_OUTPUT_GENERATOR_HPP
