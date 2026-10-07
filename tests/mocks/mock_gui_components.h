#pragma once

#include <QObject>
#include <QWidget>
#include <QByteArray>
#include <gmock/gmock.h>

class MockMainWindow;
class MockAnalyserWidget;
class MockConstellationWidget;
class MockServiceBrowser;

/**
 * @brief Mock implementation for GUI testing using Google Mock
 * 
 * Provides mock objects for all major GUI components to enable
 * isolated testing of individual widgets and interactions.
 */

/**
 * @brief Mock ETI data source for testing
 */
class MockEtiDataSource : public QObject {
    Q_OBJECT

public:
    explicit MockEtiDataSource(QObject* parent = nullptr);
    virtual ~MockEtiDataSource() = default;

    MOCK_METHOD(bool, isConnected, (), (const));
    MOCK_METHOD(void, connectToSource, (const QString& source));
    MOCK_METHOD(void, disconnectFromSource, ());
    MOCK_METHOD(QByteArray, getEtiFrame, ());

signals:
    void etiFrameReceived(const QByteArray& frame);
    void connectionStatusChanged(bool connected);
};

/**
 * @brief Mock analyser engine for testing
 */
class MockAnalyserEngine : public QObject {
    Q_OBJECT

public:
    explicit MockAnalyserEngine(QObject* parent = nullptr);
    virtual ~MockAnalyserEngine() = default;

    MOCK_METHOD(bool, processEtiFrame, (const QByteArray& frame));
    MOCK_METHOD(void, startAnalysis, ());
    MOCK_METHOD(void, stopAnalysis, ());
    MOCK_METHOD(bool, isAnalyzing, (), (const));

signals:
    void analysisStarted();
    void analysisStopped();
    void frameProcessed(int frameCount);
};

/**
 * @brief Mock service information for testing
 */
struct MockServiceInfo {
    uint32_t serviceId;
    QString serviceName;
    QString serviceLabel;
    bool isAvailable;
    int signalQuality;
    
    MockServiceInfo() : serviceId(0), isAvailable(false), signalQuality(0) {}
};

/**
 * @brief Mock constellation data for testing
 */
struct MockConstellationData {
    QList<float> iSamples;
    QList<float> qSamples;
    float snrDb;
    int symbolRate;
    
    MockConstellationData() : snrDb(0.0f), symbolRate(0) {}
};

/**
 * @brief Mock GUI component factory
 */
class MockGuiComponentFactory {
public:
    static MockEtiDataSource* createMockDataSource(QObject* parent = nullptr);
    static MockAnalyserEngine* createMockEngine(QObject* parent = nullptr);
    static MockServiceInfo createMockService(uint32_t id, const QString& name);
    static MockConstellationData createMockConstellation(int samples = 256);
};

/**
 * @brief Mock test helpers
 */
class MockTestHelpers {
public:
    // GUI interaction simulation
    static void simulateMouseClick(QWidget* widget, const QPoint& position);
    static void simulateKeyPress(QWidget* widget, int key);
    static void simulateResize(QWidget* widget, const QSize& size);
    
    // Data generation
    static QByteArray generateTestEtiFrame(uint16_t serviceId = 0x1234);
    static QList<MockServiceInfo> generateTestServices(int count = 5);
    static MockConstellationData generateTestConstellation(int samples = 256);
    
    // Timing and performance
    static bool waitForSignal(QObject* sender, const char* signal, int timeoutMs = 5000);
    static bool verifyGuiResponseTime(QWidget* widget, int maxResponseMs = 16);
};

// MOC will generate the required meta-object code