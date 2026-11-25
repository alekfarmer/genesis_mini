#include "blockfall_app.h"

#include <Arduino.h>
#include <inttypes.h>

#include "blockfall_screen.h"

using blockfall::Game;

namespace {

constexpr uint32_t kSoftDropMs = 40;  // fall interval while the button is held
constexpr uint32_t kFlashMs = 300;    // NeoPixel flash on a line clear

// Sound effects (original short figures).
const ToneSequence::Step kLandSound[] = {{220, 25}};
const ToneSequence::Step kClearSound[] = {{880, 60}, {1175, 90}};
const ToneSequence::Step kBigClearSound[] = {{880, 60}, {1175, 60}, {1480, 60}, {1760, 160}};
const ToneSequence::Step kGameOverSound[] = {{660, 150}, {0, 40}, {520, 150}, {0, 40}, {390, 320}};

template <size_t N>
void play(const ToneSequence::Step (&steps)[N], uint32_t now_ms) {
  arcade::playSound(steps, N, now_ms);
}

}  // namespace

BlockfallApp::BlockfallApp() : game_(1) {}

void BlockfallApp::enter(uint32_t now_ms) {
  best_ = arcade::loadBest(scoreKey());
  newGame(now_ms);
}

void BlockfallApp::newGame(uint32_t now_ms) {
  game_.reset(arcade::newSeed());
  last_fall_ms_ = now_ms;
  flashing_ = false;
  arcade::setLed(arcade::kLedNormal);
  Serial.printf("[blockfall] new game (best %" PRIu32 ")\n", best_);
}

void BlockfallApp::onLock(uint32_t now_ms) {
  const uint8_t cleared = game_.lastCleared();
  if (game_.state() == Game::State::kGameOver) {
    over_ms_ = now_ms;
    play(kGameOverSound, now_ms);
    arcade::setLed(arcade::kLedBad);
    const bool record = game_.score() > best_;
    if (record) {
      best_ = game_.score();
      arcade::saveBest(scoreKey(), best_);
    }
    Serial.printf("[blockfall] game over: score %" PRIu32 ", lines %" PRIu32 "%s\n", game_.score(),
                  game_.lines(), record ? " (new best)" : "");
    return;
  }
  if (cleared > 0) {
    if (cleared == 4) {
      play(kBigClearSound, now_ms);
    } else {
      play(kClearSound, now_ms);
    }
    arcade::setLed(arcade::kLedFlash);
    flashing_ = true;
    flash_until_ms_ = now_ms + kFlashMs;
    Serial.printf("[blockfall] cleared %u line(s): score %" PRIu32 ", level %" PRIu32 "\n",
                  static_cast<unsigned>(cleared), game_.score(), game_.level());
  } else {
    play(kLandSound, now_ms);
  }
}

void BlockfallApp::update(const arcade::Input& in, uint32_t now_ms) {
  if (flashing_ && static_cast<int32_t>(now_ms - flash_until_ms_) >= 0) {
    flashing_ = false;
    if (game_.state() == Game::State::kPlaying) arcade::setLed(arcade::kLedNormal);
  }
  if (game_.state() == Game::State::kGameOver) {
    if ((in.knob_pressed || in.button_tap) && now_ms - over_ms_ >= arcade::kRestartGuardMs) {
      newGame(now_ms);
    }
    return;
  }
  for (int32_t i = 0; i < in.knob_steps; ++i) game_.moveRight();
  for (int32_t i = 0; i > in.knob_steps; --i) game_.moveLeft();
  if (in.knob_pressed) {
    game_.rotate();
  }
  if (in.button_tap) {
    game_.hardDrop();
    last_fall_ms_ = now_ms;
    onLock(now_ms);
    return;
  }
  const bool soft = in.button_held;
  const uint32_t interval = soft ? kSoftDropMs : game_.fallIntervalMs();
  if (now_ms - last_fall_ms_ >= interval) {
    last_fall_ms_ = now_ms;
    if (game_.tick(soft)) {
      onLock(now_ms);
    }
  }
}

bool BlockfallApp::needsRedraw() const { return game_.revision() != drawn_revision_ || best_ != drawn_best_; }

void BlockfallApp::draw() {
  blockfall::drawGame(game_, best_);
  drawn_revision_ = game_.revision();
  drawn_best_ = best_;
}
