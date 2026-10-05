/**
 * @file mot_pad_adapter.cpp
 * @brief Implementation of the MOT MSC data group -> MOTProtocol adapter.
 *
 * @author StreamDAB Development Team
 * @date October 2026
 */

#include "mot_pad_adapter.hpp"

#include <algorithm>

#include "crc16.hpp"

namespace eti::mot {

namespace {
// Fragment bounds / sanity limits.
constexpr int kMaxTransportIdLength = 4;  // 0/2/4 bytes per EN 300 401
}

MotPadAdapter::MotPadAdapter(MOTProtocol* mot)
    : m_mot(mot)
{
}

void MotPadAdapter::reset()
{
    m_assemblies.clear();
    m_tick = 0;
}

void MotPadAdapter::evictStaleAndCap()
{
    // Housekeeping: drop incomplete assemblies that have seen no activity for
    // staleTicks data groups.
    for (auto it = m_assemblies.begin(); it != m_assemblies.end();) {
        if (m_tick - it->second.lastActivityTick > m_limits.staleTicks) {
            it = m_assemblies.erase(it);
            ++m_stats.staleEvictions;
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
        ++m_stats.staleEvictions;
    }
}

bool MotPadAdapter::parseStandardHeader(const QByteArray& header, uint32_t& bodySize,
                                        uint16_t& headerSize, uint8_t& contentType,
                                        uint16_t& contentSubType)
{
    if (header.size() < 7) {
        return false;
    }
    const uint8_t* d = reinterpret_cast<const uint8_t*>(header.constData());
    // ETSI EN 301 234 §5.1 header core: body size(28) header size(13)
    // content type(6) content subtype(9).
    bodySize = (static_cast<uint32_t>(d[0]) << 20) |
               (static_cast<uint32_t>(d[1]) << 12) |
               (static_cast<uint32_t>(d[2]) << 4) |
               (static_cast<uint32_t>(d[3]) >> 4);
    headerSize = static_cast<uint16_t>(((d[3] & 0x0F) << 9) |
                                       (static_cast<uint16_t>(d[4]) << 1) |
                                       (d[5] >> 7));
    contentType = static_cast<uint8_t>((d[5] & 0x7F) >> 1);
    contentSubType = static_cast<uint16_t>(((d[5] & 0x01) << 8) | d[6]);
    return true;
}

bool MotPadAdapter::parseStandardDataGroupHeader(const QByteArray& dg, int& offset,
                                                 int& dgType, bool& crcFlag)
{
    if (dg.size() < 2) {
        return false;
    }
    const uint8_t* d = reinterpret_cast<const uint8_t*>(dg.constData());
    const bool extensionFlag = (d[0] & 0x80) != 0;
    crcFlag = (d[0] & 0x40) != 0;
    const bool segmentFlag = (d[0] & 0x20) != 0;
    const bool userAccessFlag = (d[0] & 0x10) != 0;
    dgType = d[0] & 0x0F;

    // MOT is only carried in data group types 3 (header) / 4 (body), always
    // with CRC, segmentation and user access (ETSI TS 101 499 §4.2).
    if (!crcFlag || !segmentFlag || !userAccessFlag) {
        return false;
    }
    if (dgType != 3 && dgType != 4) {
        return false;
    }

    offset = 2;
    if (extensionFlag) {
        if (dg.size() < offset + 2) {
            return false;
        }
        offset += 2;
    }
    return true;
}

bool MotPadAdapter::parseSessionHeader(const QByteArray& dg, int& offset, bool& lastSeg,
                                       int& segNumber, uint32_t& transportId)
{
    if (dg.size() < offset + 3) {
        return false;
    }
    const uint8_t* d = reinterpret_cast<const uint8_t*>(dg.constData());
    lastSeg = (d[offset] & 0x80) != 0;
    segNumber = ((d[offset] & 0x7F) << 8) | d[offset + 1];
    const bool transportIdFlag = (d[offset + 2] & 0x10) != 0;
    const int lenIndicator = d[offset + 2] & 0x0F;
    offset += 3;

    if (!transportIdFlag || lenIndicator < 2 || lenIndicator > kMaxTransportIdLength) {
        return false;
    }
    if (dg.size() < offset + lenIndicator) {
        return false;
    }
    // Transport id is at most 4 bytes; the MOT transport id is a 16-bit field.
    transportId = 0;
    for (int i = 0; i < lenIndicator; ++i) {
        transportId = (transportId << 8) | static_cast<uint8_t>(dg[offset + i]);
    }
    offset += lenIndicator;
    return true;
}

bool MotPadAdapter::parseSegmentationHeader(const QByteArray& dg, int& offset, int& segSize)
{
    if (dg.size() < offset + 2) {
        return false;
    }
    const uint8_t* d = reinterpret_cast<const uint8_t*>(dg.constData());
    segSize = ((d[offset] & 0x1F) << 8) | d[offset + 1];
    offset += 2;
    return true;
}

bool MotPadAdapter::processDataGroup(uint32_t serviceId, const QByteArray& dataGroup)
{
    ++m_stats.dataGroups;
    ++m_tick;
    if (dataGroup.size() < 2) {
        ++m_stats.rejected;
        return false;
    }

    int offset = 0;
    int dgType = 0;
    bool crcFlag = false;
    if (!parseStandardDataGroupHeader(dataGroup, offset, dgType, crcFlag)) {
        ++m_stats.rejected;
        return false;
    }

    // Validate the data-group CRC (~CRC-16/CCITT-FALSE, ETSI EN 300 401
    // §5.3.3.2). crcFlag is guaranteed true by the header check.
    if (crcFlag) {
        if (dataGroup.size() < 2) {
            ++m_stats.rejected;
            return false;
        }
        const uint16_t stored =
            (static_cast<uint8_t>(dataGroup[dataGroup.size() - 2]) << 8) |
            static_cast<uint8_t>(dataGroup[dataGroup.size() - 1]);
        const uint16_t calced = static_cast<uint16_t>(~eti::crc16ccitt_false(
            reinterpret_cast<const uint8_t*>(dataGroup.constData()),
            static_cast<size_t>(dataGroup.size() - 2)));
        if (stored != calced) {
            ++m_stats.crcErrors;
            return false;
        }
    }

    bool lastSeg = false;
    int segNumber = 0;
    uint32_t transportId = 0;
    if (!parseSessionHeader(dataGroup, offset, lastSeg, segNumber, transportId)) {
        ++m_stats.rejected;
        return false;
    }

    int segSize = 0;
    if (!parseSegmentationHeader(dataGroup, offset, segSize)) {
        ++m_stats.rejected;
        return false;
    }

    const int dataOffset = offset;
    const int expectedRemainder = dataGroup.size() - dataOffset - 2;  // minus CRC
    if (segSize < 0 || segSize != expectedRemainder) {
        ++m_stats.rejected;
        return false;
    }

    Assembly& assembly = [&]() -> Assembly& {
        auto it = m_assemblies.find(transportId);
        if (it != m_assemblies.end()) {
            return it->second;
        }
        evictStaleAndCap();  // bounded map before adding a new key
        return m_assemblies[transportId];
    }();
    assembly.lastActivityTick = m_tick;

    const bool isHeader = (dgType == 3);
    const QByteArray payload = dataGroup.mid(dataOffset, segSize);

    // Carousel version reset: a repeated HEADER segment 0 whose bytes differ
    // from the buffered one starts a new object version, so discard the stale
    // partial assembly (two versions cannot merge). A byte-identical repeat is
    // just a retransmit; body-seg 0 repeats do NOT reset (a long body may span
    // many header re-sends, and resetting on those lost real objects).
    if (isHeader && segNumber == 0) {
        const auto it0 = assembly.headerSegs.find(0);
        if (it0 != assembly.headerSegs.end() && it0->second != payload) {
            assembly = Assembly{};
            assembly.lastActivityTick = m_tick;
            ++m_stats.objResets;
        }
    }

    std::map<int, QByteArray>& segs = isHeader ? assembly.headerSegs
                                               : assembly.bodySegs;
    int& last = isHeader ? assembly.lastHeaderSeg : assembly.lastBodySeg;

    if (segs.size() >= static_cast<size_t>(MAX_SEGMENTS)) {
        ++m_stats.rejected;
        ++m_stats.oversized;
        m_assemblies.erase(transportId);  // corrupted/oversized object -> drop
        return false;
    }

    if (segs.find(segNumber) == segs.end()) {
        // Running size bound: reject immediately instead of buffering up to
        // MAX_SEGMENTS × segment size.
        if (assembly.headerBytes + assembly.bodyBytes +
                static_cast<size_t>(payload.size()) > m_limits.maxBodyBytes) {
            ++m_stats.rejected;
            ++m_stats.oversized;
            m_assemblies.erase(transportId);
            return false;
        }
        segs.emplace(segNumber, payload);
        if (isHeader) {
            assembly.headerBytes += static_cast<size_t>(payload.size());
        } else {
            assembly.bodyBytes += static_cast<size_t>(payload.size());
        }
        m_stats.bytesReceived += static_cast<uint64_t>(payload.size());
    }
    if (lastSeg) {
        last = std::max(last, segNumber);
    }
    if (isHeader) {
        ++m_stats.headerSegments;
    } else {
        ++m_stats.bodySegments;
    }

    // Parse the MOT header core as soon as the header entity completes so the
    // body size (for progress) and content type are known.
    if (isHeader && assembly.bodySize == 0 &&
        entityComplete(assembly.headerSegs, assembly.lastHeaderSeg)) {
        const QByteArray header = concatEntity(assembly.headerSegs, assembly.lastHeaderSeg);
        uint32_t bodySize = 0;
        uint16_t headerSize = 0;
        uint8_t contentType = 0;
        uint16_t contentSubType = 0;
        if (parseStandardHeader(header, bodySize, headerSize, contentType, contentSubType)) {
            assembly.bodySize = bodySize;
            assembly.contentType = contentType;
            assembly.contentSubType = contentSubType;
        }
    }

    // Live progress for the UI: announced body size vs. body bytes buffered
    // (running counter — O(1) per segment).
    if (!isHeader && m_onProgress && assembly.bodySize > 0) {
        const double fraction = std::min(1.0, static_cast<double>(assembly.bodyBytes) /
                                                  static_cast<double>(assembly.bodySize));
        m_onProgress(transportId, fraction);
    }

    // Full object (header + body) reassembled?
    if (entityComplete(assembly.headerSegs, assembly.lastHeaderSeg) &&
        entityComplete(assembly.bodySegs, assembly.lastBodySeg) &&
        !assembly.bodySegs.empty()) {
        emitObject(serviceId, transportId, assembly);
        m_assemblies.erase(transportId);  // bounded memory; carousel re-sends
    }

    return true;
}

bool MotPadAdapter::entityComplete(const std::map<int, QByteArray>& segs, int lastSeg)
{
    if (lastSeg < 0) {
        return false;
    }
    for (int i = 0; i <= lastSeg; ++i) {
        if (segs.find(i) == segs.end()) {
            return false;
        }
    }
    return !segs.empty();
}

QByteArray MotPadAdapter::concatEntity(const std::map<int, QByteArray>& segs, int lastSeg)
{
    QByteArray result;
    for (int i = 0; i <= lastSeg; ++i) {
        const auto it = segs.find(i);
        if (it == segs.end()) {
            break;
        }
        result.append(it->second);
    }
    return result;
}

QByteArray MotPadAdapter::buildReframedDataGroup(uint32_t transportId, uint8_t contentType,
                                                 const QByteArray& body)
{
    // FICTIONAL 7-byte MOT header core matching MOTProtocol::parseMOTHeader():
    //   body_size   = ((h[0] & 0x1F) << 8) | h[1]
    //   header_size = ((h[1] & 0xE0) >> 5) | (h[2] << 3)
    //   content_type= (h[3] >> 2) & 0x3F
    // With h = {0x01, 0xE0, 0x00, ct << 2, 0, 0, 0} this yields header_size 7
    // and the requested content type, so MOTProtocol extracts the real body
    // starting exactly at offset 7 (header core size).
    const uint8_t core[7] = {
        0x01,
        0xE0,
        0x00,
        static_cast<uint8_t>((contentType & 0x3F) << 2),
        0x00,
        0x00,
        0x00,
    };

    QByteArray dg;
    // MSC data group header: CRC + segment + type 3. The CRC is raw
    // (non-complemented) because MOTProtocol::validateDataGroupCRC() compares
    // against a non-complemented CRC-16/CCITT-FALSE.
    dg.append(static_cast<char>(0x40 | 0x20 | 0x03));
    dg.append(static_cast<char>(0x00));  // continuity / repetition index

    // MOTProtocol "MOT segment header": repetition(2) segment(2) transport(2)
    // last+total(2). total_segments = 1, single complete segment.
    dg.append(static_cast<char>(0x00));
    dg.append(static_cast<char>(0x00));
    dg.append(static_cast<char>(0x00));
    dg.append(static_cast<char>(0x00));
    dg.append(static_cast<char>((transportId >> 8) & 0xFF));
    dg.append(static_cast<char>(transportId & 0xFF));
    dg.append(static_cast<char>(0x80));  // last segment flag + total high nibble
    dg.append(static_cast<char>(0x01));  // total low byte = 1 (== core[0])

    // Data field [7..] is reassembled by MOTProtocol as core[0..6] + body:
    // core[0] is contributed by the total-segments low byte above.
    dg.append(reinterpret_cast<const char*>(core) + 1, 6);
    dg.append(body);

    const uint16_t crc = eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(dg.constData()),
        static_cast<size_t>(dg.size()));
    dg.append(static_cast<char>((crc >> 8) & 0xFF));
    dg.append(static_cast<char>(crc & 0xFF));
    return dg;
}

void MotPadAdapter::emitObject(uint32_t serviceId, uint32_t transportId, Assembly& assembly)
{
    ++m_stats.objectsCompleted;

    const QByteArray body = concatEntity(assembly.bodySegs, assembly.lastBodySeg);
    if (body.size() > static_cast<int>(m_limits.maxBodyBytes)) {
        ++m_stats.rejected;
        ++m_stats.oversized;
        return;  // refuse oversized object
    }
    if (static_cast<uint64_t>(body.size()) > m_stats.maxObjectBytes) {
        m_stats.maxObjectBytes = static_cast<uint64_t>(body.size());
    }

    // Keep the full 16-bit transport id end to end (MOTProtocol now accepts
    // 0..0xFFFF), so the UI/progress reports ONE consistent id.
    const QByteArray reframed =
        buildReframedDataGroup(transportId, assembly.contentType, body);
    if (m_mot) {
        m_mot->processMOTData(serviceId, reframed);
        ++m_stats.objectsEmitted;
    }
}

} // namespace eti::mot
