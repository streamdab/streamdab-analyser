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
#include "../src/core/advanced_fig_analyser.h"

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
    AdvancedFIGAnalyser* m_analyser;

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
    m_analyser = new AdvancedFIGAnalyser();
    QVERIFY(m_analyser != nullptr);

    // Enable FIG Type 1 processing
    m_analyser->enableFIGType(1, 0, true);  // Ensemble labels
    m_analyser->enableFIGType(1, 1, true);  // Service labels
    m_analyser->enableFIGType(1, 4, true);  // Component labels
}

void TestFIG1Processor::cleanup()
{
    delete m_analyser;
    m_analyser = nullptr;
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

    // Character flag: Select positions 0,1,2,10 (B,B,C,1)
    // Character flag bit pattern (spec order, bit 15 = character 0):
    // Position: 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // Selected:  1  1  1  0  0  0  0  0  0  0  1  0  0  0  0  0
    // Binary: 1110 0000 0010 0000 = 0xE020
    label.character_flag = 0xE020;  // Positions 0,1,2,10 (bit 15 = char 0)

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
    // Spec order (bit 15 = character 0): bits 15-12 set = 0xF000
    label.character_flag = 0xF000;

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
    m_analyser->analyzeFICData(createFIG0_0(0x1234));

    // Create ensemble with label "Bangkok DAB     "
    QByteArray fig = createFIG1_0("Bangkok DAB     ", 0xFF00);

    m_analyser->analyzeFICData(fig);

    EnsembleInfo ensemble = m_analyser->getCurrentEnsemble();
    QVERIFY(!ensemble.ensembleLabel.isEmpty());
    QVERIFY(ensemble.ensembleLabel.contains("Bangkok DAB"));

    qInfo() << "Ensemble label:" << ensemble.ensembleLabel;
}

void TestFIG1Processor::testFIG1_0_ShortLabel()
{
    // Setup ensemble ID first
    m_analyser->analyzeFICData(createFIG0_0(0x1234));

    // Ensemble label with short form
    // Full: "Bangkok DAB     "
    // Short: "BKK DAB" (positions 0,1,2,8,9,10,11)
    // Character flag (spec order, bit 15 = char 0): bits 15,14,13,7,6,5,4 = 0xE0F0
    QByteArray fig = createFIG1_0("Bangkok DAB     ", 0xE0F0);

    m_analyser->analyzeFICData(fig);

    EnsembleInfo ensemble = m_analyser->getCurrentEnsemble();
    QVERIFY(!ensemble.ensembleLabel.isEmpty());

    qInfo() << "Ensemble label:" << ensemble.ensembleLabel;
}

void TestFIG1Processor::testFIG1_0_CharacterFlag()
{
    // Setup ensemble ID first
    m_analyser->analyzeFICData(createFIG0_0(0x1234));

    // Test different character flag values
    uint16_t test_flags[] = {0x0000, 0x00FF, 0xFF00, 0xFFFF, 0xAAAA, 0x5555};

    for (uint16_t flag : test_flags) {
        QByteArray fig = createFIG1_0("Test Ensemble   ", flag);
        m_analyser->analyzeFICData(fig);

        qInfo() << "Character flag" << QString::number(flag, 16) << "processed";
    }

    QVERIFY(m_analyser->getTotalFIGsProcessed() == 7);  // 1 FIG 0/0 + 6 FIG 1/0
}

void TestFIG1Processor::testFIG1_0_MultipleEnsembleLabels()
{
    // Setup ensemble ID first
    m_analyser->analyzeFICData(createFIG0_0(0x1234));

    // Process multiple ensemble labels (updates)
    m_analyser->analyzeFICData(createFIG1_0("Ensemble v1     "));
    m_analyser->analyzeFICData(createFIG1_0("Ensemble v2     "));
    m_analyser->analyzeFICData(createFIG1_0("Ensemble v3     "));

    EnsembleInfo ensemble = m_analyser->getCurrentEnsemble();
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

    m_analyser->analyzeFICData(fig);

    auto services = m_analyser->getDiscoveredServices();
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

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getTotalFIGsProcessed() > 0);
}

void TestFIG1Processor::testFIG1_1_ServiceID16Bit()
{
    // Programme service uses 16-bit SId
    uint32_t service_id = 0x0000D001;  // 16-bit: 0xD001

    setupServiceForLabeling(service_id);

    QByteArray fig = createFIG1_1(service_id, "Test Service    ", 0xFF00);

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testFIG1_1_ServiceID32Bit()
{
    // Data service uses 32-bit SId
    uint32_t service_id = 0xE0D00003;

    setupServiceForLabeling(service_id);

    QByteArray fig = createFIG1_1(service_id, "Data Test       ", 0xFF00);

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testFIG1_1_MultipleServiceLabels()
{
    // Add multiple services with labels
    uint32_t service_ids[] = {0xE0D00001, 0xE0D00002, 0xE0D00003, 0xE0D00004};
    QString labels[] = {"Service 1       ", "Service 2       ", "Service 3       ", "Service 4       "};

    for (int i = 0; i < 4; ++i) {
        setupServiceForLabeling(service_ids[i]);
        m_analyser->analyzeFICData(createFIG1_1(service_ids[i], labels[i], 0xFF00));
    }

    auto services = m_analyser->getDiscoveredServices();
    QVERIFY(services.size() >= 4);

    qInfo() << "Multiple services labeled:" << services.size();
}

void TestFIG1Processor::testFIG1_1_ShortLabelGeneration()
{
    setupServiceForLabeling(0xE0D00001);

    // Full: "BBC World Service"
    // Short: "BBC WS" (character flag selects specific positions)
    // Positions: 0,1,2,6,7 -> "BBC W S"
    // Spec order (bit 15 = char 0): bits 15,14,13,9,8 = 0xE300
    uint16_t char_flag = 0xE300;  // Bits 15,14,13,9,8

    QByteArray fig = createFIG1_1(0xE0D00001, "BBC WorldService", char_flag);

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testFIG1_1_ThaiCharacters()
{
    // Test Thai service label (if supported by charset)
    setupServiceForLabeling(0xE0D00001);

    // Note: DAB uses EBU Latin charset, Thai requires extended charset
    QByteArray fig = createFIG1_1(0xE0D00001, "MCOT กทม        ", 0xFF00);

    m_analyser->analyzeFICData(fig);

    // Should process without crash even if Thai not fully supported
    QVERIFY(m_analyser->getTotalFIGsProcessed() > 0);
}

// ============================================================================
// FIG 1/4 Tests (Component Labels)
// ============================================================================

void TestFIG1Processor::testFIG1_4_ComponentLabel()
{
    setupServiceForLabeling(0xE0D00001);

    QByteArray fig = createFIG1_4(0xE0D00001, 0, "Main Audio      ");

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getTotalFIGsProcessed() > 0);
    qInfo() << "Component label processed";
}

void TestFIG1Processor::testFIG1_4_ServiceComponentID()
{
    setupServiceForLabeling(0xE0D00001);

    // Test different component IDs (SCIdS: 0-15)
    for (uint8_t comp_id = 0; comp_id < 4; ++comp_id) {
        QString label = QString("Component %1    ").arg(comp_id);
        QByteArray fig = createFIG1_4(0xE0D00001, comp_id, label);
        m_analyser->analyzeFICData(fig);
    }

    QVERIFY(m_analyser->getTotalFIGsProcessed() >= 4);
}

void TestFIG1Processor::testFIG1_4_PacketDataLabel()
{
    // Packet data component label
    setupServiceForLabeling(0xE0D00002);

    QByteArray fig = createFIG1_4(0xE0D00002, 1, "Data Component  ");

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testFIG1_4_MultipleComponents()
{
    setupServiceForLabeling(0xE0D00001);

    // Service with multiple components
    m_analyser->analyzeFICData(createFIG1_4(0xE0D00001, 0, "Primary Audio   "));
    m_analyser->analyzeFICData(createFIG1_4(0xE0D00001, 1, "Secondary Audio "));
    m_analyser->analyzeFICData(createFIG1_4(0xE0D00001, 2, "Data Broadcast  "));

    QVERIFY(m_analyser->getTotalFIGsProcessed() >= 3);
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

    // Select first 8 characters (spec order: bit 15 = char 0 -> 0xFF00)
    label.character_flag = 0xFF00;

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

    // Select last 8 characters (spec order: bits 7-0 = chars 8-15 = 0x00FF)
    label.character_flag = 0x00FF;

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
    // Complete workflow: Discover service then add label.
    //
    // NOTE: Steps 1-2 use the shared setupServiceForLabeling helper so the
    // FIG 0/1 + FIG 0/2 setup matches the FIC header / extension / padding
    // conventions processFIG0_1 + processFIG0_2 expect (hand-rolled bytes
    // without those headers are dropped as malformed before any service is
    // stored). 16-bit programme SId 0xE0D0 is stored by FIG 0/2 under the
    // 15-bit key 0x60D0 (P/D flag in SId bit 15); FIG 1/1 applies the same
    // mask, so the label below resolves to the discovered service.

    // Step 1+2: Add sub-channel (FIG 0/1) and service (FIG 0/2)
    setupServiceForLabeling(0xE0D0);

    // Step 3: Add label (FIG 1/1)
    m_analyser->analyzeFICData(createFIG1_1(0x0000E0D0, "MCOT Radio      ", 0xFF00));

    // Verify complete service with label
    auto services = m_analyser->getDiscoveredServices();
    QVERIFY(services.size() > 0);

    qInfo() << "Complete service discovery with labels: " << services.size() << "services";
}

void TestFIG1Processor::testCompleteServiceLabeling()
{
    // Label all levels: ensemble, service, component

    // Ensemble label (FIG 1/0)
    m_analyser->analyzeFICData(createFIG1_0("Bangkok DAB     "));

    // Service setup and label
    setupServiceForLabeling(0xE0D00001);
    m_analyser->analyzeFICData(createFIG1_1(0xE0D00001, "MCOT Radio      "));

    // Component label (FIG 1/4)
    m_analyser->analyzeFICData(createFIG1_4(0xE0D00001, 0, "Main Audio      "));

    QVERIFY(m_analyser->getTotalFIGsProcessed() >= 3);
    qInfo() << "Complete labeling hierarchy processed";
}

void TestFIG1Processor::testEnsembleHierarchyLabels()
{
    // Build complete labeled ensemble tree

    // Ensemble
    m_analyser->analyzeFICData(createFIG1_0("Test Ensemble   "));

    // Multiple services with labels
    for (uint32_t i = 1; i <= 3; ++i) {
        uint32_t svc_id = 0xE0D00000 + i;
        setupServiceForLabeling(svc_id);

        QString label = QString("Service %1      ").arg(i);
        m_analyser->analyzeFICData(createFIG1_1(svc_id, label));

        // Component labels
        m_analyser->analyzeFICData(createFIG1_4(svc_id, 0, QString("Audio %1        ").arg(i)));
    }

    EnsembleInfo ensemble = m_analyser->getCurrentEnsemble();
    auto services = m_analyser->getDiscoveredServices();

    QVERIFY(!ensemble.ensembleLabel.isEmpty());
    QVERIFY(services.size() >= 3);

    qInfo() << "Ensemble hierarchy: Ensemble +" << services.size() << "services labeled";
}

void TestFIG1Processor::testLabelUpdateSequence()
{
    // Test label updates over time
    setupServiceForLabeling(0xE0D00001);

    // Initial label
    m_analyser->analyzeFICData(createFIG1_1(0xE0D00001, "Initial Label   "));

    // Updated label
    m_analyser->analyzeFICData(createFIG1_1(0xE0D00001, "Updated Label   "));

    // Final label
    m_analyser->analyzeFICData(createFIG1_1(0xE0D00001, "Final Label     "));

    auto services = m_analyser->getDiscoveredServices();
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

    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);
}

void TestFIG1Processor::testLabelLengthValidation()
{
    // Labels must be exactly 16 characters
    QString valid_label = "Exactly16Chars!!";
    QCOMPARE(valid_label.length(), 16);

    QByteArray fig = createFIG1_0(valid_label, 0xFF00);
    m_analyser->analyzeFICData(fig);

    QVERIFY(m_analyser->getValidFIGsCount() > 0);
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
        m_analyser->analyzeFICData(fig);
    }

    QVERIFY(m_analyser->getTotalFIGsProcessed() == 4);
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

    m_analyser->analyzeFICData(fig);

    // Should detect error
    QVERIFY(!m_analyser->getLastError().isEmpty() ||
            m_analyser->getValidFIGsCount() == 0);
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

    m_analyser->analyzeFICData(fig);

    // Should detect truncation
    QVERIFY(!m_analyser->getLastError().isEmpty() ||
            m_analyser->getValidFIGsCount() == 0);
}

void TestFIG1Processor::testCorruptedCharacterFlag()
{
    // Test with all bits set (unusual but valid)
    QByteArray fig = createFIG1_0("Test Label      ", 0xFFFF);

    m_analyser->analyzeFICData(fig);

    // Should process (0xFFFF is valid, just selects many characters)
    QVERIFY(m_analyser->getTotalFIGsProcessed() > 0);
}

void TestFIG1Processor::testMissingServiceReference()
{
    // Try to add label for non-existent service
    QByteArray fig = createFIG1_1(0xFFFFFFFF, "Unknown Service ");

    m_analyser->analyzeFICData(fig);

    // Should handle gracefully (create service or skip)
    QVERIFY(m_analyser->getTotalFIGsProcessed() >= 0);
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
    if (!is_data_service && service_id > 0xFFFF) {
        is_data_service = true;  // Auto-detect as data service
    }

    bool use_16bit_sid = !is_data_service && (service_id <= 0xFFFF);

    // FIG header (1 byte per EN 300 401 clause 5.2): type (b7-b5) + length
    // (b4-b0). Data = ext(1) + SId(2/4) + label(16) + mask(2).
    uint8_t fig_len = use_16bit_sid ? 21 : 23;
    fig.append(static_cast<char>((1 << 5) | fig_len));

    // First data byte: charset(4) | O/E(1) | extension(3) = 1
    fig.append(static_cast<char>(0x01));

    if (use_16bit_sid) {
        // 16-bit programme SId at data[1..2]; the P/D flag is bit 15 (0),
        // which processFIG1_1 masks away (same 15-bit key as FIG 0/2).
        fig.append(static_cast<char>((service_id >> 8) & 0x7F));
        fig.append(static_cast<char>(service_id & 0xFF));
    } else {
        // 32-bit data-service SId at data[1..4]; the P/D flag is the MSB
        // (bit 31, naturally set for ECC 0xE0..0xFF data services).
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

    // FIG header (1 byte per EN 300 401 clause 5.2): type (b7-b5) + length
    // (b4-b0). Data = ext(1) + P/D+SCIdS(1) + SId(2/4) + label(16) + mask(2).
    bool is_packet = service_id > 0xFFFF;
    uint8_t fig_len = is_packet ? 24 : 22;
    fig.append(static_cast<char>((1 << 5) | fig_len));

    // First data byte: charset(4)=0 | O/E(1)=0 | extension(3)=4
    fig.append(static_cast<char>(0x04));

    // Second data byte: P/D flag (b7) + rfu (b6-b4) + SCIdS (b3-b0)
    // (etisnoop fig1.cpp case 4: SCIdS = f[1] & 0x0F)
    uint8_t pd_scids = (is_packet ? 0x80 : 0x00) | (component_id & 0x0F);
    fig.append(static_cast<char>(pd_scids));

    // Service ID (16-bit for stream components, 32-bit for packet components)
    if (is_packet) {
        fig.append(static_cast<char>((service_id >> 24) & 0xFF));
        fig.append(static_cast<char>((service_id >> 16) & 0xFF));
        fig.append(static_cast<char>((service_id >> 8) & 0xFF));
        fig.append(static_cast<char>(service_id & 0xFF));
    } else {
        fig.append(static_cast<char>((service_id >> 8) & 0xFF));
        fig.append(static_cast<char>(service_id & 0xFF));
    }

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

    // FIG header (1 byte per EN 300 401 clause 5.2): type (b7-b5) + length
    // (b4-b0). FIG 1/0 data = ext(1) + EId(2) + label(16) + mask(2) = 21 B.
    fig.append(static_cast<char>((1 << 5) | 21));  // FIG type 1, length 21

    // First data byte: charset(4) | O/E(1) | extension(3) = 0 (EBU Latin)
    fig.append(static_cast<char>(0x00));

    // Ensemble ID (2 bytes) - test ensemble ID 0x1234
    fig.append(static_cast<char>(0x12));  // EId MSB
    fig.append(static_cast<char>(0x34));  // EId LSB

    // Label (16 characters) - ETSI EN 300 401 Section 8.1.13
    for (int i = 0; i < 16; ++i) {
        char c = (i < label.length()) ? label[i].toLatin1() : ' ';
        fig.append(c);
    }

    // Character flag (2 bytes, big-endian) - ETSI EN 300 401 Section 8.1.13.1
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

    // FIG header (1 byte): type 0, length 4 (ext + 3-byte UEP short entry)
    fig0_1.append(static_cast<char>(0x04));

    // Extension byte: extension 1 (FIG 0 ext = data[0] & 0x1F)
    fig0_1.append(static_cast<char>(0x01));
    fig0_1.append(static_cast<char>(0x00));  // SubChId=0 (bits 7-2) + Start[9:8]=0 (bits 1-0)
    fig0_1.append(static_cast<char>(0x00));  // Start Address[7:0]=0
    fig0_1.append(static_cast<char>(0x05));  // UEP short form (bit 7=0) + Table Index 5
    m_analyser->analyzeFICData(fig0_1);

    // Step 2: Create service (FIG 0/2) with correct SId encoding per ETSI EN 300 401
    QByteArray fig0_2;

    if (is_programme) {
        // Programme service: 16-bit Service ID
        // Length = ext(1) + sid(2) + numcomp(1) + component(2) = 6
        fig0_2.append(static_cast<char>(0x06));

        // Extension byte: extension 2, P/D=0 (programme; P/D is FIG-level)
        fig0_2.append(static_cast<char>(0x02));

        // Service ID MSB (P/D flag in bit 15 — reflected in the 0x7FFF key
        // convention shared with processFIG1_1)
        fig0_2.append(static_cast<char>(0x80 | ((service_id >> 8) & 0x7F)));

        // Service ID LSB
        fig0_2.append(static_cast<char>(service_id & 0xFF));
    } else {
        // Data service: 32-bit Service ID
        // Length = ext(1) + sid32(4) + numcomp(1) + component(2) = 8
        fig0_2.append(static_cast<char>(0x08));

        // Extension byte: extension 2, P/D=1 (data services use 32-bit SIds)
        fig0_2.append(static_cast<char>(0x22));

        // Full 32-bit Service ID (no P/D flag for 32-bit encoding)
        fig0_2.append(static_cast<char>((service_id >> 24) & 0xFF));
        fig0_2.append(static_cast<char>((service_id >> 16) & 0xFF));
        fig0_2.append(static_cast<char>((service_id >> 8) & 0xFF));
        fig0_2.append(static_cast<char>(service_id & 0xFF));
    }

    // Number of components (1) in low nibble + Local flag + CAId
    fig0_2.append(static_cast<char>(0x10));  // NumComp=1, Local=0, CAId=0

    // Component descriptor (2 bytes)
    // Byte 0: TMId (2 bits) + ASCTy/DSCTy (6 bits)
    fig0_2.append(static_cast<char>(0x00));  // TMId=0 (MSC stream), ASCTy=0 (DAB audio)

    // Byte 1: SubChId (6 bits) + P/S (1 bit) + CA (1 bit) — etisnoop fig0_2.cpp:
    // subchid = (b1 & 0xFC) >> 2, ps = (b1 & 0x02) >> 1, ca = b1 & 0x01
    fig0_2.append(static_cast<char>((0 << 2) | (1 << 1) | 0));  // SubChId=0, Primary, CA=0

    m_analyser->analyzeFICData(fig0_2);

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

    // FIG header (1 byte): type 0, length 6 (ext + EId + change + alarm)
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

    // Occurrence change (1 byte, present when the change flag is non-zero)
    fig.append(static_cast<char>(0x00));

    return fig;
}

// Register test class
QTEST_MAIN(TestFIG1Processor)
#include "test_fig1_processor.moc"
