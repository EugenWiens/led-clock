// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>

#include "Bluetooth.h"
#include "SwitchBotData.h"
#include "display/ISensorSource.h"

#ifndef NATIVE_ENV

#include "freertos/FreeRTOS.h"
#endif

/// Parse the SwitchBot Meter service-data payload (UUID 0xFD3D, bytes after UUID).
/// Format (verified against SwitchBot Open Protocol rev 1.2):
///   [0] device byte  [1] status  [2] battery
///   [3] bits[3:0] = temp tenths (0–9 → 0.0–0.9 °C)
///   [4] bits[6:0] = temp integer (0–99), bit[7] = 1 if positive
///   [5] bits[6:0] = humidity %
/// @return true if @p len >= 6 and data was decoded successfully.
class SwitchBot : public ISensorSource {
public:
    explicit SwitchBot(Bluetooth& bluetooth);

    /// Parse a SwitchBot Meter service-data payload.
    [[nodiscard]] static bool parseServiceData(const uint8_t* data, size_t len, SwitchBotData& out);

    /// Return true when the reading is invalid or older than SENSOR_STALE_MS.
    [[nodiscard]] static bool isStale(const SwitchBotData& data, uint64_t nowMs);

    /// Register the SwitchBot advertisement handler with the Bluetooth scanner.
    void init();

    /// Thread-safe read of the latest decoded SwitchBot data.
    /// Sets out.valid = false if no data has ever arrived or if the data is stale.
    [[nodiscard]] bool getData(SwitchBotData& out) override;

private:
    static void advertisementHandler(const BluetoothAdvertisement& advertisement, void* context);
    void handleAdvertisement(const BluetoothAdvertisement& advertisement);

#ifndef NATIVE_ENV
    static bool parseMacString(const char* value, uint8_t bytes[6]);
    static const uint8_t* findServiceData(const uint8_t* adData, size_t adLength,
                                          size_t* payloadLength);
#endif

    Bluetooth& m_bluetooth;
    uint8_t m_targetMac[6]{};
    bool m_macParsed{false};
    SwitchBotData m_data{};

#ifndef NATIVE_ENV
    portMUX_TYPE m_mux = portMUX_INITIALIZER_UNLOCKED;
#endif
};
