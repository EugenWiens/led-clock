#pragma once
#include <cstdint>

/// 5×7 pixel bitmap font stored in program memory.
/// Column encoding per row byte: bit 4 = leftmost pixel (col 0 of glyph),
/// bit 0 = rightmost pixel (col 4 of glyph).
/// Renderers apply a col_offset of +1 to centre the 5-wide glyph in the 8-wide matrix cell.
namespace font {

constexpr uint8_t GLYPH_COLS = 5u;
constexpr uint8_t GLYPH_ROWS = 7u;
constexpr uint8_t COUNT = 13u;

/// Glyph indices.
constexpr uint8_t IDX_COLON = 10u;  ///< ':'
constexpr uint8_t IDX_DASH = 11u;   ///< '-'
constexpr uint8_t IDX_DEGREE = 12u; ///< '°'

/// FONT[fontIdx][row] — indices 0–9 = digits, 10 = ':', 11 = '-', 12 = '°'.
constexpr uint8_t FONT[COUNT][GLYPH_ROWS] = {
    // 0  .XXX. X...X X...X X...X X...X X...X .XXX.
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    // 1  ..X.. .XX.. ..X.. ..X.. ..X.. ..X.. .XXX.
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    // 2  .XXX. X...X ....X ...X. ..X.. .X... XXXXX
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    // 3  XXXX. ....X ....X .XXX. ....X ....X XXXX.
    {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E},
    // 4  X..X. X..X. X..X. XXXXX ...X. ...X. ...X.
    {0x12, 0x12, 0x12, 0x1F, 0x02, 0x02, 0x02},
    // 5  XXXX. X.... X.... XXXX. ....X ....X XXXX.
    {0x1E, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E},
    // 6  .XXX. X.... X.... XXXX. X...X X...X .XXX.
    {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    // 7  XXXXX ....X ...X. ..X.. ..X.. ..X.. ..X..
    {0x1F, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04},
    // 8  .XXX. X...X X...X .XXX. X...X X...X .XXX.
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    // 9  .XXX. X...X X...X .XXXX ....X ....X .XXX.
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E},
    // ':' ..... .XX.. .XX.. ..... .XX.. .XX.. .....
    {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00},
    // '-' ..... ..... ..... .XXX. ..... ..... .....
    {0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00},
    // '°' .XX.. X..X. X..X. .XX.. ..... ..... .....
    {0x0C, 0x12, 0x12, 0x0C, 0x00, 0x00, 0x00},
};

} // namespace font
