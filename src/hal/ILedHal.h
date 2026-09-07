// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include "CRGB.h"

class ILedHal {
public:
    virtual ~ILedHal() = default;
    virtual void init(CRGB* leds, uint16_t count) = 0;
    virtual void show() = 0;
    virtual void setBrightness(uint8_t brightness) = 0;
};
