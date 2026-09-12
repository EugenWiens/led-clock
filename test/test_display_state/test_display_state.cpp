// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#include <unity.h>

#include "config.h"
#include "display/Display.h"
#include "display/Font.h"

#include "test_support/StubAdcHal.h"
#include "test_support/StubLedHal.h"

class FakeTimeSource final : public ITimeSource {
public:
    bool available{true};
    struct tm value{};

    [[nodiscard]] bool getTime(struct tm& timeInfo) override {
        if (!available) {
            return false;
        }
        timeInfo = value;
        return true;
    }
};

class FakeSensorSource final : public ISensorSource {
public:
    bool available{false};
    SwitchBotData value{};

    [[nodiscard]] bool getData(SwitchBotData& data) override {
        if (!available) {
            data = {};
            return false;
        }
        data = value;
        return true;
    }
};

static StubLedHal g_ledHal;
static StubAdcHal g_adcHal;
static FakeTimeSource g_timeSource;
static FakeSensorSource g_sensorSource;
static Display g_display{g_ledHal, g_adcHal, g_timeSource, g_sensorSource};

static uint8_t readGlyphRow(uint8_t matrixIdx, uint8_t row) {
    uint8_t bits = 0;
    for (uint8_t col = 0u; col < 5u; ++col) {
        const CRGB led =
            g_ledHal.leds[static_cast<uint16_t>(matrixIdx * 64u + row * 8u + col + 1u)];
        if (led.r > 0 || led.g > 0 || led.b > 0) {
            bits |= static_cast<uint8_t>(1u << (4u - col));
        }
    }
    return bits;
}

static bool matrixBlank(uint8_t matrixIdx) {
    for (uint16_t index = matrixIdx * 64u; index < (matrixIdx + 1u) * 64u; ++index) {
        const CRGB led = g_ledHal.leds[index];
        if (led.r > 0 || led.g > 0 || led.b > 0) {
            return false;
        }
    }
    return true;
}

static bool frameEquals(const StubLedHal& lhs, const StubLedHal& rhs) {
    for (uint16_t index = 0u; index < LED_COUNT; ++index) {
        const CRGB left = lhs.leds[index];
        const CRGB right = rhs.leds[index];
        if (left.r != right.r || left.g != right.g || left.b != right.b) {
            return false;
        }
    }
    return true;
}

void setUp() {
    g_ledHal.brightness = 128u;
    g_ledHal.leds = nullptr;
    g_ledHal.ledCount = 0u;
    g_ledHal.showCount = 0u;

    g_adcHal.value = 2048;
    g_timeSource.available = true;
    g_timeSource.value = {};
    g_timeSource.value.tm_hour = 12;
    g_timeSource.value.tm_min = 34;
    g_sensorSource.available = false;
    g_sensorSource.value = {};

    g_display.init(0u);
}

void tearDown() {}

void test_display_starts_with_clock() {
    g_display.update(0u);

    TEST_ASSERT_EQUAL_HEX8(Font::FONT[1][0], readGlyphRow(0u, 0u));
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[2][0], readGlyphRow(1u, 0u));
    TEST_ASSERT_FALSE(matrixBlank(2u));
}

void test_display_switches_to_temperature_at_clock_boundary() {
    g_sensorSource.available = true;
    g_sensorSource.value.tempC = 23.5f;

    g_display.update(CLOCK_DISPLAY_MS - 1u);
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[1][0], readGlyphRow(0u, 0u));

    g_display.update(CLOCK_DISPLAY_MS);
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[2][0], readGlyphRow(0u, 0u));
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[3][0], readGlyphRow(1u, 0u));
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[5][0], readGlyphRow(3u, 0u));
}

void test_display_switches_back_to_clock_at_temperature_boundary() {
    g_sensorSource.available = true;
    g_sensorSource.value.tempC = 23.5f;

    g_display.update(CLOCK_DISPLAY_MS);
    g_display.update(CLOCK_DISPLAY_MS + TEMP_DISPLAY_MS - 1u);
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[2][0], readGlyphRow(0u, 0u));

    g_display.update(CLOCK_DISPLAY_MS + TEMP_DISPLAY_MS);
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[1][0], readGlyphRow(0u, 0u));
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[2][0], readGlyphRow(1u, 0u));
}

void test_colon_toggles_only_at_interval() {
    g_display.update(COLON_TOGGLE_MS - 1u);
    TEST_ASSERT_FALSE(matrixBlank(2u));

    g_display.update(COLON_TOGGLE_MS);
    TEST_ASSERT_TRUE(matrixBlank(2u));

    g_display.update(2u * COLON_TOGGLE_MS);
    TEST_ASSERT_FALSE(matrixBlank(2u));
}

void test_missing_sensor_renders_fallback() {
    g_display.update(CLOCK_DISPLAY_MS);

    TEST_ASSERT_EQUAL_HEX8(Font::FONT[Font::IDX_DASH][3], readGlyphRow(0u, 3u));
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[Font::IDX_DASH][3], readGlyphRow(1u, 3u));
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[Font::IDX_DASH][3], readGlyphRow(3u, 3u));
    TEST_ASSERT_EQUAL_HEX8(Font::FONT[Font::IDX_DEGREE][0], readGlyphRow(4u, 0u));
}

void test_same_timestamp_update_is_deterministic() {
    StubLedHal firstFrame;
    g_display.update(0u);

    CRGB expected[LED_COUNT]{};
    for (uint16_t index = 0u; index < LED_COUNT; ++index) {
        expected[index] = g_ledHal.leds[index];
    }
    firstFrame.leds = expected;

    g_display.update(0u);

    TEST_ASSERT_TRUE(frameEquals(g_ledHal, firstFrame));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_display_starts_with_clock);
    RUN_TEST(test_display_switches_to_temperature_at_clock_boundary);
    RUN_TEST(test_display_switches_back_to_clock_at_temperature_boundary);
    RUN_TEST(test_colon_toggles_only_at_interval);
    RUN_TEST(test_missing_sensor_renders_fallback);
    RUN_TEST(test_same_timestamp_update_is_deterministic);
    return UNITY_END();
}