# Software Design

## Overview

The firmware is structured into four independent modules plus a main loop.
Each module owns its data and exposes a minimal interface to `main.cpp`.

```
┌─────────────────────────────────────────────────────────┐
│                        main.cpp                         │
│   State machine: SHOW_CLOCK ↔ SHOW_TEMP                 │
│   Orchestrates all modules, drives display loop         │
└───────┬──────────┬──────────┬──────────────────┬────────┘
        │          │          │                  │
        ▼          ▼          ▼                  ▼
  ┌──────────┐ ┌────────┐ ┌────────┐   ┌────────────────┐
  │ renderer │ │  ntp   │ │switchbot│  │     matrix     │
  │          │ │        │ │  (BLE) │   │  + LDR driver  │
  │render-   │ │WiFi    │ │passive │   │                │
  │Clock()   │ │connect │ │scan    │   │FastLED wrapper │
  │render-   │ │NTP sync│ │parse   │   │coord mapping   │
  │Temp()    │ │getTime │ │getData │   │setBrightness   │
  └────┬─────┘ └────────┘ └────────┘   └────────────────┘
       │                                       │
       └───────────────────────────────────────┘
              uses font.h (static bitmap data)
```

## Module Descriptions

### `src/config.h`
Compile-time configuration. All hardware pins, credentials, intervals, and
thresholds are defined here as `#define` constants. No logic.

### `src/display/matrix.h/.cpp`
Wraps FastLED. Responsibilities:
- Initialise the LED strip (320 LEDs on GPIO 8 via RMT)
- Map a logical `(matrixIndex, col, row)` coordinate to the physical LED index
- Read the LDR via ADC every 500 ms, compute an 8-sample rolling average,
  and call `FastLED.setBrightness()`
- Expose `clear()`, `setPixel(matrixIdx, col, row, color)`, `show()`

**Coordinate mapping** (single row, left-to-right chaining):
```
Matrix index:   0          1          2          3          4
               [col 0-7]  [col 8-15] [col16-23] [col24-31] [col32-39]
LED index:      0-63       64-127     128-191    192-255    256-319
```
Within each 8×8 module, pixels are indexed row by row, top-to-bottom,
left-to-right (standard WS2812B module layout).

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
- `ntpBegin()` — connects WiFi, calls `configTzTime()`, blocks until sync
  (max 30 s) or returns `false`
- `ntpGetTime(struct tm &t)` — thin wrapper around `getLocalTime()`
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
Initialisation sequence and main display loop.

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
         │               │ every TEMP_DISPLAY_DURATION_MS  │
         │               ▼                            │
         │    ┌───────────────────────┐               │
         │    │      SHOW_TEMP        │               │
         │    │  renderTemp(°C)       │               │
         │    │  (or "--.-" fallback) │               │
         │    └──────────┬────────────┘               │
         │               │ after TEMP_DISPLAY_DURATION_MS  │
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
| Display refresh | ~33 ms (30 FPS) | `FastLED.show()` in main loop |
| Colon toggle | 1 s | `millis()` delta in main loop |
| Display state switch | configurable (default 5 s) | `millis()` delta in main loop |
| LDR read | 500 ms | `millis()` delta in `matrix.cpp` |
| BLE scan | continuous | ESP32 BLE stack background task |
| NTP maintain | on WiFi reconnect | checked in main loop via `ntpMaintain()` |

## Build Configuration (`platformio.ini`)

```ini
[env:esp32c6]
platform  = espressif32
board     = esp32-c6-devkitc-1
framework = arduino
lib_deps  = fastled/FastLED
monitor_speed = 115200
```
