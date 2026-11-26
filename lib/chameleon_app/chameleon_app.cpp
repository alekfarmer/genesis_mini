#include "chameleon_app.h"

#include <Arduino.h>
#include <inttypes.h>
#include <stdio.h>

#include "arcade_ui.h"
#include "conversions.h"
#include "panel.h"

namespace ui = arcade::ui;
using chameleon::Game;
using conversions::panelColor;

namespace {

constexpr int kMaxCatchUpTicks = 5;

const ToneSequence::Step kCamoSound[] = {{2200, 20}, {1600, 20}, {1100, 30}};
const ToneSequence::Step kNoCoverSound[] = {{180, 40}};
const ToneSequence::Step kCaughtSound[] = {{880, 100}, {660, 100}, {440, 100}, {220, 300}};

template <size_t N>
void play(const ToneSequence::Step (&steps)[N], uint32_t now_ms) {
  arcade::playSound(steps, N, now_ms);
}

// Object colours on screen, and the same hues dimmed for the NeoPixel.
struct Hue {
  uint8_t r, g, b;
};
const Hue kHues[chameleon::kColors] = {
    {60, 190, 70},   // leaf green
    {255, 140, 40},  // orange
    {170, 90, 230},  // violet
    {60, 200, 220},  // cyan
    {255, 110, 170}, // pink
    {240, 220, 60},  // yellow
};

uint16_t screenColor(uint8_t c) { return panelColor(kHues[c].r, kHues[c].g, kHues[c].b); }

StatusLed::Color ledColor(uint8_t c) {
  return {static_cast<uint8_t>(kHues[c].r / 12), static_cast<uint8_t>(kHues[c].g / 12),
          static_cast<uint8_t>(kHues[c].b / 12)};
}

}  // namespace

ChameleonApp::ChameleonApp() : game_(1) {}

void ChameleonApp::enter(uint32_t now_ms) {
  best_ = arcade::loadBest(scoreKey());
  newGame(now_ms);
}

void ChameleonApp::exit() { arcade::setButtonLed(false); }

void ChameleonApp::newGame(uint32_t now_ms) {
  game_.reset(arcade::newSeed());
  new_best_ = false;
  last_tick_ms_ = now_ms;
  was_camouflaged_ = false;
  arcade::setLed(arcade::kLedNormal);
  ui_dirty_ = true;
  Serial.printf("[chameleon] new game (best %" PRIu32 " s)\n", best_);
}

void ChameleonApp::onCaught(uint32_t now_ms) {
  over_ms_ = now_ms;
  play(kCaughtSound, now_ms);
  arcade::setLed(arcade::kLedBad);
  arcade::setButtonLed(false);
  new_best_ = game_.score() > best_;
  if (new_best_) {
    best_ = game_.score();
    arcade::saveBest(scoreKey(), best_);
  }
  ui_dirty_ = true;
  Serial.printf("[chameleon] caught after %" PRIu32 " s%s\n", game_.score(), new_best_ ? " (new best)" : "");
}

void ChameleonApp::update(const arcade::Input& in, uint32_t now_ms) {
  if (game_.state() == Game::State::kCaught) {
    if ((in.knob_pressed || in.button_tap || in.led_button_pressed) && now_ms - over_ms_ >= arcade::kRestartGuardMs) {
      newGame(now_ms);
    }
    return;
  }
  game_.turn(in.knob_steps);
  if (in.knob_pressed) {
    game_.toggleWalking();
  }
  if (in.led_button_pressed) {
    if (game_.camouflage()) {
      play(kCamoSound, now_ms);
    } else {
      play(kNoCoverSound, now_ms);
    }
  }

  int ticks = 0;
  while (now_ms - last_tick_ms_ >= chameleon::kTickMs) {
    if (++ticks > kMaxCatchUpTicks) {
      last_tick_ms_ = now_ms;
      break;
    }
    last_tick_ms_ += chameleon::kTickMs;
    game_.tick();
    if (game_.state() == Game::State::kCaught) {
      onCaught(now_ms);
      return;
    }
  }

  // NeoPixel: your camouflage colour while hidden.
  if (game_.camouflaged() != was_camouflaged_) {
    was_camouflaged_ = game_.camouflaged();
    arcade::setLed(was_camouflaged_ ? ledColor(game_.camoColor()) : arcade::kLedNormal);
  }
  arcade::setButtonLed(game_.canCamouflage());
}

void ChameleonApp::draw() {
  GFXcanvas16& c = panel::canvas();
  c.fillScreen(panelColor(25, 40, 28));

  for (int i = 0; i < chameleon::kObjects; ++i) {
    const chameleon::Object& o = game_.object(i);
    c.fillCircle(o.x, o.y, o.radius, screenColor(o.color));
  }

  // Seekers: a red body, an eye towards where it looks, and the edges of its
  // sight cone (+/- 45 degrees, kSightRange long).
  const uint16_t cone = panelColor(110, 40, 40);
  for (int i = 0; i < chameleon::kMaxSeekers; ++i) {
    const chameleon::Seeker& s = game_.seeker(i);
    if (!s.active) continue;
    const int32_t fx = s.face_x, fy = s.face_y;
    const int32_t lx = (fx - fy) * 45 / 64, ly = (fx + fy) * 45 / 64;  // +45 deg
    const int32_t rx = (fx + fy) * 45 / 64, ry = (fy - fx) * 45 / 64;  // -45 deg
    const int32_t len = chameleon::kSightRange;
    c.drawLine(s.x(), s.y(), static_cast<int16_t>(s.x() + lx * len / 64), static_cast<int16_t>(s.y() + ly * len / 64),
               cone);
    c.drawLine(s.x(), s.y(), static_cast<int16_t>(s.x() + rx * len / 64), static_cast<int16_t>(s.y() + ry * len / 64),
               cone);
    c.fillCircle(s.x(), s.y(), 3, panelColor(230, 50, 50));
    c.drawPixel(static_cast<int16_t>(s.x() + fx * 2 / 64), static_cast<int16_t>(s.y() + fy * 2 / 64), ui::white());
  }

  // You: white with a nose pixel; in your camouflage colour while hidden.
  int16_t hx, hy;
  chameleon::headingVector(game_.heading(), &hx, &hy);
  const int16_t px = game_.playerX(), py = game_.playerY();
  if (game_.camouflaged()) {
    c.fillCircle(px, py, 3, screenColor(game_.camoColor()));
    c.drawPixel(px - 4, py, ui::white());  // faint outline so you can find yourself
    c.drawPixel(px + 4, py, ui::white());
    c.drawPixel(px, py - 4, ui::white());
    c.drawPixel(px, py + 4, ui::white());
  } else {
    c.fillCircle(px, py, 3, ui::white());
  }
  c.drawPixel(static_cast<int16_t>(px + hx * 5 / 64), static_cast<int16_t>(py + hy * 5 / 64), ui::accent());

  // HUD: time survived, camouflage state.
  c.fillRect(0, 0, 160, 10, ui::black());
  char line[20];
  snprintf(line, sizeof(line), "TIME %" PRIu32 "s", game_.score());
  ui::text(2, 1, 1, ui::white(), line);
  if (game_.camouflaged()) {
    snprintf(line, sizeof(line), "HIDDEN %u.%us", game_.camoTicksLeft() / 50, (game_.camoTicksLeft() % 50) / 5);
    ui::textRight(158, 1, 1, screenColor(game_.camoColor()), line);
  } else if (game_.cooldownTicks() > 0) {
    snprintf(line, sizeof(line), "RECHARGE %u.%us", game_.cooldownTicks() / 50, (game_.cooldownTicks() % 50) / 5);
    ui::textRight(158, 1, 1, ui::dim(), line);
  } else if (game_.canCamouflage()) {
    ui::textRight(158, 1, 1, ui::good(), "PRESS TO HIDE");
  } else {
    ui::textRight(158, 1, 1, ui::dim(), game_.walking() ? "FIND COVER" : "STOPPED");
  }

  if (game_.state() == Game::State::kCaught) {
    ui::gameOverPanel(game_.score(), best_, new_best_);
  }
  drawn_revision_ = game_.revision();
  ui_dirty_ = false;
}
