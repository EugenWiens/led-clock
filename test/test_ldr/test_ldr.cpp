#include <unity.h>
#include "../../src/display/ldr.h"

// ---------------------------------------------------------------------------
// ADC stub
// ---------------------------------------------------------------------------
class StubAdcHal final : public IAdcHal {
public:
    int value{2048};
    void init() override {}
    int  read() override { return value; }
};

static StubAdcHal g_adc;
static Ldr        g_ldr{g_adc};

void setUp() {
    g_adc.value = 2048;
    g_ldr.init();
}

void tearDown() {}

// ---------------------------------------------------------------------------
// init()
// ---------------------------------------------------------------------------
void test_init_resets_brightness_to_min() {
    // Drive brightness up first, then re-init — must reset to BRIGHTNESS_MIN.
    g_adc.value = 4095;
    for (uint8_t i = 0; i < LDR_SAMPLES; ++i) {
        g_ldr.update(static_cast<uint64_t>((i + 1u) * LDR_SAMPLE_MS));
    }
    TEST_ASSERT_EQUAL_UINT8(255, g_ldr.brightness()); // sanity check

    g_ldr.init();
    TEST_ASSERT_EQUAL_UINT8(BRIGHTNESS_MIN, g_ldr.brightness());
}

// ---------------------------------------------------------------------------
// Time gating
// ---------------------------------------------------------------------------
void test_update_not_triggered_before_interval() {
    g_adc.value = 0; // would produce BRIGHTNESS_MIN if sampled
    g_ldr.update(0);
    g_ldr.update(LDR_SAMPLE_MS - 1u);
    TEST_ASSERT_EQUAL_UINT8(BRIGHTNESS_MIN, g_ldr.brightness()); // unchanged
}

void test_update_triggered_at_interval() {
    g_adc.value = 4095;
    g_ldr.update(LDR_SAMPLE_MS); // exactly one interval elapsed
    // One sample taken; brightness should have moved from BRIGHTNESS_MIN
    TEST_ASSERT_GREATER_THAN(BRIGHTNESS_MIN, g_ldr.brightness());
}

// ---------------------------------------------------------------------------
// Linear brightness mapping
// ---------------------------------------------------------------------------
static void feed(int adc, uint8_t n) {
    g_adc.value = adc;
    uint64_t t  = 0;
    for (uint8_t i = 0; i < n; ++i) { t += LDR_SAMPLE_MS; g_ldr.update(t); }
}

void test_brightness_min_at_adc_zero() {
    feed(0, LDR_SAMPLES);
    TEST_ASSERT_EQUAL_UINT8(BRIGHTNESS_MIN, g_ldr.brightness());
}

void test_brightness_max_at_adc_full() {
    feed(4095, LDR_SAMPLES);
    TEST_ASSERT_EQUAL_UINT8(BRIGHTNESS_MAX, g_ldr.brightness());
}

void test_brightness_midpoint_at_adc_half() {
    feed(2048, LDR_SAMPLES);
    // 10 + 2048 * (255 - 10) / 4095 ≈ 132
    TEST_ASSERT_INT_WITHIN(1, 132, g_ldr.brightness());
}

// ---------------------------------------------------------------------------
// Rolling average
// ---------------------------------------------------------------------------
void test_rolling_average_converges() {
    // 4 samples at 0, then 4 at 4095 → avg ≈ 2048 → ~132
    feed(0,    4);
    feed(4095, 4);
    TEST_ASSERT_INT_WITHIN(2, 132, g_ldr.brightness());
}

void test_rolling_average_window_evicts_old_samples() {
    // Fill with 0 → brightness = BRIGHTNESS_MIN
    feed(0, LDR_SAMPLES);
    TEST_ASSERT_EQUAL_UINT8(BRIGHTNESS_MIN, g_ldr.brightness());

    // Fill again with 4095 → old samples are evicted, brightness = BRIGHTNESS_MAX
    feed(4095, LDR_SAMPLES);
    TEST_ASSERT_EQUAL_UINT8(BRIGHTNESS_MAX, g_ldr.brightness());
}

// ---------------------------------------------------------------------------
// Clamp at boundaries
// ---------------------------------------------------------------------------
void test_brightness_never_below_min() {
    feed(0, LDR_SAMPLES * 2); // extra samples — must not go below min
    TEST_ASSERT_GREATER_OR_EQUAL(BRIGHTNESS_MIN, g_ldr.brightness());
}

void test_brightness_never_above_max() {
    feed(4095, LDR_SAMPLES * 2);
    TEST_ASSERT_LESS_OR_EQUAL(BRIGHTNESS_MAX, g_ldr.brightness());
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_init_resets_brightness_to_min);
    RUN_TEST(test_update_not_triggered_before_interval);
    RUN_TEST(test_update_triggered_at_interval);
    RUN_TEST(test_brightness_min_at_adc_zero);
    RUN_TEST(test_brightness_max_at_adc_full);
    RUN_TEST(test_brightness_midpoint_at_adc_half);
    RUN_TEST(test_rolling_average_converges);
    RUN_TEST(test_rolling_average_window_evicts_old_samples);
    RUN_TEST(test_brightness_never_below_min);
    RUN_TEST(test_brightness_never_above_max);
    return UNITY_END();
}
