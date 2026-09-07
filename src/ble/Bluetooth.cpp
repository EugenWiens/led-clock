// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#include "Bluetooth.h"

#include <cstring>

#ifndef NATIVE_ENV

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

Bluetooth* Bluetooth::s_instance{nullptr};

int Bluetooth::gapEventCallback(struct ble_gap_event* event, void* /*argument*/) {
    if (s_instance == nullptr || event->type != BLE_GAP_EVENT_DISC) {
        return 0;
    }

    const struct ble_gap_disc_desc& discovery = event->disc;
    const uint64_t timestampMs = static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);
    s_instance->notify(discovery.addr.val, discovery.data,
                       static_cast<size_t>(discovery.length_data), timestampMs);
    return 0;
}

void Bluetooth::syncCallback() {
    struct ble_gap_disc_params discoveryParams{};
    discoveryParams.passive = 1;
    discoveryParams.filter_duplicates = 0;
    discoveryParams.itvl = 0;
    discoveryParams.window = 0;
    discoveryParams.filter_policy = 0;
    discoveryParams.limited = 0;

    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &discoveryParams,
                 &Bluetooth::gapEventCallback, nullptr);
}

void Bluetooth::resetCallback(int reason) {
    (void)reason;
}

void Bluetooth::hostTask(void* /*parameter*/) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

#endif // !NATIVE_ENV

void Bluetooth::setAdvertisementHandler(BluetoothAdvertisementHandler handler, void* context) {
    m_handler = handler;
    m_context = context;
}

void Bluetooth::init() {
#ifndef NATIVE_ENV
    s_instance = this;
    nimble_port_init();
    ble_hs_cfg.sync_cb = &Bluetooth::syncCallback;
    ble_hs_cfg.reset_cb = &Bluetooth::resetCallback;
    nimble_port_freertos_init(&Bluetooth::hostTask);
#endif
}

void Bluetooth::notify(const uint8_t address[6], const uint8_t* data, size_t dataLength,
                       uint64_t timestampMs) {
    if (m_handler == nullptr) {
        return;
    }

    BluetoothAdvertisement advertisement{};
    std::memcpy(advertisement.address, address, sizeof(advertisement.address));
    advertisement.data = data;
    advertisement.dataLength = dataLength;
    advertisement.timestampMs = timestampMs;
    m_handler(advertisement, m_context);
}
