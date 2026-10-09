// ==============================================================================
// Mock Broadcast Automation System - TDD Testing Support
// ==============================================================================

#include "mock_broadcast_automation_system.h"

MockBroadcastAutomationSystem::MockBroadcastAutomationSystem(QObject *parent)
    : QObject(parent)
    , m_systemStatus(false)
{
}

MockBroadcastAutomationSystem::~MockBroadcastAutomationSystem() = default;

bool MockBroadcastAutomationSystem::initialize()
{
    m_systemStatus = true;
    emit systemInitialized();
    return true;
}

void MockBroadcastAutomationSystem::shutdown()
{
    m_systemStatus = false;
    emit systemShutdown();
}

bool MockBroadcastAutomationSystem::isSystemReady() const
{
    return m_systemStatus;
}

void MockBroadcastAutomationSystem::processAutomationCommand(const QString& command)
{
    Q_UNUSED(command)
    emit commandProcessed(command);
}