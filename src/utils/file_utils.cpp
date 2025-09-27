#include "file_utils.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include <QDebug>
#include <QMimeDatabase>
#include <QMimeType>
#include <algorithm>

// Thread-local error storage
thread_local QString FileUtils::s_lastError;

// ETI format constants
namespace {
    const int ETI_NI_FRAME_SIZE = 6144;  // 6144 bytes per ETI-NI frame
    const double ETI_FRAME_RATE = 24.0;  // 24ms per frame = ~41.67 frames/sec
    
    // ETI sync patterns
    const QByteArray ETI_NI_SYNC = QByteArray::fromHex("FF1F"); // ETI-NI sync pattern
    const QByteArray ETI_LI_SYNC = QByteArray::fromHex("FF00"); // ETI-LI sync pattern
    const QByteArray EDI_MAGIC = QByteArray("EDI");             // EDI magic header
}

// Format Detection
FileUtils::EtiFileFormat FileUtils::detectFileFormat(const QString& filePath)
{
    clearError();
    
    if (!isReadableFile(filePath)) {
        setError(QString("Cannot read file: %1").arg(filePath));
        return EtiFileFormat::Unknown;
    }
    
    QByteArray headerData = readFileHeader(filePath, 1024);
    if (headerData.isEmpty()) {
        setError("Failed to read file header");
        return EtiFileFormat::Unknown;
    }
    
    return detectFormatFromData(headerData);
}

FileUtils::EtiFileFormat FileUtils::detectFormatFromData(const QByteArray& data)
{
    if (data.size() < 16) {
        return EtiFileFormat::Unknown;
    }
    
    // Check for EDI format (starts with "EDI" magic)
    if (data.startsWith(EDI_MAGIC)) {
        return EtiFileFormat::EDI;
    }
    
    // Check for ETI-NI sync pattern
    if (data.startsWith(ETI_NI_SYNC)) {
        return EtiFileFormat::ETI_NI;
    }
    
    // Check for ETI-LI sync pattern
    if (data.startsWith(ETI_LI_SYNC)) {
        return EtiFileFormat::ETI_LI;
    }
    
    // Search for sync patterns within first 1KB
    int position = 0;
    if (findEtiSyncPattern(data, position)) {
        // Determine format based on sync pattern found
        if (position + 2 < data.size()) {
            QByteArray syncBytes = data.mid(position, 2);
            if (syncBytes == ETI_NI_SYNC) {
                return EtiFileFormat::ETI_NI;
            } else if (syncBytes == ETI_LI_SYNC) {
                return EtiFileFormat::ETI_LI;
            }
        }
    }
    
    // If no specific pattern found, assume raw ETI data
    return EtiFileFormat::Raw;
}

bool FileUtils::isEtiFile(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    
    // Check extension first
    if (isValidEtiExtension(fileInfo.suffix().toLower())) {
        return true;
    }
    
    // Check content format
    EtiFileFormat format = detectFileFormat(filePath);
    return format != EtiFileFormat::Unknown;
}

bool FileUtils::isValidEtiExtension(const QString& extension)
{
    QStringList validExtensions = getSupportedEtiExtensions();
    return validExtensions.contains(extension.toLower());
}

// File Validation
FileUtils::ValidationResult FileUtils::validateEtiFile(const QString& filePath)
{
    clearError();
    ValidationResult result;
    
    if (!isReadableFile(filePath)) {
        result.errorMessage = QString("Cannot read file: %1").arg(filePath);
        setError(result.errorMessage);
        return result;
    }
    
    QFileInfo fileInfo(filePath);
    result.fileSize = fileInfo.size();
    
    if (result.fileSize == 0) {
        result.errorMessage = "File is empty";
        setError(result.errorMessage);
        return result;
    }
    
    // Detect format
    result.format = detectFileFormat(filePath);
    if (result.format == EtiFileFormat::Unknown) {
        result.errorMessage = "Unknown or invalid ETI format";
        setError(result.errorMessage);
        return result;
    }
    
    // Validate header
    QByteArray headerData = readFileHeader(filePath, 1024);
    if (!validateEtiHeader(headerData)) {
        result.errorMessage = "Invalid ETI header";
        setError(result.errorMessage);
        return result;
    }
    
    // Estimate frame count
    result.estimatedFrameCount = estimateFrameCount(filePath, result.format);
    
    // Validate first frame
    int frameSize = extractEtiFrameSize(result.format);
    if (frameSize > 0 && result.fileSize >= frameSize) {
        QByteArray firstFrame = readFileChunk(filePath, 0, frameSize);
        if (!validateEtiFrame(firstFrame)) {
            result.errorMessage = "Invalid ETI frame structure";
            setError(result.errorMessage);
            return result;
        }
    }
    
    result.isValid = true;
    return result;
}

bool FileUtils::validateEtiHeader(const QByteArray& headerData)
{
    if (headerData.size() < 16) {
        return false;
    }
    
    EtiFileFormat format = detectFormatFromData(headerData);
    
    switch (format) {
        case EtiFileFormat::ETI_NI:
            return validateEtiNiFrame(headerData);
        case EtiFileFormat::ETI_LI:
            return validateEtiLiFrame(headerData);
        case EtiFileFormat::EDI:
            return validateEdiFrame(headerData);
        case EtiFileFormat::Raw:
            return true; // Raw format validation is less strict
        default:
            return false;
    }
}

bool FileUtils::validateEtiFrame(const QByteArray& frameData)
{
    if (frameData.isEmpty()) {
        return false;
    }
    
    // Frame validation same as header for now
    return validateEtiHeader(frameData);
}

// File Size and Statistics
qint64 FileUtils::getFileSize(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.exists() ? fileInfo.size() : -1;
}

qint64 FileUtils::estimateFrameCount(const QString& filePath, EtiFileFormat format)
{
    qint64 fileSize = getFileSize(filePath);
    if (fileSize <= 0) {
        return 0;
    }
    
    int frameSize = extractEtiFrameSize(format);
    if (frameSize <= 0) {
        return 0;
    }
    
    return fileSize / frameSize;
}

QPair<qint64, qint64> FileUtils::getFrameRange(const QString& filePath, EtiFileFormat format)
{
    qint64 frameCount = estimateFrameCount(filePath, format);
    return QPair<qint64, qint64>(0, frameCount - 1);
}

double FileUtils::calculateFileDuration(qint64 frameCount, double frameRate)
{
    if (frameRate <= 0.0) {
        frameRate = ETI_FRAME_RATE;
    }
    
    return static_cast<double>(frameCount) / frameRate;
}

// File Operations
bool FileUtils::copyFile(const QString& source, const QString& destination, bool overwrite)
{
    clearError();
    
    if (!isReadableFile(source)) {
        setError(QString("Source file not readable: %1").arg(source));
        return false;
    }
    
    QFileInfo destInfo(destination);
    if (destInfo.exists() && !overwrite) {
        setError(QString("Destination file exists: %1").arg(destination));
        return false;
    }
    
    // Ensure destination directory exists
    if (!createDirectory(destInfo.dir().path())) {
        setError(QString("Cannot create destination directory: %1").arg(destInfo.dir().path()));
        return false;
    }
    
    if (destInfo.exists()) {
        QFile::remove(destination);
    }
    
    bool success = QFile::copy(source, destination);
    if (!success) {
        setError(QString("Failed to copy file from %1 to %2").arg(source, destination));
    }
    
    return success;
}

bool FileUtils::moveFile(const QString& source, const QString& destination, bool overwrite)
{
    clearError();
    
    if (!copyFile(source, destination, overwrite)) {
        return false; // Error already set by copyFile
    }
    
    bool success = deleteFile(source);
    if (!success) {
        // Copy succeeded but delete failed - try to cleanup destination
        QFile::remove(destination);
        setError(QString("Failed to delete source file after copy: %1").arg(source));
    }
    
    return success;
}

bool FileUtils::deleteFile(const QString& filePath)
{
    clearError();
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        return true; // File doesn't exist, consider it deleted
    }
    
    bool success = QFile::remove(filePath);
    if (!success) {
        setError(QString("Failed to delete file: %1").arg(filePath));
    }
    
    return success;
}

bool FileUtils::createBackup(const QString& filePath, const QString& backupSuffix)
{
    clearError();
    
    if (!isReadableFile(filePath)) {
        setError(QString("Source file not readable: %1").arg(filePath));
        return false;
    }
    
    QString backupPath = filePath + backupSuffix;
    return copyFile(filePath, backupPath, true);
}

// Directory Operations
bool FileUtils::createDirectory(const QString& dirPath)
{
    clearError();
    
    QDir dir(dirPath);
    if (dir.exists()) {
        return true;
    }
    
    bool success = dir.mkpath(".");
    if (!success) {
        setError(QString("Failed to create directory: %1").arg(dirPath));
    }
    
    return success;
}

bool FileUtils::removeDirectory(const QString& dirPath, bool recursive)
{
    clearError();
    
    QDir dir(dirPath);
    if (!dir.exists()) {
        return true; // Directory doesn't exist
    }
    
    bool success;
    if (recursive) {
        success = dir.removeRecursively();
    } else {
        success = dir.rmdir(".");
    }
    
    if (!success) {
        setError(QString("Failed to remove directory: %1").arg(dirPath));
    }
    
    return success;
}

FileUtils::ScanResult FileUtils::scanDirectory(const QString& dirPath, bool recursive)
{
    ScanResult result;
    clearError();
    
    QDir dir(dirPath);
    if (!dir.exists()) {
        setError(QString("Directory does not exist: %1").arg(dirPath));
        return result;
    }
    
    QDir::Filters filters = QDir::Files | QDir::Readable;
    QDir::SortFlags sort = QDir::Name | QDir::IgnoreCase;
    
    QFileInfoList fileList;
    if (recursive) {
        QDirIterator iterator(dirPath, filters, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            fileList.append(iterator.fileInfo());
        }
    } else {
        fileList = dir.entryInfoList(filters, sort);
    }
    
    for (const QFileInfo& fileInfo : fileList) {
        QString filePath = fileInfo.absoluteFilePath();
        result.totalSize += fileInfo.size();
        result.fileCount++;
        
        if (isEtiFile(filePath)) {
            result.etiFiles.append(filePath);
        } else {
            result.otherFiles.append(filePath);
        }
    }
    
    return result;
}

QStringList FileUtils::findEtiFiles(const QString& dirPath, bool recursive)
{
    ScanResult result = scanDirectory(dirPath, recursive);
    return result.etiFiles;
}

// Path Utilities
QString FileUtils::getCanonicalPath(const QString& path)
{
    QFileInfo fileInfo(path);
    return fileInfo.canonicalFilePath();
}

QString FileUtils::getRelativePath(const QString& basePath, const QString& targetPath)
{
    QDir baseDir(basePath);
    return baseDir.relativeFilePath(targetPath);
}

QString FileUtils::generateUniqueFileName(const QString& basePath, const QString& baseName, const QString& extension)
{
    QDir dir(basePath);
    QString fileName = baseName + "." + extension;
    QString fullPath = dir.filePath(fileName);
    
    int counter = 1;
    while (QFile::exists(fullPath)) {
        fileName = QString("%1_%2.%3").arg(baseName).arg(counter).arg(extension);
        fullPath = dir.filePath(fileName);
        counter++;
    }
    
    return fullPath;
}

QString FileUtils::sanitizeFileName(const QString& fileName)
{
    QString sanitized = fileName;
    
    // Replace invalid characters with underscores
    QStringList invalidChars = {"<", ">", ":", "\"", "|", "?", "*", "/", "\\"};
    for (const QString& invalid : invalidChars) {
        sanitized.replace(invalid, "_");
    }
    
    // Remove leading/trailing spaces and dots
    sanitized = sanitized.trimmed();
    while (sanitized.endsWith('.')) {
        sanitized.chop(1);
    }
    
    // Ensure non-empty result
    if (sanitized.isEmpty()) {
        sanitized = "unnamed_file";
    }
    
    return sanitized;
}

// Temporary File Operations
QString FileUtils::createTempFile(const QString& templateName)
{
    clearError();
    
    QTemporaryFile tempFile(templateName);
    if (!tempFile.open()) {
        setError("Failed to create temporary file");
        return QString();
    }
    
    tempFile.setAutoRemove(false); // Don't auto-remove, caller manages lifecycle
    QString tempPath = tempFile.fileName();
    tempFile.close();
    
    return tempPath;
}

QString FileUtils::createTempDirectory(const QString& templateName)
{
    clearError();
    
    QTemporaryDir tempDir(templateName);
    if (!tempDir.isValid()) {
        setError("Failed to create temporary directory");
        return QString();
    }
    
    tempDir.setAutoRemove(false); // Don't auto-remove, caller manages lifecycle
    return tempDir.path();
}

bool FileUtils::cleanupTempFiles(const QStringList& tempFiles)
{
    bool allSuccess = true;
    
    for (const QString& tempFile : tempFiles) {
        QFileInfo fileInfo(tempFile);
        if (fileInfo.isDir()) {
            if (!removeDirectory(tempFile, true)) {
                allSuccess = false;
            }
        } else if (fileInfo.isFile()) {
            if (!deleteFile(tempFile)) {
                allSuccess = false;
            }
        }
    }
    
    return allSuccess;
}

// File Content Operations
QByteArray FileUtils::readFileChunk(const QString& filePath, qint64 offset, qint64 size)
{
    clearError();
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QString("Cannot open file for reading: %1").arg(filePath));
        return QByteArray();
    }
    
    if (!file.seek(offset)) {
        setError(QString("Cannot seek to position %1 in file %2").arg(offset).arg(filePath));
        return QByteArray();
    }
    
    QByteArray data = file.read(size);
    if (data.isEmpty() && file.error() != QFile::NoError) {
        setError(QString("Error reading from file: %1").arg(file.errorString()));
    }
    
    return data;
}

bool FileUtils::writeFileChunk(const QString& filePath, qint64 offset, const QByteArray& data)
{
    clearError();
    
    QFile file(filePath);
    QIODevice::OpenMode mode = QIODevice::WriteOnly;
    
    // If file exists and we're not writing at beginning, open in ReadWrite mode
    if (QFile::exists(filePath) && offset > 0) {
        mode = QIODevice::ReadWrite;
    }
    
    if (!file.open(mode)) {
        setError(QString("Cannot open file for writing: %1").arg(filePath));
        return false;
    }
    
    if (!file.seek(offset)) {
        setError(QString("Cannot seek to position %1 in file %2").arg(offset).arg(filePath));
        return false;
    }
    
    qint64 bytesWritten = file.write(data);
    if (bytesWritten != data.size()) {
        setError(QString("Failed to write complete data to file: %1").arg(filePath));
        return false;
    }
    
    return true;
}

QByteArray FileUtils::calculateFileHash(const QString& filePath, const QString& algorithm)
{
    clearError();
    
    QCryptographicHash::Algorithm hashAlgo = QCryptographicHash::Md5;
    if (algorithm.toUpper() == "SHA1") {
        hashAlgo = QCryptographicHash::Sha1;
    } else if (algorithm.toUpper() == "SHA256") {
        hashAlgo = QCryptographicHash::Sha256;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QString("Cannot open file for hashing: %1").arg(filePath));
        return QByteArray();
    }
    
    QCryptographicHash hash(hashAlgo);
    if (!hash.addData(&file)) {
        setError(QString("Failed to calculate hash for file: %1").arg(filePath));
        return QByteArray();
    }
    
    return hash.result();
}

bool FileUtils::compareFiles(const QString& file1, const QString& file2)
{
    clearError();
    
    QFileInfo info1(file1);
    QFileInfo info2(file2);
    
    // Quick size check
    if (info1.size() != info2.size()) {
        return false;
    }
    
    // Hash comparison for larger files
    if (info1.size() > 1024 * 1024) { // 1MB threshold
        QByteArray hash1 = calculateFileHash(file1);
        QByteArray hash2 = calculateFileHash(file2);
        return hash1 == hash2;
    }
    
    // Byte-by-byte comparison for smaller files
    QFile f1(file1);
    QFile f2(file2);
    
    if (!f1.open(QIODevice::ReadOnly) || !f2.open(QIODevice::ReadOnly)) {
        setError("Cannot open files for comparison");
        return false;
    }
    
    const int chunkSize = 4096;
    while (!f1.atEnd() && !f2.atEnd()) {
        QByteArray chunk1 = f1.read(chunkSize);
        QByteArray chunk2 = f2.read(chunkSize);
        
        if (chunk1 != chunk2) {
            return false;
        }
    }
    
    return f1.atEnd() && f2.atEnd();
}

// ETI-Specific Utilities
int FileUtils::extractEtiFrameSize(EtiFileFormat format)
{
    switch (format) {
        case EtiFileFormat::ETI_NI:
        case EtiFileFormat::ETI_LI:
            return ETI_NI_FRAME_SIZE;
        case EtiFileFormat::EDI:
        case EtiFileFormat::Raw:
            return 0; // Variable or unknown size
        default:
            return 0;
    }
}

QByteArray FileUtils::extractEtiSyncPattern(EtiFileFormat format)
{
    switch (format) {
        case EtiFileFormat::ETI_NI:
            return ETI_NI_SYNC;
        case EtiFileFormat::ETI_LI:
            return ETI_LI_SYNC;
        case EtiFileFormat::EDI:
            return EDI_MAGIC;
        default:
            return QByteArray();
    }
}

bool FileUtils::findEtiSyncPattern(const QByteArray& data, int& position)
{
    // Search for any known sync pattern
    QList<QByteArray> patterns = {ETI_NI_SYNC, ETI_LI_SYNC, EDI_MAGIC};
    
    for (const QByteArray& pattern : patterns) {
        int pos = data.indexOf(pattern);
        if (pos >= 0) {
            position = pos;
            return true;
        }
    }
    
    position = -1;
    return false;
}

QStringList FileUtils::getSupportedEtiExtensions()
{
    return {"eti", "ett", "edi", "dab", "raw", "bin"};
}

// Error Handling
QString FileUtils::getLastErrorString()
{
    return s_lastError;
}

bool FileUtils::hasError()
{
    return !s_lastError.isEmpty();
}

void FileUtils::clearError()
{
    s_lastError.clear();
}

// Private Implementation
bool FileUtils::validateEtiNiFrame(const QByteArray& frameData)
{
    if (frameData.size() < 8) return false;
    
    // Check sync bytes
    if (!frameData.startsWith(ETI_NI_SYNC)) {
        return false;
    }
    
    // Basic frame structure validation
    // ETI-NI frame: [SYNC][FC][NST][FP][STC]...
    // This is a simplified validation
    
    return true;
}

bool FileUtils::validateEtiLiFrame(const QByteArray& frameData)
{
    if (frameData.size() < 8) return false;
    
    // Check sync bytes
    if (!frameData.startsWith(ETI_LI_SYNC)) {
        return false;
    }
    
    // Basic ETI-LI structure validation
    return true;
}

bool FileUtils::validateEdiFrame(const QByteArray& frameData)
{
    if (frameData.size() < 8) return false;
    
    // Check EDI magic header
    if (!frameData.startsWith(EDI_MAGIC)) {
        return false;
    }
    
    // Basic EDI structure validation
    return true;
}

bool FileUtils::isReadableFile(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable();
}

bool FileUtils::isWritableLocation(const QString& dirPath)
{
    QFileInfo dirInfo(dirPath);
    return dirInfo.exists() && dirInfo.isDir() && dirInfo.isWritable();
}

QByteArray FileUtils::readFileHeader(const QString& filePath, int headerSize)
{
    return readFileChunk(filePath, 0, headerSize);
}

void FileUtils::setError(const QString& error)
{
    s_lastError = error;
    qWarning() << "FileUtils error:" << error;
}