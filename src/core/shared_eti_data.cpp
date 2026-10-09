// src/core/shared_eti_data.cpp
#include "shared_eti_data.hpp"
#include <QMutexLocker>
#include <QDebug>

namespace streamdab {
namespace core {

SharedETIData::SharedETIData(QObject *parent)
    : QObject(parent)
{
    // Register metatypes for signal/slot across threads
    qRegisterMetaType<ETIFrame>("ETIFrame");
    qRegisterMetaType<FICData>("FICData");
    qRegisterMetaType<ServiceInfo>("ServiceInfo");
    qRegisterMetaType<SubchannelConfig>("SubchannelConfig");
    qRegisterMetaType<MSCData>("MSCData");
    // NOTE (v1.3 crash fix): register under the fully-qualified name. The
    // reader thread (eti_reader_thread.cpp) also registers its own *different*
    // global `ErrorStats` under the bare name "ErrorStats"; the duplicate name
    // caused an undefined registration-order race where queued delivery copied
    // the wrong struct size (stack corruption / random SIGSEGV).
    qRegisterMetaType<ErrorStats>("streamdab::core::ErrorStats");
}

// ============================================================================
// Thread-safe setters (Pattern 5: emit signals AFTER mutex unlock)
// ============================================================================

void SharedETIData::setCurrentFrame(const ETIFrame& frame)
{
    uint64_t frame_number = 0;
    
    {
        QMutexLocker locker(&m_mutex);  // RAII lock
        m_current_frame = frame;
        m_last_update = QDateTime::currentDateTime();
        frame_number = m_current_frame.frame_number;
    } // Mutex unlocked here
    
    // Emit signal AFTER mutex unlock (prevents deadlock)
    emit frameUpdated(frame_number);
}

void SharedETIData::setFICData(const FICData& fic)
{
    uint16_t ensemble_id = 0;
    QString ensemble_label;
    
    {
        QMutexLocker locker(&m_mutex);
        
        bool ensemble_changed = (m_fic_data.ensemble_id != fic.ensemble_id);
        
        m_fic_data = fic;
        m_last_update = QDateTime::currentDateTime();
        
        ensemble_id = m_fic_data.ensemble_id;
        ensemble_label = m_fic_data.ensemble_label;
        
        // Store ensemble change flag
        if (ensemble_changed) {
            locker.unlock();
            emit ensembleInfoChanged(ensemble_id, ensemble_label);
            locker.relock();
        }
    }
    
    emit ficDataUpdated();
}

void SharedETIData::updateService(const ServiceInfo& service)
{
    int service_count = 0;
    uint32_t service_id = service.service_id;
    QString service_label = service.service_label;
    bool is_new_service = false;
    
    {
        QMutexLocker locker(&m_mutex);
        
        is_new_service = !m_services.contains(service_id);
        m_services[service_id] = service;
        service_count = m_services.size();
        m_last_update = QDateTime::currentDateTime();
    }
    
    if (is_new_service) {
        emit serviceAdded(service_id, service_label);
    }
    emit serviceListUpdated(service_count);
}

void SharedETIData::setSubchannelConfig(uint8_t subchannel_id, const SubchannelConfig& config)
{
    QMutexLocker locker(&m_mutex);
    m_subchannels[subchannel_id] = config;
    m_last_update = QDateTime::currentDateTime();
}

void SharedETIData::setMSCData(uint8_t subchannel_id, QSharedPointer<QByteArray> data)
{
    {
        QMutexLocker locker(&m_mutex);
        m_msc_data[subchannel_id] = data;  // Zero-copy with QSharedPointer
        m_last_update = QDateTime::currentDateTime();
    }
    
    emit mscDataUpdated(subchannel_id);
}

void SharedETIData::updateErrorStats(const ErrorStats& stats)
{
    {
        QMutexLocker locker(&m_mutex);
        m_error_stats = stats;
        m_last_update = QDateTime::currentDateTime();
    }
    
    emit errorStatsUpdated();
}

void SharedETIData::incrementErrorCount(const QString& error_type)
{
    {
        QMutexLocker locker(&m_mutex);
        
        if (error_type == "crc") {
            m_error_stats.crc_errors++;
        } else if (error_type == "sync") {
            m_error_stats.sync_errors++;
        } else if (error_type == "fic") {
            m_error_stats.fic_errors++;
        }
        
        m_last_update = QDateTime::currentDateTime();
    }
    
    emit errorStatsUpdated();
}

// ============================================================================
// Thread-safe getters (return copies with mutex lock)
// ============================================================================

ETIFrame SharedETIData::getCurrentFrame() const
{
    QMutexLocker locker(&m_mutex);
    return m_current_frame;  // Return copy for thread safety
}

FICData SharedETIData::getFICData() const
{
    QMutexLocker locker(&m_mutex);
    return m_fic_data;
}

QMap<uint32_t, ServiceInfo> SharedETIData::getServices() const
{
    QMutexLocker locker(&m_mutex);
    return m_services;
}

ServiceInfo SharedETIData::getService(uint32_t service_id) const
{
    QMutexLocker locker(&m_mutex);
    return m_services.value(service_id);
}

QMap<uint8_t, SubchannelConfig> SharedETIData::getSubchannels() const
{
    QMutexLocker locker(&m_mutex);
    return m_subchannels;
}

QSharedPointer<QByteArray> SharedETIData::getMSCData(uint8_t subchannel_id) const
{
    QMutexLocker locker(&m_mutex);
    return m_msc_data.value(subchannel_id);
}

ErrorStats SharedETIData::getErrorStats() const
{
    QMutexLocker locker(&m_mutex);
    return m_error_stats;
}

// ============================================================================
// Utility methods
// ============================================================================

void SharedETIData::clearAll()
{
    QMutexLocker locker(&m_mutex);
    
    m_current_frame = ETIFrame();
    m_fic_data = FICData();
    m_services.clear();
    m_subchannels.clear();
    m_msc_data.clear();
    m_error_stats = ErrorStats();
    m_last_update = QDateTime::currentDateTime();
}

void SharedETIData::clearServices()
{
    int service_count = 0;
    
    {
        QMutexLocker locker(&m_mutex);
        m_services.clear();
        m_last_update = QDateTime::currentDateTime();
        service_count = 0;
    }
    
    emit serviceListUpdated(service_count);
}

void SharedETIData::resetErrorStats()
{
    {
        QMutexLocker locker(&m_mutex);
        m_error_stats = ErrorStats();
        m_error_stats.last_reset = QDateTime::currentDateTime();
        m_last_update = QDateTime::currentDateTime();
    }
    
    emit errorStatsUpdated();
}

QDateTime SharedETIData::getLastUpdateTime() const
{
    QMutexLocker locker(&m_mutex);
    return m_last_update;
}

int SharedETIData::getServiceCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_services.size();
}

int SharedETIData::getSubchannelCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_subchannels.size();
}

} // namespace core
} // namespace streamdab
