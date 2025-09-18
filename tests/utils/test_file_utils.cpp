#include <gtest/gtest.h>
#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QCryptographicHash>
#include "../../src/utils/file_utils.h"

class FileUtilsTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        // Create temporary directory for tests
        tempDir = new QTemporaryDir();
        ASSERT_TRUE(tempDir->isValid());
        tempDirPath = tempDir->path();
        
        // Clear any previous errors
        FileUtils::clearError();
    }

    void TearDown() override 
    {
        delete tempDir;
        tempDir = nullptr;
        FileUtils::clearError();
    }

    // Helper methods
    QString createTestFile(const QString& fileName, const QByteArray& content = QByteArray())
    {
        QString filePath = QDir(tempDirPath).filePath(fileName);
        QFile file(filePath);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        
        if (!content.isEmpty()) {
            file.write(content);
        } else {
            file.write("Test file content");
        }
        file.close();
        
        return filePath;
    }

    QString createTestDirectory(const QString& dirName)
    {
        QString dirPath = QDir(tempDirPath).filePath(dirName);
        QDir().mkpath(dirPath);
        return dirPath;
    }

    QByteArray createEtiNiFrame()
    {
        QByteArray frame(6144, 0x00);
        // ETI-NI sync pattern
        frame[0] = 0xFF;
        frame[1] = 0x1F;
        // Add minimal ETI-NI structure
        frame[2] = 0x01; // FC (Frame Count)
        frame[3] = 0x02; // NST (Number of Streams)
        return frame;
    }

    QByteArray createEtiLiFrame()
    {
        QByteArray frame(6144, 0x00);
        // ETI-LI sync pattern
        frame[0] = 0xFF;
        frame[1] = 0x00;
        // Add minimal ETI-LI structure
        frame[2] = 0x01;
        frame[3] = 0x02;
        return frame;
    }

    QByteArray createEdiFrame()
    {
        QByteArray frame(1024, 0x00);
        // EDI magic header
        frame[0] = 'E';
        frame[1] = 'D';
        frame[2] = 'I';
        frame[3] = 0x00;
        return frame;
    }

protected:
    QTemporaryDir* tempDir;
    QString tempDirPath;
};

// Format Detection Tests
TEST_F(FileUtilsTest, DetectEtiNiFormat)
{
    QByteArray etiData = createEtiNiFrame();
    QString filePath = createTestFile("test.eti", etiData);
    
    FileUtils::EtiFileFormat format = FileUtils::detectFileFormat(filePath);
    EXPECT_EQ(format, FileUtils::EtiFileFormat::ETI_NI);
}

TEST_F(FileUtilsTest, DetectEtiLiFormat)
{
    QByteArray etiData = createEtiLiFrame();
    QString filePath = createTestFile("test.eti", etiData);
    
    FileUtils::EtiFileFormat format = FileUtils::detectFileFormat(filePath);
    EXPECT_EQ(format, FileUtils::EtiFileFormat::ETI_LI);
}

TEST_F(FileUtilsTest, DetectEdiFormat)
{
    QByteArray ediData = createEdiFrame();
    QString filePath = createTestFile("test.edi", ediData);
    
    FileUtils::EtiFileFormat format = FileUtils::detectFileFormat(filePath);
    EXPECT_EQ(format, FileUtils::EtiFileFormat::EDI);
}

TEST_F(FileUtilsTest, DetectFormatFromData)
{
    QByteArray etiNiData = createEtiNiFrame();
    FileUtils::EtiFileFormat format = FileUtils::detectFormatFromData(etiNiData);
    EXPECT_EQ(format, FileUtils::EtiFileFormat::ETI_NI);
    
    QByteArray etiLiData = createEtiLiFrame();
    format = FileUtils::detectFormatFromData(etiLiData);
    EXPECT_EQ(format, FileUtils::EtiFileFormat::ETI_LI);
    
    QByteArray ediData = createEdiFrame();
    format = FileUtils::detectFormatFromData(ediData);
    EXPECT_EQ(format, FileUtils::EtiFileFormat::EDI);
}

TEST_F(FileUtilsTest, DetectUnknownFormat)
{
    QByteArray randomData(100, 0xAA);
    FileUtils::EtiFileFormat format = FileUtils::detectFormatFromData(randomData);
    EXPECT_EQ(format, FileUtils::EtiFileFormat::Raw);
}

TEST_F(FileUtilsTest, DetectFormatNonExistentFile)
{
    FileUtils::EtiFileFormat format = FileUtils::detectFileFormat("/non/existent/file.eti");
    EXPECT_EQ(format, FileUtils::EtiFileFormat::Unknown);
    EXPECT_TRUE(FileUtils::hasError());
}

TEST_F(FileUtilsTest, IsEtiFileValidExtension)
{
    QString filePath = createTestFile("test.eti");
    EXPECT_TRUE(FileUtils::isEtiFile(filePath));
    
    filePath = createTestFile("test.ett");
    EXPECT_TRUE(FileUtils::isEtiFile(filePath));
    
    filePath = createTestFile("test.edi");
    EXPECT_TRUE(FileUtils::isEtiFile(filePath));
}

TEST_F(FileUtilsTest, IsEtiFileInvalidExtension)
{
    QString filePath = createTestFile("test.txt");
    EXPECT_FALSE(FileUtils::isEtiFile(filePath));
    
    filePath = createTestFile("test.mp3");
    EXPECT_FALSE(FileUtils::isEtiFile(filePath));
}

TEST_F(FileUtilsTest, IsValidEtiExtension)
{
    EXPECT_TRUE(FileUtils::isValidEtiExtension("eti"));
    EXPECT_TRUE(FileUtils::isValidEtiExtension("ETI"));
    EXPECT_TRUE(FileUtils::isValidEtiExtension("ett"));
    EXPECT_TRUE(FileUtils::isValidEtiExtension("edi"));
    EXPECT_TRUE(FileUtils::isValidEtiExtension("dab"));
    EXPECT_TRUE(FileUtils::isValidEtiExtension("raw"));
    EXPECT_TRUE(FileUtils::isValidEtiExtension("bin"));
    
    EXPECT_FALSE(FileUtils::isValidEtiExtension("txt"));
    EXPECT_FALSE(FileUtils::isValidEtiExtension("mp3"));
    EXPECT_FALSE(FileUtils::isValidEtiExtension("doc"));
}

// File Validation Tests
TEST_F(FileUtilsTest, ValidateValidEtiFile)
{
    QByteArray etiData = createEtiNiFrame();
    QString filePath = createTestFile("valid.eti", etiData);
    
    FileUtils::ValidationResult result = FileUtils::validateEtiFile(filePath);
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.format, FileUtils::EtiFileFormat::ETI_NI);
    EXPECT_EQ(result.fileSize, etiData.size());
    EXPECT_GT(result.estimatedFrameCount, 0);
    EXPECT_TRUE(result.errorMessage.isEmpty());
}

TEST_F(FileUtilsTest, ValidateEmptyFile)
{
    QString filePath = createTestFile("empty.eti", QByteArray());
    
    FileUtils::ValidationResult result = FileUtils::validateEtiFile(filePath);
    
    EXPECT_FALSE(result.isValid);
    EXPECT_EQ(result.fileSize, 0);
    EXPECT_FALSE(result.errorMessage.isEmpty());
}

TEST_F(FileUtilsTest, ValidateNonExistentFile)
{
    FileUtils::ValidationResult result = FileUtils::validateEtiFile("/non/existent/file.eti");
    
    EXPECT_FALSE(result.isValid);
    EXPECT_FALSE(result.errorMessage.isEmpty());
}

TEST_F(FileUtilsTest, ValidateEtiHeader)
{
    QByteArray etiNiHeader = createEtiNiFrame().left(1024);
    EXPECT_TRUE(FileUtils::validateEtiHeader(etiNiHeader));
    
    QByteArray etiLiHeader = createEtiLiFrame().left(1024);
    EXPECT_TRUE(FileUtils::validateEtiHeader(etiLiHeader));
    
    QByteArray ediHeader = createEdiFrame().left(1024);
    EXPECT_TRUE(FileUtils::validateEtiHeader(ediHeader));
    
    QByteArray invalidHeader(10, 0xAA);
    EXPECT_FALSE(FileUtils::validateEtiHeader(invalidHeader));
}

TEST_F(FileUtilsTest, ValidateEtiFrame)
{
    QByteArray validFrame = createEtiNiFrame();
    EXPECT_TRUE(FileUtils::validateEtiFrame(validFrame));
    
    QByteArray emptyFrame;
    EXPECT_FALSE(FileUtils::validateEtiFrame(emptyFrame));
}

// File Size and Statistics Tests
TEST_F(FileUtilsTest, GetFileSize)
{
    QByteArray testData(1000, 'A');
    QString filePath = createTestFile("size_test.eti", testData);
    
    qint64 size = FileUtils::getFileSize(filePath);
    EXPECT_EQ(size, 1000);
    
    // Non-existent file
    qint64 invalidSize = FileUtils::getFileSize("/non/existent/file.eti");
    EXPECT_EQ(invalidSize, -1);
}

TEST_F(FileUtilsTest, EstimateFrameCount)
{
    QByteArray etiData = createEtiNiFrame() + createEtiNiFrame() + createEtiNiFrame();
    QString filePath = createTestFile("frames.eti", etiData);
    
    qint64 frameCount = FileUtils::estimateFrameCount(filePath, FileUtils::EtiFileFormat::ETI_NI);
    EXPECT_EQ(frameCount, 3);
    
    // Unknown format should return 0
    frameCount = FileUtils::estimateFrameCount(filePath, FileUtils::EtiFileFormat::Unknown);
    EXPECT_EQ(frameCount, 0);
}

TEST_F(FileUtilsTest, GetFrameRange)
{
    QByteArray etiData = createEtiNiFrame() + createEtiNiFrame();
    QString filePath = createTestFile("range.eti", etiData);
    
    auto range = FileUtils::getFrameRange(filePath, FileUtils::EtiFileFormat::ETI_NI);
    EXPECT_EQ(range.first, 0);
    EXPECT_EQ(range.second, 1); // 2 frames: 0-1
}

TEST_F(FileUtilsTest, CalculateFileDuration)
{
    qint64 frameCount = 100;
    double duration = FileUtils::calculateFileDuration(frameCount, 25.0);
    EXPECT_DOUBLE_EQ(duration, 4.0); // 100 frames / 25 fps = 4 seconds
    
    // Test with default frame rate
    duration = FileUtils::calculateFileDuration(frameCount);
    EXPECT_GT(duration, 0.0);
}

// File Operations Tests
TEST_F(FileUtilsTest, CopyFileSuccess)
{
    QString sourceFile = createTestFile("source.eti", "test content");
    QString destFile = QDir(tempDirPath).filePath("destination.eti");
    
    bool result = FileUtils::copyFile(sourceFile, destFile);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(destFile));
    
    // Verify content
    QFile source(sourceFile);
    QFile dest(destFile);
    EXPECT_TRUE(source.open(QIODevice::ReadOnly));
    EXPECT_TRUE(dest.open(QIODevice::ReadOnly));
    EXPECT_EQ(source.readAll(), dest.readAll());
}

TEST_F(FileUtilsTest, CopyFileOverwriteProtection)
{
    QString sourceFile = createTestFile("source.eti", "source content");
    QString destFile = createTestFile("existing.eti", "existing content");
    
    // Without overwrite flag
    bool result = FileUtils::copyFile(sourceFile, destFile, false);
    EXPECT_FALSE(result);
    EXPECT_TRUE(FileUtils::hasError());
    
    FileUtils::clearError();
    
    // With overwrite flag
    result = FileUtils::copyFile(sourceFile, destFile, true);
    EXPECT_TRUE(result);
    EXPECT_FALSE(FileUtils::hasError());
}

TEST_F(FileUtilsTest, CopyNonExistentFile)
{
    QString destFile = QDir(tempDirPath).filePath("dest.eti");
    
    bool result = FileUtils::copyFile("/non/existent/source.eti", destFile);
    EXPECT_FALSE(result);
    EXPECT_TRUE(FileUtils::hasError());
}

TEST_F(FileUtilsTest, MoveFileSuccess)
{
    QString sourceFile = createTestFile("move_source.eti", "content to move");
    QString destFile = QDir(tempDirPath).filePath("move_dest.eti");
    
    bool result = FileUtils::moveFile(sourceFile, destFile);
    
    EXPECT_TRUE(result);
    EXPECT_FALSE(QFile::exists(sourceFile));
    EXPECT_TRUE(QFile::exists(destFile));
}

TEST_F(FileUtilsTest, DeleteFileSuccess)
{
    QString testFile = createTestFile("delete_me.eti");
    EXPECT_TRUE(QFile::exists(testFile));
    
    bool result = FileUtils::deleteFile(testFile);
    EXPECT_TRUE(result);
    EXPECT_FALSE(QFile::exists(testFile));
    
    // Deleting non-existent file should succeed
    result = FileUtils::deleteFile(testFile);
    EXPECT_TRUE(result);
}

TEST_F(FileUtilsTest, CreateBackup)
{
    QString originalFile = createTestFile("original.eti", "backup this content");
    
    bool result = FileUtils::createBackup(originalFile);
    
    EXPECT_TRUE(result);
    
    QString backupFile = originalFile + ".bak";
    EXPECT_TRUE(QFile::exists(backupFile));
    
    // Verify backup content
    QFile original(originalFile);
    QFile backup(backupFile);
    EXPECT_TRUE(original.open(QIODevice::ReadOnly));
    EXPECT_TRUE(backup.open(QIODevice::ReadOnly));
    EXPECT_EQ(original.readAll(), backup.readAll());
}

// Directory Operations Tests
TEST_F(FileUtilsTest, CreateDirectory)
{
    QString newDirPath = QDir(tempDirPath).filePath("new_directory");
    
    bool result = FileUtils::createDirectory(newDirPath);
    EXPECT_TRUE(result);
    EXPECT_TRUE(QDir(newDirPath).exists());
    
    // Creating existing directory should succeed
    result = FileUtils::createDirectory(newDirPath);
    EXPECT_TRUE(result);
}

TEST_F(FileUtilsTest, CreateNestedDirectory)
{
    QString nestedPath = QDir(tempDirPath).filePath("level1/level2/level3");
    
    bool result = FileUtils::createDirectory(nestedPath);
    EXPECT_TRUE(result);
    EXPECT_TRUE(QDir(nestedPath).exists());
}

TEST_F(FileUtilsTest, RemoveDirectory)
{
    QString dirToRemove = createTestDirectory("remove_me");
    EXPECT_TRUE(QDir(dirToRemove).exists());
    
    bool result = FileUtils::removeDirectory(dirToRemove, false);
    EXPECT_TRUE(result);
    EXPECT_FALSE(QDir(dirToRemove).exists());
    
    // Removing non-existent directory should succeed
    result = FileUtils::removeDirectory(dirToRemove, false);
    EXPECT_TRUE(result);
}

TEST_F(FileUtilsTest, RemoveDirectoryRecursive)
{
    QString parentDir = createTestDirectory("parent");
    QString childDir = QDir(parentDir).filePath("child");
    QDir().mkpath(childDir);
    createTestFile(QDir(childDir).filePath("file.txt"));
    
    bool result = FileUtils::removeDirectory(parentDir, true);
    EXPECT_TRUE(result);
    EXPECT_FALSE(QDir(parentDir).exists());
}

TEST_F(FileUtilsTest, ScanDirectory)
{
    // Create test structure
    QString subDir = createTestDirectory("subdir");
    createTestFile("file1.eti", createEtiNiFrame());
    createTestFile("file2.txt", "text content");
    createTestFile(QDir(subDir).filePath("file3.eti"), createEtiLiFrame());
    
    FileUtils::ScanResult result = FileUtils::scanDirectory(tempDirPath, true);
    
    EXPECT_EQ(result.fileCount, 3);
    EXPECT_EQ(result.etiFiles.size(), 2);
    EXPECT_EQ(result.otherFiles.size(), 1);
    EXPECT_GT(result.totalSize, 0);
}

TEST_F(FileUtilsTest, FindEtiFiles)
{
    createTestFile("test1.eti", createEtiNiFrame());
    createTestFile("test2.txt", "not eti");
    createTestFile("test3.edi", createEdiFrame());
    
    QStringList etiFiles = FileUtils::findEtiFiles(tempDirPath, false);
    
    EXPECT_EQ(etiFiles.size(), 2);
    EXPECT_TRUE(etiFiles.filter(".eti").size() >= 1);
    EXPECT_TRUE(etiFiles.filter(".edi").size() >= 1);
}

// Path Utilities Tests
TEST_F(FileUtilsTest, GetCanonicalPath)
{
    QString testFile = createTestFile("canonical_test.eti");
    QString canonical = FileUtils::getCanonicalPath(testFile);
    
    EXPECT_FALSE(canonical.isEmpty());
    EXPECT_TRUE(QFileInfo(canonical).isAbsolute());
}

TEST_F(FileUtilsTest, GetRelativePath)
{
    QString basePath = tempDirPath;
    QString targetPath = QDir(tempDirPath).filePath("subdir/file.eti");
    
    QString relativePath = FileUtils::getRelativePath(basePath, targetPath);
    EXPECT_EQ(relativePath, "subdir/file.eti");
}

TEST_F(FileUtilsTest, GenerateUniqueFileName)
{
    // Create existing file
    createTestFile("existing.eti");
    
    QString uniquePath = FileUtils::generateUniqueFileName(tempDirPath, "existing", "eti");
    EXPECT_FALSE(uniquePath.isEmpty());
    EXPECT_NE(uniquePath, QDir(tempDirPath).filePath("existing.eti"));
    EXPECT_TRUE(uniquePath.contains("existing_"));
}

TEST_F(FileUtilsTest, SanitizeFileName)
{
    QString dirty = "file<name>with:invalid|chars?.txt";
    QString clean = FileUtils::sanitizeFileName(dirty);
    
    EXPECT_FALSE(clean.contains("<"));
    EXPECT_FALSE(clean.contains(">"));
    EXPECT_FALSE(clean.contains(":"));
    EXPECT_FALSE(clean.contains("|"));
    EXPECT_FALSE(clean.contains("?"));
    
    QString empty = FileUtils::sanitizeFileName("");
    EXPECT_FALSE(empty.isEmpty());
    
    QString spaces = FileUtils::sanitizeFileName("   file   ");
    EXPECT_FALSE(spaces.startsWith(" "));
    EXPECT_FALSE(spaces.endsWith(" "));
}

// Temporary File Operations Tests
TEST_F(FileUtilsTest, CreateTempFile)
{
    QString tempFile = FileUtils::createTempFile();
    
    EXPECT_FALSE(tempFile.isEmpty());
    EXPECT_TRUE(QFile::exists(tempFile));
    
    // Cleanup
    QFile::remove(tempFile);
}

TEST_F(FileUtilsTest, CreateTempDirectory)
{
    QString tempDir = FileUtils::createTempDirectory();
    
    EXPECT_FALSE(tempDir.isEmpty());
    EXPECT_TRUE(QDir(tempDir).exists());
    
    // Cleanup
    QDir(tempDir).removeRecursively();
}

TEST_F(FileUtilsTest, CleanupTempFiles)
{
    QString tempFile = FileUtils::createTempFile();
    QString tempDir = FileUtils::createTempDirectory();
    
    QStringList tempItems = {tempFile, tempDir};
    bool result = FileUtils::cleanupTempFiles(tempItems);
    
    EXPECT_TRUE(result);
    EXPECT_FALSE(QFile::exists(tempFile));
    EXPECT_FALSE(QDir(tempDir).exists());
}

// File Content Operations Tests
TEST_F(FileUtilsTest, ReadFileChunk)
{
    QByteArray fullContent = "0123456789ABCDEF";
    QString testFile = createTestFile("chunk_test.eti", fullContent);
    
    QByteArray chunk = FileUtils::readFileChunk(testFile, 5, 5);
    EXPECT_EQ(chunk, QByteArray("56789"));
    
    // Read beyond file end
    chunk = FileUtils::readFileChunk(testFile, 10, 100);
    EXPECT_EQ(chunk, QByteArray("ABCDEF"));
}

TEST_F(FileUtilsTest, WriteFileChunk)
{
    QString testFile = createTestFile("write_chunk.eti", QByteArray(20, 'X'));
    
    QByteArray newData = "12345";
    bool result = FileUtils::writeFileChunk(testFile, 5, newData);
    
    EXPECT_TRUE(result);
    
    // Verify write
    QByteArray chunk = FileUtils::readFileChunk(testFile, 5, 5);
    EXPECT_EQ(chunk, newData);
}

TEST_F(FileUtilsTest, CalculateFileHash)
{
    QByteArray testContent = "hash test content";
    QString testFile = createTestFile("hash_test.eti", testContent);
    
    QByteArray hash = FileUtils::calculateFileHash(testFile, "MD5");
    EXPECT_FALSE(hash.isEmpty());
    
    // Verify hash consistency
    QByteArray hash2 = FileUtils::calculateFileHash(testFile, "MD5");
    EXPECT_EQ(hash, hash2);
    
    // Different algorithm
    QByteArray sha1Hash = FileUtils::calculateFileHash(testFile, "SHA1");
    EXPECT_FALSE(sha1Hash.isEmpty());
    EXPECT_NE(hash, sha1Hash); // Different algorithms should give different hashes
}

TEST_F(FileUtilsTest, CompareFiles)
{
    QByteArray content = "identical content";
    QString file1 = createTestFile("compare1.eti", content);
    QString file2 = createTestFile("compare2.eti", content);
    QString file3 = createTestFile("compare3.eti", "different content");
    
    EXPECT_TRUE(FileUtils::compareFiles(file1, file2));
    EXPECT_FALSE(FileUtils::compareFiles(file1, file3));
}

// ETI-Specific Utilities Tests
TEST_F(FileUtilsTest, ExtractEtiFrameSize)
{
    EXPECT_EQ(FileUtils::extractEtiFrameSize(FileUtils::EtiFileFormat::ETI_NI), 6144);
    EXPECT_EQ(FileUtils::extractEtiFrameSize(FileUtils::EtiFileFormat::ETI_LI), 6144);
    EXPECT_EQ(FileUtils::extractEtiFrameSize(FileUtils::EtiFileFormat::EDI), 0);
    EXPECT_EQ(FileUtils::extractEtiFrameSize(FileUtils::EtiFileFormat::Unknown), 0);
}

TEST_F(FileUtilsTest, ExtractEtiSyncPattern)
{
    QByteArray niSync = FileUtils::extractEtiSyncPattern(FileUtils::EtiFileFormat::ETI_NI);
    EXPECT_EQ(niSync, QByteArray::fromHex("FF1F"));
    
    QByteArray liSync = FileUtils::extractEtiSyncPattern(FileUtils::EtiFileFormat::ETI_LI);
    EXPECT_EQ(liSync, QByteArray::fromHex("FF00"));
    
    QByteArray ediSync = FileUtils::extractEtiSyncPattern(FileUtils::EtiFileFormat::EDI);
    EXPECT_EQ(ediSync, QByteArray("EDI"));
}

TEST_F(FileUtilsTest, FindEtiSyncPattern)
{
    QByteArray data = QByteArray(100, 0x00);
    data.replace(50, 2, QByteArray::fromHex("FF1F")); // ETI-NI sync at position 50
    
    int position = -1;
    bool found = FileUtils::findEtiSyncPattern(data, position);
    
    EXPECT_TRUE(found);
    EXPECT_EQ(position, 50);
    
    // No sync pattern
    QByteArray noSync(100, 0xAA);
    found = FileUtils::findEtiSyncPattern(noSync, position);
    EXPECT_FALSE(found);
    EXPECT_EQ(position, -1);
}

TEST_F(FileUtilsTest, GetSupportedEtiExtensions)
{
    QStringList extensions = FileUtils::getSupportedEtiExtensions();
    
    EXPECT_TRUE(extensions.contains("eti"));
    EXPECT_TRUE(extensions.contains("ett"));
    EXPECT_TRUE(extensions.contains("edi"));
    EXPECT_TRUE(extensions.contains("dab"));
    EXPECT_TRUE(extensions.contains("raw"));
    EXPECT_TRUE(extensions.contains("bin"));
}

// Error Handling Tests
TEST_F(FileUtilsTest, ErrorHandling)
{
    FileUtils::clearError();
    EXPECT_FALSE(FileUtils::hasError());
    EXPECT_TRUE(FileUtils::getLastErrorString().isEmpty());
    
    // Trigger an error
    FileUtils::detectFileFormat("/non/existent/file.eti");
    
    EXPECT_TRUE(FileUtils::hasError());
    EXPECT_FALSE(FileUtils::getLastErrorString().isEmpty());
    
    FileUtils::clearError();
    EXPECT_FALSE(FileUtils::hasError());
    EXPECT_TRUE(FileUtils::getLastErrorString().isEmpty());
}

// Edge Cases and Error Conditions Tests
TEST_F(FileUtilsTest, HandleEmptyData)
{
    FileUtils::EtiFileFormat format = FileUtils::detectFormatFromData(QByteArray());
    EXPECT_EQ(format, FileUtils::EtiFileFormat::Unknown);
    
    EXPECT_FALSE(FileUtils::validateEtiHeader(QByteArray()));
    EXPECT_FALSE(FileUtils::validateEtiFrame(QByteArray()));
}

TEST_F(FileUtilsTest, HandleVerySmallData)
{
    QByteArray tinyData(5, 0x00);
    FileUtils::EtiFileFormat format = FileUtils::detectFormatFromData(tinyData);
    EXPECT_EQ(format, FileUtils::EtiFileFormat::Unknown);
    
    EXPECT_FALSE(FileUtils::validateEtiHeader(tinyData));
}

TEST_F(FileUtilsTest, HandleLargeFiles)
{
    // Create a large ETI file (multiple frames)
    QByteArray largeData;
    for (int i = 0; i < 100; ++i) {
        largeData += createEtiNiFrame();
    }
    
    QString largeFile = createTestFile("large.eti", largeData);
    
    FileUtils::ValidationResult result = FileUtils::validateEtiFile(largeFile);
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.estimatedFrameCount, 100);
    
    qint64 size = FileUtils::getFileSize(largeFile);
    EXPECT_EQ(size, largeData.size());
}

TEST_F(FileUtilsTest, HandleSpecialCharactersInPaths)
{
    QString specialName = "test file with spaces & symbols!.eti";
    QString specialFile = createTestFile(specialName);
    
    EXPECT_TRUE(QFile::exists(specialFile));
    
    qint64 size = FileUtils::getFileSize(specialFile);
    EXPECT_GT(size, 0);
    
    bool deleted = FileUtils::deleteFile(specialFile);
    EXPECT_TRUE(deleted);
}

// Integration Tests
TEST_F(FileUtilsTest, CompleteFileProcessingWorkflow)
{
    // Create test ETI file with multiple frames
    QByteArray etiData = createEtiNiFrame() + createEtiNiFrame() + createEtiNiFrame();
    QString originalFile = createTestFile("workflow.eti", etiData);
    
    // 1. Validate file
    FileUtils::ValidationResult validation = FileUtils::validateEtiFile(originalFile);
    EXPECT_TRUE(validation.isValid);
    EXPECT_EQ(validation.format, FileUtils::EtiFileFormat::ETI_NI);
    EXPECT_EQ(validation.estimatedFrameCount, 3);
    
    // 2. Create backup
    bool backupResult = FileUtils::createBackup(originalFile);
    EXPECT_TRUE(backupResult);
    
    // 3. Copy to new location
    QString copyFile = QDir(tempDirPath).filePath("workflow_copy.eti");
    bool copyResult = FileUtils::copyFile(originalFile, copyFile);
    EXPECT_TRUE(copyResult);
    
    // 4. Verify copy integrity
    bool filesMatch = FileUtils::compareFiles(originalFile, copyFile);
    EXPECT_TRUE(filesMatch);
    
    // 5. Calculate hash for verification
    QByteArray originalHash = FileUtils::calculateFileHash(originalFile);
    QByteArray copyHash = FileUtils::calculateFileHash(copyFile);
    EXPECT_EQ(originalHash, copyHash);
    
    // 6. Read and modify chunk
    QByteArray chunk = FileUtils::readFileChunk(copyFile, 100, 50);
    EXPECT_EQ(chunk.size(), 50);
    
    QByteArray modifiedChunk(50, 0xDD);
    bool writeResult = FileUtils::writeFileChunk(copyFile, 100, modifiedChunk);
    EXPECT_TRUE(writeResult);
    
    // 7. Verify modification
    QByteArray readBack = FileUtils::readFileChunk(copyFile, 100, 50);
    EXPECT_EQ(readBack, modifiedChunk);
    
    // 8. Cleanup
    EXPECT_TRUE(FileUtils::deleteFile(originalFile));
    EXPECT_TRUE(FileUtils::deleteFile(copyFile));
    EXPECT_TRUE(FileUtils::deleteFile(originalFile + ".bak"));
}

TEST_F(FileUtilsTest, DirectoryManagementWorkflow)
{
    // Create complex directory structure with ETI files
    QString projectDir = createTestDirectory("eti_project");
    QString dataDir = QDir(projectDir).filePath("data");
    QString backupDir = QDir(projectDir).filePath("backup");
    
    EXPECT_TRUE(FileUtils::createDirectory(dataDir));
    EXPECT_TRUE(FileUtils::createDirectory(backupDir));
    
    // Create ETI files in data directory
    createTestFile(QDir(dataDir).filePath("stream1.eti"), createEtiNiFrame());
    createTestFile(QDir(dataDir).filePath("stream2.eti"), createEtiLiFrame());
    createTestFile(QDir(dataDir).filePath("config.txt"), "config data");
    
    // Scan directory
    FileUtils::ScanResult scanResult = FileUtils::scanDirectory(projectDir, true);
    EXPECT_EQ(scanResult.etiFiles.size(), 2);
    EXPECT_EQ(scanResult.otherFiles.size(), 1);
    EXPECT_EQ(scanResult.fileCount, 3);
    
    // Find only ETI files
    QStringList etiFiles = FileUtils::findEtiFiles(projectDir, true);
    EXPECT_EQ(etiFiles.size(), 2);
    
    // Cleanup entire project
    bool cleanupResult = FileUtils::removeDirectory(projectDir, true);
    EXPECT_TRUE(cleanupResult);
    EXPECT_FALSE(QDir(projectDir).exists());
}