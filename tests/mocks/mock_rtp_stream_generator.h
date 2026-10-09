/**
 * @file mock_rtp_stream_generator.h
 * @brief Mock RTP stream generator for testing
 */

#pragma once

#include <QObject>
#include <QString>
#include <QHostAddress>

class QUdpSocket;
class QTimer;

class MockRtpStreamGenerator : public QObject
{
    Q_OBJECT

public:
    explicit MockRtpStreamGenerator(QObject* parent = nullptr);
    ~MockRtpStreamGenerator();

    bool start(const QString& address, quint16 port);
    void stop();
    bool isRunning() const;

signals:
    void packetSent(quint16 sequenceNumber, int packetSize);

private slots:
    void sendRtpPacket();

private:
    bool m_isRunning;
    quint16 m_sequenceNumber;
    quint32 m_timestamp;
    QHostAddress m_targetAddress;
    quint16 m_targetPort;
    
    QUdpSocket* m_socket;
    QTimer* m_timer;
};