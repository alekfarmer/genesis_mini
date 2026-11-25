#pragma once

#include <stdint.h>

/// Drawing helpers and the shared palette for arcade apps. Everything draws
/// into panel::canvas(). Text sizes: size 1 = 6x8 px per character, size 2 =
/// 12x16 px.
namespace arcade {
namespace ui {

// Palette (already converted for the BGR panel).
uint16_t black();
uint16_t white();
uint16_t dim();
uint16_t accent();
uint16_t good();
uint16_t bad();
uint16_t panelBg();
uint16_t border();

/// Width in pixels of `text` at `size`.
int16_t textWidth(const char* text, uint8_t size);

void text(int16_t x, int16_t y, uint8_t size, uint16_t color, const char* s);
void textRight(int16_t right_x, int16_t y, uint8_t size, uint16_t color, const char* s);
void textCentered(int16_t y, uint8_t size, uint16_t color, const char* s);

/// Landscape "GAME OVER" panel with the score, the best and a restart hint.
/// Box at x 20-139, y 18-71.
void gameOverPanel(uint32_t score, uint32_t best, bool new_best);

/// "PAUSED" banner across the middle of a landscape screen.
void pausedBanner();

}  // namespace ui
}  // namespace arcade
