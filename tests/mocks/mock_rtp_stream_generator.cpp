/**
 * @file mock_rtp_stream_generator.cpp
 * @brief Mock RTP stream generator for testing
 */

#include "mock_rtp_stream_generator.h"
#include <QTimer>
#include <QUdpSocket>

MockRtpStreamGenerator::MockRtpStreamGenerator(QObject* parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_sequenceNumber(0)
    , m_timestamp(0)
    , m_socket(nullptr)
    , m_timer(nullptr)
{
    m_socket = new QUdpSocket(this);
    m_timer = new QTimer(this);
    
    connect(m_timer, &QTimer::timeout, this, &MockRtpStreamGenerator::sendRtpPacket);
}

MockRtpStreamGenerator::~MockRtpStreamGenerator()
{
    stop();
}

bool MockRtpStreamGenerator::start(const QString& address, quint16 port)
{
    if (m_isRunning) {
        return false;
    }
    
    m_targetAddress = QHostAddress(address);
    m_targetPort = port;
    m_isRunning = true;
    
    // Send packets every 24ms (standard ETI frame rate)
    m_timer->start(24);
    
    return true;
}

void MockRtpStreamGenerator::stop()
{
    if (!m_isRunning) {
        return;
    }
    
    m_timer->stop();
    m_isRunning = false;
    m_sequenceNumber = 0;
    m_timestamp = 0;
}

void MockRtpStreamGenerator::sendRtpPacket()
{
    if (!m_isRunning) {
        return;
    }
    
    // Create mock ETI frame data (6144 bytes)
    QByteArray etiFrame(6144, 0x00);
    
    // Fill with simple test pattern
    for (int i = 0; i < etiFrame.size(); ++i) {
        etiFrame[i] = static_cast<char>(i % 256);
    }
    
    // Create simple RTP header (12 bytes)
    QByteArray rtpPacket;
    rtpPacket.resize(12 + etiFrame.size());
    
    // RTP header fields (simplified)
    rtpPacket[0] = 0x80;  // Version 2, no padding, no extension
    rtpPacket[1] = 0x60;  // No marker, payload type 96 (dynamic)
    
    // Sequence number (big endian)
    rtpPacket[2] = (m_sequenceNumber >> 8) & 0xFF;
    rtpPacket[3] = m_sequenceNumber & 0xFF;
    
    // Timestamp (big endian, 32-bit)
    rtpPacket[4] = (m_timestamp >> 24) & 0xFF;
    rtpPacket[5] = (m_timestamp >> 16) & 0xFF;
    rtpPacket[6] = (m_timestamp >> 8) & 0xFF;
    rtpPacket[7] = m_timestamp & 0xFF;
    
    // SSRC (static for testing)
    rtpPacket[8] = 0x12;
    rtpPacket[9] = 0x34;
    rtpPacket[10] = 0x56;
    rtpPacket[11] = 0x78;
    
    // Copy ETI frame data
    rtpPacket.replace(12, etiFrame.size(), etiFrame);
    
    // Send packet
    m_socket->writeDatagram(rtpPacket, m_targetAddress, m_targetPort);
    
    // Update counters
    m_sequenceNumber++;
    m_timestamp += 6144;  // ETI frame size
    
    emit packetSent(m_sequenceNumber, rtpPacket.size());
}

bool MockRtpStreamGenerator::isRunning() const
{
    return m_isRunning;
}