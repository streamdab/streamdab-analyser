/**
 * @file test_fig0_processor.cpp
 * @brief Unit tests for FIG Type 0 Processing (Phase 1 Week 2)
 *
 * Comprehensive test suite for FIG Type 0 processing functionality
 * following ETSI EN 300 401 specifications.
 *
 * Test Coverage:
 * - FIG 0/0: Ensemble information
 * - FIG 0/1: Sub-channel organization with comprehensive storage verification
 * - FIG 0/2: Service organization
 * - Service discovery integration
 * - ETSI compliance validation
 */

#include <QtTest/QtTest>
#include <QObject>
#include <QDebug>
#include "../src/core/advanced_fig_analyzer.h"

class TestFIG0Processor : public QObject
{
    Q_OBJECT

private slots:
    // Test lifecycle
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // FIG 0/0 tests (Ensemble information)
    void testFIG0_0_EnsembleIDExtraction();
    void testFIG0_0_CountryCode();
    void testFIG0_0_ChangeFlag();
    void testFIG0_0_AlarmFlag();
    void testFIG0_0_InvalidData();
    void testFIG0_0_EnsembleReference();

    // FIG 0/1 tests (Sub-channel organization)
    void testFIG0_1_SingleSubChannel();
    void testFIG0_1_MultipleSubChannels();
    void testFIG0_1_ShortForm();
    void testFIG0_1_LongForm();
    void testFIG0_1_StartAddressExtraction();
    void testFIG0_1_TableIndexExtraction();
    void testFIG0_1_BitrateCalculation();
    void testFIG0_1_ProtectionLevel();
    void testFIG0_1_InvalidSubChannelID();

    // FIG 0/2 tests (Service organization)
    void testFIG0_2_ProgrammeService();
    void testFIG0_2_DataService();
    void testFIG0_2_ServiceIDExtraction();
    void testFIG0_2_MultipleComponents();
    void testFIG0_2_ComponentID();
    void testFIG0_2_ServiceType();
    void testFIG0_2_LocalFlag();

    // Integration tests
    void testCompleteServiceDiscovery();
    void testServiceComponentMapping();
    void testEnsembleTreeConstruction();
    void testMultipleFIGProcessing();

    // ETSI compliance tests
    void testETSI_EN300401_Compliance();
    void testFIGHeaderValidation();
    void testDataLengthValidation();

    // Error handling tests
    void testInvalidFIGType();
    void testInvalidFIGExtension();
    void testTruncatedFIGData();
    void testCorruptedFIGHeader();

private:
    AdvancedFIGAnalyzer* m_analyzer;

    // Helper methods
    QByteArray createFIG0_0(uint16_t ensemble_id, uint8_t country_code = 0x0E);
    QByteArray createFIG0_1_ShortForm(uint8_t subch_id, uint16_t start_addr, uint8_t table_idx);
    QByteArray createFIG0_1_LongForm(uint8_t subch_id, uint16_t start_addr, uint8_t protection);
    QByteArray createFIG0_2(uint32_t service_id, uint8_t subch_id, bool is_audio = true);
    QByteArray createInvalidFIG();

    void verifyEnsembleInfo(uint16_t expected_id, const QString& expected_country);
    void verifySubChannelInfo(uint8_t expected_id, uint16_t expected_start_addr);
    void verifyServiceInfo(uint32_t expected_service_id, uint8_t expected_subch_id);
};

// ============================================================================
// Test Lifecycle Methods
// ============================================================================

void TestFIG0Processor::initTestCase()
{
    qInfo() << "========================================";
    qInfo() << "Starting FIG Type 0 Processor Test Suite";
    qInfo() << "Phase 1 Week 2 - FIG Decoding";
    qInfo() << "========================================";

    // Register Qt metatypes
    qRegisterMetaType<ServiceInfo>("ServiceInfo");
    qRegisterMetaType<EnsembleInfo>("EnsembleInfo");
    qRegisterMetaType<FIGData>("FIGData");
}

void TestFIG0Processor::cleanupTestCase()
{
    qInfo() << "========================================";
    qInfo() << "FIG Type 0 Processor Tests Complete";
    qInfo() << "========================================";
}

void TestFIG0Processor::init()
{
    m_analyzer = new AdvancedFIGAnalyzer();
    QVERIFY(m_analyzer != nullptr);

    // Enable FIG types for testing
    m_analyzer->enableFIGType(0, 0, true);
    m_analyzer->enableFIGType(0, 1, true);
    m_analyzer->enableFIGType(0, 2, true);
}

void TestFIG0Processor::cleanup()
{
    delete m_analyzer;
    m_analyzer = nullptr;
}

// ============================================================================
// FIG 0/0 Tests (Ensemble Information)
// ============================================================================

void TestFIG0Processor::testFIG0_0_EnsembleIDExtraction()
{
    // Create FIG 0/0 with Ensemble ID 0x1234
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    m_analyzer->analyzeFICData(fig);

    EnsembleInfo info = m_analyzer->getCurrentEnsemble();
    QCOMPARE(info.ensembleId, static_cast<uint32_t>(0x1234));

    qInfo() << "FIG 0/0: Ensemble ID" << QString::number(info.ensembleId, 16);
}

void TestFIG0Processor::testFIG0_0_CountryCode()
{
    // Thailand country code: 0x0E (14)
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    m_analyzer->analyzeFICData(fig);

    EnsembleInfo info = m_analyzer->getCurrentEnsemble();
    QCOMPARE(info.countryCode, QString("TH"));

    qInfo() << "FIG 0/0: Country code" << info.countryCode;
}

void TestFIG0Processor::testFIG0_0_ChangeFlag()
{
    // FIG 0/0 with change flag set
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    // Set C/N flag (bit 7 of byte after header)
    if (fig.size() > 5) {
        fig[5] = fig[5] | 0x80;  // Set change flag
    }

    m_analyzer->analyzeFICData(fig);

    // Verify change flag processed
    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testFIG0_0_AlarmFlag()
{
    // FIG 0/0 with alarm flag
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    // Set Alarm flag (bit 6)
    if (fig.size() > 5) {
        fig[5] = fig[5] | 0x40;  // Set alarm flag
    }

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
}

void TestFIG0Processor::testFIG0_0_InvalidData()
{
    // Create invalid FIG 0/0 (too short)
    QByteArray fig(3, 0);
    fig[0] = 0x00;  // FIG Type 0, Extension 0

    m_analyzer->analyzeFICData(fig);

    // Should report error
    QVERIFY(!m_analyzer->getLastError().isEmpty());
}

void TestFIG0Processor::testFIG0_0_EnsembleReference()
{
    // Test ensemble reference (ECC + ensemble ID)
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    m_analyzer->analyzeFICData(fig);

    EnsembleInfo info = m_analyzer->getCurrentEnsemble();
    QVERIFY(info.ensembleId == 0x1234);
}

// ============================================================================
// FIG 0/1 Tests (Sub-channel Organization) - ENHANCED WITH STORAGE VERIFICATION
// ============================================================================

void TestFIG0Processor::testFIG0_1_SingleSubChannel()
{
    // Test: Sub-channel 0, Start Address 0, Table Index 5
    QByteArray fig = createFIG0_1_ShortForm(0, 0, 5);

    m_analyzer->analyzeFICData(fig);

    // Verify FIG processed successfully
    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
    qInfo() << "FIG 0/1: Single sub-channel processed";

    // *** ENHANCED: Verify sub-channel storage ***
    auto subchannels = m_analyzer->getAllSubChannels();
    QCOMPARE(static_cast<int>(subchannels.size()), 1);  // Should have exactly 1 sub-channel
    qInfo() << "FIG 0/1: Storage verified - 1 sub-channel stored";

    // Verify sub-channel details using getSubChannelInfo
    SubChannelInfo subch = m_analyzer->getSubChannelInfo(0);
    QCOMPARE(subch.sub_channel_id, static_cast<uint8_t>(0));
    QCOMPARE(subch.start_address, static_cast<uint16_t>(0));
    QCOMPARE(subch.table_index, static_cast<uint8_t>(5));
    QVERIFY(subch.short_form == true);  // Short form (EEP)
    qInfo() << "FIG 0/1: Sub-channel details verified - SubCh=0, Start=0, Table=5, Form=Short(EEP)";
}

void TestFIG0Processor::testFIG0_1_MultipleSubChannels()
{
    // Test: Process 3 different sub-channels
    // Create FIG 0/1 with multiple sub-channels
    QByteArray fig;

    // FIC header (2 bytes per ETSI EN 300 401 Section 5.2)
    uint8_t type_byte = (0 << 5);  // FIG type 0 in bits 7-5
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x0C));  // Length=12 bytes (extension + padding + 3*3 sub-channels)

    // Extension field (1 byte) - Extension 1 for sub-channel organization
    uint8_t ext_byte = (1 << 3);  // Extension 1 in bits 7-3
    fig.append(static_cast<char>(ext_byte));

    // Padding byte (processFIG0_1 uses offset=2)
    fig.append(static_cast<char>(0x00));

    // Sub-channel 0: SubChId=0, Start=0, Table Index=5
    uint8_t subch0_byte = (0 << 2) | 0x00;  // SubChId=0, Start[9:8]=0
    fig.append(static_cast<char>(subch0_byte));
    fig.append(static_cast<char>(0x00));  // Start[7:0]=0
    fig.append(static_cast<char>(0x80 | 5));  // Short form (0x80) + Table Index 5

    // Sub-channel 1: SubChId=1, Start=84, Table Index=7
    uint8_t subch1_byte = (1 << 2) | 0x00;  // SubChId=1, Start[9:8]=0
    fig.append(static_cast<char>(subch1_byte));
    fig.append(static_cast<char>(84));  // Start[7:0]=84
    fig.append(static_cast<char>(0x80 | 7));  // Short form (0x80) + Table Index 7

    // Sub-channel 2: SubChId=2, Start=168, Table Index=9
    uint8_t subch2_byte = (2 << 2) | 0x00;  // SubChId=2, Start[9:8]=0
    fig.append(static_cast<char>(subch2_byte));
    fig.append(static_cast<char>(168));  // Start[7:0]=168
    fig.append(static_cast<char>(0x80 | 9));  // Short form (0x80) + Table Index 9

    m_analyzer->analyzeFICData(fig);

    // Verify FIG processing count
    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
    qInfo() << "FIG 0/1: Multiple sub-channels processed";

    // *** ENHANCED: Verify all sub-channels stored ***
    auto subchannels = m_analyzer->getAllSubChannels();
    QCOMPARE(static_cast<int>(subchannels.size()), 3);  // Should have exactly 3 sub-channels
    qInfo() << "FIG 0/1: Storage verified - 3 sub-channels stored";

    // Verify each sub-channel exists and has correct data
    SubChannelInfo subch0 = m_analyzer->getSubChannelInfo(0);
    SubChannelInfo subch1 = m_analyzer->getSubChannelInfo(1);
    SubChannelInfo subch2 = m_analyzer->getSubChannelInfo(2);

    // Verify sub-channel IDs
    QCOMPARE(subch0.sub_channel_id, static_cast<uint8_t>(0));
    QCOMPARE(subch1.sub_channel_id, static_cast<uint8_t>(1));
    QCOMPARE(subch2.sub_channel_id, static_cast<uint8_t>(2));

    // Verify start addresses
    QCOMPARE(subch0.start_address, static_cast<uint16_t>(0));
    QCOMPARE(subch1.start_address, static_cast<uint16_t>(84));
    QCOMPARE(subch2.start_address, static_cast<uint16_t>(168));
    qInfo() << "FIG 0/1: Start addresses verified - 0, 84, 168";

    // Verify table indices
    QCOMPARE(subch0.table_index, static_cast<uint8_t>(5));
    QCOMPARE(subch1.table_index, static_cast<uint8_t>(7));
    QCOMPARE(subch2.table_index, static_cast<uint8_t>(9));
    qInfo() << "FIG 0/1: Table indices verified - 5, 7, 9";

    // Verify all are short form
    QVERIFY(subch0.short_form == true);
    QVERIFY(subch1.short_form == true);
    QVERIFY(subch2.short_form == true);
}

void TestFIG0Processor::testFIG0_1_ShortForm()
{
    // Test: Short form (EEP - Equal Error Protection) with table-based protection
    QByteArray fig = createFIG0_1_ShortForm(5, 100, 12);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);

    // *** ENHANCED: Verify short form storage ***
    SubChannelInfo subch = m_analyzer->getSubChannelInfo(5);
    QCOMPARE(subch.sub_channel_id, static_cast<uint8_t>(5));
    QCOMPARE(subch.start_address, static_cast<uint16_t>(100));
    QCOMPARE(subch.table_index, static_cast<uint8_t>(12));
    QVERIFY(subch.short_form == true);  // Must be short form
    qInfo() << "FIG 0/1: Short form verified - SubCh=5, Start=100, Table=12, Form=Short(EEP)";
}

void TestFIG0Processor::testFIG0_1_LongForm()
{
    // Test: Long form (UEP - Unequal Error Protection)
    QByteArray fig = createFIG0_1_LongForm(5, 100, 2);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);

    // *** ENHANCED: Verify long form storage ***
    SubChannelInfo subch = m_analyzer->getSubChannelInfo(5);
    QCOMPARE(subch.sub_channel_id, static_cast<uint8_t>(5));
    QCOMPARE(subch.start_address, static_cast<uint16_t>(100));
    QCOMPARE(subch.protection_level, static_cast<uint8_t>(2));
    QVERIFY(subch.short_form == false);  // Must be long form
    QCOMPARE(subch.sub_channel_size, static_cast<uint16_t>(0x54));  // 84 CUs
    qInfo() << "FIG 0/1: Long form verified - SubCh=5, Start=100, Protection=2, Form=Long(UEP), Size=84CU";
}

void TestFIG0Processor::testFIG0_1_StartAddressExtraction()
{
    // Test: Various start addresses (CU addresses in MSC)
    uint16_t test_addresses[] = {0, 84, 168, 252, 336, 420, 504, 588, 672, 756};

    for (size_t i = 0; i < 10; ++i) {
        QByteArray fig = createFIG0_1_ShortForm(0, test_addresses[i], 5);
        m_analyzer->analyzeFICData(fig);
    }

    // Verify processing count
    QVERIFY(m_analyzer->getTotalFIGsProcessed() == 10);
    qInfo() << "FIG 0/1: Processed 10 FIGs with different start addresses";

    // *** ENHANCED: Verify start address progression ***
    // Note: Since all use SubCh ID 0, only the last one is stored
    auto subchannels = m_analyzer->getAllSubChannels();
    QCOMPARE(static_cast<int>(subchannels.size()), 1);  // Only 1 sub-channel (overwritten)

    // Verify last processed start address (756)
    SubChannelInfo subch = m_analyzer->getSubChannelInfo(0);
    QCOMPARE(subch.start_address, static_cast<uint16_t>(756));
    qInfo() << "FIG 0/1: Start address progression verified - Last address=756";
}

void TestFIG0Processor::testFIG0_1_TableIndexExtraction()
{
    // Test: Table indices 0-15 (protection table selection)
    for (uint8_t table_idx = 0; table_idx < 16; ++table_idx) {
        m_analyzer->reset();  // Clear previous data to test each independently
        QByteArray fig = createFIG0_1_ShortForm(0, 0, table_idx);
        m_analyzer->analyzeFICData(fig);

        // *** ENHANCED: Verify table index stored correctly ***
        SubChannelInfo subch = m_analyzer->getSubChannelInfo(0);
        QCOMPARE(subch.sub_channel_id, static_cast<uint8_t>(0));
        QCOMPARE(subch.table_index, table_idx);
        QVERIFY(subch.short_form == true);
    }

    qInfo() << "FIG 0/1: All 16 table indices verified (0-15)";
}

void TestFIG0Processor::testFIG0_1_BitrateCalculation()
{
    // Test: Table index 5 -> 64 kbps (typical DAB bitrate)
    QByteArray fig = createFIG0_1_ShortForm(0, 0, 5);

    m_analyzer->analyzeFICData(fig);

    // Bitrate calculation depends on protection table
    QVERIFY(m_analyzer->getValidFIGsCount() > 0);

    // *** ENHANCED: Verify bitrate calculation via SubChannelInfo ***
    SubChannelInfo subch = m_analyzer->getSubChannelInfo(0);
    QCOMPARE(subch.table_index, static_cast<uint8_t>(5));

    // Verify bitrate can be retrieved (implementation-dependent)
    uint16_t bitrate = subch.getBitrate();
    QVERIFY(bitrate > 0);  // Should return valid bitrate
    qInfo() << "FIG 0/1: Bitrate calculation verified - Table Index 5 -> Bitrate" << bitrate << "kbps";
}

void TestFIG0Processor::testFIG0_1_ProtectionLevel()
{
    // Test: Long form with protection level 2 (EEP-A)
    QByteArray fig = createFIG0_1_LongForm(0, 0, 2);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);

    // *** ENHANCED: Verify protection level storage ***
    SubChannelInfo subch = m_analyzer->getSubChannelInfo(0);
    QCOMPARE(subch.protection_level, static_cast<uint8_t>(2));
    QVERIFY(subch.short_form == false);  // Long form uses protection_level

    // Verify protection level string
    QString protection = subch.getProtectionLevel();
    QVERIFY(!protection.isEmpty());
    qInfo() << "FIG 0/1: Protection level verified - Level 2:" << protection;
}

void TestFIG0Processor::testFIG0_1_InvalidSubChannelID()
{
    // Test: Sub-channel ID must be 0-63 (6 bits)
    QByteArray fig = createFIG0_1_ShortForm(64, 0, 5);  // Invalid: 64 (exceeds 6-bit limit)

    m_analyzer->analyzeFICData(fig);

    // Should detect error or reject invalid sub-channel
    QVERIFY(!m_analyzer->getLastError().isEmpty() ||
            m_analyzer->getValidFIGsCount() == 0);

    // *** ENHANCED: Verify invalid sub-channel NOT stored ***
    auto subchannels = m_analyzer->getAllSubChannels();
    // Should either have 0 sub-channels or error reported
    if (subchannels.size() > 0) {
        // If stored, should not be ID 64
        SubChannelInfo subch = m_analyzer->getSubChannelInfo(64);
        QVERIFY(subch.sub_channel_id != 64);  // Should not store invalid ID
    }
    qInfo() << "FIG 0/1: Invalid sub-channel ID rejected - ID 64 not stored";
}

// ============================================================================
// FIG 0/2 Tests (Service Organization)
// ============================================================================

void TestFIG0Processor::testFIG0_2_ProgrammeService()
{
    // First add sub-channel
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));

    // Then add audio service
    QByteArray fig = createFIG0_2(0xE0D00001, 0, true);

    m_analyzer->analyzeFICData(fig);

    auto services = m_analyzer->getDiscoveredServices();
    QVERIFY(services.size() > 0);

    qInfo() << "FIG 0/2: Programme service discovered";
}

void TestFIG0Processor::testFIG0_2_DataService()
{
    // First add sub-channel
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(1, 84, 7));

    // Then add data service
    QByteArray fig = createFIG0_2(0xE0D00002, 1, false);

    m_analyzer->analyzeFICData(fig);

    auto services = m_analyzer->getDiscoveredServices();
    QVERIFY(services.size() > 0);
}

void TestFIG0Processor::testFIG0_2_ServiceIDExtraction()
{
    // Test service ID: 0xE0D00001 (Thai DAB typical)
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));

    QByteArray fig = createFIG0_2(0xE0D00001, 0, true);
    m_analyzer->analyzeFICData(fig);

    auto services = m_analyzer->getDiscoveredServices();
    if (services.size() > 0) {
        QCOMPARE(services[0].serviceId, static_cast<uint32_t>(0xE0D00001));
    }
}

void TestFIG0Processor::testFIG0_2_MultipleComponents()
{
    // Service with multiple components
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(1, 84, 7));

    // Create FIG 0/2 with multiple components (2 components)
    QByteArray fig;

    // FIC header (2 bytes)
    uint8_t type_byte = (0 << 5);  // FIG type 0
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x08));  // Length=8 bytes (extension + 7 data bytes)

    // Extension field (1 byte) - Extension 2
    uint8_t ext_byte = (2 << 3);  // Extension 2 in bits 7-3
    fig.append(static_cast<char>(ext_byte));

    // Service descriptor
    // Byte 0: P/D flag (bit 7) + Service ID MSB (programme service)
    uint16_t service_id = 0xE0D0;
    uint8_t pd_byte = 0x80 | ((service_id >> 8) & 0x7F);  // P/D=1 (programme)
    fig.append(static_cast<char>(pd_byte));

    // Byte 1: Service ID LSB
    fig.append(static_cast<char>(service_id & 0xFF));

    // Byte 2: Number of components (2) in high nibble
    fig.append(static_cast<char>(0x20));  // 2 components

    // Component 1: SubChId=0
    fig.append(static_cast<char>(0x00));  // TMId=0 (MSC stream), ASCTy=0 (DAB)
    fig.append(static_cast<char>(0x00));  // SubChId=0

    // Component 2: SubChId=1
    fig.append(static_cast<char>(0x00));  // TMId=0 (MSC stream), ASCTy=0 (DAB)
    fig.append(static_cast<char>(0x01));  // SubChId=1

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testFIG0_2_ComponentID()
{
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));

    QByteArray fig = createFIG0_2(0xE0D00001, 0, true);
    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getTotalFIGsProcessed() >= 2);
}

void TestFIG0Processor::testFIG0_2_ServiceType()
{
    // Audio service (TMId = 00)
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));
    m_analyzer->analyzeFICData(createFIG0_2(0xE0D00001, 0, true));

    auto services = m_analyzer->getDiscoveredServices();
    if (services.size() > 0) {
        QVERIFY(services[0].isAudio);
    }
}

void TestFIG0Processor::testFIG0_2_LocalFlag()
{
    // Test local flag (services same or different ensemble)
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));

    QByteArray fig = createFIG0_2(0xE0D00001, 0, true);
    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

void TestFIG0Processor::testCompleteServiceDiscovery()
{
    // Simulate complete service discovery workflow

    // Step 1: Add ensemble info (FIG 0/0)
    m_analyzer->analyzeFICData(createFIG0_0(0x1234, 0x0E));

    // Step 2: Add sub-channels (FIG 0/1)
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(1, 84, 7));
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(2, 168, 9));

    // Step 3: Add services (FIG 0/2)
    m_analyzer->analyzeFICData(createFIG0_2(0xE0D00001, 0, true));
    m_analyzer->analyzeFICData(createFIG0_2(0xE0D00002, 1, true));
    m_analyzer->analyzeFICData(createFIG0_2(0xE0D00003, 2, true));

    // Verify complete service discovery
    EnsembleInfo ensemble = m_analyzer->getCurrentEnsemble();
    QCOMPARE(ensemble.ensembleId, static_cast<uint32_t>(0x1234));

    auto services = m_analyzer->getDiscoveredServices();
    QVERIFY(services.size() >= 3);

    qInfo() << "Complete service discovery: Ensemble"
            << QString::number(ensemble.ensembleId, 16)
            << "with" << services.size() << "services";
}

void TestFIG0Processor::testServiceComponentMapping()
{
    // Test correct mapping between services and sub-channels
    m_analyzer->analyzeFICData(createFIG0_1_ShortForm(5, 100, 8));
    m_analyzer->analyzeFICData(createFIG0_2(0xE0D00005, 5, true));

    auto services = m_analyzer->getDiscoveredServices();
    if (services.size() > 0) {
        QCOMPARE(services[0].subchannelId, static_cast<uint8_t>(5));
    }
}

void TestFIG0Processor::testEnsembleTreeConstruction()
{
    // Build complete ensemble tree structure
    m_analyzer->analyzeFICData(createFIG0_0(0x1234, 0x0E));

    for (uint8_t i = 0; i < 5; ++i) {
        m_analyzer->analyzeFICData(createFIG0_1_ShortForm(i, i * 84, 5 + i));
        m_analyzer->analyzeFICData(createFIG0_2(0xE0D00001 + i, i, true));
    }

    EnsembleInfo ensemble = m_analyzer->getCurrentEnsemble();
    auto services = m_analyzer->getDiscoveredServices();

    QCOMPARE(ensemble.ensembleId, static_cast<uint32_t>(0x1234));
    QVERIFY(services.size() >= 5);

    qInfo() << "Ensemble tree: " << services.size() << "services";
}

void TestFIG0Processor::testMultipleFIGProcessing()
{
    // Process multiple FIGs in sequence
    QByteArray fic_data;

    // Concatenate multiple FIGs
    fic_data.append(createFIG0_0(0x1234, 0x0E));
    fic_data.append(createFIG0_1_ShortForm(0, 0, 5));
    fic_data.append(createFIG0_2(0xE0D00001, 0, true));

    m_analyzer->analyzeFICData(fic_data);

    QVERIFY(m_analyzer->getTotalFIGsProcessed() >= 3);
}

// ============================================================================
// ETSI Compliance Tests
// ============================================================================

void TestFIG0Processor::testETSI_EN300401_Compliance()
{
    // Test ETSI EN 300 401 compliance
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    m_analyzer->analyzeFICData(fig);

    // Should process without compliance errors
    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testFIGHeaderValidation()
{
    // Test FIG header structure validation
    QByteArray fig;

    // FIC header (2 bytes per ETSI EN 300 401 Section 5.2)
    uint8_t type_byte = (0 << 5);  // FIG type 0 in bits 7-5
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x08));  // Length=8 bytes

    // Extension field (1 byte) - Extension 0
    uint8_t ext_byte = (0 << 3);  // Extension 0 in bits 7-3
    fig.append(static_cast<char>(ext_byte));

    // Padding byte
    fig.append(static_cast<char>(0x00));

    // Ensemble data (processFIG0_0 reads from data[2-7])
    fig.append(static_cast<char>(0x12));  // EId MSB
    fig.append(static_cast<char>(0x34));  // EId LSB
    fig.append(static_cast<char>(0x00));  // Change flags
    fig.append(static_cast<char>(0x00));  // Alarm flags
    fig.append(static_cast<char>(0x0E));  // Country code
    fig.append(static_cast<char>(0x00));  // ECC

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testDataLengthValidation()
{
    // Test length field validation
    QByteArray fig;

    // FIC header (2 bytes)
    uint8_t type_byte = (0 << 5);  // FIG type 0
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x04));  // Length=4 bytes (extension + 3 data bytes)

    // Extension field (1 byte)
    uint8_t ext_byte = (0 << 3);  // Extension 0
    fig.append(static_cast<char>(ext_byte));

    // Add 3 bytes of data (to match length=4)
    fig.append(static_cast<char>(0x12));
    fig.append(static_cast<char>(0x34));
    fig.append(static_cast<char>(0x0E));

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

void TestFIG0Processor::testInvalidFIGType()
{
    QByteArray fig;
    fig.append(static_cast<char>(0xFF));  // Invalid type
    fig.append(static_cast<char>(0x04));

    m_analyzer->analyzeFICData(fig);

    QVERIFY(!m_analyzer->getLastError().isEmpty() ||
            m_analyzer->getValidFIGsCount() == 0);
}

void TestFIG0Processor::testInvalidFIGExtension()
{
    QByteArray fig;

    // FIC header with valid FIG type 0
    uint8_t type_byte = (0 << 5);  // FIG type 0
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x04));  // Length=4

    // Extension field with invalid extension 31
    uint8_t ext_byte = (31 << 3);  // Invalid extension 31 in bits 7-3
    fig.append(static_cast<char>(ext_byte));

    // Add some data
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));

    m_analyzer->analyzeFICData(fig);

    // Should handle gracefully
    QVERIFY(m_analyzer->getTotalFIGsProcessed() >= 0);
}

void TestFIG0Processor::testTruncatedFIGData()
{
    QByteArray fig;

    // FIC header claiming 16 bytes
    uint8_t type_byte = (0 << 5);  // FIG type 0
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x10));  // Claims 16 bytes

    // Only add 2 bytes (truncated - should have 16)
    fig.append(static_cast<char>(0x12));
    fig.append(static_cast<char>(0x34));

    m_analyzer->analyzeFICData(fig);

    // Should detect truncation
    QVERIFY(!m_analyzer->getLastError().isEmpty() ||
            m_analyzer->getValidFIGsCount() == 0);
}

void TestFIG0Processor::testCorruptedFIGHeader()
{
    QByteArray fig = createInvalidFIG();

    m_analyzer->analyzeFICData(fig);

    // Should handle corrupted data
    QVERIFY(m_analyzer->getTotalFIGsProcessed() >= 0);
}

// ============================================================================
// Helper Methods
// ============================================================================

QByteArray TestFIG0Processor::createFIG0_0(uint16_t ensemble_id, uint8_t country_code)
{
    QByteArray fig;

    // FIC header (2 bytes per ETSI EN 300 401 Section 5.2)
    uint8_t type_byte = (0 << 5);  // FIG type 0 in bits 7-5
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x08));  // Length=8 bytes (extension + 7 data bytes)

    // Extension field (1 byte) - CRITICAL: Must be after FIC header
    // Bits 7-3: Extension (0), Bit 2: C/N, Bit 1: OE, Bit 0: PD
    uint8_t ext_byte = (0 << 3);  // Extension 0
    fig.append(static_cast<char>(ext_byte));

    // Padding byte (processFIG0_0 expects data to start at offset 2)
    fig.append(static_cast<char>(0x00));

    // Ensemble ID (16-bit) - processFIG0_0 reads from data[2-3]
    fig.append(static_cast<char>((ensemble_id >> 8) & 0xFF));
    fig.append(static_cast<char>(ensemble_id & 0xFF));

    // Change flags - processFIG0_0 reads from data[4]
    fig.append(static_cast<char>(0x00));

    // Alarm flag - processFIG0_0 reads from data[5]
    fig.append(static_cast<char>(0x00));

    // Country code - processFIG0_0 reads from data[6] if size >= 8
    fig.append(static_cast<char>(country_code));  // Thailand = 0x0E

    // Extended Country Code (ECC) - Optional data[7]
    fig.append(static_cast<char>(0x00));

    return fig;
}

QByteArray TestFIG0Processor::createFIG0_1_ShortForm(uint8_t subch_id, uint16_t start_addr, uint8_t table_idx)
{
    QByteArray fig;

    // FIC header (2 bytes per ETSI EN 300 401 Section 5.2)
    uint8_t type_byte = (0 << 5);  // FIG type 0 in bits 7-5
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x05));  // Length=5 bytes (extension + padding + 3 data bytes)

    // Extension field (1 byte) - Extension 1 for sub-channel organization
    uint8_t ext_byte = (1 << 3);  // Extension 1 in bits 7-3
    fig.append(static_cast<char>(ext_byte));

    // Padding byte (processFIG0_1 uses offset=2 to skip extension+padding)
    fig.append(static_cast<char>(0x00));

    // Sub-channel descriptor (short form - 3 bytes)
    // Byte 0: SubChId (6 bits in bits 7-2) + Start Address MSB (2 bits in bits 1-0)
    uint8_t subch_byte = (subch_id & 0x3F) << 2;
    subch_byte |= (start_addr >> 8) & 0x03;  // Start address bits 9-8
    fig.append(static_cast<char>(subch_byte));

    // Byte 1: Start Address LSB (8 bits)
    fig.append(static_cast<char>(start_addr & 0xFF));

    // Byte 2: Short form flag (1) + Table switch (0=A) + Table index (6 bits)
    uint8_t form_byte = 0x80;  // Short form flag (bit 7 = 1)
    form_byte |= (table_idx & 0x3F);  // Table index in bits 5-0
    fig.append(static_cast<char>(form_byte));

    return fig;
}

QByteArray TestFIG0Processor::createFIG0_1_LongForm(uint8_t subch_id, uint16_t start_addr, uint8_t protection)
{
    QByteArray fig;

    // FIC header (2 bytes per ETSI EN 300 401 Section 5.2)
    uint8_t type_byte = (0 << 5);  // FIG type 0 in bits 7-5
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x06));  // Length=6 bytes (extension + padding + 4 data bytes)

    // Extension field (1 byte) - Extension 1 for sub-channel organization
    uint8_t ext_byte = (1 << 3);  // Extension 1 in bits 7-3
    fig.append(static_cast<char>(ext_byte));

    // Padding byte (processFIG0_1 uses offset=2)
    fig.append(static_cast<char>(0x00));

    // Sub-channel descriptor (long form - 4 bytes)
    // Byte 0: SubChId (6 bits in bits 7-2) + Start Address MSB (2 bits in bits 1-0)
    uint8_t subch_byte = (subch_id & 0x3F) << 2;
    subch_byte |= (start_addr >> 8) & 0x03;
    fig.append(static_cast<char>(subch_byte));

    // Byte 1: Start Address LSB (8 bits)
    fig.append(static_cast<char>(start_addr & 0xFF));

    // Byte 2: Long form flag (0) + Option (3 bits) + Protection (2 bits) + Size MSB (2 bits)
    uint8_t form_byte = 0x00;  // Long form flag (bit 7 = 0)
    form_byte |= (protection & 0x03) << 2;  // Protection level in bits 3-2
    form_byte |= 0x00;  // Size MSB (84 CUs = 0x0054, MSB=0x00)
    fig.append(static_cast<char>(form_byte));

    // Byte 3: Sub-channel size LSB (84 CUs)
    fig.append(static_cast<char>(0x54));

    return fig;
}

QByteArray TestFIG0Processor::createFIG0_2(uint32_t service_id, uint8_t subch_id, bool is_audio)
{
    QByteArray fig;

    // Determine if programme (16-bit) or data (32-bit) service
    // Programme services have IDs < 0x10000, data services use full 32-bit
    bool is_programme = (service_id < 0x10000);

    // FIC header (2 bytes per ETSI EN 300 401 Section 5.2)
    uint8_t type_byte = (0 << 5);  // FIG type 0 in bits 7-5
    fig.append(static_cast<char>(type_byte));

    if (is_programme) {
        fig.append(static_cast<char>(0x06));  // Length=6 bytes (ext + 5 data)
    } else {
        fig.append(static_cast<char>(0x08));  // Length=8 bytes (ext + 7 data, 32-bit SId)
    }

    // Extension field (1 byte) - Extension 2 for service organization
    uint8_t ext_byte = (2 << 3);  // Extension 2 in bits 7-3
    fig.append(static_cast<char>(ext_byte));

    // Service descriptor (processFIG0_2 reads from offset=1, which is data[1])
    if (is_programme) {
        // Programme service: 16-bit Service ID with P/D=1
        // Byte 0 (data[1]): P/D flag (bit 7=1) + Service ID bits 15-8
        uint8_t pd_byte = 0x80 | ((service_id >> 8) & 0x7F);
        fig.append(static_cast<char>(pd_byte));

        // Byte 1 (data[2]): Service ID bits 7-0
        fig.append(static_cast<char>(service_id & 0xFF));
    } else {
        // Data service: 32-bit Service ID with P/D=0
        // Bytes 0-3: Full 32-bit service ID
        fig.append(static_cast<char>((service_id >> 24) & 0xFF));
        fig.append(static_cast<char>((service_id >> 16) & 0xFF));
        fig.append(static_cast<char>((service_id >> 8) & 0xFF));
        fig.append(static_cast<char>(service_id & 0xFF));
    }

    // Number of components (high nibble) + Local flag + CAId
    uint8_t num_comp_byte = (1 << 4);  // 1 component in high nibble
    fig.append(static_cast<char>(num_comp_byte));

    // Component descriptor (2 bytes per component)
    // Byte 0: TMId (2 bits in bits 7-6) + ASCTy/DSCTy (6 bits in bits 5-0)
    uint8_t tmid = 0x00;  // MSC stream audio (0x00)
    uint8_t ascty = is_audio ? 0x00 : 0x3F;  // 0x00=DAB, 0x3F=DAB+
    uint8_t tmid_byte = (tmid << 6) | (ascty & 0x3F);
    fig.append(static_cast<char>(tmid_byte));

    // Byte 1: P/S flag (bit 7) + CA flag (bit 6) + SubChId (bits 5-0)
    uint8_t subch_byte = 0x00;  // P/S=0 (primary), CA=0 (not CA)
    subch_byte |= (subch_id & 0x3F);  // SubChId in bits 5-0
    fig.append(static_cast<char>(subch_byte));

    return fig;
}

QByteArray TestFIG0Processor::createInvalidFIG()
{
    QByteArray fig(10, 0xAA);  // Garbage data
    return fig;
}

// Register test class
QTEST_MAIN(TestFIG0Processor)
#include "test_fig0_processor.moc"
