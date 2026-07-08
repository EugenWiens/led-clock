# Requirements

## Functional Requirements

| ID | Requirement | Priority |
|---|---|---|
| FA-01 | The system shall display the current time in HH:MM format on the LED matrix | Must |
| FA-02 | The time shall be synchronized via NTP over WiFi on startup | Must |
| FA-03 | The system shall automatically reconnect to WiFi and re-sync NTP after connection loss | Must |
| FA-04 | The display shall alternate between time and temperature at a configurable interval | Must |
| FA-05 | The system shall receive temperature and humidity data from a SwitchBot Meter via passive BLE scan | Must |
| FA-06 | The SwitchBot device to read from shall be identified by its BLE MAC address, configured in `config.h` | Must |
| FA-07 | The colon between hours and minutes shall blink once per second | Should |
| FA-08 | When BLE sensor data is older than a configurable threshold (default 5 minutes), the display shall show `--.-°C` as a fallback | Must |
| FA-09 | The LED brightness shall be adjusted automatically based on ambient light measured by an LDR | Must |
| FA-10 | Brightness adaptation shall use a rolling average of LDR readings to avoid flickering | Should |
| FA-11 | All user-configurable parameters (WiFi, timezone, pins, intervals, MAC) shall be centralised in `config.h` | Must |
| FA-12 | The system shall be buildable and flashable using PlatformIO CLI (`pio run --target upload`) | Must |

## Non-Functional Requirements

| ID | Requirement | Category |
|---|---|---|
| NFA-01 | The firmware shall be written in C++ using the Arduino framework on ESP32-C6 | Technology |
| NFA-02 | The LED driver shall use FastLED to ensure compatibility with the ESP32-C6 RMT peripheral | Technology |
| NFA-03 | The firmware shall compile without errors or warnings under PlatformIO with the `espressif32` platform | Quality |
| NFA-04 | No WiFi credentials or secrets shall be committed to version control; they reside only in `config.h` | Security |
| NFA-05 | Brightness adjustment shall respond within 1 second of a significant ambient light change | Performance |
| NFA-06 | The BLE scan shall run continuously in the background without blocking the display refresh loop | Performance |
| NFA-07 | The NTP sync shall complete within 30 seconds of a successful WiFi connection | Performance |
| NFA-08 | The display refresh rate shall be at least 30 FPS to avoid visible flicker | Performance |
| NFA-09 | The firmware shall handle WiFi and BLE failures gracefully — the clock shall remain functional without network connectivity after initial sync | Robustness |
| NFA-10 | Memory usage shall remain within the ESP32-C6 heap limits; no dynamic allocation in the main display loop | Resource |
| NFA-11 | The hardware design shall use a level shifter (74HCT125) to protect WS2812B DIN from 3.3V logic | Hardware |
| NFA-12 | The power supply shall be rated for at least 3 A at 5 V to safely power all 320 LEDs | Hardware |
