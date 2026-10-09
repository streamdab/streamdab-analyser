// src/core/eti_reader_thread.hpp
#ifndef ETI_READER_THREAD_HPP
#define ETI_READER_THREAD_HPP

#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QSharedPointer>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QVector>
#include <QFile>
#include <QElapsedTimer>
#include <QUdpSocket>          // Phase C: UDP streaming support
#include <QHostAddress>        // Phase C: Multicast support
#include <memory>
#include "eti_types.h"

// Forward declarations for existing components
class EnhancedETIProcessorQt;
class AdvancedFIGAnalyser;

/**
 * @brief Additional data structures for ETI processing signals
 */
struct FICDataSignal {
    quint16 ensembleId;
    QString ensembleLabel;
    QList<ServiceInfo> services;
    
    FICDataSignal() : ensembleId(0) {}
};

struct MSCData {
    QByteArray subchannelData;
    QVector<quint8> errorFlags;
};

struct ErrorStats {
    int frameErrors;
    int crcErrors;
    int syncLost;
    
    ErrorStats() : frameErrors(0), crcErrors(0), syncLost(0) {}
};

/**
 * @brief Pattern 3 (Worker Thread + Event Loop) for ETI frame processing
 */
class ETIReaderThread : public QThread {
    Q_OBJECT

public:
    enum class InputMode {
        FileMode,
        UDPMode
    };

    explicit ETIReaderThread(QObject *parent = nullptr);
    ~ETIReaderThread() override;

    void setFilePath(const QString &filePath);
    void setInputMode(InputMode mode);
    int getFramesProcessed() const { return m_framesProcessed; }
    int getProcessingTimeMs() const { return m_processingTimeMs; }
    
    // Phase C: UDP configuration methods
    void setUDPPort(quint16 port);
    // F7: configured UDP port (default 12000) — used to seed the settings dialog.
    quint16 getUDPPort() const { return m_udpPort; }
    void setMulticastAddress(const QString& address);
    void setUseMulticast(bool enable);
    int getPacketsReceived() const { return m_packetsReceived; }
    int getPacketsLost() const { return m_packetsLost; }

public slots:
    void startProcessing();
    void stopProcessing();
    void pauseProcessing();
    void resumeProcessing();

signals:
    // Phase D: Pattern 7 - Specialized signals with throttling at source
    void frameCounterUpdated(quint32 frameNumber);       // 5 fps (throttled)
    void figDataReady(QSharedPointer<FICDataSignal> data);  // 1 fps (throttled)
    void errorStatsReady(ErrorStats stats);              // 0.5 fps (throttled)
    void mscDataReady(QSharedPointer<MSCData> data);     // 2 fps (throttled)
    
    // Event-driven signals (not throttled - fire on change only)
    void serviceListChanged(QList<ServiceInfo> services);
    void ensembleInfoChanged(EnsembleInfo info);
    
    // Legacy signals (Phase A/B compatibility - keep for existing connections)
    void frameReceived(quint32 frameNumber);             // 41 fps (internal)
    void ficDataUpdated(QSharedPointer<FICDataSignal> data);
    void mscDataUpdated(QSharedPointer<MSCData> data);
    void errorStatsUpdated(ErrorStats stats);
    void serviceListUpdated(QList<ServiceInfo> services);
    
    // Lifecycle signals
    void processingStarted();
    void processingComplete(int totalFrames, int timeMs);
    void processingError(QString error);

protected:
    void run() override;

private slots:
    void processNextFrame();

private:
    bool openFile();
    void closeFile();
    bool readFrame(QByteArray &buffer);
    
    bool openUDPSocket();
    void closeUDPSocket();
    bool receiveFrame(QByteArray &buffer);
    
    QSharedPointer<FICDataSignal> extractFICData();
    QSharedPointer<MSCData> extractMSCData();
    ErrorStats extractErrorStats();
    EnsembleInfo extractEnsembleInfo();
    QList<ServiceInfo> extractServiceList();

    std::unique_ptr<EnhancedETIProcessorQt> m_processor;
    std::unique_ptr<AdvancedFIGAnalyser> m_figAnalyser;

    mutable QMutex m_controlMutex;
    bool m_running;
    bool m_paused;

    InputMode m_inputMode;
    QString m_filePath;
    QFile m_file;

    // Phase C: UDP streaming members
    QUdpSocket* m_udpSocket;
    QString m_multicastAddress;
    quint16 m_udpPort;
    bool m_useMulticast;
    QByteArray m_networkBuffer;
    int m_packetsReceived;
    int m_packetsLost;

    quint32 m_currentFrame;
    int m_framesProcessed;
    int m_processingTimeMs;
    QElapsedTimer m_elapsedTimer;

    ErrorStats m_errorStats;
    EnsembleInfo m_ensembleInfo;
    
    // Phase D: Pattern 8 - Throttling timers for specialized signals
    QElapsedTimer m_frameCounterThrottle;
    QElapsedTimer m_figDataThrottle;
    QElapsedTimer m_errorStatsThrottle;
    QElapsedTimer m_mscDataThrottle;

    static constexpr int ETI_FRAME_SIZE = 6144;
    static constexpr int FRAME_INTERVAL_MS = 24;
    
    // Phase D: Throttling intervals for specialized signals
    static constexpr int FRAME_COUNTER_INTERVAL_MS = 200;   // 5 fps
    static constexpr int FIG_DATA_INTERVAL_MS = 1000;       // 1 fps
    static constexpr int ERROR_STATS_INTERVAL_MS = 2000;    // 0.5 fps
    static constexpr int MSC_DATA_INTERVAL_MS = 500;        // 2 fps
};

Q_DECLARE_METATYPE(QSharedPointer<FICDataSignal>)
Q_DECLARE_METATYPE(QSharedPointer<MSCData>)
Q_DECLARE_METATYPE(ErrorStats)

#endif // ETI_READER_THREAD_HPP
