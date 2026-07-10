#pragma once
#include <cstdint>

/// Simple RGB pixel — no external library dependency.
struct CRGB {
    uint8_t r{}, g{}, b{};
    constexpr CRGB() = default;
    constexpr CRGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
};

/// Abstract LED hardware interface — injected into Matrix via constructor.
class ILedHal {
public:
    virtual ~ILedHal() = default;
    virtual void init(CRGB* leds, uint16_t count) = 0;
    virtual void show() = 0;
    virtual void setBrightness(uint8_t brightness) = 0;
};

#ifndef NATIVE_ENV
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

/// ESP-IDF RMT-based WS2812B implementation (no Arduino dependency).
class EspLedStripHal final : public ILedHal {
public:
    void init(CRGB* leds, uint16_t count) override;
    void show() override;
    void setBrightness(uint8_t brightness) override;
private:
    rmt_channel_handle_t m_txChan{nullptr};
    rmt_encoder_handle_t m_bytesEnc{nullptr};
    CRGB*    m_leds{nullptr};
    uint16_t m_count{0};
    uint8_t  m_brightness{255};
};
#endif
