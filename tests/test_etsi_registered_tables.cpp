/**
 * @file test_etsi_registered_tables.cpp
 * @brief Comprehensive tests for ETSI TS 101 756 Registered Tables
 *
 * Tests all registered tables:
 * - Extended Country Code (ECC) table
 * - Language code table (ISO 639-2 mapping)
 * - Programme Type table (32 international types)
 *
 * @version 1.1.0
 * @date October 2025
 */

#include <QtTest/QtTest>
#include "core/etsi_registered_tables.hpp"
#include <chrono>

using namespace etsi::ts101756;

class TestETSIRegisteredTables : public QObject {
    Q_OBJECT

private slots:
    // ========================================================================
    // Extended Country Code (ECC) Tests
    // ========================================================================

    void test_ecc_thailand_lookup() {
        // CRITICAL: Thailand (ECC=0xF3, Country ID=0x02)
        // Verified against the Bangkok DAB+ ensemble (FIG 0/9 "Ensemble ECC: 0xF3")
        const auto* thailand = lookupCountry(0xF3, 0x02);

        QVERIFY(thailand != nullptr);
        QCOMPARE(thailand->ecc, static_cast<uint8_t>(0xF3));
        QCOMPARE(thailand->country_id, static_cast<uint8_t>(0x02));
        QCOMPARE(QString(thailand->country_name), QString("Thailand"));
        QCOMPARE(QString(thailand->iso_3166_1_alpha2), QString("TH"));

        // Test unique ID generation (12-bit)
        uint16_t unique_id = thailand->getUniqueID();
        QCOMPARE(unique_id, static_cast<uint16_t>(0xF32));  // 0xF3 << 4 | 0x02
    }

    void test_ecc_germany_lookup() {
        // Germany (ECC=0xE1, Country ID=0x01)
        const auto* germany = lookupCountry(0xE1, 0x01);

        QVERIFY(germany != nullptr);
        QCOMPARE(germany->ecc, static_cast<uint8_t>(0xE1));
        QCOMPARE(germany->country_id, static_cast<uint8_t>(0x01));
        QCOMPARE(QString(germany->country_name), QString("Germany"));
        QCOMPARE(QString(germany->iso_3166_1_alpha2), QString("DE"));
    }

    void test_ecc_france_lookup() {
        // France (ECC=0xE2, Country ID=0x0F)
        const auto* france = lookupCountry(0xE2, 0x0F);

        QVERIFY(france != nullptr);
        QCOMPARE(france->ecc, static_cast<uint8_t>(0xE2));
        QCOMPARE(france->country_id, static_cast<uint8_t>(0x0F));
        QCOMPARE(QString(france->country_name), QString("France"));
        QCOMPARE(QString(france->iso_3166_1_alpha2), QString("FR"));
    }

    void test_ecc_uk_lookup() {
        // United Kingdom (ECC=0xE3, Country ID=0x0C)
        const auto* uk = lookupCountry(0xE3, 0x0C);

        QVERIFY(uk != nullptr);
        QCOMPARE(uk->ecc, static_cast<uint8_t>(0xE3));
        QCOMPARE(uk->country_id, static_cast<uint8_t>(0x0C));
        QCOMPARE(QString(uk->country_name), QString("United Kingdom"));
        QCOMPARE(QString(uk->iso_3166_1_alpha2), QString("GB"));
    }

    void test_ecc_italy_lookup() {
        // Italy (ECC=0xE4, Country ID=0x05)
        const auto* italy = lookupCountry(0xE4, 0x05);

        QVERIFY(italy != nullptr);
        QCOMPARE(italy->ecc, static_cast<uint8_t>(0xE4));
        QCOMPARE(italy->country_id, static_cast<uint8_t>(0x05));
        QCOMPARE(QString(italy->country_name), QString("Italy"));
        QCOMPARE(QString(italy->iso_3166_1_alpha2), QString("IT"));
    }

    void test_ecc_invalid_lookup() {
        // Test invalid/reserved ECC combination
        const auto* invalid = lookupCountry(0xFF, 0x0F);

        // Should return reserved entry or nullptr behavior
        // (Current implementation returns reserved entry)
        if (invalid) {
            QCOMPARE(QString(invalid->country_name), QString("Reserved"));
            QCOMPARE(QString(invalid->iso_3166_1_alpha2), QString("XX"));
        }
    }

    void test_ecc_helper_functions() {
        // Test getCountryName()
        QCOMPARE(QString(getCountryName(0xF3, 0x02)), QString("Thailand"));
        QCOMPARE(QString(getCountryName(0xE1, 0x01)), QString("Germany"));

        // Test getISOCode()
        QCOMPARE(QString(getISOCode(0xF3, 0x02)), QString("TH"));
        QCOMPARE(QString(getISOCode(0xE2, 0x0F)), QString("FR"));

        // Test validation
        QVERIFY(isValidCountryCode(0xF3, 0x02));  // Thailand
        QVERIFY(isValidCountryCode(0xE1, 0x01));  // Germany
        QVERIFY(!isValidCountryCode(0x00, 0x00)); // Reserved
    }

    void test_ecc_description_formatting() {
        // Test getCountryDescription() formatting
        std::string thailand_desc = getCountryDescription(0xF3, 0x02);
        QVERIFY(thailand_desc.find("Thailand") != std::string::npos);
        QVERIFY(thailand_desc.find("TH") != std::string::npos);

        // Test unknown country formatting
        std::string unknown_desc = getCountryDescription(0x00, 0x00);
        QVERIFY(unknown_desc.find("Unknown") != std::string::npos ||
                unknown_desc.find("Reserved") != std::string::npos);
    }

    // ========================================================================
    // Language Table Tests
    // ========================================================================

    void test_language_thai_lookup() {
        // CRITICAL: Thai language (0x2B)
        auto thai = lookupLanguage(0x2B);

        QCOMPARE(thai.language_code, static_cast<uint8_t>(0x2B));
        QCOMPARE(QString(thai.language_name), QString("Thai"));
        QCOMPARE(QString(thai.iso_639_2), QString("tha"));
        QVERIFY(!thai.is_regional_variant);
        QVERIFY(thai.isValid());
    }

    void test_language_english_lookup() {
        // English (0x09)
        auto english = lookupLanguage(0x09);

        QCOMPARE(english.language_code, static_cast<uint8_t>(0x09));
        QCOMPARE(QString(english.language_name), QString("English"));
        QCOMPARE(QString(english.iso_639_2), QString("eng"));
        QVERIFY(!english.is_regional_variant);
    }

    void test_language_german_lookup() {
        // German (0x0F)
        auto german = lookupLanguage(0x0F);

        QCOMPARE(german.language_code, static_cast<uint8_t>(0x0F));
        QCOMPARE(QString(german.language_name), QString("German"));
        QCOMPARE(QString(german.iso_639_2), QString("ger"));
        QVERIFY(!german.is_regional_variant);
    }

    void test_language_french_lookup() {
        // French (0x0C)
        auto french = lookupLanguage(0x0C);

        QCOMPARE(french.language_code, static_cast<uint8_t>(0x0C));
        QCOMPARE(QString(french.language_name), QString("French"));
        QCOMPARE(QString(french.iso_639_2), QString("fre"));
    }

    void test_language_italian_lookup() {
        // Italian (0x14)
        auto italian = lookupLanguage(0x14);

        QCOMPARE(italian.language_code, static_cast<uint8_t>(0x14));
        QCOMPARE(QString(italian.language_name), QString("Italian"));
        QCOMPARE(QString(italian.iso_639_2), QString("ita"));
    }

    void test_language_regional_variants() {
        // Test regional variant flag (Flemish is regional Dutch)
        auto flemish = lookupLanguage(0x0B);

        QCOMPARE(QString(flemish.language_name), QString("Flemish"));
        QCOMPARE(QString(flemish.iso_639_2), QString("dut"));  // Same as Dutch
        QVERIFY(flemish.is_regional_variant);
    }

    void test_language_unknown_lookup() {
        // Test unknown language code (0xFF not in table)
        auto unknown = lookupLanguage(0xFF);

        QCOMPARE(QString(unknown.language_name), QString("Unknown"));
        QCOMPARE(QString(unknown.iso_639_2), QString("und"));  // Undetermined
    }

    void test_language_helper_functions() {
        // Test getLanguageName()
        QCOMPARE(QString(getLanguageName(0x2B)), QString("Thai"));
        QCOMPARE(QString(getLanguageName(0x09)), QString("English"));

        // Test getISO639Code()
        QCOMPARE(QString(getISO639Code(0x2B)), QString("tha"));
        QCOMPARE(QString(getISO639Code(0x0F)), QString("ger"));

        // Test validation
        QVERIFY(isValidLanguageCode(0x2B));  // Thai
        QVERIFY(isValidLanguageCode(0x09));  // English
        QVERIFY(!isValidLanguageCode(0x00)); // Unknown
    }

    void test_language_description_formatting() {
        // Test getLanguageDescription() formatting
        std::string thai_desc = getLanguageDescription(0x2B);
        QVERIFY(thai_desc.find("Thai") != std::string::npos);
        QVERIFY(thai_desc.find("tha") != std::string::npos);

        // Test regional variant formatting
        std::string flemish_desc = getLanguageDescription(0x0B);
        QVERIFY(flemish_desc.find("Flemish") != std::string::npos);
        QVERIFY(flemish_desc.find("[Regional]") != std::string::npos);
    }

    // ========================================================================
    // Programme Type Table Tests
    // ========================================================================

    void test_pty_all_entries_valid() {
        // Test all 32 programme types (0-31)
        for (uint8_t pty = 0; pty <= 31; ++pty) {
            const auto* pty_info = lookupProgrammeType(pty);

            QVERIFY(pty_info != nullptr);
            QCOMPARE(pty_info->pty_code, pty);
            QVERIFY(pty_info->isValid());
            QVERIFY(QString(pty_info->international_name).length() > 0);
            QVERIFY(QString(pty_info->category).length() > 0);
        }
    }

    void test_pty_specific_entries() {
        // Test specific key programme types

        // PTy 0: No programme type
        const auto* pty0 = lookupProgrammeType(0);
        QCOMPARE(QString(pty0->international_name), QString("No programme type"));
        QCOMPARE(QString(pty0->category), QString("None"));

        // PTy 1: News
        const auto* pty1 = lookupProgrammeType(1);
        QCOMPARE(QString(pty1->international_name), QString("News"));
        QCOMPARE(QString(pty1->category), QString("Information"));

        // PTy 4: Sport
        const auto* pty4 = lookupProgrammeType(4);
        QCOMPARE(QString(pty4->international_name), QString("Sport"));
        QCOMPARE(QString(pty4->category), QString("Entertainment"));

        // PTy 10: Pop Music
        const auto* pty10 = lookupProgrammeType(10);
        QCOMPARE(QString(pty10->international_name), QString("Pop Music"));
        QCOMPARE(QString(pty10->category), QString("Music"));

        // PTy 16: Weather
        const auto* pty16 = lookupProgrammeType(16);
        QCOMPARE(QString(pty16->international_name), QString("Weather"));
        QCOMPARE(QString(pty16->category), QString("Information"));

        // PTy 31: Alarm (Emergency)
        const auto* pty31 = lookupProgrammeType(31);
        QCOMPARE(QString(pty31->international_name), QString("Alarm"));
        QCOMPARE(QString(pty31->category), QString("Other"));
    }

    void test_pty_categories() {
        // Test category classification

        // Information category
        QCOMPARE(QString(lookupProgrammeType(1)->category), QString("Information"));  // News
        QCOMPARE(QString(lookupProgrammeType(3)->category), QString("Information"));  // Information
        QCOMPARE(QString(lookupProgrammeType(16)->category), QString("Information")); // Weather

        // Music category
        QCOMPARE(QString(lookupProgrammeType(10)->category), QString("Music"));  // Pop Music
        QCOMPARE(QString(lookupProgrammeType(15)->category), QString("Music"));  // Other Music
        QCOMPARE(QString(lookupProgrammeType(24)->category), QString("Music"));  // Jazz Music

        // Entertainment category
        QCOMPARE(QString(lookupProgrammeType(4)->category), QString("Entertainment"));  // Sport
        QCOMPARE(QString(lookupProgrammeType(18)->category), QString("Entertainment")); // Children's
    }

    void test_pty_invalid_code() {
        // Test invalid PTy code (>31)
        const auto* invalid = lookupProgrammeType(32);
        QVERIFY(invalid == nullptr);

        QVERIFY(!isValidProgrammeType(32));
        QVERIFY(!isValidProgrammeType(255));
        QVERIFY(isValidProgrammeType(0));
        QVERIFY(isValidProgrammeType(31));
    }

    void test_pty_helper_functions() {
        // Test getProgrammeTypeName()
        QCOMPARE(QString(getProgrammeTypeName(1)), QString("News"));
        QCOMPARE(QString(getProgrammeTypeName(10)), QString("Pop Music"));
        QCOMPARE(QString(getProgrammeTypeName(32)), QString("Unknown"));

        // Test getProgrammeTypeCategory()
        QCOMPARE(QString(getProgrammeTypeCategory(1)), QString("Information"));
        QCOMPARE(QString(getProgrammeTypeCategory(10)), QString("Music"));
        QCOMPARE(QString(getProgrammeTypeCategory(32)), QString("Unknown"));
    }

    void test_pty_description_formatting() {
        // Test getProgrammeTypeDescription() formatting
        std::string news_desc = getProgrammeTypeDescription(1);
        QVERIFY(news_desc.find("News") != std::string::npos);
        QVERIFY(news_desc.find("Information") != std::string::npos);

        std::string pop_desc = getProgrammeTypeDescription(10);
        QVERIFY(pop_desc.find("Pop Music") != std::string::npos);
        QVERIFY(pop_desc.find("Music") != std::string::npos);

        // Test unknown PTy
        std::string unknown_desc = getProgrammeTypeDescription(32);
        QVERIFY(unknown_desc.find("Unknown") != std::string::npos);
    }

    // ========================================================================
    // Performance Tests
    // ========================================================================

    void test_performance_country_lookup() {
        // Benchmark country lookup performance
        const int iterations = 10000;

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            lookupCountry(0xF3, 0x02);  // Thailand
        }
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double avg_ns = duration.count() / static_cast<double>(iterations);

        qDebug() << "Country lookup average time:" << avg_ns << "ns";
        QVERIFY(avg_ns < 1000);  // Should be < 1 microsecond
    }

    void test_performance_language_lookup() {
        // Benchmark language lookup performance
        const int iterations = 10000;

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            lookupLanguage(0x2B);  // Thai
        }
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double avg_ns = duration.count() / static_cast<double>(iterations);

        qDebug() << "Language lookup average time:" << avg_ns << "ns";
        QVERIFY(avg_ns < 1000);  // Should be < 1 microsecond
    }

    void test_performance_pty_lookup() {
        // Benchmark programme type lookup performance
        const int iterations = 10000;

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            lookupProgrammeType(10);  // Pop Music
        }
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double avg_ns = duration.count() / static_cast<double>(iterations);

        qDebug() << "PTy lookup average time:" << avg_ns << "ns";
        QVERIFY(avg_ns < 100);  // Should be < 100 nanoseconds (array access)
    }

    // ========================================================================
    // Integration Tests
    // ========================================================================

    void test_thai_broadcasting_scenario() {
        // Simulate Thai DAB ensemble metadata lookup

        // Country: Thailand
        const auto* country = lookupCountry(0xF3, 0x02);
        QVERIFY(country != nullptr);
        QCOMPARE(QString(country->country_name), QString("Thailand"));

        // Language: Thai
        auto language = lookupLanguage(0x2B);
        QCOMPARE(QString(language.language_name), QString("Thai"));

        // Programme Type: News in Thai
        const auto* pty = lookupProgrammeType(1);
        QCOMPARE(QString(pty->international_name), QString("News"));

        // Generate complete description
        std::string country_desc = getCountryDescription(0xF3, 0x02);
        std::string lang_desc = getLanguageDescription(0x2B);
        std::string pty_desc = getProgrammeTypeDescription(1);

        qDebug() << "Thai Broadcasting Metadata:";
        qDebug() << "  Country:" << QString::fromStdString(country_desc);
        qDebug() << "  Language:" << QString::fromStdString(lang_desc);
        qDebug() << "  Programme Type:" << QString::fromStdString(pty_desc);

        QVERIFY(country_desc.find("Thailand") != std::string::npos);
        QVERIFY(lang_desc.find("Thai") != std::string::npos);
        QVERIFY(pty_desc.find("News") != std::string::npos);
    }

    void test_european_broadcasting_scenario() {
        // Simulate European DAB ensemble (Germany, German, Music)

        const auto* country = lookupCountry(0xE1, 0x01);
        auto language = lookupLanguage(0x0F);
        const auto* pty = lookupProgrammeType(10);

        QCOMPARE(QString(country->country_name), QString("Germany"));
        QCOMPARE(QString(language.language_name), QString("German"));
        QCOMPARE(QString(pty->international_name), QString("Pop Music"));

        qDebug() << "European Broadcasting Metadata:";
        qDebug() << "  Country:" << QString::fromStdString(getCountryDescription(0xE1, 0x01));
        qDebug() << "  Language:" << QString::fromStdString(getLanguageDescription(0x0F));
        qDebug() << "  Programme Type:" << QString::fromStdString(getProgrammeTypeDescription(10));
    }
};

QTEST_MAIN(TestETSIRegisteredTables)
#include "test_etsi_registered_tables.moc"
