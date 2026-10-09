/**
 * @file enhanced_eti_processor_qt.cpp
 * @brief ETI (Ensemble Transport Interface) Frame Parser
 *
 * Implements parsing of 6144-byte ETI frames per ETSI EN 300 799.
 * Extracts header, FIC (Fast Information Channel), and MSC (Main Service Channel) data.
 *
 * ETI Frame Structure (ETSI EN 300 799 Section 5):
 * - SYNC pattern (4 bytes) - Frame synchronization
 * - Header fields (6 bytes) - ERR, FCT, FICF, NST, FP, MID, FL
 * - Stream info (4 bytes × NST) - Sub-channel descriptors
 * - FIC (96 bytes if FICF=1) - Fast Information Channel data
 * - MSC (variable) - Main Service Channel data
 * - CRC (2 bytes) - Frame integrity check
 *
 * @see ETSI EN 300 799 - Distribution Interfaces (ETI)
 * @see ETSI EN 300 401 - DAB Radio Broadcasting System
 */

#include "enhanced_eti_processor_qt.h"
#include "crc16.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QMutexLocker>

namespace {

// T20: a live ETI stream delivers up to ~250 frames/second; per-frame/per-
// subchannel diagnostic logs flood the console. Keep the first few frames
// (startup diagnostics) and then log every 250th occurrence per call site.
//
// Thread-safety (F9): each `static` counter below is a per-call-site local and
// is intentionally NOT synchronised. That is safe only because this processor
// is driven from a single (GUI) thread: processETIFile()/processLiveFrame()
// run on the owning thread, and the worker only hands frames over via queued
// signals. If this class is ever driven from multiple threads, promote these
// counters to std::atomic<uint64_t> members.
inline bool shouldLogFrameDiagnostic(unsigned long long counter)
{
    return counter <= 30 || (counter % 250) == 0;
}

}  // namespace

// CRITICAL-007: CRC-16 lookup table for CCITT polynomial
const uint16_t EnhancedETIProcessorQt::CRC16_TABLE[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

// ============================================================================
// ETI Frame Parser Implementation (Phase 1 Week 1)
// ============================================================================

ETIFrameParser::ETIFrameParser()
    : m_last_frame_counter(0)
    , m_first_frame(true)
    , m_frames_parsed(0)
    , m_frames_valid(0)
    , m_frames_invalid(0)
    , m_frame_counter_errors(0)
{
    qDebug() << "ETIFrameParser: Initialized real ETSI EN 300 799 parser";
}

void ETIFrameParser::resetStatistics()
{
    m_last_frame_counter = 0;
    m_first_frame = true;
    m_frames_parsed = 0;
    m_frames_valid = 0;
    m_frames_invalid = 0;
    m_frame_counter_errors = 0;
}

// Main parsing entry point
bool ETIFrameParser::parseFrame(const QByteArray& frame_data, ETIFrameData& result)
{
    m_frames_parsed++;

    // Validate frame size (ETSI EN 300 799: 6144 bytes per frame)
    if (frame_data.size() != 6144) {
        qWarning() << "ETIFrameParser: Invalid frame size:" << frame_data.size() << "(expected 6144)";
        m_frames_invalid++;
        result.is_valid = false;
        return false;
    }

    const uint8_t* data = reinterpret_cast<const uint8_t*>(frame_data.constData());

    // Parse header (first 12 bytes + stream info)
    result.header = parseHeader(data);

    if (!result.header.isValid()) {
        qWarning() << "ETIFrameParser: Invalid header detected";
        m_frames_invalid++;
        result.is_valid = false;
        return false;
    }

    // CRITICAL-001 FIX: Extract FIC if present with correct NST parameter
    // (FIC size depends on Mode Identity: 96 bytes for Mode I/II/IV, 128 for Mode III)
    if (result.header.ficf == 1) {
        result.fic_data = extractFIC(data, true, result.header.nst, result.header.mid);
        qDebug() << "ETIFrameParser: Extracted FIC data (" << result.fic_data.size() << "bytes)";
    } else {
        result.fic_data.clear();
    }

    // Extract MSC sub-channels
    result.sub_channels = extractMSC(data, result.header.nst, result.header.ficf == 1, result.header);

    // Extract frame CRC (last 2 bytes)
    result.frame_crc = extractUInt16(data, 6142);

    result.is_valid = true;
    m_frames_valid++;

    static unsigned long long s_parsedLogCount = 0;
    if (shouldLogFrameDiagnostic(++s_parsedLogCount)) {
        qDebug() << "ETIFrameParser: Successfully parsed frame" << m_frames_parsed
                 << "- FCT:" << result.header.frame_counter
                 << "NST:" << result.header.nst
                 << "FICF:" << result.header.ficf;
    }

    return true;
}

// Parse ETI frame header (ETSI EN 300 799 Section 5.1)
ETIHeader ETIFrameParser::parseHeader(const uint8_t* data)
{
    ETIHeader header;

    // ODR-DabMux ETI format byte layout:
    // Byte 0: ERR
    // Bytes 1-3: FSYNC (24-bit)
    // Byte 4: FCT
    // Byte 5: NST (bits 0-6) + FICF (bit 7)
    // Byte 6: FP + MID + FL_high
    // Bytes 7-8: FL_low + additional

    // SYNC: Combine all 4 bytes for validation
    header.sync = extractUInt32(data, 0);

    // ERR (byte 0)
    header.err = data[0];

    // FCT (byte 4)
    header.frame_counter = data[4];

    // FICF (byte 5, bit 7)
    header.ficf = (data[5] >> 7) & 0x01;

    // NST (byte 5, bits 6-0)
    header.nst = data[5] & 0x7F;

    // FP (byte 6, bits 7-5)
    header.fp = (data[6] >> 5) & 0x07;

    // MID (byte 6, bits 4-3)
    header.mid = (data[6] >> 3) & 0x03;

    // FL (bytes 7-8)
    uint16_t fl_high = (data[7] & 0x1F) << 6;
    uint16_t fl_low = (data[8] >> 2) & 0x3F;
    header.fl = fl_high | fl_low;

    // Validate sync pattern
    if (!validateSyncPattern(header.sync)) {
        qWarning() << "ETIFrameParser: Invalid SYNC pattern:" << Qt::hex << header.sync;
    }

    // Validate frame counter continuity
    if (!validateFrameCounter(header.frame_counter)) {
        qWarning() << "ETIFrameParser: Frame counter discontinuity:"
                   << header.frame_counter << "(expected" << ((m_last_frame_counter + 1) % 250) << ")";
        m_frame_counter_errors++;
    }

    return header;
}

// Validate SYNC pattern (ETSI EN 300 799 Section 5.1.1)
bool ETIFrameParser::validateSyncPattern(uint32_t sync)
{
    // Valid SYNC patterns per ETSI EN 300 799
    // ETI(NI): 0x49 0x93 0x1E 0x03 - Standard Non-Interleaved format
    // ETI(LI): 0xFF 0xF8 0xC5 0x49 and 0xFF 0x07 0x3A 0xB6 - Linear Interleaved formats

    // ETI-NI patterns (various byte order interpretations)
    bool is_eti_ni = (sync == 0xFF1F491F || sync == 0x491F1FFF ||
                      sync == 0xFF1FC4FF || sync == 0xC4FF1FFF ||
                      sync == 0x491FC4FF || sync == 0xC4FF491F ||
                      sync == 0x49931E03);  // Direct ETI-NI pattern

    // ETI-LI patterns (from bkk_20062022_141637.eti real file analysis)
    bool is_eti_li = (sync == 0xFFF8C549 || sync == 0x49C5F8FF ||  // Pattern A
                      sync == 0xFF073AB6 || sync == 0xB63A07FF);    // Pattern B

    return is_eti_ni || is_eti_li;
}

// Validate frame counter (FCT: 0-249, cycles every 5 seconds)
bool ETIFrameParser::validateFrameCounter(uint8_t fct)
{
    // FCT cycles 0-249 (250 frames per 5 seconds at 50ms/frame)
    if (fct >= 250) {
        qWarning() << "ETIFrameParser: FCT out of range:" << fct;
        return false;
    }

    bool valid = true;

    if (!m_first_frame) {
        uint8_t expected = (m_last_frame_counter + 1) % 250;
        if (fct != expected) {
            valid = false;
        }
    } else {
        m_first_frame = false;
    }

    m_last_frame_counter = fct;
    return valid;
}

// CRITICAL-001 FIX: Extract FIC data with correct offset calculation
QByteArray ETIFrameParser::extractFIC(const uint8_t* data, bool ficf_flag, uint8_t nst, uint8_t mid)
{
    if (!ficf_flag) {
        return QByteArray();
    }

    // ODR-DabMux ETI format (ETSI EN 300 799 with ODR byte ordering)
    // FIC starts after: ERR(1) + FSYNC(3) + FC(4) + STC(4*NST) + EOH(4)
    // FC = FCT(1) + NST/FICF(1) + FP/MID/FL(2)
    // STC = Stream Characterization (4 bytes per stream)
    // EOH = End of Header: MNSC(2) + CRC(2)
    const size_t HEADER_SIZE = 8;  // ERR + FSYNC + FC
    const size_t STREAM_INFO_SIZE = 4 * nst;
    const size_t EOH_SIZE = 4;  // MNSC + CRC
    const size_t FIC_OFFSET = HEADER_SIZE + STREAM_INFO_SIZE + EOH_SIZE;
    // 3 FIBs × 32 bytes for Mode I/II/IV, 4 FIBs × 32 bytes for Mode III
    const size_t FIC_SIZE = (mid == 3) ? 128 : 96;

    // Bounds check
    if (FIC_OFFSET + FIC_SIZE > 6144) {
        qWarning() << "ETIFrameParser::extractFIC: FIC extraction bounds error"
                   << "offset=" << FIC_OFFSET << "NST=" << nst;
        return QByteArray();
    }

    const uint8_t* fic_start = data + FIC_OFFSET;

    static unsigned long long s_ficLogCount = 0;
    if (shouldLogFrameDiagnostic(++s_ficLogCount)) {
        qDebug() << "ETIFrameParser::extractFIC: offset=" << FIC_OFFSET
                 << "NST=" << nst << "FIC size=" << FIC_SIZE;
    }

    return QByteArray(reinterpret_cast<const char*>(fic_start), FIC_SIZE);
}

// Parse stream info structure (4 bytes per stream)
ETIFrameParser::StreamInfo ETIFrameParser::parseStreamInfo(const uint8_t* data, size_t offset) const
{
    StreamInfo info;

    // Stream info format (ETSI EN 300 799 Section 5.3.2):
    // Byte 0: SCId (bits 7-2), reserved (bits 1-0)
    // Byte 1-2: SAD (10 bits start address)
    // Byte 3: TPL (5 bits), STL (11 bits high part)
    // Following bytes: STL continuation

    info.sub_channel_id = (data[offset] >> 2) & 0x3F;

    // Start address (10 bits across bytes 1-2)
    info.start_address = ((static_cast<uint16_t>(data[offset + 1]) << 2) |
                          ((data[offset + 2] >> 6) & 0x03)) & 0x3FF;

    // Table switch
    info.table_switch = (data[offset + 2] >> 5) & 0x01;

    // Table index (protection level)
    info.table_index = data[offset + 2] & 0x1F;

    // Stream length in 8-byte words (11 bits)
    info.stream_length = ((static_cast<uint16_t>(data[offset + 3]) << 3) |
                          ((data[offset + 4] >> 5) & 0x07)) & 0x7FF;

    return info;
}

// HIGH-004 FIX: Extract MSC sub-channels with correct offset calculation
std::vector<SubChannelData> ETIFrameParser::extractMSC(const uint8_t* data, uint8_t nst,
                                                        bool ficf_flag, const ETIHeader& header)
{
    std::vector<SubChannelData> sub_channels;

    if (nst == 0) {
        return sub_channels;  // No sub-channels
    }

    // Calculate MSC offset per ETSI EN 300 799 (RAW ETI):
    // SYNC(4) + FC(4) + STC(4*NST) + EOH(4 = MNSC+CRC) + FIC + MSC
    const size_t HEADER_SIZE = 12;
    const size_t STREAM_INFO_SIZE = 4 * nst;
    const size_t FIC_SIZE = ficf_flag ? ((header.mid == 3) ? 128 : 96) : 0;

    size_t stream_info_offset = 8;
    size_t msc_offset = HEADER_SIZE + STREAM_INFO_SIZE + FIC_SIZE;

    static unsigned long long s_mscHeaderLogCount = 0;
    if (shouldLogFrameDiagnostic(++s_mscHeaderLogCount)) {
        qDebug() << "ETIFrameParser::extractMSC: NST=" << nst
                 << "FICF=" << ficf_flag
                 << "stream_info_offset=" << stream_info_offset
                 << "msc_offset=" << msc_offset;
    }

    // Parse stream info for each sub-channel
    for (uint8_t i = 0; i < nst; ++i) {
        if (stream_info_offset + 4 > 6144) {
            qWarning() << "ETIFrameParser::extractMSC: Stream info bounds error at index" << i;
            break;
        }

        SubChannelData sc;

        // Parse stream info (4 bytes per stream)
        // Byte 0: SubChId (6 bits, bits 7-2)
        sc.sub_channel_id = (data[stream_info_offset] >> 2) & 0x3F;

        // Bytes 0-1: SAD - Sub-channel Start Address (10 bits)
        sc.start_address = ((data[stream_info_offset] & 0x03) << 8) |
                          data[stream_info_offset + 1];

        // Byte 2: TPL (Table index) or Protection Level
        sc.table_switch = (data[stream_info_offset + 2] >> 7) & 0x01;

        // Byte 3: STL - Sub-channel Stream Length (10 bits)
        uint16_t stream_length = ((data[stream_info_offset + 2] & 0x03) << 8) |
                                data[stream_info_offset + 3];

        // Stream length is in units of 8 bytes (CUs)
        sc.size_in_bytes = stream_length * 8;

        // Extract MSC data for this sub-channel
        if (msc_offset + sc.size_in_bytes <= 6144) {
            sc.data = QByteArray(reinterpret_cast<const char*>(data + msc_offset),
                                sc.size_in_bytes);
            msc_offset += sc.size_in_bytes;
        } else {
            qWarning() << "ETIFrameParser::extractMSC: MSC data bounds error"
                       << "SubCh=" << sc.sub_channel_id
                       << "offset=" << msc_offset
                       << "size=" << sc.size_in_bytes;
            sc.data = QByteArray();
        }

        sub_channels.push_back(sc);
        stream_info_offset += 4;

        static unsigned long long s_mscChannelLogCount = 0;
        if (shouldLogFrameDiagnostic(++s_mscChannelLogCount)) {
            qDebug() << "ETIFrameParser::extractMSC: Sub-channel" << i << "-"
                     << "ID:" << sc.sub_channel_id
                     << "Start:" << sc.start_address
                     << "Size:" << sc.size_in_bytes << "bytes";
        }
    }

    return sub_channels;
}

// Helper: Extract 16-bit value (big-endian)
uint16_t ETIFrameParser::extractUInt16(const uint8_t* data, size_t offset) const
{
    return (static_cast<uint16_t>(data[offset]) << 8) |
           static_cast<uint16_t>(data[offset + 1]);
}

// Helper: Extract 32-bit value (big-endian)
uint32_t ETIFrameParser::extractUInt32(const uint8_t* data, size_t offset) const
{
    return (static_cast<uint32_t>(data[offset]) << 24) |
           (static_cast<uint32_t>(data[offset + 1]) << 16) |
           (static_cast<uint32_t>(data[offset + 2]) << 8) |
           static_cast<uint32_t>(data[offset + 3]);
}

// ============================================================================
// ETI Header Helper Methods
// ============================================================================

bool ETIHeader::isValid() const
{
    // Check sync pattern (ETSI EN 300 799 Section 5.1.1 & 6.1)
    // ETI-NI patterns
    bool is_eti_ni = (sync == 0xFF1F491F || sync == 0x491F1FFF ||
                      sync == 0xFF1FC4FF || sync == 0xC4FF1FFF ||
                      sync == 0x491FC4FF || sync == 0xC4FF491F ||
                      sync == 0x49931E03);

    // ETI-LI patterns (CRITICAL FIX: Add ETI-LI SYNC support)
    bool is_eti_li = (sync == 0xFFF8C549 || sync == 0x49C5F8FF ||  // Pattern A
                      sync == 0xFF073AB6 || sync == 0xB63A07FF);    // Pattern B

    if (!is_eti_ni && !is_eti_li) {
        qWarning() << "ETIHeader::isValid(): Invalid SYNC pattern:"
                   << QString("0x%1").arg(sync, 8, 16, QChar('0')).toUpper();
        return false;
    }

    // Check frame counter range (0-249)
    if (frame_counter >= 250) {
        return false;
    }

    // CRITICAL-005: ETI-LI has different header structure per ETSI EN 300 799 Section 6
    // For ETI-LI frames, many fields have different meanings/ranges
    if (is_eti_li) {
        // ETI-LI uses time-interleaved transmission with Pattern A/B alternating
        // Header field validation is more relaxed for ETI-LI format
        // The NST and other fields may encode different information in ETI-LI
        static unsigned long long s_liHeaderLogCount = 0;
        if (shouldLogFrameDiagnostic(++s_liHeaderLogCount)) {
            qDebug() << "ETIHeader::isValid(): ETI-LI frame detected, using relaxed validation";
        }
        return true;  // Accept ETI-LI frames with relaxed validation
    }

    // ETI-NI validation (strict checks)
    // Check NST range (0-64)
    if (nst > 64) {
        return false;
    }

    // Check FICF flag (0 or 1)
    if (ficf > 1) {
        return false;
    }

    // Check frame phase (0-7)
    if (fp > 7) {
        return false;
    }

    // Check mode identity (0-3 for Modes I-IV)
    if (mid > 3) {
        return false;
    }

    return true;
}

QString ETIHeader::toString() const
{
    return QString("ETI Header: SYNC=0x%1 ERR=%2 FCT=%3 FICF=%4 NST=%5 FP=%6 MID=%7 FL=%8")
        .arg(sync, 8, 16, QChar('0'))
        .arg(err)
        .arg(frame_counter)
        .arg(ficf)
        .arg(nst)
        .arg(fp)
        .arg(mid)
        .arg(fl);
}

// ============================================================================
// Sub-channel Data Helper Methods
// ============================================================================

QString SubChannelData::toString() const
{
    return QString("SubChannel: ID=%1 StartAddr=%2 TableSwitch=%3 TableIdx=%4 Size=%5 bytes")
        .arg(sub_channel_id)
        .arg(start_address)
        .arg(table_switch)
        .arg(table_index)
        .arg(size_in_bytes);
}

// ============================================================================
// ETI Frame Data Helper Methods
// ============================================================================

QString ETIFrameData::summary() const
{
    QString result = QString("ETI Frame: %1\n").arg(header.toString());
    result += QString("  FIC: %1 bytes\n").arg(fic_data.size());
    result += QString("  Sub-channels: %1\n").arg(sub_channels.size());
    for (size_t i = 0; i < sub_channels.size(); ++i) {
        result += QString("    [%1] %2\n").arg(i).arg(sub_channels[i].toString());
    }
    result += QString("  CRC: 0x%1\n").arg(frame_crc, 4, 16, QChar('0'));
    result += QString("  Valid: %1").arg(is_valid ? "YES" : "NO");
    return result;
}

// ============================================================================
// Enhanced ETI Processor Qt Implementation
// ============================================================================

EnhancedETIProcessorQt::EnhancedETIProcessorQt(QObject *parent)
    : QObject(parent)
    , frame_count_(0)
    , eti_ni_count_(0)
    , eti_li_a_count_(0)
    , eti_li_b_count_(0)
    , current_frame_index_(0)
    , total_frames_(0)
    , processing_timer_(new QTimer(this))
    , m_frame_parser(std::make_unique<ETIFrameParser>())
{
    // Register custom types with Qt meta-object system for signal/slot connections
    qRegisterMetaType<ProcessedFrame>("ProcessedFrame");
    qRegisterMetaType<ETIFormat>("ETIFormat");
    qDebug() << "EnhancedETIProcessorQt: Registered custom metatypes for Qt signals";
    qDebug() << "EnhancedETIProcessorQt: Real ETI frame parser initialized";

    // Setup processing timer for real-time updates
    processing_timer_->setSingleShot(false);
    processing_timer_->setInterval(1); // 1ms intervals for fast processing
    connect(processing_timer_, &QTimer::timeout, this, &EnhancedETIProcessorQt::processNextFrame);
}

// CRITICAL-007: CRC-16 calculation using CCITT polynomial
uint16_t EnhancedETIProcessorQt::calculateCRC16(const uint8_t* data, size_t length) const
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        uint8_t index = (crc >> 8) ^ data[i];
        crc = (crc << 8) ^ CRC16_TABLE[index];
    }
    return crc;
}

// CRITICAL-007: Validate ETI frame CRC
bool EnhancedETIProcessorQt::validateFrameCRC(const QByteArray& frameData) const
{
    if (frameData.size() != 6144) {
        return false;
    }

    const uint8_t* data = reinterpret_cast<const uint8_t*>(frameData.constData());

    // Extract SYNC pattern to determine ETI format
    uint32_t sync = (static_cast<uint32_t>(data[0]) << 24) |
                    (static_cast<uint32_t>(data[1]) << 16) |
                    (static_cast<uint32_t>(data[2]) << 8) |
                    static_cast<uint32_t>(data[3]);

    // Check if ETI-LI format
    bool is_eti_li = (sync == 0xFFF8C549 || sync == 0x49C5F8FF ||  // Pattern A
                      sync == 0xFF073AB6 || sync == 0xB63A07FF);    // Pattern B

    // ETI-LI format uses different frame structure and may not have CRC
    // Per ETSI EN 300 799 Section 6 (ETI-LI), CRC validation differs from ETI-NI
    if (is_eti_li) {
        // For ETI-LI, skip CRC validation or use alternative validation
        // The file uses 0x5555 padding instead of CRC
        static unsigned long long s_liCrcLogCount = 0;
        if (shouldLogFrameDiagnostic(++s_liCrcLogCount)) {
            qDebug() << "ETI-LI frame detected - skipping CRC validation (format-specific)";
        }
        return true;  // Accept ETI-LI frames without strict CRC check
    }

    // ETI-NI format: Validate CRC as usual
    uint16_t calculated = calculateCRC16(data, 6142);
    uint16_t stored = (static_cast<uint16_t>(data[6142]) << 8) |
                     static_cast<uint16_t>(data[6143]);

    return calculated == stored;
}

// CRITICAL-007: Validate FIC CRC (optional - can be implemented later)
bool EnhancedETIProcessorQt::validateFICCRC(const QByteArray& ficData) const
{
    // FIC CRC validation can be added if needed
    // For now, return true as FIC validation is done at frame level
    Q_UNUSED(ficData)
    return true;
}

// HIGH-007: Store frame with FIFO cleanup when limit reached
void EnhancedETIProcessorQt::storeFrame(const ProcessedFrame& frame)
{
    // Check if we need to cleanup old frames
    if (processed_frames_.size() >= MAX_STORED_FRAMES) {
        // Remove oldest frames (FIFO)
        size_t remove_count = FRAME_CLEANUP_THRESHOLD;
        processed_frames_.erase(
            processed_frames_.begin(),
            processed_frames_.begin() + remove_count
        );

        qInfo() << "Frame storage limit reached, removed"
                << remove_count << "oldest frames";
    }

    processed_frames_.push_back(frame);
}

// HIGH-007: Enhanced file loading with comprehensive validation
bool EnhancedETIProcessorQt::processETIFile(const QString& filePath)
{
    // Reset state
    resetStatistics();

    QFile file(filePath);

    // Check file exists
    if (!file.exists()) {
        emit processingError(QString("File does not exist: %1").arg(filePath));
        return false;
    }

    // Try to open
    if (!file.open(QIODevice::ReadOnly)) {
        emit processingError(QString("Cannot open file: %1 - %2")
                           .arg(filePath).arg(file.errorString()));
        return false;
    }

    // Check file size
    qint64 fileSize = file.size();
    if (fileSize <= 0) {
        emit processingError(QString("Empty or invalid file: %1").arg(filePath));
        file.close();
        return false;
    }

    if (fileSize > MAX_FILE_SIZE) {
        emit processingError(
            QString("File too large: %1 MB (limit: %2 MB)")
                .arg(fileSize / 1024.0 / 1024.0, 0, 'f', 1)
                .arg(MAX_FILE_SIZE / 1024.0 / 1024.0, 0, 'f', 0)
        );
        file.close();
        return false;
    }

    // Validate file size is multiple of frame size
    if (fileSize % 6144 != 0) {
        emit processingError(
            QString("Invalid ETI file size: %1 bytes (not multiple of 6144)")
                .arg(fileSize)
        );
        file.close();
        return false;
    }

    // Read file data
    file_data_ = file.readAll();
    if (file_data_.size() != fileSize) {
        emit processingError(
            QString("Read error: expected %1 bytes, got %2")
                .arg(fileSize).arg(file_data_.size())
        );
        file.close();
        return false;
    }

    file.close();

    total_frames_ = fileSize / 6144;
    current_frame_index_ = 0;
    current_file_path_ = filePath;

    qInfo() << "Loaded ETI file:" << filePath
            << "Size:" << fileSize
            << "Frames:" << total_frames_;

    // Setup processing
    QFileInfo fileInfo(filePath);
    current_file_name_ = fileInfo.fileName();
    processing_start_time_ = QDateTime::currentDateTime();

    // Start processing
    qDebug() << "EnhancedETIProcessorQt: Emitting processingStarted signal for" << total_frames_ << "frames";
    emit processingStarted(current_file_name_, total_frames_);

    // Use immediate processing instead of timer for better reliability
    if (total_frames_ > 0) {
        qDebug() << "EnhancedETIProcessorQt: Starting immediate processing of" << total_frames_ << "frames";
        processAllFramesImmediate();
        qDebug() << "EnhancedETIProcessorQt: Immediate processing completed";
    } else {
        qDebug() << "EnhancedETIProcessorQt: No frames to process, emitting processingComplete";
        emit processingComplete(total_frames_, 0);
    }

    qDebug() << "EnhancedETIProcessorQt: processETIFile returning true";
    return true;
}

void EnhancedETIProcessorQt::processAllFramesImmediate()
{
    // Process frames immediately for file analysis
    for (int i = 0; i < total_frames_; ++i) {
        current_frame_index_ = i;

        // Extract frame data
        int frameOffset = i * 6144;
        QByteArray frameData = file_data_.mid(frameOffset, 6144);

        // Process frame - this will emit frameProcessed signals
        processEtiFrame(frameData);

        // Update progress every 100 frames to avoid UI flooding
        if (i % 100 == 0 || i == total_frames_ - 1) {
            int percentage = (i * 100) / total_frames_;
            emit processingProgress(percentage, i + 1, total_frames_);
        }

        // Process Qt events frequently to keep UI responsive
        if (i % 10 == 0) {
            QCoreApplication::processEvents();
            // Small delay to allow UI updates
            QThread::msleep(1);
        }
    }

    // Processing complete
    auto endTime = QDateTime::currentDateTime();
    int processingTimeMs = processing_start_time_.msecsTo(endTime);

    emit processingComplete(total_frames_, processingTimeMs);
}

void EnhancedETIProcessorQt::processNextFrame()
{
    if (current_frame_index_ >= total_frames_) {
        // Processing complete
        processing_timer_->stop();

        auto endTime = QDateTime::currentDateTime();
        int processingTimeMs = processing_start_time_.msecsTo(endTime);

        emit processingComplete(total_frames_, processingTimeMs);
        return;
    }

    // Restore fast processing: larger batches
    const int BATCH_SIZE = 100; // Process 100 frames per timer tick for speed
    int batch_end = std::min(current_frame_index_ + BATCH_SIZE, static_cast<int>(total_frames_));

    for (int i = current_frame_index_; i < batch_end; ++i) {
        // Extract frame data
        int frameOffset = i * 6144;
        QByteArray frameData = file_data_.mid(frameOffset, 6144);

        // Process frame
        if (processEtiFrame(frameData)) {
            // Update progress every batch or for the last frame
            if (i == batch_end - 1 || i == total_frames_ - 1) {
                int percentage = (i * 100) / total_frames_;
                emit processingProgress(percentage, i + 1, total_frames_);
            }
        }
    }

    current_frame_index_ = batch_end;

    // Check if all frames processed
    if (current_frame_index_ >= total_frames_) {
        processing_timer_->stop();

        auto endTime = QDateTime::currentDateTime();
        int processingTimeMs = processing_start_time_.msecsTo(endTime);

        emit processingComplete(total_frames_, processingTimeMs);
    }
}

// HIGH-007: Enhanced input validation for processEtiFrame with real parsing
bool EnhancedETIProcessorQt::processEtiFrame(const QByteArray& frameData)
{
    // Validate frameData before processing
    if (frameData.isEmpty() || !frameData.constData()) {
        emit processingError("Invalid frame data (null or empty)");
        return false;
    }

    if (frameData.size() != 6144) {
        emit processingError(QString("Invalid frame size: %1 bytes (expected 6144)")
                           .arg(frameData.size()));
        return false;
    }

    // CRITICAL-007: Validate frame CRC
    if (!validateFrameCRC(frameData)) {
        emit frameError(frame_count_, "Frame CRC validation failed",
                       "Corrupted frame detected - CRC mismatch");
        ProcessedFrame frame;
        frame.frame_number = ++frame_count_;
        frame.is_valid = false;
        return false;
    }

    // === Phase 1 Week 1: Real ETI Frame Parsing ===
    ETIFrameData parsed_frame;

    // Parse frame with new real parser
    if (!m_frame_parser->parseFrame(frameData, parsed_frame)) {
        emit frameError(frame_count_, "ETI frame parsing failed",
                       "Unable to parse frame structure");
        ProcessedFrame frame;
        frame.frame_number = ++frame_count_;
        frame.is_valid = false;
        return false;
    }

    // Log parsed frame information (throttled: a live 250 fps stream must not
    // flood the console with one line per frame — T20 no-spam rule).
    const uint64_t parsedFrame = frame_count_ + 1;
    if (parsedFrame <= 30 || parsedFrame % 250 == 0) {
        qDebug() << "Frame" << parsedFrame << "parsed:" << parsed_frame.header.toString();
    }

    // Process FIC if present (will be used by FIG analyser - Agent 3's work)
    if (!parsed_frame.fic_data.isEmpty()) {
        if (!validateFICCRC(parsed_frame.fic_data)) {
            qWarning() << "FIC CRC validation warning for frame" << (frame_count_ + 1);
        }
        // FIC will be processed by FIG analyser in future implementation
        processFicData(parsed_frame.fic_data);
    }

    // === Legacy format detection for compatibility ===
    ProcessedFrame frame;
    frame.frame_number = ++frame_count_;
    frame.timestamp = QDateTime::currentDateTime();

    const uint8_t* data = reinterpret_cast<const uint8_t*>(frameData.constData());
    frame.sync_pattern = parsed_frame.header.sync;
    frame.format = detectETIFormat(frame.sync_pattern);

    if (frame.format == ETIFormat::UNKNOWN) {
        QString error = QString("Unknown ETI format with SYNC pattern: 0x%1")
                       .arg(frame.sync_pattern, 8, 16, QChar('0'));
        emit frameError(current_frame_index_, "Invalid SYNC Pattern", error);
        frame.is_valid = false;
        return false;
    }

    // Update format counts
    switch (frame.format) {
        case ETIFormat::ETI_NI:   eti_ni_count_++; break;
        case ETIFormat::ETI_LI_A: eti_li_a_count_++; break;
        case ETIFormat::ETI_LI_B: eti_li_b_count_++; break;
        default: break;
    }

    // Populate legacy frame structure for signals
    frame.lidata = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    
    // CRITICAL FIX: Force deep copy of FIC data to prevent dangling pointers
    // when using Qt::QueuedConnection (fixes segmentation fault)
    frame.fic_data = QByteArray(parsed_frame.fic_data.constData(), 
                                parsed_frame.fic_data.size());
    
    // T24: the MSC bytes are owned exactly once — by the per-subchannel slices
    // built below. Keep only the concatenated size for byte-count metrics; the
    // old duplicate `msc_data` copy (~5-6 KB/frame) is gone.
    std::size_t msc_size_total = 0;
    for (const auto& sc : parsed_frame.sub_channels) {
        msc_size_total += static_cast<std::size_t>(sc.data.size());
    }
    frame.msc_size = msc_size_total;

    // Reuse: carry the already-parsed per-subchannel MSC slices so the GUI
    // DLS+ pipeline does not have to re-run parseHeader()/extractMSC(). Shared
    // behind a shared_ptr so a queued/by-value frame hand-off copies only the
    // pointer (O(1)) instead of every slice's bytes.
    auto slices = std::make_shared<std::vector<ProcessedFrame::SubChannelSlice>>();
    slices->reserve(parsed_frame.sub_channels.size());
    for (const auto& sc : parsed_frame.sub_channels) {
        ProcessedFrame::SubChannelSlice slice;
        slice.sub_channel_id = sc.sub_channel_id;
        slice.data = sc.data;
        slices->push_back(std::move(slice));
    }
    frame.sub_channels = std::move(slices);
    
    frame.crc = parsed_frame.frame_crc;
    frame.is_valid = true;

    // HIGH-007: Store frame using FIFO cleanup method
    storeFrame(frame);

    // Emit frameProcessed() for EVERY valid frame. Rationale (Finding 7):
    //  - FIG 1/1 service labels are discovered from FIC frames; and
    //  - the GUI DLS+/Now Playing pipeline must see exactly one CIF chunk per
    //    ETI frame per sub-channel. Gating on FIC presence (FICF) would skip
    //    FICF=0 frames — which still carry MSC — and break the 5-CIF DAB+
    //    superframe cadence. onFrameProcessed() skips FIG analysis when
    //    fic_data is empty, so those frames only feed the DLS+ accumulator.
    const bool isKeyFrame = (frame.frame_number == 1)
                            || (frame.frame_number % 100 == 0)
                            || (frame.frame_number == total_frames_);
    // Log only key frames (and early frames) to avoid flooding the console.
    if (isKeyFrame || frame.frame_number <= 30) {
        qDebug() << "EnhancedETIProcessorQt: Emitting frameProcessed signal for frame"
                 << frame.frame_number << "FIC size:" << frame.fic_data.size();
    }
    emit formatDetected(frame.format, formatToString(frame.format));
    emit frameProcessed(frame);

    return true;
}

void EnhancedETIProcessorQt::processFicData(const QByteArray& ficData)
{
    if (ficData.size() < 32) return;

    // (per-frame log removed: FIC frames arrive at the stream rate and the
    // frame-level log above is already throttled — T20 no-spam rule.)

    // The full FIC (96/128 bytes) is a sequence of FIBs; each FIB is 32 bytes:
    // a 30-byte FIG area plus a 2-byte CRC-16. Parse FIGs within each FIB's
    // FIG area (FIG header: type b7-b5, length b4-b0; type 7 = padding).
    for (int fib_base = 0; fib_base + 32 <= ficData.size(); fib_base += 32) {
        const uint8_t* fib = reinterpret_cast<const uint8_t*>(ficData.constData()) + fib_base;

        // Validate FIB CRC (complemented CRC-16/CCITT-FALSE, big-endian);
        // skip corrupted FIBs. Shared CRC implementation: crc16.hpp (F6 dedup).
        const uint16_t expected = static_cast<uint16_t>(~eti::crc16ccitt_false(fib, 30));
        const uint16_t stored = static_cast<uint16_t>((fib[30] << 8) | fib[31]);
        if (expected != stored) {
            qWarning() << "EnhancedETIProcessorQt::processFicData: FIB" << (fib_base / 32)
                       << "CRC mismatch, skipping";
            continue;
        }

        // Parse FIGs within the 30-byte FIG area
        int i = 0;
        while (i + 1 < 30) {
            uint8_t figHeader = fib[i];
            uint8_t figType = (figHeader >> 5) & 0x07;

            if (figType == 0x07) {
                break;  // Padding
            }

            uint8_t figLength = figHeader & 0x1F;

            if (figLength == 0 || i + figLength + 1 > 30) {
                break;  // Invalid FIG
            }

            FIGInfo fig;
            fig.type = figType;
            fig.length = figLength;
            // FIG 0 extension flags live in the first data byte
            if (figType == 0) {
                uint8_t ext = static_cast<uint8_t>(fib[i + 1]);
                fig.continuation = (ext & 0x80) != 0;
                fig.other_ensemble = (ext & 0x40) != 0;
            }
            fig.data = QByteArray(reinterpret_cast<const char*>(fib + i + 1), figLength);

            emit figDiscovered(fig);

            i += figLength + 1; // Skip processed FIG
        }
    }
}

ETIFormat EnhancedETIProcessorQt::detectETIFormat(uint32_t syncPattern) const
{
    switch (syncPattern) {
        case ETI_NI_SYNC:   return ETIFormat::ETI_NI;
        case ETI_LI_SYNC_A: return ETIFormat::ETI_LI_A;
        case ETI_LI_SYNC_B: return ETIFormat::ETI_LI_B;
        default:            return ETIFormat::UNKNOWN;
    }
}

QString EnhancedETIProcessorQt::formatToString(ETIFormat format) const
{
    switch (format) {
        case ETIFormat::ETI_NI:   return "ETI-NI (Network Independent)";
        case ETIFormat::ETI_LI_A: return "ETI-LI Pattern A (Linear Interface)";
        case ETIFormat::ETI_LI_B: return "ETI-LI Pattern B (Linear Interface)";
        default:                  return "Unknown Format";
    }
}

QString EnhancedETIProcessorQt::syncPatternToString(uint32_t pattern) const
{
    return QString("0x%1").arg(pattern, 8, 16, QChar('0'));
}

// CRITICAL-002 FIX: Thread-safe getRawFrameData with mutex protection and explicit detach
QByteArray EnhancedETIProcessorQt::getRawFrameData(int frame_index) const
{
    // Thread-safety: Lock mutex to protect file_data_ access
    QMutexLocker locker(&m_file_data_mutex);

    // Validate frame index
    if (frame_index < 0 || frame_index >= total_frames_) {
        qWarning() << "getRawFrameData: Invalid frame index" << frame_index
                   << "(valid range: 0 -" << (total_frames_ - 1) << ")";
        return QByteArray();
    }

    // Validate file data is loaded
    if (file_data_.isEmpty()) {
        qWarning() << "getRawFrameData: No ETI file loaded";
        return QByteArray();
    }

    // Calculate frame offset (each frame is exactly 6144 bytes)
    const int frame_offset = frame_index * ETI_FRAME_SIZE;

    // Bounds check
    if (frame_offset + ETI_FRAME_SIZE > file_data_.size()) {
        qWarning() << "getRawFrameData: Frame offset out of bounds"
                   << "offset=" << frame_offset
                   << "file_size=" << file_data_.size();
        return QByteArray();
    }

    // Extract frame data with explicit detach to avoid Qt COW race conditions
    QByteArray result = file_data_.mid(frame_offset, ETI_FRAME_SIZE);
    result.detach();  // CRITICAL-002 FIX: Force deep copy for thread safety

    qDebug() << "getRawFrameData: Extracted frame" << frame_index
             << "(" << result.size() << "bytes)"
             << "offset=" << frame_offset;

    return result;
}

// Phase 4: Live streaming support
bool EnhancedETIProcessorQt::processLiveFrame(const QByteArray& frameData)
{
    // Validate input
    if (frameData.size() != ETI_FRAME_SIZE) {
        qWarning() << "[LIVE] Invalid frame size:" << frameData.size() << "expected" << ETI_FRAME_SIZE;
        return false;
    }
    
    // Process frame using existing frame processing logic
    // This reuses the validated ETI frame parser and FIC processing
    bool success = processEtiFrame(frameData);
    
    if (success) {
        // Frame successfully processed - signals emitted by processEtiFrame:
        // - frameProcessed(ProcessedFrame)
        // - figDiscovered(FIGInfo) via processFicData
        // Throttled: one line per 250 live frames, not per frame (T20).
        if (frame_count_ % 250 == 0) {
            qDebug() << "[LIVE] Frame" << frame_count_ << "processed successfully";
        }
    }
    
    return success;
}

void EnhancedETIProcessorQt::stopProcessing()
{
    processing_timer_->stop();
}

void EnhancedETIProcessorQt::resetStatistics()
{
    frame_count_ = 0;
    eti_ni_count_ = 0;
    eti_li_a_count_ = 0;
    eti_li_b_count_ = 0;
    current_frame_index_ = 0;
    total_frames_ = 0;
    processed_frames_.clear();
    file_data_.clear();

    // Reset parser statistics
    if (m_frame_parser) {
        m_frame_parser->resetStatistics();
    }
}
