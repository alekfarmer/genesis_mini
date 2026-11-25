#include "blockfall_screen.h"

#include <inttypes.h>
#include <stdio.h>

#include "conversions.h"
#include "panel.h"

using blockfall::Cell;
using blockfall::Game;
using blockfall::kCols;
using blockfall::kRows;
using blockfall::PieceType;

namespace {

// Portrait layout, 80 x 160 px:
//   y 0-18   HUD: score (y 2) and level (y 11) in size-1 text on the left,
//            next piece in 4 px squares at x 60-76 on the right.
//   y 19     top of the well border.
//   y 20-159 the well: 10 x 20 squares of 7 px at x 5-74 (border at 4 and 75).
constexpr int16_t kCellPx = 7;
constexpr int16_t kWellX = 5;
constexpr int16_t kWellY = 20;
constexpr int16_t kNextX = 60;
constexpr int16_t kNextY = 2;
constexpr int16_t kNextCellPx = 4;

// Colours go through conversions::panelColor because the panel is BGR.
// An original palette, one colour per piece type (index = PieceType).
const uint16_t kPieceColors[blockfall::kPieceTypes] = {
    conversions::panelColor(80, 200, 190),   // I: teal
    conversions::panelColor(240, 200, 80),   // O: gold
    conversions::panelColor(200, 90, 220),   // T: violet
    conversions::panelColor(140, 220, 70),   // S: lime
    conversions::panelColor(240, 100, 90),   // Z: coral
    conversions::panelColor(90, 150, 255),   // J: sky
    conversions::panelColor(230, 230, 230),  // L: pearl
};
const uint16_t kBlack = conversions::panelColor(0, 0, 0);
const uint16_t kBorder = conversions::panelColor(90, 90, 110);
const uint16_t kText = conversions::panelColor(220, 220, 220);
const uint16_t kDim = conversions::panelColor(140, 140, 140);
const uint16_t kPanelBg = conversions::panelColor(30, 30, 45);
const uint16_t kWarn = conversions::panelColor(255, 70, 70);

uint16_t colorOf(PieceType type) { return kPieceColors[static_cast<uint8_t>(type)]; }

void square(int16_t x, int16_t y, int16_t size, uint16_t color) {
  // One pixel of gap keeps neighbouring squares distinct.
  panel::canvas().fillRect(x, y, size - 1, size - 1, color);
}

void printAt(int16_t x, int16_t y, uint8_t size, uint16_t color, const char* text) {
  GFXcanvas16& c = panel::canvas();
  c.setTextSize(size);
  c.setTextColor(color);
  c.setCursor(x, y);
  c.print(text);
}

}  // namespace

BlockfallScreen::BlockfallScreen(uint8_t cs, uint8_t rst, uint8_t dc) : cs_(cs), rst_(rst), dc_(dc) {}

bool BlockfallScreen::begin() {
  ready_ = panel::begin(cs_, rst_, dc_, panel::Orientation::kPortrait);
  return ready_;
}

void BlockfallScreen::draw(const Game& game, uint32_t best) {
  if (!ready_) {
    return;
  }
  GFXcanvas16& c = panel::canvas();
  char text[16];
  c.fillScreen(kBlack);

  // HUD. Score is at most 7 digits here (42 px), well clear of the next box.
  snprintf(text, sizeof(text), "%" PRIu32, game.score() > 9999999u ? 9999999u : game.score());
  printAt(2, 2, 1, kText, text);
  snprintf(text, sizeof(text), "LV %" PRIu32, game.level());
  printAt(2, 11, 1, kDim, text);
  Cell cells[4];
  blockfall::pieceCells(game.nextType(), 0, cells);
  for (const Cell& cell : cells) {
    square(kNextX + cell.x * kNextCellPx, kNextY + cell.y * kNextCellPx, kNextCellPx,
           colorOf(game.nextType()));
  }

  // Well border and locked squares.
  c.drawRect(kWellX - 1, kWellY - 1, kCols * kCellPx + 2, kRows * kCellPx + 1, kBorder);
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      const uint8_t v = game.cell(x, y);
      if (v != 0) {
        square(kWellX + x * kCellPx, kWellY + y * kCellPx, kCellPx, colorOf(static_cast<PieceType>(v - 1)));
      }
    }
  }

  if (game.state() == Game::State::kPlaying) {
    const blockfall::Piece& p = game.piece();
    blockfall::pieceCells(p.type, p.rotation, cells);
    for (const Cell& cell : cells) {
      square(kWellX + (p.x + cell.x) * kCellPx, kWellY + (p.y + cell.y) * kCellPx, kCellPx, colorOf(p.type));
    }
  } else {
    // Game-over panel, 64 x 66 px centred over the well:
    //   "GAME"/"OVER" size 2 (48 px wide) at y 54 and 72, "BEST n" and
    //   "PRESS KNOB" size 1 at y 94 and 106 (60 px wide), ends at y 114.
    c.fillRect(8, 48, 64, 70, kPanelBg);
    c.drawRect(8, 48, 64, 70, kBorder);
    printAt(16, 54, 2, kWarn, "GAME");
    printAt(16, 72, 2, kWarn, "OVER");
    snprintf(text, sizeof(text), "BEST %" PRIu32, best > 999999u ? 999999u : best);
    printAt(12, 94, 1, kText, text);
    printAt(10, 106, 1, kDim, "PRESS KNOB");
  }

  panel::push();
}
