/**
 * @file test_charset_converter.cpp
 * @brief Unit tests for FIG 1 label charset conversion (F4)
 *
 * Covers the label character sets used by DAB FIG 1/0 and 1/1 labels
 * (EN 300 401 8.1.13):
 *   - charset 0: Complete EBU Latin (0x80 -> U+00E1 'á', 0xE1 -> 'Å')
 *   - charset 6: TIS-620 Thai (0xA1 -> U+0E01 'ก')
 *   - ASCII is preserved by both.
 *
 * @author C++ Qt Developer Agent (standards-review F4)
 */

#include <QtTest/QtTest>
#include "core/charset_converter.hpp"

class TestCharsetConverter : public QObject {
    Q_OBJECT

private slots:
    void testEBULatinAsciiPassthrough();
    void testEBULatinSpecialChar();
    void testEBULatinAccentedRow();
    void testEBULatinInvalidByte();
    void testTIS620AsciiPassthrough();
    void testTIS620ThaiBlock();
    void testTIS620Boundaries();
};

void TestCharsetConverter::testEBULatinAsciiPassthrough()
{
    // Charset 0 (EBU Latin) must preserve pure-ASCII labels byte-for-byte.
    const std::string input = "Bangkok DAB+";
    QCOMPARE(QString::fromStdString(convert_ebu_to_utf8(input)),
             QString::fromStdString(input));
}

void TestCharsetConverter::testEBULatinSpecialChar()
{
    // EBU Latin 0x80 -> UTF-8 'á' (U+00E1 = 0xC3 0xA1). (In the Complete
    // EBU Latin table the accented lowercase row starts at 0x80; 0xE1 is
    // 'Å' — verified against etisnoop's charset table.)
    const std::string input("\x80", 1);
    const std::string utf8 = convert_ebu_to_utf8(input);
    QCOMPARE(utf8, std::string("\xC3\xA1"));
    QCOMPARE(QString::fromUtf8(utf8.c_str(), static_cast<int>(utf8.size())),
             QString::fromUtf8("á"));
}

void TestCharsetConverter::testEBULatinAccentedRow()
{
    // Upper-case row: 0xE1 -> 'Å' (U+00C5 = 0xC3 0x85), matching the
    // Complete EBU Latin table used by etisnoop's charset.cpp.
    const std::string input("\xE1", 1);
    const std::string utf8 = convert_ebu_to_utf8(input);
    QCOMPARE(utf8, std::string("\xC3\x85"));
    QCOMPARE(QString::fromUtf8(utf8.c_str(), static_cast<int>(utf8.size())),
             QString::fromUtf8("Å"));
}
void TestCharsetConverter::testEBULatinInvalidByte()
{
    // 0x00 cannot be represented in the EBU Latin table -> U+2047 '⁇'.
    const std::string input("\x00", 1);
    QCOMPARE(convert_ebu_to_utf8(input), std::string("\xE2\x81\x87"));
}

void TestCharsetConverter::testTIS620AsciiPassthrough()
{
    // TIS-620 is ASCII-compatible below 0x80.
    const std::string input = "103.5 dab+ RADIO";
    QCOMPARE(QString::fromStdString(convert_tis620_to_utf8(input)),
             QString::fromStdString(input));
}

void TestCharsetConverter::testTIS620ThaiBlock()
{
    // TIS-620 0xA1 -> 'ก' (U+0E01 = 0xE0 0xB8 0x81).
    const std::string input("\xA1", 1);
    const std::string utf8 = convert_tis620_to_utf8(input);
    QCOMPARE(utf8, std::string("\xE0\xB8\x81"));
    QCOMPARE(QString::fromUtf8(utf8.c_str(), static_cast<int>(utf8.size())),
             QString::fromUtf8("ก"));
}

void TestCharsetConverter::testTIS620Boundaries()
{
    // 0xDF -> '฿' (U+0E3F), 0xFB -> U+0E5B — both ends of the linear block.
    const std::string df("\xDF", 1);
    QCOMPARE(convert_tis620_to_utf8(df), std::string("\xE0\xB8\xBF"));

    const std::string fb("\xFB", 1);
    // U+0E5B = 1110 0000 1011 1001 1001 1011 -> 0xE0 0xB9 0x9B
    QCOMPARE(convert_tis620_to_utf8(fb), std::string("\xE0\xB9\x9B"));

    // 0x80 (undefined range) passes through unchanged.
    const std::string eight0("\x80", 1);
    QCOMPARE(convert_tis620_to_utf8(eight0), eight0);
}

QTEST_MAIN(TestCharsetConverter)
#include "test_charset_converter.moc"