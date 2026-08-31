#include "switchbot.h"
#include "../config.h"

#include <cstdio>
#include <cstring>

// ===========================================================================
// Pure logic — compiled in all environments (device + native test)
// ===========================================================================

bool parseSwitchBotServiceData(const uint8_t* data, size_t len, SwitchBotData& out) {
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

bool isSwitchBotStale(const SwitchBotData& d, uint64_t nowMs) {
    if (!d.valid) {
        return true;
    }
    return (nowMs - d.lastSeenMs) >= static_cast<uint64_t>(SENSOR_STALE_MS);
}

// ===========================================================================
// Device BLE implementation — excluded from native test environment
// ===========================================================================
#ifndef NATIVE_ENV

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"

// ---------------------------------------------------------------------------
// Shared state (written from NimBLE task, read from app_main task)
// ---------------------------------------------------------------------------
static SwitchBotData s_data{};
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

// Parsed target MAC (little-endian, as stored in ble_addr_t.val[])
static uint8_t s_targetMac[6]{};
static bool s_macParsed{false};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Parse "aa:bb:cc:dd:ee:ff" into val[0..5] stored in reversed byte order
/// (BLE address little-endian convention: val[0] = least-significant octet).
static bool parseMacString(const char* str, uint8_t val[6]) {
    unsigned int b[6];
    if (std::sscanf(str, "%x:%x:%x:%x:%x:%x", &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; ++i) {
        val[i] = static_cast<uint8_t>(b[i]);
    }
    return true;
}

static bool macMatches(const ble_addr_t& addr) {
    return (std::memcmp(addr.val, s_targetMac, 6) == 0);
}

/// Walk raw advertisement data looking for service-data AD type (0x16)
/// with UUID 0xFD3D. Returns a pointer to the payload bytes (after UUID)
/// and sets *payloadLen on success; returns nullptr on failure.
static const uint8_t* findServiceData(const uint8_t* adData, uint8_t adLen, uint8_t* payloadLen) {
    const uint8_t* p = adData;
    const uint8_t* end = adData + adLen;

    while (p < end) {
        uint8_t fieldLen = p[0];
        if (fieldLen == 0u || p + 1u + fieldLen > end) {
            break;
        }
        uint8_t fieldType = p[1];
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

// ---------------------------------------------------------------------------
// NimBLE callbacks
// ---------------------------------------------------------------------------

static int gapEventCb(struct ble_gap_event* event, void* /*arg*/) {
    if (event->type != BLE_GAP_EVENT_DISC) {
        return 0;
    }

    const struct ble_gap_disc_desc& desc = event->disc;

    if (!macMatches(desc.addr)) {
        return 0;
    }

    uint8_t payloadLen = 0;
    const uint8_t* payload =
        findServiceData(desc.data, static_cast<uint8_t>(desc.length_data), &payloadLen);
    if (payload == nullptr) {
        return 0;
    }

    SwitchBotData parsed{};
    parsed.lastSeenMs = static_cast<uint64_t>(esp_timer_get_time() / 1000);
    if (parseSwitchBotServiceData(payload, payloadLen, parsed)) {
        portENTER_CRITICAL(&s_mux);
        s_data = parsed;
        portEXIT_CRITICAL(&s_mux);
    }
    return 0;
}

static void onSync() {
    struct ble_gap_disc_params dp{};
    dp.passive = 1;           // No scan requests — purely listen
    dp.filter_duplicates = 0; // Accept repeated packets for continuous updates
    dp.itvl = 0;              // Default: 625 µs units × 16 = 10 ms
    dp.window = 0;            // Default: 625 µs units × 16 = 10 ms
    dp.filter_policy = 0;     // Accept all advertisers
    dp.limited = 0;           // General discovery

    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &dp, gapEventCb, nullptr);
}

static void onReset(int reason) {
    (void)reason;
    // NimBLE stack reset — scan will be restarted on next onSync()
}

static void nimbleHostTask(void* /*param*/) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// ---------------------------------------------------------------------------
// Public device API
// ---------------------------------------------------------------------------

void switchbotBegin() {
    s_macParsed = parseMacString(SWITCHBOT_MAC, s_targetMac);

    nimble_port_init();
    ble_hs_cfg.sync_cb = onSync;
    ble_hs_cfg.reset_cb = onReset;

    nimble_port_freertos_init(nimbleHostTask);
}

bool switchbotGetData(SwitchBotData& out) {
    portENTER_CRITICAL(&s_mux);
    out = s_data;
    portEXIT_CRITICAL(&s_mux);

    if (out.valid) {
        const uint64_t nowMs = static_cast<uint64_t>(esp_timer_get_time() / 1000);
        if (isSwitchBotStale(out, nowMs)) {
            out.valid = false;
        }
    }
    return out.valid;
}

#endif // !NATIVE_ENV
