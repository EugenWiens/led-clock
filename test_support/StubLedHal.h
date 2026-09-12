// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include "hal/ILedHal.h"

class StubLedHal final : public ILedHal {
public:
    uint8_t brightness{128};
    CRGB* leds{nullptr};
    uint16_t ledCount{0};
    uint32_t showCount{0};

    void init(CRGB* values, uint16_t count) override {
        leds = values;
        ledCount = count;
    }
    void show() override { ++showCount; }
    void setBrightness(uint8_t value) override { brightness = value; }
};
