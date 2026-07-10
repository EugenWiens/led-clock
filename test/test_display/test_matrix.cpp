#include <unity.h>
#include "../../src/display/matrix.h"

// ---------------------------------------------------------------------------
// HAL stub classes — replace FastLedHal / EspAdcHal in the native test build
// ---------------------------------------------------------------------------
class StubLedHal final : public ILedHal {
public:
    uint8_t brightness{128};
    void init(CRGB*, uint16_t) override {}
    void show() override {}
    void setBrightness(uint8_t b) override { brightness = b; }
};

class StubAdcHal final : public IAdcHal {
public:
    int value{2048};
    void init() override {}
    int  read() override { return value; }
};

// ---------------------------------------------------------------------------
// Global test instances (static avoids 960-byte Matrix on the stack)
// ---------------------------------------------------------------------------
static StubLedHal g_ledHal;
static StubAdcHal g_adcHal;
static Matrix     g_matrix{g_ledHal, g_adcHal};

static constexpr CRGB RED{255, 0, 0};

void setUp() {
    g_ledHal.brightness = 128;
    g_adcHal.value      = 2048;
    g_matrix.init(); // resets all internal state and clears the LED buffer
}

void tearDown() {}

// ---------------------------------------------------------------------------
// Coordinate mapping
// ---------------------------------------------------------------------------
void test_setPixel_matrix0_origin() {
    g_matrix.setPixel(0, 0, 0, RED);
    TEST_ASSERT_EQUAL_UINT8(255, g_matrix.getLed(0).r);
}

void test_setPixel_matrix0_col7() {
    g_matrix.setPixel(0, 7, 0, RED);
    TEST_ASSERT_EQUAL_UINT8(255, g_matrix.getLed(7).r);
}

void test_setPixel_matrix0_row7() {
    g_matrix.setPixel(0, 0, 7, RED);
    TEST_ASSERT_EQUAL_UINT8(255, g_matrix.getLed(56).r);
}

void test_setPixel_matrix0_last() {
    g_matrix.setPixel(0, 7, 7, RED);
    TEST_ASSERT_EQUAL_UINT8(255, g_matrix.getLed(63).r);
}

void test_setPixel_matrix1_origin() {
    g_matrix.setPixel(1, 0, 0, RED);
    TEST_ASSERT_EQUAL_UINT8(255, g_matrix.getLed(64).r);
}

void test_setPixel_matrix4_last() {
    g_matrix.setPixel(4, 7, 7, RED);
    TEST_ASSERT_EQUAL_UINT8(255, g_matrix.getLed(319).r);
}

void test_setPixel_does_not_bleed_into_next_matrix() {
    g_matrix.setPixel(0, 7, 7, RED);
    TEST_ASSERT_EQUAL_UINT8(0, g_matrix.getLed(64).r); // matrix 1 origin untouched
}

// ---------------------------------------------------------------------------
// Bounds checking
// ---------------------------------------------------------------------------
void test_setPixel_out_of_bounds_no_crash() {
    g_matrix.setPixel(5, 0, 0, RED); // matrixIdx out of range
    g_matrix.setPixel(0, 8, 0, RED); // col out of range
    g_matrix.setPixel(0, 0, 8, RED); // row out of range
    for (uint16_t i = 0; i < 320u; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0, g_matrix.getLed(i).r);
    }
}

// ---------------------------------------------------------------------------
// clear()
// ---------------------------------------------------------------------------
void test_clear_zeroes_all_leds() {
    g_matrix.setPixel(0, 0, 0, RED);
    g_matrix.setPixel(4, 7, 7, RED);
    g_matrix.clear();
    for (uint16_t i = 0; i < 320u; ++i) {
        const CRGB led = g_matrix.getLed(i);
        TEST_ASSERT_EQUAL_UINT8(0, led.r);
        TEST_ASSERT_EQUAL_UINT8(0, led.g);
        TEST_ASSERT_EQUAL_UINT8(0, led.b);
    }
}

// ---------------------------------------------------------------------------
// LDR brightness — time gating
// ---------------------------------------------------------------------------
void test_brightness_not_updated_before_sample_interval() {
    g_adcHal.value      = 0;   // would produce BRIGHTNESS_MIN if sampled
    g_ledHal.brightness = 128; // sentinel
    g_matrix.updateBrightness(0);
    g_matrix.updateBrightness(499);
    TEST_ASSERT_EQUAL_UINT8(128, g_ledHal.brightness);
}

// ---------------------------------------------------------------------------
// LDR brightness — linear mapping
// ---------------------------------------------------------------------------
static void feed_ldr_samples(int adc, uint8_t n) {
    g_adcHal.value = adc;
    uint64_t t     = 0;
    for (uint8_t i = 0; i < n; ++i) { t += 500; g_matrix.updateBrightness(t); }
}

void test_brightness_min_at_adc_zero() {
    feed_ldr_samples(0, LDR_SAMPLES);
    TEST_ASSERT_EQUAL_UINT8(10, g_ledHal.brightness);
}

void test_brightness_max_at_adc_full() {
    feed_ldr_samples(4095, LDR_SAMPLES);
    TEST_ASSERT_EQUAL_UINT8(255, g_ledHal.brightness);
}

void test_brightness_midpoint_at_adc_half() {
    feed_ldr_samples(2048, LDR_SAMPLES);
    // Expected: 10 + 2048 * (255 - 10) / 4095 ≈ 132
    TEST_ASSERT_INT_WITHIN(1, 132, g_ledHal.brightness);
}

void test_rolling_average_converges_from_mixed_samples() {
    // First 4 at 0, then 4 at 4095 — rolling avg ≈ 2048 → ~132
    g_adcHal.value = 0;
    uint64_t t     = 0;
    for (uint8_t i = 0; i < 4; ++i) { t += 500; g_matrix.updateBrightness(t); }
    g_adcHal.value = 4095;
    for (uint8_t i = 0; i < 4; ++i) { t += 500; g_matrix.updateBrightness(t); }
    TEST_ASSERT_INT_WITHIN(2, 132, g_ledHal.brightness);
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_setPixel_matrix0_origin);
    RUN_TEST(test_setPixel_matrix0_col7);
    RUN_TEST(test_setPixel_matrix0_row7);
    RUN_TEST(test_setPixel_matrix0_last);
    RUN_TEST(test_setPixel_matrix1_origin);
    RUN_TEST(test_setPixel_matrix4_last);
    RUN_TEST(test_setPixel_does_not_bleed_into_next_matrix);
    RUN_TEST(test_setPixel_out_of_bounds_no_crash);
    RUN_TEST(test_clear_zeroes_all_leds);
    RUN_TEST(test_brightness_not_updated_before_sample_interval);
    RUN_TEST(test_brightness_min_at_adc_zero);
    RUN_TEST(test_brightness_max_at_adc_full);
    RUN_TEST(test_brightness_midpoint_at_adc_half);
    RUN_TEST(test_rolling_average_converges_from_mixed_samples);
    return UNITY_END();
}


// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------
int main() {
    UNITY_BEGIN();

    RUN_TEST(test_setPixel_matrix0_origin);
    RUN_TEST(test_setPixel_matrix0_col7);
    RUN_TEST(test_setPixel_matrix0_row7);
    RUN_TEST(test_setPixel_matrix0_last);
    RUN_TEST(test_setPixel_matrix1_origin);
    RUN_TEST(test_setPixel_matrix4_last);
    RUN_TEST(test_setPixel_does_not_bleed_into_next_matrix);
    RUN_TEST(test_setPixel_out_of_bounds_no_crash);
    RUN_TEST(test_clear_zeroes_all_leds);
    RUN_TEST(test_brightness_not_updated_before_sample_interval);
    RUN_TEST(test_brightness_min_at_adc_zero);
    RUN_TEST(test_brightness_max_at_adc_full);
    RUN_TEST(test_brightness_midpoint_at_adc_half);
    RUN_TEST(test_rolling_average_converges_from_mixed_samples);

    return UNITY_END();
}
