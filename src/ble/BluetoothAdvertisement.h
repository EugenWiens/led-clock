// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>

class BluetoothAdvertisement {
public:
    uint8_t address[6]{};
    const uint8_t* data{nullptr};
    size_t dataLength{0};
    uint64_t timestampMs{0};
};
