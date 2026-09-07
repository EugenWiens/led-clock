// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>

#include "BluetoothAdvertisement.h"

#ifndef NATIVE_ENV
#include "host/ble_gap.h"
#endif

using BluetoothAdvertisementHandler = void (*)(const BluetoothAdvertisement& advertisement,
                                               void* context);

class Bluetooth {
public:
    void setAdvertisementHandler(BluetoothAdvertisementHandler handler, void* context);
    void init();

private:
    void notify(const uint8_t address[6], const uint8_t* data, size_t dataLength,
                uint64_t timestampMs);

#ifndef NATIVE_ENV
    static int gapEventCallback(struct ble_gap_event* event, void* argument);
    static void syncCallback();
    static void resetCallback(int reason);
    static void hostTask(void* parameter);

    static Bluetooth* s_instance;
#endif

    BluetoothAdvertisementHandler m_handler{nullptr};
    void* m_context{nullptr};
};
