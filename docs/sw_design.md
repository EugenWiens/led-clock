# Software Design

## Overview

The firmware is structured into independent modules with a hardware abstraction
layer (HAL) at the bottom. All hardware-dependent code is hidden behind abstract
C++ interfaces, enabling host-side unit testing without a physical device.

```
┌──────────────────────────────────────────────────────────────────┐
│                          main.cpp                                │
│  app_main(): creates static HAL + Matrix instances, runs loop    │
│  State machine: SHOW_CLOCK ↔ SHOW_TEMP                           │
└───────┬──────────┬───────────────┬────────────────┬─────────────┘
        │          │               │                │
        ▼          ▼               ▼                ▼
  ┌──────────┐ ┌────────┐ ┌────────────┐   ┌───────────────────┐
  │ Renderer │ │  Ntp   │ │ SwitchBot  │   │      Matrix       │
  │ (Phase3) │ │(Phase4)│ │  (Phase5)  │   │ Matrix(ILedHal&,  │
  └────┬─────┘ └────────┘ └────────────┘   │         IAdcHal&) │
       │                                   │  owns: Ldr m_ldr  │
       └───────────────────────────────────│  owns: CRGB[320]  │
              uses font.h                  └────────┬──────────┘
                                                    │
                           ┌────────────────────────┘
                           │
              ┌────────────▼────────────┐
              │          Ldr            │
              │  Ldr(IAdcHal&)          │
              │  rolling avg → brightness│
              └────────────┬────────────┘
                           │
          ┌────────────────┴────────────────┐
          ▼                                 ▼
   ┌─────────────┐                  ┌─────────────┐
   │  ILedHal    │                  │  IAdcHal    │
   │  (abstract) │                  │  (abstract) │
   └──────┬──────┘                  └──────┬──────┘
          │ device                         │ device
   ┌──────▼──────┐                  ┌──────▼──────┐
   │ FastLedHal  │                  │ EspAdcHal   │
   │ FastLED/RMT │                  │ adc_oneshot │
   └─────────────┘                  └─────────────┘
```

## Module Descriptions

### `src/config.h`
Compile-time constants for hardware pins, timing, and display thresholds.
User-specific settings (`WIFI_SSID`, `WIFI_PASS`, `SWITCHBOT_MAC`, `NTP_SERVER`,
`TIMEZONE`) are injected as `-D` build flags via `[private_credentials]` in
`platformio_user.ini`. A `#error` directive fires at compile time if any of these
is missing in a device build (`NATIVE_ENV` suppresses the check for host tests).

### `src/hal/led_hal.h/.cpp`
Abstract LED hardware interface.

```cpp
class ILedHal {
    virtual void init(CRGB* leds, uint16_t count) = 0;
    virtual void show() = 0;
    virtual void setBrightness(uint8_t brightness) = 0;
};
```

`FastLedHal` (device-only, `#ifndef NATIVE_ENV`) implements `ILedHal` using
`FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>()` via the ESP32 RMT peripheral.

`led_hal.h` also provides the `CRGB` type: FastLED's `CRGB` on device, a minimal
struct stub (`r`, `g`, `b` members) under `NATIVE_ENV`.

### `src/hal/adc_hal.h/.cpp`
Abstract ADC hardware interface.

```cpp
class IAdcHal {
    virtual void init() = 0;
    virtual int  read() = 0;   // 0–4095
};
```

`EspAdcHal` (device-only) implements `IAdcHal` using the ESP-IDF
`adc_oneshot` API on `ADC_UNIT_1` / `ADC_CHANNEL_2` (GPIO2), 12-bit,
`ADC_ATTEN_DB_12` (0–3.1 V input range).

### `src/display/ldr.h/.cpp`
Reads the LDR photoresistor, computes an 8-sample rolling average, and maps
the result linearly to a brightness value.

```cpp
class Ldr {
    explicit Ldr(IAdcHal& adcHal);
    void    init();                  // reset state + init ADC
    void    update(uint64_t nowMs);  // throttled to LDR_SAMPLE_MS
    uint8_t brightness() const;      // BRIGHTNESS_MIN–BRIGHTNESS_MAX
};
```

**Brightness mapping:** `BRIGHTNESS_MIN + avg * (BRIGHTNESS_MAX - BRIGHTNESS_MIN) / 4095`
(linear, clamped).

### `src/display/matrix.h/.cpp`
Owns the LED pixel buffer and orchestrates the HAL + LDR. Accepts HAL
dependencies via constructor injection (testable without hardware).

```cpp
class Matrix {
    Matrix(ILedHal& ledHal, IAdcHal& adcHal);
    void init();
    void clear();
    void setPixel(uint8_t matrixIdx, uint8_t col, uint8_t row, CRGB color);
    void show();
    void updateBrightness(uint64_t nowMs); // delegates to Ldr
};
```

**Coordinate mapping** (single row, left-to-right chaining):
```
Matrix index:   0          1          2          3          4
               [col 0-7]  [col 8-15] [col16-23] [col24-31] [col32-39]
LED index:      0-63       64-127     128-191    192-255    256-319

globalIdx = matrixIdx * 64 + row * 8 + col
```
Within each 8×8 module, pixels are indexed row-by-row, top-to-bottom,
left-to-right (standard WS2812B module layout).

`updateBrightness(nowMs)` delegates to `m_ldr.update(nowMs)` then calls
`m_ledHal.setBrightness(m_ldr.brightness())`.

### `src/display/font.h`
Static `const uint8_t FONT[11][7]` table.
- Indices 0–9 → digit glyphs
- Index 10 → colon `:`
- Each entry is 7 bytes (one per row); bit 4 (MSB of lower nibble) = leftmost pixel
  of a 5-pixel-wide glyph, giving a 5×7 active area within the 8×8 cell.

### `src/display/renderer.h/.cpp`
Renders content onto the matrix buffer. Calls `matrix.setPixel()`.
- `renderDigit(uint8_t matrixIdx, uint8_t digit, CRGB color)`
- `renderColon(bool on, CRGB color)` — matrix index 2
- `renderClock(uint8_t hh, uint8_t mm, bool colonOn)`
- `renderTemp(float tempC)` — formats as `"23.5"` spread across 4 matrices;
  falls back to `"--.-"` when `tempC == NAN`

### `src/network/ntp.h/.cpp`
- `ntpBegin()` — connects WiFi, configures SNTP via `esp_sntp_*`, sets timezone
  via `setenv("TZ", ...)` + `tzset()`, blocks until sync (max 30 s) or returns `false`
- `ntpGetTime(struct tm &t)` — fills `t` via `localtime_r()`; returns `false` if not synced
- `ntpMaintain()` — called from main loop; reconnects WiFi + re-syncs if lost

### `src/ble/switchbot.h/.cpp`
- `switchbotBegin()` — initialises BLEDevice, sets up continuous passive scan
  with a callback
- Callback filters by MAC (`SWITCHBOT_MAC`), parses manufacturer data byte
  offsets for temperature (°C, 0.1 resolution) and relative humidity (%)
- Results stored in a `SwitchBotData` struct with a `lastSeen` timestamp
- `switchbotGetData(SwitchBotData &out)` — returns cached data; `out.valid`
  is `false` if data is older than `BLE_STALE_THRESHOLD_S`

### `src/main.cpp`
ESP-IDF entry point `app_main()`. Creates HAL and `Matrix` as `static` locals
(BSS segment, not task stack) to avoid stack overflow with the 960-byte LED buffer:

```cpp
static FastLedHal ledHal;
static EspAdcHal  adcHal;
static Matrix     matrix{ledHal, adcHal};
matrix.init();
```

Followed by a FreeRTOS loop (`vTaskDelay(pdMS_TO_TICKS(33))`). Timing uses
`ms_now()` which wraps `esp_timer_get_time() / 1000`.

## Unit Tests

Host-side tests run via `pio test -e native`. Hardware calls are replaced by
C++ stub classes implementing the same abstract interfaces — no macros required.

| Test suite | File | What is tested |
|---|---|---|
| `test_display` | `test/test_display/test_matrix.cpp` | Coordinate mapping, `clear()`, `updateBrightness` wiring |
| `test_ldr` | `test/test_ldr/test_ldr.cpp` | Rolling average, linear brightness map, time gating, clamp |

Stub pattern:
```cpp
class StubLedHal final : public ILedHal {
    uint8_t brightness{128};
    void init(CRGB*, uint16_t) override {}
    void show() override {}
    void setBrightness(uint8_t b) override { brightness = b; }
};
```

Build filter for native env: `build_src_filter = -<*> +<display/matrix.cpp> +<display/ldr.cpp>`
(HAL `.cpp` files are excluded; stub classes in the test file provide the implementations.)

## Display State Machine

```
         ┌────────────────────────────────┐
         │           BOOT                 │
         │  init matrix, BLE, WiFi/NTP    │
         └────────────────┬───────────────┘
                          │
                          ▼
              ┌───────────────────────┐
         ┌───▶│      SHOW_CLOCK       │◀──────────────┐
         │    │  renderClock(hh,mm)   │               │
         │    │  colon blinks 1 Hz    │               │
         │    └──────────┬────────────┘               │
         │               │ after CLOCK_DISPLAY_MS          │
         │               ▼                            │
         │    ┌───────────────────────┐               │
         │    │      SHOW_TEMP        │               │
         │    │  renderTemp(°C)       │               │
         │    │  (or "--.-" fallback) │               │
         │    └──────────┬────────────┘               │
         │               │ after TEMP_DISPLAY_MS       │
         └───────────────┴────────────────────────────┘
```

## Data Flow

```
[WiFi / NTP]  ──► struct tm (hh, mm, ss)  ──► renderClock()  ──► FastLED strip
[SwitchBot BLE] ─► SwitchBotData.tempC    ──► renderTemp()   ──► FastLED strip
[LDR ADC]     ──► rolling average         ──► FastLED.setBrightness()
```

## Timing

| Task | Period | Mechanism |
|---|---|---|
| Display refresh | ~33 ms (30 FPS) | `FastLED.show()` + `vTaskDelay(pdMS_TO_TICKS(33))` |
| Colon toggle | 1 s | `ms_now()` delta in main loop |
| Clock→Temp switch | `CLOCK_DISPLAY_MS` (config) | `ms_now()` delta in main loop |
| Temp→Clock switch | `TEMP_DISPLAY_MS` (default 5 s) | `ms_now()` delta in main loop |
| LDR read | 500 ms | `Ldr::update()` throttled by `ms_now()` delta |
| BLE scan | continuous | ESP32 BLE stack background task |
| NTP maintain | on WiFi reconnect | checked in main loop via `ntpMaintain()` |

## Build Configuration

See `platformio.ini` (shared) and `platformio_user.ini` (gitignored, user-specific).
User-specific settings are collected in `[private_credentials]` and interpolated
via `${private_credentials.build_flags}` in the device environment.

```ini
[env:esp32-c6-devkitm-1]
platform  = espressif32
board     = esp32-c6-devkitm-1
framework = espidf
lib_deps  = fastled/FastLED @ ^3.7.0
build_flags = -std=gnu++2a -Wall -Wextra ${private_credentials.build_flags}

[env:native]          ; host-side unit tests — pio test -e native
platform = native
build_flags = -std=c++2a -Wall -Wextra -DNATIVE_ENV
build_src_filter = -<*> +<display/matrix.cpp> +<display/ldr.cpp>
```
