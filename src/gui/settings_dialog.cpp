/**
 * @file settings_dialog.cpp
 * @brief Application settings dialog implementation.
 *
 * v1.3 settings audit. Each control is either wired to real behaviour or was
 * removed (no silent no-ops). Persistence uses the explicit application
 * QSettings scope (src/utils/app_settings.hpp):
 *
 *   general/confirmExit        -> DABAnalyserWindow::closeEvent confirmation
 *   general/configPath         -> read-only display path (not persisted)
 *   analysis/updateRate        -> dashboard sampler rate in Hz (1..10) — "Processing" tab
 *   analysis/bufferSize        -> network receiver frame-buffer size — "Processing" tab
 *   audio/device,muted,volume  -> T34 AudioPlaybackController + Audio tab
 *   advanced/logLevel          -> Logger::setLogLevel (shared with the Logging tab)
 *   analyser/ (group)          -> AnalyserSettings (already wired)
 *
 * @author C++ Qt Developer Agent (settings audit, v1.3)
 */

#include "settings_dialog.h"
#include "utils/logger.h"
#include "utils/app_settings.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QIcon>
#include <QScrollArea>
#include <QFormLayout>

namespace {
// Long tabs (notably Analyser with groups A..I) must scroll instead of being
// squeezed until only the group titles remain visible.
QWidget* wrapTabInScrollArea(QWidget* content)
{
    auto* scroll = new QScrollArea();
    scroll->setObjectName(content->objectName().isEmpty()
                              ? QStringLiteral("settingsTabScroll")
                              : content->objectName() + QStringLiteral("Scroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(content);
    return scroll;
}
} // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_buttonBox(nullptr)
    , m_settingsLoaded(false)
    , m_settings(nullptr)
{
    // Explicit application scope (org StreamDAB-Analyser / app DABAnalyser).
    m_settings = streamdab::app_settings::create(this);

    // Create UI, then populate the widgets from the persisted store.
    createUI();
    initialize();

    Logger::instance().log(Logger::Info, "SettingsDialog", "Constructor completed");
}

SettingsDialog::~SettingsDialog()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Destructor starting");
    Logger::instance().log(Logger::Info, "SettingsDialog", "Destructor completed");
}

void SettingsDialog::createUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);

    createGeneralTab();
    createAnalysisTab();
    createAudioTab();
    createAdvancedTab();
    createAnalyserTab();

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
        QDialogButtonBox::Apply | QDialogButtonBox::RestoreDefaults,
        this
    );

    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsDialog::handleApply);
    connect(m_buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &SettingsDialog::handleDefaults);

    setObjectName("SettingsDialog");
    setWindowTitle(tr("StreamDAB Stream Analyser - Settings"));
    setWindowIcon(QIcon(":/icons/settings.png"));
    setModal(true);
    setMinimumSize(640, 480);
    resize(780, 620);

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

    QGroupBox *behaviourGroup = new QGroupBox(tr("Behaviour"), m_generalTab);
    QFormLayout *behaviourLayout = new QFormLayout(behaviourGroup);

    m_confirmExitCheck = new QCheckBox(tr("Confirm before closing the application"));
    m_confirmExitCheck->setObjectName(QStringLiteral("settingsConfirmExit"));
    m_confirmExitCheck->setToolTip(tr(
        "When enabled, closing the main window (or File -> Exit) asks for "
        "confirmation before the capture is torn down. Applied immediately."));
    behaviourLayout->addRow(m_confirmExitCheck);

    // Read-only configuration-store location: tells the user where preferences
    // actually live on this platform (see docs/CONFIGURATION.md).
    QGroupBox *storeGroup = new QGroupBox(tr("Configuration"), m_generalTab);
    QFormLayout *storeLayout = new QFormLayout(storeGroup);
    m_configPathLabel = new QLabel(streamdab::app_settings::filePath());
    m_configPathLabel->setObjectName(QStringLiteral("settingsConfigPath"));
    m_configPathLabel->setWordWrap(true);
    m_configPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_configPathLabel->setToolTip(tr(
        "Native per-platform settings store written by this application. "
        "Delete this file (or remove the registry key / plist) to reset."));
    storeLayout->addRow(tr("Store:"), m_configPathLabel);

    layout->addWidget(behaviourGroup);
    layout->addWidget(storeGroup);
    layout->addStretch();

    m_tabWidget->addTab(wrapTabInScrollArea(m_generalTab), tr("General"));
}

void SettingsDialog::createAnalysisTab()
{
    m_analysisTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_analysisTab);

    // Explicit group title: the tab was previously named "Analysis" with a
    // "Performance" group, which said nothing about WHAT the two fields do.
    // Both fields now carry their unit (suffix) and a tooltip that states the
    // effect AND when it applies, so the tab is self-explanatory.
    QGroupBox *performanceGroup = new QGroupBox(tr("Sampling and buffering"), m_analysisTab);
    QFormLayout *performanceLayout = new QFormLayout(performanceGroup);

    m_updateRateSpin = new QSpinBox();
    m_updateRateSpin->setObjectName(QStringLiteral("settingsUpdateRate"));
    m_updateRateSpin->setRange(MIN_UPDATE_RATE, MAX_UPDATE_RATE);
    m_updateRateSpin->setValue(DEFAULT_UPDATE_RATE);
    m_updateRateSpin->setSuffix(tr(" Hz"));
    m_updateRateSpin->setToolTip(tr(
        "Dashboard sampler rate. Drives how often the Performance Dashboard and "
        "Real-Time Chart are refreshed with real FPS/RSS/CPU. 1 Hz (default) is "
        "the historical 1 s cadence. Applies immediately."));

    m_bufferSizeSpin = new QSpinBox();
    m_bufferSizeSpin->setObjectName(QStringLiteral("settingsBufferSize"));
    m_bufferSizeSpin->setRange(MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
    m_bufferSizeSpin->setValue(DEFAULT_BUFFER_SIZE);
    m_bufferSizeSpin->setSuffix(tr(" frames"));
    m_bufferSizeSpin->setToolTip(tr(
        "Network receive buffer in ETI frames. Applied to a live connection "
        "(re-initialises it) and to every new connection. File playback is "
        "unaffected."));

    performanceLayout->addRow(tr("Dashboard update rate:"), m_updateRateSpin);
    performanceLayout->addRow(tr("Network receive buffer:"), m_bufferSizeSpin);

    layout->addWidget(performanceGroup);
    layout->addStretch();

    m_tabWidget->addTab(wrapTabInScrollArea(m_analysisTab), tr("Processing"));
}

void SettingsDialog::createAudioTab()
{
    m_audioTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_audioTab);

    QGroupBox *deviceGroup = new QGroupBox(tr("Audio Device"), m_audioTab);
    QFormLayout *deviceLayout = new QFormLayout(deviceGroup);

    m_audioDeviceCombo = new QComboBox();
    m_audioDeviceCombo->setObjectName(QStringLiteral("settingsAudioDevice"));
    m_audioDeviceCombo->setToolTip(tr(
        "ALSA output device used by the T34 DAB+ audio playback. The same "
        "state as the Transport group's Audio tab."));

    m_audioMuteCheck = new QCheckBox(tr("Mute"));
    m_audioMuteCheck->setObjectName(QStringLiteral("settingsAudioMute"));
    deviceLayout->addRow(tr("Output device:"), m_audioDeviceCombo);
    deviceLayout->addRow(m_audioMuteCheck);

    QGroupBox *levelGroup = new QGroupBox(tr("Audio Levels"), m_audioTab);
    QFormLayout *levelLayout = new QFormLayout(levelGroup);

    m_audioVolumeSlider = new QSlider(Qt::Horizontal);
    m_audioVolumeSlider->setObjectName(QStringLiteral("settingsAudioVolume"));
    m_audioVolumeSlider->setRange(0, 100);
    m_audioVolumeSlider->setValue(DEFAULT_AUDIO_VOLUME);
    m_audioVolumeLabel = new QLabel(QStringLiteral("%1%").arg(DEFAULT_AUDIO_VOLUME));
    QHBoxLayout *volumeRow = new QHBoxLayout();
    volumeRow->addWidget(m_audioVolumeSlider, 1);
    volumeRow->addWidget(m_audioVolumeLabel);
    connect(m_audioVolumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_audioVolumeLabel) {
            m_audioVolumeLabel->setText(QString("%1%").arg(value));
        }
    });
    levelLayout->addRow(tr("Volume:"), volumeRow);

    layout->addWidget(deviceGroup);
    layout->addWidget(levelGroup);
    layout->addStretch();

    m_tabWidget->addTab(wrapTabInScrollArea(m_audioTab), tr("Audio"));
}

void SettingsDialog::createAdvancedTab()
{
    m_advancedTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_advancedTab);

    QGroupBox *debugGroup = new QGroupBox(tr("Logging"), m_advancedTab);
    QFormLayout *debugLayout = new QFormLayout(debugGroup);

    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->setObjectName(QStringLiteral("settingsLogLevel"));
    // Order/indices are the canonical encoding of `advanced/logLevel`; the
    // bottom-strip Logging tab reuses the same mapping (see indexForLoggerLevel)
    // so the two controls can never disagree. Critical is appended so the
    // mapping is a bijection (Debug/Info/Warning/Error/Critical are all named
    // in the Logging tab).
    m_logLevelCombo->addItems({tr("Error"), tr("Warning"), tr("Info"), tr("Debug"),
                               tr("Critical")});
    m_logLevelCombo->setCurrentIndex(2); // Info
    m_logLevelCombo->setToolTip(tr(
        "Minimum severity written to the log. Applied to the singleton Logger "
        "immediately (Console + file). Debug enables verbose output, Critical "
        "shows only critical messages."));

    debugLayout->addRow(tr("Log level:"), m_logLevelCombo);

    layout->addWidget(debugGroup);
    layout->addStretch();

    m_tabWidget->addTab(wrapTabInScrollArea(m_advancedTab), tr("Advanced"));
}

void SettingsDialog::createAnalyserTab()
{
    // "Analyser / Decode options" — implements the SETTING rows of
    // docs/fixes/OPTION_VARIANT_MATRIX.md (rows 1-6, 8, 10-12, 14).
    // Each default equals the etisnoop-parity behavior, so a default-config
    // run is byte-identical to the baseline. Changes are persisted under
    // group "analyser/" and honored on the next file load (CLI immediately;
    // GUI decode re-reads on load).
    m_analyserTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_analyserTab);

    auto addGroup = [layout](const QString& title, QWidget* parent) -> QGroupBox* {
        QGroupBox *group = new QGroupBox(title, parent);
        group->setLayout(new QFormLayout(group));
        layout->addWidget(group);
        return group;
    };

    // --- Row A: Label charset ---
    QGroupBox *charsetGroup = addGroup(tr("A. Label charset (matrix row 1)"), m_analyserTab);
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
    QGroupBox *etiGroup = addGroup(tr("B. ETI format (matrix row 2)"), m_analyserTab);
    QFormLayout *etiLayout = qobject_cast<QFormLayout*>(etiGroup->layout());

    m_etiModeCombo = new QComboBox();
    m_etiModeCombo->addItem(tr("auto — sync-based detection (default)"), QStringLiteral("auto"));
    m_etiModeCombo->addItem(tr("li — force ETI-LI"), QStringLiteral("li"));
    m_etiModeCombo->addItem(tr("ni — force ETI-NI"), QStringLiteral("ni"));
    m_etiModeCombo->setToolTip(tr(
        "Force the ETI transport branch; auto (default) detects by sync pattern."));
    etiLayout->addRow(tr("ETI mode:"), m_etiModeCombo);

    // --- Row C: NST legacy correction ---
    QGroupBox *nstGroup = addGroup(tr("C. NST interpretation (matrix row 3)"), m_analyserTab);
    QFormLayout *nstLayout = qobject_cast<QFormLayout*>(nstGroup->layout());

    m_nstOffset1Check = new QCheckBox(tr("Legacy count-1 correction: NST = (p[5]&0x7F) + 1"));
    m_nstOffset1Check->setToolTip(tr(
        "Some legacy muxes wrote the stream count minus one in the NST field; "
        "enable to decode one additional STC entry and shift the FIC offset."));
    nstLayout->addRow(m_nstOffset1Check);

    // --- Row D/E: FIC framework + FIB CRC ---
    QGroupBox *ficGroup = addGroup(tr("D. FIC framework / FIB CRC (matrix rows 4-5)"), m_analyserTab);
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
    QGroupBox *eccGroup = addGroup(tr("E. ECC / country (matrix rows 6, 14)"), m_analyserTab);
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
    QGroupBox *labelGroup = addGroup(tr("F. Label display (matrix row 8)"), m_analyserTab);
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
    QGroupBox *tsGroup = addGroup(tr("G. Timestamp source (matrix row 10)"), m_analyserTab);
    QFormLayout *tsLayout = qobject_cast<QFormLayout*>(tsGroup->layout());

    m_timestampSourceCombo = new QComboBox();
    m_timestampSourceCombo->addItem(tr("tist — frame TIST (default)"), QStringLiteral("tist"));
    m_timestampSourceCombo->addItem(tr("mtime — file mtime fallback"), QStringLiteral("mtime"));
    m_timestampSourceCombo->setToolTip(tr(
        "Per-frame timestamp source for verbose output / the yaml tist_ms line."));
    tsLayout->addRow(tr("Timestamp source:"), m_timestampSourceCombo);

    // --- Row H: strict frame CRC ---
    QGroupBox *crcGroup = addGroup(tr("H. EOH/EOF frame CRC (matrix row 11)"), m_analyserTab);
    QFormLayout *crcLayout = qobject_cast<QFormLayout*>(crcGroup->layout());

    m_strictFrameCrcCheck = new QCheckBox(tr("Validate + count the EOH gap frame CRC (CCITT-FALSE)"));
    m_strictFrameCrcCheck->setToolTip(tr(
        "Validate the EOH gap CRC (bytes 8+4*NST .. 11+4*NST) and report "
        "mismatch counters; decode stays independent (never blocks parsing)."));
    crcLayout->addRow(m_strictFrameCrcCheck);

    // --- Row I: SId display ---
    QGroupBox *sidGroup = addGroup(tr("I. SId display (matrix row 12)"), m_analyserTab);
    QFormLayout *sidLayout = qobject_cast<QFormLayout*>(sidGroup->layout());

    m_sidDisplayCombo = new QComboBox();
    m_sidDisplayCombo->addItem(tr("hex32 — full 32-bit hex (default)"), QStringLiteral("hex32"));
    m_sidDisplayCombo->addItem(tr("ecc-sid — ECC:CId:SRef (e.g. F3:2:00001)"), QStringLiteral("ecc-sid"));
    m_sidDisplayCombo->addItem(tr("hex16 — lower 16 bits"), QStringLiteral("hex16"));
    m_sidDisplayCombo->setToolTip(tr(
        "service_id rendering in the YAML output for data (32-bit) services."));
    sidLayout->addRow(tr("SId display:"), m_sidDisplayCombo);

    layout->addStretch();
    m_tabWidget->addTab(wrapTabInScrollArea(m_analyserTab), tr("Analyser"));
}

bool SettingsDialog::initialize()
{
    Logger::instance().log(Logger::Info, "SettingsDialog", "Initialization starting");

    try {
        loadSettings();
        m_settingsLoaded = true;
        Logger::instance().log(Logger::Info, "SettingsDialog",
                               "Initialization completed successfully");
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
        // General: opt-in exit confirmation (default ON, as before the audit).
        m_confirmExitCheck->setChecked(m_settings->value("general/confirmExit", true).toBool());

        // Analysis: dashboard sampler rate + network receiver buffer.
        m_updateRateSpin->setValue(
            qBound(MIN_UPDATE_RATE,
                   m_settings->value("analysis/updateRate", DEFAULT_UPDATE_RATE).toInt(),
                   MAX_UPDATE_RATE));
        m_bufferSizeSpin->setValue(
            qBound(MIN_BUFFER_SIZE,
                   m_settings->value("analysis/bufferSize", DEFAULT_BUFFER_SIZE).toInt(),
                   MAX_BUFFER_SIZE));

        // Audio: shared with the T34 Audio tab / controller.
        m_audioDeviceId = m_settings->value("audio/device", QString()).toString();
        if (m_audioDeviceCombo) {
            const int deviceIndex = m_audioDeviceCombo->findData(m_audioDeviceId);
            if (deviceIndex >= 0) {
                m_audioDeviceCombo->setCurrentIndex(deviceIndex);
            }
        }
        m_audioMuteCheck->setChecked(m_settings->value("audio/muted", false).toBool());
        m_audioVolumeSlider->setValue(
            qBound(0, m_settings->value("audio/volume", DEFAULT_AUDIO_VOLUME).toInt(), 100));

        // Advanced: logger level (0=Error, 1=Warning, 2=Info, 3=Debug,
        // 4=Critical; see loggerLevelForIndex()).
        m_logLevelCombo->setCurrentIndex(
            qBound(0, m_settings->value("advanced/logLevel", 2).toInt(), 4));

        // Load analyser / decode settings (option-variant matrix rows 1-14)
        {
            eti::AnalyserSettings analyser = eti::AnalyserSettings::defaults();
            analyser.loadFromQSettings(*m_settings);
            m_forceCharsetSpin->setValue(analyser.force_charset);
            m_charsetFallbackCombo->setCurrentIndex(
                m_charsetFallbackCombo->findData(
                    eti::AnalyserSettings::charsetFallbackToString(analyser.charset_fallback_policy)));
            m_etiModeCombo->setCurrentIndex(
                m_etiModeCombo->findData(eti::AnalyserSettings::etiModeToString(analyser.eti_mode)));
            m_nstOffset1Check->setChecked(analyser.nst_offset_1);
            m_ficModeCombo->setCurrentIndex(
                m_ficModeCombo->findData(eti::AnalyserSettings::ficModeToString(analyser.fic_mode)));
            m_fibIgnoreCrcCheck->setChecked(analyser.fib_ignore_crc);
            m_eccOverrideSpin->setValue(analyser.ecc_override);
            m_serviceEccDeepParseCheck->setChecked(analyser.service_ecc_deep_parse);
            m_labelSourceCombo->setCurrentIndex(
                m_labelSourceCombo->findData(
                    eti::AnalyserSettings::labelSourceToString(analyser.label_source_priority)));
            m_showShortLabelsCheck->setChecked(analyser.show_short_labels);
            m_timestampSourceCombo->setCurrentIndex(
                m_timestampSourceCombo->findData(
                    eti::AnalyserSettings::timestampSourceToString(analyser.timestamp_source)));
            m_strictFrameCrcCheck->setChecked(analyser.strict_frame_crc);
            m_sidDisplayCombo->setCurrentIndex(
                m_sidDisplayCombo->findData(
                    eti::AnalyserSettings::sidDisplayModeToString(analyser.sid_display_mode)));
        }

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
        // General
        m_settings->setValue("general/confirmExit", m_confirmExitCheck->isChecked());

        // Analysis
        m_settings->setValue("analysis/updateRate", m_updateRateSpin->value());
        m_settings->setValue("analysis/bufferSize", m_bufferSizeSpin->value());

        // Audio (shared with the T34 Audio tab). Keep the selected device id
        // when the combo is empty (dialog constructed without a backend).
        if (m_audioDeviceCombo && m_audioDeviceCombo->count() > 0) {
            m_audioDeviceId = m_audioDeviceCombo->currentData().toString();
        }
        m_settings->setValue("audio/device", m_audioDeviceId);
        m_settings->setValue("audio/muted", m_audioMuteCheck->isChecked());
        m_settings->setValue("audio/volume", m_audioVolumeSlider->value());

        // Advanced
        m_settings->setValue("advanced/logLevel", m_logLevelCombo->currentIndex());

        // Analyser / decode settings (option-variant matrix rows 1-14)
        {
            eti::AnalyserSettings analyser = eti::AnalyserSettings::defaults();
            analyser.force_charset = m_forceCharsetSpin->value();
            {
                eti::CharsetFallbackPolicy p;
                if (eti::AnalyserSettings::charsetFallbackFromString(
                        m_charsetFallbackCombo->currentData().toString(), p)) {
                    analyser.charset_fallback_policy = p;
                }
            }
            {
                eti::EtiModeSetting m;
                if (eti::AnalyserSettings::etiModeFromString(
                        m_etiModeCombo->currentData().toString(), m)) {
                    analyser.eti_mode = m;
                }
            }
            analyser.nst_offset_1 = m_nstOffset1Check->isChecked();
            {
                eti::FicModeSetting m;
                if (eti::AnalyserSettings::ficModeFromString(
                        m_ficModeCombo->currentData().toString(), m)) {
                    analyser.fic_mode = m;
                }
            }
            analyser.fib_ignore_crc = m_fibIgnoreCrcCheck->isChecked();
            analyser.ecc_override = m_eccOverrideSpin->value();
            analyser.service_ecc_deep_parse = m_serviceEccDeepParseCheck->isChecked();
            {
                eti::LabelSourcePriority p;
                if (eti::AnalyserSettings::labelSourceFromString(
                        m_labelSourceCombo->currentData().toString(), p)) {
                    analyser.label_source_priority = p;
                }
            }
            analyser.show_short_labels = m_showShortLabelsCheck->isChecked();
            {
                eti::TimestampSource t;
                if (eti::AnalyserSettings::timestampSourceFromString(
                        m_timestampSourceCombo->currentData().toString(), t)) {
                    analyser.timestamp_source = t;
                }
            }
            analyser.strict_frame_crc = m_strictFrameCrcCheck->isChecked();
            {
                eti::SidDisplayMode m;
                if (eti::AnalyserSettings::sidDisplayModeFromString(
                        m_sidDisplayCombo->currentData().toString(), m)) {
                    analyser.sid_display_mode = m;
                }
            }
            analyser.saveToQSettings(*m_settings);
        }

        m_settings->sync();

        Logger::instance().log(Logger::Debug, "SettingsDialog", "Settings saved successfully");
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "SettingsDialog",
                             QString("Failed to save settings: %1").arg(e.what()));
    }
}

void SettingsDialog::resetToDefaults()
{
    Logger::instance().log(Logger::Debug, "SettingsDialog", "Resetting settings to defaults");

    m_confirmExitCheck->setChecked(true);

    m_updateRateSpin->setValue(DEFAULT_UPDATE_RATE);
    m_bufferSizeSpin->setValue(DEFAULT_BUFFER_SIZE);

    // Audio: keep the currently selected device (hardware-specific) but reset
    // mute/volume to the Audio tab defaults.
    m_audioMuteCheck->setChecked(false);
    m_audioVolumeSlider->setValue(DEFAULT_AUDIO_VOLUME);

    m_logLevelCombo->setCurrentIndex(2); // Info

    // Reset analyser / decode options to the etisnoop-parity defaults.
    {
        const eti::AnalyserSettings analyser = eti::AnalyserSettings::defaults();
        m_forceCharsetSpin->setValue(analyser.force_charset);
        m_charsetFallbackCombo->setCurrentIndex(
            m_charsetFallbackCombo->findData(
                eti::AnalyserSettings::charsetFallbackToString(analyser.charset_fallback_policy)));
        m_etiModeCombo->setCurrentIndex(
            m_etiModeCombo->findData(eti::AnalyserSettings::etiModeToString(analyser.eti_mode)));
        m_nstOffset1Check->setChecked(analyser.nst_offset_1);
        m_ficModeCombo->setCurrentIndex(
            m_ficModeCombo->findData(eti::AnalyserSettings::ficModeToString(analyser.fic_mode)));
        m_fibIgnoreCrcCheck->setChecked(analyser.fib_ignore_crc);
        m_eccOverrideSpin->setValue(analyser.ecc_override);
        m_serviceEccDeepParseCheck->setChecked(analyser.service_ecc_deep_parse);
        m_labelSourceCombo->setCurrentIndex(
            m_labelSourceCombo->findData(
                eti::AnalyserSettings::labelSourceToString(analyser.label_source_priority)));
        m_showShortLabelsCheck->setChecked(analyser.show_short_labels);
        m_timestampSourceCombo->setCurrentIndex(
            m_timestampSourceCombo->findData(
                eti::AnalyserSettings::timestampSourceToString(analyser.timestamp_source)));
        m_strictFrameCrcCheck->setChecked(analyser.strict_frame_crc);
        m_sidDisplayCombo->setCurrentIndex(
            m_sidDisplayCombo->findData(
                eti::AnalyserSettings::sidDisplayModeToString(analyser.sid_display_mode)));
    }
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

bool SettingsDialog::isConfirmExitEnabled() const
{
    return m_confirmExitCheck ? m_confirmExitCheck->isChecked() : true;
}

int SettingsDialog::logLevelIndex() const
{
    return m_logLevelCombo ? m_logLevelCombo->currentIndex() : 2;
}

void SettingsDialog::setAudioOptions(const QStringList& deviceNames,
                                     const QStringList& deviceIds,
                                     const QString& currentDeviceId,
                                     bool muted,
                                     int volume)
{
    if (!m_audioDeviceCombo) {
        return;
    }

    m_audioDeviceCombo->clear();
    const int count = qMin(deviceNames.size(), deviceIds.size());
    for (int i = 0; i < count; ++i) {
        m_audioDeviceCombo->addItem(deviceNames.at(i), deviceIds.at(i));
    }

    // Prefer the live controller device, then the persisted device, else the
    // first entry; an empty selection means "backend default".
    QString wanted = currentDeviceId;
    if (wanted.isEmpty()) {
        wanted = m_audioDeviceId;
    }
    const int index = m_audioDeviceCombo->findData(wanted);
    if (index >= 0) {
        m_audioDeviceCombo->setCurrentIndex(index);
    }
    m_audioDeviceId = m_audioDeviceCombo->currentData().toString();

    m_audioMuteCheck->setChecked(muted);
    m_audioVolumeSlider->setValue(qBound(0, volume, 100));
}

QString SettingsDialog::audioOutputDevice() const
{
    if (m_audioDeviceCombo && m_audioDeviceCombo->count() > 0) {
        return m_audioDeviceCombo->currentData().toString();
    }
    return m_audioDeviceId;
}

bool SettingsDialog::isAudioMuted() const
{
    return m_audioMuteCheck && m_audioMuteCheck->isChecked();
}

int SettingsDialog::audioVolume() const
{
    return m_audioVolumeSlider ? m_audioVolumeSlider->value() : DEFAULT_AUDIO_VOLUME;
}

int SettingsDialog::loggerLevelForIndex(int index)
{
    // Combo order: 0 Error, 1 Warning, 2 Info, 3 Debug, 4 Critical.
    // Logger::LogLevel: Debug=0, Info=1, Warning=2, Error=3, Critical=4.
    switch (index) {
        case 0: return 3;  // Error
        case 1: return 2;  // Warning
        case 3: return 0;  // Debug
        case 4: return 4;  // Critical
        case 2:
        default: return 1; // Info
    }
}

int SettingsDialog::indexForLoggerLevel(int loggerLevel)
{
    // Inverse of loggerLevelForIndex() (see the header for the contract).
    switch (loggerLevel) {
        case 0: return 3;  // Debug
        case 2: return 1;  // Warning
        case 3: return 0;  // Error
        case 4: return 4;  // Critical
        case 1:
        default: return 2; // Info
    }
}

// Slot implementations
void SettingsDialog::accept()
{
    saveSettings();
    emit settingsChanged();
    emit analyserSettingsChanged();
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
    emit analyserSettingsChanged();
}

void SettingsDialog::handleDefaults()
{
    resetToDefaults();
}
