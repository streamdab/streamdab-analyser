#include "mock_gui_components.h"
#include <QApplication>
#include <QTest>
#include <QTimer>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <cmath>

// MockEtiDataSource implementation
MockEtiDataSource::MockEtiDataSource(QObject* parent) : QObject(parent) {
    // Initialize mock data source
}

// MockAnalyserEngine implementation  
MockAnalyserEngine::MockAnalyserEngine(QObject* parent) : QObject(parent) {
    // Initialize mock analyser engine
}

// MockGuiComponentFactory implementation
MockEtiDataSource* MockGuiComponentFactory::createMockDataSource(QObject* parent) {
    return new MockEtiDataSource(parent);
}

MockAnalyserEngine* MockGuiComponentFactory::createMockEngine(QObject* parent) {
    return new MockAnalyserEngine(parent);
}

MockServiceInfo MockGuiComponentFactory::createMockService(uint32_t id, const QString& name) {
    MockServiceInfo service;
    service.serviceId = id;
    service.serviceName = name;
    service.serviceLabel = name;
    service.isAvailable = true;
    service.signalQuality = 85; // Good signal quality for testing
    return service;
}

MockConstellationData MockGuiComponentFactory::createMockConstellation(int samples) {
    MockConstellationData data;
    data.symbolRate = 2048000; // 2.048 MHz symbol rate
    data.snrDb = 25.0f; // Good SNR for testing
    
    // Generate circular constellation pattern
    for (int i = 0; i < samples; ++i) {
        float angle = 2.0f * M_PI * i / samples;
        data.iSamples.append(std::cos(angle) * 0.8f); // Slightly inside unit circle
        data.qSamples.append(std::sin(angle) * 0.8f);
    }
    
    return data;
}

// MockTestHelpers implementation
void MockTestHelpers::simulateMouseClick(QWidget* widget, const QPoint& position) {
    if (!widget) return;
    
    QTest::mouseClick(widget, Qt::LeftButton, Qt::NoModifier, position);
    QApplication::processEvents();
}

void MockTestHelpers::simulateKeyPress(QWidget* widget, int key) {
    if (!widget) return;
    
    QTest::keyPress(widget, static_cast<Qt::Key>(key));
    QApplication::processEvents();
}

void MockTestHelpers::simulateResize(QWidget* widget, const QSize& size) {
    if (!widget) return;
    
    widget->resize(size);
    QApplication::processEvents();
}

QByteArray MockTestHelpers::generateTestEtiFrame(uint16_t serviceId) {
    QByteArray frame(256, 0x00); // 256-byte ETI frame
    
    // ETI frame header (simplified)
    frame[0] = static_cast<char>(0xE0); // ERR field
    frame[1] = static_cast<char>(0xE1); // STAT field
    frame[2] = static_cast<char>(0xFF); // LIDATA high
    frame[3] = static_cast<char>(0xFF); // LIDATA low
    frame[4] = 0x00; // FC high
    frame[5] = 0x01; // FC low
    
    // Insert service ID
    frame[24] = static_cast<char>((serviceId >> 8) & 0xFF);
    frame[25] = static_cast<char>(serviceId & 0xFF);
    
    // Add some realistic FIC data
    frame[32] = 0x00; // FIG type 0
    frame[33] = 0x01; // Extension 1 (service info)
    frame[34] = static_cast<char>((serviceId >> 8) & 0xFF);
    frame[35] = static_cast<char>(serviceId & 0xFF);
    
    return frame;
}

QList<MockServiceInfo> MockTestHelpers::generateTestServices(int count) {
    QList<MockServiceInfo> services;
    
    const QStringList serviceNames = {
        "BBC Radio 1", "BBC Radio 2", "BBC Radio 3", "BBC Radio 4",
        "Classic FM", "Heart", "Capital", "Kiss", "Magic", "LBC"
    };
    
    for (int i = 0; i < count && i < serviceNames.size(); ++i) {
        MockServiceInfo service = MockGuiComponentFactory::createMockService(
            0x1000 + i, serviceNames[i]);
        service.signalQuality = 70 + (i * 5); // Varying signal quality
        services.append(service);
    }
    
    return services;
}

MockConstellationData MockTestHelpers::generateTestConstellation(int samples) {
    return MockGuiComponentFactory::createMockConstellation(samples);
}

bool MockTestHelpers::waitForSignal(QObject* sender, const char* signal, int timeoutMs) {
    if (!sender || !signal) return false;
    
    // Use QSignalSpy for proper Qt testing signal waiting
    QSignalSpy spy(sender, signal);
    if (!spy.isValid()) {
        return false;
    }
    
    // Wait for the signal with timeout
    return spy.wait(timeoutMs);
}

bool MockTestHelpers::verifyGuiResponseTime(QWidget* widget, int maxResponseMs) {
    if (!widget) return false;
    
    QElapsedTimer timer;
    timer.start();
    
    // Simulate user interaction
    simulateMouseClick(widget, widget->rect().center());
    
    // Check response time
    return timer.elapsed() <= maxResponseMs;
}