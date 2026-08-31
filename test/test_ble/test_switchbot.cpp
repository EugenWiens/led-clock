#include <unity.h>
#include "../../src/ble/switchbot.h"
#include "../../src/config.h"

// ---------------------------------------------------------------------------
// Helpers — build a 6-byte SwitchBot service-data payload from components
// ---------------------------------------------------------------------------

/// Constructs the 6-byte service-data payload (bytes after UUID 0xFD3D).
/// Layout matches SwitchBot Open Protocol:
///   [0] device byte  [1] status  [2] battery
///   [3] temp tenths  [4] temp integer + sign  [5] humidity
static void makePayload(uint8_t buf[6], uint8_t tenths, uint8_t integer_, bool positive,
                        uint8_t humidity) {
    buf[0] = 0x00u; // device byte (don't care)
    buf[1] = 0x00u; // status
    buf[2] = 0x64u; // battery (100 %)
    buf[3] = tenths & 0x0Fu;
    buf[4] = static_cast<uint8_t>((integer_ & 0x7Fu) | (positive ? 0x80u : 0x00u));
    buf[5] = humidity & 0x7Fu;
}

void setUp() {}
void tearDown() {}

// ---------------------------------------------------------------------------
// parseSwitchBotServiceData — payload length guards
// ---------------------------------------------------------------------------

void test_parse_null_pointer_returns_false() {
    SwitchBotData out{};
    TEST_ASSERT_FALSE(parseSwitchBotServiceData(nullptr, 6u, out));
}

void test_parse_empty_returns_false() {
    uint8_t buf[6]{};
    SwitchBotData out{};
    TEST_ASSERT_FALSE(parseSwitchBotServiceData(buf, 0u, out));
}

void test_parse_too_short_returns_false() {
    uint8_t buf[5]{};
    SwitchBotData out{};
    TEST_ASSERT_FALSE(parseSwitchBotServiceData(buf, 5u, out));
}

void test_parse_exact_minimum_length_succeeds() {
    uint8_t buf[6]{};
    makePayload(buf, 3u, 22u, true, 55u);
    SwitchBotData out{};
    TEST_ASSERT_TRUE(parseSwitchBotServiceData(buf, 6u, out));
}

// ---------------------------------------------------------------------------
// parseSwitchBotServiceData — temperature decoding
// ---------------------------------------------------------------------------

void test_parse_positive_temperature() {
    uint8_t buf[6]{};
    makePayload(buf, /*tenths=*/3u, /*int=*/22u, /*pos=*/true, /*hum=*/50u);
    SwitchBotData out{};
    TEST_ASSERT_TRUE(parseSwitchBotServiceData(buf, 6u, out));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 22.3f, out.tempC);
}

void test_parse_negative_temperature() {
    uint8_t buf[6]{};
    makePayload(buf, /*tenths=*/5u, /*int=*/4u, /*pos=*/false, /*hum=*/80u);
    SwitchBotData out{};
    TEST_ASSERT_TRUE(parseSwitchBotServiceData(buf, 6u, out));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -4.5f, out.tempC);
}

void test_parse_zero_temperature() {
    uint8_t buf[6]{};
    makePayload(buf, /*tenths=*/0u, /*int=*/0u, /*pos=*/true, /*hum=*/50u);
    SwitchBotData out{};
    TEST_ASSERT_TRUE(parseSwitchBotServiceData(buf, 6u, out));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, out.tempC);
}

void test_parse_max_temperature() {
    uint8_t buf[6]{};
    makePayload(buf, /*tenths=*/9u, /*int=*/99u, /*pos=*/true, /*hum=*/99u);
    SwitchBotData out{};
    TEST_ASSERT_TRUE(parseSwitchBotServiceData(buf, 6u, out));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 99.9f, out.tempC);
}

// ---------------------------------------------------------------------------
// parseSwitchBotServiceData — humidity decoding
// ---------------------------------------------------------------------------

void test_parse_humidity_value() {
    uint8_t buf[6]{};
    makePayload(buf, 0u, 20u, true, /*hum=*/73u);
    SwitchBotData out{};
    TEST_ASSERT_TRUE(parseSwitchBotServiceData(buf, 6u, out));
    TEST_ASSERT_EQUAL_UINT8(73u, out.humidity);
}

void test_parse_humidity_max() {
    uint8_t buf[6]{};
    makePayload(buf, 0u, 25u, true, /*hum=*/100u);
    SwitchBotData out{};
    TEST_ASSERT_TRUE(parseSwitchBotServiceData(buf, 6u, out));
    // bit[7] of byte[5] is the sign bit stripped; 100 & 0x7F = 100
    TEST_ASSERT_EQUAL_UINT8(100u & 0x7Fu, out.humidity);
}

void test_parse_sets_valid_flag() {
    uint8_t buf[6]{};
    makePayload(buf, 0u, 21u, true, 60u);
    SwitchBotData out{};
    out.valid = false;
    const bool ok = parseSwitchBotServiceData(buf, 6u, out);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(out.valid);
}

// ---------------------------------------------------------------------------
// isSwitchBotStale
// ---------------------------------------------------------------------------

void test_fresh_data_is_not_stale() {
    SwitchBotData d{};
    d.valid = true;
    d.lastSeenMs = 1000u;
    // nowMs = lastSeenMs + threshold - 1  →  not stale
    TEST_ASSERT_FALSE(isSwitchBotStale(d, 1000u + SENSOR_STALE_MS - 1u));
}

void test_data_at_exact_threshold_is_stale() {
    SwitchBotData d{};
    d.valid = true;
    d.lastSeenMs = 1000u;
    // nowMs - lastSeenMs == SENSOR_STALE_MS  →  stale (>= threshold)
    TEST_ASSERT_TRUE(isSwitchBotStale(d, 1000u + SENSOR_STALE_MS));
}

void test_old_data_is_stale() {
    SwitchBotData d{};
    d.valid = true;
    d.lastSeenMs = 0u;
    TEST_ASSERT_TRUE(isSwitchBotStale(d, static_cast<uint64_t>(SENSOR_STALE_MS) + 1u));
}

void test_invalid_data_is_always_stale() {
    SwitchBotData d{};
    d.valid = false;
    d.lastSeenMs = 999999u; // very recent, but valid = false
    TEST_ASSERT_TRUE(isSwitchBotStale(d, 999999u));
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();

    // Length guards
    RUN_TEST(test_parse_null_pointer_returns_false);
    RUN_TEST(test_parse_empty_returns_false);
    RUN_TEST(test_parse_too_short_returns_false);
    RUN_TEST(test_parse_exact_minimum_length_succeeds);

    // Temperature
    RUN_TEST(test_parse_positive_temperature);
    RUN_TEST(test_parse_negative_temperature);
    RUN_TEST(test_parse_zero_temperature);
    RUN_TEST(test_parse_max_temperature);

    // Humidity
    RUN_TEST(test_parse_humidity_value);
    RUN_TEST(test_parse_humidity_max);
    RUN_TEST(test_parse_sets_valid_flag);

    // Staleness
    RUN_TEST(test_fresh_data_is_not_stale);
    RUN_TEST(test_data_at_exact_threshold_is_stale);
    RUN_TEST(test_old_data_is_stale);
    RUN_TEST(test_invalid_data_is_always_stale);

    return UNITY_END();
}
