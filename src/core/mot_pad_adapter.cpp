/**
 * @file mot_pad_adapter.cpp
 * @brief Implementation of the thin PAD → MOTProtocol façade (T26).
 *
 * No parsing, no re-framing: every standard EN 300 401 MSC data group is
 * forwarded to MOTProtocol::processMOTData() unchanged. The counters and the
 * progress callback are mapped onto MOTProtocol's state so the GUI report and
 * the existing tests keep their exact semantics.
 *
 * @author StreamDAB Development Team
 * @date October 2026
 */

#include "mot_pad_adapter.hpp"

namespace eti::mot {

MotPadAdapter::MotPadAdapter(MOTProtocol* mot)
    : m_mot(mot)
{
    if (m_mot) {
        m_mot->setLimits(MOTProtocol::Limits{
            m_limits.maxBodyBytes,
            m_limits.maxAssemblies,
            m_limits.staleTicks,
            static_cast<uint32_t>(MAX_SEGMENTS)});
    }
}

void MotPadAdapter::setMOTProtocol(MOTProtocol* mot)
{
    m_mot = mot;
    if (m_mot) {
        m_mot->setLimits(MOTProtocol::Limits{
            m_limits.maxBodyBytes,
            m_limits.maxAssemblies,
            m_limits.staleTicks,
            static_cast<uint32_t>(MAX_SEGMENTS)});
        m_mot->setProgressCallback(m_progressCallback);
    }
}

void MotPadAdapter::setLimits(const Limits& limits)
{
    m_limits = limits;
    if (m_mot) {
        m_mot->setLimits(MOTProtocol::Limits{
            m_limits.maxBodyBytes,
            m_limits.maxAssemblies,
            m_limits.staleTicks,
            static_cast<uint32_t>(MAX_SEGMENTS)});
    }
}

void MotPadAdapter::setProgressCallback(std::function<void(uint32_t, double)> cb)
{
    m_progressCallback = std::move(cb);
    if (m_mot) {
        m_mot->setProgressCallback(m_progressCallback);
    }
}

void MotPadAdapter::reset()
{
    if (m_mot) {
        m_mot->clearAll();
    }
}

bool MotPadAdapter::processDataGroup(uint32_t serviceId, const QByteArray& dataGroup)
{
    ++m_dataGroups;
    if (!m_mot) {
        return false;
    }
    return m_mot->processMOTData(serviceId, dataGroup);
}

MotPadAdapter::Statistics MotPadAdapter::statistics() const
{
    Statistics stats;
    stats.dataGroups = m_dataGroups;
    if (m_mot) {
        const MOTProtocol::Statistics p = m_mot->getStatistics();
        stats.crcErrors = p.crc_errors;
        stats.rejected = p.rejected;
        stats.headerSegments = p.header_segments;
        stats.bodySegments = p.body_segments;
        stats.objectsCompleted = p.objects_completed;
        stats.objectsEmitted = p.objects_emitted;
        stats.bytesReceived = p.bytes_received;
        stats.oversized = p.oversized;
        stats.staleEvictions = p.stale_evictions;
        stats.objResets = p.object_resets;
        stats.maxObjectBytes = p.max_object_bytes;
    }
    return stats;
}

bool MotPadAdapter::parseStandardHeader(const QByteArray& header, uint32_t& bodySize,
                                        uint16_t& headerSize, uint8_t& contentType,
                                        uint16_t& contentSubType)
{
    return MOTProtocol::parseHeaderCore(
        reinterpret_cast<const uint8_t*>(header.constData()),
        static_cast<size_t>(header.size()),
        bodySize, headerSize, contentType, contentSubType);
}

} // namespace eti::mot
