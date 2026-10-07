/**
 * @file test_fig0_processor.cpp
 * @brief Unit tests for FIG Type 0 processing (Ensemble and Service Organization)
 *
 * Tests FIG 0/0, 0/1, and 0/2 processing per ETSI EN 300 401
 * Phase 1 Week 2: FIG Decoding Implementation
 */

#include <QtTest/QtTest>
#include <QtCore/QDebug>
#include "../src/core/advanced_fig_analyser.h"

// ============================================================================
// Test Class Declaration
// ============================================================================

class TestFIG0Processor : public QObject
{
    Q_OBJECT

private slots:
    // Lifecycle
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // FIG 0/0 Tests (Ensemble Organization)
    void testFIG0_0_EnsembleInfo();
    void testFIG0_0_ChangeFlags();
    void testFIG0_0_CountryCode();
    void testFIG0_0_EnsembleReference();

    // FIG 0/1 Tests (Sub-channel Organization) - ENHANCED WITH STORAGE VERIFICATION
    void testFIG0_1_SingleSubChannel();
    void testFIG0_1_MultipleSubChannels();
    void testFIG0_1_ShortForm();
    void testFIG0_1_LongForm();
    void testFIG0_1_StartAddressExtraction();
    void testFIG0_1_TableIndexExtraction();
    void testFIG0_1_BitrateCalculation();
    void testFIG0_1_ProtectionLevel();
    void testFIG0_1_InvalidSubChannelID();

    // FIG 0/2 Tests (Service Organization)
    void testFIG0_2_ProgrammeService();
    void testFIG0_2_DataService();
    void testFIG0_2_ServiceComponent();
    void testFIG0_2_PrimarySecondaryComponent();
    void testFIG0_2_MultipleServices();
    void testFIG0_2_ServiceIDExtraction();
    void testFIG0_2_ComponentMapping();

    // Error handling tests
    void testDataLengthValidation();
    void testInvalidFIGType();
    void testInvalidFIGExtension();
    void testTruncatedFIGData();
    void testCorruptedFIGHeader();

private:
    // Test helpers
    AdvancedFIGAnalyser* m_analyser;

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
    m_analyser = new AdvancedFIGAnalyser();
    QVERIFY(m_analyser != nullptr);

    // Enable FIG types for testing
    m_analyser->enableFIGType(0, 0, true);
    m_analyser->enableFIGType(0, 1, true);
    m_analyser->enableFIGType(0, 2, true);
}

void TestFIG0Processor::cleanup()
{
    delete m_analyser;
    m_analyser = nullptr;
}

// ============================================================================
// FIG 0/0 Tests (Ensemble Organization)
// ============================================================================

void TestFIG0Processor::testFIG0_0_EnsembleInfo()
{
    // Test basic ensemble info extraction
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    m_analyser->analyzeFICData(fig);

    // Verify FIG was processed
    QVERIFY(m_analyser->getTotalFIGsProcessed() > 0);

    // Verify ensemble info
    EnsembleInfo info = m_analyser->getCurrentEnsemble();
    QCOMPARE(info.ensembleId, static_cast<uint16_t>(0x1234));
    QVERIFY(!info.countryCode.isEmpty());
}

void TestFIG0Processor::testFIG0_0_ChangeFlags()
{
    // Test change flag detection
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testFIG0_0_CountryCode()
{
    // Test country code extraction (Thailand = 0x0E)
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    m_analyser->analyzeFICData(fig);

    EnsembleInfo info = m_analyser->getCurrentEnsemble();
    QVERIFY(!info.countryCode.isEmpty());
}

void TestFIG0Processor::testFIG0_0_EnsembleReference()
{
    // Test ensemble reference (ECC + ensemble ID)
    QByteArray fig = createFIG0_0(0x1234, 0x0E);

    m_analyser->analyzeFICData(fig);

    EnsembleInfo info = m_analyser->getCurrentEnsemble();
    QVERIFY(info.ensembleId == 0x1234);
}

// ============================================================================
// FIG 0/1 Tests (Sub-channel Organization) - ENHANCED WITH STORAGE VERIFICATION
// ============================================================================

void TestFIG0Processor::testFIG0_1_SingleSubChannel()
{
    // Test: Sub-channel 0, Start Address 0, Table Index 5
    QByteArray fig = createFIG0_1_ShortForm(0, 0, 5);

    m_analyser->analyzeFICData(fig);

    // Verify FIG processed successfully
    QVERIFY(m_analyser->getTotalFIGsProcessed() > 0);
    qInfo() << "FIG 0/1: Single sub-channel processed";

    // *** ENHANCED: Verify sub-channel storage ***
    auto subchannels = m_analyser->getAllSubChannels();
    QCOMPARE(static_cast<int>(subchannels.size()), 1);  // Should have exactly 1 sub-channel
    qInfo() << "FIG 0/1: Storage verified - 1 sub-channel stored";

    // Verify sub-channel details using getSubChannelInfo
    SubChannelInfo subch = m_analyser->getSubChannelInfo(0);
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

    // FIG header (1 byte): type 0, length 10 (ext + 3*3-byte UEP entries)
    fig.append(static_cast<char>(0x0A));

    // Extension field (1 byte) - Extension 1 (FIG 0 ext = data[0] & 0x1F)
    fig.append(static_cast<char>(0x01));

    // Sub-channel 0: SubChId=0, Start=0, Table Index=5
    uint8_t subch0_byte = (0 << 2) | 0x00;  // SubChId=0, Start[9:8]=0
    fig.append(static_cast<char>(subch0_byte));
    fig.append(static_cast<char>(0x00));  // Start[7:0]=0
    fig.append(static_cast<char>(0x05));  // UEP short form (bit 7=0) + Table Index 5

    // Sub-channel 1: SubChId=1, Start=84, Table Index=7
    uint8_t subch1_byte = (1 << 2) | 0x00;  // SubChId=1, Start[9:8]=0
    fig.append(static_cast<char>(subch1_byte));
    fig.append(static_cast<char>(84));  // Start[7:0]=84
    fig.append(static_cast<char>(0x07));  // UEP short form (bit 7=0) + Table Index 7

    // Sub-channel 2: SubChId=2, Start=168, Table Index=9
    uint8_t subch2_byte = (2 << 2) | 0x00;  // SubChId=2, Start[9:8]=0
    fig.append(static_cast<char>(subch2_byte));
    fig.append(static_cast<char>(168));  // Start[7:0]=168
    fig.append(static_cast<char>(0x09));  // UEP short form (bit 7=0) + Table Index 9

    m_analyser->analyzeFICData(fig);

    // Verify FIG processing count
    QVERIFY(m_analyser->getTotalFIGsProcessed() > 0);
    qInfo() << "FIG 0/1: Multiple sub-channels processed";

    // *** ENHANCED: Verify all sub-channels stored ***
    auto subchannels = m_analyser->getAllSubChannels();
    QCOMPARE(static_cast<int>(subchannels.size()), 3);  // Should have exactly 3 sub-channels
    qInfo() << "FIG 0/1: Storage verified - 3 sub-channels stored";

    // Verify each sub-channel exists and has correct data
    SubChannelInfo subch0 = m_analyser->getSubChannelInfo(0);
    SubChannelInfo subch1 = m_analyser->getSubChannelInfo(1);
    SubChannelInfo subch2 = m_analyser->getSubChannelInfo(2);

    // Verify sub-channel IDs
    QCOMPARE(subch0.sub_channel_id, static_cast<uint8_t>(0));
    QCOMPARE(subch1.sub_channel_id, static_cast<uint8_t>(1));
    QCOMPARE(subch2.sub_channel_id, static_cast<uint8_t>(2));

    // Verify start addresses
    QCOMPARE(subch0.start_address, static_cast<uint16_t>(0));
    QCOMPARE(subch1.start_address, static_cast<uint16_t>(84));
    QCOMPARE(subch2.start_address, static_cast<uint16_t>(168));

    // Verify table indices
    QCOMPARE(subch0.table_index, static_cast<uint8_t>(5));
    QCOMPARE(subch1.table_index, static_cast<uint8_t>(7));
    QCOMPARE(subch2.table_index, static_cast<uint8_t>(9));

    qInfo() << "FIG 0/1: Multiple sub-channels verified - All 3 sub-channels have correct data";
}

void TestFIG0Processor::testFIG0_1_ShortForm()
{
    // Test: Short form (EEP - Equal Error Protection)
    QByteArray fig = createFIG0_1_ShortForm(5, 100, 12);

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);

    // *** ENHANCED: Verify short form storage ***
    SubChannelInfo subch = m_analyser->getSubChannelInfo(5);
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

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);

    // *** ENHANCED: Verify long form storage ***
    SubChannelInfo subch = m_analyser->getSubChannelInfo(5);
    QCOMPARE(subch.sub_channel_id, static_cast<uint8_t>(5));
    QCOMPARE(subch.start_address, static_cast<uint16_t>(100));
    QCOMPARE(subch.protection_level, static_cast<uint8_t>(2));
    QVERIFY(subch.short_form == false);  // Must be long form
    QCOMPARE(subch.sub_channel_size, static_cast<uint16_t>(0x54));  // 84 CUs
    qInfo() << "FIG 0/1: Long form verified - SubCh=5, Start=100, Protection=2, Form=Long(UEP), Size=84CU";
}

void TestFIG0Processor::testFIG0_1_StartAddressExtraction()
{
    // Test: Start addresses 0, 84, 168, 252, 336, 420
    uint16_t test_addresses[] = {0, 84, 168, 252, 336, 420, 504, 588, 672, 756};

    for (uint16_t addr : test_addresses) {
        QByteArray fig = createFIG0_1_ShortForm(0, addr, 5);
        m_analyser->analyzeFICData(fig);
    }

    // Verify processing count
    QVERIFY(m_analyser->getTotalFIGsProcessed() == 10);
    qInfo() << "FIG 0/1: Processed 10 FIGs with different start addresses";

    // *** ENHANCED: Verify start address progression ***
    // Note: Since all use SubCh ID 0, only the last one is stored
    auto subchannels = m_analyser->getAllSubChannels();
    QCOMPARE(static_cast<int>(subchannels.size()), 1);  // Only 1 sub-channel (overwritten)

    // Verify last processed start address (756)
    SubChannelInfo subch = m_analyser->getSubChannelInfo(0);
    QCOMPARE(subch.start_address, static_cast<uint16_t>(756));
    qInfo() << "FIG 0/1: Start address progression verified - Last address=756";
}

void TestFIG0Processor::testFIG0_1_TableIndexExtraction()
{
    // Test: Table indices 0-15 (protection table selection)
    for (uint8_t table_idx = 0; table_idx < 16; ++table_idx) {
        m_analyser->reset();  // Clear previous data to test each independently
        QByteArray fig = createFIG0_1_ShortForm(0, 0, table_idx);
        m_analyser->analyzeFICData(fig);

        // *** ENHANCED: Verify table index stored correctly ***
        SubChannelInfo subch = m_analyser->getSubChannelInfo(0);
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

    m_analyser->analyzeFICData(fig);

    // Bitrate calculation depends on protection table
    QVERIFY(m_analyser->getValidFIGsCount() > 0);

    // *** ENHANCED: Verify bitrate calculation via SubChannelInfo ***
    SubChannelInfo subch = m_analyser->getSubChannelInfo(0);
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

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);

    // *** ENHANCED: Verify protection level storage ***
    SubChannelInfo subch = m_analyser->getSubChannelInfo(0);
    QCOMPARE(subch.protection_level, static_cast<uint8_t>(2));
    QVERIFY(subch.short_form == false);  // Long form uses protection_level

    // Verify protection level string representation
    QString protection = subch.getProtectionLevel();
    QVERIFY(!protection.isEmpty());
    qInfo() << "FIG 0/1: Protection level verified - Level 2 =" << protection;
}

void TestFIG0Processor::testFIG0_1_InvalidSubChannelID()
{
    // NOTE (ETSI EN 300 401 Section 6.2.1): SubChId is a 6-bit field, so an
    // ID of 64 is unrepresentable on the wire -- (64 & 0x3F) << 2 encodes
    // byte-identically to SubChId 0, which the valid-path tests already cover
    // as accepted. No byte-level validator can distinguish the two, so this
    // test exercises the validator's malformed-descriptor rejection instead:
    // a FIG 0/1 whose declared length carries no complete 3-byte sub-channel
    // descriptor must be rejected (error set, nothing stored, no valid FIG).
    QByteArray fig;
    fig.append(static_cast<char>(0x03));  // FIG header: type 0, length 3
    fig.append(static_cast<char>(0x01));  // Extension byte: extension 1
    fig.append(static_cast<char>(0x00));  // SubChId=0
    fig.append(static_cast<char>(0x00));  // Start address low

    m_analyser->analyzeFICData(fig);

    // Should detect error or reject invalid sub-channel
    QVERIFY(!m_analyser->getLastError().isEmpty() ||
            m_analyser->getValidFIGsCount() == 0);

    // *** ENHANCED: Verify invalid sub-channel NOT stored ***
    auto subchannels = m_analyser->getAllSubChannels();
    QCOMPARE(static_cast<int>(subchannels.size()), 0);
    qInfo() << "FIG 0/1: Invalid sub-channel descriptor rejected - nothing stored";
}

// ============================================================================
// FIG 0/2 Tests (Service Organization)
// ============================================================================

void TestFIG0Processor::testFIG0_2_ProgrammeService()
{
    // First add sub-channel
    m_analyser->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));

    // Then add audio service
    QByteArray fig = createFIG0_2(0xE0D00001, 0, true);

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getServiceCount() > 0);

    // *** ENHANCED: Verify service storage ***
    auto services = m_analyser->getDABServices();
    QVERIFY(services.size() > 0);

    // Find service with ID 0xE0D00001
    bool found = false;
    for (const auto& svc : services) {
        if (svc.service_id == 0xE0D00001) {
            found = true;
            QVERIFY(svc.service_type == 0);  // Audio service
            break;
        }
    }
    QVERIFY(found);
    qInfo() << "FIG 0/2: Programme service verified - Audio service stored";
}

void TestFIG0Processor::testFIG0_2_DataService()
{
    // First add sub-channel
    m_analyser->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));

    // Then add data service
    QByteArray fig = createFIG0_2(0xE0D00002, 0, false);

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getServiceCount() > 0);

    // *** ENHANCED: Verify data service storage ***
    auto services = m_analyser->getDABServices();
    QVERIFY(services.size() > 0);

    // Find data service
    bool found = false;
    for (const auto& svc : services) {
        if (svc.service_id == 0xE0D00002) {
            found = true;
            QVERIFY(svc.service_type == 1);  // Data service
            break;
        }
    }
    QVERIFY(found);
    qInfo() << "FIG 0/2: Data service verified - Data service stored";
}

void TestFIG0Processor::testFIG0_2_ServiceComponent()
{
    // First add sub-channel
    m_analyser->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));

    // Add service with component
    m_analyser->analyzeFICData(createFIG0_2(0xE0D00001, 0, true));

    QVERIFY(m_analyser->getServiceCount() > 0);

    // *** ENHANCED: Verify component mapping ***
    auto services = m_analyser->getDABServices();
    bool found = false;
    for (const auto& svc : services) {
        if (svc.service_id == 0xE0D00001) {
            found = true;
            QVERIFY(svc.components.size() > 0);  // Should have at least 1 component
            QCOMPARE(svc.components[0].sub_channel_id, static_cast<uint8_t>(0));
            break;
        }
    }
    QVERIFY(found);
    qInfo() << "FIG 0/2: Service component verified - Component mapped to SubCh 0";
}

void TestFIG0Processor::testFIG0_2_PrimarySecondaryComponent()
{
    // Test primary/secondary component designation
    m_analyser->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));
    m_analyser->analyzeFICData(createFIG0_2(0xE0D00001, 0, true));

    QVERIFY(m_analyser->getServiceCount() > 0);

    // *** ENHANCED: Verify primary component ***
    auto services = m_analyser->getDABServices();
    for (const auto& svc : services) {
        if (svc.service_id == 0xE0D00001) {
            auto primary = svc.getPrimaryComponent();
            QVERIFY(primary.is_primary);  // Should be marked as primary
            break;
        }
    }
    qInfo() << "FIG 0/2: Primary component verified";
}

void TestFIG0Processor::testFIG0_2_MultipleServices()
{
    // Add 3 services
    m_analyser->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));
    m_analyser->analyzeFICData(createFIG0_1_ShortForm(1, 84, 7));
    m_analyser->analyzeFICData(createFIG0_1_ShortForm(2, 168, 9));

    m_analyser->analyzeFICData(createFIG0_2(0xE0D00001, 0, true));
    m_analyser->analyzeFICData(createFIG0_2(0xE0D00002, 1, true));
    m_analyser->analyzeFICData(createFIG0_2(0xE0D00003, 2, false));

    // *** ENHANCED: Verify all services stored ***
    QCOMPARE(m_analyser->getServiceCount(), 3);
    auto services = m_analyser->getDABServices();
    QCOMPARE(static_cast<int>(services.size()), 3);
    qInfo() << "FIG 0/2: Multiple services verified - 3 services stored";
}

void TestFIG0Processor::testFIG0_2_ServiceIDExtraction()
{
    // Test 32-bit service ID extraction
    m_analyser->analyzeFICData(createFIG0_1_ShortForm(0, 0, 5));
    m_analyser->analyzeFICData(createFIG0_2(0xE0D00001, 0, false));

    auto services = m_analyser->getDABServices();
    QVERIFY(services.size() > 0);

    // *** ENHANCED: Verify exact service ID ***
    bool found = false;
    for (const auto& svc : services) {
        if (svc.service_id == 0xE0D00001) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
    qInfo() << "FIG 0/2: Service ID extraction verified - 32-bit ID = 0xE0D00001";
}

void TestFIG0Processor::testFIG0_2_ComponentMapping()
{
    // Verify component-to-subchannel mapping
    m_analyser->analyzeFICData(createFIG0_1_ShortForm(5, 100, 12));
    m_analyser->analyzeFICData(createFIG0_2(0xE0D00001, 5, true));

    auto services = m_analyser->getDABServices();

    // *** ENHANCED: Verify component mapping ***
    bool found = false;
    for (const auto& svc : services) {
        if (svc.service_id == 0xE0D00001) {
            found = true;
            QVERIFY(svc.components.size() > 0);
            QCOMPARE(svc.components[0].sub_channel_id, static_cast<uint8_t>(5));
            break;
        }
    }
    QVERIFY(found);
    qInfo() << "FIG 0/2: Component mapping verified - Service mapped to SubCh 5";
}

// ============================================================================
// Error Handling Tests
// ============================================================================

void TestFIG0Processor::testDataLengthValidation()
{
    // FIX: Create valid FIG 0/0 with sufficient data (at least 5 bytes)
    QByteArray fig;

    // FIG header (1 byte per EN 300 401 clause 5.2): type 0, length 6
    fig.append(static_cast<char>(0x06));

    // Extension field (1 byte) - Extension 0 (FIG 0 ext = data[0] & 0x1F)
    fig.append(static_cast<char>(0x00));

    // FIG 0/0 requires at least 5 bytes of data: EId (2) + change/alarm/CIF (2) + occ (1)
    fig.append(static_cast<char>(0x12));  // EId high
    fig.append(static_cast<char>(0x34));  // EId low
    fig.append(static_cast<char>(0x00));  // Change flag + Alarm + CIF count high
    fig.append(static_cast<char>(0x00));  // CIF count low
    fig.append(static_cast<char>(0x0E));  // Occurrence change / AL flag + Country code

    m_analyser->analyzeFICData(fig);

    // FIX: Should process successfully now with valid data
    QVERIFY(m_analyser->getTotalFIGsProcessed() > 0);
}

void TestFIG0Processor::testInvalidFIGType()
{
    // Test unsupported FIG type
    QByteArray fig;
    uint8_t type_byte = (7 << 5);  // FIG type 7 (unsupported)
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x04));

    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));

    m_analyser->analyzeFICData(fig);

    // Unsupported types should not increment counter
    QVERIFY(m_analyser->getTotalFIGsProcessed() == 0);
}

void TestFIG0Processor::testInvalidFIGExtension()
{
    // Test unsupported FIG 0 extension
    QByteArray fig;
    uint8_t type_byte = (0 << 5);  // FIG type 0
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x04));

    uint8_t ext_byte = (31 << 3);  // Extension 31 (unsupported)
    fig.append(static_cast<char>(ext_byte));

    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));

    m_analyser->analyzeFICData(fig);

    // Disabled extensions should not increment counter
    QVERIFY(m_analyser->getTotalFIGsProcessed() == 0);
}

void TestFIG0Processor::testTruncatedFIGData()
{
    // Test FIG with incomplete data
    QByteArray fig;
    uint8_t type_byte = (0 << 5);
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x10));  // Claims 16 bytes

    // Only provide 5 bytes
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x00));

    m_analyser->analyzeFICData(fig);

    // Truncated data should be detected
    QVERIFY(!m_analyser->getLastError().isEmpty());
}

void TestFIG0Processor::testCorruptedFIGHeader()
{
    // Test FIG with zero length (invalid)
    QByteArray fig;
    uint8_t type_byte = (0 << 5);
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x00));  // Zero length (invalid)

    m_analyser->analyzeFICData(fig);

    // Should detect invalid header
    QVERIFY(!m_analyser->getLastError().isEmpty());
}

// ============================================================================
// Helper Functions
// ============================================================================

QByteArray TestFIG0Processor::createFIG0_0(uint16_t ensemble_id, uint8_t country_code)
{
    QByteArray fig;

    // FIG header (1 byte per EN 300 401 clause 5.2): type (b7-b5) + length
    // (b4-b0). Data = ext(1) + EId(2) + change/alarm/CIF(2) + occ(1) = 6 bytes.
    fig.append(static_cast<char>(0x06));

    // Extension field (1 byte) - Extension 0 (FIG 0 ext = data[0] & 0x1F)
    fig.append(static_cast<char>(0x00));

    // Ensemble ID (2 bytes)
    fig.append(static_cast<char>((ensemble_id >> 8) & 0xFF));
    fig.append(static_cast<char>(ensemble_id & 0xFF));

    // Change flag (2 bits) + Alarm flag (1 bit) + CIF count high (5 bits)
    fig.append(static_cast<char>(0x00));

    // CIF count low (1 byte)
    fig.append(static_cast<char>(0x00));

    // Occurrence change / AL flag + Country code (1 byte)
    fig.append(static_cast<char>(country_code & 0x0F));

    return fig;
}

QByteArray TestFIG0Processor::createFIG0_1_ShortForm(uint8_t subch_id, uint16_t start_addr, uint8_t table_idx)
{
    QByteArray fig;

    // FIG header (1 byte per EN 300 401 clause 5.2): type 0, length 4
    // (ext + 3-byte UEP short-form entry).
    fig.append(static_cast<char>(0x04));

    // Extension field (1 byte) - Extension 1 (FIG 0 ext = data[0] & 0x1F)
    fig.append(static_cast<char>(0x01));

    // Sub-channel descriptor (UEP short form - 3 bytes)
    // Byte 0: SubChId (6 bits in bits 7-2) + Start Address MSB (2 bits in bits 1-0)
    uint8_t subch_byte = (subch_id & 0x3F) << 2;
    subch_byte |= (start_addr >> 8) & 0x03;  // Start address bits 9-8
    fig.append(static_cast<char>(subch_byte));

    // Byte 1: Start Address LSB (8 bits)
    fig.append(static_cast<char>(start_addr & 0xFF));

    // Byte 2: Form flag (bit 7 = 0 for UEP short form) + Table switch (0=A)
    //         + Table index (6 bits)  [etisnoop fig0_1.cpp semantics]
    uint8_t form_byte = 0x00;
    form_byte |= (table_idx & 0x3F);  // Table index in bits 5-0
    fig.append(static_cast<char>(form_byte));

    return fig;
}

QByteArray TestFIG0Processor::createFIG0_1_LongForm(uint8_t subch_id, uint16_t start_addr, uint8_t protection)
{
    QByteArray fig;

    // FIG header (1 byte per EN 300 401 clause 5.2): type 0, length 5
    // (ext + 4-byte EEP long-form entry).
    fig.append(static_cast<char>(0x05));

    // Extension field (1 byte) - Extension 1 (FIG 0 ext = data[0] & 0x1F)
    fig.append(static_cast<char>(0x01));

    // Sub-channel descriptor (EEP long form - 4 bytes, bit 7 of byte 2 = 1)
    // Byte 0: SubChId (6 bits) + Start Address MSB (2 bits)
    uint8_t subch_byte = (subch_id & 0x3F) << 2;
    subch_byte |= (start_addr >> 8) & 0x03;
    fig.append(static_cast<char>(subch_byte));

    // Byte 1: Start Address LSB
    fig.append(static_cast<char>(start_addr & 0xFF));

    // Byte 2: Long form flag (bit 7 = 1, EEP) + Option (3 bits) + Protection (2 bits) + Size MSB (2 bits)
    uint16_t sub_channel_size = 0x54;  // 84 CUs (example)
    uint8_t form_byte = 0x80;  // EEP long form flag (bit 7 = 1)
    form_byte |= ((protection & 0x03) << 2);  // Protection level
    form_byte |= (sub_channel_size >> 8) & 0x03;  // Size bits 9-8
    fig.append(static_cast<char>(form_byte));

    // Byte 3: Size LSB
    fig.append(static_cast<char>(sub_channel_size & 0xFF));

    return fig;
}

QByteArray TestFIG0Processor::createFIG0_2(uint32_t service_id, uint8_t subch_id, bool is_audio)
{
    QByteArray fig;

    bool use_32bit = (service_id > 0xFFFF);

    // FIG data length (excluding the 1-byte FIG header)
    // 16-bit SId: ext(1) + SId(2) + NumComp(1) + CompDesc(2) = 6 bytes
    // 32-bit SId: ext(1) + SId(4) + NumComp(1) + CompDesc(2) = 8 bytes
    uint8_t fig_length = use_32bit ? 0x08 : 0x06;

    // FIG header (1 byte per EN 300 401 clause 5.2): Type (bits 7-5) + Length (bits 4-0)
    fig.append(static_cast<char>(fig_length));

    // FIG 0/2 extension byte: extension 2 in bits 4-0, P/D flag in bit 5
    // (etisnoop fig0_2.cpp: fig0.pd()). P/D=1 selects 32-bit data SIds.
    uint8_t ext_byte = 0x02;
    if (use_32bit) {
        ext_byte |= 0x20;  // P/D=1 -> data service with 32-bit SId
    }
    fig.append(static_cast<char>(ext_byte));

    // Service ID encoding
    if (use_32bit) {
        // 32-bit Service ID (full 4 bytes)
        fig.append(static_cast<char>((service_id >> 24) & 0xFF));
        fig.append(static_cast<char>((service_id >> 16) & 0xFF));
        fig.append(static_cast<char>((service_id >> 8) & 0xFF));
        fig.append(static_cast<char>(service_id & 0xFF));
    } else {
        // 16-bit Service ID with P/D flag in bit 15 (key convention shared
        // with FIG 1/1: both sides mask 0x7FFF)
        uint16_t sid_16 = static_cast<uint16_t>(service_id & 0xFFFF);
        uint8_t sid_high = static_cast<uint8_t>((sid_16 >> 8) & 0x7F);
        if (is_audio) {
            sid_high |= 0x80;  // P/D in bit 15 (house key convention)
        }
        fig.append(static_cast<char>(sid_high));
        fig.append(static_cast<char>(sid_16 & 0xFF));
    }

    // Number of service components (low nibble of byte)
    uint8_t num_components = 1;
    fig.append(static_cast<char>(num_components & 0x0F));

    // Service Component Descriptor (2 bytes)
    // Byte 1: TMId (bits 7-6) + ASCTy/DSCTy (bits 5-0)
    uint8_t tmid_ascty = 0x00;  // TMId=0 (MSC stream)
    if (is_audio) {
        tmid_ascty |= 0x3F;  // ASCTy = 0x3F for DAB+ (triggers is_dab_plus flag)
    } else {
        tmid_ascty |= 0x01;  // DSCTy = 0x01 for generic data service (not audio)
    }
    fig.append(static_cast<char>(tmid_ascty));

    // Byte 2: SubChId (bits 7-2) + P/S flag (bit 1) + CA flag (bit 0)
    // [etisnoop fig0_2.cpp: subchid = (b1 & 0xFC) >> 2, ps = (b1 & 0x02) >> 1]
    uint8_t ps_subch = (subch_id & 0x3F) << 2;  // SubChId in bits 7-2
    ps_subch |= (1 << 1);  // P/S = 1 (Primary component)
    fig.append(static_cast<char>(ps_subch));

    return fig;
}

QByteArray TestFIG0Processor::createInvalidFIG()
{
    QByteArray fig;
    fig.append(static_cast<char>(0xFF));  // Invalid header
    return fig;
}

// ============================================================================
// Test Registration
// ============================================================================

QTEST_MAIN(TestFIG0Processor)
#include "test_fig0_processor.moc"
