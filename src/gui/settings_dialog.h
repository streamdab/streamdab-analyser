#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSettings>
#include <QString>
#include <QStringList>
#include "../core/analyser_settings.hpp"

/**
 * @class SettingsDialog
 * @brief Application settings dialog.
 *
 * v1.3 settings audit: every control either drives real behaviour or was
 * removed. The dialog persists to the application's explicit QSettings scope
 * (see src/utils/app_settings.hpp / docs/CONFIGURATION.md) and is the SAME
 * state the application reads:
 *   - General    : confirm-exit (closeEvent) + config-file location (read-only)
 *   - Processing : dashboard sampler update rate (Hz) + network receiver buffer
 *   - Audio      : output device / mute / volume (shared with the T34 Audio tab)
 *   - Advanced   : log level (Logger)
 *   - Analyser   : analyser/decode options (already wired, unchanged)
 *
 * Controls with no backend were deleted: theme, auto-save + interval,
 * display colours/font, real-time processing, hardware acceleration,
 * proxy server, memory limit and process priority. See docs/CONFIGURATION.md
 * for the rationale and the removed-key list.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    enum class SettingsCategory {
        General = 0,
        Processing,   // the "Processing" tab (sampler rate + network buffer)
        Audio,
        Advanced,
        Analyser
    };

    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    /** @brief Load current settings into the widgets (called by the ctor). */
    bool initialize();

    /** @brief Load settings from the application QSettings scope. */
    void loadSettings();

    /** @brief Persist the widgets to the application QSettings scope. */
    void saveSettings();

    /** @brief Reset the widgets to their parity-preserving defaults. */
    void resetToDefaults();

    /** @brief Dashboard sampler rate in Hz (1..10). */
    int getUpdateRate() const;

    /** @brief Network receiver frame-buffer size in frames (10..10000). */
    int getBufferSize() const;

    /** @brief True when the window should ask before closing. */
    bool isConfirmExitEnabled() const;

    /** @brief Log-level combo index (0=Error, 1=Warning, 2=Info, 3=Debug, 4=Critical). */
    int logLevelIndex() const;

    /**
     * @brief Populate the Audio tab from the live T34 backend state.
     *
     * deviceNames/deviceIds are parallel lists in combo order (display text /
     * backend device name). The dialog and the Audio tab therefore show and
     * edit the SAME state; no second, disagreeing copy exists.
     */
    void setAudioOptions(const QStringList& deviceNames,
                         const QStringList& deviceIds,
                         const QString& currentDeviceId,
                         bool muted,
                         int volume);

    /** @brief Selected backend output-device name (empty = backend default). */
    QString audioOutputDevice() const;
    /** @brief True when audio output is muted. */
    bool isAudioMuted() const;
    /** @brief Output volume 0..100. */
    int audioVolume() const;

    /**
     * @brief Map the log-level combo index to Logger::LogLevel's integer value.
     *
     * Kept as a plain int so this header does not depend on utils/logger.h.
     * 0(Error)->3, 1(Warning)->2, 2(Info)->1, 3(Debug)->0, 4(Critical)->4.
     */
    static int loggerLevelForIndex(int index);

    /**
     * @brief Inverse of loggerLevelForIndex(): Logger::LogLevel int -> combo index.
     *
     * Used by the bottom-strip Logging tab to persist the level into the SAME
     * `advanced/logLevel` key (single source of truth: both write/read Logger).
     * Debug(0)->3, Info(1)->2, Warning(2)->1, Error(3)->0, Critical(4)->4.
     */
    static int indexForLoggerLevel(int loggerLevel);

signals:
    /** @brief Emitted after settings are saved (OK or Apply). */
    void settingsChanged();
    /** @brief Emitted when the Analyser / Decode options change. */
    void analyserSettingsChanged();
    /** @brief Emitted when a specific category changes (reserved). */
    void categoryChanged(SettingsCategory category);

protected:
    void accept() override;
    void reject() override;

private slots:
    void handleApply();
    void handleDefaults();

private:
    void createUI();
    void createGeneralTab();
    void createAnalysisTab();
    void createAudioTab();
    void createAdvancedTab();
    void createAnalyserTab();

    // UI components
    QTabWidget *m_tabWidget = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;

    // General tab
    QWidget *m_generalTab = nullptr;
    QCheckBox *m_confirmExitCheck = nullptr;
    QLabel *m_configPathLabel = nullptr;

    // Analysis tab
    QWidget *m_analysisTab = nullptr;
    QSpinBox *m_updateRateSpin = nullptr;
    QSpinBox *m_bufferSizeSpin = nullptr;

    // Audio tab (mirrors the T34 Audio tab / controller state)
    QWidget *m_audioTab = nullptr;
    QComboBox *m_audioDeviceCombo = nullptr;
    QCheckBox *m_audioMuteCheck = nullptr;
    QSlider *m_audioVolumeSlider = nullptr;
    QLabel *m_audioVolumeLabel = nullptr;

    // Advanced tab
    QWidget *m_advancedTab = nullptr;
    QComboBox *m_logLevelCombo = nullptr;

    // Analyser / Decode options tab (option-variant matrix rows 1-14)
    QWidget *m_analyserTab = nullptr;
    // Row A (charset)
    QSpinBox *m_forceCharsetSpin = nullptr;
    QComboBox *m_charsetFallbackCombo = nullptr;
    // Row B (ETI format)
    QComboBox *m_etiModeCombo = nullptr;
    // Row C (NST legacy correction)
    QCheckBox *m_nstOffset1Check = nullptr;
    // Row D/E (FIC framework + FIB CRC)
    QComboBox *m_ficModeCombo = nullptr;
    QCheckBox *m_fibIgnoreCrcCheck = nullptr;
    // Row F/G (ECC override + per-service deep parse)
    QSpinBox *m_eccOverrideSpin = nullptr;
    QCheckBox *m_serviceEccDeepParseCheck = nullptr;
    // Row H (label source + short label)
    QComboBox *m_labelSourceCombo = nullptr;
    QCheckBox *m_showShortLabelsCheck = nullptr;
    // Row I (timestamp source)
    QComboBox *m_timestampSourceCombo = nullptr;
    // Row J (strict frame CRC)
    QCheckBox *m_strictFrameCrcCheck = nullptr;
    // Row K (SId display)
    QComboBox *m_sidDisplayCombo = nullptr;

    // Settings state
    bool m_settingsLoaded = false;
    QSettings *m_settings = nullptr;

    // Last-selected audio device (used when the device list is (re)populated).
    QString m_audioDeviceId;

    // Constants
    static constexpr int MIN_UPDATE_RATE = 1;          // Hz (1 Hz == the old
    static constexpr int MAX_UPDATE_RATE = 10;         // 1 s sampler cadence)
    static constexpr int DEFAULT_UPDATE_RATE = 1;
    static constexpr int MIN_BUFFER_SIZE = 10;
    static constexpr int MAX_BUFFER_SIZE = 10000;
    static constexpr int DEFAULT_BUFFER_SIZE = 1000;
    static constexpr int DEFAULT_AUDIO_VOLUME = 100;
};
