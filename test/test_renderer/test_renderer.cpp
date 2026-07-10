#include <unity.h>
#include <limits>
#include "../../src/display/renderer.h"
#include "../../src/display/font.h"

// ---------------------------------------------------------------------------
// HAL stubs
// ---------------------------------------------------------------------------
class StubLedHal final : public ILedHal {
public:
    void init(CRGB*, uint16_t) override {}
    void show() override {}
    void setBrightness(uint8_t) override {}
};

class StubAdcHal final : public IAdcHal {
public:
    void init() override {}
    int  read() override { return 2048; }
};

static StubLedHal g_ledHal;
static StubAdcHal g_adcHal;
static Matrix     g_matrix{g_ledHal, g_adcHal};
static Renderer   g_renderer{g_matrix};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Read back the 5-bit glyph row from the LED array (col_offset = 1).
static uint8_t readGlyphRow(uint8_t matrixIdx, uint8_t row) {
    uint8_t bits = 0;
    for (uint8_t c = 0u; c < 5u; ++c) {
        const CRGB led = g_matrix.getLed(
            static_cast<uint16_t>(matrixIdx * 64u + row * 8u + c + 1u));
        if (led.r > 0 || led.g > 0 || led.b > 0) {
            bits |= static_cast<uint8_t>(1u << (4u - c));
        }
    }
    return bits;
}

/// Returns true if all LEDs in the given matrix index are off.
static bool matrixBlank(uint8_t matrixIdx) {
    for (uint16_t i = matrixIdx * 64u; i < (matrixIdx + 1u) * 64u; ++i) {
        const CRGB led = g_matrix.getLed(i);
        if (led.r > 0 || led.g > 0 || led.b > 0) return false;
    }
    return true;
}

void setUp()    { g_matrix.init(); }
void tearDown() {}

// ---------------------------------------------------------------------------
// Font spot-checks
// ---------------------------------------------------------------------------
void test_font_digit0_row0_matches_table() {
    g_renderer.renderClock(0, 0, false); // matrix 0 = '0'
    TEST_ASSERT_EQUAL_HEX8(font::FONT[0][0], readGlyphRow(0, 0));
}

void test_font_digit8_all_rows_match_table() {
    g_renderer.renderClock(8, 8, false); // matrix 1 = '8'
    for (uint8_t row = 0u; row < font::GLYPH_ROWS; ++row) {
        TEST_ASSERT_EQUAL_HEX8(font::FONT[8][row], readGlyphRow(1, row));
    }
}

// ---------------------------------------------------------------------------
// renderClock — digit placement
// ---------------------------------------------------------------------------
void test_renderClock_hour_tens_on_matrix0() {
    g_renderer.renderClock(23, 0, false);
    TEST_ASSERT_EQUAL_HEX8(font::FONT[2][0], readGlyphRow(0, 0)); // '2'
}

void test_renderClock_hour_units_on_matrix1() {
    g_renderer.renderClock(23, 0, false);
    TEST_ASSERT_EQUAL_HEX8(font::FONT[3][0], readGlyphRow(1, 0)); // '3'
}

void test_renderClock_minute_tens_on_matrix3() {
    g_renderer.renderClock(0, 45, false);
    TEST_ASSERT_EQUAL_HEX8(font::FONT[4][0], readGlyphRow(3, 0)); // '4'
}

void test_renderClock_minute_units_on_matrix4() {
    g_renderer.renderClock(0, 45, false);
    TEST_ASSERT_EQUAL_HEX8(font::FONT[5][0], readGlyphRow(4, 0)); // '5'
}

// ---------------------------------------------------------------------------
// renderClock — colon
// ---------------------------------------------------------------------------
void test_renderClock_colon_on_renders_glyph() {
    g_renderer.renderClock(0, 0, true);
    // Row 1 of colon glyph (.XX..) should appear on matrix 2
    TEST_ASSERT_EQUAL_HEX8(font::FONT[font::IDX_COLON][1], readGlyphRow(2, 1));
}

void test_renderClock_colon_off_matrix2_is_blank() {
    g_renderer.renderClock(0, 0, false);
    TEST_ASSERT_TRUE(matrixBlank(2));
}

// ---------------------------------------------------------------------------
// renderTemp — normal two-digit
// ---------------------------------------------------------------------------
void test_renderTemp_two_digit_integer_part() {
    g_renderer.renderTemp(23.5f);
    TEST_ASSERT_EQUAL_HEX8(font::FONT[2][0],              readGlyphRow(0, 0)); // '2'
    TEST_ASSERT_EQUAL_HEX8(font::FONT[3][0],              readGlyphRow(1, 0)); // '3'
    TEST_ASSERT_EQUAL_HEX8(font::FONT[5][0],              readGlyphRow(3, 0)); // '5'
    TEST_ASSERT_EQUAL_HEX8(font::FONT[font::IDX_DEGREE][0], readGlyphRow(4, 0)); // '°'
}

void test_renderTemp_decimal_dot_on_matrix2() {
    g_renderer.renderTemp(23.5f);
    // 2×2 dot: top-left corner at (col 3, row 5) of matrix 2
    const CRGB led = g_matrix.getLed(2u * 64u + 5u * 8u + 3u);
    TEST_ASSERT_GREATER_THAN(0u, static_cast<unsigned>(led.r + led.g + led.b));
}

// ---------------------------------------------------------------------------
// renderTemp — single-digit integer (matrix 0 blank)
// ---------------------------------------------------------------------------
void test_renderTemp_single_digit_matrix0_blank() {
    g_renderer.renderTemp(5.2f);
    TEST_ASSERT_TRUE(matrixBlank(0));
    TEST_ASSERT_EQUAL_HEX8(font::FONT[5][0], readGlyphRow(1, 0)); // '5'
}

// ---------------------------------------------------------------------------
// renderTemp — negative
// ---------------------------------------------------------------------------
void test_renderTemp_negative_shows_dash_and_digit() {
    g_renderer.renderTemp(-3.7f);
    // Row 3 is the only non-zero row of the '-' glyph
    TEST_ASSERT_EQUAL_HEX8(font::FONT[font::IDX_DASH][3], readGlyphRow(0, 3)); // '-'
    TEST_ASSERT_EQUAL_HEX8(font::FONT[3][0],              readGlyphRow(1, 0)); // '3'
    TEST_ASSERT_EQUAL_HEX8(font::FONT[7][0],              readGlyphRow(3, 0)); // '7'
}

// ---------------------------------------------------------------------------
// renderTemp — fallback "--.-°"
// ---------------------------------------------------------------------------
void test_renderTemp_nan_shows_fallback() {
    g_renderer.renderTemp(std::numeric_limits<float>::quiet_NaN());
    TEST_ASSERT_EQUAL_HEX8(font::FONT[font::IDX_DASH][3],   readGlyphRow(0, 3)); // '-'
    TEST_ASSERT_EQUAL_HEX8(font::FONT[font::IDX_DASH][3],   readGlyphRow(1, 3)); // '-'
    TEST_ASSERT_EQUAL_HEX8(font::FONT[font::IDX_DASH][3],   readGlyphRow(3, 3)); // '-'
    TEST_ASSERT_EQUAL_HEX8(font::FONT[font::IDX_DEGREE][0], readGlyphRow(4, 0)); // '°'
}

void test_renderTemp_out_of_range_shows_fallback() {
    g_renderer.renderTemp(100.0f);
    TEST_ASSERT_EQUAL_HEX8(font::FONT[font::IDX_DASH][3], readGlyphRow(0, 3));
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_font_digit0_row0_matches_table);
    RUN_TEST(test_font_digit8_all_rows_match_table);
    RUN_TEST(test_renderClock_hour_tens_on_matrix0);
    RUN_TEST(test_renderClock_hour_units_on_matrix1);
    RUN_TEST(test_renderClock_minute_tens_on_matrix3);
    RUN_TEST(test_renderClock_minute_units_on_matrix4);
    RUN_TEST(test_renderClock_colon_on_renders_glyph);
    RUN_TEST(test_renderClock_colon_off_matrix2_is_blank);
    RUN_TEST(test_renderTemp_two_digit_integer_part);
    RUN_TEST(test_renderTemp_decimal_dot_on_matrix2);
    RUN_TEST(test_renderTemp_single_digit_matrix0_blank);
    RUN_TEST(test_renderTemp_negative_shows_dash_and_digit);
    RUN_TEST(test_renderTemp_nan_shows_fallback);
    RUN_TEST(test_renderTemp_out_of_range_shows_fallback);
    return UNITY_END();
}
