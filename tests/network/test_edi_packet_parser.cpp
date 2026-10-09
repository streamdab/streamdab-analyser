/**
 * @file test_edi_packet_parser.cpp
 * @brief Unit tests for EDI packet parser
 *
 * Tests AF packet parsing, TAG item extraction, and CRC validation
 * using Qt Test framework.
 *
 * @author Backend Developer
 * @date 2025-10-19
 * @copyright Copyright (c) 2025 StreamDAB Analyser Team
 */

#include <QtTest/QtTest>
#include "../src/network/edi_packet_parser.hpp"
#include <vector>
#include <cstring>

class TestEDIPacketParser : public QObject {
    Q_OBJECT

private:
    /**
     * @brief Create valid AF packet for testing
     */
    std::vector<uint8_t> createValidAFPacket(const std::vector<uint8_t>& tag_payload) {
        std::vector<uint8_t> packet;

        // AF sync bytes (big endian): 0x4146 = 'AF'
        packet.push_back(0x41);
        packet.push_back(0x46);

        // Length field (big endian, 4 bytes)
        uint32_t length = static_cast<uint32_t>(tag_payload.size());
        packet.push_back((length >> 24) & 0xFF);
        packet.push_back((length >> 16) & 0xFF);
        packet.push_back((length >> 8) & 0xFF);
        packet.push_back(length & 0xFF);

        // TAG payload
        packet.insert(packet.end(), tag_payload.begin(), tag_payload.end());

        // Calculate CRC-32 over SYNC + LEN + PAYLOAD
        edi::EDIPacketParser parser;
        uint32_t crc = parser.calculateCRC32(std::span<const uint8_t>(packet));

        // Append CRC (big endian, 4 bytes)
        packet.push_back((crc >> 24) & 0xFF);
        packet.push_back((crc >> 16) & 0xFF);
        packet.push_back((crc >> 8) & 0xFF);
        packet.push_back(crc & 0xFF);

        return packet;
    }

    /**
     * @brief Create TAG item for testing
     * @note Uses ETSI TS 102 693 format: 4-byte name + 3-byte length + value
     */
    std::vector<uint8_t> createTAGItem(const std::string& name,
                                       const std::vector<uint8_t>& value) {
        std::vector<uint8_t> tag_item;

        // TAG name (4 bytes)
        tag_item.insert(tag_item.end(), name.begin(), name.begin() + 4);

        // TAG length (big endian, 3 bytes) - ETSI TS 102 693 uses 24-bit length
        uint32_t length = static_cast<uint32_t>(value.size());
        tag_item.push_back((length >> 16) & 0xFF);
        tag_item.push_back((length >> 8) & 0xFF);
        tag_item.push_back(length & 0xFF);

        // TAG value
        tag_item.insert(tag_item.end(), value.begin(), value.end());

        return tag_item;
    }

private slots:
    /**
     * @brief Test valid AF packet parsing
     */
    void testValidAFPacket() {
        // Create TAG payload with test data
        std::vector<uint8_t> test_value = {0x01, 0x02, 0x03, 0x04};
        auto tag_item = createTAGItem("est*", test_value);
        auto af_packet = createValidAFPacket(tag_item);

        // Parse AF packet
        edi::EDIPacketParser parser;
        auto result = parser.parseAFPacket(std::span<const uint8_t>(af_packet));

        // Verify parsing succeeded
        QVERIFY(result.has_value());
        QVERIFY(result->isValid());
        QCOMPARE(result->sync_pattern, static_cast<uint16_t>(0x4146));
        QCOMPARE(result->packet_length, static_cast<uint32_t>(tag_item.size()));
        QCOMPARE(result->payload.size(), tag_item.size());
    }

    /**
     * @brief Test AF packet with invalid sync bytes
     */
    void testInvalidSyncBytes() {
        std::vector<uint8_t> packet = {
            0xFF, 0xFF,  // Invalid sync bytes
            0x00, 0x00, 0x00, 0x08,  // Length
            0x74, 0x65, 0x73, 0x74,  // TAG name "test"
            0x00, 0x00, 0x00,        // TAG length 0 (3 bytes)
            0x00, 0x00, 0x00, 0x00   // CRC
        };

        edi::EDIPacketParser parser;
        auto result = parser.parseAFPacket(std::span<const uint8_t>(packet));

        QVERIFY(!result.has_value());
        QVERIFY(parser.getStatistics().af_sync_errors > 0);
    }

    /**
     * @brief Test AF packet with bad length field
     */
    void testBadLength() {
        std::vector<uint8_t> packet = {
            0x41, 0x46,              // Valid sync bytes
            0xFF, 0xFF, 0xFF, 0xFF,  // Invalid length (too large)
            0x00, 0x00, 0x00, 0x00   // CRC
        };

        edi::EDIPacketParser parser;
        auto result = parser.parseAFPacket(std::span<const uint8_t>(packet));

        QVERIFY(!result.has_value());
        QVERIFY(parser.getStatistics().parse_errors > 0);
    }

    /**
     * @brief Test truncated AF packet
     */
    void testTruncatedPacket() {
        std::vector<uint8_t> packet = {
            0x41, 0x46,              // Valid sync bytes
            0x00, 0x00, 0x00, 0x10   // Length indicates 16 bytes, but packet ends here
        };

        edi::EDIPacketParser parser;
        auto result = parser.parseAFPacket(std::span<const uint8_t>(packet));

        QVERIFY(!result.has_value());
    }

    /**
     * @brief Test single TAG item extraction
     */
    void testSingleTAGItem() {
        std::vector<uint8_t> test_value = {0xAA, 0xBB, 0xCC, 0xDD};
        auto tag_data = createTAGItem("est*", test_value);

        edi::EDIPacketParser parser;
        auto tag_items = parser.extractTAGItems(std::span<const uint8_t>(tag_data));

        QCOMPARE(tag_items.size(), static_cast<size_t>(1));
        QVERIFY(tag_items[0].isValid());
        QCOMPARE(tag_items[0].tag_name, std::string("est*"));
        QCOMPARE(tag_items[0].tag_length, static_cast<uint32_t>(4));
        QVERIFY(std::equal(tag_items[0].tag_value.begin(), tag_items[0].tag_value.end(),
                          test_value.begin()));
    }

    /**
     * @brief Test multiple TAG items extraction
     */
    void testMultipleTAGItems() {
        std::vector<uint8_t> value1 = {0x01, 0x02};
        std::vector<uint8_t> value2 = {0x03, 0x04, 0x05};
        std::vector<uint8_t> value3 = {0x06};

        auto tag1 = createTAGItem("est*", value1);
        auto tag2 = createTAGItem("deti", value2);
        auto tag3 = createTAGItem("sst*", value3);

        std::vector<uint8_t> tag_packet;
        tag_packet.insert(tag_packet.end(), tag1.begin(), tag1.end());
        tag_packet.insert(tag_packet.end(), tag2.begin(), tag2.end());
        tag_packet.insert(tag_packet.end(), tag3.begin(), tag3.end());

        edi::EDIPacketParser parser;
        auto tag_items = parser.extractTAGItems(std::span<const uint8_t>(tag_packet));

        QCOMPARE(tag_items.size(), static_cast<size_t>(3));
        QCOMPARE(tag_items[0].tag_name, std::string("est*"));
        QCOMPARE(tag_items[1].tag_name, std::string("deti"));
        QCOMPARE(tag_items[2].tag_name, std::string("sst*"));
    }

    /**
     * @brief Test TAG item with zero-length value
     */
    void testZeroLengthTAGItem() {
        std::vector<uint8_t> empty_value;
        auto tag_data = createTAGItem("test", empty_value);

        edi::EDIPacketParser parser;
        auto tag_items = parser.extractTAGItems(std::span<const uint8_t>(tag_data));

        QCOMPARE(tag_items.size(), static_cast<size_t>(1));
        QCOMPARE(tag_items[0].tag_length, static_cast<uint32_t>(0));
        QVERIFY(tag_items[0].tag_value.empty());
        QVERIFY(tag_items[0].isValid());
    }

    /**
     * @brief Test TAG item validation
     */
    void testTAGItemValidation() {
        edi::EDIPacketParser parser;

        // Valid TAG item
        edi::TAGItem valid_item;
        valid_item.tag_name = "test";
        valid_item.tag_length = 4;
        valid_item.tag_value = {0x01, 0x02, 0x03, 0x04};
        QVERIFY(parser.validateTAGItem(valid_item));

        // Invalid: length mismatch
        edi::TAGItem invalid_item;
        invalid_item.tag_name = "test";
        invalid_item.tag_length = 10;  // Says 10 bytes
        invalid_item.tag_value = {0x01, 0x02};  // But only 2 bytes
        QVERIFY(!parser.validateTAGItem(invalid_item));
    }

    /**
     * @brief Test finding TAG item by name
     */
    void testFindTAGItem() {
        edi::TAGPacket packet;

        // Add multiple TAG items
        edi::TAGItem item1;
        item1.tag_name = "est*";
        item1.tag_length = 4;
        item1.tag_value = {0x01, 0x02, 0x03, 0x04};

        edi::TAGItem item2;
        item2.tag_name = "deti";
        item2.tag_length = 2;
        item2.tag_value = {0xAA, 0xBB};

        packet.items.push_back(item1);
        packet.items.push_back(item2);

        edi::EDIPacketParser parser;

        // Find existing TAG
        auto found1 = parser.findTAGItem(packet, "est*");
        QVERIFY(found1.has_value());
        QCOMPARE(found1->tag_name, std::string("est*"));

        auto found2 = parser.findTAGItem(packet, "deti");
        QVERIFY(found2.has_value());
        QCOMPARE(found2->tag_name, std::string("deti"));

        // Try to find non-existent TAG
        auto not_found = parser.findTAGItem(packet, "xxxx");
        QVERIFY(!not_found.has_value());
    }

    /**
     * @brief Test CRC-32 calculation
     */
    void testCRC32Calculation() {
        edi::EDIPacketParser parser;

        // Test with known data
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
        uint32_t crc = parser.calculateCRC32(std::span<const uint8_t>(data));

        // CRC should be deterministic
        uint32_t crc2 = parser.calculateCRC32(std::span<const uint8_t>(data));
        QCOMPARE(crc, crc2);

        // Different data should produce different CRC
        std::vector<uint8_t> different_data = {0x05, 0x06, 0x07, 0x08};
        uint32_t different_crc = parser.calculateCRC32(std::span<const uint8_t>(different_data));
        QVERIFY(crc != different_crc);
    }

    /**
     * @brief Test CRC validation enable/disable
     */
    void testCRCValidationToggle() {
        // Create packet with intentionally wrong CRC
        std::vector<uint8_t> test_value = {0x01, 0x02};
        auto tag_item = createTAGItem("test", test_value);
        auto af_packet = createValidAFPacket(tag_item);

        // Corrupt the CRC
        af_packet[af_packet.size() - 1] ^= 0xFF;

        edi::EDIPacketParser parser;

        // With CRC validation enabled (default), should fail
        parser.setCRCValidation(true);
        auto result1 = parser.parseAFPacket(std::span<const uint8_t>(af_packet));
        QVERIFY(!result1.has_value());
        QVERIFY(parser.getStatistics().crc_errors > 0);

        // Disable CRC validation, should succeed (in non-strict mode)
        parser.resetStatistics();
        parser.setCRCValidation(false);
        parser.setStrictMode(false);
        auto result2 = parser.parseAFPacket(std::span<const uint8_t>(af_packet));
        QVERIFY(result2.has_value());
    }

    /**
     * @brief Test statistics tracking
     */
    void testStatistics() {
        edi::EDIPacketParser parser;

        // Initial statistics should be zero
        QCOMPARE(parser.getStatistics().total_af_packets_received, static_cast<uint64_t>(0));
        QCOMPARE(parser.getStatistics().total_tag_items_parsed, static_cast<uint64_t>(0));

        // Parse valid packet
        std::vector<uint8_t> test_value = {0x01, 0x02, 0x03};
        auto tag_item = createTAGItem("test", test_value);
        auto af_packet = createValidAFPacket(tag_item);

        parser.parseAFPacket(std::span<const uint8_t>(af_packet));

        // Statistics should be updated
        QCOMPARE(parser.getStatistics().total_af_packets_received, static_cast<uint64_t>(1));
        QCOMPARE(parser.getStatistics().total_tag_items_parsed, static_cast<uint64_t>(1));

        // Reset statistics
        parser.resetStatistics();
        QCOMPARE(parser.getStatistics().total_af_packets_received, static_cast<uint64_t>(0));
    }

    /**
     * @brief Test error callback
     */
    void testErrorCallback() {
        edi::EDIPacketParser parser;

        QString last_error;
        int last_severity = -1;

        parser.setErrorCallback([&](const std::string& error, int severity) {
            last_error = QString::fromStdString(error);
            last_severity = severity;
        });

        // Trigger error with invalid sync bytes
        std::vector<uint8_t> invalid_packet = {0xFF, 0xFF, 0x00, 0x00};
        parser.parseAFPacket(std::span<const uint8_t>(invalid_packet));

        // Verify callback was invoked
        QVERIFY(!last_error.isEmpty());
        QVERIFY(last_severity >= 0);
    }

    /**
     * @brief Test strict mode behavior
     */
    void testStrictMode() {
        // Create packet with CRC error
        std::vector<uint8_t> test_value = {0x01};
        auto tag_item = createTAGItem("test", test_value);
        auto af_packet = createValidAFPacket(tag_item);
        af_packet[af_packet.size() - 1] ^= 0xFF; // Corrupt CRC

        edi::EDIPacketParser parser;
        parser.setCRCValidation(true);

        // Non-strict mode: CRC error is warning, parsing may continue
        parser.setStrictMode(false);
        auto result1 = parser.parseAFPacket(std::span<const uint8_t>(af_packet));
        // In non-strict mode, parse continues despite CRC error
        QVERIFY(!result1.has_value()); // Still fails due to CRC validation

        // Strict mode: CRC error causes immediate failure
        parser.resetStatistics();
        parser.setStrictMode(true);
        auto result2 = parser.parseAFPacket(std::span<const uint8_t>(af_packet));
        QVERIFY(!result2.has_value());
    }

    /**
     * @brief Test incomplete TAG header handling
     */
    void testIncompleteTAGHeader() {
        // TAG header requires 7 bytes minimum (4 name + 3 length), provide only 6
        std::vector<uint8_t> incomplete_data = {
            0x74, 0x65, 0x73, 0x74,  // TAG name "test"
            0x00, 0x00               // Incomplete length field (only 2 bytes)
        };

        edi::EDIPacketParser parser;
        auto tag_items = parser.extractTAGItems(std::span<const uint8_t>(incomplete_data));

        // Should return empty vector
        QVERIFY(tag_items.empty());
    }

    /**
     * @brief Test TAG packet convenience methods
     */
    void testTAGPacketMethods() {
        edi::TAGPacket packet;

        // Test TAG lookup
        edi::TAGItem item;
        item.tag_name = "est*";
        item.tag_length = 0;
        packet.items.push_back(item);

        // Test item count
        QCOMPARE(packet.items.size(), static_cast<size_t>(1));

        // Test item access
        QVERIFY(!packet.items.empty());
        QCOMPARE(packet.items[0].tag_name, std::string("est*"));
    }
};

QTEST_MAIN(TestEDIPacketParser)
#include "test_edi_packet_parser.moc"
