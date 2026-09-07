// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#include "SwitchBot.h"
#include "config.h"

#ifndef NATIVE_ENV
#include "esp_timer.h"
#endif

#include <cstdio>
#include <cstring>

// ===========================================================================
// Pure logic — compiled in all environments (device + native test)
// ===========================================================================

bool SwitchBot::parseServiceData(const uint8_t* data, size_t len, SwitchBotData& out) {
    if (data == nullptr || len < 6u) {
        return false;
    }

    // Byte [3]: lower nibble = temperature tenths (0–9)
    // Byte [4]: bits[6:0] = temperature integer (0–99), bit[7] = sign (1 = positive)
    const auto tenths = static_cast<float>(data[3] & 0x0Fu) / 10.0f;
    const auto integral = static_cast<float>(data[4] & 0x7Fu);
    const float sign = (data[4] & 0x80u) ? 1.0f : -1.0f;

    out.tempC = sign * (integral + tenths);
    out.humidity = static_cast<uint8_t>(data[5] & 0x7Fu);
    out.valid = true;
    return true;
}

bool SwitchBot::isStale(const SwitchBotData& data, uint64_t nowMs) {
    if (!data.valid) {
        return true;
    }
    return (nowMs - data.lastSeenMs) >= static_cast<uint64_t>(SENSOR_STALE_MS);
}

// ===========================================================================
// SwitchBot device implementation — excluded from native test environment
// ===========================================================================
#ifndef NATIVE_ENV

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Parse "aa:bb:cc:dd:ee:ff" into val[0..5] stored in reversed byte order
/// (BLE address little-endian convention: val[0] = least-significant octet).
bool SwitchBot::parseMacString(const char* str, uint8_t val[6]) {
    unsigned int b[6];
    if (std::sscanf(str, "%x:%x:%x:%x:%x:%x", &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; ++i) {
        val[i] = static_cast<uint8_t>(b[i]);
    }
    return true;
}

/// Walk raw advertisement data looking for service-data AD type (0x16)
/// with UUID 0xFD3D. Returns a pointer to the payload bytes (after UUID)
/// and sets *payloadLen on success; returns nullptr on failure.
const uint8_t* SwitchBot::findServiceData(const uint8_t* adData, size_t adLen, size_t* payloadLen) {
    const uint8_t* p = adData;
    const uint8_t* end = adData + adLen;

    while (p < end) {
        const size_t fieldLen = p[0];
        if (fieldLen == 0u || p + 1u + fieldLen > end) {
            break;
        }
        const uint8_t fieldType = p[1];
        // AD type 0x16 = Service Data — 16-bit UUID
        if (fieldType == 0x16u && fieldLen >= 3u) {
            // UUID is little-endian: FD3D → bytes 0x3D, 0xFD
            if (p[2] == 0x3Du && p[3] == 0xFDu) {
                // Payload starts after 1-byte length, 1-byte type, 2-byte UUID
                *payloadLen = fieldLen - 3u; // subtract type + uuid bytes
                return p + 4u;
            }
        }
        p += 1u + fieldLen;
    }
    return nullptr;
}

SwitchBot::SwitchBot(Bluetooth& bluetooth) : m_bluetooth{bluetooth} {}

void SwitchBot::init() {
    m_macParsed = parseMacString(SWITCHBOT_MAC, m_targetMac);
    m_bluetooth.setAdvertisementHandler(&SwitchBot::advertisementHandler, this);
}

void SwitchBot::advertisementHandler(const BluetoothAdvertisement& advertisement, void* context) {
    auto* switchBot = static_cast<SwitchBot*>(context);
    if (switchBot != nullptr) {
        switchBot->handleAdvertisement(advertisement);
    }
}

void SwitchBot::handleAdvertisement(const BluetoothAdvertisement& advertisement) {
    if (!m_macParsed || std::memcmp(advertisement.address, m_targetMac, sizeof(m_targetMac)) != 0) {
        return;
    }

    size_t payloadLength = 0;
    const uint8_t* payload =
        findServiceData(advertisement.data, advertisement.dataLength, &payloadLength);
    if (payload == nullptr) {
        return;
    }

    SwitchBotData parsed{};
    parsed.lastSeenMs = advertisement.timestampMs;
    if (parseServiceData(payload, payloadLength, parsed)) {
        portENTER_CRITICAL(&m_mux);
        m_data = parsed;
        portEXIT_CRITICAL(&m_mux);
    }
}

bool SwitchBot::getData(SwitchBotData& out) {
    portENTER_CRITICAL(&m_mux);
    out = m_data;
    portEXIT_CRITICAL(&m_mux);

    if (out.valid) {
        const uint64_t nowMs = static_cast<uint64_t>(esp_timer_get_time() / 1000);
        if (isStale(out, nowMs)) {
            out.valid = false;
        }
    }
    return out.valid;
}

#else

SwitchBot::SwitchBot(Bluetooth& bluetooth) : m_bluetooth{bluetooth} {}

void SwitchBot::init() {}

bool SwitchBot::getData(SwitchBotData& out) {
    out = {};
    return false;
}

#endif // !NATIVE_ENV
