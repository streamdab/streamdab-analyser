// ==============================================================================
// Mock Broadcast Equipment - TDD Testing Support
// ==============================================================================
//
// Mock implementation for broadcast equipment integration testing
// Supports TDD workflow for network component validation
//
// Author: Network/Stream Agent - TDD Lead
// Date: 2025-09-26
// ==============================================================================

#include "mock_broadcast_equipment.h"
#include <QTimer>
#include <QDebug>

MockBroadcastEquipment::MockBroadcastEquipment(QObject *parent)
    : QObject(parent)
    , m_connected(false)
    , m_streaming(false)
    , m_frameCount(0)
    , m_mockTimer(new QTimer(this))
{
    // Setup mock timer for simulated streaming
    connect(m_mockTimer, &QTimer::timeout, this, &MockBroadcastEquipment::simulateFrame);
}

MockBroadcastEquipment::~MockBroadcastEquipment() = default;

bool MockBroadcastEquipment::connectToEquipment(const QString& address, int port)
{
    Q_UNUSED(address)
    Q_UNUSED(port)
    
    m_connected = true;
    emit connected();
    return true;
}

void MockBroadcastEquipment::disconnect()
{
    m_connected = false;
    m_streaming = false;
    m_mockTimer->stop();
    emit disconnected();
}

bool MockBroadcastEquipment::startStreaming()
{
    if (!m_connected) return false;
    
    m_streaming = true;
    m_frameCount = 0;
    m_mockTimer->start(24); // 24ms = ~41.67 fps DAB frame rate
    emit streamingStarted();
    return true;
}

void MockBroadcastEquipment::stopStreaming()
{
    m_streaming = false;
    m_mockTimer->stop();
    emit streamingStopped();
}

void MockBroadcastEquipment::simulateFrame()
{
    if (!m_streaming) return;
    
    // Create mock ETI frame data (6144 bytes)
    QByteArray mockFrame(6144, 0);
    
    // Add basic ETI frame structure
    mockFrame[0] = 0xFF; // Sync word start
    mockFrame[1] = 0x00;
    mockFrame[2] = 0xF0;
    mockFrame[3] = 0x96;
    
    // Add frame counter
    mockFrame[4] = (m_frameCount >> 8) & 0xFF;
    mockFrame[5] = m_frameCount & 0xFF;
    
    m_frameCount++;
    emit frameReceived(mockFrame);
}

bool MockBroadcastEquipment::isConnected() const
{
    return m_connected;
}

bool MockBroadcastEquipment::isStreaming() const
{
    return m_streaming;
}

quint64 MockBroadcastEquipment::getFrameCount() const
{
    return m_frameCount;
}