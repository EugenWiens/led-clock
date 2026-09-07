#pragma once
#include <cstddef>
#include <cstdint>

#include "bluetooth.h"

#ifndef NATIVE_ENV
#include "freertos/FreeRTOS.h"
#endif

// ---------------------------------------------------------------------------
// Data — no hardware dependency; safe in native test environment
// ---------------------------------------------------------------------------

/// Holds a single decoded SwitchBot Meter reading.
struct SwitchBotData {
    float tempC{0.0f};      ///< Temperature in °C
    uint8_t humidity{0};    ///< Relative humidity in %
    uint64_t lastSeenMs{0}; ///< esp_timer_get_time()/1000 at time of receipt
    bool valid{false};      ///< False if never received or stale
};

/// Parse the SwitchBot Meter service-data payload (UUID 0xFD3D, bytes after UUID).
/// Format (verified against SwitchBot Open Protocol rev 1.2):
///   [0] device byte  [1] status  [2] battery
///   [3] bits[3:0] = temp tenths (0–9 → 0.0–0.9 °C)
///   [4] bits[6:0] = temp integer (0–99), bit[7] = 1 if positive
///   [5] bits[6:0] = humidity %
/// @return true if @p len >= 6 and data was decoded successfully.
[[nodiscard]] bool parseSwitchBotServiceData(const uint8_t* data, size_t len, SwitchBotData& out);

/// @return true if @p d.lastSeenMs is more than SENSOR_STALE_MS ms before @p nowMs.
[[nodiscard]] bool isSwitchBotStale(const SwitchBotData& d, uint64_t nowMs);

class SwitchBot {
public:
    explicit SwitchBot(Bluetooth& bluetooth);

    /// Register the SwitchBot advertisement handler with the Bluetooth scanner.
    void init();

    /// Thread-safe read of the latest decoded SwitchBot data.
    /// Sets out.valid = false if no data has ever arrived or if the data is stale.
    [[nodiscard]] bool getData(SwitchBotData& out);

private:
    static void advertisementHandler(const BluetoothAdvertisement& advertisement, void* context);
    void handleAdvertisement(const BluetoothAdvertisement& advertisement);

    Bluetooth& m_bluetooth;
    uint8_t m_targetMac[6]{};
    bool m_macParsed{false};
    SwitchBotData m_data{};

#ifndef NATIVE_ENV
    portMUX_TYPE m_mux = portMUX_INITIALIZER_UNLOCKED;
#endif
};
