#ifndef OLED_FONT_H
#define OLED_FONT_H

#include <stdint.h>

#define OLED_FONT_FIRST_CHAR 0x20U
#define OLED_FONT_LAST_CHAR 0x7FU
#define OLED_FONT_GLYPH_WIDTH 6U

extern const uint8_t OLED_Font6x8[96][OLED_FONT_GLYPH_WIDTH];

#endif
