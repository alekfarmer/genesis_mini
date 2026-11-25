#include "shooter_app.h"

#include <Arduino.h>
#include <inttypes.h>
#include <stdio.h>

#include "arcade_ui.h"
#include "conversions.h"
#include "panel.h"

namespace ui = arcade::ui;
using shooter::Game;

namespace {

// If the loop falls behind, run at most this many ticks to catch up, then
// resynchronise, so a stall never turns into a burst of fast motion.
constexpr int kMaxCatchUpTicks = 5;
constexpr uint32_t kHurtFlashMs = 250;

const ToneSequence::Step kShotSound[] = {{1800, 15}, {1200, 15}};
const ToneSequence::Step kBoomSound[] = {{300, 20}, {180, 30}, {110, 40}};
const ToneSequence::Step kHurtSound[] = {{200, 80}, {150, 120}};
const ToneSequence::Step kOverSound[] = {{520, 150}, {0, 40}, {390, 150}, {0, 40}, {260, 320}};

// Background stars: fixed positions that drift left at different speeds.
struct Star {
  int16_t x;
  int16_t y;
  uint8_t speed;  // pixels per 4 ticks
};
const Star kStars[] = {{12, 20, 1}, {37, 64, 2}, {58, 33, 1}, {81, 72, 3}, {99, 15, 2}, {118, 48, 1},
                       {133, 27, 3}, {149, 60, 2}, {24, 44, 3}, {70, 55, 1}, {110, 38, 2}, {155, 12, 1}};

}  // namespace

ShooterApp::ShooterApp() : game_(1) {}

void ShooterApp::enter(uint32_t now_ms) {
  best_ = arcade::loadBest(scoreKey());
  newGame(now_ms);
}

void ShooterApp::newGame(uint32_t now_ms) {
  game_.reset(arcade::newSeed());
  new_best_ = false;
  last_tick_ms_ = now_ms;
  frame_ = 0;
  flashing_ = false;
  arcade::setLed(arcade::kLedNormal);
  ui_dirty_ = true;
  Serial.printf("[shooter] new game (best %" PRIu32 ")\n", best_);
}

void ShooterApp::onGameOver(uint32_t now_ms) {
  over_ms_ = now_ms;
  arcade::playSound(kOverSound, sizeof(kOverSound) / sizeof(kOverSound[0]), now_ms);
  arcade::setLed(arcade::kLedBad);
  new_best_ = game_.score() > best_;
  if (new_best_) {
    best_ = game_.score();
    arcade::saveBest(scoreKey(), best_);
  }
  ui_dirty_ = true;
  Serial.printf("[shooter] game over: score %" PRIu32 "%s\n", game_.score(), new_best_ ? " (new best)" : "");
}

void ShooterApp::shoot(uint32_t now_ms) {
  if (game_.fire()) {
    arcade::playSound(kShotSound, sizeof(kShotSound) / sizeof(kShotSound[0]), now_ms);
  }
}

void ShooterApp::update(const arcade::Input& in, uint32_t now_ms) {
  if (game_.state() == Game::State::kGameOver) {
    if ((in.knob_pressed || in.button_tap) && now_ms - over_ms_ >= arcade::kRestartGuardMs) {
      newGame(now_ms);
    }
    return;
  }
  game_.moveShip(in.knob_steps);
  if (in.knob_pressed || in.button_held) {
    shoot(now_ms);  // fire() itself enforces the cooldown
  }

  int ticks = 0;
  while (now_ms - last_tick_ms_ >= shooter::kTickMs) {
    if (++ticks > kMaxCatchUpTicks) {
      last_tick_ms_ = now_ms;
      break;
    }
    last_tick_ms_ += shooter::kTickMs;
    game_.tick();
    ++frame_;
    if (game_.shipHitLastTick()) {
      arcade::playSound(kHurtSound, sizeof(kHurtSound) / sizeof(kHurtSound[0]), now_ms);
      arcade::setLed(arcade::kLedBad);
      flashing_ = true;
      flash_until_ms_ = now_ms + kHurtFlashMs;
    } else if (game_.rocksHitLastTick() > 0) {
      arcade::playSound(kBoomSound, sizeof(kBoomSound) / sizeof(kBoomSound[0]), now_ms);
    }
    if (game_.state() == Game::State::kGameOver) {
      onGameOver(now_ms);
      return;
    }
  }
  if (flashing_ && static_cast<int32_t>(now_ms - flash_until_ms_) >= 0) {
    flashing_ = false;
    arcade::setLed(arcade::kLedNormal);
  }
}

bool ShooterApp::needsRedraw() const { return ui_dirty_ || game_.revision() != drawn_revision_; }

void ShooterApp::draw() {
  GFXcanvas16& c = panel::canvas();
  c.fillScreen(ui::black());

  const uint16_t star = conversions::panelColor(90, 90, 120);
  for (const Star& s : kStars) {
    const int32_t x = (s.x - static_cast<int32_t>((frame_ * s.speed) / 4)) % shooter::kWidth;
    c.drawPixel(static_cast<int16_t>(x < 0 ? x + shooter::kWidth : x), s.y, star);
  }

  // Rocks: filled circles with a darker rim.
  const uint16_t rock = conversions::panelColor(150, 130, 110);
  const uint16_t rim = conversions::panelColor(90, 75, 60);
  for (int i = 0; i < shooter::kMaxRocks; ++i) {
    const shooter::Rock& r = game_.rock(i);
    if (!r.active) continue;
    c.fillCircle(r.x(), r.y, r.radius, rock);
    c.drawCircle(r.x(), r.y, r.radius, rim);
  }

  const uint16_t shot = conversions::panelColor(255, 230, 90);
  for (int i = 0; i < shooter::kMaxBullets; ++i) {
    const shooter::Bullet& b = game_.bullet(i);
    if (b.active) c.drawFastHLine(b.x, b.y, 3, shot);
  }

  // Ship: a triangle pointing right; it blinks while invulnerable.
  const bool blink_hidden = game_.invulnerable() && ((frame_ / 5) % 2 == 0);
  if (game_.state() == Game::State::kPlaying && !blink_hidden) {
    const int16_t y = game_.shipY();
    c.fillTriangle(shooter::kShipX, y, shooter::kShipX, static_cast<int16_t>(y + shooter::kShipHeight - 1),
                   static_cast<int16_t>(shooter::kShipNoseX - 1), static_cast<int16_t>(y + shooter::kShipHeight / 2),
                   conversions::panelColor(110, 200, 255));
    c.drawPixel(shooter::kShipX + 5, static_cast<int16_t>(y + shooter::kShipHeight / 2), ui::white());
  }

  // HUD on top: score left, lives as small ships right.
  char line[20];
  snprintf(line, sizeof(line), "SCORE %" PRIu32, game_.score());
  ui::text(2, 1, 1, ui::white(), line);
  for (uint8_t i = 0; i < game_.lives(); ++i) {
    const int16_t x = static_cast<int16_t>(150 - i * 10);
    c.fillTriangle(x, 1, x, 7, static_cast<int16_t>(x + 7), 4, conversions::panelColor(110, 200, 255));
  }

  if (game_.state() == Game::State::kGameOver) {
    ui::gameOverPanel(game_.score(), best_, new_best_);
  }
  drawn_revision_ = game_.revision();
  ui_dirty_ = false;
}
