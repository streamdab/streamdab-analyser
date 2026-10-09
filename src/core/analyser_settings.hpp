/**
 * @file analyser_settings.hpp
 * @brief User-selectable analyser decode options (option-variant matrix).
 *
 * Implements the "SETTING" rows of docs/fixes/OPTION_VARIANT_MATRIX.md as a
 * single settings struct shared by the GUI (QSettings, org
 * "StreamDAB-Analyser", app "DABAnalyser", group "analyser/") and the CLI
 * (command-line flags built on top of the same defaults).
 *
 * CRITICAL RULE (user mandate / gate 2): every default equals the current
 * etisnoop-parity behavior, so a default-config run (no QSettings values, no
 * CLI flags) is byte-identical to the committed baseline. Never change a
 * default without updating AnalyserSettings::defaults() and the unit tests.
 *
 * Rows covered (option-variant matrix):
 *   1  label charset (force_charset / charset_fallback_policy)
 *   2  ETI format (eti_mode)
 *   3  NST legacy correction (nst_offset_1)
 *   4  FIC framework (fic_mode)
 *   5  FIB CRC strictness (fib_ignore_crc)
 *   6  ECC / country override (ecc_override)
 *   8  label source + short label (label_source_priority, show_short_labels)
 *   10 timestamp source (timestamp_source)
 *   11 EOH/EOF frame CRC validation (strict_frame_crc)
 *   12 SId display (sid_display_mode)
 *   14 per-service ECC deep parse (service_ecc_deep_parse)
 *
 * @author C++ Qt Developer Agent (settings feature, v1.3)
 */

#ifndef STREAMDAB_ANALYSER_SETTINGS_HPP
#define STREAMDAB_ANALYSER_SETTINGS_HPP

#include "eti_types.hpp"   // FicDecodeMode (row 4 helper)
#include <QString>
#include <QMap>
#include <QVariant>
#include <cstdint>

class QSettings;

namespace eti {

/** Row 1: behaviour when a FIG 1 label carries an unknown charset flag. */
enum class CharsetFallbackPolicy : uint8_t {
    Raw = 0,       // default: raw bytes + warn-once (current behavior)
    ForceUtf8 = 1, // treat bytes as UTF-8 passthrough, no warning
    Skip = 2       // drop the label (empty string), no warning
};

/** Row 2: forced ETI transport selection (auto = sync-based detection). */
enum class EtiModeSetting : uint8_t {
    Auto = 0,  // default: detectETIFormat() sync-based
    Li = 1,    // force the ETI-LI branch
    Ni = 2     // force the ETI-NI branch
};

/** Row 4: FIC decode mode (maps 1:1 to eti::FicDecodeMode). */
enum class FicModeSetting : uint8_t {
    Strict = 0, // always validate per-FIB CRC, drop bad FIBs
    Raw = 1,    // whole FIC as a raw FIG stream, no per-FIB CRC
    Auto = 2    // default: strict + all-FIB-CRC-fail -> raw fallback
};

/** Row 8: which label source wins on name conflicts. */
enum class LabelSourcePriority : uint8_t {
    Fig016First = 0, // default: FIG 0/16 service-org label preferred
    Fig11First = 1   // FIG 1/1 label-only services preferred
};

/** Row 10: headless per-frame timestamp source. */
enum class TimestampSource : uint8_t {
    Tist = 0,  // default: tail TIST (NI) / LIDATA TIST (LI)
    Mtime = 1  // file mtime fallback (per-frame stepping)
};

/** Row 12: service_id rendering in YAML output. */
enum class SidDisplayMode : uint8_t {
    Hex32 = 0,  // default: full 32-bit "0xF3200001" / programme "0x00002500"
    EccSid = 1, // "ECC:CId:SRef" decomposition, e.g. "F3:2:00001"
    Hex16 = 2   // lower 16 bits only, e.g. "0x0001"
};

/**
 * @brief User-selected analyser decode options (option-variant matrix).
 *
 * Plain data struct, no Qt Widgets. Persisted under group "analyser/" via
 * QSettings (org "StreamDAB-Analyser", app "DABAnalyser"). The CLI builds the
 * struct from defaults() + CLI-flag overrides only (never from the user's
 * GUI QSettings), keeping default runs byte-identical to the baseline.
 */
struct AnalyserSettings {
    // --- Row 1: label charset ---
    int force_charset{-1};                 // -1 = follow stream; 0/3/6 = override
    CharsetFallbackPolicy charset_fallback_policy{CharsetFallbackPolicy::Raw};

    // --- Row 2: ETI format ---
    EtiModeSetting eti_mode{EtiModeSetting::Auto};

    // --- Row 3: NST legacy correction ---
    bool nst_offset_1{false};              // NST = (p[5]&0x7F) + 1 (legacy muxes)

    // --- Row 4/5: FIC framework + FIB CRC ---
    FicModeSetting fic_mode{FicModeSetting::Auto};
    bool fib_ignore_crc{false};            // decode FIBs even when CRC fails

    // --- Row 6: ECC / country override ---
    int ecc_override{-1};                  // -1 = auto from stream; >=0 manual ECC
    bool service_ecc_deep_parse{false};    // FIG 0/9 extended per-service ECC

    // --- Row 8: label source + short label ---
    LabelSourcePriority label_source_priority{LabelSourcePriority::Fig016First};
    bool show_short_labels{false};         // emit service/ensemble_short_label

    // --- Row 10: timestamp source ---
    TimestampSource timestamp_source{TimestampSource::Tist};

    // --- Row 11: strict EOH/EOF frame CRC validation ---
    bool strict_frame_crc{false};          // validate + count; never blocks decode

    // --- Row 12: SId display ---
    SidDisplayMode sid_display_mode{SidDisplayMode::Hex32};

    /** @brief Defaults == current etisnoop-parity behavior (byte parity gate). */
    static AnalyserSettings defaults();

    // --- QSettings persistence (group "analyser/") ---
    /** @brief Save into an explicit QSettings instance. */
    void saveToQSettings(QSettings& s) const;
    /** @brief Load from an explicit QSettings instance (unknown keys -> defaults). */
    void loadFromQSettings(QSettings& s);
    /** @brief Save into the default application QSettings. */
    void saveToQSettings() const;
    /** @brief Load from the default application QSettings. */
    void loadFromQSettings();

    /**
     * @brief Apply CLI-flag overrides on top of the current values.
     *
     * Only keys present in @p overrides are applied; missing keys keep their
     * current value (so defaults() + sparse map == defaults).
     * Recognized keys: force_charset (int), charset_fallback_policy (QString
     * "raw|utf8|skip"), eti_mode ("auto|li|ni"), nst_offset_1 (bool),
     * fic_mode ("strict|raw|auto"), fib_ignore_crc (bool),
     * strict_frame_crc (bool), ecc_override (int, -1..255),
     * service_ecc_deep_parse (bool), label_source_priority ("fig016|fig11"),
     * show_short_labels (bool), timestamp_source ("tist|mtime"),
     * sid_display_mode ("hex32|ecc-sid|hex16").
     */
    void applyCliOverrides(const QMap<QString, QVariant>& overrides);

    /**
     * @brief Validate ranges / known enum values.
     * @param[out] error Optional human-readable problem description.
     * @return true when the struct is sane, false otherwise.
     */
    bool validate(QString* error = nullptr) const;

    /** @brief Effective ECC for country resolution (row 6). */
    int effectiveEcc(int streamEcc) const noexcept {
        return ecc_override >= 0 ? ecc_override : streamEcc;
    }

    /** @brief Map FicModeSetting to the shared FicDecodeMode (row 4). */
    static FicDecodeMode toFicDecodeMode(FicModeSetting mode) noexcept {
        switch (mode) {
            case FicModeSetting::Strict: return FicDecodeMode::Strict;
            case FicModeSetting::Raw:    return FicDecodeMode::Raw;
            case FicModeSetting::Auto:
            default:                     return FicDecodeMode::AutoFallback;
        }
    }

    /** @brief String conversions used by QSettings/CLI (stable keys). */
    static QString charsetFallbackToString(CharsetFallbackPolicy p);
    static bool charsetFallbackFromString(const QString& s, CharsetFallbackPolicy& out);
    static QString etiModeToString(EtiModeSetting m);
    static bool etiModeFromString(const QString& s, EtiModeSetting& out);
    static QString ficModeToString(FicModeSetting m);
    static bool ficModeFromString(const QString& s, FicModeSetting& out);
    static QString labelSourceToString(LabelSourcePriority p);
    static bool labelSourceFromString(const QString& s, LabelSourcePriority& out);
    static QString timestampSourceToString(TimestampSource t);
    static bool timestampSourceFromString(const QString& s, TimestampSource& out);
    static QString sidDisplayModeToString(SidDisplayMode m);
    static bool sidDisplayModeFromString(const QString& s, SidDisplayMode& out);
};

} // namespace eti

#endif // STREAMDAB_ANALYSER_SETTINGS_HPP