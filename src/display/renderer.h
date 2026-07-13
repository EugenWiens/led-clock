#pragma once
#include <cstdint>
#include "matrix.h"

/// Display colours — defined here as they are a rendering concern.
inline constexpr CRGB CLOCK_COLOR{255, 120, 0}; // warm amber
inline constexpr CRGB TEMP_COLOR{0, 200, 255};  // cyan

class Renderer {
public:
    explicit Renderer(Matrix& matrix);

    /// Clear the buffer then render HH:MM. Matrix layout:
    ///   0 = hour tens,  1 = hour units,  2 = colon,
    ///   3 = minute tens, 4 = minute units.
    void renderClock(uint8_t hh, uint8_t mm, bool colonOn);

    /// Clear the buffer then render temperature across all 5 matrices:
    ///   0-1 = integer part, 2 = decimal dot, 3 = fractional digit, 4 = '°'.
    /// Single-digit integers leave matrix 0 blank. NaN or out of [-9.9, 99.9]
    /// shows "--.-°".
    void renderTemp(float tempC);

private:
    Matrix& m_matrix;

    /// Render one FONT glyph centred in an 8×8 matrix cell (col_offset = 1).
    void renderGlyph(uint8_t matrixIdx, uint8_t fontIdx, CRGB color);

    /// Render a 2×2 decimal dot at lower-centre of matrix 2.
    void renderDecimalDot(CRGB color);
};
