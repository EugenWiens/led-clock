# Plan: ESP32-C6 WS2812 LED Clock with SwitchBot

## Decisions
- Board: esp32-c6-devkitm-1 (PlatformIO, ESP-IDF)
- 5x 8x8 WS2812B in a single row (40x8 pixels total, 320 LEDs)
- Display: HH:MM + alternates with temperature every N seconds
- Time source: NTP via WiFi (configTzTime)
- SwitchBot BLE MAC: fixed in config.h
- SwitchBot connection: passive BLE scan, Manufacturer Data 0x0969 / service fd3d
- LED library: FastLED (RMT5 support on ESP32-C6)
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
│   ├── config.h                  WiFi credentials, pins, timezone, SwitchBot MAC
│   ├── display/
│   │   ├── matrix.h/.cpp         FastLED wrapper + coordinate mapping
│   │   ├── font.h                5x7 bitmap data
│   │   └── renderer.h/.cpp       Clock/temperature rendering
│   ├── network/
│   │   └── ntp.h/.cpp            WiFi + NTP sync
│   └── ble/
│       └── switchbot.h/.cpp      BLE scanner + parser
├── docs/
│   ├── plan.md                   Project plan (this file)
│   ├── circuit.md                ASCII circuit schematic
│   ├── sw_design.md              SW architecture, modules, data flow, state machine
│   └── requirements.md           Requirements overview (functional + non-functional)
└── cad/
    └── housing_description.md    FreeCAD dimensions sheet
```

## Phases

### Phase 1: PlatformIO Setup
- [x] platformio.ini: board=esp32-c6-devkitm-1, framework=espidf, platform=espressif32
- [x] Dependencies: FastLED
- [x] Create project directory structure
- [x] config.h with WiFi credentials, timezone, LED pin, LDR pin, SwitchBot MAC

### Phase 2: LED Matrix Driver + LDR (src/display/)
- [x] FastLED: 320 LEDs (5x64), GPIO8
- [x] Coordinate mapping: (matrixIndex, col, row) → global LED index
- [x] Core functions: clear(), setPixel(), show()
- [x] LDR: adc_oneshot_read (GPIO2/ADC_CHANNEL_2) every 500ms, rolling average (8 samples), map to FastLED setBrightness(10–255)
- [x] HAL wrapper (src/hal/led_hal, adc_hal) — FFF-mockable boundary for unit tests
- [x] Unit tests: test/test_display/test_matrix.cpp (14 test cases)

### Phase 3: Font & Renderer (src/display/)
- [x] 5x7 bitmap font as `uint8_t FONT[13][7]` — digits 0–9, ':', '-', '°' (namespace `font`)
- [x] `Renderer(Matrix&)` class with constructor injection
- [x] `renderGlyph(matrixIdx, fontIdx, color)` — col_offset=1, bits 4–0 per row
- [x] `renderClock(hh, mm, colonOn)` — matrices 0,1 (hour), 2 (colon), 3,4 (minute)
- [x] `renderTemp(float)` — matrices 0,1 (int), 2 (dot), 3 (frac), 4 (°); NaN/OOB → "--.-°"
- [x] `inline constexpr` colours: `CLOCK_COLOR` (amber), `TEMP_COLOR` (cyan)
- [x] Unit tests: `test/test_renderer/test_renderer.cpp` (14 test cases)

### Phase 4: WiFi + NTP (src/network/)
- [ ] configTzTime() with POSIX string (e.g. "CET-1CEST,M3.5.0,M10.5.0/3")
- [ ] getLocalTime() poll loop until sync
- [ ] WiFi reconnect on connection loss

### Phase 5: SwitchBot BLE (src/ble/) — parallel to Phase 4
- [x] Enable NimBLE in `sdkconfig.esp32-c6-devkitm-1` (`CONFIG_BT_ENABLED`, `CONFIG_BT_NIMBLE_ENABLED`, `CONFIG_BT_NIMBLE_ROLE_OBSERVER`)
- [x] `struct SwitchBotData { float tempC; uint8_t humidity; uint64_t lastSeenMs; bool valid; }`
- [x] `parseSwitchBotServiceData(data, len, out)` — pure function, decodes UUID-0xFD3D service-data payload
- [x] `isSwitchBotStale(data, nowMs)` — pure staleness check against `SENSOR_STALE_MS`
- [x] Unit tests: `test/test_ble/test_switchbot.cpp` (15 test cases — length guards, temperature, humidity, staleness)
- [x] `switchbotBegin()` — NimBLE passive scan via `nimble_port_freertos_init()`; MAC filtered by `SWITCHBOT_MAC`; ad-data parsed for service UUID 0xFD3D
- [x] `switchbotGetData(out)` — thread-safe read via `portMUX_TYPE` critical section; staleness applied on read
- [x] `switchbotBegin()` called from `app_main()` before main loop
- [ ] Verify byte offsets against physical SwitchBot Meter device (serial log `tempC` + `humidity`)

### Phase 6: Display Logic (src/main.cpp)
- [ ] State machine: SHOW_CLOCK / SHOW_TEMP
- [ ] Configurable switch interval (default: 5s temperature, then back to clock)
- [ ] Colon toggle every second
- [ ] Fallback "--.-" when BLE data is older than 5 minutes

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
