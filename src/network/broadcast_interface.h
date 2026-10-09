#pragma once

#include "../core/eti_types.h"
#include <QObject>
#include <QString>
#include <QHostAddress>

/**
 * @file broadcast_interface.h
 * @brief Professional broadcast equipment integration interface
 * 
 * This interface provides standardized communication protocols for
 * professional broadcast equipment integration and monitoring.
 */

namespace network {

/**
 * @brief Professional broadcast equipment interface
 * 
 * Provides standardized interface for communicating with professional
 * broadcast equipment following industry standards.
 */
class BroadcastInterface : public QObject {
    Q_OBJECT

public:
    explicit BroadcastInterface(QObject* parent = nullptr);
    virtual ~BroadcastInterface() = default;

    /**
     * @brief Connect to broadcast equipment
     * @param address Equipment IP address
     * @param port Equipment control port
     * @return true if connection successful
     */
    virtual bool connectToEquipment(const QHostAddress& address, uint16_t port) = 0;

    /**
     * @brief Disconnect from broadcast equipment
     */
    virtual void disconnect() = 0;

    /**
     * @brief Check if connected to equipment
     * @return true if connected
     */
    virtual bool isConnected() const = 0;

signals:
    /**
     * @brief Emitted when connection status changes
     * @param connected true if connected
     */
    void connectionStatusChanged(bool connected);

    /**
     * @brief Emitted when equipment status updates
     * @param status Equipment status information
     */
    void equipmentStatusReceived(const QString& status);
};

} // namespace network