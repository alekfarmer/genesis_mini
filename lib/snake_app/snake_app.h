#pragma once

#include <stdint.h>

#include "arcade_app.h"
#include "snake_core.h"

/// Snake for the arcade. Landscape.
///   Knob, turn             steer: each click turns the snake left/right
///   On-board button, tap   pause / resume
///   Knob press or tap      new game, from the game-over panel
class SnakeApp : public arcade::App {
 public:
  SnakeApp();

  const char* name() const override { return "SNAKE"; }
  const char* scoreKey() const override { return "snake"; }
  void enter(uint32_t now_ms) override;
  void update(const arcade::Input& in, uint32_t now_ms) override;
  bool needsRedraw() const override;
  void draw() override;

 private:
  void newGame(uint32_t now_ms);
  void onGameOver(uint32_t now_ms);

  snake::Game game_;
  uint32_t best_ = 0;
  bool new_best_ = false;
  bool paused_ = false;
  uint32_t last_step_ms_ = 0;
  uint32_t over_ms_ = 0;
  uint32_t drawn_revision_ = UINT32_MAX;
  bool ui_dirty_ = true;
};
