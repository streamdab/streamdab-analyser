#include "eti_processor.h"
#include "utils/logger.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>

EtiProcessor::EtiProcessor(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_frameCount(0)
    , m_status("Not initialized")
{
}

EtiProcessor::~EtiProcessor()
{
    Logger::instance().log(Logger::Info, "EtiProcessor", "Destructor called");
}

bool EtiProcessor::initialize()
{
    Logger::instance().log(Logger::Info, "EtiProcessor", "Initializing ETI processor");
    
    try {
        // Initialize ETI processing components
        m_frameCount = 0;
        m_status = "Ready";
        m_initialized = true;
        
        emit statusChanged(m_status);
        
        Logger::instance().log(Logger::Info, "EtiProcessor", "ETI processor initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        QString error = QString("Failed to initialize ETI processor: %1").arg(e.what());
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
}

bool EtiProcessor::processFile(const QString& filename)
{
    if (!m_initialized) {
        QString error = "ETI processor not initialized";
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
    
    QFileInfo fileInfo(filename);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        QString error = QString("File not found or not readable: %1").arg(filename);
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
    
    Logger::instance().log(Logger::Info, "EtiProcessor", 
                          QString("Processing ETI file: %1").arg(filename));
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QString error = QString("Failed to open file: %1").arg(filename);
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
    
    m_currentFile = filename;
    QByteArray data = file.readAll();
    file.close();
    
    return processData(data);
}

bool EtiProcessor::processData(const QByteArray& data)
{
    if (!m_initialized) {
        QString error = "ETI processor not initialized";
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
    
    if (data.isEmpty()) {
        QString error = "Empty data provided for processing";
        Logger::instance().log(Logger::Warning, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
    
    try {
        // Basic ETI frame processing (placeholder implementation)
        // In a real implementation, this would parse ETI frames according to ETSI standards
        
        m_frameCount++;
        m_status = QString("Processing... Frame: %1").arg(m_frameCount);
        
        emit frameProcessed(m_frameCount, data);
        emit statusChanged(m_status);
        
        Logger::instance().log(Logger::Debug, "EtiProcessor", 
                              QString("Processed frame %1, data size: %2 bytes")
                              .arg(m_frameCount).arg(data.size()));
        
        return true;
    }
    catch (const std::exception& e) {
        QString error = QString("Error processing ETI data: %1").arg(e.what());
        Logger::instance().log(Logger::Error, "EtiProcessor", error);
        emit errorOccurred(error);
        return false;
    }
}