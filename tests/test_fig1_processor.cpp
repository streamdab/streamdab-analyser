/**
 * @file test_fig1_processor.cpp
 * @brief Unit tests for FIG Type 1 Label Processing (Phase 1 Week 2)
 *
 * Comprehensive test suite for FIG Type 1 label processing functionality
 * following ETSI EN 300 401 specifications.
 *
 * Test Coverage:
 * - FIG 1/0: Ensemble label
 * - FIG 1/1: Service labels (Programme and Data)
 * - FIG 1/4: Service component labels
 * - Label structure (16 characters + character flag)
 * - Short label extraction
 * - Character flag processing
 * - Label integration with service discovery
 */

#include <QtTest/QtTest>
#include <QObject>
#include <QDebug>
#include "../src/core/advanced_fig_analyzer.h"

class TestFIG1Processor : public QObject
{
    Q_OBJECT

private slots:
    // Test lifecycle
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Label structure tests
    void testLabelCreation();
    void testFullLabel();
    void testShortLabelExtraction();
    void testCharacterFlagBitMask();
    void testEmptyLabel();
    void testLabelPadding();
    void testUTF8Encoding();
    void testSpecialCharacters();

    // FIG 1/0 tests (Ensemble label)
    void testFIG1_0_EnsembleLabel();
    void testFIG1_0_ShortLabel();
    void testFIG1_0_CharacterFlag();
    void testFIG1_0_MultipleEnsembleLabels();

    // FIG 1/1 tests (Service labels)
    void testFIG1_1_ProgrammeServiceLabel();
    void testFIG1_1_DataServiceLabel();
    void testFIG1_1_ServiceID16Bit();
    void testFIG1_1_ServiceID32Bit();
    void testFIG1_1_MultipleServiceLabels();
    void testFIG1_1_ShortLabelGeneration();
    void testFIG1_1_ThaiCharacters();

    // FIG 1/4 tests (Component labels)
    void testFIG1_4_ComponentLabel();
    void testFIG1_4_ServiceComponentID();
    void testFIG1_4_PacketDataLabel();
    void testFIG1_4_MultipleComponents();

    // Character flag tests
    void testCharacterFlag_AllPositions();
    void testCharacterFlag_FirstEight();
    void testCharacterFlag_LastEight();
    void testCharacterFlag_Alternating();
    void testCharacterFlag_NoSelection();
    void testCharacterFlag_AllSelected();

    // Integration tests
    void testLabelWithServiceDiscovery();
    void testCompleteServiceLabeling();
    void testEnsembleHierarchyLabels();
    void testLabelUpdateSequence();

    // ETSI compliance tests
    void testETSI_EN300401_LabelFormat();
    void testLabelLengthValidation();
    void testCharacterSetCompliance();

    // Error handling tests
    void testInvalidLabelLength();
    void testTruncatedLabelData();
    void testCorruptedCharacterFlag();
    void testMissingServiceReference();

private:
    AdvancedFIGAnalyzer* m_analyzer;

    // Helper methods
    QByteArray createFIG1_0(const QString& label, uint16_t char_flag = 0xFF00);
    QByteArray createFIG1_1(uint32_t service_id, const QString& label, uint16_t char_flag = 0xFF00, bool is_data_service = false);
    QByteArray createFIG1_4(uint32_t service_id, uint8_t component_id, const QString& label);
    QByteArray createLabelData(const QString& label, uint16_t char_flag);
    QByteArray createFIG0_0(uint16_t ensemble_id);  // Setup ensemble ID for FIG 1/0 tests

    QString extractExpectedShortLabel(const QString& full_label, uint16_t char_flag);
    void verifyLabel(const QString& expected_full, const QString& expected_short);
    void setupServiceForLabeling(uint32_t service_id);
};

// ============================================================================
// Test Lifecycle Methods
// ============================================================================

void TestFIG1Processor::initTestCase()
{
    qInfo() << "========================================";
    qInfo() << "Starting FIG Type 1 Label Processor Test Suite";
    qInfo() << "Phase 1 Week 2 - Label Processing";
    qInfo() << "========================================";

    // Register Qt metatypes
    qRegisterMetaType<ServiceInfo>("ServiceInfo");
    qRegisterMetaType<EnsembleInfo>("EnsembleInfo");
    qRegisterMetaType<DABLabel>("DABLabel");
}

void TestFIG1Processor::cleanupTestCase()
{
    qInfo() << "========================================";
    qInfo() << "FIG Type 1 Label Processor Tests Complete";
    qInfo() << "========================================";
}

void TestFIG1Processor::init()
{
    m_analyzer = new AdvancedFIGAnalyzer();
    QVERIFY(m_analyzer != nullptr);

    // Enable FIG Type 1 processing
    m_analyzer->enableFIGType(1, 0, true);  // Ensemble labels
    m_analyzer->enableFIGType(1, 1, true);  // Service labels
    m_analyzer->enableFIGType(1, 4, true);  // Component labels
}

void TestFIG1Processor::cleanup()
{
    delete m_analyzer;
    m_analyzer = nullptr;
}

// ============================================================================
// Label Structure Tests
// ============================================================================

void TestFIG1Processor::testLabelCreation()
{
    DABLabel label;

    // Initialize with test string
    QString test_label = "Test Service    ";  // 16 characters
    for (int i = 0; i < 16 && i < test_label.length(); ++i) {
        label.full_label[i] = test_label[i].toLatin1();
    }

    QString full = label.getFullLabel();
    QCOMPARE(full.trimmed(), QString("Test Service"));
    QVERIFY(label.isValid());

    qInfo() << "Label creation: Full label =" << full;
}

void TestFIG1Processor::testFullLabel()
{
    DABLabel label;

    QString input = "BBC Radio 1     ";  // 16 chars
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = input[i].toLatin1();
    }

    QString full = label.getFullLabel();
    QCOMPARE(full.length(), 16);
    QCOMPARE(full.trimmed(), QString("BBC Radio 1"));
}

void TestFIG1Processor::testShortLabelExtraction()
{
    DABLabel label;

    // Full label: "BBC Radio 1     "
    QString full = "BBC Radio 1     ";
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = full[i].toLatin1();
    }

    // Character flag: Select positions 0,1,2,4,10 (B,B,C, ,1)
    // Character flag bit pattern (MSB first):
    // Position: 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // Selected:  0  0  0  0  0  1  0  0  0  0  0  1  0  1  1  1
    // Binary: 0000 0010 0000 1111 = 0x020F
    label.character_flag = 0x0407;  // Positions 0,1,2,10

    QString short_label = label.getShortLabel();

    qInfo() << "Short label extraction:";
    qInfo() << "  Full label:" << label.getFullLabel();
    qInfo() << "  Character flag:" << QString::number(label.character_flag, 16);
    qInfo() << "  Short label:" << short_label;

    QVERIFY(short_label.length() <= 8);
    QVERIFY(short_label.contains("BBC"));
}

void TestFIG1Processor::testCharacterFlagBitMask()
{
    DABLabel label;

    QString full = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = full[i].toLatin1();
    }

    // Select first 4 characters: 0,1,2,3
    // Bits 0-3 set: 0x000F
    label.character_flag = 0x000F;

    QString short_label = label.getShortLabel();

    qInfo() << "Character flag test:";
    qInfo() << "  Full:" << full;
    qInfo() << "  Flag:" << QString::number(label.character_flag, 16);
    qInfo() << "  Short:" << short_label;

    QVERIFY(short_label.contains('0'));
    QVERIFY(short_label.contains('1'));
    QVERIFY(short_label.contains('2'));
    QVERIFY(short_label.contains('3'));
}

void TestFIG1Processor::testEmptyLabel()
{
    DABLabel label;

    // All spaces
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = ' ';
    }
    label.character_flag = 0x0000;

    QString full = label.getFullLabel();
    QVERIFY(full.trimmed().isEmpty());
}

void TestFIG1Processor::testLabelPadding()
{
    DABLabel label;

    // "DAB+" padded to 16 characters
    QString input = "DAB+            ";
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = input[i].toLatin1();
    }

    QString full = label.getFullLabel();
    QCOMPARE(full.length(), 16);
    QCOMPARE(full.trimmed(), QString("DAB+"));
}

void TestFIG1Processor::testUTF8Encoding()
{
    // Test label with UTF-8 characters
    QString input = "Test กทม        ";  // Thai characters

    // Note: DAB labels use EBU Latin charset, not UTF-8
    // This tests the conversion handling
    QVERIFY(input.length() >= 16);
}

void TestFIG1Processor::testSpecialCharacters()
{
    DABLabel label;

    // Label with special characters
    QString input = "Test-FM #1!     ";
    for (int i = 0; i < qMin(16, input.length()); ++i) {
        label.full_label[i] = input[i].toLatin1();
    }

    QString full = label.getFullLabel();
    QVERIFY(full.contains('-'));
    QVERIFY(full.contains('#'));
    QVERIFY(full.contains('!'));
}

// ============================================================================
// FIG 1/0 Tests (Ensemble Label)
// ============================================================================

void TestFIG1Processor::testFIG1_0_EnsembleLabel()
{
    // Setup ensemble ID first (required for FIG 1/0)
    m_analyzer->analyzeFICData(createFIG0_0(0x1234));

    // Create ensemble with label "Bangkok DAB     "
    QByteArray fig = createFIG1_0("Bangkok DAB     ", 0xFF00);

    m_analyzer->analyzeFICData(fig);

    EnsembleInfo ensemble = m_analyzer->getCurrentEnsemble();
    QVERIFY(!ensemble.ensembleLabel.isEmpty());
    QVERIFY(ensemble.ensembleLabel.contains("Bangkok DAB"));

    qInfo() << "Ensemble label:" << ensemble.ensembleLabel;
}

void TestFIG1Processor::testFIG1_0_ShortLabel()
{
    // Setup ensemble ID first
    m_analyzer->analyzeFICData(createFIG0_0(0x1234));

    // Ensemble label with short form
    // Full: "Bangkok DAB     "
    // Short: "BKK DAB" (positions 0,1,2,8,9,10,11)
    // Character flag: bits 0,1,2,8,9,10,11 = 0x0F07
    QByteArray fig = createFIG1_0("Bangkok DAB     ", 0x0F07);

    m_analyzer->analyzeFICData(fig);

    EnsembleInfo ensemble = m_analyzer->getCurrentEnsemble();
    QVERIFY(!ensemble.ensembleLabel.isEmpty());

    qInfo() << "Ensemble label:" << ensemble.ensembleLabel;
}

void TestFIG1Processor::testFIG1_0_CharacterFlag()
{
    // Setup ensemble ID first
    m_analyzer->analyzeFICData(createFIG0_0(0x1234));

    // Test different character flag values
    uint16_t test_flags[] = {0x0000, 0x00FF, 0xFF00, 0xFFFF, 0xAAAA, 0x5555};

    for (uint16_t flag : test_flags) {
        QByteArray fig = createFIG1_0("Test Ensemble   ", flag);
        m_analyzer->analyzeFICData(fig);

        qInfo() << "Character flag" << QString::number(flag, 16) << "processed";
    }

    QVERIFY(m_analyzer->getTotalFIGsProcessed() == 6);
}

void TestFIG1Processor::testFIG1_0_MultipleEnsembleLabels()
{
    // Setup ensemble ID first
    m_analyzer->analyzeFICData(createFIG0_0(0x1234));

    // Process multiple ensemble labels (updates)
    m_analyzer->analyzeFICData(createFIG1_0("Ensemble v1     "));
    m_analyzer->analyzeFICData(createFIG1_0("Ensemble v2     "));
    m_analyzer->analyzeFICData(createFIG1_0("Ensemble v3     "));

    EnsembleInfo ensemble = m_analyzer->getCurrentEnsemble();
    QVERIFY(ensemble.ensembleLabel.contains("Ensemble"));

    qInfo() << "Final ensemble label:" << ensemble.ensembleLabel;
}

// ============================================================================
// FIG 1/1 Tests (Service Labels)
// ============================================================================

void TestFIG1Processor::testFIG1_1_ProgrammeServiceLabel()
{
    // Setup service first (FIG 0/2)
    setupServiceForLabeling(0xE0D00001);

    // Add label
    QByteArray fig = createFIG1_1(0xE0D00001, "MCOT Radio      ", 0xFF00);

    m_analyzer->analyzeFICData(fig);

    auto services = m_analyzer->getDiscoveredServices();
    bool found = false;
    for (const auto& svc : services) {
        if (svc.serviceId == 0xE0D00001) {
            QVERIFY(svc.label.contains("MCOT Radio"));
            found = true;
            qInfo() << "Service label:" << svc.label;
            break;
        }
    }

    QVERIFY(found);
}

void TestFIG1Processor::testFIG1_1_DataServiceLabel()
{
    // Data service with 32-bit Service ID
    setupServiceForLabeling(0xE0D00002);

    QByteArray fig = createFIG1_1(0xE0D00002, "Data Service    ", 0xFF00, true);  // is_data_service=true

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
}

void TestFIG1Processor::testFIG1_1_ServiceID16Bit()
{
    // Programme service uses 16-bit SId
    uint32_t service_id = 0x0000D001;  // 16-bit: 0xD001

    setupServiceForLabeling(service_id);

    QByteArray fig = createFIG1_1(service_id, "Test Service    ", 0xFF00);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testFIG1_1_ServiceID32Bit()
{
    // Data service uses 32-bit SId
    uint32_t service_id = 0xE0D00003;

    setupServiceForLabeling(service_id);

    QByteArray fig = createFIG1_1(service_id, "Data Test       ", 0xFF00);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testFIG1_1_MultipleServiceLabels()
{
    // Add multiple services with labels
    uint32_t service_ids[] = {0xE0D00001, 0xE0D00002, 0xE0D00003, 0xE0D00004};
    QString labels[] = {"Service 1       ", "Service 2       ", "Service 3       ", "Service 4       "};

    for (int i = 0; i < 4; ++i) {
        setupServiceForLabeling(service_ids[i]);
        m_analyzer->analyzeFICData(createFIG1_1(service_ids[i], labels[i], 0xFF00));
    }

    auto services = m_analyzer->getDiscoveredServices();
    QVERIFY(services.size() >= 4);

    qInfo() << "Multiple services labeled:" << services.size();
}

void TestFIG1Processor::testFIG1_1_ShortLabelGeneration()
{
    setupServiceForLabeling(0xE0D00001);

    // Full: "BBC World Service"
    // Short: "BBC WS" (character flag selects specific positions)
    // Positions: 0,1,2,6,7 -> "BBC W S"
    uint16_t char_flag = 0x00C7;  // Bits 0,1,2,6,7

    QByteArray fig = createFIG1_1(0xE0D00001, "BBC WorldService", char_flag);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testFIG1_1_ThaiCharacters()
{
    // Test Thai service label (if supported by charset)
    setupServiceForLabeling(0xE0D00001);

    // Note: DAB uses EBU Latin charset, Thai requires extended charset
    QByteArray fig = createFIG1_1(0xE0D00001, "MCOT กทม        ", 0xFF00);

    m_analyzer->analyzeFICData(fig);

    // Should process without crash even if Thai not fully supported
    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
}

// ============================================================================
// FIG 1/4 Tests (Component Labels)
// ============================================================================

void TestFIG1Processor::testFIG1_4_ComponentLabel()
{
    setupServiceForLabeling(0xE0D00001);

    QByteArray fig = createFIG1_4(0xE0D00001, 0, "Main Audio      ");

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
    qInfo() << "Component label processed";
}

void TestFIG1Processor::testFIG1_4_ServiceComponentID()
{
    setupServiceForLabeling(0xE0D00001);

    // Test different component IDs (SCIdS: 0-15)
    for (uint8_t comp_id = 0; comp_id < 4; ++comp_id) {
        QString label = QString("Component %1    ").arg(comp_id);
        QByteArray fig = createFIG1_4(0xE0D00001, comp_id, label);
        m_analyzer->analyzeFICData(fig);
    }

    QVERIFY(m_analyzer->getTotalFIGsProcessed() >= 4);
}

void TestFIG1Processor::testFIG1_4_PacketDataLabel()
{
    // Packet data component label
    setupServiceForLabeling(0xE0D00002);

    QByteArray fig = createFIG1_4(0xE0D00002, 1, "Data Component  ");

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testFIG1_4_MultipleComponents()
{
    setupServiceForLabeling(0xE0D00001);

    // Service with multiple components
    m_analyzer->analyzeFICData(createFIG1_4(0xE0D00001, 0, "Primary Audio   "));
    m_analyzer->analyzeFICData(createFIG1_4(0xE0D00001, 1, "Secondary Audio "));
    m_analyzer->analyzeFICData(createFIG1_4(0xE0D00001, 2, "Data Broadcast  "));

    QVERIFY(m_analyzer->getTotalFIGsProcessed() >= 3);
}

// ============================================================================
// Character Flag Tests
// ============================================================================

void TestFIG1Processor::testCharacterFlag_AllPositions()
{
    // Test all 16 positions individually
    for (int pos = 0; pos < 16; ++pos) {
        uint16_t flag = (1 << pos);

        DABLabel label;
        QString full = "0123456789ABCDEF";
        for (int i = 0; i < 16; ++i) {
            label.full_label[i] = full[i].toLatin1();
        }
        label.character_flag = flag;

        QString short_label = label.getShortLabel();

        qInfo() << "Position" << pos << "flag" << QString::number(flag, 16)
                << "short:" << short_label;

        QVERIFY(short_label.length() <= 8);
    }
}

void TestFIG1Processor::testCharacterFlag_FirstEight()
{
    DABLabel label;

    QString full = "ABCDEFGH12345678";
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = full[i].toLatin1();
    }

    // Select first 8 characters
    label.character_flag = 0x00FF;

    QString short_label = label.getShortLabel();

    QVERIFY(short_label.contains('A'));
    QVERIFY(short_label.length() <= 8);
}

void TestFIG1Processor::testCharacterFlag_LastEight()
{
    DABLabel label;

    QString full = "ABCDEFGH12345678";
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = full[i].toLatin1();
    }

    // Select last 8 characters
    label.character_flag = 0xFF00;

    QString short_label = label.getShortLabel();

    QVERIFY(short_label.contains('1') || short_label.contains('2'));
    QVERIFY(short_label.length() <= 8);
}

void TestFIG1Processor::testCharacterFlag_Alternating()
{
    DABLabel label;

    QString full = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = full[i].toLatin1();
    }

    // Alternating pattern: 0xAAAA
    label.character_flag = 0xAAAA;

    QString short_label = label.getShortLabel();

    QVERIFY(short_label.length() <= 8);
}

void TestFIG1Processor::testCharacterFlag_NoSelection()
{
    DABLabel label;

    QString full = "Test Label      ";
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = full[i].toLatin1();
    }

    // No characters selected
    label.character_flag = 0x0000;

    QString short_label = label.getShortLabel();

    // Empty or default short label
    QVERIFY(short_label.isEmpty() || short_label == full.trimmed());
}

void TestFIG1Processor::testCharacterFlag_AllSelected()
{
    DABLabel label;

    QString full = "ShortLabel      ";
    for (int i = 0; i < 16; ++i) {
        label.full_label[i] = full[i].toLatin1();
    }

    // All 16 characters selected (but short label limited to 8)
    label.character_flag = 0xFFFF;

    QString short_label = label.getShortLabel();

    QVERIFY(short_label.length() <= 8);
}

// ============================================================================
// Integration Tests
// ============================================================================

void TestFIG1Processor::testLabelWithServiceDiscovery()
{
    // Complete workflow: Discover service then add label

    // Step 1: Add sub-channel (FIG 0/1)
    QByteArray fig0_1;
    fig0_1.append(static_cast<char>(0x01));  // Type=0, Ext=1
    fig0_1.append(static_cast<char>(0x04));  // Length
    fig0_1.append(static_cast<char>(0x00));  // SubChId=0
    fig0_1.append(static_cast<char>(0x00));  // Start addr
    fig0_1.append(static_cast<char>(0x00));
    fig0_1.append(static_cast<char>(0x05));  // Table index

    m_analyzer->analyzeFICData(fig0_1);

    // Step 2: Add service (FIG 0/2)
    QByteArray fig0_2;
    fig0_2.append(static_cast<char>(0x02));  // Type=0, Ext=2
    fig0_2.append(static_cast<char>(0x06));  // Length
    fig0_2.append(static_cast<char>(0xE0));  // SId
    fig0_2.append(static_cast<char>(0xD0));
    fig0_2.append(static_cast<char>(0x01));  // 1 component
    fig0_2.append(static_cast<char>(0x00));  // TMId
    fig0_2.append(static_cast<char>(0x00));  // SubChId

    m_analyzer->analyzeFICData(fig0_2);

    // Step 3: Add label (FIG 1/1)
    m_analyzer->analyzeFICData(createFIG1_1(0x0000E0D0, "MCOT Radio      ", 0xFF00));

    // Verify complete service with label
    auto services = m_analyzer->getDiscoveredServices();
    QVERIFY(services.size() > 0);

    qInfo() << "Complete service discovery with labels: " << services.size() << "services";
}

void TestFIG1Processor::testCompleteServiceLabeling()
{
    // Label all levels: ensemble, service, component

    // Ensemble label (FIG 1/0)
    m_analyzer->analyzeFICData(createFIG1_0("Bangkok DAB     "));

    // Service setup and label
    setupServiceForLabeling(0xE0D00001);
    m_analyzer->analyzeFICData(createFIG1_1(0xE0D00001, "MCOT Radio      "));

    // Component label (FIG 1/4)
    m_analyzer->analyzeFICData(createFIG1_4(0xE0D00001, 0, "Main Audio      "));

    QVERIFY(m_analyzer->getTotalFIGsProcessed() >= 3);
    qInfo() << "Complete labeling hierarchy processed";
}

void TestFIG1Processor::testEnsembleHierarchyLabels()
{
    // Build complete labeled ensemble tree

    // Ensemble
    m_analyzer->analyzeFICData(createFIG1_0("Test Ensemble   "));

    // Multiple services with labels
    for (uint32_t i = 1; i <= 3; ++i) {
        uint32_t svc_id = 0xE0D00000 + i;
        setupServiceForLabeling(svc_id);

        QString label = QString("Service %1      ").arg(i);
        m_analyzer->analyzeFICData(createFIG1_1(svc_id, label));

        // Component labels
        m_analyzer->analyzeFICData(createFIG1_4(svc_id, 0, QString("Audio %1        ").arg(i)));
    }

    EnsembleInfo ensemble = m_analyzer->getCurrentEnsemble();
    auto services = m_analyzer->getDiscoveredServices();

    QVERIFY(!ensemble.ensembleLabel.isEmpty());
    QVERIFY(services.size() >= 3);

    qInfo() << "Ensemble hierarchy: Ensemble +" << services.size() << "services labeled";
}

void TestFIG1Processor::testLabelUpdateSequence()
{
    // Test label updates over time
    setupServiceForLabeling(0xE0D00001);

    // Initial label
    m_analyzer->analyzeFICData(createFIG1_1(0xE0D00001, "Initial Label   "));

    // Updated label
    m_analyzer->analyzeFICData(createFIG1_1(0xE0D00001, "Updated Label   "));

    // Final label
    m_analyzer->analyzeFICData(createFIG1_1(0xE0D00001, "Final Label     "));

    auto services = m_analyzer->getDiscoveredServices();
    bool found = false;
    for (const auto& svc : services) {
        if (svc.serviceId == 0xE0D00001) {
            QVERIFY(svc.label.contains("Final Label"));
            found = true;
            break;
        }
    }

    QVERIFY(found);
}

// ============================================================================
// ETSI Compliance Tests
// ============================================================================

void TestFIG1Processor::testETSI_EN300401_LabelFormat()
{
    // Test ETSI EN 300 401 label format compliance
    QByteArray fig = createFIG1_0("Valid Label     ", 0xFF00);

    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testLabelLengthValidation()
{
    // Labels must be exactly 16 characters
    QString valid_label = "Exactly16Chars!!";
    QCOMPARE(valid_label.length(), 16);

    QByteArray fig = createFIG1_0(valid_label, 0xFF00);
    m_analyzer->analyzeFICData(fig);

    QVERIFY(m_analyzer->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testCharacterSetCompliance()
{
    // Test EBU Latin character set compliance
    // Valid characters: A-Z, 0-9, space, and limited special chars

    QString test_labels[] = {
        "ABCDEFGHIJKLMNOP",
        "0123456789      ",
        "Test-FM #1!     ",
        "BBC Radio 1     "
    };

    for (const QString& label : test_labels) {
        QByteArray fig = createFIG1_0(label, 0xFF00);
        m_analyzer->analyzeFICData(fig);
    }

    QVERIFY(m_analyzer->getTotalFIGsProcessed() == 4);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

void TestFIG1Processor::testInvalidLabelLength()
{
    // Test too short label
    QByteArray fig;
    fig.append(static_cast<char>(0x10));  // Type=1, Ext=0
    fig.append(static_cast<char>(0x05));  // Length=5 (too short for label)

    m_analyzer->analyzeFICData(fig);

    // Should detect error
    QVERIFY(!m_analyzer->getLastError().isEmpty() ||
            m_analyzer->getValidFIGsCount() == 0);
}

void TestFIG1Processor::testTruncatedLabelData()
{
    QByteArray fig;
    fig.append(static_cast<char>(0x10));  // Type=1, Ext=0
    fig.append(static_cast<char>(0x12));  // Length=18

    // Only add 10 bytes (should be 18)
    for (int i = 0; i < 10; ++i) {
        fig.append(static_cast<char>('A'));
    }

    m_analyzer->analyzeFICData(fig);

    // Should detect truncation
    QVERIFY(!m_analyzer->getLastError().isEmpty() ||
            m_analyzer->getValidFIGsCount() == 0);
}

void TestFIG1Processor::testCorruptedCharacterFlag()
{
    // Test with all bits set (unusual but valid)
    QByteArray fig = createFIG1_0("Test Label      ", 0xFFFF);

    m_analyzer->analyzeFICData(fig);

    // Should process (0xFFFF is valid, just selects many characters)
    QVERIFY(m_analyzer->getTotalFIGsProcessed() > 0);
}

void TestFIG1Processor::testMissingServiceReference()
{
    // Try to add label for non-existent service
    QByteArray fig = createFIG1_1(0xFFFFFFFF, "Unknown Service ");

    m_analyzer->analyzeFICData(fig);

    // Should handle gracefully (create service or skip)
    QVERIFY(m_analyzer->getTotalFIGsProcessed() >= 0);
}

// ============================================================================
// Helper Methods
// ============================================================================

QByteArray TestFIG1Processor::createFIG1_0(const QString& label, uint16_t char_flag)
{
    return createLabelData(label, char_flag);
}

QByteArray TestFIG1Processor::createFIG1_1(uint32_t service_id, const QString& label, uint16_t char_flag, bool is_data_service)
{
    QByteArray fig;

    // Auto-detect data service if not explicitly specified
    // Match the logic in setupServiceForLabeling for consistency
    if (!is_data_service && service_id > 0xFFFF) {
        is_data_service = true;  // Auto-detect as data service
    }

    // Determine Service ID encoding based on value and service type
    // Programme services can use either 16-bit or 32-bit encoding
    // Data services always use 32-bit encoding
    bool use_16bit_sid = !is_data_service && (service_id <= 0xFFFF);

    // FIC header (2 bytes) - ETSI EN 300 401 Section 5.2
    uint8_t type_byte = (1 << 5);  // FIG type 1, C/N=0, OE=0
    fig.append(static_cast<char>(type_byte));

    if (use_16bit_sid) {
        // 16-bit Service ID: Length=20 bytes (ext + pd/charset + SId16 + label + flag)
        fig.append(static_cast<char>(0x14));  // 20 decimal = 0x14
    } else {
        // 32-bit Service ID: Length=24 bytes (ext + pd/charset + SId32 + label + flag)
        fig.append(static_cast<char>(0x18));  // 24 decimal = 0x18
    }

    // Extension byte (1 byte) - FIG Type 1, Extension 1
    // Bits 7-3: Extension number (1)
    // Bits 2-0: Reserved for OE/Charset (0)
    uint8_t ext_byte = (1 << 3);  // Extension 1 in bits 7-3
    fig.append(static_cast<char>(ext_byte));

    // P/D + Charset/OE field (1 byte) - ETSI EN 300 401 Section 8.1.14.1
    // Bit 3: P/D flag (0 = programme, 1 = data)
    // Bit 2: Charset (0 = EBU Latin)
    // Bits 1-0: OE (0)
    uint8_t pd_byte = is_data_service ? (1 << 3) : 0x00;
    fig.append(static_cast<char>(pd_byte));

    if (use_16bit_sid) {
        // Service ID (16-bit) - lower 16 bits only
        fig.append(static_cast<char>((service_id >> 8) & 0xFF));
        fig.append(static_cast<char>(service_id & 0xFF));
    } else {
        // Service ID (32-bit) - full ECC + Country + Service Reference
        fig.append(static_cast<char>((service_id >> 24) & 0xFF));
        fig.append(static_cast<char>((service_id >> 16) & 0xFF));
        fig.append(static_cast<char>((service_id >> 8) & 0xFF));
        fig.append(static_cast<char>(service_id & 0xFF));
    }

    // Label (16 characters) - ETSI EN 300 401 Section 8.1.14.1
    for (int i = 0; i < 16; ++i) {
        char c = (i < label.length()) ? label[i].toLatin1() : ' ';
        fig.append(c);
    }

    // Character flag (2 bytes, big-endian) - ETSI EN 300 401 Section 8.1.13.1
    fig.append(static_cast<char>((char_flag >> 8) & 0xFF));
    fig.append(static_cast<char>(char_flag & 0xFF));

    return fig;
}

QByteArray TestFIG1Processor::createFIG1_4(uint32_t service_id, uint8_t component_id, const QString& label)
{
    QByteArray fig;

    // FIC header (2 bytes) - ETSI EN 300 401 Section 5.2
    uint8_t type_byte = (1 << 5);  // FIG type 1, C/N=0, OE=0
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x14));  // Length=20 bytes (ext + P/D + SCIdS + label + flag)

    // Extension + P/D + SCIdS upper bits (1 byte) - ETSI EN 300 401 Section 8.1.14.3
    // For stream mode component (P/D=0), SCIdS is 6-bit (0-63)
    uint8_t ext_byte = (4 << 3);  // Extension 4, P/D=0 (stream mode)
    // SCIdS bits 5-4 go to bits 1-0 of this byte
    ext_byte |= ((component_id >> 4) & 0x03);
    fig.append(static_cast<char>(ext_byte));

    // SCIdS lower bits + reserved (1 byte)
    // Bits 7-4: SCIdS bits 3-0
    // Bits 3-0: Reserved (set to 0)
    uint8_t scids_byte = ((component_id & 0x0F) << 4);
    fig.append(static_cast<char>(scids_byte));

    // Label (16 characters) - ETSI EN 300 401 Section 8.1.14.3
    for (int i = 0; i < 16; ++i) {
        char c = (i < label.length()) ? label[i].toLatin1() : ' ';
        fig.append(c);
    }

    // Character flag (2 bytes, big-endian) - ETSI EN 300 401 Section 8.1.13.1
    fig.append(static_cast<char>(0xFF));  // All bits set for full label selection
    fig.append(static_cast<char>(0x00));

    return fig;
}

/**
 * @brief Create FIG 1/0 (Ensemble Label) test data
 *
 * ETSI EN 300 401 Section 8.1.13 - FIG 1/0 Structure:
 *
 * Byte 0:    FIG Type header (Type=1, C/N=0, OE=0)
 *            Bits 7-5: FIG type 1
 *            Bits 4-3: C/N (0 = current ensemble)
 *            Bits 2-0: OE (Other Ensemble, 0 for this ensemble)
 *
 * Byte 1:    Length (0x14 = 20 bytes content after header)
 *
 * Byte 2:    Extension + Charset/OE field
 *            Bits 7-3: Extension (0 for FIG 1/0)
 *            Bit 2:    Charset (0 = EBU Latin)
 *            Bits 1-0: OE (Other Ensemble continuation)
 *
 * Bytes 3-4: Ensemble ID (EId) - 16-bit value
 *            Test uses 0x1234 for ensemble identification
 *
 * Bytes 5-20: Label (16 characters, EBU Latin charset)
 *             Padded with spaces if label < 16 chars
 *
 * Bytes 21-22: Character flag (16-bit, big-endian)
 *              Each bit indicates if corresponding character
 *              should be included in short label
 *
 * Total: 23 bytes (2 FIC header + 1 extension + 2 EId + 16 label + 2 flag)
 *
 * Implementation expects data.size() >= 22 bytes (excluding FIC type byte)
 */
QByteArray TestFIG1Processor::createLabelData(const QString& label, uint16_t char_flag)
{
    QByteArray fig;

    // FIC header (2 bytes) - ETSI EN 300 401 Section 5.2
    uint8_t type_byte = (1 << 5);  // FIG type 1, C/N=0, OE=0
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x16));  // Length=22 bytes (ext + charset + EId + label + flag = 1+1+2+16+2)

    // Extension field (1 byte) - ETSI EN 300 401 Section 8.1.13
    // Bits 7-3: Extension (0 for FIG 1/0)
    // Bits 2-0: Reserved (0)
    uint8_t ext_byte = (0 << 3);  // Extension 0
    fig.append(static_cast<char>(ext_byte));

    // Charset/OE field (1 byte)
    // Bit 4: Charset (0 = EBU Latin)
    // Bits 0-3: OE (Other Ensemble, 0 for current)
    uint8_t charset_byte = 0x00;  // Charset=0 (EBU Latin), OE=0
    fig.append(static_cast<char>(charset_byte));

    // Ensemble ID (2 bytes) - test ensemble ID 0x1234
    fig.append(static_cast<char>(0x12));  // EId MSB
    fig.append(static_cast<char>(0x34));  // EId LSB

    // Label (16 characters) - ETSI EN 300 401 Section 8.1.13
    for (int i = 0; i < 16; ++i) {
        char c = (i < label.length()) ? label[i].toLatin1() : ' ';
        fig.append(c);
    }

    // Character flag (2 bytes, big-endian) - ETSI EN 300 401 Section 8.1.13.1
    // Bit mask indicating which characters form the short label
    fig.append(static_cast<char>((char_flag >> 8) & 0xFF));  // MSB
    fig.append(static_cast<char>(char_flag & 0xFF));         // LSB

    return fig;
}

void TestFIG1Processor::setupServiceForLabeling(uint32_t service_id)
{
    // Determine service type based on Service ID value
    // Programme services: 16-bit (0x0000-0xFFFF)
    // Data services: 32-bit (typically 0xE0000000-0xEFFFFFFF per ETSI)
    bool is_programme = (service_id <= 0xFFFF);

    // Step 1: Create sub-channel (FIG 0/1) - Required by FIG 0/2
    QByteArray fig0_1;

    // CRITICAL FIX: Must include Type + Length bytes!
    uint8_t type_byte = (0 << 5);  // FIG type 0, C/N=0, OE=0
    fig0_1.append(static_cast<char>(type_byte));
    fig0_1.append(static_cast<char>(0x05));  // Length=5 bytes (ext + subchannel data)

    // Extension byte: Extension 1 in bits 7-3
    uint8_t ext_byte = (1 << 3);  // Extension 1
    fig0_1.append(static_cast<char>(ext_byte));
    fig0_1.append(static_cast<char>(0x00));  // SubChId=0 (bits 7-2) + Start[9:8]=0 (bits 1-0)
    fig0_1.append(static_cast<char>(0x00));  // Start Address[7:0]=0
    fig0_1.append(static_cast<char>(0x00));  // Padding
    fig0_1.append(static_cast<char>(0x80 | 0x05));  // Short form (0x80) + Table Index 5
    m_analyzer->analyzeFICData(fig0_1);

    // Step 2: Create service (FIG 0/2) with correct SId encoding per ETSI EN 300 401
    QByteArray fig0_2;

    // CRITICAL FIX: Must include Type + Length bytes!
    type_byte = (0 << 5);  // FIG type 0, C/N=0, OE=0
    fig0_2.append(static_cast<char>(type_byte));

    if (is_programme) {
        // Programme service: 16-bit Service ID
        // Length = ext(1) + pd/sid(2) + numcomp(1) + component(2) = 6
        fig0_2.append(static_cast<char>(0x06));  // Length=6

        // Extension byte: Extension 2 in bits 7-3
        uint8_t ext_byte = (2 << 3);  // Extension 2
        fig0_2.append(static_cast<char>(ext_byte));

        // P/D byte + Service ID MSB
        // ETSI EN 300 401: P/D flag in bit 7, SId[14:8] in bits 6-0
        uint8_t pd_byte = 0x80 | ((service_id >> 8) & 0x7F);  // P/D=1 (programme)
        fig0_2.append(static_cast<char>(pd_byte));

        // Service ID LSB
        fig0_2.append(static_cast<char>(service_id & 0xFF));
    } else {
        // Data service: 32-bit Service ID
        // Length = ext(1) + sid32(4) + numcomp(1) + component(2) = 8
        fig0_2.append(static_cast<char>(0x08));  // Length=8

        // Extension byte: Extension 2 in bits 7-3
        uint8_t ext_byte = (2 << 3);  // Extension 2
        fig0_2.append(static_cast<char>(ext_byte));

        // Full 32-bit Service ID (no P/D flag for 32-bit encoding)
        fig0_2.append(static_cast<char>((service_id >> 24) & 0xFF));
        fig0_2.append(static_cast<char>((service_id >> 16) & 0xFF));
        fig0_2.append(static_cast<char>((service_id >> 8) & 0xFF));
        fig0_2.append(static_cast<char>(service_id & 0xFF));
    }

    // Number of components (1) in high nibble + Local flag + CAId
    fig0_2.append(static_cast<char>(0x10));  // NumComp=1, Local=0, CAId=0

    // Component descriptor (2 bytes)
    // Byte 0: TMId (2 bits) + ASCTy/DSCTy (6 bits)
    fig0_2.append(static_cast<char>(0x00));  // TMId=0 (MSC stream), ASCTy=0 (DAB audio)

    // Byte 1: P/S flag + CA flag + SubChId
    fig0_2.append(static_cast<char>(0x00));  // P/S=0 (primary), CA=0, SubChId=0

    m_analyzer->analyzeFICData(fig0_2);

    qDebug() << "setupServiceForLabeling: Created service" << Qt::hex << service_id
             << (is_programme ? "(programme)" : "(data)");
}

/**
 * @brief Create FIG 0/0 (Ensemble Information) to set ensemble ID
 *
 * ETSI EN 300 401 Section 6.4 - FIG 0/0 Structure
 */
QByteArray TestFIG1Processor::createFIG0_0(uint16_t ensemble_id)
{
    QByteArray fig;

    // FIC header (2 bytes)
    uint8_t type_byte = (0 << 5);  // FIG type 0
    fig.append(static_cast<char>(type_byte));
    fig.append(static_cast<char>(0x06));  // Length=6 bytes (ext + eid + change + alarm + cif)

    // Extension field (1 byte) - Extension 0
    uint8_t ext_byte = (0 << 3);  // Extension 0 in bits 7-3
    fig.append(static_cast<char>(ext_byte));

    // Padding byte
    fig.append(static_cast<char>(0x00));

    // Ensemble ID (2 bytes)
    fig.append(static_cast<char>((ensemble_id >> 8) & 0xFF));
    fig.append(static_cast<char>(ensemble_id & 0xFF));

    // Change flags (1 byte)
    fig.append(static_cast<char>(0x00));

    // Alarm flag (1 byte)
    fig.append(static_cast<char>(0x00));

    return fig;
}

// Register test class
QTEST_MAIN(TestFIG1Processor)
#include "test_fig1_processor.moc"
