/**
 * @file mock_streaming_source.cpp
 * @brief Mock streaming source for testing
 */

#include "mock_streaming_source.h"

MockStreamingSource::MockStreamingSource(QObject* parent)
    : QObject(parent)
    , m_isRunning(false)
{
}

MockStreamingSource::~MockStreamingSource()
{
    stop();
}

bool MockStreamingSource::start()
{
    m_isRunning = true;
    return true;
}

void MockStreamingSource::stop()
{
    m_isRunning = false;
}

bool MockStreamingSource::isRunning() const
{
    return m_isRunning;
}

void MockStreamingSource::sendTestData()
{
    if (m_isRunning) {
        QByteArray testData(1024, 0x55); // Test pattern
        emit dataAvailable(testData);
    }
}