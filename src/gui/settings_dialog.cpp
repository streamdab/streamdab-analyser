/**
 * @file settings_dialog.cpp
 * @brief Professional settings configuration dialog implementation
 * 
 * This file implements the SettingsDialog class providing comprehensive
 * configuration options for the ETI Stream Analyzer application following
 * professional broadcast industry standards with manual widget creation.
 */

#include "settings_dialog.h"
#include "utils/logger.h"

#include <QApplication>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QScreen>
#include <QDebug>
#include <QFormLayout>
#include <QIcon>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_buttonBox(nullptr)
    , m_backgroundColor(20, 20, 30)
    , m_gridColor(60, 60, 80)
    , m_textColor(200, 200, 220)
    , m_displayFont("Arial", 9)
    , m_settingsLoaded(false)
    , m_settingsChanged(false)
    , m_settings(nullptr)
{
    // Create UI manually
    createUI();
    
    // Initialize settings
    m_settings = new QSettings(this);
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Constructor completed");
}

SettingsDialog::~SettingsDialog()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Destructor starting");
    
    // Qt handles widget cleanup automatically
    
    Logger::instance().log(Logger::Info, "SettingsDialog", "Destructor completed");
}

void SettingsDialog::createUI()
{
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    // Create tabs
    createGeneralTab();
    createAnalysisTab();
    createDisplayTab();
    createAudioTab();
    createNetworkTab();
    createPerformanceTab();
    createAdvancedTab();
    createAnalyzerTab();
    
    // Create button box
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | 
        QDialogButtonBox::Apply | QDialogButtonBox::RestoreDefaults,
        this
    );
    
    // Add to main layout
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(m_buttonBox);
    
    // Connect signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsDialog::handleApply);
    connect(m_buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &SettingsDialog::handleDefaults);
    
    // Set dialog properties
    setObjectName("SettingsDialog");
    setWindowTitle(tr("StreamDAB Stream Analyzer - Settings"));
    setWindowIcon(QIcon(":/icons/settings.png"));
    setModal(true);
    setMinimumSize(600, 450);
    resize(700, 550);
    
    // Center dialog on parent
    if (parentWidget()) {
        QRect parentGeometry = parentWidget()->geometry();
        int x = parentGeometry.x() + (parentGeometry.width() - width()) / 2;
        int y = parentGeometry.y() + (parentGeometry.height() - height()) / 2;
        move(x, y);
    }
}

void SettingsDialog::createGeneralTab()
{
    m_generalTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_generalTab);
    
    // Appearance group
    m_appearanceGroup = new QGroupBox(tr("Appearance"), m_generalTab);
    QFormLayout *appearanceLayout = new QFormLayout(m_appearanceGroup);
    
    m_themeCombo = new QComboBox();
    m_themeCombo->addItems({tr("Dark"), tr("Light"), tr("System")});
    appearanceLayout->addRow(tr("Theme:"), m_themeCombo);
    
    // Behavior group
    m_behaviorGroup = new QGroupBox(tr("Behavior"), m_generalTab);
    QFormLayout *behaviorLayout = new QFormLayout(m_behaviorGroup);
    
    m_autoSaveCheck = new QCheckBox(tr("Enable auto-save"));
    m_confirmExitCheck = new QCheckBox(tr("Confirm on exit"));
    m_autoSaveIntervalSpin = new QSpinBox();
    m_autoSaveIntervalSpin->setRange(30, 600);
    m_autoSaveIntervalSpin->setSuffix(tr(" seconds"));
    m_autoSaveIntervalSpin->setValue(60);
    
    behaviorLayout->addRow(m_autoSaveCheck);
    behaviorLayout->addRow(m_confirmExitCheck);
    behaviorLayout->addRow(tr("Auto-save interval:"), m_autoSaveIntervalSpin);
    
    layout->addWidget(m_appearanceGroup);
    layout->addWidget(m_behaviorGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_generalTab, tr("General"));
}

void SettingsDialog::createAnalysisTab()
{
    m_analysisTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_analysisTab);
    
    // Performance group
    m_performanceGroup = new QGroupBox(tr("Performance"), m_analysisTab);
    QFormLayout *performanceLayout = new QFormLayout(m_performanceGroup);
    
    m_updateRateSpin = new QSpinBox();
    m_updateRateSpin->setRange(MIN_UPDATE_RATE, MAX_UPDATE_RATE);
    m_updateRateSpin->setValue(DEFAULT_UPDATE_RATE);
    m_updateRateSpin->setSuffix(tr(" FPS"));
    
    m_bufferSizeSpin = new QSpinBox();
    m_bufferSizeSpin->setRange(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
    m_bufferSizeSpin->setValue(DEFAULT_BUFFER_SIZE);
    m_bufferSizeSpin->setSuffix(tr(" frames"));
    
    m_realTimeCheck = new QCheckBox(tr("Enable real-time processing"));
    m_hardwareAccelCheck = new QCheckBox(tr("Enable hardware acceleration"));
    
    performanceLayout->addRow(tr("Update rate:"), m_updateRateSpin);
    performanceLayout->addRow(tr("Buffer size:"), m_bufferSizeSpin);
    performanceLayout->addRow(m_realTimeCheck);
    performanceLayout->addRow(m_hardwareAccelCheck);
    
    layout->addWidget(m_performanceGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_analysisTab, tr("Analysis"));
}

void SettingsDialog::createDisplayTab()
{
    m_displayTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_displayTab);
    
    // Colors group
    m_colorsGroup = new QGroupBox(tr("Colors"), m_displayTab);
    QFormLayout *colorsLayout = new QFormLayout(m_colorsGroup);
    
    m_backgroundColorButton = new QPushButton(tr("Background Color"));
    m_gridColorButton = new QPushButton(tr("Grid Color"));
    m_textColorButton = new QPushButton(tr("Text Color"));
    
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
    connect(m_gridColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
    connect(m_textColorButton, &QPushButton::clicked, this, &SettingsDialog::handleColorSelection);
    
    colorsLayout->addRow(tr("Background:"), m_backgroundColorButton);
    colorsLayout->addRow(tr("Grid:"), m_gridColorButton);
    colorsLayout->addRow(tr("Text:"), m_textColorButton);
    
    // Fonts group
    m_fontsGroup = new QGroupBox(tr("Fonts"), m_displayTab);
    QFormLayout *fontsLayout = new QFormLayout(m_fontsGroup);
    
    m_fontButton = new QPushButton(tr("Select Font"));
    connect(m_fontButton, &QPushButton::clicked, this, &SettingsDialog::handleFontSelection);
    
    fontsLayout->addRow(tr("Display font:"), m_fontButton);
    
    layout->addWidget(m_colorsGroup);
    layout->addWidget(m_fontsGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_displayTab, tr("Display"));
}

void SettingsDialog::createAudioTab()
{
    m_audioTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_audioTab);
    
    // Audio device group
    m_audioDeviceGroup = new QGroupBox(tr("Audio Device"), m_audioTab);
    QFormLayout *deviceLayout = new QFormLayout(m_audioDeviceGroup);
    
    m_audioDeviceCombo = new QComboBox();
    m_audioDeviceCombo->addItems({tr("Default"), tr("System Audio"), tr("ASIO Driver")});
    
    m_audioMonitorCheck = new QCheckBox(tr("Enable audio monitoring"));
    
    deviceLayout->addRow(tr("Output device:"), m_audioDeviceCombo);
    deviceLayout->addRow(m_audioMonitorCheck);
    
    // Level group
    m_levelGroup = new QGroupBox(tr("Audio Levels"), m_audioTab);
    QFormLayout *levelLayout = new QFormLayout(m_levelGroup);
    
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    
    levelLayout->addRow(tr("Volume:"), m_volumeSlider);
    
    layout->addWidget(m_audioDeviceGroup);
    layout->addWidget(m_levelGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_audioTab, tr("Audio"));
}

void SettingsDialog::createNetworkTab()
{
    m_networkTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_networkTab);
    
    // Connection group
    m_connectionGroup = new QGroupBox(tr("Network Connection"), m_networkTab);
    QFormLayout *connectionLayout = new QFormLayout(m_connectionGroup);
    
    m_useProxyCheck = new QCheckBox(tr("Use proxy server"));
    m_proxyHostEdit = new QLineEdit();
    m_proxyPortSpin = new QSpinBox();
    m_proxyPortSpin->setRange(1, 65535);
    m_proxyPortSpin->setValue(8080);
    
    connectionLayout->addRow(m_useProxyCheck);
    connectionLayout->addRow(tr("Proxy host:"), m_proxyHostEdit);
    connectionLayout->addRow(tr("Proxy port:"), m_proxyPortSpin);
    
    layout->addWidget(m_connectionGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_networkTab, tr("Network"));
}

void SettingsDialog::createPerformanceTab()
{
    m_performanceTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_performanceTab);
    
    // Memory group
    m_memoryGroup = new QGroupBox(tr("Memory Management"), m_performanceTab);
    QFormLayout *memoryLayout = new QFormLayout(m_memoryGroup);
    
    m_memoryLimitSpin = new QSpinBox();
    m_memoryLimitSpin->setRange(MIN_MEMORY_LIMIT, MAX_MEMORY_LIMIT);
    m_memoryLimitSpin->setValue(DEFAULT_MEMORY_LIMIT);
    m_memoryLimitSpin->setSuffix(tr(" MB"));
    
    m_priorityCombo = new QComboBox();
    m_priorityCombo->addItems({tr("Low"), tr("Normal"), tr("High"), tr("Real-time")});
    m_priorityCombo->setCurrentIndex(1); // Normal
    
    memoryLayout->addRow(tr("Memory limit:"), m_memoryLimitSpin);
    memoryLayout->addRow(tr("Process priority:"), m_priorityCombo);
    
    layout->addWidget(m_memoryGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_performanceTab, tr("Performance"));
}

void SettingsDialog::createAdvancedTab()
{
    m_advancedTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_advancedTab);
    
    // Debug group
    m_debugGroup = new QGroupBox(tr("Advanced Options"), m_advancedTab);
    QFormLayout *debugLayout = new QFormLayout(m_debugGroup);
    
    m_debugModeCheck = new QCheckBox(tr("Enable debug mode"));
    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->addItems({tr("Error"), tr("Warning"), tr("Info"), tr("Debug")});
    m_logLevelCombo->setCurrentIndex(2); // Info
    
    debugLayout->addRow(m_debugModeCheck);
    debugLayout->addRow(tr("Log level:"), m_logLevelCombo);
    
    layout->addWidget(m_debugGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_advancedTab, tr("Advanced"));
}

void SettingsDialog::createAnalyzerTab()
{
    // "Analyzer / Decode options" — implements the SETTING rows of
    // docs/fixes/OPTION_VARIANT_MATRIX.md (rows 1-6, 8, 10-12, 14).
    // Each default equals the etisnoop-parity behavior, so a default-config
    // run is byte-identical to the baseline. Changes are persisted under
    // group "analyzer/" and honored on the next file load (CLI immediately;
    // GUI decode re-reads on load).
    m_analyzerTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_analyzerTab);

    auto addGroup = [layout](const QString& title, QWidget* parent) -> QGroupBox* {
        QGroupBox *group = new QGroupBox(title, parent);
        group->setLayout(new QFormLayout(group));
        layout->addWidget(group);
        return group;
    };

    // --- Row A: Label charset ---
    QGroupBox *charsetGroup = addGroup(tr("A. Label charset (matrix row 1)"), m_analyzerTab);
    QFormLayout *charsetLayout = qobject_cast<QFormLayout*>(charsetGroup->layout());

    m_forceCharsetSpin = new QSpinBox();
    m_forceCharsetSpin->setObjectName(QStringLiteral("settingsForceCharset"));
    m_forceCharsetSpin->setRange(-1, 6);
    m_forceCharsetSpin->setSpecialValueText(tr("-1 = Follow stream (default)"));
    m_forceCharsetSpin->setToolTip(tr(
        "Override the FIG 1 label charset flag: 0 = EBU Latin, 3 = UTF-8, "
        "6 = Thai TIS-620. Default (-1) follows the stream's charset flag."));
    charsetLayout->addRow(tr("Force charset:"), m_forceCharsetSpin);

    m_charsetFallbackCombo = new QComboBox();
    m_charsetFallbackCombo->addItem(tr("raw — pass bytes through (default)"),
                                    QStringLiteral("raw"));
    m_charsetFallbackCombo->addItem(tr("utf8 — treat as UTF-8"),
                                    QStringLiteral("utf8"));
    m_charsetFallbackCombo->addItem(tr("skip — drop the label"),
                                    QStringLiteral("skip"));
    m_charsetFallbackCombo->setToolTip(tr(
        "Policy for unknown charset flags (raw bytes + warn once is the "
        "default etisnoop-parity behavior)."));
    charsetLayout->addRow(tr("Unknown-charset fallback:"), m_charsetFallbackCombo);

    // --- Row B: ETI format ---
    QGroupBox *etiGroup = addGroup(tr("B. ETI format (matrix row 2)"), m_analyzerTab);
    QFormLayout *etiLayout = qobject_cast<QFormLayout*>(etiGroup->layout());

    m_etiModeCombo = new QComboBox();
    m_etiModeCombo->addItem(tr("auto — sync-based detection (default)"), QStringLiteral("auto"));
    m_etiModeCombo->addItem(tr("li — force ETI-LI"), QStringLiteral("li"));
    m_etiModeCombo->addItem(tr("ni — force ETI-NI"), QStringLiteral("ni"));
    m_etiModeCombo->setToolTip(tr(
        "Force the ETI transport branch; auto (default) detects by sync pattern."));
    etiLayout->addRow(tr("ETI mode:"), m_etiModeCombo);

    // --- Row C: NST legacy correction ---
    QGroupBox *nstGroup = addGroup(tr("C. NST interpretation (matrix row 3)"), m_analyzerTab);
    QFormLayout *nstLayout = qobject_cast<QFormLayout*>(nstGroup->layout());

    m_nstOffset1Check = new QCheckBox(tr("Legacy count-1 correction: NST = (p[5]&0x7F) + 1"));
    m_nstOffset1Check->setToolTip(tr(
        "Some legacy muxes wrote the stream count minus one in the NST field; "
        "enable to decode one additional STC entry and shift the FIC offset."));
    nstLayout->addRow(m_nstOffset1Check);

    // --- Row D/E: FIC framework + FIB CRC ---
    QGroupBox *ficGroup = addGroup(tr("D. FIC framework / FIB CRC (matrix rows 4-5)"), m_analyzerTab);
    QFormLayout *ficLayout = qobject_cast<QFormLayout*>(ficGroup->layout());

    m_ficModeCombo = new QComboBox();
    m_ficModeCombo->addItem(tr("auto — strict + raw fallback (default)"), QStringLiteral("auto"));
    m_ficModeCombo->addItem(tr("strict — validate every FIB CRC, drop bad FIBs"), QStringLiteral("strict"));
    m_ficModeCombo->addItem(tr("raw — whole FIC as one FIG stream (no per-FIB CRC)"), QStringLiteral("raw"));
    m_ficModeCombo->setToolTip(tr(
        "auto = strict FIB-CRC validation; when every FIB CRC fails the FIC is "
        "re-walked as a raw FIG stream (the DABX_FIC_MODE env hook is honored "
        "in auto mode)."));
    ficLayout->addRow(tr("FIC decode mode:"), m_ficModeCombo);

    m_fibIgnoreCrcCheck = new QCheckBox(tr("Ignore FIB CRC failures (decode bad FIBs anyway)"));
    m_fibIgnoreCrcCheck->setToolTip(tr(
        "When enabled, FIBs whose CRC fails are still decoded instead of being "
        "dropped; failures are still counted for diagnostics."));
    ficLayout->addRow(m_fibIgnoreCrcCheck);

    // --- Row E: ECC / country override + deep parse ---
    QGroupBox *eccGroup = addGroup(tr("E. ECC / country (matrix rows 6, 14)"), m_analyzerTab);
    QFormLayout *eccLayout = qobject_cast<QFormLayout*>(eccGroup->layout());

    m_eccOverrideSpin = new QSpinBox();
    m_eccOverrideSpin->setRange(-1, 255);
    m_eccOverrideSpin->setDisplayIntegerBase(16);
    m_eccOverrideSpin->setPrefix("0x");
    m_eccOverrideSpin->setSpecialValueText(tr("-1 = Auto (from stream, default)"));
    m_eccOverrideSpin->setToolTip(tr(
        "Manual Extended Country Code used for country resolution (TS 101 756); "
        "-1 (default) uses the value from the stream."));
    eccLayout->addRow(tr("ECC override:"), m_eccOverrideSpin);

    m_serviceEccDeepParseCheck = new QCheckBox(tr("Deep-parse FIG 0/9 extended per-service ECC"));
    m_serviceEccDeepParseCheck->setToolTip(tr(
        "Parse the extended sub-field of FIG 0/9 for per-service ECC/LTO "
        "precision instead of inheriting the ensemble ECC."));
    eccLayout->addRow(m_serviceEccDeepParseCheck);

    // --- Row F: label source + short label ---
    QGroupBox *labelGroup = addGroup(tr("F. Label display (matrix row 8)"), m_analyzerTab);
    QFormLayout *labelLayout = qobject_cast<QFormLayout*>(labelGroup->layout());

    m_labelSourceCombo = new QComboBox();
    m_labelSourceCombo->addItem(tr("fig016 — FIG 0/16 first (default)"), QStringLiteral("fig016"));
    m_labelSourceCombo->addItem(tr("fig11 — FIG 1/1 first"), QStringLiteral("fig11"));
    m_labelSourceCombo->setToolTip(tr(
        "Source priority for conflicting names between FIG 0/16 service-org "
        "labels and FIG 1/1 label-only services."));
    labelLayout->addRow(tr("Label source priority:"), m_labelSourceCombo);

    m_showShortLabelsCheck = new QCheckBox(tr("Emit short labels (16-char label + 16-bit mask)"));
    m_showShortLabelsCheck->setToolTip(tr(
        "Derive the short label from the 16-char label and the 16-bit character "
        "mask (EN 300 401 8.1.14, etisnoop shortlabel()) and emit "
        "service_short_label / ensemble_short_label in the YAML output."));
    labelLayout->addRow(m_showShortLabelsCheck);

    // --- Row G: timestamp source ---
    QGroupBox *tsGroup = addGroup(tr("G. Timestamp source (matrix row 10)"), m_analyzerTab);
    QFormLayout *tsLayout = qobject_cast<QFormLayout*>(tsGroup->layout());

    m_timestampSourceCombo = new QComboBox();
    m_timestampSourceCombo->addItem(tr("tist — frame TIST (default)"), QStringLiteral("tist"));
    m_timestampSourceCombo->addItem(tr("mtime — file mtime fallback"), QStringLiteral("mtime"));
    m_timestampSourceCombo->setToolTip(tr(
        "Per-frame timestamp source for verbose output / the yaml tist_ms line."));
    tsLayout->addRow(tr("Timestamp source:"), m_timestampSourceCombo);

    // --- Row H: strict frame CRC ---
    QGroupBox *crcGroup = addGroup(tr("H. EOH/EOF frame CRC (matrix row 11)"), m_analyzerTab);
    QFormLayout *crcLayout = qobject_cast<QFormLayout*>(crcGroup->layout());

    m_strictFrameCrcCheck = new QCheckBox(tr("Validate + count the EOH gap frame CRC (CCITT-FALSE)"));
    m_strictFrameCrcCheck->setToolTip(tr(
        "Validate the EOH gap CRC (bytes 8+4*NST .. 11+4*NST) and report "
        "mismatch counters; decode stays independent (never blocks parsing)."));
    crcLayout->addRow(m_strictFrameCrcCheck);

    // --- Row I: SId display ---
    QGroupBox *sidGroup = addGroup(tr("I. SId display (matrix row 12)"), m_analyzerTab);
    QFormLayout *sidLayout = qobject_cast<QFormLayout*>(sidGroup->layout());

    m_sidDisplayCombo = new QComboBox();
    m_sidDisplayCombo->addItem(tr("hex32 — full 32-bit hex (default)"), QStringLiteral("hex32"));
    m_sidDisplayCombo->addItem(tr("ecc-sid — ECC:CId:SRef (e.g. F3:2:00001)"), QStringLiteral("ecc-sid"));
    m_sidDisplayCombo->addItem(tr("hex16 — lower 16 bits"), QStringLiteral("hex16"));
    m_sidDisplayCombo->setToolTip(tr(
        "service_id rendering in the YAML output for data (32-bit) services."));
    sidLayout->addRow(tr("SId display:"), m_sidDisplayCombo);

    layout->addStretch();
    m_tabWidget->addTab(m_analyzerTab, tr("Analyzer"));
}

bool SettingsDialog::initialize()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Initialization starting");
    
    try {
        // Initialize combo boxes
        initializeComboBoxes();
        
        // Update UI with current settings
        updateUI();
        
        // Mark as loaded
        m_settingsLoaded = true;
        
        Logger::instance().log(Logger::Info, "SettingsDialog", "Initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "SettingsDialog", 
                             QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void SettingsDialog::loadSettings()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Loading settings from configuration");
    
    if (!m_settings) {
        Logger::instance().log(Logger::Warning, "SettingsDialog", "Settings object not initialized");
        return;
    }
    
    try {
        // Load general settings
        m_themeCombo->setCurrentIndex(m_settings->value("general/theme", 0).toInt());
        m_autoSaveCheck->setChecked(m_settings->value("general/autoSave", true).toBool());
        m_confirmExitCheck->setChecked(m_settings->value("general/confirmExit", true).toBool());
        m_autoSaveIntervalSpin->setValue(m_settings->value("general/autoSaveInterval", 60).toInt());
        
        // Load analysis settings
        m_updateRateSpin->setValue(m_settings->value("analysis/updateRate", DEFAULT_UPDATE_RATE).toInt());
        m_bufferSizeSpin->setValue(m_settings->value("analysis/bufferSize", DEFAULT_BUFFER_SIZE).toInt());
        m_realTimeCheck->setChecked(m_settings->value("analysis/realTime", false).toBool());
        m_hardwareAccelCheck->setChecked(m_settings->value("analysis/hardwareAccel", false).toBool());
        
        // Load display settings
        m_backgroundColor = m_settings->value("display/backgroundColor", QColor(20, 20, 30)).value<QColor>();
        m_gridColor = m_settings->value("display/gridColor", QColor(60, 60, 80)).value<QColor>();
        m_textColor = m_settings->value("display/textColor", QColor(200, 200, 220)).value<QColor>();
        m_displayFont = m_settings->value("display/font", QFont("Arial", 9)).value<QFont>();
        
        // Load performance settings
        m_memoryLimitSpin->setValue(m_settings->value("performance/memoryLimit", DEFAULT_MEMORY_LIMIT).toInt());
        m_priorityCombo->setCurrentIndex(m_settings->value("performance/priority", 1).toInt());
        
        // Load advanced settings
        m_debugModeCheck->setChecked(m_settings->value("advanced/debugMode", false).toBool());
        m_logLevelCombo->setCurrentIndex(m_settings->value("advanced/logLevel", 2).toInt());

        // Load analyzer / decode settings (option-variant matrix rows 1-14)
        {
            eti::AnalyzerSettings analyzer = eti::AnalyzerSettings::defaults();
            analyzer.loadFromQSettings(*m_settings);
            m_forceCharsetSpin->setValue(analyzer.force_charset);
            m_charsetFallbackCombo->setCurrentIndex(
                m_charsetFallbackCombo->findData(
                    eti::AnalyzerSettings::charsetFallbackToString(analyzer.charset_fallback_policy)));
            m_etiModeCombo->setCurrentIndex(
                m_etiModeCombo->findData(eti::AnalyzerSettings::etiModeToString(analyzer.eti_mode)));
            m_nstOffset1Check->setChecked(analyzer.nst_offset_1);
            m_ficModeCombo->setCurrentIndex(
                m_ficModeCombo->findData(eti::AnalyzerSettings::ficModeToString(analyzer.fic_mode)));
            m_fibIgnoreCrcCheck->setChecked(analyzer.fib_ignore_crc);
            m_eccOverrideSpin->setValue(analyzer.ecc_override);
            m_serviceEccDeepParseCheck->setChecked(analyzer.service_ecc_deep_parse);
            m_labelSourceCombo->setCurrentIndex(
                m_labelSourceCombo->findData(
                    eti::AnalyzerSettings::labelSourceToString(analyzer.label_source_priority)));
            m_showShortLabelsCheck->setChecked(analyzer.show_short_labels);
            m_timestampSourceCombo->setCurrentIndex(
                m_timestampSourceCombo->findData(
                    eti::AnalyzerSettings::timestampSourceToString(analyzer.timestamp_source)));
            m_strictFrameCrcCheck->setChecked(analyzer.strict_frame_crc);
            m_sidDisplayCombo->setCurrentIndex(
                m_sidDisplayCombo->findData(
                    eti::AnalyzerSettings::sidDisplayModeToString(analyzer.sid_display_mode)));
        }
        
        // Update UI
        updateColorButtons();
        m_fontButton->setText(QString("%1, %2pt").arg(m_displayFont.family()).arg(m_displayFont.pointSize()));
        
        Logger::instance().log(Logger::Debug, "SettingsDialog", "Settings loaded successfully");
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "SettingsDialog", 
                             QString("Failed to load settings: %1").arg(e.what()));
    }
}

void SettingsDialog::saveSettings()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Saving settings to configuration");
    
    if (!m_settings) {
        Logger::instance().log(Logger::Warning, "SettingsDialog", "Settings object not initialized");
        return;
    }
    
    try {
        // Save general settings
        m_settings->setValue("general/theme", m_themeCombo->currentIndex());
        m_settings->setValue("general/autoSave", m_autoSaveCheck->isChecked());
        m_settings->setValue("general/confirmExit", m_confirmExitCheck->isChecked());
        m_settings->setValue("general/autoSaveInterval", m_autoSaveIntervalSpin->value());
        
        // Save analysis settings
        m_settings->setValue("analysis/updateRate", m_updateRateSpin->value());
        m_settings->setValue("analysis/bufferSize", m_bufferSizeSpin->value());
        m_settings->setValue("analysis/realTime", m_realTimeCheck->isChecked());
        m_settings->setValue("analysis/hardwareAccel", m_hardwareAccelCheck->isChecked());
        
        // Save display settings
        m_settings->setValue("display/backgroundColor", m_backgroundColor);
        m_settings->setValue("display/gridColor", m_gridColor);
        m_settings->setValue("display/textColor", m_textColor);
        m_settings->setValue("display/font", m_displayFont);
        
        // Save performance settings
        m_settings->setValue("performance/memoryLimit", m_memoryLimitSpin->value());
        m_settings->setValue("performance/priority", m_priorityCombo->currentIndex());
        
        // Save advanced settings
        m_settings->setValue("advanced/debugMode", m_debugModeCheck->isChecked());
        m_settings->setValue("advanced/logLevel", m_logLevelCombo->currentIndex());

        // Save analyzer / decode settings (option-variant matrix rows 1-14)
        {
            eti::AnalyzerSettings analyzer = eti::AnalyzerSettings::defaults();
            analyzer.force_charset = m_forceCharsetSpin->value();
            {
                eti::CharsetFallbackPolicy p;
                if (eti::AnalyzerSettings::charsetFallbackFromString(
                        m_charsetFallbackCombo->currentData().toString(), p)) {
                    analyzer.charset_fallback_policy = p;
                }
            }
            {
                eti::EtiModeSetting m;
                if (eti::AnalyzerSettings::etiModeFromString(
                        m_etiModeCombo->currentData().toString(), m)) {
                    analyzer.eti_mode = m;
                }
            }
            analyzer.nst_offset_1 = m_nstOffset1Check->isChecked();
            {
                eti::FicModeSetting m;
                if (eti::AnalyzerSettings::ficModeFromString(
                        m_ficModeCombo->currentData().toString(), m)) {
                    analyzer.fic_mode = m;
                }
            }
            analyzer.fib_ignore_crc = m_fibIgnoreCrcCheck->isChecked();
            analyzer.ecc_override = m_eccOverrideSpin->value();
            analyzer.service_ecc_deep_parse = m_serviceEccDeepParseCheck->isChecked();
            {
                eti::LabelSourcePriority p;
                if (eti::AnalyzerSettings::labelSourceFromString(
                        m_labelSourceCombo->currentData().toString(), p)) {
                    analyzer.label_source_priority = p;
                }
            }
            analyzer.show_short_labels = m_showShortLabelsCheck->isChecked();
            {
                eti::TimestampSource t;
                if (eti::AnalyzerSettings::timestampSourceFromString(
                        m_timestampSourceCombo->currentData().toString(), t)) {
                    analyzer.timestamp_source = t;
                }
            }
            analyzer.strict_frame_crc = m_strictFrameCrcCheck->isChecked();
            {
                eti::SidDisplayMode m;
                if (eti::AnalyzerSettings::sidDisplayModeFromString(
                        m_sidDisplayCombo->currentData().toString(), m)) {
                    analyzer.sid_display_mode = m;
                }
            }
            analyzer.saveToQSettings(*m_settings);
        }
        
        m_settings->sync();
        m_settingsChanged = false;
        
        Logger::instance().log(Logger::Debug, "SettingsDialog", "Settings saved successfully");
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "SettingsDialog", 
                             QString("Failed to save settings: %1").arg(e.what()));
    }
}

void SettingsDialog::resetToDefaults()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Resetting settings to defaults");
    
    // Reset to default values
    m_themeCombo->setCurrentIndex(0);
    m_autoSaveCheck->setChecked(true);
    m_confirmExitCheck->setChecked(true);
    m_autoSaveIntervalSpin->setValue(60);
    
    m_updateRateSpin->setValue(DEFAULT_UPDATE_RATE);
    m_bufferSizeSpin->setValue(DEFAULT_BUFFER_SIZE);
    m_realTimeCheck->setChecked(false);
    m_hardwareAccelCheck->setChecked(false);
    
    m_backgroundColor = QColor(20, 20, 30);
    m_gridColor = QColor(60, 60, 80);
    m_textColor = QColor(200, 200, 220);
    m_displayFont = QFont("Arial", 9);
    
    m_memoryLimitSpin->setValue(DEFAULT_MEMORY_LIMIT);
    m_priorityCombo->setCurrentIndex(1);
    
    m_debugModeCheck->setChecked(false);
    m_logLevelCombo->setCurrentIndex(2);

    // Reset analyzer / decode options to the etisnoop-parity defaults.
    {
        const eti::AnalyzerSettings analyzer = eti::AnalyzerSettings::defaults();
        m_forceCharsetSpin->setValue(analyzer.force_charset);
        m_charsetFallbackCombo->setCurrentIndex(
            m_charsetFallbackCombo->findData(
                eti::AnalyzerSettings::charsetFallbackToString(analyzer.charset_fallback_policy)));
        m_etiModeCombo->setCurrentIndex(
            m_etiModeCombo->findData(eti::AnalyzerSettings::etiModeToString(analyzer.eti_mode)));
        m_nstOffset1Check->setChecked(analyzer.nst_offset_1);
        m_ficModeCombo->setCurrentIndex(
            m_ficModeCombo->findData(eti::AnalyzerSettings::ficModeToString(analyzer.fic_mode)));
        m_fibIgnoreCrcCheck->setChecked(analyzer.fib_ignore_crc);
        m_eccOverrideSpin->setValue(analyzer.ecc_override);
        m_serviceEccDeepParseCheck->setChecked(analyzer.service_ecc_deep_parse);
        m_labelSourceCombo->setCurrentIndex(
            m_labelSourceCombo->findData(
                eti::AnalyzerSettings::labelSourceToString(analyzer.label_source_priority)));
        m_showShortLabelsCheck->setChecked(analyzer.show_short_labels);
        m_timestampSourceCombo->setCurrentIndex(
            m_timestampSourceCombo->findData(
                eti::AnalyzerSettings::timestampSourceToString(analyzer.timestamp_source)));
        m_strictFrameCrcCheck->setChecked(analyzer.strict_frame_crc);
        m_sidDisplayCombo->setCurrentIndex(
            m_sidDisplayCombo->findData(
                eti::AnalyzerSettings::sidDisplayModeToString(analyzer.sid_display_mode)));
    }
    
    updateColorButtons();
    m_fontButton->setText(QString("%1, %2pt").arg(m_displayFont.family()).arg(m_displayFont.pointSize()));
    
    m_settingsChanged = true;
}

// Getters
int SettingsDialog::getUpdateRate() const
{
    return m_updateRateSpin ? m_updateRateSpin->value() : DEFAULT_UPDATE_RATE;
}

int SettingsDialog::getBufferSize() const
{
    return m_bufferSizeSpin ? m_bufferSizeSpin->value() : DEFAULT_BUFFER_SIZE;
}

int SettingsDialog::getMemoryLimit() const
{
    return m_memoryLimitSpin ? m_memoryLimitSpin->value() : DEFAULT_MEMORY_LIMIT;
}

bool SettingsDialog::isRealTimeProcessingEnabled() const
{
    return m_realTimeCheck ? m_realTimeCheck->isChecked() : false;
}

bool SettingsDialog::isHardwareAccelerationEnabled() const
{
    return m_hardwareAccelCheck ? m_hardwareAccelCheck->isChecked() : false;
}

// Slot implementations
void SettingsDialog::accept()
{
    saveSettings();
    emit settingsChanged();
    emit analyzerSettingsChanged();
    QDialog::accept();
}

void SettingsDialog::reject()
{
    QDialog::reject();
}

void SettingsDialog::handleApply()
{
    saveSettings();
    emit settingsChanged();
    emit analyzerSettingsChanged();
}

void SettingsDialog::handleReset()
{
    resetToDefaults();
}

void SettingsDialog::handleDefaults()
{
    resetToDefaults();
}

void SettingsDialog::handleBrowseFilePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"));
    if (!dir.isEmpty()) {
        // Handle directory selection
        Logger::instance().log(Logger::Debug, "SettingsDialog", QString("Directory selected: %1").arg(dir));
    }
}

void SettingsDialog::handleColorSelection()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QColor currentColor;
    QColor *targetColor = nullptr;
    
    if (button == m_backgroundColorButton) {
        currentColor = m_backgroundColor;
        targetColor = &m_backgroundColor;
    } else if (button == m_gridColorButton) {
        currentColor = m_gridColor;
        targetColor = &m_gridColor;
    } else if (button == m_textColorButton) {
        currentColor = m_textColor;
        targetColor = &m_textColor;
    }
    
    if (targetColor) {
        QColor color = QColorDialog::getColor(currentColor, this, tr("Select Color"));
        if (color.isValid()) {
            *targetColor = color;
            updateColorButtons();
            m_settingsChanged = true;
        }
    }
}

void SettingsDialog::handleFontSelection()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_displayFont, this, tr("Select Font"));
    
    if (ok) {
        m_displayFont = font;
        m_fontButton->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
    }
}

void SettingsDialog::handleSettingsValidation()
{
    // Placeholder for settings validation
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Settings validation - placeholder");
}

// Private helper methods
void SettingsDialog::initializeComboBoxes()
{
    // Combo boxes are already initialized in createUI methods
}

void SettingsDialog::updateUI()
{
    // Load settings if not already loaded
    if (!m_settingsLoaded) {
        loadSettings();
    }
}

void SettingsDialog::updateColorButtons()
{
    if (m_backgroundColorButton) {
        m_backgroundColorButton->setStyleSheet(QString("background-color: %1").arg(m_backgroundColor.name()));
    }
    if (m_gridColorButton) {
        m_gridColorButton->setStyleSheet(QString("background-color: %1").arg(m_gridColor.name()));
    }
    if (m_textColorButton) {
        m_textColorButton->setStyleSheet(QString("background-color: %1").arg(m_textColor.name()));
    }
}

bool SettingsDialog::validateSettings()
{
    // Basic validation
    return true;
}

QVariantMap SettingsDialog::getDefaultSettings() const
{
    QVariantMap defaults;
    defaults["general/theme"] = 0;
    defaults["general/autoSave"] = true;
    defaults["general/confirmExit"] = true;
    defaults["general/autoSaveInterval"] = 60;
    defaults["analysis/updateRate"] = DEFAULT_UPDATE_RATE;
    defaults["analysis/bufferSize"] = DEFAULT_BUFFER_SIZE;
    defaults["analysis/realTime"] = false;
    defaults["analysis/hardwareAccel"] = false;
    defaults["performance/memoryLimit"] = DEFAULT_MEMORY_LIMIT;
    defaults["performance/priority"] = 1;
    defaults["advanced/debugMode"] = false;
    defaults["advanced/logLevel"] = 2;
    return defaults;
}