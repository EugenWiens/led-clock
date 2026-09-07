// SPDX-FileCopyrightText: 2026 Eugen Wiens
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include "ILedHal.h"

#ifndef NATIVE_ENV

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

class EspLedStripHal final : public ILedHal {
public:
    void init(CRGB* leds, uint16_t count) override;
    void show() override;
    void setBrightness(uint8_t brightness) override;

private:
    static constexpr uint32_t RMT_RESOLUTION_HZ = 10'000'000u;

    rmt_channel_handle_t m_txChan{nullptr};
    rmt_encoder_handle_t m_bytesEnc{nullptr};
    CRGB* m_leds{nullptr};
    uint16_t m_count{0};
    uint8_t m_brightness{255};
};
#endif
