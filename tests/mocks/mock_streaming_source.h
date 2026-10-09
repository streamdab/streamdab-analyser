/**
 * @file mock_streaming_source.h
 * @brief Mock streaming source for testing
 */

#pragma once

#include <QObject>
#include <QByteArray>

class MockStreamingSource : public QObject
{
    Q_OBJECT

public:
    explicit MockStreamingSource(QObject* parent = nullptr);
    ~MockStreamingSource();

    bool start();
    void stop();
    bool isRunning() const;
    
    void sendTestData();

signals:
    void dataAvailable(const QByteArray& data);

private:
    bool m_isRunning;
};