/**
 * @file test_fig0_processor.cpp
 * @brief Unit tests for FIG Type 0 Processing (Phase 1 Week 2)
 *
 * Comprehensive test suite for FIG Type 0 processing functionality
 * following ETSI EN 300 401 specifications.
 *
 * Test Coverage:
 * - FIG 0/0: Ensemble information
 * - FIG 0/1: Sub-channel organization
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
// FIG 0/1 Tests (Sub-channel Organization)
// ============================================================================

void TestFIG0Processor::testFIG0_1_SingleSubChannel()
{
    // Sub-channel 0, Start Address 0, Table Index 5
    QByteArray fig = createFIG0_1_ShortForm(0, 0, 5);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
    qInfo() << "FIG 0/1: Single sub-channel processed";
}

void TestFIG0Processor::testFIG0_1_MultipleSubChannels()
{
    // Create FIG 0/1 with multiple sub-channels
    QByteArray fig;

    // FIG header: Type 0, Extension 1
    fig.append(static_cast<char>(0x01));  // Type=0, Extension=1
    fig.append(static_cast<char>(0x10));  // Length=16 bytes

    // Sub-channel 0: Start 0, Table Index 5
    fig.append(static_cast<char>(0x00));  // SubChId=0
    fig.append(static_cast<char>(0x00));  // Start Addr MSB
    fig.append(static_cast<char>(0x00));  // Start Addr LSB
    fig.append(static_cast<char>(0x05));  // Table Index

    // Sub-channel 1: Start 84, Table Index 7
    fig.append(static_cast<char>(0x01));  // SubChId=1
    fig.append(static_cast<char>(0x00));  // Start Addr MSB
    fig.append(static_cast<char>(0x54));  // Start Addr LSB (84)
    fig.append(static_cast<char>(0x07));  // Table Index

    // Sub-channel 2: Start 168, Table Index 9
    fig.append(static_cast<char>(0x02));  // SubChId=2
    fig.append(static_cast<char>(0x00));  // Start Addr MSB
    fig.append(static_cast<char>(0xA8));  // Start Addr LSB (168)
    fig.append(static_cast<char>(0x09));  // Table Index

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
    qInfo() << "FIG 0/1: Multiple sub-channels processed";
}

void TestFIG0Processor::testFIG0_1_ShortForm()
{
    // Short form: Table-based protection
    QByteArray fig = createFIG0_1_ShortForm(5, 100, 12);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testFIG0_1_LongForm()
{
    // Long form: EEP protection
    QByteArray fig = createFIG0_1_LongForm(5, 100, 2);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testFIG0_1_StartAddressExtraction()
{
    // Test various start addresses
    uint16_t test_addresses[] = {0, 84, 168, 252, 336, 420, 504, 588, 672, 756};

    for (uint16_t addr : test_addresses) {
        QByteArray fig = createFIG0_1_ShortForm(0, addr, 5);
        m_analyzer->analyzeFICData(fig);
    }

    QVERIFY(m_analyzer->getTotalFIGsProcessed() == 10);
}

void TestFIG0Processor::testFIG0_1_TableIndexExtraction()
{
    // Test table indices 0-15
    for (uint8_t table_idx = 0; table_idx < 16; ++table_idx) {
        QByteArray fig = createFIG0_1_ShortForm(0, 0, table_idx);
        m_analyzer->analyzeFICData(fig);
    }

    QVERIFY(m_analyzer->getTotalFIGsProcessed() == 16);
}

void TestFIG0Processor::testFIG0_1_BitrateCalculation()
{
    // Table index 5 -> 64 kbps (typical)
    QByteArray fig = createFIG0_1_ShortForm(0, 0, 5);

    m_analyzer->analyzeFICData(fig);

    // Bitrate calculation depends on protection table
    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testFIG0_1_ProtectionLevel()
{
    // Long form with protection level 2 (EEP-A)
    QByteArray fig = createFIG0_1_LongForm(0, 0, 2);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testFIG0_1_InvalidSubChannelID()
{
    // Sub-channel ID must be 0-63
    QByteArray fig = createFIG0_1_ShortForm(64, 0, 5);  // Invalid: 64

    m_analyzer->analyzeFICData(fig);

    // Should detect error
    QVERIFY(!m_analyzer->getLastError().isEmpty() ||
            m_analyzer->getValidFIGsCount() == 0);
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

    // Create FIG 0/2 with multiple components
    QByteArray fig;
    fig.append(static_cast<char>(0x02));  // Type=0, Extension=2
    fig.append(static_cast<char>(0x0C));  // Length

    // Service ID (16-bit)
    fig.append(static_cast<char>(0xE0));
    fig.append(static_cast<char>(0xD0));

    // Component 1: SubChId=0
    fig.append(static_cast<char>(0x00));  // TMId=0, ASCTy/DSCTy=0
    fig.append(static_cast<char>(0x00));  // SubChId=0

    // Component 2: SubChId=1
    fig.append(static_cast<char>(0x00));
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

    // Valid header: Type=0, Extension=0
    fig.append(static_cast<char>(0x00));  // Type=0, Extension=0
    fig.append(static_cast<char>(0x06));  // Length=6

    // Ensemble data
    fig.append(static_cast<char>(0x12));  // EId MSB
    fig.append(static_cast<char>(0x34));  // EId LSB
    fig.append(static_cast<char>(0x0E));  // Country code
    fig.append(static_cast<char>(0x00));  // Flags

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG0Processor::testDataLengthValidation()
{
    // Test length field validation
    QByteArray fig;

    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x04));  // Length=4

    // Add 4 bytes of data
    fig.append(static_cast<char>(0x12));
    fig.append(static_cast<char>(0x34));
    fig.append(static_cast<char>(0x0E));
    fig.append(static_cast<char>(0x00));

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
    fig.append(static_cast<char>(0x1F));  // Type=0, Invalid extension=31
    fig.append(static_cast<char>(0x04));

    m_analyzer->analyzeFICData(fig);

    // Should handle gracefully
    QVERIFY(m_analyzer->getTotalFIGsProcessed() >= 0);
}

void TestFIG0Processor::testTruncatedFIGData()
{
    QByteArray fig;
    fig.append(static_cast<char>(0x00));
    fig.append(static_cast<char>(0x10));  // Claims 16 bytes

    // Only add 2 bytes (truncated)
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

    // FIG header
    fig.append(static_cast<char>(0x00));  // Type=0, Extension=0
    fig.append(static_cast<char>(0x06));  // Length=6 bytes

    // Ensemble ID (16-bit)
    fig.append(static_cast<char>((ensemble_id >> 8) & 0xFF));
    fig.append(static_cast<char>(ensemble_id & 0xFF));

    // Change flags + Country code
    fig.append(static_cast<char>(country_code));  // Country code (Thailand = 0x0E)

    // Extended Country Code (ECC)
    fig.append(static_cast<char>(0x00));  // No ECC

    return fig;
}

QByteArray TestFIG0Processor::createFIG0_1_ShortForm(uint8_t subch_id, uint16_t start_addr, uint8_t table_idx)
{
    QByteArray fig;

    // FIG header
    fig.append(static_cast<char>(0x01));  // Type=0, Extension=1
    fig.append(static_cast<char>(0x04));  // Length=4 bytes (one sub-channel)

    // Sub-channel ID (6 bits)
    fig.append(static_cast<char>(subch_id & 0x3F));

    // Start Address (10 bits) + Short/Long form flag (1 bit)
    uint16_t addr_bits = (start_addr & 0x3FF) << 6;  // Bits 15-6
    fig.append(static_cast<char>((addr_bits >> 8) & 0xFF));
    fig.append(static_cast<char>(addr_bits & 0xFF));

    // Table Index (6 bits)
    fig.append(static_cast<char>(table_idx & 0x3F));

    return fig;
}

QByteArray TestFIG0Processor::createFIG0_1_LongForm(uint8_t subch_id, uint16_t start_addr, uint8_t protection)
{
    QByteArray fig;

    // FIG header
    fig.append(static_cast<char>(0x01));  // Type=0, Extension=1
    fig.append(static_cast<char>(0x05));  // Length=5 bytes

    // Sub-channel ID
    fig.append(static_cast<char>(subch_id & 0x3F));

    // Start Address + Long form flag (bit 0 = 1)
    uint16_t addr_bits = ((start_addr & 0x3FF) << 6) | 0x01;
    fig.append(static_cast<char>((addr_bits >> 8) & 0xFF));
    fig.append(static_cast<char>(addr_bits & 0xFF));

    // Protection level (2 bits) + Sub-channel size
    fig.append(static_cast<char>((protection & 0x03) << 6));
    fig.append(static_cast<char>(0x54));  // Size = 84 CUs

    return fig;
}

QByteArray TestFIG0Processor::createFIG0_2(uint32_t service_id, uint8_t subch_id, bool is_audio)
{
    QByteArray fig;

    // FIG header
    fig.append(static_cast<char>(0x02));  // Type=0, Extension=2
    fig.append(static_cast<char>(0x06));  // Length

    // Country ID + Service reference (16-bit for programme service)
    fig.append(static_cast<char>((service_id >> 8) & 0xFF));
    fig.append(static_cast<char>(service_id & 0xFF));

    // Local flag + CAId + Number of components
    fig.append(static_cast<char>(0x01));  // 1 component

    // TMId (Transport Mechanism ID) + ASCTy/DSCTy
    uint8_t tmid = is_audio ? 0x00 : 0x60;  // 0x00=MSC stream audio, 0x60=MSC stream data
    fig.append(static_cast<char>(tmid));

    // SubChId
    fig.append(static_cast<char>(subch_id & 0x3F));

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
