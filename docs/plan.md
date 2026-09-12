<!-- SPDX-FileCopyrightText: 2026 Eugen Wiens -->
<!-- SPDX-License-Identifier: MIT -->

# Plan: ESP32-C6 WS2812 LED Clock with SwitchBot

## Decisions
- Board: esp32-c6-devkitm-1 (PlatformIO, ESP-IDF)
- 5x 8x8 WS2812B in a single row (40x8 pixels total, 320 LEDs)
- Display: HH:MM + alternates with temperature every N seconds
- Time source: NTP via WiFi (configTzTime)
- SwitchBot BLE MAC: fixed in config.h
- SwitchBot connection: passive BLE scan, Manufacturer Data 0x0969 / service fd3d
- LED driver: ESP-IDF `esp_driver_rmt` component for WS2812B
- LED data pin: GPIO8
- Brightness: LDR photoresistor on GPIO2 (ADC1), analogRead + rolling average
- LDR circuit: voltage divider LDR + 10kΩ pull-down to 3.3V
- Font: 5x7 bitmap glyphs per 8x8 matrix
- Circuit diagram: ASCII/Markdown in docs/circuit.md
- 3D housing: FreeCAD

## Project Structure

```
led_clock/
├── README.md                     Project description, features, quickstart, hardware list
├── platformio.ini
├── src/
│   ├── main.cpp
│   ├── Application.h/.cpp       Application composition and main loop
│   ├── config.h                  Hardware pins, timing, and display constants
│   ├── display/
│   │   ├── Display.h/.cpp        Display facade, initialization, and state machine
│   │   ├── DisplayState.h        Display state enumeration
│   │   ├── Matrix.h/.cpp         RMT HAL wrapper + coordinate mapping
│   │   ├── Font.h                5x7 bitmap data
│   │   └── Renderer.h/.cpp       Clock/temperature rendering
│   ├── network/
│   │   └── Ntp.h/.cpp            WiFi + NTP sync
│   ├── hal/
│   │   ├── CRGB.h
│   │   ├── ILedHal.h
│   │   ├── EspLedStripHal.h/.cpp
│   │   ├── IAdcHal.h
│   │   └── EspAdcHal.h/.cpp
│   └── ble/
│       ├── Bluetooth.h/.cpp       Generic passive BLE scanner
│       ├── BluetoothAdvertisement.h
│       ├── SwitchBot.h/.cpp       SwitchBot filter and parser
│       └── SwitchBotData.h
├── docs/
│   ├── plan.md                   Project plan (this file)
│   ├── circuit.md                ASCII circuit schematic
│   ├── sw_design.md              SW architecture, modules, data flow, state machine
│   └── requirements.md           Requirements overview (functional + non-functional)
└── cad/
    └── housing_description.md    FreeCAD dimensions sheet
```

## Unit Test Plan (native, hardware-free)

### Goal and test boundary

- Run all unit tests with Unity through `pio test -e native`.
- Test pure logic and module behavior on the host; do not require an ESP32, LEDs, WiFi, BLE radio, or an ADC.
- Keep RMT, WiFi, and NimBLE calls outside native unit tests. Verify those integrations separately on the target hardware.
- Use `StubLedHal` and `StubAdcHal` for display and brightness tests. Add small fakes for time and sensor sources where needed.

### Test suites and cases

#### Matrix and LDR (`test/test_display/` and `test/test_ldr/`)

- Verify matrix coordinate mapping for all edges and all five matrix indices.
- Verify out-of-range coordinates are ignored without corrupting neighboring LEDs.
- Verify `clear()` resets the complete 320 LED buffer.
- Verify brightness sampling is time-gated and happens at the exact sample interval.
- Verify minimum, midpoint, and maximum ADC values map to the configured brightness range.
- Verify the rolling average converges, evicts the oldest sample, and does not exceed the configured limits.

#### Renderer (`test/test_renderer/`)

- Verify digit, colon, dash, and degree glyph placement on the intended matrix.
- Verify clock digit placement for representative hours and minutes.
- Verify colon on/off behavior and color selection.
- Verify temperature rendering for positive, zero, negative, single-digit, and boundary values.
- Verify NaN and out-of-range temperatures render the fallback `--.-` display.
- Add a regression test that rendering a new value clears pixels left by the previous value.

#### SwitchBot parser (`test/test_ble/`)

- Verify null, empty, short, and minimum-length payload handling.
- Verify positive, negative, zero, and boundary temperature values.
- Verify humidity decoding and the `valid` flag.
- Verify fresh data, the exact stale threshold, old data, and invalid data.
- Verify malformed or unsupported payload data does not produce a valid reading.

#### Display state machine (`test/test_display_state/`)

- Verify initialization starts in `SHOW_CLOCK` with the colon enabled.
- Verify the clock-to-temperature transition just before and exactly at `CLOCK_DISPLAY_MS`.
- Verify the temperature-to-clock transition just before and exactly at `TEMP_DISPLAY_MS`.
- Verify the colon toggles at `COLON_TOGGLE_MS` and remains stable before the boundary.
- Verify a valid NTP time is rendered in clock mode.
- Verify valid sensor data is rendered in temperature mode and unavailable data renders the fallback.
- Verify repeated `update()` calls at the same timestamp are deterministic.

The state-machine tests require `Display` to receive fakeable time and sensor sources, or for the timing logic to be extracted into a small pure-logic class. Add `Display.cpp` and the required test seam to the native source filter only after that boundary exists.

### Implementation order

1. Keep the existing Matrix, LDR, Renderer, and SwitchBot tests green as the baseline.
2. Add the renderer regression and malformed-payload boundary cases.
3. Introduce the smallest fakeable boundary for NTP and SwitchBot access in `Display`.
4. Add the display state-machine suite with exact-boundary timing tests.
5. Run native tests with warnings treated as errors and fix any nondeterministic shared state.
6. Run the coverage configuration and raise every test module to at least 80% line coverage.
7. Run the complete native test and coverage commands in CI for every pull request.

### Definition of done

- `pio test -e native` passes without hardware.
- Each test is independent, deterministic, and uses controlled timestamps and input data.
- All pure-logic behavior required by QA-11 has a corresponding test.
- The native test build does not include ESP-IDF-only implementation files.
- `pio test -e native_coverage` produces coverage data and every test module reaches at least 80% line coverage.

## Phases

### Phase 1: PlatformIO Setup
- [x] platformio.ini: board=esp32-c6-devkitm-1, framework=espidf, platform=espressif32
- [x] Dependencies: ESP-IDF RMT driver
- [x] Create project directory structure
- [x] config.h with hardware pins, timing, and display constants
- [x] Local platformio_user.ini build flags for WiFi, timezone, NTP, and SwitchBot MAC

### Phase 2: LED Matrix Driver + LDR (src/display/)
- [x] WS2812B: 320 LEDs (5x64), GPIO8
- [x] Coordinate mapping: (matrixIndex, col, row) → global LED index
- [x] Core functions: clear(), setPixel(), show()
- [x] LDR: adc_oneshot_read (GPIO2/ADC_CHANNEL_2) every 500ms, rolling average (8 samples), map to LED brightness (10–255)
- [x] HAL interfaces and ESP-IDF implementations — test-double boundary for unit tests
- [x] Unit tests: test/test_display/test_matrix.cpp (14 test cases)

### Phase 3: Font & Renderer (src/display/)
- [x] 5x7 bitmap font as `Font::FONT[13][7]` — digits 0–9, ':', '-', '°'
- [x] `Renderer(Matrix&)` class with constructor injection
- [x] `renderGlyph(matrixIdx, fontIdx, color)` — col_offset=1, bits 4–0 per row
- [x] `renderClock(hh, mm, colonOn)` — matrices 0,1 (hour), 2 (colon), 3,4 (minute)
- [x] `renderTemp(float)` — matrices 0,1 (int), 2 (dot), 3 (frac), 4 (°); NaN/OOB → "--.-°"
- [x] `inline constexpr` colours: `CLOCK_COLOR` (amber), `TEMP_COLOR` (cyan)
- [x] Unit tests: `test/test_renderer/test_renderer.cpp` (14 test cases)

### Phase 4: WiFi + NTP (src/network/)
- [x] configTzTime() with POSIX string (e.g. "CET-1CEST,M3.5.0,M10.5.0/3")
- [x] getLocalTime() poll loop until sync
- [x] WiFi reconnect on connection loss

### Phase 5: SwitchBot BLE (src/ble/) — parallel to Phase 4
- [x] Enable NimBLE in `sdkconfig.esp32-c6-devkitm-1` (`CONFIG_BT_ENABLED`, `CONFIG_BT_NIMBLE_ENABLED`, `CONFIG_BT_NIMBLE_ROLE_OBSERVER`)
- [x] `class SwitchBotData { float tempC; uint8_t humidity; uint64_t lastSeenMs; bool valid; }`
- [x] `SwitchBot::parseServiceData(data, len, out)` — static class method decoding UUID-0xFD3D service data
- [x] `SwitchBot::isStale(data, nowMs)` — static staleness check against `SENSOR_STALE_MS`
- [x] Unit tests: `test/test_ble/test_switchbot.cpp` (15 test cases — length guards, temperature, humidity, staleness)
- [x] `Bluetooth` — NimBLE passive scan via `nimble_port_freertos_init()` and raw advertisement callback
- [x] `SwitchBot` — MAC filter, UUID-0xFD3D parser, thread-safe data store, and staleness check
- [x] `Bluetooth`, `SwitchBot`, `Ntp`, and `Display` are constructed and initialized by `Application`
- [ ] Verify byte offsets against physical SwitchBot Meter device (serial log `tempC` + `humidity`)

### Phase 6: Display Logic (src/display/)
- [x] State machine: SHOW_CLOCK / SHOW_TEMP
- [x] Configurable switch interval (default: 5s temperature, then back to clock)
- [x] Colon toggle every second
- [x] Fallback "--.-" when BLE data is older than 5 minutes

### Phase 7: Circuit Diagram (docs/circuit.md)
- [ ] ASCII schematic: ESP32-C6 GPIO8 → 330Ω → 74HCT125 → Matrix 0 DIN
- [ ] DOUT0→DIN1→…→DIN4 (chain)
- [ ] 5V/3A power supply → VCC all matrices, 100µF+100nF per matrix
- [ ] LDR voltage divider: 3.3V → LDR → GPIO2 → 10kΩ → GND

### Phase 8: Documentation ✅
- [x] README.md: project title, features, hardware list, wiring reference, build guide, configuration
- [x] docs/requirements.md: functional requirements (FA-01..12), non-functional requirements (NFA-01..12)
- [x] docs/sw_design.md: module overview, ASCII component diagram, data flow, state machine diagram

### Phase 9: 3D Housing (cad/housing_description.md)
- [ ] Outer dimensions: approx. 215mm x 50mm x 30mm
- [ ] Front: diffuser panel (3mm frosted acrylic)
- [ ] Rear: USB-C cutout, cable routing
- [ ] Mounting points for 5 LED modules
- [ ] LDR opening front top
