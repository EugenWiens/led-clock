#include "ldr.h"
#include <algorithm>

Ldr::Ldr(IAdcHal& adcHal) : m_adcHal{adcHal} {}

void Ldr::init() {
    m_lastSampleMs = 0;
    m_sampleIdx = 0;
    m_bufferFull = false;
    m_brightness = static_cast<uint8_t>(BRIGHTNESS_MIN);
    for (auto& s : m_samples)
        s = 0;
    m_adcHal.init();
}

bool Ldr::update(uint64_t nowMs) {
    if (nowMs - m_lastSampleMs < LDR_SAMPLE_MS) return false;
    m_lastSampleMs = nowMs;

    m_samples[m_sampleIdx] = m_adcHal.read();
    m_sampleIdx = static_cast<uint8_t>((m_sampleIdx + 1u) % LDR_SAMPLES);
    if (m_sampleIdx == 0u) m_bufferFull = true;

    const uint8_t count = m_bufferFull ? LDR_SAMPLES : m_sampleIdx;

    int32_t sum = 0;
    for (uint8_t i = 0; i < count; ++i)
        sum += m_samples[i];
    const int avg = static_cast<int>(sum / count);

    // Linear map [0, 4095] → [BRIGHTNESS_MIN, BRIGHTNESS_MAX]
    const int b =
        static_cast<int>(BRIGHTNESS_MIN) +
        (avg * (static_cast<int>(BRIGHTNESS_MAX) - static_cast<int>(BRIGHTNESS_MIN)) / 4095);

    m_brightness = static_cast<uint8_t>(
        std::clamp(b, static_cast<int>(BRIGHTNESS_MIN), static_cast<int>(BRIGHTNESS_MAX)));
    return true;
}

uint8_t Ldr::brightness() const {
    return m_brightness;
}
