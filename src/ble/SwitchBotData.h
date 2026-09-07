// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

class SwitchBotData {
public:
    float tempC{0.0f};
    uint8_t humidity{0};
    uint64_t lastSeenMs{0};
    bool valid{false};
};
