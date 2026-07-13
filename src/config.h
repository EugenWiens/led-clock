#pragma once

// ---------------------------------------------------------------------------
// LED matrix
// ---------------------------------------------------------------------------
#define LED_DATA_PIN 8 // GPIO8 → 74HCT125 level-shifter → Matrix 0 DIN
#define LED_COUNT 320  // 5 matrices × 64 LEDs each

// ---------------------------------------------------------------------------
// LDR (ambient light sensor — GPIO2 = ADC_UNIT_1 / ADC_CHANNEL_2)
// ---------------------------------------------------------------------------
#define LDR_SAMPLE_MS 500u // sample interval (ms)
#define LDR_SAMPLES 8u     // rolling-average window size
#define BRIGHTNESS_MIN 10u
#define BRIGHTNESS_MAX 255u

// ---------------------------------------------------------------------------
// NTP / Timezone — must be set in [private_credentials] in platformio_user.ini
// ---------------------------------------------------------------------------
#if !defined(NTP_SERVER) && !defined(NATIVE_ENV)
#error                                                                                             \
    "NTP_SERVER not set — add '-DNTP_SERVER=\"pool.ntp.org\"' to [private_credentials] in platformio_user.ini"
#endif
#ifndef NTP_SERVER
#define NTP_SERVER ""
#endif

#if !defined(TIMEZONE) && !defined(NATIVE_ENV)
#error                                                                                             \
    "TIMEZONE not set — add '-DTIMEZONE=\"CET-1CEST,M3.5.0,M10.5.0/3\"' to [private_credentials] in platformio_user.ini"
#endif
#ifndef TIMEZONE
#define TIMEZONE ""
#endif

// ---------------------------------------------------------------------------
// WiFi — must be set in [private_credentials] in platformio_user.ini
// ---------------------------------------------------------------------------
#if !defined(WIFI_SSID) && !defined(NATIVE_ENV)
#error                                                                                             \
    "WIFI_SSID not set — add '-DWIFI_SSID=\"your_ssid\"' to [private_credentials] in platformio_user.ini"
#endif
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#if !defined(WIFI_PASS) && !defined(NATIVE_ENV)
#error                                                                                             \
    "WIFI_PASS not set — add '-DWIFI_PASS=\"your_password\"' to [private_credentials] in platformio_user.ini"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

// ---------------------------------------------------------------------------
// SwitchBot Meter BLE MAC — must be set in [private_credentials] in platformio_user.ini
// ---------------------------------------------------------------------------
#if !defined(SWITCHBOT_MAC) && !defined(NATIVE_ENV)
#error                                                                                             \
    "SWITCHBOT_MAC not set — add '-DSWITCHBOT_MAC=\"aa:bb:cc:dd:ee:ff\"' to [private_credentials] in platformio_user.ini"
#endif
#ifndef SWITCHBOT_MAC
#define SWITCHBOT_MAC ""
#endif

// ---------------------------------------------------------------------------
// Display timing
// ---------------------------------------------------------------------------
#define CLOCK_DISPLAY_MS 10000UL // ms to show clock before switching to temp
#define TEMP_DISPLAY_MS 5000UL   // ms to show temp before reverting to clock
#define SENSOR_STALE_MS 300000UL // BLE data older than this → "--.-" fallback
