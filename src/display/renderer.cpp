#include "renderer.h"
#include "font.h"
#include <cmath>
#include <cstdint>

Renderer::Renderer(Matrix& matrix) : m_matrix{matrix} {}

void Renderer::renderGlyph(uint8_t matrixIdx, uint8_t fontIdx, CRGB color) {
    constexpr uint8_t COL_OFFSET = 1u; // centres 5-wide glyph in 8-wide cell
    for (uint8_t row = 0u; row < font::GLYPH_ROWS; ++row) {
        const uint8_t rowBits = font::FONT[fontIdx][row];
        for (uint8_t c = 0u; c < font::GLYPH_COLS; ++c) {
            if ((rowBits >> (4u - c)) & 1u) {
                m_matrix.setPixel(matrixIdx, c + COL_OFFSET, row, color);
            }
        }
    }
}

void Renderer::renderDecimalDot(CRGB color) {
    // 2×2 pixel block at lower-centre of matrix 2 (cols 3-4, rows 5-6)
    m_matrix.setPixel(2u, 3u, 5u, color);
    m_matrix.setPixel(2u, 4u, 5u, color);
    m_matrix.setPixel(2u, 3u, 6u, color);
    m_matrix.setPixel(2u, 4u, 6u, color);
}

void Renderer::renderClock(uint8_t hh, uint8_t mm, bool colonOn) {
    m_matrix.clear();
    renderGlyph(0u, static_cast<uint8_t>(hh / 10u), CLOCK_COLOR);
    renderGlyph(1u, static_cast<uint8_t>(hh % 10u), CLOCK_COLOR);
    if (colonOn) {
        renderGlyph(2u, font::IDX_COLON, CLOCK_COLOR);
    }
    renderGlyph(3u, static_cast<uint8_t>(mm / 10u), CLOCK_COLOR);
    renderGlyph(4u, static_cast<uint8_t>(mm % 10u), CLOCK_COLOR);
}

void Renderer::renderTemp(float tempC) {
    m_matrix.clear();

    if (std::isnan(tempC) || tempC < -9.9f || tempC > 99.9f) {
        renderGlyph(0u, font::IDX_DASH, TEMP_COLOR);
        renderGlyph(1u, font::IDX_DASH, TEMP_COLOR);
        renderDecimalDot(TEMP_COLOR);
        renderGlyph(3u, font::IDX_DASH, TEMP_COLOR);
        renderGlyph(4u, font::IDX_DEGREE, TEMP_COLOR);
        return;
    }

    const bool negative = tempC < 0.0f;
    const float absTemp = negative ? -tempC : tempC;
    const int intPart = static_cast<int>(absTemp);
    int fracPart = static_cast<int>(std::lround((absTemp - static_cast<float>(intPart)) * 10.0f));
    if (fracPart >= 10) {
        fracPart = 9;
    }

    if (negative) {
        renderGlyph(0u, font::IDX_DASH, TEMP_COLOR);
        renderGlyph(1u, static_cast<uint8_t>(intPart % 10), TEMP_COLOR);
    } else if (intPart >= 10) {
        renderGlyph(0u, static_cast<uint8_t>(intPart / 10), TEMP_COLOR);
        renderGlyph(1u, static_cast<uint8_t>(intPart % 10), TEMP_COLOR);
    } else {
        // Single-digit: leave matrix 0 blank (already cleared)
        renderGlyph(1u, static_cast<uint8_t>(intPart), TEMP_COLOR);
    }

    renderDecimalDot(TEMP_COLOR);
    renderGlyph(3u, static_cast<uint8_t>(fracPart), TEMP_COLOR);
    renderGlyph(4u, font::IDX_DEGREE, TEMP_COLOR);
}
