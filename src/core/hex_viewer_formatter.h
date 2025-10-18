#ifndef HEX_VIEWER_FORMATTER_H
#define HEX_VIEWER_FORMATTER_H

#include <QString>
#include <QByteArray>
#include <array>
#include <cstdint>

/**
 * @brief Professional hex viewer formatter for ETI frame data
 *
 * Provides efficient, color-coded hex dump visualization for 6144-byte ETI frames
 * with ETSI EN 300 799 structure highlighting and ASCII representation.
 *
 * Performance: Optimized for 6144-byte frames (384 lines)
 * - Pre-allocated string buffers
 * - Single-pass formatting
 * - Efficient HTML generation
 */
class HexViewerFormatter {
public:
    HexViewerFormatter();

    /**
     * @brief Format complete ETI frame as HTML hex dump
     * @param frame_data Raw 6144-byte ETI frame
     * @param frame_index Frame number for display (0-based)
     * @return HTML-formatted hex dump with color coding
     *
     * Performance: ~2-3ms per frame (optimized string building)
     *
     * CRITICAL-003 FIX: Input validation added for frame size and index
     * HIGH-003 FIX: FICF flag handling for accurate FIC boundary detection
     */
    QString formatFrameHex(const QByteArray& frame_data, int frame_index);

    /**
     * @brief Format specific byte range as hex dump
     * @param data Byte array to format
     * @param start_offset Starting byte offset for address display
     * @param title Section title (e.g., "FIC Data", "MSC")
     * @return HTML-formatted hex dump section
     *
     * Use case: Display only FIC or specific sub-channel data
     */
    QString formatSection(const QByteArray& data, size_t start_offset, const QString& title);

    /**
     * @brief Enable/disable color coding
     * @param enabled True to enable structure-based color highlighting
     */
    void setColorCoding(bool enabled) { m_color_coding_enabled = enabled; }

    /**
     * @brief Set bytes per line (default: 16)
     * @param bytes_per_line Number of bytes to display per line (8, 16, or 32)
     */
    void setBytesPerLine(int bytes_per_line);

    /**
     * @brief Enable/disable ASCII column
     * @param enabled True to show ASCII representation
     */
    void setAsciiColumn(bool enabled) { m_ascii_column_enabled = enabled; }

private:
    /**
     * @brief Determine CSS class based on byte offset in ETI frame
     * @param byte_offset Absolute byte position in frame
     * @param nst Number of streams (for dynamic MSC boundary calculation)
     * @param ficf_present FICF flag indicating FIC presence (HIGH-003 FIX)
     * @return CSS class name for color coding
     *
     * ETI Frame Structure (ETSI EN 300 799):
     * - Bytes 0-3:   SYNC pattern
     * - Bytes 4-7:   ERR, FC (frame counter), NST, etc.
     * - Bytes 8-11:  LIDATA fields
     * - Bytes 12+:   Stream headers (4 bytes * NST)
     * - After headers: FIC (if FICF=1, 96 bytes) - HIGH-003 FIX: Conditional on FICF
     * - After FIC:   MSC data
     * - Last 2 bytes: CRC
     */
    QString getCssClassForByte(size_t byte_offset, uint8_t nst, bool ficf_present) const;

    /**
     * @brief Generate CSS stylesheet for hex viewer
     * @return Complete CSS style block
     */
    QString generateStyleSheet() const;

    /**
     * @brief Format single line of hex dump
     * @param data Source data
     * @param line_offset Offset in data for this line
     * @param bytes_per_line Number of bytes per line
     * @param absolute_offset Absolute address for display
     * @param nst Number of streams for structure parsing (HIGH-003 FIX)
     * @param ficf_present FIC presence flag (HIGH-003 FIX)
     * @return HTML-formatted line with hex bytes and ASCII
     *
     * HIGH-001 FIX: HTML escaping added for ASCII column to prevent injection
     */
    QString formatHexLine(const QByteArray& data, size_t line_offset,
                          int bytes_per_line, size_t absolute_offset,
                          uint8_t nst, bool ficf_present) const;

    /**
     * @brief Generate structure legend footer
     * @return HTML legend explaining color coding
     */
    QString generateLegend() const;

    // Configuration
    bool m_color_coding_enabled;
    bool m_ascii_column_enabled;
    int m_bytes_per_line;

    // ETI Frame Structure Offsets (ETSI EN 300 799)
    static constexpr size_t SYNC_OFFSET = 0;
    static constexpr size_t SYNC_SIZE = 4;
    static constexpr size_t ERR_OFFSET = 4;
    static constexpr size_t FC_OFFSET = 5;      // Frame Counter
    static constexpr size_t NST_OFFSET = 6;     // Number of streams (and FICF)
    static constexpr size_t LIDATA_OFFSET = 8;
    static constexpr size_t LIDATA_SIZE = 4;
    static constexpr size_t STREAM_HEADER_SIZE = 4;
    static constexpr size_t FIC_SIZE = 96;      // When FICF=1
    static constexpr size_t FRAME_SIZE = 6144;
    static constexpr size_t CRC_SIZE = 2;

    // Pre-allocated buffer size for HTML generation
    // Estimate: 384 lines * ~120 chars/line = ~46KB
    static constexpr size_t ESTIMATED_HTML_SIZE = 50000;
};

#endif // HEX_VIEWER_FORMATTER_H
