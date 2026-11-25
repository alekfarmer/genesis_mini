#include "arcade_ui.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "conversions.h"
#include "panel.h"

namespace arcade {
namespace ui {

using conversions::panelColor;

uint16_t black() { return panelColor(0, 0, 0); }
uint16_t white() { return panelColor(230, 230, 230); }
uint16_t dim() { return panelColor(140, 140, 150); }
uint16_t accent() { return panelColor(255, 190, 60); }
uint16_t good() { return panelColor(80, 220, 120); }
uint16_t bad() { return panelColor(255, 70, 70); }
uint16_t panelBg() { return panelColor(30, 30, 45); }
uint16_t border() { return panelColor(90, 90, 110); }

int16_t textWidth(const char* s, uint8_t size) { return static_cast<int16_t>(strlen(s) * 6u * size); }

void text(int16_t x, int16_t y, uint8_t size, uint16_t color, const char* s) {
  GFXcanvas16& c = panel::canvas();
  c.setTextSize(size);
  c.setTextColor(color);
  c.setCursor(x, y);
  c.print(s);
}

void textRight(int16_t right_x, int16_t y, uint8_t size, uint16_t color, const char* s) {
  text(static_cast<int16_t>(right_x - textWidth(s, size)), y, size, color, s);
}

void textCentered(int16_t y, uint8_t size, uint16_t color, const char* s) {
  text(static_cast<int16_t>((panel::width() - textWidth(s, size)) / 2), y, size, color, s);
}

void gameOverPanel(uint32_t score, uint32_t best, bool new_best) {
  // Layout (height 54): title size 2 at y 22 (ends 38), score y 42, best
  // y 51, hint y 61 (ends 69). "GAME OVER" at size 2 is 108 px wide.
  GFXcanvas16& c = panel::canvas();
  c.fillRect(20, 18, 120, 54, panelBg());
  c.drawRect(20, 18, 120, 54, border());
  textCentered(22, 2, bad(), "GAME OVER");
  char line[24];
  snprintf(line, sizeof(line), "SCORE %" PRIu32, score);
  textCentered(42, 1, white(), line);
  snprintf(line, sizeof(line), new_best ? "NEW BEST!" : "BEST %" PRIu32, best);
  textCentered(51, 1, new_best ? accent() : dim(), line);
  textCentered(61, 1, dim(), "PRESS KNOB");
}

void pausedBanner() {
  GFXcanvas16& c = panel::canvas();
  c.fillRect(30, 30, 100, 22, panelBg());
  c.drawRect(30, 30, 100, 22, border());
  textCentered(33, 2, accent(), "PAUSED");
}

}  // namespace ui
}  // namespace arcade
