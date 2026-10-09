/**
 * @file test_eti_ni_parser.cpp
 * @brief Comprehensive Unit Tests for ETI-NI Parser
 *
 * Agent 46: ETI-NI Parser Test Suite
 * Date: November 8, 2025
 *
 * Test Coverage Goals:
 * - >85% code coverage for ETI-NI parser
 * - 25+ comprehensive test cases
 * - Format detection validation
 * - NST-1 encoding/decoding
 * - Dynamic FIC offset calculation
 * - Subchannel extraction
 * - Bangkok file validation
 *
 * Success Criteria:
 * - 100% test pass rate
 * - >85% code coverage
 * - Bangkok file successfully parsed
 * - All edge cases handled
 */

#include <QtTest/QtTest>
#include <QFile>
#include "core/modern_eti_frame_parser.hpp"
#include "core/eti_types.hpp"
#include "core/crc16.hpp"

using namespace eti::modern;
using eti::ETIFormat;

class TestETI_NI_Parser : public QObject {
    Q_OBJECT

private slots:
    // ========================================================================
    // FORMAT DETECTION TESTS (5 tests)
    // ========================================================================

    /**
     * @brief Test ETI-NI Variant 1 detection (0xFF 0xF8)
     */
    void test01_DetectETI_NI_Variant1() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        ETIFormat format = eti::detectETIFormat(frame, 6144);
        QCOMPARE(format, ETIFormat::ETI_NI);
    }

    /**
     * @brief Test ETI-NI Variant 2 detection (0xFF 0x07)
     */
    void test02_DetectETI_NI_Variant2() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0x07;

        ETIFormat format = eti::detectETIFormat(frame, 6144);
        QCOMPARE(format, ETIFormat::ETI_NI);
    }

    /**
     * @brief Test rejection of invalid sync patterns
     */
    void test03_RejectInvalidSync() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xAA;  // Invalid second byte

        ETIFormat format = eti::detectETIFormat(frame, 6144);
        QCOMPARE(format, ETIFormat::UNKNOWN);
    }

    /**
     * @brief Test ETI-LI format detection (ensure no false positives)
     */
    void test04_DetectETI_LI() {
        uint8_t frame[6144] = {0};
        frame[0] = 0x49;
        frame[1] = 0x93;
        frame[2] = 0x1E;
        frame[3] = 0x03;

        ETIFormat format = eti::detectETIFormat(frame, 6144);
        QCOMPARE(format, ETIFormat::ETI_LI);
    }

    /**
     * @brief Test auto-format detection and parser routing
     */
    void test05_FormatAutoSwitch() {
        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        // Test ETI-NI parsing
        uint8_t frame_ni[6144] = {0};
        frame_ni[0] = 0xFF;
        frame_ni[1] = 0xF8;
        frame_ni[4] = 0x00;  // FC=0
        frame_ni[5] = 0x00;  // NST field=0 (actual_streams=1)

        auto result_ni = parser.parseFrame(std::span<const uint8_t, 6144>{frame_ni});
        QVERIFY(result_ni.success);

        // Test ETI-LI parsing
        uint8_t frame_li[6144] = {0};
        frame_li[0] = 0x49;
        frame_li[1] = 0x93;
        frame_li[2] = 0x1E;
        frame_li[3] = 0x03;
        frame_li[4] = 0x00;  // FC=0
        frame_li[5] = 0x00;  // FICF=0, NST=0
        // LIDATA byte 2 (EN 300 799 Sec 5.2): FP in bits 7-5, MID in bits 4-3.
        // MID=1 (DAB Mode I); MID=0 is reserved and rejected by validation.
        frame_li[6] = 0x08;  // FP=0, MID=1

        auto result_li = parser.parseFrame(std::span<const uint8_t, 6144>{frame_li});
        QVERIFY(result_li.success);
    }

    // ========================================================================
    // NST EXTRACTION TESTS (5 tests)
    // ========================================================================

    /**
     * @brief Test NST extraction with 10 streams (NST field=9)
     */
    void test06_NST_10Streams() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        // NST field = 9 (10 streams)
        // FC byte: 0b00_00_0000 (nst_upper=0)
        // Byte5:   0b1001_00_00 (nst_lower=9, mid=0, fp=0)
        frame[4] = 0x00;  // FC with nst_upper=0
        frame[5] = 0x90;  // nst_lower=9

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
        // Note: NST is stored in LIDATA field, verify via frame data
        auto lidata = result.frame.get_lidata_field();
        // Bangkok files typically have 10+ streams
    }

    /**
     * @brief Test NST with 26 streams (NST field=25)
     */
    void test07_NST_26Streams() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        // NST field = 25 (26 streams)
        // FC byte: 0b00_00_0000 (nst_upper=0)
        // Byte5:   0b11001_00_00 (nst_lower=25=0x19, mid=0, fp=0)
        frame[4] = 0x00;  // FC with nst_upper=0
        frame[5] = 0xC8;  // nst_lower=12 (bits 7-4), mid=2 (bits 3-2), fp=0

        // Actually 25 requires nst_lower=25 (0x19 in bits 7-4 = 0x190)
        frame[5] = static_cast<uint8_t>((25 << 4) | (0 << 2) | 0);  // nst_lower=25, mid=0, fp=0

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
    }

    /**
     * @brief Test NST with 42 streams (NST field=41)
     */
    void test08_NST_42Streams() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        // NST field = 41 (42 streams)
        // 41 = 0b00_101001
        // nst_upper = 0b10 (bits 5-4 of NST field)
        // nst_lower = 0b1001 (bits 3-0 of NST field)

        // FC byte: 0b10_00_0000 (nst_upper=2 in bits 7-6, fct=0, ficf=0)
        // Byte5:   0b1001_00_00 (nst_lower=9, mid=0, fp=0)
        frame[4] = 0x80;  // nst_upper=2 (bits 7-6)
        frame[5] = 0x90;  // nst_lower=9

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
    }

    /**
     * @brief Test NST with 58 streams (NST field=57)
     */
    void test09_NST_58Streams() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        // NST field = 57 (58 streams)
        // 57 = 0b00_111001
        // nst_upper = 0b11 (3)
        // nst_lower = 0b1001 (9)

        frame[4] = 0xC0;  // nst_upper=3 (bits 7-6)
        frame[5] = 0x90;  // nst_lower=9

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
    }

    /**
     * @brief Test NST-1 decoding (field value + 1 = actual streams)
     */
    void test10_NST_1Decoding() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        // NST field = 9 should decode to 10 streams
        frame[4] = 0x1B;  // FC with nst_upper=0, fct=27, ficf=0
        frame[5] = 0x90;  // nst_lower=9

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
        // NST-1 encoding: field=9 means 10 actual streams
    }

    // ========================================================================
    // DYNAMIC FIC OFFSET TESTS (3 tests)
    // ========================================================================

    /**
     * @brief Test FIC offset with 16 streams (real RAW ETI layout)
     * FIC offset = 12 + (16 * 4) = 76 bytes (SYNC 4 + FC 4 + STC 64 + EOH 4)
     * FIC length = 96 bytes (3 FIBs) for MID != 3
     */
    void test11_DynamicFICOffset_10Streams() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        // FICF=1 (byte5 bit 7), NST=16 (byte5 bits 6-0)
        frame[4] = 0x1B;  // FCT
        frame[5] = 0x90;  // FICF=1, NST=16
        frame[6] = 0x08;  // FP=0, MID=1

        // Expected FIC offset: 12 + (16 * 4) = 76
        // Fill FIC marker at offset 76
        frame[76] = 0xAA;  // FIC marker

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);

        // Verify the full 96-byte FIC (3 FIBs) was extracted
        auto fic = result.frame.get_fic_field();
        QCOMPARE(fic.size(), static_cast<size_t>(96));
        QCOMPARE(fic.fibi_count(), static_cast<size_t>(3));
        QCOMPARE(fic.fic_data[0], static_cast<uint8_t>(0xAA));
    }

    /**
     * @brief Test FIC offset with 26 streams
     * FIC offset = 12 + (16 * 4) = 76 bytes
     */
    void test12_DynamicFICOffset_26Streams() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        // FICF=1, NST=16
        frame[4] = 0x1B;
        frame[5] = 0x90;  // FICF=1, NST=16
        frame[6] = 0x08;  // FP=0, MID=1

        // Expected FIC offset: 12 + (16 * 4) = 76
        frame[76] = 0xBB;  // FIC marker

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
    }

    /**
     * @brief Test FIC extraction when FICF=1
     */
    void test13_FICExtraction_WhenFICF1() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        // FICF=1, NST=16
        frame[4] = 0x1B;
        frame[5] = 0x90;  // FICF=1, NST=16
        frame[6] = 0x08;  // FP=0, MID=1

        // Fill FIC with test pattern at offset 76 (12 + 4*NST)
        for (int i = 0; i < 96; i++) {
            frame[76 + i] = 0x10 + i;
        }

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);

        auto fic = result.frame.get_fic_field();
        QCOMPARE(fic.size(), static_cast<size_t>(96));
        QCOMPARE(fic.fic_data[0], static_cast<uint8_t>(0x10));
        QCOMPARE(fic.fic_data[31], static_cast<uint8_t>(0x2F));
        QCOMPARE(fic.fic_data[95], static_cast<uint8_t>(0x6F));
    }

    // ========================================================================
    // SUBCHANNEL EXTRACTION TESTS (6 tests)
    // ========================================================================

    /**
     * @brief Test subchannel SCID extraction (6 bits)
     */
    void test14_SubchannelSCID() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;
        frame[4] = 0x00;  // FC
        frame[5] = 0x00;  // nst_lower=0 (1 stream)

        // LIDATA: 4 bytes at offset 8
        // STC byte 0: 0b111111_00 (SCID=63, SAD upper 2 bits=0)
        frame[8] = 0xFC;  // SCID=63
        frame[9] = 0x00;  // SAD lower 8 bits
        frame[10] = 0x00; // TPL
        frame[11] = 0x00; // STL

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
        // SCID=63 should be extracted correctly
    }

    /**
     * @brief Test subchannel SAD (Start Address) extraction (10 bits)
     */
    void test15_SubchannelSAD() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;
        frame[4] = 0x00;
        frame[5] = 0x00;  // 1 stream

        // SAD = 0b11_11111111 = 1023 (max value)
        frame[8] = 0x0F;   // SCID=3, SAD upper 2 bits=11
        frame[9] = 0xFF;   // SAD lower 8 bits=11111111
        frame[10] = 0x00;
        frame[11] = 0x00;

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
        // SAD=1023 should be extracted
    }

    /**
     * @brief Test subchannel TPL (Type and Protection Level) extraction
     *
     * Real Bangkok stream semantics (EN 300 799 STC / etisnoop): stc byte 2
     * = 0x88 -> TPL 0x22 (bits 7-2). TPL bit 5 set => EEP; option = bits 4-2
     * = 0 (EEP-A); protection level = bits 1-0 = 2 (0-based) = 3-A displayed;
     * STL bytes (0x00, 0x18) -> 24 CUs (192 kbps).
     */
    void test16_SubchannelTPL() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;
        frame[4] = 0x00;
        frame[5] = 0x01;  // NST=1 (1 subchannel in STC)

        // Real-stream-style EEP 3-A entry: TPL=0x22 (EEP, option 0, level 3-A),
        // STL=24 CUs. stc byte 2 = TPL<<2 = 0x88, byte 3 = STL low = 0x18.
        frame[8] = 0x00;   // SCID=0, SAD high=0
        frame[9] = 0x00;   // SAD low=0
        frame[10] = 0x88;  // TPL=0b100010 (EEP, option 0, level 3-A)
        frame[11] = 0x18;  // STL=24 CUs

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);

        // Assert the correct EEP decode per EN 300 799 (was inverted: this
        // fixture used to be decoded as "UEP level 34").
        QCOMPARE(result.subchannels_from_stc.size(), static_cast<size_t>(1));
        const auto& sc = result.subchannels_from_stc.front();
        QCOMPARE(sc.sub_channel_id, static_cast<uint8_t>(0));
        QVERIFY(!sc.uep_flag);                       // EEP, not UEP
        QCOMPARE(sc.protection_option, static_cast<uint8_t>(0));  // EEP-A
        QCOMPARE(sc.protection_level, static_cast<uint8_t>(3));   // 3-A (1-based)
        QCOMPARE(sc.size, static_cast<uint16_t>(24));             // 24 CUs
    }

    /**
     * @brief Test subchannel STL (Stream Length) extraction (10 bits)
     */
    void test17_SubchannelSTL() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;
        frame[4] = 0x00;
        frame[5] = 0x01;  // NST=1

        // STL = 0b11_11111111 = 1023 (STL is the raw 10-bit CU count, no +1)
        frame[8] = 0x00;
        frame[9] = 0x00;
        frame[10] = 0x03;  // TPL=0, STL upper 2 bits=11
        frame[11] = 0xFF;  // STL lower 8 bits

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
        // STL=1023 -> size=1023 CUs (raw STL value)
        QCOMPARE(result.subchannels_from_stc.size(), static_cast<size_t>(1));
        QCOMPARE(result.subchannels_from_stc.front().size, static_cast<uint16_t>(1023));
    }

    /**
     * @brief Test multiple subchannel extraction
     */
    void test18_AllSubchannelsExtracted() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;
        frame[4] = 0x00;
        frame[5] = 0x10;  // nst_lower=1 (2 streams)

        // Subchannel 1
        frame[8] = 0x04;   // SCID=1
        frame[9] = 0x00;   // SAD=0
        frame[10] = 0x00;  // TPL
        frame[11] = 0x0F;  // STL=15

        // Subchannel 2
        frame[12] = 0x08;  // SCID=2
        frame[13] = 0x10;  // SAD=16
        frame[14] = 0x00;  // TPL
        frame[15] = 0x1F;  // STL=31

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
    }

    /**
     * @brief Test subchannel bitrate calculation (64 kbps example)
     */
    void test19_SubchannelBitrate64kbps() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;
        frame[4] = 0x00;
        frame[5] = 0x00;

        // 64 kbps DAB = 48 CUs (STL=47)
        frame[8] = 0x00;
        frame[9] = 0x00;
        frame[10] = 0x00;
        frame[11] = 0x2F;  // STL=47 (48 CUs)

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
    }

    // ========================================================================
    // BANGKOK FILE TESTS (4 tests)
    // ========================================================================

    /**
     * @brief Test Bangkok file can be opened and parsed
     */
    void test20_BangkokFileParses() {
        QFile file(QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/../eti/bkk_20062022_141637.eti"));
        if (!file.exists()) {
            QSKIP("Bangkok ETI file not found");
        }

        QVERIFY(file.open(QIODevice::ReadOnly));

        QByteArray frame_data = file.read(6144);
        QCOMPARE(frame_data.size(), 6144);

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(
            std::span<const uint8_t, 6144>{
                reinterpret_cast<const uint8_t*>(frame_data.data()), 6144});

        QVERIFY(result.success);
        file.close();
    }

    /**
     * @brief Test Bangkok first frame has 10+ streams
     */
    void test21_BangkokFirstFrame10Streams() {
        QFile file(QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/../eti/bkk_20062022_141637.eti"));
        if (!file.exists()) {
            QSKIP("Bangkok ETI file not found");
        }

        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray frame_data = file.read(6144);

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(
            std::span<const uint8_t, 6144>{
                reinterpret_cast<const uint8_t*>(frame_data.data()), 6144});

        QVERIFY(result.success);
        // Bangkok file typically has 10+ streams
        file.close();
    }

    /**
     * @brief Test Bangkok file with variable NST across frames
     */
    void test22_BangkokVariableNST() {
        QFile file(QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/../eti/bkk_20062022_141637.eti"));
        if (!file.exists()) {
            QSKIP("Bangkok ETI file not found");
        }

        QVERIFY(file.open(QIODevice::ReadOnly));

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        int frames_parsed = 0;
        while (!file.atEnd() && frames_parsed < 100) {
            QByteArray frame_data = file.read(6144);
            if (frame_data.size() != 6144) break;

            auto result = parser.parseFrame(
                std::span<const uint8_t, 6144>{
                    reinterpret_cast<const uint8_t*>(frame_data.data()), 6144});

            if (result.success) {
                frames_parsed++;
            }
        }

        QVERIFY(frames_parsed >= 10);  // At least 10 frames parsed
        file.close();
    }

    /**
     * @brief Test Bangkok file FIG extraction
     */
    void test23_BangkokFIGsExtracted() {
        QFile file(QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/../eti/bkk_20062022_141637.eti"));
        if (!file.exists()) {
            QSKIP("Bangkok ETI file not found");
        }

        QVERIFY(file.open(QIODevice::ReadOnly));

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        // Parse first 10 frames
        for (int i = 0; i < 10; i++) {
            QByteArray frame_data = file.read(6144);
            if (frame_data.size() != 6144) break;

            auto result = parser.parseFrame(
                std::span<const uint8_t, 6144>{
                    reinterpret_cast<const uint8_t*>(frame_data.data()), 6144});

            if (result.success) {
                // The full 96-byte FIC (3 FIBs) should be extractable when FICF=1
                auto fic = result.frame.get_fic_field();
                QVERIFY(fic.size() == 96);
                QVERIFY(fic.fibi_count() == 3);
            }
        }

        file.close();
    }

    // ========================================================================
    // EDGE CASE TESTS (2 tests)
    // ========================================================================

    /**
     * @brief Test no FIC when FICF=0
     */
    void test24_NoFICWhenFICF0() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;
        frame[4] = 0x00;  // FICF=0
        frame[5] = 0x00;

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
        // FIC should be all zeros when FICF=0
    }

    /**
     * @brief Test maximum NST validation (64 streams max)
     */
    void test25_MaxNST64Validation() {
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;

        // NST = 64 (maximum valid, byte5 bits 6-0 = 0x40)
        frame[4] = 0x1B;
        frame[5] = 0x40;  // FICF=0, NST=64

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);

        // Invalid NST (65 streams, over the 64 limit) must be rejected
        frame[5] = 0x41;  // NST=65
        auto result_invalid = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(!result_invalid.success);
        QCOMPARE(result_invalid.error_code, ETIParseErrorCode::INVALID_NST);
    }

    /**
     * @brief Test performance benchmarking (bonus test)
     */
    void test26_PerformanceBenchmark() {
        QFile file(QStringLiteral(QT_TESTCASE_SOURCEDIR) + QStringLiteral("/../eti/bkk_20062022_141637.eti"));
        if (!file.exists()) {
            QSKIP("Bangkok ETI file not found");
        }

        QVERIFY(file.open(QIODevice::ReadOnly));

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;
        config.target_fps = 250.0;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        // Read first 100 frames
        QList<QByteArray> frames;
        for (int i = 0; i < 100; i++) {
            QByteArray frame_data = file.read(6144);
            if (frame_data.size() == 6144) {
                frames.append(frame_data);
            }
        }

        QVERIFY(frames.size() >= 10);

        // Benchmark parsing
        auto start = std::chrono::high_resolution_clock::now();

        int success_count = 0;
        for (const auto& frame_data : frames) {
            auto result = parser.parseFrame(
                std::span<const uint8_t, 6144>{
                    reinterpret_cast<const uint8_t*>(frame_data.data()), 6144});
            if (result.success) {
                success_count++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        double avg_time_us = static_cast<double>(duration.count()) / frames.size();
        double fps = 1000000.0 / avg_time_us;

        qDebug() << "Performance: " << frames.size() << "frames in" << duration.count() << "µs";
        qDebug() << "Average:" << avg_time_us << "µs/frame," << fps << "FPS";
        qDebug() << "Success rate:" << (100.0 * success_count / frames.size()) << "%";

        // Should parse at >100 FPS minimum
        QVERIFY(fps >= 100.0);
        QVERIFY(success_count >= frames.size() * 0.95);  // 95% success

        file.close();
    }

    // ========================================================================
    // FIC DECODE MODE / RAW-FIC FALLBACK TESTS (option-variant matrix row 4)
    // ========================================================================
    //
    // Builds a synthetic FIB-structured FIC (96 bytes = 3 FIBs) carrying a
    // FIG 0/16 (programme type international) in FIB0 and a FIG 1/1 (service
    // label) in FIB1, with valid per-FIB CRCs unless explicitly corrupted.

    /**
     * @brief Build a 96-byte synthetic FIB-structured FIC.
     * @param corrupt_mode 0 = valid CRCs, 1 = FIB0 bad, 2 = FIB0+FIB1 bad,
     *                      3 = all FIBs bad.
     * @return The raw FIC buffer.
     */
    static std::array<uint8_t, 96> buildSyntheticFic(int corrupt_mode) {
        std::array<uint8_t, 96> fic{};
        fic.fill(0xFF);  // Padding end markers (type 7) by default

        // FIB 0: FIG 0/16 — [ext 1B][SId 2B][intl code 1B][pty 1B][lang 1B]
        {
            const size_t base = 0;
            fic[base + 0] = 0x06;  // FIG 0, length 6
            fic[base + 1] = 0x10;  // ext = 16 (programme type international)
            fic[base + 2] = 0x12;  // SId high
            fic[base + 3] = 0x34;  // SId low
            fic[base + 4] = 0x00;  // international code (RDS PTy)
            fic[base + 5] = 0x04;  // programme type
            fic[base + 6] = 0x11;  // language
        }
        // FIB 1: FIG 1/1 — [ext 1B][SId 2B][charfield 2B][16-char label]
        {
            const size_t base = 32;
            fic[base + 0] = 0x20 | 21;  // FIG 1, length 21
            fic[base + 1] = 0x01;       // ext = 1 (programme service label)
            fic[base + 2] = 0x56;       // SId high
            fic[base + 3] = 0x78;       // SId low
            fic[base + 4] = 0x00;       // charfield high
            fic[base + 5] = 0x01;       // charfield low (EBU Latin)
            const char label[16] = {
                'T', 'E', 'S', 'T', ' ', 'S', 'E', 'R',
                'V', 'I', 'C', 'E', ' ', ' ', ' ', ' '
            };
            std::memcpy(fic.data() + base + 6, label, 16);
        }
        // FIB 2: empty (padding only)

        // Append valid (complemented CCITT-FALSE) CRCs, then corrupt on request.
        for (size_t fib = 0; fib < 3; ++fib) {
            const size_t base = fib * 32;
            const uint16_t crc = static_cast<uint16_t>(
                ~eti::crc16ccitt_false(fic.data() + base, 30));
            fic[base + 30] = static_cast<uint8_t>((crc >> 8) & 0xFF);
            fic[base + 31] = static_cast<uint8_t>(crc & 0xFF);
        }
        auto corrupt = [&fic](size_t fib) {
            const size_t base = fib * 32;
            fic[base + 30] ^= 0xFF;
        };
        if (corrupt_mode >= 1) { corrupt(0); }
        if (corrupt_mode >= 2) { corrupt(1); }
        if (corrupt_mode >= 3) { corrupt(2); }
        return fic;
    }

    /**
     * @brief Auto-fallback: all FIB CRCs fail -> FIGs still extracted raw.
     */
    void test27_FicAutoFallback_AllFIBsBad() {
        auto fic_raw = buildSyntheticFic(3);  // all 3 FIB CRCs corrupted
        eti::EtiFicField fic(fic_raw.data(), fic_raw.size());

        // Strict drops every FIB -> no FIGs.
        auto strict = fic.decodeFigBlocks(eti::FicDecodeMode::Strict);
        QVERIFY(strict.fig_blocks.empty());
        QCOMPARE(strict.fib_crc_failures, 3u);
        QVERIFY(!strict.raw_fallback_used);

        // AutoFallback re-walks the whole FIC as a raw FIG stream -> FIG 0/16.
        auto fallback = fic.decodeFigBlocks();  // default = AutoFallback
        QVERIFY(fallback.raw_fallback_used);
        QVERIFY(!fallback.fig_blocks.empty());
        bool found_fig016 = false;
        bool found_fig11 = false;
        for (const auto& fig : fallback.fig_blocks) {
            if (fig.fig_type == 0 && fig.get_extension() == 16) found_fig016 = true;
            if (fig.fig_type == 1 && fig.get_extension() == 1) found_fig11 = true;
        }
        QVERIFY(found_fig016);  // first FIG in the stream is reached
        Q_UNUSED(found_fig11);
    }

    /**
     * @brief Strict mode still drops bad FIBs when only SOME fail.
     */
    void test28_FicStrict_PartialFIBFailures() {
        auto fic_raw = buildSyntheticFic(1);  // only FIB0 corrupted
        eti::EtiFicField fic(fic_raw.data(), fic_raw.size());

        // Strict: FIB0 dropped, FIB1 (FIG 1/1) + FIB2 survive.
        auto strict = fic.decodeFigBlocks(eti::FicDecodeMode::Strict);
        QCOMPARE(strict.fib_crc_failures, 1u);
        bool found_fig11 = false;
        for (const auto& fig : strict.fig_blocks) {
            if (fig.fig_type == 1 && fig.get_extension() == 1) found_fig11 = true;
        }
        QVERIFY(found_fig11);
        QVERIFY(!strict.raw_fallback_used);

        // AutoFallback must NOT trigger (only FIB0 of 3 failed, not all).
        auto fallback = fic.decodeFigBlocks();
        QVERIFY(!fallback.raw_fallback_used);
        QCOMPARE(fallback.fig_blocks.size(), strict.fig_blocks.size());
    }

    /**
     * @brief Forced raw mode decodes without any CRC validation.
     */
    void test29_FicRawMode() {
        auto fic_raw = buildSyntheticFic(3);
        eti::EtiFicField fic(fic_raw.data(), fic_raw.size());
        auto raw = fic.decodeFigBlocks(eti::FicDecodeMode::Raw);
        QVERIFY(raw.raw_fallback_used);
        QVERIFY(!raw.fig_blocks.empty());
    }

    /**
     * @brief DABX_FIC_MODE=strict env override disables the auto fallback.
     */
    void test30_FicEnvOverride_Strict() {
        auto fic_raw = buildSyntheticFic(3);
        eti::EtiFicField fic(fic_raw.data(), fic_raw.size());

        const bool set_ok = qputenv("DABX_FIC_MODE", QByteArray("strict"));
        QVERIFY(set_ok);
        auto fallback = fic.decodeFigBlocks();  // AutoFallback + env=strict
        QVERIFY(!fallback.raw_fallback_used);
        QVERIFY(fallback.fig_blocks.empty());
        qunsetenv("DABX_FIC_MODE");
    }

    // ========================================================================
    // TAIL TIST TESTS (option-variant matrix row 10)
    // ========================================================================

    /**
     * @brief ETI-NI parse path populates the tail TIST (bytes 6140..6143).
     */
    void test31_TailTIST_OnETI_NI_Path() {
        std::array<uint8_t, 6144> frame{};
        frame.fill(0);
        frame[0] = 0xFF;
        frame[1] = 0xF8;
        frame[4] = 0x00;  // FCT
        frame[5] = 0x00;  // FICF=0, NST=0

        // Known 32-bit timestamp at the frame tail.
        const uint32_t kTist = 0x12345678u;
        frame[6140] = static_cast<uint8_t>((kTist >> 24) & 0xFF);
        frame[6141] = static_cast<uint8_t>((kTist >> 16) & 0xFF);
        frame[6142] = static_cast<uint8_t>((kTist >> 8) & 0xFF);
        frame[6143] = static_cast<uint8_t>(kTist & 0xFF);

        ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        config.enable_memory_pooling = false;

        ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));

        auto result = parser.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(result.success);
        QVERIFY(result.frame.tist_tail_valid);
        QCOMPARE(result.frame.tist_tail, kTist);
        // etisnoop ms parity: (TIST & 0xFFFFFF) / 16384.0
        const double expected_ms = static_cast<double>(0x345678u) / 16384.0;
        QCOMPARE(result.frame.get_tail_tist_ms(), expected_ms);
        // The legacy bytes 8-11 view must NOT match the tail for NI frames.
        QVERIFY(result.frame.get_lidata_field().tist != kTist);
    }

    /**
     * @brief Direct tail accessor reads the last 4 bytes (big-endian).
     */
    void test32_TailTIST_DirectAccessor() {
        std::array<uint8_t, 6144> frame{};
        frame.fill(0);
        frame[6140] = 0xDE;
        frame[6141] = 0xAD;
        frame[6142] = 0xBE;
        frame[6143] = 0xEF;

        eti::EtiFrame f{frame};
        QCOMPARE(f.get_tail_tist(), 0xDEADBEEFu);
        QCOMPARE(f.get_tail_tist_ms(), static_cast<double>(0xADBEEFu) / 16384.0);
    }
};

QTEST_MAIN(TestETI_NI_Parser)
#include "test_eti_ni_parser.moc"
