#pragma once

#include <stdint.h>

#include "arcade_app.h"
#include "shooter_core.h"

/// The asteroid shooter for the arcade. Landscape.
///   Knob, turn              move the ship (clockwise = up)
///   Knob, press             fire
///   On-board button, hold   rapid fire while held
///   Knob press or tap       new game, from the game-over panel
class ShooterApp : public arcade::App {
 public:
  ShooterApp();

  const char* name() const override { return "SHOOTER"; }
  const char* scoreKey() const override { return "shooter"; }
  void enter(uint32_t now_ms) override;
  void update(const arcade::Input& in, uint32_t now_ms) override;
  bool needsRedraw() const override;
  void draw() override;

 private:
  void newGame(uint32_t now_ms);
  void onGameOver(uint32_t now_ms);
  void shoot(uint32_t now_ms);

  shooter::Game game_;
  uint32_t best_ = 0;
  bool new_best_ = false;
  uint32_t last_tick_ms_ = 0;
  uint32_t over_ms_ = 0;
  uint32_t frame_ = 0;  // ticks since the game started, for star scrolling and blinking
  uint32_t flash_until_ms_ = 0;
  bool flashing_ = false;
  uint32_t drawn_revision_ = UINT32_MAX;
  bool ui_dirty_ = true;
};
