// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include "hal/ILedHal.h"

class StubLedHal final : public ILedHal {
public:
    uint8_t brightness{128};

    void init(CRGB*, uint16_t) override {}
    void show() override {}
    void setBrightness(uint8_t value) override { brightness = value; }
};
