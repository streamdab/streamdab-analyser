// /home/seksan/workspace/streamdab-analyzer/src/core/eti_reader_thread.hpp
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
#include <QUdpSocket>          // Phase C: UDP streaming support
#include <QHostAddress>        // Phase C: Multicast support
#include <memory>
#include "eti_types.h"

// Forward declarations for existing components
class EnhancedETIProcessorQt;
class AdvancedFIGAnalyzer;

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
    void frameReceived(quint32 frameNumber);
    void ficDataUpdated(QSharedPointer<FICDataSignal> data);
    void mscDataUpdated(QSharedPointer<MSCData> data);
    void errorStatsUpdated(ErrorStats stats);
    void ensembleInfoChanged(EnsembleInfo info);
    void serviceListUpdated(QList<ServiceInfo> services);
    
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
    std::unique_ptr<AdvancedFIGAnalyzer> m_figAnalyzer;

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

    static constexpr int ETI_FRAME_SIZE = 6144;
    static constexpr int FRAME_INTERVAL_MS = 24;
};

Q_DECLARE_METATYPE(QSharedPointer<FICDataSignal>)
Q_DECLARE_METATYPE(QSharedPointer<MSCData>)
Q_DECLARE_METATYPE(ErrorStats)

#endif // ETI_READER_THREAD_HPP
