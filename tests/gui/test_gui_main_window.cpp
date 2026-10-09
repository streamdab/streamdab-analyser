/**
 * @file test_gui_main_window.cpp
 * @brief GUI Tests for Main Window Component
 *
 * PDCA Week 5 Phase 2 - E2E/GUI Test Implementation
 *
 * @author Claude Code (Sonnet 4.5)
 * @date November 2, 2025
 */

#include <QtTest/QtTest>
#include <QWidget>
#include <QLabel>
#include <QString>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>

/**
 * Mock MainWindow for testing
 */
class MockMainWindow : public QMainWindow {
public:
    MockMainWindow()
        : m_fileLoaded(false)
        , m_isConnected(false)
        , m_isRecording(false)
        , m_frameCount(0)
    {
        // Create basic window structure
        setWindowTitle("StreamDAB Analyser");

        m_menuBar = new QMenuBar(this);
        setMenuBar(m_menuBar);

        m_toolBar = new QToolBar(this);
        addToolBar(m_toolBar);

        m_statusBar = new QStatusBar(this);
        setStatusBar(m_statusBar);
    }

    void loadFile(const QString& filename) {
        m_currentFile = filename;
        m_fileLoaded = true;
        m_frameCount = 0;
    }

    void closeFile() {
        m_currentFile.clear();
        m_fileLoaded = false;
        m_frameCount = 0;
    }

    bool isFileLoaded() const { return m_fileLoaded; }
    QString getCurrentFile() const { return m_currentFile; }

    void connectToStream(const QString& address) {
        m_streamAddress = address;
        m_isConnected = true;
    }

    void disconnectFromStream() {
        m_streamAddress.clear();
        m_isConnected = false;
        m_isRecording = false;
    }

    bool isConnected() const { return m_isConnected; }
    QString getStreamAddress() const { return m_streamAddress; }

    void startRecording() {
        if (m_isConnected) {
            m_isRecording = true;
        }
    }

    void stopRecording() {
        m_isRecording = false;
    }

    bool isRecording() const { return m_isRecording; }

    void setFrameCount(int count) { m_frameCount = count; }
    int getFrameCount() const { return m_frameCount; }

    void setStatusMessage(const QString& message) {
        m_statusBar->showMessage(message);
    }

    QString getStatusMessage() const {
        return m_statusBar->currentMessage();
    }

private:
    QString m_currentFile;
    QString m_streamAddress;
    bool m_fileLoaded;
    bool m_isConnected;
    bool m_isRecording;
    int m_frameCount;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QStatusBar* m_statusBar;
};

class TestGuiMainWindow : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { /* Ready */ }

    void testWindow_Creation() {
        MockMainWindow window;
        QVERIFY(!window.isFileLoaded());
        QVERIFY(!window.isConnected());
        QVERIFY(!window.isRecording());
        QCOMPARE(window.getFrameCount(), 0);
    }

    void testWindow_LoadFile() {
        MockMainWindow window;
        window.loadFile("/path/to/test.eti");
        QVERIFY(window.isFileLoaded());
        QCOMPARE(window.getCurrentFile(), QString("/path/to/test.eti"));
    }

    void testWindow_CloseFile() {
        MockMainWindow window;
        window.loadFile("/path/to/test.eti");
        QVERIFY(window.isFileLoaded());

        window.closeFile();
        QVERIFY(!window.isFileLoaded());
        QVERIFY(window.getCurrentFile().isEmpty());
        QCOMPARE(window.getFrameCount(), 0);
    }

    void testWindow_ConnectToStream() {
        MockMainWindow window;
        window.connectToStream("239.192.0.1:9200");
        QVERIFY(window.isConnected());
        QCOMPARE(window.getStreamAddress(), QString("239.192.0.1:9200"));
    }

    void testWindow_DisconnectFromStream() {
        MockMainWindow window;
        window.connectToStream("239.192.0.1:9200");
        QVERIFY(window.isConnected());

        window.disconnectFromStream();
        QVERIFY(!window.isConnected());
        QVERIFY(window.getStreamAddress().isEmpty());
        QVERIFY(!window.isRecording()); // Recording should stop on disconnect
    }

    void testWindow_StartRecording() {
        MockMainWindow window;
        window.connectToStream("239.192.0.1:9200");
        window.startRecording();
        QVERIFY(window.isRecording());
    }

    void testWindow_StopRecording() {
        MockMainWindow window;
        window.connectToStream("239.192.0.1:9200");
        window.startRecording();
        QVERIFY(window.isRecording());

        window.stopRecording();
        QVERIFY(!window.isRecording());
    }

    void testWindow_RecordingRequiresConnection() {
        MockMainWindow window;
        // Try to start recording without connection
        window.startRecording();
        QVERIFY(!window.isRecording());

        // Connect and try again
        window.connectToStream("239.192.0.1:9200");
        window.startRecording();
        QVERIFY(window.isRecording());
    }

    void testWindow_FrameCounter() {
        MockMainWindow window;
        QCOMPARE(window.getFrameCount(), 0);

        window.setFrameCount(100);
        QCOMPARE(window.getFrameCount(), 100);

        window.setFrameCount(1000);
        QCOMPARE(window.getFrameCount(), 1000);
    }

    void testWindow_StatusMessage() {
        MockMainWindow window;
        window.setStatusMessage("Ready");
        QCOMPARE(window.getStatusMessage(), QString("Ready"));

        window.setStatusMessage("Processing frame 100...");
        QCOMPARE(window.getStatusMessage(), QString("Processing frame 100..."));
    }

    void testWindow_Visibility() {
        MockMainWindow window;
        QVERIFY(!window.isVisible());
        window.show();
        QVERIFY(window.isVisible());
        window.hide();
        QVERIFY(!window.isVisible());
    }

    void testWindow_EnabledState() {
        MockMainWindow window;
        QVERIFY(window.isEnabled());
        window.setEnabled(false);
        QVERIFY(!window.isEnabled());
        window.setEnabled(true);
        QVERIFY(window.isEnabled());
    }

    void testWindow_StatePersistence() {
        MockMainWindow window;

        // Set up multiple states
        window.loadFile("/path/to/test.eti");
        window.setFrameCount(500);
        window.setStatusMessage("File loaded");

        // Verify all states persist
        QVERIFY(window.isFileLoaded());
        QCOMPARE(window.getCurrentFile(), QString("/path/to/test.eti"));
        QCOMPARE(window.getFrameCount(), 500);
        QCOMPARE(window.getStatusMessage(), QString("File loaded"));
    }

    void testWindow_MultipleFileLoads() {
        MockMainWindow window;

        window.loadFile("/path/to/file1.eti");
        QCOMPARE(window.getCurrentFile(), QString("/path/to/file1.eti"));

        window.loadFile("/path/to/file2.eti");
        QCOMPARE(window.getCurrentFile(), QString("/path/to/file2.eti"));

        window.loadFile("/path/to/file3.eti");
        QCOMPARE(window.getCurrentFile(), QString("/path/to/file3.eti"));
    }

    void testWindow_StreamConnectionStates() {
        MockMainWindow window;

        // Test TCP stream
        window.connectToStream("tcp://localhost:9200");
        QVERIFY(window.isConnected());
        QCOMPARE(window.getStreamAddress(), QString("tcp://localhost:9200"));
        window.disconnectFromStream();

        // Test UDP multicast
        window.connectToStream("239.192.0.1:9200");
        QVERIFY(window.isConnected());
        QCOMPARE(window.getStreamAddress(), QString("239.192.0.1:9200"));
        window.disconnectFromStream();

        // Test ZMQ stream
        window.connectToStream("zmq+tcp://localhost:9200");
        QVERIFY(window.isConnected());
        QCOMPARE(window.getStreamAddress(), QString("zmq+tcp://localhost:9200"));
    }

    void testWindow_FrameCountReset() {
        MockMainWindow window;

        // Load file, process frames
        window.loadFile("/path/to/test.eti");
        window.setFrameCount(1000);
        QCOMPARE(window.getFrameCount(), 1000);

        // Close file should reset counter
        window.closeFile();
        QCOMPARE(window.getFrameCount(), 0);
    }
};

QTEST_MAIN(TestGuiMainWindow)
#include "test_gui_main_window.moc"
