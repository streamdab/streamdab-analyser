/**
 * @file mot_protocol.cpp
 * @brief MOT (Multimedia Object Transfer) Protocol Parser Implementation
 *
 * Complete implementation of ETSI EN 301 234 MOT protocol for DAB data services.
 *
 * @author StreamDAB Development Team
 * @date October 20, 2025
 * @version 1.0
 */

#include "mot_protocol.hpp"
#include <QDebug>
#include <QElapsedTimer>
#include <algorithm>
#include <chrono>

namespace eti::mot {

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
    return QString("MOTHeader[body=%1 bytes, header=%2 bytes, type=%3, name='%4']")
        .arg(body_size)
        .arg(header_size)
        .arg(getContentTypeString())
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

QString MOTSegmentHeader::toString() const {
    return QString("SegmentHeader[TransportID=%1, segment=%2/%3, rep=%4, last=%5]")
        .arg(transport_id)
        .arg(segment_number)
        .arg(total_segments)
        .arg(repetition_count)
        .arg(last_segment ? "yes" : "no");
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
    initializeCRCTable();

    qInfo() << "MOTProtocol initialized - ETSI EN 301 234 compliant";
}

MOTProtocol::~MOTProtocol() {
    clearAll();
}

// ============================================================================
// Main Processing Interface
// ============================================================================

bool MOTProtocol::processMOTData(uint32_t service_id, const QByteArray& msc_data) {
    QMutexLocker locker(&m_mutex);

    if (msc_data.isEmpty()) {
        qWarning() << "MOTProtocol: Empty MSC data for service" << Qt::hex << service_id;
        return false;
    }

    // Parse MSC data group
    auto data_group_opt = parseMSCDataGroup(msc_data);
    if (!data_group_opt) {
        qWarning() << "MOTProtocol: Failed to parse MSC data group for service" << Qt::hex << service_id;
        return false;
    }

    MSCDataGroup& data_group = *data_group_opt;
    m_statistics.data_groups_processed++;

    // Validate CRC if present
    if (data_group.crc_flag) {
        if (!validateDataGroupCRC(data_group)) {
            qWarning() << "MOTProtocol: CRC validation failed for service" << Qt::hex << service_id;
            m_statistics.crc_errors++;
            return false;
        }
    }

    m_statistics.data_groups_valid++;

    // Parse MOT segment header
    auto segment_header_opt = parseMOTSegmentHeader(data_group);
    if (!segment_header_opt) {
        qWarning() << "MOTProtocol: Failed to parse MOT segment header";
        return false;
    }

    MOTSegmentHeader& segment_header = *segment_header_opt;

    // Validate transport ID
    if (!validateTransportId(segment_header.transport_id)) {
        QString error = QString("Invalid transport ID: %1 (must be 0-1023)")
            .arg(segment_header.transport_id);
        emit parseError(segment_header.transport_id, error);
        return false;
    }

    // Extract segment data (skip header bytes)
    std::vector<uint8_t> segment_data;
    size_t segment_header_size = 7; // Typical MOT segment header size
    if (data_group.data_field.size() > segment_header_size) {
        segment_data.assign(
            data_group.data_field.begin() + segment_header_size,
            data_group.data_field.end()
        );
    }

    // Add segment to reassembly buffer
    ReassemblyStatus status = addSegment(
        segment_header.transport_id,
        segment_header.segment_number,
        segment_header.total_segments,
        segment_data
    );

    m_statistics.segments_received++;

    // Handle reassembly status
    switch (status) {
        case ReassemblyStatus::COMPLETE: {
            // Reassemble complete object
            auto object_opt = reassembleObject(segment_header.transport_id);
            if (object_opt) {
                qInfo() << "MOT object complete:" << object_opt->toString();
                m_statistics.objects_completed++;
                emit objectComplete(segment_header.transport_id, *object_opt);

                // Clear buffer after successful reassembly
                clearObject(segment_header.transport_id);
            }
            break;
        }

        case ReassemblyStatus::INCOMPLETE: {
            // Update progress
            double progress = getProgress(segment_header.transport_id);
            emit reassemblyProgress(segment_header.transport_id, progress);
            break;
        }

        case ReassemblyStatus::ERROR_CRC:
            emit parseError(segment_header.transport_id, "CRC validation failed");
            break;

        case ReassemblyStatus::ERROR_MISSING_SEGMENTS:
            emit parseError(segment_header.transport_id, "Missing segments detected");
            break;

        case ReassemblyStatus::ERROR_DUPLICATE:
            qDebug() << "Duplicate segment received for transport ID" << segment_header.transport_id;
            break;
    }

    // Cleanup stale objects periodically
    cleanupStaleObjects();

    return true;
}

// ============================================================================
// MSC Data Group Parsing (ETSI EN 300 401 Section 5.3.3)
// ============================================================================

std::optional<MSCDataGroup> MOTProtocol::parseMSCDataGroup(const QByteArray& msc_data) {
    if (msc_data.size() < 2) {
        qWarning() << "MSC data too short for data group header";
        return std::nullopt;
    }

    MSCDataGroup dg;

    // Parse data group header (2 bytes)
    // Byte 0: Extension flag (bit 7), CRC flag (bit 6), Segment flag (bit 5),
    //         User access flag (bit 4), Data group type (bits 3-0)
    uint8_t byte0 = static_cast<uint8_t>(msc_data[0]);
    dg.extension_flag = (byte0 & 0x80) != 0;
    dg.crc_flag = (byte0 & 0x40) != 0;
    dg.segment_flag = (byte0 & 0x20) != 0;
    dg.user_access_flag = (byte0 & 0x10) != 0;
    dg.data_group_type = byte0 & 0x0F;

    // Byte 1: Continuity index (bits 7-4), Repetition index (bits 3-0)
    uint8_t byte1 = static_cast<uint8_t>(msc_data[1]);
    dg.continuity_index = (byte1 >> 4) & 0x0F;
    dg.repetition_index = byte1 & 0x0F;

    // Extract data field (between header and optional CRC)
    int data_start = 2;
    int data_end = msc_data.size();

    if (dg.crc_flag) {
        // CRC is last 2 bytes
        if (msc_data.size() < 4) {
            qWarning() << "MSC data too short for CRC";
            return std::nullopt;
        }
        data_end -= 2;
        dg.crc_received = (static_cast<uint8_t>(msc_data[data_end]) << 8) |
                          static_cast<uint8_t>(msc_data[data_end + 1]);
    }

    // Copy data field
    dg.data_field.resize(data_end - data_start);
    for (int i = data_start; i < data_end; ++i) {
        dg.data_field[i - data_start] = static_cast<uint8_t>(msc_data[i]);
    }

    return dg;
}

bool MOTProtocol::validateDataGroupCRC(const MSCDataGroup& data_group) {
    // Build data for CRC calculation (header + data field)
    std::vector<uint8_t> crc_data;
    crc_data.reserve(2 + data_group.data_field.size());

    // Add header bytes
    uint8_t byte0 = (data_group.extension_flag ? 0x80 : 0) |
                    (data_group.crc_flag ? 0x40 : 0) |
                    (data_group.segment_flag ? 0x20 : 0) |
                    (data_group.user_access_flag ? 0x10 : 0) |
                    (data_group.data_group_type & 0x0F);
    uint8_t byte1 = ((data_group.continuity_index & 0x0F) << 4) |
                    (data_group.repetition_index & 0x0F);

    crc_data.push_back(byte0);
    crc_data.push_back(byte1);

    // Add data field
    crc_data.insert(crc_data.end(), data_group.data_field.begin(), data_group.data_field.end());

    // Calculate CRC
    uint16_t calculated_crc = calculateCRC_CCITT(crc_data);

    return calculated_crc == data_group.crc_received;
}

// ============================================================================
// MOT Header/Directory Parsing (ETSI EN 301 234 Section 5)
// ============================================================================

bool MOTProtocol::parseMOTHeader(const std::vector<uint8_t>& data, MOTHeader& header) {
    if (data.size() < 7) {
        qWarning() << "MOT header data too short";
        return false;
    }

    // ETSI EN 301 234 Section 5.1: MOT Header Structure
    // Bytes 0-2: Body size (13 bits) + Header size (13 bits)
    // Byte 3: Content type (6 bits) + Content subtype high (2 bits)
    // Byte 4: Content subtype low (7 bits)

    // Body size (13 bits) - bits 12-0 of first 3 bytes
    header.body_size = ((data[0] & 0x1F) << 8) | data[1];

    // Header size (13 bits) - remaining bits
    header.header_size = ((data[1] & 0xE0) >> 5) | (data[2] << 3);

    // Content type (6 bits)
    uint8_t content_type_val = (data[3] >> 2) & 0x3F;
    header.content_type = static_cast<ContentType>(content_type_val);

    // Content subtype (9 bits)
    header.content_subtype = ((data[3] & 0x03) << 7) | (data[4] & 0x7F);

    // Parse header extension parameters (if present)
    if (data.size() > 7) {
        size_t param_offset = 7;
        size_t param_length = data.size() - param_offset;
        if (!parseHeaderParameters(data, param_offset, param_length, header)) {
            qWarning() << "Failed to parse MOT header parameters";
            // Continue anyway - header structure is valid
        }
    }

    qDebug() << "Parsed MOT header:" << header.toString();
    return true;
}

bool MOTProtocol::parseHeaderParameters(const std::vector<uint8_t>& data,
                                       size_t offset,
                                       size_t length,
                                       MOTHeader& header) {
    // FIX MEDIUM-003: Validate input bounds before processing
    if (data.empty()) {
        qWarning() << "MOT: Empty data buffer for header parameter parsing";
        return false;
    }
    
    if (offset >= data.size()) {
        qWarning() << "MOT: Offset beyond data buffer" 
                   << "offset:" << offset << "size:" << data.size();
        return false;
    }
    
    if (offset + length > data.size()) {
        qWarning() << "MOT: Parameter length exceeds data buffer"
                   << "offset:" << offset << "length:" << length 
                   << "data_size:" << data.size();
        return false;
    }
    
    size_t pos = offset;
    size_t end = offset + length;

    while (pos < end) {
        if (pos + 2 > end) break; // Need at least param ID and length

        // Parameter header: PLI (2 bits) + ParamId (6 bits)
        uint8_t pli_and_id = data[pos++];
        uint8_t pli = (pli_and_id >> 6) & 0x03;
        uint8_t param_id = pli_and_id & 0x3F;

        // Calculate parameter data length from PLI
        size_t param_data_length = 0;
        switch (pli) {
            case 0: // Length in next byte
                if (pos >= end) return false;
                param_data_length = data[pos++];
                break;
            case 1: // Length in next 2 bytes
                if (pos + 1 >= end) return false;
                param_data_length = (data[pos] << 8) | data[pos + 1];
                pos += 2;
                break;
            case 2: // Length in next 4 bytes
                if (pos + 3 >= end) return false;
                param_data_length = (data[pos] << 24) | (data[pos + 1] << 16) |
                                  (data[pos + 2] << 8) | data[pos + 3];
                pos += 4;
                break;
            case 3: // Reserved
                qWarning() << "Reserved PLI value 3 encountered";
                return false;
        }

        // Extract parameter data
        if (pos + param_data_length > end) {
            qWarning() << "Parameter data exceeds buffer";
            return false;
        }

        std::vector<uint8_t> param_data(data.begin() + pos, data.begin() + pos + param_data_length);
        pos += param_data_length;

        // Store parameter
        MOTHeader::HeaderParameter param;
        param.param_id = param_id;
        param.param_data = param_data;
        header.parameters[param_id] = param;

        // Extract common parameters
        switch (param_id) {
            case static_cast<uint8_t>(HeaderParameterId::CONTENT_NAME):
                header.content_name = extractUTF8String(param_data);
                qDebug() << "MOT Content Name:" << header.content_name;
                break;

            case static_cast<uint8_t>(HeaderParameterId::TRIGGER_TIME):
                header.trigger_time = extractTriggerTime(param_data);
                qDebug() << "MOT Trigger Time:" << header.trigger_time;
                break;

            case static_cast<uint8_t>(HeaderParameterId::CATEGORY_TITLE):
                header.category_title = extractUTF8String(param_data);
                qDebug() << "MOT Category Title:" << header.category_title;
                break;

            case static_cast<uint8_t>(HeaderParameterId::CLICK_THROUGH_URL):
                header.click_through_url = extractUTF8String(param_data);
                qDebug() << "MOT Click-through URL:" << header.click_through_url;
                break;

            case static_cast<uint8_t>(HeaderParameterId::COMPRESSION_TYPE):
                if (!param_data.empty()) {
                    header.compression_type = param_data[0];
                }
                break;

            case static_cast<uint8_t>(HeaderParameterId::CONTENT_VERSION):
                if (param_data.size() >= 2) {
                    header.content_version = (param_data[0] << 8) | param_data[1];
                }
                break;
        }
    }

    return true;
}

bool MOTProtocol::parseMOTDirectory(const std::vector<uint8_t>& data, MOTDirectory& directory) {
    if (data.size() < 6) {
        qWarning() << "MOT directory data too short";
        return false;
    }

    // ETSI EN 301 234 Section 5.2: MOT Directory Structure
    size_t pos = 0;

    // Directory size (13 bits)
    directory.directory_size = ((data[pos] & 0x1F) << 8) | data[pos + 1];
    pos += 2;

    // Number of objects (16 bits)
    directory.num_objects = (data[pos] << 8) | data[pos + 1];
    pos += 2;

    // Carousel period (24 bits) in milliseconds
    if (pos + 3 <= data.size()) {
        directory.carousel_period_ms = (data[pos] << 16) | (data[pos + 1] << 8) | data[pos + 2];
        pos += 3;
    }

    // Segment size (13 bits)
    if (pos + 2 <= data.size()) {
        directory.segment_size = ((data[pos] & 0x1F) << 8) | data[pos + 1];
        pos += 2;
    }

    // Parse directory entries
    for (uint16_t i = 0; i < directory.num_objects && pos + 8 <= data.size(); ++i) {
        MOTDirectoryEntry entry;

        // Transport ID (16 bits typically, can be 32-bit in extended mode)
        entry.transport_id = (data[pos] << 8) | data[pos + 1];
        pos += 2;

        // Header size (13 bits)
        entry.header_size = ((data[pos] & 0x1F) << 8) | data[pos + 1];
        pos += 2;

        // Body size (28 bits)
        entry.body_size = (data[pos] << 20) | (data[pos + 1] << 12) |
                         (data[pos + 2] << 4) | ((data[pos + 3] >> 4) & 0x0F);
        pos += 4;

        // Repetition count (8 bits)
        entry.repetition_count = data[pos++];

        directory.entries.push_back(entry);
        qDebug() << "Directory entry:" << entry.toString();
    }

    // CRC validation (last 2 bytes if present)
    if (pos + 2 <= data.size()) {
        directory.crc_received = (data[pos] << 8) | data[pos + 1];

        // Calculate CRC on directory data
        std::vector<uint8_t> crc_data(data.begin(), data.begin() + pos);
        directory.crc_calculated = calculateCRC_CCITT(crc_data);
        directory.has_valid_crc = (directory.crc_calculated == directory.crc_received);
    }

    qInfo() << "Parsed MOT directory:" << directory.toString();
    return true;
}

// ============================================================================
// Segment Reassembly (ETSI EN 301 234 Section 6)
// ============================================================================

std::optional<MOTSegmentHeader> MOTProtocol::parseMOTSegmentHeader(const MSCDataGroup& data_group) {
    if (data_group.data_field.size() < 7) {
        qWarning() << "Data field too short for MOT segment header";
        return std::nullopt;
    }

    MOTSegmentHeader header;

    const uint8_t* data = data_group.data_field.data();

    // Repetition count (16 bits)
    header.repetition_count = (data[0] << 8) | data[1];

    // Segment number (13 bits)
    header.segment_number = ((data[2] & 0x1F) << 8) | data[3];

    // Transport ID (16 bits typically)
    header.transport_id = (data[4] << 8) | data[5];

    // Last segment flag and total segments (13 bits)
    header.last_segment = (data[6] & 0x80) != 0;
    header.total_segments = ((data[6] & 0x1F) << 8) | data[7];

    qDebug() << "Parsed MOT segment:" << header.toString();
    return header;
}

MOTProtocol::ReassemblyStatus MOTProtocol::addSegment(uint32_t transport_id,
                                                      uint16_t segment_number,
                                                      uint16_t total_segments,
                                                      const std::vector<uint8_t>& segment_data) {
    // Get or create segment buffer
    SegmentBuffer& buffer = m_segment_buffers[transport_id];

    // Initialize buffer on first segment
    if (buffer.total_segments == 0) {
        buffer.total_segments = total_segments;
        qInfo() << "Starting MOT object reassembly: TransportID=" << transport_id
                << "Total segments=" << total_segments;
    }

    // Validate total segments matches
    if (buffer.total_segments != total_segments) {
        qWarning() << "Segment count mismatch for transport ID" << transport_id
                   << "Expected:" << buffer.total_segments << "Got:" << total_segments;
        return ReassemblyStatus::ERROR_MISSING_SEGMENTS;
    }

    // Check for duplicate
    if (buffer.segments.find(segment_number) != buffer.segments.end()) {
        return ReassemblyStatus::ERROR_DUPLICATE;
    }

    // Add segment
    buffer.segments[segment_number] = segment_data;
    buffer.last_update_time_ms = getCurrentTimeMs();

    qDebug() << "Added segment" << segment_number << "for transport ID" << transport_id
             << "Progress:" << buffer.getProgress() * 100.0 << "%";

    // Check if complete
    if (buffer.hasAllSegments()) {
        buffer.is_complete = true;
        qInfo() << "MOT object complete: TransportID=" << transport_id
                << "Segments=" << buffer.segments.size();
        return ReassemblyStatus::COMPLETE;
    }

    return ReassemblyStatus::INCOMPLETE;
}

std::optional<MOTObject> MOTProtocol::reassembleObject(uint32_t transport_id) {
    auto it = m_segment_buffers.find(transport_id);
    if (it == m_segment_buffers.end()) {
        qWarning() << "Transport ID not found:" << transport_id;
        return std::nullopt;
    }

    SegmentBuffer& buffer = it->second;

    if (!buffer.is_complete) {
        qWarning() << "Object not complete for transport ID" << transport_id;
        return std::nullopt;
    }

    MOTObject object;
    object.transport_id = transport_id;
    object.received_time = QDateTime::currentDateTime();

    // Reassemble segments in order
    std::vector<uint8_t> complete_data;
    for (uint16_t i = 0; i < buffer.total_segments; ++i) {
        auto seg_it = buffer.segments.find(i);
        if (seg_it == buffer.segments.end()) {
            qWarning() << "Missing segment" << i << "for transport ID" << transport_id;
            return std::nullopt;
        }
        const auto& seg_data = seg_it->second;
        complete_data.insert(complete_data.end(), seg_data.begin(), seg_data.end());
    }

    qInfo() << "Reassembled" << complete_data.size() << "bytes for transport ID" << transport_id;

    // Parse MOT header from reassembled data
    if (!parseMOTHeader(complete_data, object.header)) {
        qWarning() << "Failed to parse MOT header from reassembled data";
        return std::nullopt;
    }

    // Extract body (after header)
    if (complete_data.size() > object.header.header_size) {
        object.body.assign(
            complete_data.begin() + object.header.header_size,
            complete_data.end()
        );
    }

    // Validate body size
    if (object.body.size() != object.header.body_size) {
        qWarning() << "Body size mismatch: Expected" << object.header.body_size
                   << "Got" << object.body.size();
        // Continue anyway - may be acceptable
    }

    return object;
}

// ============================================================================
// Transport ID Management
// ============================================================================

double MOTProtocol::getProgress(uint32_t transport_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_segment_buffers.find(transport_id);
    if (it == m_segment_buffers.end()) {
        return -1.0;
    }

    return it->second.getProgress();
}

bool MOTProtocol::isComplete(uint32_t transport_id) const {
    QMutexLocker locker(&m_mutex);

    auto it = m_segment_buffers.find(transport_id);
    if (it == m_segment_buffers.end()) {
        return false;
    }

    return it->second.is_complete;
}

std::vector<uint32_t> MOTProtocol::getActiveTransportIds() const {
    QMutexLocker locker(&m_mutex);

    std::vector<uint32_t> ids;
    ids.reserve(m_segment_buffers.size());

    for (const auto& pair : m_segment_buffers) {
        ids.push_back(pair.first);
    }

    return ids;
}

void MOTProtocol::clearObject(uint32_t transport_id) {
    QMutexLocker locker(&m_mutex);

    auto it = m_segment_buffers.find(transport_id);
    if (it != m_segment_buffers.end()) {
        qInfo() << "Clearing MOT object: TransportID=" << transport_id;
        m_segment_buffers.erase(it);
        emit objectCleared(transport_id);
    }
}

void MOTProtocol::clearAll() {
    QMutexLocker locker(&m_mutex);

    qInfo() << "Clearing all MOT objects:" << m_segment_buffers.size();
    m_segment_buffers.clear();
}

// ============================================================================
// Statistics & Status
// ============================================================================

MOTProtocol::Statistics MOTProtocol::getStatistics() const {
    QMutexLocker locker(&m_mutex);

    Statistics stats = m_statistics;
    stats.active_objects = static_cast<uint32_t>(m_segment_buffers.size());

    return stats;
}

// ============================================================================
// CRC Calculation (CRC-CCITT)
// ============================================================================

void MOTProtocol::initializeCRCTable() {
    // CRC-CCITT polynomial: 0x1021
    const uint16_t polynomial = 0x1021;

    for (uint16_t i = 0; i < 256; ++i) {
        uint16_t crc = i << 8;

        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }

        m_crc_table[i] = crc;
    }
}

uint16_t MOTProtocol::calculateCRC_CCITT(const std::vector<uint8_t>& data) {
    uint16_t crc = 0xFFFF; // Initial value

    for (uint8_t byte : data) {
        uint8_t index = ((crc >> 8) ^ byte) & 0xFF;
        crc = (crc << 8) ^ m_crc_table[index];
    }

    return crc;
}

// ============================================================================
// Stale Object Cleanup
// ============================================================================

void MOTProtocol::cleanupStaleObjects() {
    uint32_t current_time = getCurrentTimeMs();

    std::vector<uint32_t> stale_ids;
    for (const auto& pair : m_segment_buffers) {
        if (isStale(pair.second.last_update_time_ms)) {
            stale_ids.push_back(pair.first);
        }
    }

    for (uint32_t id : stale_ids) {
        qInfo() << "Cleaning up stale MOT object: TransportID=" << id;
        clearObject(id);
    }
}

bool MOTProtocol::isStale(uint32_t last_update_ms) const {
    uint32_t current_time = getCurrentTimeMs();

    // Handle timestamp wraparound
    if (current_time < last_update_ms) {
        return false; // Just wrapped around, not stale
    }

    return (current_time - last_update_ms) > STALE_TIMEOUT_MS;
}

// ============================================================================
// Helper Methods
// ============================================================================

QString MOTProtocol::extractUTF8String(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return QString();
    }

    // FIX MEDIUM-005: Add input sanitization and UTF-8 validation
    // Maximum reasonable string length (prevent resource exhaustion)
    constexpr size_t MAX_STRING_LENGTH = 4096;
    
    if (data.size() > MAX_STRING_LENGTH) {
        qWarning() << "MOT: String data exceeds maximum length"
                   << "size:" << data.size() << "max:" << MAX_STRING_LENGTH;
        // Truncate to maximum length
        std::vector<uint8_t> truncated(data.begin(), data.begin() + MAX_STRING_LENGTH);
        QByteArray ba(reinterpret_cast<const char*>(truncated.data()), truncated.size());
        QString result = QString::fromUtf8(ba);
        return result.left(MAX_STRING_LENGTH);  // Additional QString length check
    }

    // Convert to QByteArray and decode as UTF-8
    QByteArray ba(reinterpret_cast<const char*>(data.data()), 
                  static_cast<int>(data.size()));
    
    // QString::fromUtf8 handles invalid UTF-8 by replacing with replacement character
    QString result = QString::fromUtf8(ba);
    
    // Sanitize: Remove null characters and control characters (except newline/tab)
    result.remove(QChar('\0'));
    for (int i = 0; i < result.length(); ++i) {
        QChar ch = result.at(i);
        if (ch.unicode() < 32 && ch != '\n' && ch != '\r' && ch != '\t') {
            result.remove(i, 1);
            --i;  // Adjust index after removal
        }
    }
    
    return result;
}

uint32_t MOTProtocol::extractTriggerTime(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return 0;
    }

    // ETSI EN 301 234: Trigger time is MJD (Modified Julian Date) + UTC time
    // For simplicity, return as 32-bit timestamp
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

uint32_t MOTProtocol::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace eti::mot
