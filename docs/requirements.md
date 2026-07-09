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
| FA-07 | When BLE sensor data is older than a configurable threshold (default 5 minutes), the display shall show `--.-°C` as a fallback | Must |
| FA-08 | The LED brightness shall be adjusted automatically based on ambient light measured by an LDR | Must |
| FA-09 | Brightness adaptation shall use a rolling average of LDR readings to avoid flickering | Should |
| FA-10 | All user-configurable parameters (WiFi, timezone, pins, intervals, MAC) shall be centralised in `config.h` | Must |
| FA-11 | The system shall be buildable and flashable using PlatformIO CLI (`pio run --target upload`) | Must |

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
| NFA-11 | The power supply shall be rated for at least 3 A at 5 V to safely power all 320 LEDs | Hardware |

## Quality Assurance Requirements

### Static Code Analysis

| ID | Requirement | Tool | Priority |
|---|---|---|---|
| QA-01 | All C++ source files shall be analysed with **cppcheck** (free, OSS) on every CI run; any error-level finding shall fail the build | [cppcheck](https://cppcheck.sourceforge.io/) | Must |
| QA-02 | The codebase shall compile with zero warnings using `-Wall -Wextra -Wpedantic` under PlatformIO | GCC / Clang (bundled with PlatformIO) | Must |
| QA-03 | All source files shall be checked with **clang-tidy** using a project-level `.clang-tidy` config; findings of severity `warning` or above shall block merging | [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) (LLVM, free) | Should |
| QA-04 | A `.clang-format` file shall be committed to the repository; CI shall verify conformance with `clang-format --dry-run --Werror` | [clang-format](https://clang.llvm.org/docs/ClangFormat.html) (LLVM, free) | Must |
| QA-05 | No `TODO`, `FIXME`, or `HACK` comments shall remain in production code at release; a **grep**-based CI step shall enforce this | grep (POSIX, free) | Should |

### Unit Testing

| ID | Requirement | Tool | Priority |
|---|---|---|---|
| QA-10 | Unit tests shall be written using the **Unity** test framework via PlatformIO's native test environment | [Unity](https://github.com/ThrowTheSwitch/Unity) (MIT, free) | Must |
| QA-11 | All pure-logic functions (time formatting, brightness mapping, BLE data-age check, rolling-average calculation) shall have corresponding unit tests | Unity / PlatformIO | Must |
| QA-12 | Unit tests shall run on the host (native) environment (`pio test -e native`) without requiring hardware | PlatformIO native env (free) | Must |
| QA-13 | Each unit test module shall achieve a minimum line coverage of 80 %; coverage shall be measured with **gcov** and reported via **lcov** / **genhtml** | [gcov](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html) + [lcov](https://github.com/linux-test-project/lcov) (GPLv2, free) | Should |
| QA-14 | Tests shall be independent and deterministic — no shared mutable state between test cases | Unity | Must |
| QA-15 | Hardware-dependent code (FastLED, WiFi, BLE) shall be abstracted behind interfaces so it can be replaced with **FFF** fake functions in the native test environment | [FFF](https://github.com/meekrosoft/fff) (MIT, free) | Must |

### Code Review & Process

| ID | Requirement | Tool | Priority |
|---|---|---|---|
| QA-20 | All changes to `main` shall be made via pull requests; at least one peer review approval shall be required before merging | GitHub (free for public OSS) | Should |
| QA-21 | Static analysis and unit test steps shall execute automatically on every pull request via **GitHub Actions** | [GitHub Actions](https://github.com/features/actions) (free for public repos) | Should |
| QA-22 | The CI workflow file (`.github/workflows/ci.yml`) shall be version-controlled alongside the source code | GitHub Actions (YAML) | Must |
| QA-23 | The repository shall be connected to **CodeFactor** for continuous automated code-quality grading; the project shall maintain a minimum grade of **B** and the CodeFactor badge shall be displayed in `README.md` | [CodeFactor](https://www.codefactor.io/) (free for public repos) | Should |
