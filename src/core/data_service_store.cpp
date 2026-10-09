/**
 * @file data_service_store.cpp
 * @brief GUI-agnostic per-service data model implementation.
 *
 * See data_service_store.hpp. Passive aggregation only: decoding happens in
 * the EPG/Journaline/TPEG decoders; this store exposes the results.
 *
 * @author StreamDAB Development Team
 * @date October 2026
 */

#include "data_service_store.hpp"
#include <QDebug>

namespace eti::data {

DataServiceStore::DataServiceStore(QObject* parent)
    : QObject(parent)
{
}

void DataServiceStore::updateService(const ServiceDataSnapshot& snapshot)
{
    const uint32_t key = snapshot.sub_channel_id;
    m_bySubChannel[key] = snapshot;
    emit serviceUpdated(key);
}

bool DataServiceStore::getService(uint32_t sub_channel_id,
                                  ServiceDataSnapshot& out) const
{
    auto it = m_bySubChannel.find(sub_channel_id);
    if (it == m_bySubChannel.end()) {
        return false;
    }
    out = it.value();
    return true;
}

QVector<ServiceDataSnapshot> DataServiceStore::getServices() const
{
    QVector<ServiceDataSnapshot> services;
    services.reserve(m_bySubChannel.size());
    for (auto it = m_bySubChannel.cbegin(); it != m_bySubChannel.cend(); ++it) {
        services.append(it.value());
    }
    return services;
}

int DataServiceStore::serviceCount() const
{
    return m_bySubChannel.size();
}

void DataServiceStore::clear()
{
    if (m_bySubChannel.isEmpty()) {
        return;  // nothing changed — do not churn connected views
    }
    m_bySubChannel.clear();
    qDebug() << "[DataServiceStore] Cleared all service snapshots";
    emit servicesCleared();
}

} // namespace eti::data