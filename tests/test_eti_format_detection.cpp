/**
 * @file test_eti_format_detection.cpp
 * @brief Unit tests for ETI format auto-detection (ETI-LI vs ETI-NI)
 *
 * Tests automatic detection of ETI format variants based on sync patterns:
 * - ETI-LI: 0x49 0x93 0x1E 0x03 (Linear, most common)
 * - ETI-NI: 0xFF 0xF8 or 0xFF 0x07 (Network Independent)
 *
 * Reference: ETSI EN 300 799 Section 5.1
 *
 * @author Agent 43: ETI Format Auto-Detection
 * @date 2025-11-07
 */

#include <QtTest/QtTest>
#include <QFile>
#include <QByteArray>
#include "../src/core/eti_types.hpp"

class TestETIFormatDetection : public QObject
{
    Q_OBJECT

private slots:
    // ========================================================================
    // BASIC DETECTION TESTS (5 tests)
    // ========================================================================

    /**
     * @brief Test detection of standard ETI-LI format
     */
    void testDetectsETI_LI()
    {
        uint8_t data[] = {0x49, 0x93, 0x1E, 0x03, 0x00, 0x00, 0x00, 0x00};
        eti::ETIFormat format = eti::detectETIFormat(data, sizeof(data));
        QCOMPARE(format, eti::ETIFormat::ETI_LI);
    }

    /**
     * @brief Test detection of ETI-NI variant 1 (0xFF 0xF8)
     */
    void testDetectsETI_NI_Variant1()
    {
        uint8_t data[] = {0xFF, 0xF8, 0xC5, 0x49, 0x00, 0x00, 0x00, 0x00};
        eti::ETIFormat format = eti::detectETIFormat(data, sizeof(data));
        QCOMPARE(format, eti::ETIFormat::ETI_NI);
    }

    /**
     * @brief Test detection of ETI-NI variant 2 (0xFF 0x07)
     */
    void testDetectsETI_NI_Variant2()
    {
        uint8_t data[] = {0xFF, 0x07, 0x3A, 0xB6, 0x00, 0x00, 0x00, 0x00};
        eti::ETIFormat format = eti::detectETIFormat(data, sizeof(data));
        QCOMPARE(format, eti::ETIFormat::ETI_NI);
    }

    /**
     * @brief Test rejection of invalid sync pattern
     */
    void testRejectsInvalidData()
    {
        uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        eti::ETIFormat format = eti::detectETIFormat(data, sizeof(data));
        QCOMPARE(format, eti::ETIFormat::UNKNOWN);
    }

    /**
     * @brief Test formatToString conversion
     */
    void testFormatToString()
    {
        QCOMPARE(QString(eti::formatToString(eti::ETIFormat::ETI_LI)), QString("ETI-LI"));
        QCOMPARE(QString(eti::formatToString(eti::ETIFormat::ETI_NI)), QString("ETI-NI"));
        QCOMPARE(QString(eti::formatToString(eti::ETIFormat::ETI_NA)), QString("ETI-NA"));
        QCOMPARE(QString(eti::formatToString(eti::ETIFormat::UNKNOWN)), QString("UNKNOWN"));
    }

    // ========================================================================
    // EDGE CASE TESTS (5 tests)
    // ========================================================================

    /**
     * @brief Test with buffer too small (< 4 bytes)
     */
    void testBufferTooSmall()
    {
        uint8_t data[] = {0x49, 0x93, 0x1E};  // Only 3 bytes
        eti::ETIFormat format = eti::detectETIFormat(data, 3);
        QCOMPARE(format, eti::ETIFormat::UNKNOWN);
    }

    /**
     * @brief Test with null pointer
     */
    void testNullPointer()
    {
        eti::ETIFormat format = eti::detectETIFormat(nullptr, 100);
        QCOMPARE(format, eti::ETIFormat::UNKNOWN);
    }

    /**
     * @brief Test with zero size
     */
    void testZeroSize()
    {
        uint8_t data[] = {0x49, 0x93, 0x1E, 0x03};
        eti::ETIFormat format = eti::detectETIFormat(data, 0);
        QCOMPARE(format, eti::ETIFormat::UNKNOWN);
    }

    /**
     * @brief Test with partial ETI-LI sync (first 2 bytes correct)
     */
    void testPartialETI_LISync()
    {
        uint8_t data[] = {0x49, 0x93, 0x00, 0x00};  // Only first 2 bytes match
        eti::ETIFormat format = eti::detectETIFormat(data, sizeof(data));
        QCOMPARE(format, eti::ETIFormat::UNKNOWN);
    }

    /**
     * @brief Test with partial ETI-NI sync (first byte correct)
     */
    void testPartialETI_NISync()
    {
        uint8_t data[] = {0xFF, 0x00, 0x00, 0x00};  // Only first byte matches
        eti::ETIFormat format = eti::detectETIFormat(data, sizeof(data));
        QCOMPARE(format, eti::ETIFormat::UNKNOWN);
    }

    // ========================================================================
    // BOUNDARY TESTS (3 tests)
    // ========================================================================

    /**
     * @brief Test with exactly 4 bytes (minimum valid)
     */
    void testExactlyFourBytes()
    {
        uint8_t data[] = {0x49, 0x93, 0x1E, 0x03};
        eti::ETIFormat format = eti::detectETIFormat(data, 4);
        QCOMPARE(format, eti::ETIFormat::ETI_LI);
    }

    /**
     * @brief Test with large buffer (6144 bytes)
     */
    void testLargeBuffer()
    {
        uint8_t data[6144];
        data[0] = 0xFF;
        data[1] = 0xF8;
        data[2] = 0x00;
        data[3] = 0x00;
        eti::ETIFormat format = eti::detectETIFormat(data, 6144);
        QCOMPARE(format, eti::ETIFormat::ETI_NI);
    }

    /**
     * @brief Test detection with trailing garbage
     */
    void testTrailingGarbage()
    {
        uint8_t data[] = {0x49, 0x93, 0x1E, 0x03, 0xAA, 0xBB, 0xCC, 0xDD};
        eti::ETIFormat format = eti::detectETIFormat(data, sizeof(data));
        QCOMPARE(format, eti::ETIFormat::ETI_LI);  // Should still detect ETI-LI
    }

    // ========================================================================
    // REAL FILE INTEGRATION TEST (1 test)
    // ========================================================================

    /**
     * @brief Test detection with real Bangkok ETI file (ETI-NI)
     */
    void testDetectsBangkokFile()
    {
        QFile file("../eti/bkk_20062022_141637.eti");
        if (!file.exists()) {
            QSKIP("Bangkok ETI file not found, skipping integration test");
        }

        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray data = file.read(16);
        QVERIFY(data.size() >= 4);

        eti::ETIFormat format = eti::detectETIFormat(
            reinterpret_cast<const uint8_t*>(data.data()),
            static_cast<size_t>(data.size())
        );

        // Bangkok file should be ETI-NI (0xFF 0xF8)
        QCOMPARE(format, eti::ETIFormat::ETI_NI);

        // Verify exact sync bytes
        QCOMPARE(static_cast<uint8_t>(data[0]), static_cast<uint8_t>(0xFF));
        QCOMPARE(static_cast<uint8_t>(data[1]), static_cast<uint8_t>(0xF8));

        file.close();
    }

    // ========================================================================
    // FALSE POSITIVE TESTS (2 tests)
    // ========================================================================

    /**
     * @brief Test that random data doesn't match ETI-LI
     */
    void testRandomDataNotETI_LI()
    {
        uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
        eti::ETIFormat format = eti::detectETIFormat(data, sizeof(data));
        QCOMPARE(format, eti::ETIFormat::UNKNOWN);
    }

    /**
     * @brief Test that 0xFF alone doesn't match ETI-NI
     */
    void testSingleFFNotETI_NI()
    {
        uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF};  // All 0xFF but not valid ETI-NI
        eti::ETIFormat format = eti::detectETIFormat(data, sizeof(data));
        QCOMPARE(format, eti::ETIFormat::UNKNOWN);
    }
};

QTEST_MAIN(TestETIFormatDetection)
#include "test_eti_format_detection.moc"
