/**
 * @file test_analyser_settings.cpp
 * @brief Unit tests for the user-selectable analyser decode options
 *        (option-variant matrix rows 1-6, 8, 10-12, 14).
 *
 * Covers:
 *  - defaults() byte-parity with the etisnoop-parity behavior (gate 2)
 *  - QSettings round-trip (group "analyser/")
 *  - CLI-flag -> AnalyserSettings mapping
 *  - behavior changes on synthetic data:
 *      force_charset overrides a FIG 1/1 label charset
 *      nst_offset_1 decodes the STC count-1 correction
 *      fib_ignore_crc recovers FIGs from a FIB with a bad CRC
 *      ecc_override changes the country lookup key
 *      sid_display_mode formats 0xF3200001 as hex32 / ecc-sid / hex16
 *      show_short_labels emits service_short_label
 *
 * @author C++ Qt Developer Agent (settings feature, v1.3)
 */

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <cstring>
#include <string>
#include <vector>

#include "core/analyser_settings.hpp"
#include "core/eti_types.hpp"
#include "core/modern_eti_frame_parser.hpp"
#include "core/crc16.hpp"
#include "core/etsi_registered_tables.hpp"
#include "cli/cli_argument_parser.hpp"
#include "cli/yaml_output_generator.hpp"
#include "utils/app_settings.hpp"

using namespace eti;

class TestAnalyserSettings : public QObject {
    Q_OBJECT

private slots:
    // ======================================================================
    // 1. Defaults must equal the current etisnoop-parity behavior (gate 2)
    // ======================================================================
    void defaultsMatchCurrentBehavior()
    {
        const AnalyserSettings d = AnalyserSettings::defaults();

        QCOMPARE(d.force_charset, -1);                                  // row 1: follow stream
        QCOMPARE(d.charset_fallback_policy, CharsetFallbackPolicy::Raw); // row 1: raw + warn-once
        QCOMPARE(d.eti_mode, EtiModeSetting::Auto);                     // row 2: sync detection
        QCOMPARE(d.nst_offset_1, false);                                // row 3: no count-1 correction
        QCOMPARE(d.fic_mode, FicModeSetting::Auto);                     // row 4: strict + auto fallback
        QCOMPARE(d.fib_ignore_crc, false);                              // row 5: drop bad FIBs
        QCOMPARE(d.ecc_override, -1);                                   // row 6: auto from stream
        QCOMPARE(d.service_ecc_deep_parse, false);                      // row 14: ensemble ECC only
        QCOMPARE(d.label_source_priority, LabelSourcePriority::Fig016First); // row 8
        QCOMPARE(d.show_short_labels, false);                           // row 8: no short labels
        QCOMPARE(d.timestamp_source, TimestampSource::Tist);            // row 10: tail TIST
        QCOMPARE(d.strict_frame_crc, false);                            // row 11: decode independent
        QCOMPARE(d.sid_display_mode, SidDisplayMode::Hex32);            // row 12: full hex

        // The default FIC decode mode maps to the shared auto-fallback mode.
        QCOMPARE(AnalyserSettings::toFicDecodeMode(FicModeSetting::Auto),
                 FicDecodeMode::AutoFallback);
        QCOMPARE(AnalyserSettings::toFicDecodeMode(FicModeSetting::Strict),
                 FicDecodeMode::Strict);
        QCOMPARE(AnalyserSettings::toFicDecodeMode(FicModeSetting::Raw),
                 FicDecodeMode::Raw);
    }

    void validateRejectsBadValues()
    {
        AnalyserSettings s = AnalyserSettings::defaults();
        QVERIFY(s.validate());

        s.force_charset = 2;  // only -1/0/3/6 allowed
        QString err;
        QVERIFY(!s.validate(&err));

        s = AnalyserSettings::defaults();
        s.ecc_override = 300;
        QVERIFY(!s.validate());
    }

    // ======================================================================
    // 2. QSettings round-trip (group "analyser/")
    // ======================================================================
    void qsettingsRoundTrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings s(dir.path() + QStringLiteral("/settings.ini"), QSettings::IniFormat);

        AnalyserSettings a = AnalyserSettings::defaults();
        a.force_charset = 6;
        a.charset_fallback_policy = CharsetFallbackPolicy::Skip;
        a.eti_mode = EtiModeSetting::Ni;
        a.nst_offset_1 = true;
        a.fic_mode = FicModeSetting::Strict;
        a.fib_ignore_crc = true;
        a.ecc_override = 0x20;
        a.service_ecc_deep_parse = true;
        a.label_source_priority = LabelSourcePriority::Fig11First;
        a.show_short_labels = true;
        a.timestamp_source = TimestampSource::Mtime;
        a.strict_frame_crc = true;
        a.sid_display_mode = SidDisplayMode::EccSid;
        a.saveToQSettings(s);
        s.sync();

        AnalyserSettings b = AnalyserSettings::defaults();
        b.loadFromQSettings(s);

        QCOMPARE(b.force_charset, 6);
        QCOMPARE(b.charset_fallback_policy, CharsetFallbackPolicy::Skip);
        QCOMPARE(b.eti_mode, EtiModeSetting::Ni);
        QCOMPARE(b.nst_offset_1, true);
        QCOMPARE(b.fic_mode, FicModeSetting::Strict);
        QCOMPARE(b.fib_ignore_crc, true);
        QCOMPARE(b.ecc_override, 0x20);
        QCOMPARE(b.service_ecc_deep_parse, true);
        QCOMPARE(b.label_source_priority, LabelSourcePriority::Fig11First);
        QCOMPARE(b.show_short_labels, true);
        QCOMPARE(b.timestamp_source, TimestampSource::Mtime);
        QCOMPARE(b.strict_frame_crc, true);
        QCOMPARE(b.sid_display_mode, SidDisplayMode::EccSid);
    }

    void qsettingsEmptyMeansDefaults()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings s(dir.path() + QStringLiteral("/empty.ini"), QSettings::IniFormat);

        AnalyserSettings b = AnalyserSettings::defaults();
        b.force_charset = 3;  // mutate, then prove a fresh load resets to defaults
        b.loadFromQSettings(s);

        const AnalyserSettings d = AnalyserSettings::defaults();
        QCOMPARE(b.force_charset, d.force_charset);
        QCOMPARE(b.fic_mode, d.fic_mode);
        QCOMPARE(b.sid_display_mode, d.sid_display_mode);
    }

    // ======================================================================
    // 3. CLI-flag -> AnalyserSettings mapping
    // ======================================================================
    void cliFlagMapping()
    {
        // Build a CLI parse with every analyser flag set.
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile input(dir.path() + "/x.eti");
        QVERIFY(input.open(QIODevice::WriteOnly));
        input.write("dummy");
        input.close();
        const QByteArray inputPath = input.fileName().toUtf8();

        const char* argv[] = {
            "streamdab-analyser",
            "--input", inputPath.constData(),
            "--force-charset", "3",
            "--charset-fallback", "skip",
            "--eti-mode", "ni",
            "--nst-offset-1",
            "--fic-mode", "strict",
            "--fib-ignore-crc",
            "--strict-frame-crc",
            "--ecc-override", "0x20",
            "--service-ecc-deep-parse",
            "--label-source", "fig11",
            "--show-short-labels",
            "--timestamp-source", "mtime",
            "--sid-display", "ecc-sid"
        };
        const int argc = static_cast<int>(sizeof(argv) / sizeof(argv[0]));

        cli::CLIOptions options = cli::CLIArgumentParser::parse(argc, const_cast<char**>(argv));

        QCOMPARE(options.force_charset, 3);
        QCOMPARE(options.charset_fallback_policy, QString("skip"));
        QCOMPARE(options.eti_mode, QString("ni"));
        QVERIFY(options.nst_offset_1);
        QCOMPARE(options.fic_mode, QString("strict"));
        QVERIFY(options.fib_ignore_crc);
        QVERIFY(options.strict_frame_crc);
        QCOMPARE(options.ecc_override, 0x20);
        QVERIFY(options.service_ecc_deep_parse);
        QCOMPARE(options.label_source_priority, QString("fig11"));
        QVERIFY(options.show_short_labels);
        QCOMPARE(options.timestamp_source, QString("mtime"));
        QCOMPARE(options.sid_display_mode, QString("ecc-sid"));

        AnalyserSettings s = AnalyserSettings::defaults();
        s.applyCliOverrides(options.analyserOverrides());

        QCOMPARE(s.force_charset, 3);
        QCOMPARE(s.charset_fallback_policy, CharsetFallbackPolicy::Skip);
        QCOMPARE(s.eti_mode, EtiModeSetting::Ni);
        QVERIFY(s.nst_offset_1);
        QCOMPARE(s.fic_mode, FicModeSetting::Strict);
        QVERIFY(s.fib_ignore_crc);
        QVERIFY(s.strict_frame_crc);
        QCOMPARE(s.ecc_override, 0x20);
        QVERIFY(s.service_ecc_deep_parse);
        QCOMPARE(s.label_source_priority, LabelSourcePriority::Fig11First);
        QVERIFY(s.show_short_labels);
        QCOMPARE(s.timestamp_source, TimestampSource::Mtime);
        QCOMPARE(s.sid_display_mode, SidDisplayMode::EccSid);
    }

    void cliNoFlagsKeepsDefaults()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile input(dir.path() + "/y.eti");
        QVERIFY(input.open(QIODevice::WriteOnly));
        input.write("dummy");
        input.close();
        const QByteArray inputPath = input.fileName().toUtf8();

        const char* argv[] = {"streamdab-analyser", "--input", inputPath.constData()};
        const int argc = 3;
        cli::CLIOptions options = cli::CLIArgumentParser::parse(argc, const_cast<char**>(argv));

        AnalyserSettings s = AnalyserSettings::defaults();
        s.force_charset = 6;  // ensure overrides never leak in
        s.applyCliOverrides(options.analyserOverrides());

        QCOMPARE(s.force_charset, 6);  // untouched: flag absent
        QCOMPARE(s.fic_mode, FicModeSetting::Auto);
        QCOMPARE(s.nst_offset_1, false);
    }

    // ======================================================================
    // 4. Behavior: force_charset override changes a FIG 1/1 label (row 1)
    // ======================================================================
    void forceCharsetChangesLabel()
    {
        // FIC area (32-byte legacy raw field at the normalized offset 12)
        // carrying one FIG 1/1 with charset 6 (Thai TIS-620) and Thai bytes.
        uint8_t frame[6144] = {0};
        uint8_t* fic = frame + 12;

        // FIG 1/1: header (type 1, length 21) + [charset/OE/ext][SId 2][label
        // 16][mask 2] = 22 bytes.
        uint8_t fig[22] = {0};
        fig[0] = static_cast<uint8_t>((1 << 5) | 21);
        fig[1] = static_cast<uint8_t>((6 << 4) | 0x01);   // charset 6, ext 1
        fig[2] = 0x2C;
        fig[3] = 0x61;                                    // SId 0x2C61
        // Thai label bytes: 0xA1 0xA2 0xA3 (TIS-620 ก ข ค), rest spaces.
        fig[4]  = 0xA1;
        fig[5]  = 0xA2;
        fig[6]  = 0xA3;
        for (int i = 7; i < 20; ++i) {
            fig[i] = 0x20;
        }
        fig[20] = 0xFF;                                   // mask: all bits
        fig[21] = 0xFF;
        std::memcpy(fic, fig, sizeof(fig));

        eti::EtiFrame ef;
        std::memcpy(ef.data(), frame, 6144);
        ef.fic_field_size = 32;  // legacy single-FIB view
        eti::modern::ETIParseResult pr;
        pr.has_fic = true;
        pr.detected_format = eti::ETIFormat::ETI_NI;

        // Default (follow stream): charset 6 -> Thai TIS-620 conversion.
        std::string defaultLabel = parseLabelFromFrame(ef, pr, AnalyserSettings::defaults());
        QVERIFY(!defaultLabel.empty());
        QVERIFY(QString::fromStdString(defaultLabel)
                    .contains(QString::fromUtf8("\u0E01\u0E02\u0E03")));  // ก ข ค

        // force_charset = 0 (EBU Latin): a different conversion of the same bytes.
        AnalyserSettings forced = AnalyserSettings::defaults();
        forced.force_charset = 0;
        std::string forcedLabel = parseLabelFromFrame(ef, pr, forced);
        QVERIFY(!forcedLabel.empty());
        QVERIFY(forcedLabel != defaultLabel);
    }

    void charsetFallbackSkipDropsLabel()
    {
        uint8_t frame[6144] = {0};
        uint8_t* fic = frame + 12;
        uint8_t fig[22] = {0};
        fig[0] = static_cast<uint8_t>((1 << 5) | 21);
        fig[1] = static_cast<uint8_t>((9 << 4) | 0x01);   // unknown charset 9
        fig[2] = 0x25;
        fig[3] = 0x00;
        for (int i = 4; i < 20; ++i) {
            fig[i] = 0x20;
        }
        fig[4] = 'X';
        fig[20] = 0xFF;
        fig[21] = 0xFF;
        std::memcpy(fic, fig, sizeof(fig));

        eti::EtiFrame ef;
        std::memcpy(ef.data(), frame, 6144);
        ef.fic_field_size = 32;
        eti::modern::ETIParseResult pr;
        pr.has_fic = true;
        pr.detected_format = eti::ETIFormat::ETI_NI;

        std::string rawLabel = parseLabelFromFrame(ef, pr, AnalyserSettings::defaults());
        QVERIFY(!rawLabel.empty());   // raw passthrough (default)

        AnalyserSettings skip = AnalyserSettings::defaults();
        skip.charset_fallback_policy = CharsetFallbackPolicy::Skip;
        std::string skipLabel = parseLabelFromFrame(ef, pr, skip);
        QVERIFY(skipLabel.empty());
    }

    // ======================================================================
    // 5. Behavior: nst_offset_1 decodes the count-1 correction (row 3)
    // ======================================================================
    void nstOffset1DecodesExtraStream()
    {
        // ETI-NI frame whose NST field says 16 but actually carries 17 STC
        // entries (legacy count-1 mux).
        uint8_t frame[6144] = {0};
        frame[0] = 0xFF;
        frame[1] = 0xF8;
        frame[4] = 0x00;   // FCT
        frame[5] = 16;     // FICF=0, NST=16 (real streams = 17)

        // Fill 17 STC entries (4 bytes each from offset 8).
        for (int i = 0; i < 17; ++i) {
            const size_t off = 8 + static_cast<size_t>(i) * 4;
            frame[off] = static_cast<uint8_t>(i << 2);  // SCID = i
            frame[off + 1] = 0;
            frame[off + 2] = 0;
            frame[off + 3] = 0;
        }

        eti::modern::ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;

        // Default: only the 16 streams encoded in the field are parsed.
        eti::modern::ModernETIFrameParser parserDefault;
        QVERIFY(parserDefault.initialize(config));
        auto rDefault = parserDefault.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(rDefault.success);
        QCOMPARE(static_cast<int>(rDefault.subchannels_from_stc.size()), 16);

        // nst_offset_1: the missing 17th stream is recovered.
        AnalyserSettings settings = AnalyserSettings::defaults();
        settings.nst_offset_1 = true;
        eti::modern::ModernETIFrameParser parserOffset;
        QVERIFY(parserOffset.initialize(config));
        parserOffset.setAnalyserSettings(settings);
        auto rOffset = parserOffset.parseFrame(std::span<const uint8_t, 6144>{frame});
        QVERIFY(rOffset.success);
        QCOMPARE(static_cast<int>(rOffset.subchannels_from_stc.size()), 17);
    }

    // ======================================================================
    // 6. Behavior: fib_ignore_crc recovers FIGs from a bad-CRC FIB (row 5)
    // ======================================================================
    void fibIgnoreCrcRecoversFigBlocks()
    {
        // 96-byte FIB-structured FIC: 3 FIBs, all CRCs deliberately wrong.
        std::array<uint8_t, 96> fic{};
        // FIB 0 carries one FIG 0/0 (ensemble info) then padding.
        uint8_t fig00[7] = {0};
        fig00[0] = static_cast<uint8_t>((0 << 5) | 6);  // type 0, length 6
        fig00[1] = 0x00;   // ext
        fig00[2] = 0x20;   // EId 0x2000
        fig00[3] = 0x00;
        fig00[4] = 0x00;
        fig00[5] = 0x00;
        fig00[6] = 0x00;
        std::memcpy(fic.data(), fig00, sizeof(fig00));
        // FIBs 1-2 all-0xFF padding -> type 7 break in their own areas.
        for (size_t i = 32; i < fic.size(); ++i) {
            fic[i] = 0xFF;
        }
        // Wrong CRCs everywhere (0x0000 instead of the computed value).
        for (size_t fib = 0; fib < 3; ++fib) {
            fic[fib * 32 + 30] = 0x00;
            fic[fib * 32 + 31] = 0x00;
        }

        eti::EtiFicField field(fic.data(), fic.size());

        // Strict + drop (default strict): nothing survives the CRC gate.
        auto strictResult = field.decodeFigBlocks(eti::FicDecodeMode::Strict, false);
        QCOMPARE(static_cast<int>(strictResult.fib_crc_failures), 3);
        QCOMPARE(static_cast<int>(strictResult.fig_blocks.size()), 0);
        QVERIFY(!strictResult.raw_fallback_used);

        // Strict + ignore CRC: the FIG is recovered despite the failure.
        auto ignoreResult = field.decodeFigBlocks(eti::FicDecodeMode::Strict, true);
        QCOMPARE(static_cast<int>(ignoreResult.fib_crc_failures), 3);
        QCOMPARE(static_cast<int>(ignoreResult.fig_blocks.size()), 1);
        QCOMPARE(static_cast<int>(ignoreResult.fig_blocks[0].fig_type), 0);
        QCOMPARE(static_cast<int>(ignoreResult.fig_blocks[0].get_extension()), 0);

        // Auto (default): all-FIB-fail triggers the raw fallback (row 4).
        auto autoResult = field.decodeFigBlocks(eti::FicDecodeMode::AutoFallback, false);
        QVERIFY(autoResult.raw_fallback_used);
        QVERIFY(!autoResult.fig_blocks.empty());
    }

    // ======================================================================
    // 7. Behavior: ecc_override changes the country lookup key (row 6)
    // ======================================================================
    void eccOverrideChangesCountryKey()
    {
        // TS 101 756: (0xF3, 0x02) -> Thailand (the fixture).
        QVERIFY(QString::fromLatin1(etsi::ts101756::getCountryName(0xF3, 0x02)) == "Thailand");
        // Manual ECC 0x20 must not resolve to Thailand for the same CId.
        QVERIFY(QString::fromLatin1(etsi::ts101756::getCountryName(0x20, 0x02)) != "Thailand");

        AnalyserSettings s = AnalyserSettings::defaults();
        QCOMPARE(s.effectiveEcc(0xF3), 0xF3);   // no override -> stream value
        s.ecc_override = 0x20;
        QCOMPARE(s.effectiveEcc(0xF3), 0x20);   // override wins

        // End-to-end through the YAML layer: the override changes country_name.
        cli::YAMLOutputGenerator y;
        eti::AnalyserSettings a = AnalyserSettings::defaults();
        a.ecc_override = 0x20;
        y.setAnalyserSettings(a);

        eti::DabService service;
        service.service_id = 0x2C61;
        service.sid32 = 0;
        service.extended_country_code = 0xF3;   // stream says Thailand
        service.country_id = 0x02;
        service.is_programme = true;
        service.label = "RROne FM 101";
        y.addService(service);

        const QString yaml = y.generateYAML();
        QVERIFY(yaml.contains(QStringLiteral("country_name: \"Unknown\"")));
        QVERIFY(!yaml.contains(QStringLiteral("country_name: \"Thailand\"")));
    }

    // ======================================================================
    // 8. Behavior: sid_display_mode formats 0xF3200001 (row 12)
    // ======================================================================
    void sidDisplayModes()
    {
        // --- hex32 (default) ---
        {
            cli::YAMLOutputGenerator y;
            y.setAnalyserSettings(AnalyserSettings::defaults());
            y.addService(makeDataService());
            QVERIFY(y.generateYAML().contains(QStringLiteral("service_id: 0xF3200001")));
        }
        // --- ecc-sid: ECC:CId:SRef decomposition ---
        {
            cli::YAMLOutputGenerator y;
            eti::AnalyserSettings a = AnalyserSettings::defaults();
            a.sid_display_mode = SidDisplayMode::EccSid;
            y.setAnalyserSettings(a);
            y.addService(makeDataService());
            QVERIFY(y.generateYAML().contains(QStringLiteral("service_id: \"F3:2:00001\"")));
        }
        // --- hex16: lower 16 bits ---
        {
            cli::YAMLOutputGenerator y;
            eti::AnalyserSettings a = AnalyserSettings::defaults();
            a.sid_display_mode = SidDisplayMode::Hex16;
            y.setAnalyserSettings(a);
            y.addService(makeDataService());
            QVERIFY(y.generateYAML().contains(QStringLiteral("service_id: 0x0001")));
        }
        // --- ecc-sid for a 16-bit programme service ---
        {
            cli::YAMLOutputGenerator y;
            eti::AnalyserSettings a = AnalyserSettings::defaults();
            a.sid_display_mode = SidDisplayMode::EccSid;
            y.setAnalyserSettings(a);

            eti::DabService prog;
            prog.service_id = 0x2C61;
            prog.sid32 = 0;
            prog.extended_country_code = 0xF3;
            prog.country_id = 0x02;
            prog.is_programme = true;
            prog.label = "RROne FM 101";
            y.addService(prog);
            QVERIFY(y.generateYAML().contains(QStringLiteral("service_id: \"F3:2:2C61\"")));
        }
    }

    // ======================================================================
    // 9. Behavior: show_short_labels emits service_short_label (row 8)
    // ======================================================================
    void shortLabelFromMask()
    {
        // Synthetic FIG 1/1: label "ABCDEFGHIJKLMNOP", mask 0xF0F0.
        // etisnoop shortlabel(): select bytes where (mask & (0x8000 >> i)).
        // 0xF0F0 selects i = 0..3 and 8..11 -> "ABCD" + "IJKL" = "ABCDIJKL".
        uint8_t frame[6144] = {0};
        uint8_t* fic = frame + 12;
        uint8_t fig[22] = {0};
        const char* labelBytes = "ABCDEFGHIJKLMNOP";
        fig[0] = static_cast<uint8_t>((1 << 5) | 21);
        fig[1] = static_cast<uint8_t>((3 << 4) | 0x01);   // charset 3 (UTF-8)
        fig[2] = 0x2C;
        fig[3] = 0x61;
        std::memcpy(fig + 4, labelBytes, 16);
        fig[20] = 0xF0;
        fig[21] = 0xF0;
        std::memcpy(fic, fig, sizeof(fig));

        eti::EtiFrame ef;
        std::memcpy(ef.data(), frame, 6144);
        ef.fic_field_size = 32;
        eti::modern::ETIParseResult pr;
        pr.has_fic = true;
        pr.detected_format = eti::ETIFormat::ETI_NI;

        eti::modern::ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;

        eti::modern::ModernETIFrameParser parser;
        QVERIFY(parser.initialize(config));
        auto figResult = parser.analyzeFIGData(ef, pr);

        QVERIFY(!figResult.discovered_services.empty());
        const auto& svc = figResult.discovered_services.front();
        QCOMPARE(QString::fromStdString(svc.label), QString("ABCDEFGHIJKLMNOP"));
        QCOMPARE(svc.character_flag, static_cast<uint16_t>(0xF0F0));
        QCOMPARE(QString::fromStdString(svc.short_label), QString("ABCDIJKL"));

        // The YAML layer emits the short label only when the setting is on.
        cli::YAMLOutputGenerator y;
        y.setAnalyserSettings(AnalyserSettings::defaults());  // off
        y.addService(svc);
        QVERIFY(!y.generateYAML().contains(QStringLiteral("service_short_label")));

        cli::YAMLOutputGenerator y2;
        eti::AnalyserSettings a = AnalyserSettings::defaults();
        a.show_short_labels = true;
        y2.setAnalyserSettings(a);
        y2.addService(svc);
        QVERIFY(y2.generateYAML().contains(
            QStringLiteral("service_short_label: \"ABCDIJKL\"")));
    }

    // ======================================================================
    // 5. QSettings scope migration (old American spelling -> "Analyser")
    // ======================================================================
    void legacyScopeMigratesOnce()
    {
        QStandardPaths::setTestModeEnabled(true);
        using namespace streamdab::app_settings;

        // Start from a clean slate for both scopes.
        { QSettings n(organization(), application()); n.clear(); n.sync(); }
        { QSettings o(legacyOrganization(), legacyApplication()); o.clear(); o.sync(); }

        // Seed the legacy (pre-rename) scope.
        {
            QSettings o(legacyOrganization(), legacyApplication());
            o.setValue(QStringLiteral("docking/tab1"), QByteArray("layout"));
            o.setValue(streamdab::app_settings::legacy::settingsGroup()
                           + QStringLiteral("/force_charset"),
                       6);
            o.setValue(QStringLiteral("window/geometry"), QByteArray("geom"));
            o.sync();
        }

        QVERIFY(migrateLegacyScope());  // migration performed

        {
            QSettings n(organization(), application());
            QCOMPARE(n.value(QStringLiteral("docking/tab1")).toByteArray(), QByteArray("layout"));
            QCOMPARE(n.value(QStringLiteral("window/geometry")).toByteArray(), QByteArray("geom"));
            // The old key group is renamed to the Analyser spelling.
            QCOMPARE(n.value(QStringLiteral("analyser/force_charset")).toInt(), 6);
            QVERIFY(n.value(streamdab::app_settings::legacy::settingsGroup()
                                + QStringLiteral("/force_charset"))
                        .isNull());
        }
        {
            QSettings o(legacyOrganization(), legacyApplication());
            o.setFallbacksEnabled(false);  // macOS: ignore the global domain
            QVERIFY(o.allKeys().isEmpty());  // legacy scope removed
        }

        // Idempotent: the legacy scope is now empty -> no-op.
        QVERIFY(!migrateLegacyScope());
    }

    void legacyScopeNotOverwrittenWhenNewScopeInUse()
    {
        QStandardPaths::setTestModeEnabled(true);
        using namespace streamdab::app_settings;

        { QSettings n(organization(), application()); n.clear(); n.sync(); }
        { QSettings o(legacyOrganization(), legacyApplication()); o.clear(); o.sync(); }

        // New scope already carries a persisted docking layout.
        {
            QSettings n(organization(), application());
            n.setValue(QStringLiteral("docking/layout_version"), 8);
            n.setValue(QStringLiteral("window/geometry"), QByteArray("new"));
            n.sync();
        }
        // Legacy scope still holds stale data.
        {
            QSettings o(legacyOrganization(), legacyApplication());
            o.setValue(QStringLiteral("window/geometry"), QByteArray("old"));
            o.sync();
        }

        QVERIFY(!migrateLegacyScope());  // skipped: new scope already in use

        {
            QSettings n(organization(), application());
            QCOMPARE(n.value(QStringLiteral("window/geometry")).toByteArray(), QByteArray("new"));
        }
        {
            QSettings o(legacyOrganization(), legacyApplication());
            QVERIFY(!o.allKeys().isEmpty());  // legacy scope left untouched
            o.clear();
            o.sync();
        }
        { QSettings n(organization(), application()); n.clear(); n.sync(); }
    }

    // MEDIUM fix regression: new scope has SOME data but no docking/layout_version
    // (abnormal exit before the dock state was saved). Migration must copy the
    // legacy keys but NEVER overwrite the newer new-scope values.
    void legacyScopeDoesNotOverwriteExistingNewKeysWithoutLayoutVersion()
    {
        QStandardPaths::setTestModeEnabled(true);
        using namespace streamdab::app_settings;

        { QSettings n(organization(), application()); n.clear(); n.sync(); }
        { QSettings o(legacyOrganization(), legacyApplication()); o.clear(); o.sync(); }

        // New scope: a partial store (e.g. a Settings-dialog edit), no layout_version.
        {
            QSettings n(organization(), application());
            n.setValue(QStringLiteral("window/geometry"), QByteArray("new-geom"));
            n.setValue(QStringLiteral("analyser/force_charset"), 7);
            n.sync();
        }
        // Legacy scope: stale values for the colliding keys + one missing key.
        {
            QSettings o(legacyOrganization(), legacyApplication());
            o.setValue(QStringLiteral("window/geometry"), QByteArray("old-geom"));
            o.setValue(streamdab::app_settings::legacy::settingsGroup()
                           + QStringLiteral("/force_charset"),
                       3);
            o.setValue(QStringLiteral("docking/tab1"), QByteArray("layout"));
            o.sync();
        }

        QVERIFY(migrateLegacyScope());  // partial new scope -> migration runs

        {
            QSettings n(organization(), application());
            // Colliding keys keep the NEWER new-scope values.
            QCOMPARE(n.value(QStringLiteral("window/geometry")).toByteArray(), QByteArray("new-geom"));
            QCOMPARE(n.value(QStringLiteral("analyser/force_charset")).toInt(), 7);
            // The missing key is migrated in.
            QCOMPARE(n.value(QStringLiteral("docking/tab1")).toByteArray(), QByteArray("layout"));
        }
        {
            QSettings o(legacyOrganization(), legacyApplication());
            o.setFallbacksEnabled(false);  // macOS: ignore the global domain
            QVERIFY(o.allKeys().isEmpty());  // legacy scope removed after copy
        }
        { QSettings n(organization(), application()); n.clear(); n.sync(); }
    }

private:
    eti::DabService makeDataService() const
    {
        eti::DabService service;
        service.service_id = 0x0001;            // SRef low 16 bits
        service.sid32 = 0xF3200001;             // ECC F3, CId 2, SRef 0x00001
        service.extended_country_code = 0xF3;
        service.country_id = 0x02;
        service.is_programme = false;
        service.label = "data1";
        return service;
    }

    std::string parseLabelFromFrame(const eti::EtiFrame& ef,
                                    const eti::modern::ETIParseResult& pr,
                                    const AnalyserSettings& settings)
    {
        eti::modern::ProcessingConfig config;
        config.enable_threading = false;
        config.enable_caching = false;
        eti::modern::ModernETIFrameParser parser;
        if (!parser.initialize(config)) {
            return std::string();
        }
        parser.setAnalyserSettings(settings);
        auto figResult = parser.analyzeFIGData(ef, pr);
        if (figResult.discovered_services.empty()) {
            return std::string();
        }
        return figResult.discovered_services.front().label;
    }
};

QTEST_MAIN(TestAnalyserSettings)
#include "test_analyser_settings.moc"