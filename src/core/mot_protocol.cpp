/**
 * @file mot_protocol.cpp
 * @brief MOT (Multimedia Object Transfer) Protocol Parser Implementation
 *
 * Standard-layout implementation of the ETSI EN 301 234 MOT protocol for DAB
 * data services: processMOTData() consumes a complete ETSI EN 300 401 §5.3.3
 * MSC data group, parses the MOT session/segmentation headers and reassembles
 * the MOT header entity (data-group type 3) and body entity (type 4) with no
 * internal re-framing.
 *
 * @author StreamDAB Development Team
 * @date October 2026
 * @version 2.0
 */

#include "mot_protocol.hpp"

#include <QDebug>
#include <algorithm>

#include "charset_converter.hpp"
#include "crc16.hpp"

namespace eti::mot {

namespace {

// Parse the MOT session header (EN 300 401 §5.3.3.1; X-PAD §7.4.2) at
// @p dg[offset].
bool parseSessionHeader(const QByteArray& dg, int& offset, bool& lastSeg,
                        uint16_t& segNumber, uint32_t& transportId)
{
    if (dg.size() < offset + 3) {
        return false;
    }
    const uint8_t* d = reinterpret_cast<const uint8_t*>(dg.constData());
    lastSeg = (d[offset] & 0x80) != 0;
    segNumber = static_cast<uint16_t>(((d[offset] & 0x7F) << 8) | d[offset + 1]);
    const bool transportIdFlag = (d[offset + 2] & 0x10) != 0;
    const int lenIndicator = d[offset + 2] & 0x0F;
    offset += 3;

    // The transport-id length indicator is a 4-bit field. EN 300 401 §5.3.3.1
    // allows a 0-byte id (flag clear) or a 2-/4-byte id; the value 3 is not a
    // legal MOT transport-id length and must be rejected (as must 1 and >4).
    if (!transportIdFlag || (lenIndicator != 2 && lenIndicator != 4)) {
        return false;
    }
    if (dg.size() < offset + lenIndicator) {
        return false;
    }
    transportId = 0;
    for (int i = 0; i < lenIndicator; ++i) {
        transportId = (transportId << 8) | static_cast<uint8_t>(dg[offset + i]);
    }
    offset += lenIndicator;
    return true;
}

// Parse the 13-bit segment size (EN 301 234 §5.1.1) at @p dg[offset]. The top
// 3 bits are the RepetitionCount (parsed but ignored: MOT reassembly is keyed
// by transport id + segment number, not by repetition).
bool parseSegmentationHeader(const QByteArray& dg, int& offset, int& segSize,
                             int& repetitionCount)
{
    if (dg.size() < offset + 2) {
        return false;
    }
    const uint8_t* d = reinterpret_cast<const uint8_t*>(dg.constData());
    repetitionCount = (d[offset] >> 5) & 0x07;
    segSize = ((d[offset] & 0x1F) << 8) | d[offset + 1];
    offset += 2;
    return true;
}

}  // namespace

// ============================================================================
// Helper Functions - Content Type Conversion
// ============================================================================

QString MOTHeader::getContentTypeString() const {
    switch (content_type) {
        case ContentType::GENERAL_DATA:
            return "General Data";
        case ContentType::TEXT:
            return "Text";
        case ContentType::IMAGE_JPEG:
            return "JPEG Image";
        case ContentType::IMAGE_PNG:
            return "PNG Image";
        case ContentType::IMAGE_BMP:
            return "BMP Image";
        case ContentType::HTML:
            return "HTML Document";
        case ContentType::AUDIO_MPEG:
            return "MPEG Audio";
        case ContentType::VIDEO_MPEG:
            return "MPEG Video";
        default:
            return QString("Unknown (0x%1)").arg(static_cast<uint8_t>(content_type), 2, 16, QChar('0'));
    }
}

QString MOTHeader::toString() const {
    return QString("MOTHeader[body=%1 bytes, header=%2 bytes, type=%3, subtype=%4, name='%5']")
        .arg(body_size)
        .arg(header_size)
        .arg(getContentTypeString())
        .arg(content_subtype)
        .arg(content_name);
}

QString MOTDirectoryEntry::toString() const {
    return QString("DirectoryEntry[TransportID=%1, header=%2, body=%3, rep=%4]")
        .arg(transport_id)
        .arg(header_size)
        .arg(body_size)
        .arg(repetition_count);
}

QString MOTDirectory::toString() const {
    return QString("MOTDirectory[size=%1, objects=%2, carousel=%3ms, segment_size=%4]")
        .arg(directory_size)
        .arg(num_objects)
        .arg(carousel_period_ms)
        .arg(segment_size);
}

QString MOTObject::toString() const {
    return QString("MOTObject[TransportID=%1, %2, body=%3 bytes, received=%4]")
        .arg(transport_id)
        .arg(header.getContentTypeString())
        .arg(body.size())
        .arg(received_time.toString("yyyy-MM-dd hh:mm:ss"));
}

QString MSCDataGroup::toString() const {
    return QString("DataGroup[type=%1, CI=%2, RI=%3, CRC=%4, data=%5 bytes]")
        .arg(data_group_type)
        .arg(continuity_index)
        .arg(repetition_index)
        .arg(crc_valid ? "valid" : "invalid")
        .arg(data_field.size());
}

// ============================================================================
// MOTProtocol Class Implementation
// ============================================================================

MOTProtocol::MOTProtocol(QObject* parent)
    : QObject(parent)
{
    qInfo() << "MOTProtocol initialized - ETSI EN 301 234 / EN 300 401 standard layout";
}

MOTProtocol::~MOTProtocol() {
    clearAll();
}

// ============================================================================
// Reassembly limits
// ============================================================================

void MOTProtocol::setLimits(const Limits& limits) {
    QMutexLocker locker(&m_mutex);
    m_limits = limits;
}

void MOTProtocol::setProgressCallback(std::function<void(uint32_t, double)> cb) {
    QMutexLocker locker(&m_mutex);
    m_progressCallback = std::move(cb);
}

// ============================================================================
// Main Processing Interface
// ============================================================================

bool MOTProtocol::processMOTData(uint32_t service_id, const QByteArray& msc_data) {
    QMutexLocker locker(&m_mutex);

    ++m_tick;  // monotonic data-group counter (staleness/LRU ordering)

    if (msc_data.isEmpty()) {
        ++m_statistics.rejected;
        return false;
    }

    // --- Data group header + CRC -------------------------------------------
    auto data_group_opt = parseMSCDataGroup(msc_data);
    if (!data_group_opt) {
        ++m_statistics.rejected;
        return false;
    }
    MSCDataGroup& data_group = *data_group_opt;
    m_statistics.data_groups_processed++;

    // MOT is carried in data group types 3/4 with CRC, segmentation and user
    // access set (ETSI TS 101 499 §4.2); anything else is not ours.
    if (!data_group.crc_flag || !data_group.segment_flag ||
        !data_group.user_access_flag ||
        (data_group.data_group_type != 3 && data_group.data_group_type != 4)) {
        ++m_statistics.rejected;
        return false;
    }

    // F8: parseMSCDataGroup() already validated the CRC (one's complement of
    // CRC-16/CCITT-FALSE); reuse its result instead of recomputing.
    if (!data_group.crc_valid) {
        ++m_statistics.crc_errors;
        return false;
    }
    m_statistics.data_groups_valid++;

    // --- MOT session header + segmentation header --------------------------
    int offset = data_group.extension_flag ? 4 : 2;

    bool lastSeg = false;
    uint16_t segNumber = 0;
    uint32_t transportId = 0;
    if (!parseSessionHeader(msc_data, offset, lastSeg, segNumber, transportId)) {
        ++m_statistics.rejected;
        return false;
    }
    if (!validateTransportId(transportId)) {
        ++m_statistics.rejected;
        emit parseError(transportId, QString("Invalid transport ID: %1 (must be 0-65535)")
                                        .arg(transportId));
        return false;
    }

    int segSize = 0;
    int repetitionCount = 0;
    if (!parseSegmentationHeader(msc_data, offset, segSize, repetitionCount)) {
        ++m_statistics.rejected;
        return false;
    }
    Q_UNUSED(repetitionCount);  // RepetitionCount: parsed but ignored (F7).

    const int dataOffset = offset;
    const int expectedRemainder = msc_data.size() - dataOffset - 2;  // minus CRC
    if (segSize < 0 || segSize != expectedRemainder) {
        ++m_statistics.rejected;
        return false;
    }

    const QByteArray payload = msc_data.mid(dataOffset, segSize);
    const bool isHeader = (data_group.data_group_type == 3);

    // --- Assembly lookup / bounded housekeeping ----------------------------
    Assembly& assembly = [&]() -> Assembly& {
        auto it = m_assemblies.find(transportId);
        if (it != m_assemblies.end()) {
            return it->second;
        }
        evictStaleAndCap();  // bounded map before adding a new key
        return m_assemblies[transportId];
    }();
    assembly.lastActivityTick = m_tick;

    // Carousel version reset: a repeated HEADER segment 0 whose bytes differ
    // from the buffered one starts a new object version, so discard the stale
    // partial assembly (two versions cannot merge). A byte-identical repeat is
    // just a retransmit; body-seg 0 repeats do NOT reset.
    if (isHeader && segNumber == 0) {
        const auto it0 = assembly.headerSegs.find(0);
        if (it0 != assembly.headerSegs.end() && it0->second != payload) {
            assembly = Assembly{};
            assembly.lastActivityTick = m_tick;
            ++m_statistics.object_resets;
        }
    }

    std::map<uint16_t, QByteArray>& segs = isHeader ? assembly.headerSegs
                                                    : assembly.bodySegs;
    int& last = isHeader ? assembly.lastHeaderSeg : assembly.lastBodySeg;

    if (segs.size() >= static_cast<size_t>(m_limits.maxSegments)) {
        ++m_statistics.rejected;
        ++m_statistics.oversized;
        m_assemblies.erase(transportId);  // corrupted/oversized object -> drop
        return false;
    }

    if (segs.find(segNumber) == segs.end()) {
        // Running size bound: reject immediately instead of buffering up to
        // maxSegments × segment size.
        if (assembly.headerBytes + assembly.bodyBytes +
                static_cast<size_t>(payload.size()) > m_limits.maxBodyBytes) {
            ++m_statistics.rejected;
            ++m_statistics.oversized;
            m_assemblies.erase(transportId);
            return false;
        }
        segs.emplace(segNumber, payload);
        if (isHeader) {
            assembly.headerBytes += static_cast<size_t>(payload.size());
        } else {
            assembly.bodyBytes += static_cast<size_t>(payload.size());
        }
        m_statistics.bytes_received += static_cast<uint64_t>(payload.size());
        ++m_statistics.segments_received;
    }
    if (lastSeg) {
        last = std::max(last, static_cast<int>(segNumber));
    }
    if (isHeader) {
        ++m_statistics.header_segments;
    } else {
        ++m_statistics.body_segments;
    }

    // Parse the standard header core (and its extension parameters) as soon as
    // the header entity completes, so the body size / content type are known.
    if (isHeader && !assembly.headerParsed &&
        entityComplete(assembly.headerSegs, assembly.lastHeaderSeg)) {
        const QByteArray headerBytes = concatEntity(assembly.headerSegs, assembly.lastHeaderSeg);
        std::vector<uint8_t> header(headerBytes.begin(), headerBytes.end());
        MOTHeader parsed;
        if (parseMOTHeader(header, parsed)) {
            assembly.header = parsed;
            assembly.headerParsed = true;
        }
    }

    // Live progress for the UI: announced body size vs. body bytes buffered.
    if (!isHeader && assembly.headerParsed && assembly.header.body_size > 0) {
        const double fraction = std::min(1.0, static_cast<double>(assembly.bodyBytes) /
                                                  static_cast<double>(assembly.header.body_size));
        emit reassemblyProgress(transportId, fraction);
        if (m_progressCallback) {
            m_progressCallback(transportId, fraction);
        }
    }

    // Full object (header + body) reassembled?
    if (assembly.headerParsed &&
        entityComplete(assembly.headerSegs, assembly.lastHeaderSeg) &&
        entityComplete(assembly.bodySegs, assembly.lastBodySeg) &&
        !assembly.bodySegs.empty()) {
        emitObject(service_id, transportId, assembly);
        m_assemblies.erase(transportId);  // bounded memory; carousel re-sends
    }

    return true;
}

// ============================================================================
// MSC Data Group Parsing (ETSI EN 300 401 Section 5.3.3)
// ============================================================================

std::optional<MSCDataGroup> MOTProtocol::parseMSCDataGroup(const QByteArray& msc_data) {
    if (msc_data.size() < 2) {
        return std::nullopt;
    }

    MSCDataGroup dg;

    // Data group header byte 0.
    const uint8_t byte0 = static_cast<uint8_t>(msc_data[0]);
    dg.extension_flag = (byte0 & 0x80) != 0;
    dg.crc_flag = (byte0 & 0x40) != 0;
    dg.segment_flag = (byte0 & 0x20) != 0;
    dg.user_access_flag = (byte0 & 0x10) != 0;
    dg.data_group_type = byte0 & 0x0F;

    // Data group header byte 1: continuity index (4) | repetition index (4).
    const uint8_t byte1 = static_cast<uint8_t>(msc_data[1]);
    dg.continuity_index = (byte1 >> 4) & 0x0F;
    dg.repetition_index = byte1 & 0x0F;

    int data_start = 2;
    if (dg.extension_flag) {
        if (msc_data.size() < 4) {
            return std::nullopt;
        }
        dg.extension_data = {static_cast<uint8_t>(msc_data[2]),
                             static_cast<uint8_t>(msc_data[3])};
        data_start = 4;
    }

    int data_end = msc_data.size();
    if (dg.crc_flag) {
        if (msc_data.size() < data_start + 2) {
            return std::nullopt;
        }
        data_end -= 2;
        dg.crc_received = static_cast<uint16_t>(
            (static_cast<uint8_t>(msc_data[data_end]) << 8) |
            static_cast<uint8_t>(msc_data[data_end + 1]));
    }

    dg.data_field.assign(
        reinterpret_cast<const uint8_t*>(msc_data.constData()) + data_start,
        reinterpret_cast<const uint8_t*>(msc_data.constData()) + data_end);
    dg.crc_valid = validateDataGroupCRC(dg);

    return dg;
}

bool MOTProtocol::validateDataGroupCRC(const MSCDataGroup& data_group) {
    if (!data_group.crc_flag) {
        return true;
    }

    // Reconstruct the exact bytes the CRC covers: header, optional extension,
    // then the data field.
    std::vector<uint8_t> crc_data;
    crc_data.reserve(4 + data_group.extension_data.size() + data_group.data_field.size());

    const uint8_t byte0 = static_cast<uint8_t>((data_group.extension_flag ? 0x80 : 0) |
                                               (data_group.crc_flag ? 0x40 : 0) |
                                               (data_group.segment_flag ? 0x20 : 0) |
                                               (data_group.user_access_flag ? 0x10 : 0) |
                                               (data_group.data_group_type & 0x0F));
    const uint8_t byte1 = static_cast<uint8_t>(((data_group.continuity_index & 0x0F) << 4) |
                                               (data_group.repetition_index & 0x0F));
    crc_data.push_back(byte0);
    crc_data.push_back(byte1);
    crc_data.insert(crc_data.end(),
                    data_group.extension_data.begin(),
                    data_group.extension_data.end());
    crc_data.insert(crc_data.end(),
                    data_group.data_field.begin(),
                    data_group.data_field.end());

    // ETSI EN 300 401 §5.3.3.2: the transmitted value is the one's complement
    // of CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection).
    const uint16_t calculated =
        static_cast<uint16_t>(~eti::crc16ccitt_false(crc_data.data(), crc_data.size()));
    return calculated == data_group.crc_received;
}

// ============================================================================
// MOT Header/Directory Parsing (ETSI EN 301 234 §6; MOT directory)
// ============================================================================

bool MOTProtocol::parseHeaderCore(const uint8_t* data, size_t size,
                                  uint32_t& bodySize, uint16_t& headerSize,
                                  uint8_t& contentType, uint16_t& contentSubType) {
    if (data == nullptr || size < 7) {
        return false;
    }
    // ETSI EN 301 234 §6.1 header core (56 bits):
    //   body size(28) | header size(13) | content type(6) | content subtype(9)
    bodySize = (static_cast<uint32_t>(data[0]) << 20) |
               (static_cast<uint32_t>(data[1]) << 12) |
               (static_cast<uint32_t>(data[2]) << 4) |
               (static_cast<uint32_t>(data[3]) >> 4);
    headerSize = static_cast<uint16_t>(((data[3] & 0x0F) << 9) |
                                       (static_cast<uint16_t>(data[4]) << 1) |
                                       (data[5] >> 7));
    contentType = static_cast<uint8_t>((data[5] & 0x7F) >> 1);
    contentSubType = static_cast<uint16_t>(((data[5] & 0x01) << 8) | data[6]);
    return true;
}

bool MOTProtocol::parseMOTHeader(const std::vector<uint8_t>& data, MOTHeader& header) {
    uint32_t bodySize = 0;
    uint16_t headerSize = 0;
    uint8_t contentType = 0;
    uint16_t contentSubType = 0;
    if (!parseHeaderCore(data.data(), data.size(), bodySize, headerSize,
                         contentType, contentSubType)) {
        qDebug() << "MOT header data too short";
        return false;
    }

    header.body_size = bodySize;
    header.header_size = headerSize;
    header.content_type = static_cast<ContentType>(contentType);
    header.content_subtype = contentSubType;

    // Header extension parameters live in [7, header_size). When the declared
    // header_size is 0 or exceeds the buffer, fall back to the whole remaining
    // buffer (tolerant of a truncated/odd producer).
    if (data.size() > 7) {
        size_t param_end = data.size();
        if (header.header_size >= 7 && header.header_size <= data.size()) {
            param_end = header.header_size;
        }
        if (param_end > 7) {
            if (!parseHeaderParameters(data, 7, param_end - 7, header)) {
                qDebug() << "Failed to parse MOT header parameters";
                // Continue anyway - header structure is valid.
            }
        }
    }

    qDebug() << "Parsed MOT header:" << header.toString();
    return true;
}

bool MOTProtocol::parseHeaderParameters(const std::vector<uint8_t>& data,
                                       size_t offset,
                                       size_t length,
                                       MOTHeader& header) {
    if (data.empty()) {
        qDebug() << "MOT: Empty data buffer for header parameter parsing";
        return false;
    }

    if (offset >= data.size()) {
        qDebug() << "MOT: Offset beyond data buffer"
                 << "offset:" << offset << "size:" << data.size();
        return false;
    }

    if (offset + length > data.size()) {
        qDebug() << "MOT: Parameter length exceeds data buffer"
                 << "offset:" << offset << "length:" << length
                 << "data_size:" << data.size();
        return false;
    }

    size_t pos = offset;
    size_t end = offset + length;

    // EN 301 234 §6.2 Fig.22: each header extension parameter is coded as
    //   ParameterLengthIndicator (2 bits) | ParameterType (6 bits) |
    //   DataFieldLength | DataField.
    // PLI semantics (EN 301 234 §6.2.2, Fig.22):
    //   00 -> DataField length 0 (no length field)
    //   01 -> 8-bit DataFieldLength follows
    //   10 -> 32-bit DataFieldLength follows
    //   11 -> 7-/15-bit DataFieldLength indicator follows: if the indicator's
    //         MSB is set it is 15-bit ((ind & 0x7F) << 8 | next); otherwise
    //         7-bit (ind & 0x7F).
    while (pos < end) {
        // PLI=00 is a valid 1-byte parameter, so the loop guard must allow the
        // parameter header to be the last byte.
        const uint8_t pli_and_id = data[pos++];
        const uint8_t pli = (pli_and_id >> 6) & 0x03;
        const uint8_t param_id = pli_and_id & 0x3F;

        size_t param_data_length = 0;
        switch (pli) {
            case 0x0:  // DataField length 0
                param_data_length = 0;
                break;
            case 0x1:  // 8-bit DataFieldLength
                if (pos >= end) return false;
                param_data_length = data[pos++];
                break;
            case 0x2:  // 32-bit DataFieldLength
                if (pos + 3 >= end) return false;
                param_data_length = (static_cast<size_t>(data[pos]) << 24) |
                                    (static_cast<size_t>(data[pos + 1]) << 16) |
                                    (static_cast<size_t>(data[pos + 2]) << 8) |
                                    static_cast<size_t>(data[pos + 3]);
                pos += 4;
                break;
            case 0x3: {  // 7-bit / 15-bit DataFieldLength indicator (Ext)
                if (pos >= end) return false;
                const uint8_t ind = data[pos++];
                if (ind & 0x80) {
                    if (pos >= end) return false;
                    param_data_length = (static_cast<size_t>(ind & 0x7F) << 8) |
                                        static_cast<size_t>(data[pos++]);
                } else {
                    param_data_length = ind & 0x7F;
                }
                break;
            }
        }

        // The parameter data must stay within the header extension (header_size).
        if (param_data_length > end - pos) {
            qDebug() << "Parameter data exceeds header extension bounds";
            return false;
        }

        std::vector<uint8_t> param_data(data.begin() + pos,
                                        data.begin() + pos + param_data_length);
        pos += param_data_length;

        // Store parameter
        MOTHeader::HeaderParameter param;
        param.param_id = param_id;
        param.param_data = param_data;
        header.parameters[param_id] = param;

        // Extract common parameters (EN 301 234 §6.2.2.2 Table 5). Text
        // parameters carry a leading charset/Rfa byte (§6.2.2.1.1 Fig.24).
        switch (param_id) {
            case static_cast<uint8_t>(HeaderParameterId::CONTENT_NAME):
                header.content_name = decodeTextParameter(param_data);
                qDebug() << "MOT Content Name:" << header.content_name;
                break;

            case static_cast<uint8_t>(HeaderParameterId::TRIGGER_TIME):
                header.trigger_time = extractTriggerTime(param_data);
                qDebug() << "MOT Trigger Time:" << header.trigger_time;
                break;

            case static_cast<uint8_t>(HeaderParameterId::CATEGORY_TITLE):
                header.category_title = decodeTextParameter(param_data);
                qDebug() << "MOT Category Title:" << header.category_title;
                break;

            case static_cast<uint8_t>(HeaderParameterId::CLICK_THROUGH_URL):
                header.click_through_url = decodeTextParameter(param_data);
                qDebug() << "MOT Click-through URL:" << header.click_through_url;
                break;

            case static_cast<uint8_t>(HeaderParameterId::COMPRESSION_TYPE):
                if (!param_data.empty()) {
                    header.compression_type = param_data[0];
                }
                break;

            case static_cast<uint8_t>(HeaderParameterId::UNIQUE_BODY_VERSION):
                if (param_data.size() >= 2) {
                    header.unique_body_version =
                        static_cast<uint16_t>((param_data[0] << 8) | param_data[1]);
                }
                break;
        }
    }

    return true;
}

bool MOTProtocol::parseMOTDirectory(const std::vector<uint8_t>& data, MOTDirectory& directory) {
    if (data.size() < 6) {
        qDebug() << "MOT directory data too short";
        return false;
    }

    // ETSI EN 301 234: MOT directory structure Structure
    size_t pos = 0;

    // Directory size (13 bits)
    directory.directory_size = static_cast<uint16_t>(((data[pos] & 0x1F) << 8) | data[pos + 1]);
    pos += 2;

    // Number of objects (16 bits)
    directory.num_objects = static_cast<uint16_t>((data[pos] << 8) | data[pos + 1]);
    pos += 2;

    // Carousel period (24 bits) in milliseconds
    if (pos + 3 <= data.size()) {
        directory.carousel_period_ms = (static_cast<uint32_t>(data[pos]) << 16) |
                                       (static_cast<uint32_t>(data[pos + 1]) << 8) |
                                       static_cast<uint32_t>(data[pos + 2]);
        pos += 3;
    }

    // Segment size (13 bits)
    if (pos + 2 <= data.size()) {
        directory.segment_size = static_cast<uint16_t>(((data[pos] & 0x1F) << 8) | data[pos + 1]);
        pos += 2;
    }

    // Parse directory entries
    for (uint16_t i = 0; i < directory.num_objects && pos + 8 <= data.size(); ++i) {
        MOTDirectoryEntry entry;

        // Transport ID (16 bits typically, can be 32-bit in extended mode)
        entry.transport_id = static_cast<uint32_t>((data[pos] << 8) | data[pos + 1]);
        pos += 2;

        // Header size (13 bits)
        entry.header_size = static_cast<uint16_t>(((data[pos] & 0x1F) << 8) | data[pos + 1]);
        pos += 2;

        // Body size (28 bits)
        entry.body_size = (static_cast<uint32_t>(data[pos]) << 20) |
                          (static_cast<uint32_t>(data[pos + 1]) << 12) |
                          (static_cast<uint32_t>(data[pos + 2]) << 4) |
                          (static_cast<uint32_t>(data[pos + 3]) >> 4);
        pos += 4;

        // Repetition count (8 bits)
        entry.repetition_count = data[pos++];

        directory.entries.push_back(entry);
        qDebug() << "Directory entry:" << entry.toString();
    }

    // CRC validation (last 2 bytes if present)
    if (pos + 2 <= data.size()) {
        directory.crc_received = static_cast<uint16_t>((data[pos] << 8) | data[pos + 1]);

        // ~CRC-16/CCITT-FALSE over the directory data.
        const uint16_t raw = eti::crc16ccitt_false(data.data(), pos);
        directory.crc_calculated = static_cast<uint16_t>(~raw);
        directory.has_valid_crc = (directory.crc_calculated == directory.crc_received);
    }

    qInfo() << "Parsed MOT directory:" << directory.toString();
    return true;
}

// ============================================================================
// Segment/entity reassembly
// ============================================================================

bool MOTProtocol::entityComplete(const std::map<uint16_t, QByteArray>& segs, int lastSeg) {
    if (lastSeg < 0) {
        return false;
    }
    for (int i = 0; i <= lastSeg; ++i) {
        if (segs.find(static_cast<uint16_t>(i)) == segs.end()) {
            return false;
        }
    }
    return !segs.empty();
}

QByteArray MOTProtocol::concatEntity(const std::map<uint16_t, QByteArray>& segs, int lastSeg) {
    QByteArray result;
    for (int i = 0; i <= lastSeg; ++i) {
        const auto it = segs.find(static_cast<uint16_t>(i));
        if (it == segs.end()) {
            break;
        }
        result.append(it->second);
    }
    return result;
}

void MOTProtocol::evictStaleAndCap() {
    // Drop incomplete assemblies with no activity for staleTicks data groups.
    for (auto it = m_assemblies.begin(); it != m_assemblies.end();) {
        if (m_tick - it->second.lastActivityTick > m_limits.staleTicks) {
            it = m_assemblies.erase(it);
            ++m_statistics.stale_evictions;
        } else {
            ++it;
        }
    }

    // Cap the map: evict the least-recently-active assembly until there is room.
    while (m_assemblies.size() >= m_limits.maxAssemblies && !m_assemblies.empty()) {
        auto oldest = m_assemblies.begin();
        for (auto it = m_assemblies.begin(); it != m_assemblies.end(); ++it) {
            if (it->second.lastActivityTick < oldest->second.lastActivityTick) {
                oldest = it;
            }
        }
        m_assemblies.erase(oldest);
        ++m_statistics.stale_evictions;
    }
}

void MOTProtocol::emitObject(uint32_t serviceId, uint32_t transportId, Assembly& assembly) {
    Q_UNUSED(serviceId);

    const QByteArray bodyBytes = concatEntity(assembly.bodySegs, assembly.lastBodySeg);
    if (static_cast<uint32_t>(bodyBytes.size()) > m_limits.maxBodyBytes) {
        ++m_statistics.rejected;
        ++m_statistics.oversized;
        return;  // refuse oversized object
    }

    MOTObject object;
    object.transport_id = transportId;
    object.header = assembly.header;
    object.body.assign(bodyBytes.begin(), bodyBytes.end());
    object.received_time = QDateTime::currentDateTime();

    // The standard header body_size (28 bits) must equal the assembled body.
    if (static_cast<uint32_t>(object.body.size()) != object.header.body_size) {
        // Do not silently rewrite the standard header; surface it at debug level
        // (a genuine producer bug) but still emit the exact bytes.
        qDebug() << "MOT body size mismatch: declared" << object.header.body_size
                 << "actual" << object.body.size()
                 << "transport" << transportId;
    }

    if (static_cast<uint64_t>(object.body.size()) > m_statistics.max_object_bytes) {
        m_statistics.max_object_bytes = static_cast<uint64_t>(object.body.size());
    }

    ++m_statistics.objects_completed;
    ++m_statistics.objects_emitted;

    qInfo() << "MOT object complete:" << object.toString();
    emit objectComplete(transportId, object);
}

// ============================================================================
// Transport ID Management
// ============================================================================

double MOTProtocol::getProgress(uint32_t transport_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_assemblies.find(transport_id);
    if (it == m_assemblies.end()) {
        return -1.0;
    }
    if (!it->second.headerParsed || it->second.header.body_size == 0) {
        return 0.0;
    }
    return std::min(1.0, static_cast<double>(it->second.bodyBytes) /
                             static_cast<double>(it->second.header.body_size));
}

bool MOTProtocol::isComplete(uint32_t transport_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_assemblies.find(transport_id);
    if (it == m_assemblies.end()) {
        return false;
    }
    const Assembly& a = it->second;
    return a.headerParsed &&
           entityComplete(a.headerSegs, a.lastHeaderSeg) &&
           entityComplete(a.bodySegs, a.lastBodySeg) &&
           !a.bodySegs.empty();
}

std::vector<uint32_t> MOTProtocol::getActiveTransportIds() const {
    QMutexLocker locker(&m_mutex);

    std::vector<uint32_t> ids;
    ids.reserve(m_assemblies.size());

    for (const auto& pair : m_assemblies) {
        ids.push_back(pair.first);
    }

    return ids;
}

void MOTProtocol::clearObject(uint32_t transport_id) {
    QMutexLocker locker(&m_mutex);

    auto it = m_assemblies.find(transport_id);
    if (it != m_assemblies.end()) {
        qInfo() << "Clearing MOT object: TransportID=" << transport_id;
        m_assemblies.erase(it);
        emit objectCleared(transport_id);
    }
}

void MOTProtocol::clearAll() {
    QMutexLocker locker(&m_mutex);

    qInfo() << "Clearing all MOT objects:" << m_assemblies.size();
    m_assemblies.clear();
}

void MOTProtocol::resetStatistics() {
    QMutexLocker locker(&m_mutex);
    m_statistics = Statistics{};
}

// ============================================================================
// Statistics & Status
// ============================================================================

MOTProtocol::Statistics MOTProtocol::getStatistics() const {
    QMutexLocker locker(&m_mutex);

    Statistics stats = m_statistics;
    stats.active_objects = static_cast<uint32_t>(m_assemblies.size());

    return stats;
}

// ============================================================================
// Helper Methods
// ============================================================================

QString MOTProtocol::decodeTextParameter(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return QString();
    }

    // EN 301 234 §6.2.2.1.1 Fig.24: the first byte holds the 4-bit character
    // set indicator (high nibble) and 4-bit Rfa (low nibble); the character
    // field follows. Table 3: 0x0 = complete EBU Latin repertoire,
    // 0xF = UTF-8 (ISO/IEC 10646-1).
    const uint8_t charset = (data[0] >> 4) & 0x0F;

    // Strip the charset/Rfa byte; the rest is the character field.
    std::vector<uint8_t> text(data.begin() + 1, data.end());
    if (text.empty()) {
        return QString();
    }

    // Input sanitization: cap the character field (defend against a garbled
    // length even though header_size bounds it to 13 bits).
    constexpr size_t MAX_STRING_LENGTH = 4096;
    if (text.size() > MAX_STRING_LENGTH) {
        text.resize(MAX_STRING_LENGTH);
    }

    QString result;
    switch (charset) {
        case 0x0:
            result = QString::fromStdString(convert_ebu_to_utf8(
                std::string(reinterpret_cast<const char*>(text.data()), text.size())));
            break;
        case 0xF:
            result = QString::fromUtf8(reinterpret_cast<const char*>(text.data()),
                                       static_cast<int>(text.size()));
            break;
        default:
            // Other registered charsets (ISO-8859-1/-2, …) are not covered by
            // the project's EBU/TIS-620 helpers; decode as UTF-8 (the dominant
            // on-air encoding) with a byte-preserving fallback.
            result = QString::fromUtf8(reinterpret_cast<const char*>(text.data()),
                                       static_cast<int>(text.size()));
            break;
    }

    // Sanitize: drop NULs and control characters (except newline/tab).
    result.remove(QChar('\0'));
    for (int i = 0; i < result.length(); ++i) {
        const QChar ch = result.at(i);
        if (ch.unicode() < 32 && ch != '\n' && ch != '\r' && ch != '\t') {
            result.remove(i, 1);
            --i;
        }
    }
    return result;
}

uint32_t MOTProtocol::extractTriggerTime(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return 0;
    }

    // ETSI EN 301 234: Trigger time is MJD (Modified Julian Date) + UTC time.
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

} // namespace eti::mot
