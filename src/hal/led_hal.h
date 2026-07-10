#pragma once
#include <cstdint>

// On device FastLED provides CRGB; in the native test env we use a minimal stub.
#ifndef NATIVE_ENV
#  include <FastLED.h>
#else
struct CRGB {
    uint8_t r{}, g{}, b{};
    constexpr CRGB() = default;
    constexpr CRGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
};
#endif

/// Abstract LED hardware interface — injected into Matrix via constructor.
class ILedHal {
public:
    virtual ~ILedHal() = default;
    virtual void init(CRGB* leds, uint16_t count) = 0;
    virtual void show() = 0;
    virtual void setBrightness(uint8_t brightness) = 0;
};

#ifndef NATIVE_ENV
/// FastLED-based implementation for the physical WS2812B strip.
class FastLedHal final : public ILedHal {
public:
    void init(CRGB* leds, uint16_t count) override;
    void show() override;
    void setBrightness(uint8_t brightness) override;
};
#endif
