// ==============================================================================
// Mock Broadcast Automation System Header - TDD Testing Support
// ==============================================================================

#pragma once

#include <QObject>
#include <QString>

class MockBroadcastAutomationSystem : public QObject
{
    Q_OBJECT

public:
    explicit MockBroadcastAutomationSystem(QObject *parent = nullptr);
    ~MockBroadcastAutomationSystem() override;

    bool initialize();
    void shutdown();
    bool isSystemReady() const;
    void processAutomationCommand(const QString& command);

signals:
    void systemInitialized();
    void systemShutdown();
    void commandProcessed(const QString& command);

private:
    bool m_systemStatus;
};