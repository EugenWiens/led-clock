#include "matrix.h"
#include <cstdint>

Matrix::Matrix(ILedHal& ledHal, IAdcHal& adcHal)
    : m_ledHal{ledHal}, m_ldr{adcHal} {}

void Matrix::init() {
    m_ldr.init();
    m_ledHal.init(m_leds, LED_COUNT);
    clear();
    show();
}

void Matrix::clear() {
    for (auto& led : m_leds) {
        led = CRGB(0, 0, 0);
    }
}

void Matrix::setPixel(uint8_t matrixIdx, uint8_t col, uint8_t row, CRGB color) {
    if (matrixIdx >= 5u || col >= 8u || row >= 8u) return;
    m_leds[matrixIdx * 64u + row * 8u + col] = color;
}

void Matrix::show() {
    m_ledHal.show();
}

void Matrix::updateBrightness(uint64_t nowMs) {
    if (m_ldr.update(nowMs)) {
        m_ledHal.setBrightness(m_ldr.brightness());
    }
}

#ifdef NATIVE_ENV
CRGB Matrix::getLed(uint16_t idx) const {
    if (idx >= LED_COUNT) return CRGB(0, 0, 0);
    return m_leds[idx];
}
#endif
