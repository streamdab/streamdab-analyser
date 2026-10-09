// ==============================================================================
// Mock Broadcast Equipment Header - TDD Testing Support
// ==============================================================================
//
// Mock implementation for broadcast equipment integration testing
// Supports TDD workflow for network component validation
//
// Author: Network/Stream Agent - TDD Lead
// Date: 2025-09-26
// ==============================================================================

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class MockBroadcastEquipment : public QObject
{
    Q_OBJECT

public:
    explicit MockBroadcastEquipment(QObject *parent = nullptr);
    ~MockBroadcastEquipment() override;

    // Connection management
    bool connectToEquipment(const QString& address, int port);
    void disconnect();
    bool isConnected() const;

    // Streaming control
    bool startStreaming();
    void stopStreaming();
    bool isStreaming() const;

    // Statistics
    quint64 getFrameCount() const;

signals:
    void connected();
    void disconnected();
    void streamingStarted();
    void streamingStopped();
    void frameReceived(const QByteArray& frame);

private slots:
    void simulateFrame();

private:
    bool m_connected;
    bool m_streaming;
    quint64 m_frameCount;
    QTimer* m_mockTimer;
};