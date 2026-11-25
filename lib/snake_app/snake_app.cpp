#include "snake_app.h"

#include <Arduino.h>
#include <inttypes.h>
#include <stdio.h>

#include "arcade_ui.h"
#include "conversions.h"
#include "panel.h"

namespace ui = arcade::ui;

namespace {

// Layout, 160 x 80: score line at y 1 (size 1), rule at y 9, then the
// 32 x 14 grid of 5 px squares from y 10 (ends y 79, full width).
constexpr int16_t kCellPx = 5;
constexpr int16_t kGridY = 10;

const ToneSequence::Step kEatSound[] = {{1320, 30}, {1760, 40}};
const ToneSequence::Step kOverSound[] = {{440, 120}, {0, 30}, {330, 120}, {0, 30}, {220, 260}};

void cell(int x, int y, uint16_t color) {
  panel::canvas().fillRect(static_cast<int16_t>(x * kCellPx), static_cast<int16_t>(kGridY + y * kCellPx),
                           kCellPx - 1, kCellPx - 1, color);
}

}  // namespace

SnakeApp::SnakeApp() : game_(1) {}

void SnakeApp::enter(uint32_t now_ms) {
  best_ = arcade::loadBest(scoreKey());
  newGame(now_ms);
}

void SnakeApp::newGame(uint32_t now_ms) {
  game_.reset(arcade::newSeed());
  paused_ = false;
  new_best_ = false;
  last_step_ms_ = now_ms;
  arcade::setLed(arcade::kLedNormal);
  ui_dirty_ = true;
  Serial.printf("[snake] new game (best %" PRIu32 ")\n", best_);
}

void SnakeApp::onGameOver(uint32_t now_ms) {
  over_ms_ = now_ms;
  arcade::playSound(kOverSound, sizeof(kOverSound) / sizeof(kOverSound[0]), now_ms);
  arcade::setLed(arcade::kLedBad);
  new_best_ = game_.score() > best_;
  if (new_best_) {
    best_ = game_.score();
    arcade::saveBest(scoreKey(), best_);
  }
  ui_dirty_ = true;
  Serial.printf("[snake] game over: score %" PRIu32 ", length %u%s\n", game_.score(),
                static_cast<unsigned>(game_.length()), new_best_ ? " (new best)" : "");
}

void SnakeApp::update(const arcade::Input& in, uint32_t now_ms) {
  if (game_.state() == snake::Game::State::kGameOver) {
    if ((in.knob_pressed || in.button_tap) && now_ms - over_ms_ >= arcade::kRestartGuardMs) {
      newGame(now_ms);
    }
    return;
  }
  if (in.button_tap) {
    paused_ = !paused_;
    last_step_ms_ = now_ms;  // don't jump forward after a pause
    ui_dirty_ = true;
  }
  if (paused_) {
    return;
  }
  for (int32_t i = 0; i < in.knob_steps; ++i) game_.turnRight();
  for (int32_t i = 0; i > in.knob_steps; --i) game_.turnLeft();
  if (now_ms - last_step_ms_ >= game_.stepIntervalMs()) {
    last_step_ms_ = now_ms;
    if (game_.step()) {
      arcade::playSound(kEatSound, sizeof(kEatSound) / sizeof(kEatSound[0]), now_ms);
    }
    if (game_.state() == snake::Game::State::kGameOver) {
      onGameOver(now_ms);
    }
  }
}

bool SnakeApp::needsRedraw() const { return ui_dirty_ || game_.revision() != drawn_revision_; }

void SnakeApp::draw() {
  GFXcanvas16& c = panel::canvas();
  c.fillScreen(ui::black());
  char line[20];
  snprintf(line, sizeof(line), "SCORE %" PRIu32, game_.score());
  ui::text(2, 1, 1, ui::white(), line);
  snprintf(line, sizeof(line), "BEST %" PRIu32, best_);
  ui::textRight(158, 1, 1, ui::dim(), line);
  c.drawFastHLine(0, kGridY - 1, 160, ui::border());

  const snake::Cell food = game_.food();
  cell(food.x, food.y, ui::bad());
  const uint16_t body = conversions::panelColor(60, 190, 90);
  const uint16_t head = conversions::panelColor(170, 255, 170);
  for (uint16_t i = game_.length(); i-- > 0;) {
    const snake::Cell s = game_.segment(i);
    cell(s.x, s.y, i == 0 ? head : body);
  }

  if (game_.state() == snake::Game::State::kGameOver) {
    ui::gameOverPanel(game_.score(), best_, new_best_);
  } else if (paused_) {
    ui::pausedBanner();
  }
  drawn_revision_ = game_.revision();
  ui_dirty_ = false;
}
