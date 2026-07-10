#pragma once
#include <cstdint>
#include "../hal/adc_hal.h"
#include "../config.h"

/// Reads the LDR via ADC, maintains an 8-sample rolling average, and
/// computes a brightness value in [BRIGHTNESS_MIN, BRIGHTNESS_MAX].
class Ldr {
public:
    explicit Ldr(IAdcHal& adcHal);

    /// Reset internal state and initialise the ADC hardware.
    void init();

    /// Sample the ADC if LDR_SAMPLE_MS ms have elapsed since the last sample.
    /// @param nowMs  Current time in milliseconds.
    void update(uint64_t nowMs);

    /// Returns the last computed brightness (BRIGHTNESS_MIN–BRIGHTNESS_MAX).
    uint8_t brightness() const;

private:
    IAdcHal& m_adcHal;

    uint64_t m_lastSampleMs{0};
    int      m_samples[LDR_SAMPLES]{};
    uint8_t  m_sampleIdx{0};
    bool     m_bufferFull{false};
    uint8_t  m_brightness{static_cast<uint8_t>(BRIGHTNESS_MIN)};
};
