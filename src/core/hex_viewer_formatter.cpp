#include "hex_viewer_formatter.h"
#include <QStringBuilder>
#include <QDebug>
#include <cstring>

HexViewerFormatter::HexViewerFormatter()
    : m_color_coding_enabled(true)
    , m_ascii_column_enabled(true)
    , m_bytes_per_line(16)
{
}

void HexViewerFormatter::setBytesPerLine(int bytes_per_line)
{
    // Validate and set (only allow 8, 16, or 32)
    if (bytes_per_line == 8 || bytes_per_line == 16 || bytes_per_line == 32) {
        m_bytes_per_line = bytes_per_line;
    } else {
        m_bytes_per_line = 16; // Default
    }
}

QString HexViewerFormatter::formatFrameHex(const QByteArray& frame_data, int frame_index)
{
    // ========================================================================
    // CRITICAL-003 FIX: Input validation for frame size
    // ========================================================================
    if (frame_data.size() != 6144) {
        qWarning() << "HexViewerFormatter::formatFrameHex: Invalid frame size"
                   << frame_data.size() << "(expected 6144)";

        return QString("<html><body style='background:#1e1e1e;color:#d4d4d4;'>"
                      "<div style='color:#ff6b6b; padding:20px;'>"
                      "<b>Error: Invalid ETI Frame Size</b><br>"
                      "Expected: 6144 bytes<br>"
                      "Received: %1 bytes<br><br>"
                      "Please ensure the file is a valid ETI stream."
                      "</div></body></html>").arg(frame_data.size());
    }

    // ========================================================================
    // CRITICAL-003 FIX: Input validation for frame index
    // ========================================================================
    if (frame_index < 0) {
        qWarning() << "HexViewerFormatter::formatFrameHex: Invalid frame index" << frame_index;
        frame_index = 0;  // Fallback to frame 0
    }

    // Performance optimization: pre-allocate string buffer
    QString html;
    html.reserve(ESTIMATED_HTML_SIZE);

    // Build HTML with efficient string concatenation
    html += "<html><head>";
    html += generateStyleSheet();
    html += "</head><body>";

    // Header section
    html += QString("<div class='header'>"
                   "ETI Frame #%1 - Hex Viewer (%2 bytes)"
                   "</div><br>")
                .arg(frame_index + 1)
                .arg(frame_data.size());

    // ========================================================================
    // HIGH-003 FIX: Extract both NST and FICF flag for accurate structure parsing
    // ========================================================================
    uint8_t nst = 0;
    bool ficf_present = false;

    if (frame_data.size() >= NST_OFFSET + 1) {
        // NST is in byte 6, bits 5-0 (lower 6 bits)
        nst = static_cast<uint8_t>(frame_data[NST_OFFSET]) & 0x3F;

        // FICF flag is in byte 6, bit 7 (ETSI EN 300 799 Section 5.3.1)
        // FICF=1 means FIC is present, FICF=0 means no FIC
        ficf_present = (static_cast<uint8_t>(frame_data[NST_OFFSET]) & 0x80) != 0;

        qDebug() << "HexViewerFormatter: Frame" << (frame_index + 1)
                 << "NST=" << nst << "FICF=" << ficf_present;
    }

    // Generate hex dump lines
    const int total_lines = (frame_data.size() + m_bytes_per_line - 1) / m_bytes_per_line;

    for (int line = 0; line < total_lines; ++line) {
        const size_t line_offset = line * m_bytes_per_line;

        // HIGH-003 FIX: Pass nst and ficf_present to formatHexLine
        html += formatHexLine(frame_data, line_offset, m_bytes_per_line,
                             line_offset, nst, ficf_present);
        html += "<br>";

        // Performance: Add section markers for better navigation
        if (line_offset == SYNC_OFFSET) {
            html += "<span class='section-marker'>--- SYNC PATTERN ---</span><br>";
        } else if (line_offset == LIDATA_OFFSET) {
            html += "<span class='section-marker'>--- LIDATA ---</span><br>";
        } else if (nst > 0 && ficf_present &&
                   line_offset == (LIDATA_OFFSET + LIDATA_SIZE + (nst * STREAM_HEADER_SIZE))) {
            // HIGH-003 FIX: Only show FIC marker if FICF=1
            html += "<span class='section-marker'>--- FIC DATA (96 bytes, FICF=1) ---</span><br>";
        } else if (nst > 0 && !ficf_present &&
                   line_offset == (LIDATA_OFFSET + LIDATA_SIZE + (nst * STREAM_HEADER_SIZE))) {
            // HIGH-003 FIX: Show MSC marker when no FIC
            html += "<span class='section-marker'>--- MSC DATA (FICF=0, no FIC) ---</span><br>";
        } else if (line_offset == (frame_data.size() - CRC_SIZE)) {
            html += "<span class='section-marker'>--- CRC (2 bytes) ---</span><br>";
        }
    }

    // Legend footer
    html += "<br>";
    html += generateLegend();
    html += "</body></html>";

    return html;
}

QString HexViewerFormatter::formatSection(const QByteArray& data, size_t start_offset, const QString& title)
{
    QString html;
    html.reserve(data.size() * 5); // Rough estimate

    html += QString("<div class='section-title'>%1 (%2 bytes, offset 0x%3)</div><br>")
                .arg(title)
                .arg(data.size())
                .arg(start_offset, 4, 16, QChar('0')).toUpper();

    const int total_lines = (data.size() + m_bytes_per_line - 1) / m_bytes_per_line;

    // For section formatting, we don't have frame context, so use default values
    uint8_t nst = 0;
    bool ficf_present = false;

    for (int line = 0; line < total_lines; ++line) {
        const size_t line_offset = line * m_bytes_per_line;
        const size_t absolute_offset = start_offset + line_offset;
        html += formatHexLine(data, line_offset, m_bytes_per_line,
                             absolute_offset, nst, ficf_present);
        html += "<br>";
    }

    return html;
}

QString HexViewerFormatter::formatHexLine(const QByteArray& data, size_t line_offset,
                                          int bytes_per_line, size_t absolute_offset,
                                          uint8_t nst, bool ficf_present) const
{
    QString line;
    line.reserve(200); // Estimated line length

    // Address column (4-digit hex)
    line += QString("<span class='address'>%1:</span> ")
                .arg(absolute_offset, 4, 16, QChar('0')).toUpper();

    // Hex bytes section
    QString hex_section;
    QString ascii_section;

    for (int byte_idx = 0; byte_idx < bytes_per_line; ++byte_idx) {
        const size_t data_offset = line_offset + byte_idx;

        if (data_offset < static_cast<size_t>(data.size())) {
            const uint8_t byte_value = static_cast<uint8_t>(data[data_offset]);

            // Format hex byte with color coding
            QString hex_byte = QString("%1").arg(byte_value, 2, 16, QChar('0')).toUpper();

            if (m_color_coding_enabled) {
                // HIGH-003 FIX: Pass nst and ficf_present to color coding function
                QString css_class = getCssClassForByte(absolute_offset + byte_idx, nst, ficf_present);
                hex_section += QString("<span class='%1'>%2</span> ")
                                   .arg(css_class, hex_byte);
            } else {
                hex_section += hex_byte + " ";
            }

            // ====================================================================
            // HIGH-001 FIX: HTML escaping for ASCII column to prevent injection
            // ====================================================================
            if (m_ascii_column_enabled) {
                if (byte_value >= 32 && byte_value <= 126) {
                    QChar ascii_char = QChar(static_cast<char>(byte_value));

                    // HTML escape special characters to prevent injection
                    if (ascii_char == '<') {
                        ascii_section += "&lt;";
                    } else if (ascii_char == '>') {
                        ascii_section += "&gt;";
                    } else if (ascii_char == '&') {
                        ascii_section += "&amp;";
                    } else if (ascii_char == '"') {
                        ascii_section += "&quot;";
                    } else {
                        ascii_section += ascii_char;
                    }
                } else {
                    // Non-printable characters shown as dot
                    ascii_section += '.';
                }
            }
        } else {
            // Padding for incomplete lines
            hex_section += "   ";
            if (m_ascii_column_enabled) {
                ascii_section += " ";
            }
        }
    }

    line += hex_section;

    if (m_ascii_column_enabled) {
        line += QString("<span class='ascii'>| %1</span>").arg(ascii_section);
    }

    return line;
}

QString HexViewerFormatter::getCssClassForByte(size_t byte_offset, uint8_t nst, bool ficf_present) const
{
    // ========================================================================
    // ETSI EN 300 799 frame structure color coding with FICF handling
    // HIGH-003 FIX: Accurate FIC boundary detection based on FICF flag
    // ========================================================================

    // SYNC pattern (bytes 0-3)
    if (byte_offset < SYNC_SIZE) {
        return "sync";
    }

    // ERR field (byte 4)
    if (byte_offset == ERR_OFFSET) {
        return "err";
    }

    // Frame Counter (FC) - byte 5
    if (byte_offset == FC_OFFSET) {
        return "fc";
    }

    // NST and mode fields (bytes 6-7)
    if (byte_offset == NST_OFFSET || byte_offset == NST_OFFSET + 1) {
        return "nst";
    }

    // LIDATA (bytes 8-11)
    if (byte_offset >= LIDATA_OFFSET && byte_offset < LIDATA_OFFSET + LIDATA_SIZE) {
        return "lidata";
    }

    // Stream headers (4 bytes per stream)
    const size_t stream_headers_start = LIDATA_OFFSET + LIDATA_SIZE;
    const size_t stream_headers_size = nst * STREAM_HEADER_SIZE;
    if (byte_offset >= stream_headers_start &&
        byte_offset < stream_headers_start + stream_headers_size) {
        return "stream-header";
    }

    // ========================================================================
    // HIGH-003 FIX: FIC data conditional on FICF flag
    // FIC is only present if FICF=1 (ETSI EN 300 799 Section 5.3.1)
    // ========================================================================
    if (ficf_present) {
        const size_t fic_start = stream_headers_start + stream_headers_size;
        if (byte_offset >= fic_start && byte_offset < fic_start + FIC_SIZE) {
            return "fic";
        }
    }

    // CRC (last 2 bytes)
    if (byte_offset >= FRAME_SIZE - CRC_SIZE) {
        return "crc";
    }

    // ========================================================================
    // HIGH-003 FIX: MSC data starts after stream headers and FIC (if present)
    // MSC offset adjusts based on FICF flag
    // ========================================================================
    const size_t msc_start = stream_headers_start + stream_headers_size +
                            (ficf_present ? FIC_SIZE : 0);
    if (byte_offset >= msc_start) {
        return "msc";
    }

    return "unknown";
}

QString HexViewerFormatter::generateStyleSheet() const
{
    return QString(
        "<style>"
        "body { "
            "font-family: 'Consolas', 'Monaco', 'Courier New', monospace; "
            "font-size: 10px; "
            "background-color: #1e1e1e; "  // Dark background for professional look
            "color: #d4d4d4; "
            "padding: 10px; "
        "}"

        ".header { "
            "background-color: #264f78; "
            "color: #ffffff; "
            "font-weight: bold; "
            "padding: 8px; "
            "border-radius: 4px; "
            "margin-bottom: 8px; "
        "}"

        ".section-title { "
            "background-color: #3c3c3c; "
            "color: #569cd6; "
            "font-weight: bold; "
            "padding: 4px 8px; "
            "border-left: 3px solid #569cd6; "
            "margin-top: 8px; "
        "}"

        ".section-marker { "
            "color: #6a9955; "
            "font-style: italic; "
            "font-size: 9px; "
        "}"

        ".address { "
            "color: #858585; "
            "font-weight: bold; "
            "margin-right: 8px; "
        "}"

        // ETI Structure Color Coding (Professional DAB Analysis Colors)
        ".sync { "
            "background-color: #5a1a1a; "  // Dark red background
            "color: #f48771; "             // Bright red text
            "font-weight: bold; "
        "}"

        ".err { "
            "background-color: #4a3c1a; "  // Dark orange
            "color: #dcdcaa; "
        "}"

        ".fc { "
            "background-color: #1a4a1a; "  // Dark green
            "color: #4ec9b0; "
            "font-weight: bold; "
        "}"

        ".nst { "
            "background-color: #1a3a4a; "  // Dark cyan
            "color: #4fc1ff; "
        "}"

        ".lidata { "
            "background-color: #3a2a5a; "  // Dark purple
            "color: #c586c0; "
        "}"

        ".stream-header { "
            "background-color: #2a4a3a; "  // Dark teal
            "color: #9cdcfe; "
        "}"

        ".fic { "
            "background-color: #4a4a1a; "  // Dark yellow
            "color: #d7ba7d; "
        "}"

        ".msc { "
            "color: #b5cea8; "             // Light green for MSC data
        "}"

        ".crc { "
            "background-color: #4a1a4a; "  // Dark magenta
            "color: #d16969; "
            "font-weight: bold; "
        "}"

        ".ascii { "
            "color: #808080; "
            "margin-left: 20px; "
            "font-family: 'Consolas', monospace; "
        "}"

        ".legend { "
            "margin-top: 16px; "
            "padding: 10px; "
            "background-color: #252526; "
            "border: 1px solid #3c3c3c; "
            "border-radius: 4px; "
        "}"

        ".legend-title { "
            "font-weight: bold; "
            "color: #569cd6; "
            "margin-bottom: 8px; "
        "}"

        ".legend-item { "
            "display: inline-block; "
            "margin-right: 15px; "
            "font-size: 9px; "
        "}"

        "</style>"
    );
}

QString HexViewerFormatter::generateLegend() const
{
    return QString(
        "<div class='legend'>"
        "<div class='legend-title'>ETI Frame Structure Legend (ETSI EN 300 799):</div>"
        "<span class='legend-item'><span class='sync'>SYNC</span> Sync Pattern (bytes 0-3)</span>"
        "<span class='legend-item'><span class='err'>ERR</span> Error Field (byte 4)</span>"
        "<span class='legend-item'><span class='fc'>FC</span> Frame Counter (byte 5)</span>"
        "<span class='legend-item'><span class='nst'>NST</span> Stream Count (bytes 6-7)</span>"
        "<br>"
        "<span class='legend-item'><span class='lidata'>LIDATA</span> Linear Data (bytes 8-11)</span>"
        "<span class='legend-item'><span class='stream-header'>STR-HDR</span> Stream Headers</span>"
        "<span class='legend-item'><span class='fic'>FIC</span> Fast Info Channel (96 bytes, if FICF=1)</span>"
        "<span class='legend-item'><span class='msc'>MSC</span> Main Service Channel</span>"
        "<span class='legend-item'><span class='crc'>CRC</span> CRC-16 (last 2 bytes)</span>"
        "</div>"
    );
}
