#pragma once
#include <cstdint>
#include "../hal/led_hal.h" // provides CRGB + ILedHal
#include "../hal/adc_hal.h" // provides IAdcHal
#include "ldr.h"
#include "../config.h"

class Matrix {
public:
    /// Construct with injected HAL dependencies.
    Matrix(ILedHal& ledHal, IAdcHal& adcHal);

    /// Initialise hardware and push one blank frame. Resets all internal state.
    void init();

    /// Zero every pixel in the buffer (does not call show()).
    void clear();

    /// Write @p color to the logical coordinate (matrixIdx, col, row).
    /// Mapping: globalIdx = matrixIdx * 64 + row * 8 + col.
    /// Out-of-range arguments are silently ignored.
    void setPixel(uint8_t matrixIdx, uint8_t col, uint8_t row, CRGB color);

    /// Push the current buffer to the physical strip.
    void show();

    /// Delegate to Ldr::update(); apply the resulting brightness to the strip.
    /// Internally throttled to LDR_SAMPLE_MS; call every loop iteration.
    /// @param nowMs  Current time in milliseconds.
    void updateBrightness(uint64_t nowMs);

#ifdef NATIVE_ENV
    /// Test helper — returns the CRGB value at absolute LED index @p idx.
    [[nodiscard]] CRGB getLed(uint16_t idx) const;
#endif

private:
    ILedHal& m_ledHal;
    Ldr      m_ldr;

    CRGB m_leds[LED_COUNT];
};
