// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

class CRGB {
public:
    uint8_t r{}, g{}, b{};
    constexpr CRGB() = default;
    constexpr CRGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
};
