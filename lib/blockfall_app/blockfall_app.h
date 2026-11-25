#pragma once

#include <stdint.h>

#include "arcade_app.h"
#include "blockfall_core.h"

/// blockfall for the arcade. Portrait: hold the board with the screen tall.
///   Knob, turn              move the piece left / right
///   Knob, press             rotate
///   On-board button, tap    drop the piece to the bottom
///   On-board button, hold   fall faster while held
///   Knob press or tap       new game, from the game-over panel
class BlockfallApp : public arcade::App {
 public:
  BlockfallApp();

  const char* name() const override { return "BLOCKFALL"; }
  const char* scoreKey() const override { return "blockfall"; }
  panel::Orientation orientation() const override { return panel::Orientation::kPortrait; }
  void enter(uint32_t now_ms) override;
  void update(const arcade::Input& in, uint32_t now_ms) override;
  bool needsRedraw() const override;
  void draw() override;

 private:
  void newGame(uint32_t now_ms);
  void onLock(uint32_t now_ms);

  blockfall::Game game_;
  uint32_t best_ = 0;
  uint32_t last_fall_ms_ = 0;
  uint32_t over_ms_ = 0;
  uint32_t flash_until_ms_ = 0;
  bool flashing_ = false;
  uint32_t drawn_revision_ = UINT32_MAX;
  uint32_t drawn_best_ = UINT32_MAX;
};
