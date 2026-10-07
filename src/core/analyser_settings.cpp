/**
 * @file analyser_settings.cpp
 * @brief Implementation of the user-selectable analyser decode options.
 *
 * @author C++ Qt Developer Agent (settings feature, v1.3)
 */

#include "analyser_settings.hpp"
#include "utils/app_settings.hpp"
#include <QSettings>
#include <QStringList>

namespace eti {

AnalyserSettings AnalyserSettings::defaults()
{
    // Values must equal the current etisnoop-parity behavior (gate 2). The
    // brace-init here mirrors the member defaults; keep them in sync.
    AnalyserSettings s;
    s.force_charset = -1;
    s.charset_fallback_policy = CharsetFallbackPolicy::Raw;
    s.eti_mode = EtiModeSetting::Auto;
    s.nst_offset_1 = false;
    s.fic_mode = FicModeSetting::Auto;
    s.fib_ignore_crc = false;
    s.ecc_override = -1;
    s.service_ecc_deep_parse = false;
    s.label_source_priority = LabelSourcePriority::Fig016First;
    s.show_short_labels = false;
    s.timestamp_source = TimestampSource::Tist;
    s.strict_frame_crc = false;
    s.sid_display_mode = SidDisplayMode::Hex32;
    return s;
}

// ---------------------------------------------------------------------------
// String conversions (stable keys shared by QSettings + CLI overrides)
// ---------------------------------------------------------------------------

QString AnalyserSettings::charsetFallbackToString(CharsetFallbackPolicy p)
{
    switch (p) {
        case CharsetFallbackPolicy::ForceUtf8: return QStringLiteral("utf8");
        case CharsetFallbackPolicy::Skip:      return QStringLiteral("skip");
        case CharsetFallbackPolicy::Raw:
        default:                               return QStringLiteral("raw");
    }
}

bool AnalyserSettings::charsetFallbackFromString(const QString& s, CharsetFallbackPolicy& out)
{
    const QString v = s.trimmed().toLower();
    if (v == QLatin1String("utf8") || v == QLatin1String("utf-8")) {
        out = CharsetFallbackPolicy::ForceUtf8;
        return true;
    }
    if (v == QLatin1String("skip")) {
        out = CharsetFallbackPolicy::Skip;
        return true;
    }
    if (v == QLatin1String("raw")) {
        out = CharsetFallbackPolicy::Raw;
        return true;
    }
    return false;
}

QString AnalyserSettings::etiModeToString(EtiModeSetting m)
{
    switch (m) {
        case EtiModeSetting::Li:   return QStringLiteral("li");
        case EtiModeSetting::Ni:   return QStringLiteral("ni");
        case EtiModeSetting::Auto:
        default:                   return QStringLiteral("auto");
    }
}

bool AnalyserSettings::etiModeFromString(const QString& s, EtiModeSetting& out)
{
    const QString v = s.trimmed().toLower();
    if (v == QLatin1String("li")) { out = EtiModeSetting::Li; return true; }
    if (v == QLatin1String("ni")) { out = EtiModeSetting::Ni; return true; }
    if (v == QLatin1String("auto")) { out = EtiModeSetting::Auto; return true; }
    return false;
}

QString AnalyserSettings::ficModeToString(FicModeSetting m)
{
    switch (m) {
        case FicModeSetting::Strict: return QStringLiteral("strict");
        case FicModeSetting::Raw:    return QStringLiteral("raw");
        case FicModeSetting::Auto:
        default:                     return QStringLiteral("auto");
    }
}

bool AnalyserSettings::ficModeFromString(const QString& s, FicModeSetting& out)
{
    const QString v = s.trimmed().toLower();
    if (v == QLatin1String("strict")) { out = FicModeSetting::Strict; return true; }
    if (v == QLatin1String("raw"))    { out = FicModeSetting::Raw;    return true; }
    if (v == QLatin1String("auto"))   { out = FicModeSetting::Auto;   return true; }
    return false;
}

QString AnalyserSettings::labelSourceToString(LabelSourcePriority p)
{
    switch (p) {
        case LabelSourcePriority::Fig11First: return QStringLiteral("fig11");
        case LabelSourcePriority::Fig016First:
        default:                              return QStringLiteral("fig016");
    }
}

bool AnalyserSettings::labelSourceFromString(const QString& s, LabelSourcePriority& out)
{
    const QString v = s.trimmed().toLower();
    if (v == QLatin1String("fig11") || v == QLatin1String("1/1")) {
        out = LabelSourcePriority::Fig11First;
        return true;
    }
    if (v == QLatin1String("fig016") || v == QLatin1String("0/16") ||
        v == QLatin1String("fig0_16") || v == QLatin1String("fig16")) {
        out = LabelSourcePriority::Fig016First;
        return true;
    }
    return false;
}

QString AnalyserSettings::timestampSourceToString(TimestampSource t)
{
    switch (t) {
        case TimestampSource::Mtime: return QStringLiteral("mtime");
        case TimestampSource::Tist:
        default:                     return QStringLiteral("tist");
    }
}

bool AnalyserSettings::timestampSourceFromString(const QString& s, TimestampSource& out)
{
    const QString v = s.trimmed().toLower();
    if (v == QLatin1String("mtime")) { out = TimestampSource::Mtime; return true; }
    if (v == QLatin1String("tist"))  { out = TimestampSource::Tist;  return true; }
    return false;
}

QString AnalyserSettings::sidDisplayModeToString(SidDisplayMode m)
{
    switch (m) {
        case SidDisplayMode::EccSid: return QStringLiteral("ecc-sid");
        case SidDisplayMode::Hex16:  return QStringLiteral("hex16");
        case SidDisplayMode::Hex32:
        default:                     return QStringLiteral("hex32");
    }
}

bool AnalyserSettings::sidDisplayModeFromString(const QString& s, SidDisplayMode& out)
{
    const QString v = s.trimmed().toLower();
    if (v == QLatin1String("ecc-sid") || v == QLatin1String("eccsid") ||
        v == QLatin1String("ecc_sid")) {
        out = SidDisplayMode::EccSid;
        return true;
    }
    if (v == QLatin1String("hex16")) { out = SidDisplayMode::Hex16; return true; }
    if (v == QLatin1String("hex32")) { out = SidDisplayMode::Hex32; return true; }
    return false;
}

// ---------------------------------------------------------------------------
// QSettings persistence  (org "StreamDAB-Analyser", app "DABAnalyser",
// group "analyser/")
// ---------------------------------------------------------------------------

void AnalyserSettings::saveToQSettings(QSettings& s) const
{
    s.beginGroup(QStringLiteral("analyser"));
    s.setValue(QStringLiteral("force_charset"), force_charset);
    s.setValue(QStringLiteral("charset_fallback_policy"),
               charsetFallbackToString(charset_fallback_policy));
    s.setValue(QStringLiteral("eti_mode"), etiModeToString(eti_mode));
    s.setValue(QStringLiteral("nst_offset_1"), nst_offset_1);
    s.setValue(QStringLiteral("fic_mode"), ficModeToString(fic_mode));
    s.setValue(QStringLiteral("fib_ignore_crc"), fib_ignore_crc);
    s.setValue(QStringLiteral("ecc_override"), ecc_override);
    s.setValue(QStringLiteral("service_ecc_deep_parse"), service_ecc_deep_parse);
    s.setValue(QStringLiteral("label_source_priority"),
               labelSourceToString(label_source_priority));
    s.setValue(QStringLiteral("show_short_labels"), show_short_labels);
    s.setValue(QStringLiteral("timestamp_source"), timestampSourceToString(timestamp_source));
    s.setValue(QStringLiteral("strict_frame_crc"), strict_frame_crc);
    s.setValue(QStringLiteral("sid_display_mode"), sidDisplayModeToString(sid_display_mode));
    s.endGroup();
}

void AnalyserSettings::loadFromQSettings(QSettings& s)
{
    *this = defaults();

    s.beginGroup(QStringLiteral("analyser"));
    force_charset = s.value(QStringLiteral("force_charset"), force_charset).toInt();

    QString v = s.value(QStringLiteral("charset_fallback_policy"),
                        charsetFallbackToString(charset_fallback_policy)).toString();
    CharsetFallbackPolicy fallback;
    if (charsetFallbackFromString(v, fallback)) {
        charset_fallback_policy = fallback;
    }

    v = s.value(QStringLiteral("eti_mode"), etiModeToString(eti_mode)).toString();
    EtiModeSetting eti;
    if (etiModeFromString(v, eti)) {
        eti_mode = eti;
    }

    nst_offset_1 = s.value(QStringLiteral("nst_offset_1"), nst_offset_1).toBool();

    v = s.value(QStringLiteral("fic_mode"), ficModeToString(fic_mode)).toString();
    FicModeSetting fic;
    if (ficModeFromString(v, fic)) {
        fic_mode = fic;
    }

    fib_ignore_crc = s.value(QStringLiteral("fib_ignore_crc"), fib_ignore_crc).toBool();
    ecc_override = s.value(QStringLiteral("ecc_override"), ecc_override).toInt();
    service_ecc_deep_parse =
        s.value(QStringLiteral("service_ecc_deep_parse"), service_ecc_deep_parse).toBool();

    v = s.value(QStringLiteral("label_source_priority"),
                labelSourceToString(label_source_priority)).toString();
    LabelSourcePriority prio;
    if (labelSourceFromString(v, prio)) {
        label_source_priority = prio;
    }

    show_short_labels = s.value(QStringLiteral("show_short_labels"), show_short_labels).toBool();

    v = s.value(QStringLiteral("timestamp_source"),
                timestampSourceToString(timestamp_source)).toString();
    TimestampSource ts;
    if (timestampSourceFromString(v, ts)) {
        timestamp_source = ts;
    }

    strict_frame_crc = s.value(QStringLiteral("strict_frame_crc"), strict_frame_crc).toBool();

    v = s.value(QStringLiteral("sid_display_mode"),
                sidDisplayModeToString(sid_display_mode)).toString();
    SidDisplayMode sid;
    if (sidDisplayModeFromString(v, sid)) {
        sid_display_mode = sid;
    }

    s.endGroup();
}

void AnalyserSettings::saveToQSettings() const
{
    QSettings s(streamdab::app_settings::organization(),
                streamdab::app_settings::application());
    saveToQSettings(s);
}

void AnalyserSettings::loadFromQSettings()
{
    QSettings s(streamdab::app_settings::organization(),
                streamdab::app_settings::application());
    loadFromQSettings(s);
}

// ---------------------------------------------------------------------------
// CLI overrides
// ---------------------------------------------------------------------------

void AnalyserSettings::applyCliOverrides(const QMap<QString, QVariant>& overrides)
{
    auto it = overrides.constFind(QStringLiteral("force_charset"));
    if (it != overrides.constEnd()) {
        const int v = it->toInt();
        if (v >= -1 && v <= 15) {
            force_charset = v;
        }
    }

    it = overrides.constFind(QStringLiteral("charset_fallback_policy"));
    if (it != overrides.constEnd()) {
        CharsetFallbackPolicy p;
        if (charsetFallbackFromString(it->toString(), p)) {
            charset_fallback_policy = p;
        }
    }

    it = overrides.constFind(QStringLiteral("eti_mode"));
    if (it != overrides.constEnd()) {
        EtiModeSetting m;
        if (etiModeFromString(it->toString(), m)) {
            eti_mode = m;
        }
    }

    it = overrides.constFind(QStringLiteral("nst_offset_1"));
    if (it != overrides.constEnd()) {
        nst_offset_1 = it->toBool();
    }

    it = overrides.constFind(QStringLiteral("fic_mode"));
    if (it != overrides.constEnd()) {
        FicModeSetting m;
        if (ficModeFromString(it->toString(), m)) {
            fic_mode = m;
        }
    }

    it = overrides.constFind(QStringLiteral("fib_ignore_crc"));
    if (it != overrides.constEnd()) {
        fib_ignore_crc = it->toBool();
    }

    it = overrides.constFind(QStringLiteral("strict_frame_crc"));
    if (it != overrides.constEnd()) {
        strict_frame_crc = it->toBool();
    }

    it = overrides.constFind(QStringLiteral("ecc_override"));
    if (it != overrides.constEnd()) {
        const int v = it->toInt();
        if (v >= 0 && v <= 255) {
            ecc_override = v;
        }
    }

    it = overrides.constFind(QStringLiteral("service_ecc_deep_parse"));
    if (it != overrides.constEnd()) {
        service_ecc_deep_parse = it->toBool();
    }

    it = overrides.constFind(QStringLiteral("label_source_priority"));
    if (it != overrides.constEnd()) {
        LabelSourcePriority p;
        if (labelSourceFromString(it->toString(), p)) {
            label_source_priority = p;
        }
    }

    it = overrides.constFind(QStringLiteral("show_short_labels"));
    if (it != overrides.constEnd()) {
        show_short_labels = it->toBool();
    }

    it = overrides.constFind(QStringLiteral("timestamp_source"));
    if (it != overrides.constEnd()) {
        TimestampSource t;
        if (timestampSourceFromString(it->toString(), t)) {
            timestamp_source = t;
        }
    }

    it = overrides.constFind(QStringLiteral("sid_display_mode"));
    if (it != overrides.constEnd()) {
        SidDisplayMode m;
        if (sidDisplayModeFromString(it->toString(), m)) {
            sid_display_mode = m;
        }
    }
}

bool AnalyserSettings::validate(QString* error) const
{
    if (force_charset != -1 && force_charset != 0 && force_charset != 3 &&
        force_charset != 6) {
        if (error) {
            *error = QStringLiteral("force_charset must be -1, 0, 3 or 6 (got %1)")
                         .arg(force_charset);
        }
        return false;
    }
    if (ecc_override < -1 || ecc_override > 255) {
        if (error) {
            *error = QStringLiteral("ecc_override must be -1..255 (got %1)").arg(ecc_override);
        }
        return false;
    }
    return true;
}

} // namespace eti