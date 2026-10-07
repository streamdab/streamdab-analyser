#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QFileInfo>
#include <QDir>
#include <QPair>

/**
 * @class FileUtils
 * @brief Utility functions for file and directory operations
 * 
 * Provides common file operations for ETI Stream Analyser including
 * ETI file validation, format detection, and file system utilities.
 */
class FileUtils
{
public:
    /**
     * @brief ETI file format types
     */
    enum class EtiFileFormat {
        Unknown = 0,
        ETI_NI = 1,      // ETI Native Interface (6144 bytes/frame)
        ETI_LI = 2,      // ETI Linear Interface
        EDI = 3,         // Encapsulated DAB Interface
        Raw = 4          // Raw binary ETI data
    };

    /**
     * @brief File validation result
     */
    struct ValidationResult {
        bool isValid = false;
        EtiFileFormat format = EtiFileFormat::Unknown;
        QString errorMessage;
        qint64 fileSize = 0;
        qint64 estimatedFrameCount = 0;
        
        ValidationResult() = default;
        ValidationResult(bool valid, EtiFileFormat fmt, const QString& error = QString())
            : isValid(valid), format(fmt), errorMessage(error) {}
    };

    /**
     * @brief Directory scanning result
     */
    struct ScanResult {
        QStringList etiFiles;
        QStringList otherFiles;
        qint64 totalSize = 0;
        int fileCount = 0;
        
        ScanResult() = default;
    };

    // File format detection
    static EtiFileFormat detectFileFormat(const QString& filePath);
    static EtiFileFormat detectFormatFromData(const QByteArray& data);
    static bool isEtiFile(const QString& filePath);
    static bool isValidEtiExtension(const QString& extension);

    // File validation
    static ValidationResult validateEtiFile(const QString& filePath);
    static bool validateEtiHeader(const QByteArray& headerData);
    static bool validateEtiFrame(const QByteArray& frameData);

    // File size and statistics
    static qint64 getFileSize(const QString& filePath);
    static qint64 estimateFrameCount(const QString& filePath, EtiFileFormat format);
    static QPair<qint64, qint64> getFrameRange(const QString& filePath, EtiFileFormat format);
    static double calculateFileDuration(qint64 frameCount, double frameRate = 24.0);

    // File operations
    static bool copyFile(const QString& source, const QString& destination, bool overwrite = false);
    static bool moveFile(const QString& source, const QString& destination, bool overwrite = false);
    static bool deleteFile(const QString& filePath);
    static bool createBackup(const QString& filePath, const QString& backupSuffix = ".bak");

    // Directory operations
    static bool createDirectory(const QString& dirPath);
    static bool removeDirectory(const QString& dirPath, bool recursive = false);
    static ScanResult scanDirectory(const QString& dirPath, bool recursive = true);
    static QStringList findEtiFiles(const QString& dirPath, bool recursive = true);

    // Path utilities
    static QString getCanonicalPath(const QString& path);
    static QString getRelativePath(const QString& basePath, const QString& targetPath);
    static QString generateUniqueFileName(const QString& basePath, const QString& baseName, const QString& extension);
    static QString sanitizeFileName(const QString& fileName);

    // Temporary file operations
    static QString createTempFile(const QString& templateName = "eti_temp_XXXXXX");
    static QString createTempDirectory(const QString& templateName = "eti_temp_dir_XXXXXX");
    static bool cleanupTempFiles(const QStringList& tempFiles);

    // File content operations
    static QByteArray readFileChunk(const QString& filePath, qint64 offset, qint64 size);
    static bool writeFileChunk(const QString& filePath, qint64 offset, const QByteArray& data);
    static QByteArray calculateFileHash(const QString& filePath, const QString& algorithm = "MD5");
    static bool compareFiles(const QString& file1, const QString& file2);

    // ETI-specific utilities
    static int extractEtiFrameSize(EtiFileFormat format);
    static QByteArray extractEtiSyncPattern(EtiFileFormat format);
    static bool findEtiSyncPattern(const QByteArray& data, int& position);
    static QStringList getSupportedEtiExtensions();

    // Error handling
    static QString getLastErrorString();
    static bool hasError();
    static void clearError();

private:
    // Internal validation helpers
    static bool validateEtiNiFrame(const QByteArray& frameData);
    static bool validateEtiLiFrame(const QByteArray& frameData);
    static bool validateEdiFrame(const QByteArray& frameData);
    
    // Internal utility methods
    static bool isReadableFile(const QString& filePath);
    static bool isWritableLocation(const QString& dirPath);
    static QByteArray readFileHeader(const QString& filePath, int headerSize = 1024);
    
    // Error tracking
    static thread_local QString s_lastError;
    static void setError(const QString& error);
};