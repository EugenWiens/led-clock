# ESP32-C6 WS2812 LED Clock

[![CI](https://github.com/EugenWiens/led-clock/actions/workflows/ci.yml/badge.svg)](https://github.com/EugenWiens/led-clock/actions/workflows/ci.yml)
[![CodeQL](https://github.com/EugenWiens/led-clock/actions/workflows/codeql.yml/badge.svg?branch=main)](https://github.com/EugenWiens/led-clock/actions/workflows/codeql.yml)
[![CodeFactor](https://www.codefactor.io/repository/github/EugenWiens/led-clock/badge)](https://www.codefactor.io/repository/github/EugenWiens/led-clock)

An ESP32-C6-based LED matrix clock displaying time and ambient temperature.
The display alternates between the current time (NTP-synced) and temperature
data received from a SwitchBot Meter via Bluetooth LE.

## Features

- **Time display** — HH:MM on 5× 8×8 WS2812B LED matrices (40×8 pixels total)
- **Temperature display** — passive BLE scan of a SwitchBot Meter, alternates with clock
- **Auto-brightness** — LDR photoresistor continuously adjusts LED brightness
- **NTP sync** — time is kept accurate via WiFi (no RTC module required)
- **Fallback display** — shows `--.-°C` when no fresh BLE data is available (>5 min)
- **Blinking colon** — toggles every second as a visual seconds indicator

## Hardware

| Component | Qty | Notes |
|---|---|---|
| ESP32-C6 DevKitC-1 | 1 | RISC-V, WiFi 6, Bluetooth 5 LE |
| 8×8 WS2812B LED matrix module | 5 | Chained in a single row (320 LEDs total) |
| LDR photoresistor | 1 | e.g. GL5528 |
| 74HCT125 level shifter | 1 | 3.3V → 5V for WS2812B data line |
| Resistor 330 Ω | 1 | Data line protection |
| Resistor 10 kΩ | 1 | LDR pull-down (voltage divider) |
| Capacitor 100 µF | 5 | Electrolytic, one per matrix |
| Capacitor 100 nF | 5 | Ceramic, one per matrix |
| Power supply 5 V / 3 A | 1 | For LED matrices |
| USB-C cable | 1 | For ESP32-C6 power/programming |
| SwitchBot Meter / Meter Plus | 1 | BLE temperature/humidity sensor |

## Wiring Quick Reference

| ESP32-C6 Pin | Connection |
|---|---|
| GPIO 8 | 330 Ω → 74HCT125 input → Matrix 0 DIN |
| GPIO 2 | LDR voltage divider output (ADC1) |
| 3.3V | LDR top rail |
| GND | 10 kΩ pull-down to GND |
| 5V (external) | VCC of all 5 matrices, 74HCT125 VCC |

Matrix chaining: `Matrix 0 DOUT → Matrix 1 DIN → … → Matrix 4 DIN`

See [docs/circuit.md](docs/circuit.md) for the full ASCII schematic.

## Project Structure

```
led_clock/
├── platformio.ini          PlatformIO build config
├── src/
│   ├── main.cpp            ESP-IDF entry point
│   ├── Application.h/.cpp  Application composition and main loop
│   ├── config.h            Hardware, timing, and display constants
│   ├── display/
│   │   ├── Display.h/.cpp  Display facade and state machine
│   │   ├── DisplayState.h  Display state enumeration
│   │   ├── Matrix.h/.cpp   Matrix buffer + coordinate mapping
│   │   ├── Font.h          5×7 bitmap font (digits + colon)
│   │   └── Renderer.h/.cpp Clock/temperature rendering
│   ├── network/
│   │   └── Ntp.h/.cpp      WiFi connection + NTP sync
│   ├── hal/
│   │   ├── CRGB.h          RGB pixel value
│   │   ├── ILedHal.h       LED hardware interface
│   │   ├── EspLedStripHal.h/.cpp  ESP-IDF RMT LED implementation
│   │   ├── IAdcHal.h       ADC hardware interface
│   │   └── EspAdcHal.h/.cpp         ESP-IDF ADC implementation
│   └── ble/
│       ├── Bluetooth.h/.cpp             Generic passive BLE scanner
│       ├── BluetoothAdvertisement.h    Advertisement value
│       ├── SwitchBot.h/.cpp             SwitchBot parser and data store
│       └── SwitchBotData.h             Sensor reading value
├── docs/
│   ├── requirements.md     Functional and non-functional requirements
│   ├── sw_design.md        Software architecture and design
│   └── circuit.md          ASCII circuit schematic
└── cad/
    └── housing_description.md  3D housing dimensions for FreeCAD
```

## Quickstart

### 1. Configure

Create or edit `platformio_user.ini` in the project root. This gitignored file
provides the device-specific values as PlatformIO build flags:

```ini
[private_credentials]
build_flags =
    '-DWIFI_SSID="your-ssid"'
    '-DWIFI_PASS="your-password"'
    '-DNTP_SERVER="pool.ntp.org"'
    '-DTIMEZONE="CET-1CEST,M3.5.0,M10.5.0/3"'
    '-DSWITCHBOT_MAC="AA:BB:CC:DD:EE:FF"'
```

Do not commit this file or put private values in `src/config.h`.

### 2. Build & Upload

```bash
# Install PlatformIO CLI if needed
pip install platformio

# Build
pio run

# Upload to ESP32-C6
pio run --target upload

# Monitor serial output
pio device monitor --baud 115200
```

### 3. First Boot

On first boot the device will:
1. Connect to WiFi and sync time via NTP
2. Start a passive BLE scan for the configured SwitchBot Meter
3. Display `HH:MM` — automatically switching to temperature every 5 seconds

## Configuration Reference

WiFi, NTP, timezone, and SwitchBot settings are supplied through
`platformio_user.ini`. Hardware and timing constants remain in `src/config.h`.

| Constant | Default | Description |
|---|---|---|
| `WIFI_SSID` | — | WiFi network name |
| `WIFI_PASS` | — | WiFi password |
| `NTP_SERVER` | `"pool.ntp.org"` | NTP server hostname |
| `TIMEZONE` | `"CET-1CEST,M3.5.0,M10.5.0/3"` | POSIX timezone string |
| `LED_DATA_PIN` | `8` | GPIO pin for WS2812B data |
| `LED_NUM_MATRICES` | `5` | Number of 8×8 matrices |
| `LDR_PIN` | `2` | ADC pin for LDR |
| `LDR_MIN_BRIGHTNESS` | `10` | Minimum LED brightness (0–255) |
| `LDR_MAX_BRIGHTNESS` | `200` | Maximum LED brightness (0–255) |
| `SWITCHBOT_MAC` | — | BLE MAC address of SwitchBot Meter |
| `TEMP_DISPLAY_DURATION_MS` | `5000` | How long temperature is shown (ms) |
| `BLE_STALE_THRESHOLD_S` | `300` | Seconds before BLE data is considered stale |

## Dependencies

- ESP-IDF `esp_driver_rmt` — WS2812B LED driver
- ESP-IDF WiFi, NimBLE, SNTP, ADC and FreeRTOS components

## License

MIT
