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
- [ ] FastLED: 320 LEDs (5x64), GPIO8
- [ ] Coordinate mapping: (matrixIndex, col, row) → global LED index
- [ ] Core functions: clear(), setPixel(), show()
- [ ] LDR: analogRead(GPIO2) every 500ms, rolling average (8 samples), map to FastLED setBrightness(10–255)

### Phase 3: Font & Renderer (src/display/)
- [ ] 5x7 bitmap font as uint8_t[11][7] for digits 0–9 and ':'
- [ ] renderDigit(matrixIndex, digit, color)
- [ ] renderClock(hh, mm) — digits on matrices 0, 1, 2 (colon), 3, 4
- [ ] renderTemp(float temp) — format: "23.5" or "--.-"

### Phase 4: WiFi + NTP (src/network/)
- [ ] configTzTime() with POSIX string (e.g. "CET-1CEST,M3.5.0,M10.5.0/3")
- [ ] getLocalTime() poll loop until sync
- [ ] WiFi reconnect on connection loss

### Phase 5: SwitchBot BLE (src/ble/) — parallel to Phase 4
- [ ] Passive BLE scan, callback on advertisement
- [ ] Filter by configured MAC (SWITCHBOT_MAC in config.h)
- [ ] Parse temperature/humidity from manufacturer data
- [ ] Cache result in global struct with timestamp for staleness check

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
